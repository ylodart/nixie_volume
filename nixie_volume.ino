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
#include <Oversample.h>

// I'm not sure sure what this does 
Oversample * sampler;

// define RTC module
RTC_DS3231 rtc;

// should the program set the clock time to the compile time? 1 for yes, 0 for no
const int SET_TIME = 1;

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
const int VOL_VALS[] = {0,32,96,128,160,224,256,288,352,384,416,480,512,544,608,640,672,736,768,800,864,896,928,992,1152,1312,1504,1664,1824,1984,2176,2336,2496,2656,2848,3008,3168,3328,3520,3680,3840,4000,4192,4352,4512,4672,4864,5024,5184,5344,5536,5696,5856,6016,6208,6368,6528,6688,6880,7040,7200,7360,7552,7712,7872,8352,8832,9280,9760,10240,10720,11168,11648,12128,12896,13664,14432,15200,15936,16704,17472,18240,19008,19776,20544,21312,22080,22816,23584,24352,25120,25888,26656,27424,28192,28960,29696,30464,31232,32000};

//14 bit
//const int VOL_VALS[] = {0,16,48,64,80,112,128,144,176,192,208,240,256,272,304,320,336,368,384,400,432,448,464,496,576,656,752,832,912,992,1088,1168,1248,1328,1424,1504,1584,1664,1760,1840,1920,2000,2096,2176,2256,2336,2432,2512,2592,2672,2768,2848,2928,3008,3104,3184,3264,3344,3440,3520,3600,3680,3776,3856,3936,4176,4416,4640,4880,5120,5360,5584,5824,6064,6448,6832,7216,7600,7968,8352,8736,9120,9504,9888,10272,10656,11040,11408,11792,12176,12560,12944,13328,13712,14096,14480,14848,15232,15616,16000};

//16 bit
//const unsigned int VOL_VALS[] = {0,64,192,256,320,448,512,576,704,768,832,960,1024,1088,1216,1280,1344,1472,1536,1600,1728,1792,1856,1984,2304,2624,3008,3328,3648,3968,4352,4672,4992,5312,5696,6016,6336,6656,7040,7360,7680,8000,8384,8704,9024,9344,9728,10048,10368,10688,11072,11392,11712,12032,12416,12736,13056,13376,13760,14080,14400,14720,15104,15424,15744,16704,17664,18560,19520,20480,21440,22336,23296,24256,25792,27328,28864,30400,31872,33408,34944,36480,38016,39552,41088,42624,44160,45632,47168,48704,50240,51776,53312,54848,56384,57920,59392,60928,62464,64000,65535};

//12 bit
//const int VOL_VALS[] = {0,4,12,16,20,28,32,36,44,48,52,60,64,68,76,80,84,92,96,100,108,112,116,124,144,164,188,208,228,248,272,292,312,332,356,376,396,416,440,460,480,500,524,544,564,584,608,628,648,668,692,712,732,752,776,796,816,836,860,880,900,920,944,964,984,1044,1104,1160,1220,1280,1340,1396,1456,1516,1612,1708,1804,1900,1992,2088,2184,2280,2376,2472,2568,2664,2760,2852,2948,3044,3140,3236,3332,3428,3524,3620,3712,3808,3904,4000,4096};

//10 bit
//const int VOL_VALS[] = {0, 1, 3, 4, 5, 7, 8, 9, 11, 12, 13, 15, 16, 17, 19, 20, 21, 23, 24, 25, 27, 28, 29, 31, 36, 41, 47, 52, 57, 62, 68, 73, 78, 83, 89, 94, 99, 104, 110, 115, 120, 125, 131, 136, 141, 146, 152, 157, 162, 167, 173, 178, 183, 188, 194, 199, 204, 209, 215, 220, 225, 230, 236, 241, 246, 261, 276, 290, 305, 320, 335, 349, 364, 379, 403, 427, 451, 475, 498, 522, 546, 570, 594, 618, 642, 666, 690, 713, 737, 761, 785, 809, 833, 857, 881, 905, 928, 952, 976, 1000, 1024};

//old
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

  // initialize sampler
  sampler = new Oversample(VOL_IN, 15);

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
    //for (k = 0; k < numRdgs; k++) {
    //  delayMicroseconds(260);              // max conversion time according to atmega328 datasheet
    //  avgRdg += analogRead(VOL_IN);
    //}
    //reading = avgRdg / numRdgs;
    //avgRdg = 0;

    int single = analogRead(VOL_IN);
    double oversampled = sampler->read();
    long reading = sampler->readDecimated();    

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
