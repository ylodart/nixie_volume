/*
  nixie tube volume display program for arduino nano

  reads position of ganged logarithmic volume pot and displays linear value (uses averaging to prevent jumping back and forth between two values)
  gets codes from apple remote and controls motorized volume 
  to program, select board: Nano and processor: Atmega328P (old bootloader)

  J. Scott
  last update: 2/13/2022
*/

#include <IRremote.h>
#include <RTClib.h>

// define RTC module
RTC_DS3231 rtc;

// should the program set the clock time to the compile time? 1 for yes, 0 for no
const int SET_TIME = 0;

// multiplexer input arduino output pins
const int TENS_A = 8;
const int TENS_B = 4;
const int TENS_C = 5;
const int TENS_D = 9;

const int ONES_A = 3;
const int ONES_B = 7;
const int ONES_C = 6;
const int ONES_D = 2;

// Volume PCB remote OFF/ON button input
const int IR = 12;
const int MOTOR_A = 10;
const int MOTOR_B = 11;

// ADC input pins
const int VOL_IN = A6;

// volume ADC count array that maps ADC counts to display numbers (due to logarithmic pot)
const int VOL_VALS[] = {0, 1, 3, 4, 5, 7, 8, 9, 11, 12, 13, 15, 16, 17, 19, 20, 21, 23, 24, 25, 27, 28, 29, 31, 36, 41, 47, 52, 57, 62, 68, 73, 78, 83, 89, 94, 99, 104, 110, 115, 120, 125, 131, 136, 141, 146, 152, 157, 162, 167, 173, 178, 183, 188, 194, 199, 204, 209, 215, 220, 225, 230, 236, 241, 246, 261, 276, 290, 305, 320, 335, 349, 364, 379, 403, 427, 451, 475, 498, 522, 546, 570, 594, 618, 642, 666, 690, 713, 737, 761, 785, 809, 833, 857, 881, 905, 928, 952, 976, 1000, 1024};
//const int VOL_VALS[] = {0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38, 40, 42, 44, 46, 48, 50, 52, 54, 57, 62, 68, 73, 78, 83, 89, 94, 99, 104, 110, 115, 120, 125, 131, 136, 141, 146, 152, 157, 162, 167, 173, 178, 183, 188, 194, 199, 204, 209, 215, 220, 225, 230, 236, 241, 246, 261, 276, 290, 305, 320, 335, 349, 364, 379, 403, 427, 451, 475, 498, 522, 546, 570, 594, 618, 642, 666, 690, 713, 737, 761, 785, 809, 833, 857, 881, 905, 928, 952, 976, 1000, 1024};
//const int VOL_VALS[] = {0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,35,40,45,50,55,60,65,70,75,80,85,90,95,100,105,110,115,120,125,130,135,140,145,150,155,160,165,170,175,180,185,190,195,200,205,210,215,220,225,230,235,240,245,260,275,290,305,320,335,350,365,380,400,420,440,460,480,500,520,540,560,580,600,620,640,660,680,700,720,740,760,780,800,820,840,860,880,900,920,940,960,980,1000,1024};

// declare time constants
const int MOTOR_MS = 20; //this is the number of milliseconds the motor will turn before it checks for a new IR input or turns the motor off -- effectively the resolution of a single remote volume button press

// how many repeats of menu button before entering fun mode
const int FUN_COUNTS = 15;

// apple remote IR codes
const long UP_BTN = 0xDE0B87EE;
const long DOWN_BTN = 0xDE0D87EE;
const int REP_BTN = 0x0;
const long MENU_BTN = 0xDE0287EE;
const long PLAY_BTN1 = 0xDE5E87EE;
const long PLAY_BTN2 = 0xDE0487EE;
const long CENTER_BTN1 = 0xDE5D87EE;
const long CENTER_BTN2 = 0xDE0487EE;
const long RIGHT_BTN = 0xDE0787EE;
const long LEFT_BTN = 0xDE0887EE;

// times (ms) for clock display state machine
const int MIN_HR_MILS = 1300;    // time btwn displaying minute and hour
const int DIGIT_MILS = 1300;    // time hr/min will display
const int HR_MIN_MILS = 200;    // time btwn displaying hour and minute

// configure arduino IO
void setup() {

  // declare the multiplexer pins as output
  pinMode(TENS_A, OUTPUT);
  pinMode(TENS_B, OUTPUT);
  pinMode(TENS_C, OUTPUT);
  pinMode(TENS_D, OUTPUT);

  pinMode(ONES_A, OUTPUT);
  pinMode(ONES_B, OUTPUT);
  pinMode(ONES_C, OUTPUT);
  pinMode(ONES_D, OUTPUT);

  // set display to blank
  digitalWrite(TENS_A, HIGH);
  digitalWrite(TENS_B, HIGH);
  digitalWrite(TENS_C, HIGH);
  digitalWrite(TENS_D, HIGH);

  digitalWrite(ONES_A, HIGH);
  digitalWrite(ONES_B, HIGH);
  digitalWrite(ONES_C, HIGH);
  digitalWrite(ONES_D, HIGH);

  //analogReference(INTERNAL);

  // initialize serial for debug
  Serial.begin(9600);

  // initialize IR receiver
  IrReceiver.begin(IR, DISABLE_LED_FEEDBACK);

  // set motor control pins low
  pinMode(MOTOR_A ,OUTPUT);
  pinMode(MOTOR_B ,OUTPUT);
  digitalWrite(MOTOR_A, LOW);
  digitalWrite(MOTOR_B, LOW);

  // initialize RTC
  rtc.begin();

  // set clock
  if (SET_TIME) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }
}

void loop() {

  // initialize variables
  int reading = 0;                // volume pot ADC reading
  int numRdgs = 300;              // number of volume pot readings to average
  int j;                          // index of volume array (also volume setting number to display)
  int oldj = 200;                 // last index of volume array - set to impossible value at first to force display update
  int k;                          // volume ADC reading counter
  unsigned long avgRdg = 0;       // average reading
  int lastRdg = 0;                // last volume ADC reading
  int upd = 1;                    // update volume display flag
  
  long lastCode;                  // last IR code received
  long code;                      // current IR code
  long milliCount = 0;            // millisecond counter for stopping vol motor if remote codes stop coming in 
  int funCount = 0;               // fun button repeat counter (must hold down a button to enter fun mode)
  
  int clockCount;                 // millisecond counter for timing clock display
  int clockMode = 0;              // clock mode flag
  int millisNow;                  // var for capturing current millisecond timer value
  int hr;                         // variable for storing hour so we can convert it from 24 hr to 12 hr mode

  // vars below convert clock time constants into accumulating time for state machine
  int clockTime1 = MIN_HR_MILS;                                    
  int clockTime2 = MIN_HR_MILS + DIGIT_MILS;
  int clockTime3 = MIN_HR_MILS + DIGIT_MILS + HR_MIN_MILS;
  int clockTime4 = MIN_HR_MILS + DIGIT_MILS + HR_MIN_MILS + DIGIT_MILS;

  // display all numbers quickly (for fun! and to prevent cathode poisoning)
  for (int i = 0; i < 10; i++)
  {
    dispNum(i * 11, 1);
    delay(120);
  }
  dispBlank();
  delay(50);

  // I don't know why the hell this is necessary but the first 1000 readings are garbage. They ramp up from zero to the proper value, no matter what.
  for (int l = 0; l < 1000; l++) {
    analogRead(VOL_IN);
    delayMicroseconds(100);
  }

  // main loop
  while (1) {

    // get average reading
    for (k = 0; k < numRdgs; k++) {
      delayMicroseconds(260);              // max conversion time according to atmega328 datasheet
      avgRdg += analogRead(VOL_IN);
    }
    reading = avgRdg / numRdgs;
    avgRdg = 0;

    // set the update reading flag if we have a new reading
    if (abs(reading - lastRdg) > 0) {
      upd = 1;
      lastRdg = reading;
    }

    // if update flag is set, find the value to display
    if (upd) {
      for (j = 0; j < 100; j++) {    
        if (VOL_VALS[j] > reading) {
          break;
        }
      }

      // update display if display number has changed
      if (j != oldj && !clockMode) {
        dispNum(j - 1, 1);
        oldj = j;
        
        // reset update flag
        upd = 0;
      }
    }

    // check for remote codes and act accordingly
    if (IrReceiver.decode()) {
      milliCount = millis();
      code = IrReceiver.decodedIRData.decodedRawData;
      if(code != REP_BTN || code == PLAY_BTN2) {  // play button generates 2 codes so ignore the second one
        lastCode = code;
      }
      else {
        if (lastCode == UP_BTN) {
          goUp();
        }
        if (lastCode == DOWN_BTN) {
          goDown();
        }
        if (lastCode == PLAY_BTN1) {
          funCount += 1;
          if (funCount == FUN_COUNTS) {
            funCount = 0;
            funDisp();
            oldj = 200; // force a vol display update
            upd = 1;
          }
        }
      }
      if(code == UP_BTN)
        goUp();
      if(code == DOWN_BTN)
        goDown();
      if(code == MENU_BTN) {
        if (clockMode) {
          clockMode = 0;
          oldj = 200; // force a vol display update
          upd = 1;
        }
        else {
          clockMode = 1;
          clockCount = millis();  
        }          
      }
      IrReceiver.resume();
    }

    // stop motor if we haven't received an IR code
    if (millis() - milliCount > MOTOR_MS) {
      stopMotor();
    }

    // get current time
    DateTime now = rtc.now();

    // state machine for displaying hours and minutes
    if (clockMode) {
      millisNow = millis() - clockCount;
      if (millisNow < clockTime1) {
        dispBlank();
      }
      if (millisNow > clockTime1 && millisNow < clockTime2) {
        // convert 24 hr time to 12 hr time
        hr = now.hour();
        if (hr > 12)
          hr = hr - 12;
        dispNum(hr, 0);
      }
      if (millisNow > clockTime2 && millisNow < clockTime3) {
        dispBlank();
      }
      if (millisNow > clockTime3 && millisNow < clockTime4) {
        dispNum(now.minute(), 1);
      }
      if (millisNow > clockTime4) {
        clockCount = millis();
      }
    }

  } // main loop
} // void loop

// tell the motor to go up
void goUp() {
  digitalWrite(MOTOR_A, HIGH); 
  digitalWrite(MOTOR_B, LOW); 
}

// tell the motor to go down
void goDown() {
  digitalWrite(MOTOR_B, HIGH);
  digitalWrite(MOTOR_A, LOW); 
}

// stop the motor
void stopMotor() {  
  digitalWrite(MOTOR_A, LOW); 
  digitalWrite(MOTOR_B, LOW); 
}

// blank display
void dispBlank () {
  digitalWrite(TENS_A, HIGH);
  digitalWrite(TENS_B, HIGH);
  digitalWrite(TENS_C, HIGH);
  digitalWrite(TENS_D, HIGH);

  digitalWrite(ONES_A, HIGH);
  digitalWrite(ONES_B, HIGH);
  digitalWrite(ONES_C, HIGH);
  digitalWrite(ONES_D, HIGH);
}

// display given number
void dispNum (int num, int dispZero) {

  // get ones and tens digits
  int tens = num / 10;
  int ones = num % 10;

  // map display number to multiplexer codes
  if (dispZero || tens > 0) {
    switch (tens) {
      case 0:
        dispOut(3, 10);
        break;
      case 1:
        dispOut(7, 10);
        break;
      case 2:
        dispOut(4, 10);
        break;
      case 3:
        dispOut(2, 10);
        break;
      case 4:
        dispOut(9, 10);
        break;
      case 5:
        dispOut(0, 10);
        break;
      case 6:
        dispOut(5, 10);
        break;
      case 7:
        dispOut(1, 10);
        break;
      case 8:
        dispOut(6, 10);
        break;
      case 9:
        dispOut(8, 10);
        break;
    }
  }

  switch (ones) {
    case 0:
      dispOut(8, 1);
      break;
    case 1:
      dispOut(2, 1);
      break;
    case 2:
      dispOut(3, 1);
      break;
    case 3:
      dispOut(1, 1);
      break;
    case 4:
      dispOut(0, 1);
      break;
    case 5:
      dispOut(4, 1);
      break;
    case 6:
      dispOut(7, 1);
      break;
    case 7:
      dispOut(6, 1);
      break;
    case 8:
      dispOut(9, 1);
      break;
    case 9:
      dispOut(5, 1);
      break;
  }
}

// display given digits
void dispOut (int num, int digit) {

  // multiplexer inputs for tens digit
  int a = TENS_A;
  int b = TENS_B;
  int c = TENS_C;
  int d = TENS_D;

  // multiplexer inputs for ones digit
  if (digit == 1) {
    a = ONES_A;
    b = ONES_B;
    c = ONES_C;
    d = ONES_D;
  }

  // write multiplexer inputs
  switch (num) {
    case 0:
      digitalWrite(a, LOW); //0
      digitalWrite(b, LOW);
      digitalWrite(c, LOW);
      digitalWrite(d, LOW);
      break;
    case 1:
      digitalWrite(a, HIGH); //1
      digitalWrite(b, LOW);
      digitalWrite(c, LOW);
      digitalWrite(d, LOW);
      break;
    case 2:
      digitalWrite(a, LOW); //2
      digitalWrite(b, HIGH);
      digitalWrite(c, LOW);
      digitalWrite(d, LOW);
      break;
    case 3:
      digitalWrite(a, HIGH); //3
      digitalWrite(b, HIGH);
      digitalWrite(c, LOW);
      digitalWrite(d, LOW);
      break;
    case 4:
      digitalWrite(a, LOW); //4
      digitalWrite(b, LOW);
      digitalWrite(c, HIGH);
      digitalWrite(d, LOW);
      break;
    case 5:
      digitalWrite(a, HIGH); //5
      digitalWrite(b, LOW);
      digitalWrite(c, HIGH);
      digitalWrite(d, LOW);
      break;
    case 6:
      digitalWrite(a, LOW); //6
      digitalWrite(b, HIGH);
      digitalWrite(c, HIGH);
      digitalWrite(d, LOW);
      break;
    case 7:
      digitalWrite(a, HIGH); //7
      digitalWrite(b, HIGH);
      digitalWrite(c, HIGH);
      digitalWrite(d, LOW);
      break;
    case 8:
      digitalWrite(a, LOW); //8
      digitalWrite(b, LOW);
      digitalWrite(c, LOW);
      digitalWrite(d, HIGH);
      break;
    case 9:
      digitalWrite(a, HIGH); //9
      digitalWrite(b, LOW);
      digitalWrite(c, LOW);
      digitalWrite(d, HIGH);
      break;
  }
}

// do a fun display!
int funDisp() {
  int i;
  int dly = 1300;
  int randoNums = 15;
  int randoDly = 50;
  
  for (i=0; i < randoNums; i++) {
    dispNum(random(100), 1);
    delay(randoDly);
  }
  
  dispBlank();
  delay(dly*2);
  dispNum(4, 0);
  delay(dly);
  dispNum(8, 0);
  delay(dly);
  dispNum(15, 1);
  delay(dly);
  dispNum(16, 1);
  delay(dly);
  dispNum(23, 1);
  delay(dly);
  dispNum(42, 1);
  
  delay(dly);
  for (i=0; i < randoNums; i++) {
    dispNum(random(100), 1);
    delay(randoDly);
  }

  dispBlank();
  delay(dly);
}
