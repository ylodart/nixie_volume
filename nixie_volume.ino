/*
  nixie tube volume display program for arduino nano

  reads position of ganged logarithmic volume pot and displays linear value (uses averaging and hysteresis to prevent jumping back and forth between two values)
  reads position of select input switch and displays value for a short period of time on startup and each time the switch is changed
  activates a relay a short while after startup (this is used to prevent the motorized volume control microcontroller from performing its startup routine, which moves the pot to a certain position)

  J. Scott
  5/4/2020
*/

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
const int OFFON = 10;

// ADC input pins
const int VOL_IN = A6;
const int SEL_IN = A0;

// relay coil pin
const int RELAY = 12;

// volume ADC count array that maps ADC counts to display numbers (due to logarithmic pot)
const int VOL_VALS[] = {0, 1, 3, 4, 5, 7, 8, 9, 11, 12, 13, 15, 16, 17, 19, 20, 21, 23, 24, 25, 27, 28, 29, 31, 36, 41, 47, 52, 57, 62, 68, 73, 78, 83, 89, 94, 99, 104, 110, 115, 120, 125, 131, 136, 141, 146, 152, 157, 162, 167, 173, 178, 183, 188, 194, 199, 204, 209, 215, 220, 225, 230, 236, 241, 246, 261, 276, 290, 305, 320, 335, 349, 364, 379, 403, 427, 451, 475, 498, 522, 546, 570, 594, 618, 642, 666, 690, 713, 737, 761, 785, 809, 833, 857, 881, 905, 928, 952, 976, 1000, 1024};

// ADC counts for select inputs
const int SEL_IN1 = 138;
const int SEL_IN2 = 334;
const int SEL_IN3 = 458;
const int SEL_IN4 = 544;
const int SEL_IN5 = 650;

// relay ms timer variable
unsigned long relayMils = 0;

// declare time constants
const int SEL_TIME = 1250;      // time to display select
const int STARTUP_TIME = 500;   // time to wait after startup before allowing volume display to be updated (this is necessary because the RC on the volume ADC input causes a slow updating of the display that clears the display of the select switch value)
const int RELAY_TIME = 14000;    // time to wait after startup before firing relay

// configure arduino IO
void setup() {

  // declare relay pin as output
  pinMode(RELAY, OUTPUT);

  // declare select pin as input with pullup
  pinMode(SEL_IN, INPUT_PULLUP);

  // declare offon pin as input with pullup
  pinMode(OFFON, INPUT_PULLUP);

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

  // initialize serial for debug
  Serial.begin(9600);

  // record current ms timer value
  relayMils = millis();

  //analogReference(3);
}

void loop() {

  // initialize variables
  int reading = 0;                // volume pot ADC reading
  int numRdgs = 200;              // number of volume pot readings to average
  int j;                          // index of volume array (also volume setting number to display)
  int oldj = 0;                   // last index of volume array
  int k;                          // volume ADC reading counter
  unsigned long avgRdg;           // average reading
  int dir = 1; //down=0 up=1      // direction volume pot is being turned
  int lastRdg = 0;                // last volume ADC reading
  int upd = 0;                    // update volume display flag
  int dirChgCount = 0;            // counter for number of times volume pot has changed direction
  int relaySet = 0;               // relay set flag
  int oldSelect = 0;              // last value of select switch
  int sel;                        // select switch value
  unsigned long selMils = 0;      // time select switch was set
  unsigned long startupMils = 0;  // time since startup
  int selSet = 0;                 // select switch set flag
  int startup = 1;                // startup flag
  
  int offonState = digitalRead(OFFON);  // state of offon

  // display all numbers quickly (for fun!)
  for (int i = 0; i < 10; i++)
  {
    dispNum(i * 11);
    delay(100);
  }

  // main loop
  while (1) {

    if (digitalRead(OFFON) != offonState) {
      //Serial.println("we got here");
      funDisp();
      offonState = digitalRead(OFFON);
      upd = 1;
    }

    // set the relay if it's time
    if (!relaySet) {
      if (millis() - relayMils > RELAY_TIME) {
        digitalWrite(RELAY, HIGH);
      }
    }

    // get average reading
    for (k = 0; k < numRdgs; k++) {
      delayMicroseconds(260);              // max conversion time according to atmega328 datasheet
      avgRdg += analogRead(VOL_IN);
    }
    reading = avgRdg / numRdgs;
    avgRdg = 0;

    // update display with hysteresis if we've changed directions twice
    if (reading - lastRdg > 0) {          // going up
      if (dir == 0) {                     // last direction was down, so we changed directions
        dirChgCount += 1;                 // increase direction change counter
        if (dirChgCount > 1) {            // we changed directions more than once
          if (reading - lastRdg != 1) {   // we changed by more than one count
            upd = 1;                      // set update flag
            dir = 1;                      // set direction to up
            lastRdg = reading;            // record current reading as last reading
            dirChgCount = 0;              // reset direction change count
          }
        } else {                          // we changed directions less than twice
          upd = 1;
          dir = 1;
          lastRdg = reading;
        }
      } else {                            // continuing down
        upd = 1;
        lastRdg = reading;
      }
    }
    else if (reading - lastRdg < 0) {     // going down
      if (dir == 1) {                     // last direction was up, so we changed directions
        dirChgCount += 1;
        if (dirChgCount > 1) {
          if (reading - lastRdg != 1) {
            upd = 1;
            dir = 0;                      // set direction to down
            lastRdg = reading;
            dirChgCount = 0;
          }
        } else {                          // we changed directions less than twice
          upd = 1;
          dir = 0;
          lastRdg = reading;
        }
      } else {                            // continuing up
        upd = 1;
        lastRdg = reading;
      }
    }

    // update display if update flag is set
    if (upd) {
      for (j = 0; j < 100; j++) {     // find display number in array for given counts
        if (VOL_VALS[j] > reading) {
          break;
        }
      }

      // update display if display number has changed and we're not in startup
      if (j != oldj && !startup) {
        dispNum(j - 1);
        oldj = j;
      }

      // reset update flag
      upd = 0;
    }

    // clear select display if it's time
    if (selSet) {
      if (millis() - selMils > SEL_TIME) {
        upd = 1;
        oldj = 100;
        selSet = 0;
      }
    }

    // display input selection if it's changed
    sel = getSelect();
    if (sel != oldSelect && sel != 0) {
      dispSel(sel);
      selMils = millis();
      startupMils = millis();
      oldSelect = sel;
      selSet = 1;
    }

    // if startup time has expired, clear startup flag
    if (startup) {
      if (millis() - startupMils > STARTUP_TIME) {
        startup = 0;
      }
    }

  } // main loop
} // void loop

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

// display select input number
void dispSel (int num) {

  switch (num) {
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
  }

  // blank the 10s digit
  dispOut(15, 10);
}

// display given number
void dispNum (int num) {

  // get ones and tens digits
  int tens = num / 10;
  int ones = num % 10;

  // map display number to multiplexer codes
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
      digitalWrite(a, LOW);
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
    case 15:
      digitalWrite(a, HIGH); //15
      digitalWrite(a, HIGH);
      digitalWrite(c, HIGH);
      digitalWrite(d, HIGH);
  }
}

// get the select switch input number
int getSelect () {

  // read the select switch
  int selRdg = analogRead(SEL_IN);
  delayMicroseconds(260); // max conversion time according to atmega328 datasheet

  // return the select switch value based on the reading
  if (selRdg < SEL_IN1) {
    return 1;
  }
  else if (selRdg > SEL_IN1 && selRdg < SEL_IN2) {
    return 2;
  }
  else if (selRdg > SEL_IN2 && selRdg < SEL_IN3) {
    return 3;
  }
  else if (selRdg > SEL_IN3 && selRdg < SEL_IN4) {
    return 4;
  }
  else if (selRdg > SEL_IN4 && selRdg < SEL_IN5) {
    return 5;
  } else {
    return 0;
  }
}

// do a fun display!
int funDisp() {
  int i;
  dispBlank();
  delay(800);
  //for (i=0; i < 50; i++) {
  //  dispNum(random(100));
  //  delay(100);
  //} 
  dispNum(16);
  delay(1000);
  dispNum(12);
  delay(1000);
  dispBlank();
  delay(3400);
  dispOut(2, 1);
  delay(500);
  dispOut(7, 1);
  delay(500);
  dispOut(2, 1);
  delay(500);
  dispOut(3, 1);
  delay(1000);
  dispBlank();
  delay(3100); 
  dispNum(16);
  delay(1000);
  dispNum(12);
  delay(1000);
  dispBlank();
  delay(3400);
  dispOut(2, 1);
  delay(700);
  dispOut(7, 1);
  delay(700);
  dispOut(2, 1);
  delay(700);
  dispOut(3, 1);
  delay(1000);
  dispBlank();
  delay(1000); 
}
