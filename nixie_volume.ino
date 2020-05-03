/*
  nixie tube display test program
  J. Scott 3/26/20
*/

const int TENS_A = 8;
const int TENS_B = 4;
const int TENS_C = 5;
const int TENS_D = 9;

const int ONES_A = 3;
const int ONES_B = 7;
const int ONES_C = 6;
const int ONES_D = 2;

const int VOL_IN = 6;

const int VOL_VALS[] = {0,1,3,4,5,7,8,9,11,12,13,15,16,17,19,20,21,23,24,25,27,28,29,31,36,41,47,52,57,62,68,73,78,83,89,94,99,104,110,115,120,125,131,136,141,146,152,157,162,167,173,178,183,188,194,199,204,209,215,220,225,230,236,241,246,261,276,290,305,320,335,349,364,379,403,427,451,475,498,522,546,570,594,618,642,666,690,713,737,761,785,809,833,857,881,905,928,952,976,1000,1024};

void setup() {
  // declare the ledPin as an OUTPUT:
  pinMode(TENS_A, OUTPUT);
  pinMode(TENS_B, OUTPUT);
  pinMode(TENS_C, OUTPUT);
  pinMode(TENS_D, OUTPUT);
  
  pinMode(ONES_A, OUTPUT);
  pinMode(ONES_B, OUTPUT);
  pinMode(ONES_C, OUTPUT);
  pinMode(ONES_D, OUTPUT);  

  digitalWrite(TENS_A, HIGH);
  digitalWrite(TENS_B, HIGH);
  digitalWrite(TENS_C, HIGH);
  digitalWrite(TENS_D, HIGH);
  
  digitalWrite(ONES_A, HIGH);
  digitalWrite(ONES_B, HIGH);
  digitalWrite(ONES_C, HIGH);
  digitalWrite(ONES_D, HIGH);

  Serial.begin(9600);
}

void loop() {

  // initialize variables
  int reading = 0;
  int numRdgs = 200;
  int j;
  int oldj = 0;
  int k;
  unsigned long avgRdg;
  int dir = 1; //down=0 up=1
  int lastRdg = 0;
  int upd = 0;

  // display all numbers quickly 
  for (int i = 0; i < 10; i++)
    { 
      dispNum(i*11);
      delay(100);  
    }

  // main loop
  while(1) {

    // get average reading
    for (k = 0; k < numRdgs; k++) {
      avgRdg += analogRead(VOL_IN);
      delayMicroseconds(100);        
    }    
    reading = avgRdg / numRdgs;
    avgRdg = 0;      

    // update display with hysteresis
    if (reading - lastRdg > 0) {          // going up
      if (dir == 0) {                     // last direction was down, so we changed directions
        if (reading - lastRdg != 1) {     // we changed by more than one count
          upd = 1;                        // set update flag  
          dir = 1;                        // set direction to up
          lastRdg = reading;
        }
      } else {                            // continuing down
        upd = 1;
        lastRdg = reading;
      }
    }
    else if (reading - lastRdg < 0) {     // going down
      if (dir == 1) {                     // last direction was up, so we changed directions
        if (reading - lastRdg != 1) {     
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

      // update display if display number has changed
      if (j != oldj) {                
        dispNum(j-1);  
        oldj = j;          
      }

      // reset update flag
      upd = 0;
    }

      
      
  }
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
void dispNum (int num) {
  
  // get ones and tens digits
  int tens = num / 10;
  int ones = num %10;

  // map display number to multiplexer codes
  switch (tens) {
    case 0:
      dispOut(3,10);
      break;
    case 1:
      dispOut(7,10);
      break;
    case 2:
      dispOut(4,10);
      break;
    case 3:
      dispOut(2,10);
      break;
    case 4:
      dispOut(9,10);
      break;
    case 5:
      dispOut(0,10);
      break;
    case 6:
      dispOut(5,10);
      break;
    case 7:
      dispOut(1,10);
      break;
    case 8:
      dispOut(6,10);
      break;        
    case 9:
      dispOut(8,10);
      break;
  }

  switch (ones) {
    case 0:
      dispOut(8,1);
      break;
    case 1:
      dispOut(2,1);
      break;
    case 2:
      dispOut(3,1);
      break;
    case 3:
      dispOut(1,1);
      break;
    case 4:
      dispOut(0,1);
      break;
    case 5:
      dispOut(4,1);
      break;
    case 6:
      dispOut(7,1);
      break;
    case 7:
      dispOut(6,1);
      break;
    case 8:
      dispOut(9,1);
      break;        
    case 9:
      dispOut(5,1);
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
  }  
}
