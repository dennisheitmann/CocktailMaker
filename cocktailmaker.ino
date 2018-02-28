#include <SevenSegmentTM1637.h>
#include <HX711_ADC.h>
#include <Encoder.h>

#define ENCODER_OPTIMIZE_INTERRUPTS

//HX711 constructor (dout pin, sck pin)
const int HX_DOUT = 6;
const int HX_SCK = 5;
HX711_ADC LoadCell(HX_DOUT, HX_SCK);

const byte PIN_CLK = 12;   // define CLK pin (any digital pin)
const byte PIN_DIO = 11;   // define DIO pin (any digital pin)
SevenSegmentTM1637 display(PIN_CLK, PIN_DIO);

const int MOT_1 = 2;
int MOT_1_state = LOW;
const int MOT_2 = 3;
int MOT_2_state = LOW;
const int MOT_3 = 4;
int MOT_3_state = LOW;

const int ENC_SW = 8;
int buttonState = LOW;       // the current reading from the input pin

const int ENC_DT = 9;
const int ENC_CLK = 10;
Encoder myEnc(ENC_DT, ENC_CLK);
int oldPosition;

// Dosiermengen
int d_1_g = 24;
int d_2_g = 144;
int d_3_g = 12;
int dosage;

// Messdauer Waagenwert
int timespan = 500;

// minimales Bechergewicht
int glasgewicht = 5;

// Dosierungstimer
long dosage_timer = 0;
long max_dosage = 180;

// menu
String menustr = "COCKTAIL";
String oldmenustr = "COCKTAIL";

// Errorvariable
int errorvalue = 0;

void setup() {
  Serial.begin(9600);
  pinMode(13, OUTPUT);
  pinMode(MOT_1, OUTPUT);
  pinMode(MOT_2, OUTPUT);
  pinMode(MOT_3, OUTPUT);
  Serial.println("Wait...");
  display.begin();            // initializes the display
  display.setBacklight(100);  // set the brightness to 100 %
  display.print("INIT");      // display INIT on the display
  LoadCell.begin();
  long stabilisingtime = 2000; // tare preciscion can be improved by adding a few seconds of stabilising time
  LoadCell.start(stabilisingtime);
  LoadCell.setCalFactor(425.0); // user set calibration factor (float)
  Serial.println("Startup + tare is complete");
}

int button() {
  int reading = digitalRead(ENC_SW);
  if (reading == HIGH) {
    Serial.println("Button pressed!");
    display.blink();
    delay(1000);
    return reading;
  }
}

void okbutton() {
  while (true) {
    int reading = digitalRead(ENC_SW);
    if (reading == HIGH) {
      Serial.println("Button pressed!");
      display.blink();
      delay(1000);
      return reading;
    }
  }
}

int encoder() {
  int newPosition = myEnc.read();
  return newPosition;
}

int balance(int timespan) {
  long t = millis() + timespan;
  while (true) {
    LoadCell.update();
    //get smoothed value from data set + current calibration factor
    if (millis() > t) {
      float i = LoadCell.getData();
      return (int)i;
    }
  }
}

void tarebalance() {
  menustr = "TARE";
  display.clear();
  display.print(menustr);
  Serial.println(menustr);
  long t = millis() + 1500;
  while (true) {
    LoadCell.update();
    //get smoothed value from data set + current calibration factor
    if (millis() > t) {
      LoadCell.tareNoDelay();
      break;
    }
  }
  // nach der Tarierung Werte updaten
  Serial.println(balance(1500));
}

void loop() {
  errorvalue = 0;
  menustr = "COCKTAIL";
  oldmenustr = "COCKTAIL";
  Serial.println(menustr);
  while (true) {
    if (millis() % 2000 < 1000) {
      menustr = "COCK";
    } else {
      menustr = "TAIL";
    }
    if (menustr != oldmenustr) {
      display.clear();
      display.print(menustr);
      oldmenustr = menustr;
    }
    buttonState = button();
    if (buttonState == HIGH) {
      buttonState = LOW;
      break;
    }
    serial_purge();
  }

  menustr = "S--1";
  display.print(menustr);
  Serial.println(menustr);
  okbutton();

  myEnc.write(d_1_g);
  while (true) {
    d_1_g = encoder();
    //myEnc.write(d_1_g);
    display.clear();
    display.print(d_1_g);
    Serial.println(d_1_g);
    buttonState = button();
    if (buttonState == HIGH) {
      buttonState = LOW;
      break;
    }
  }

  menustr = "S--2";
  display.print(menustr);
  Serial.println(menustr);
  okbutton();

  myEnc.write(d_2_g);
  while (true) {
    d_2_g = encoder();
    //myEnc.write(d_2_g);
    display.clear();
    display.print(d_2_g);
    Serial.println(d_2_g);
    buttonState = button();
    if (buttonState == HIGH) {
      buttonState = LOW;
      break;
    }
  }

  menustr = "S--3";
  display.print(menustr);
  Serial.println(menustr);
  okbutton();

  myEnc.write(d_3_g);
  while (true) {
    d_3_g = encoder();
    myEnc.write(d_3_g);
    display.clear();
    display.print(d_3_g);
    Serial.println(d_3_g);
    buttonState = button();
    if (buttonState == HIGH) {
      buttonState = LOW;
      break;
    }
  }

  int balancevalue;
  int balancevalue_old = balance(timespan);
  balancevalue_old = balance(timespan);
  menustr = "GLAS";
  display.print(menustr);
  Serial.println(menustr);
  okbutton();

  long wait = millis() + 2500;
  while (true) {
    balancevalue = balance(timespan);
    display.clear();
    display.print(balancevalue);
    Serial.println(balancevalue);
    if (millis() > wait) {
      if (balancevalue > (balancevalue_old + glasgewicht)) {
        break;
      }
    }
  }

  menustr = "DOSE";
  display.print(menustr);
  Serial.println(menustr);
  delay(1000);

  // dose 1
  tarebalance();
  // der erste Aufruf von Balance nach Tarierung ist noch ungenau
  Serial.println(balance(2000));
  dosage = 0;
  dosage_timer = millis() + (max_dosage * 1000);
  while (true) {
    if (errorvalue == 1) {
      break;
    }
    dosage = balance(timespan);
    digitalWrite(MOT_1, HIGH);
    display.clear();
    display.print(dosage);
    Serial.print("1 ");
    Serial.print(d_1_g);
    Serial.print(" ");
    Serial.println(dosage);
    if (dosage > d_1_g) {
      digitalWrite(MOT_1, LOW);
      break;
    }
    if (dosage < -(glasgewicht)) {
      digitalWrite(MOT_1, LOW);
      errorvalue = 1;
      display.clear();
      menustr = "STOP";
      display.print(menustr);
      Serial.println(menustr);
      break;
    }
    if (millis() > dosage_timer) {
      digitalWrite(MOT_1, LOW);
      errorvalue = 1;
      display.clear();
      menustr = "STOP";
      display.print(menustr);
      Serial.println(menustr);
      break;
    }
  }

  // dose 2
  tarebalance();
  // der erste Aufruf von Balance nach Tarierung ist noch ungenau
  Serial.println(balance(2000));
  dosage = 0;
  dosage_timer = millis() + (max_dosage * 1000);
  while (true) {
    if (errorvalue == 1) {
      break;
    }
    dosage = balance(timespan);
    digitalWrite(MOT_2, HIGH);
    display.clear();
    display.print(dosage);
    Serial.print("2 ");
    Serial.print(d_2_g);
    Serial.print(" ");
    Serial.println(dosage);
    if (dosage > d_2_g) {
      digitalWrite(MOT_2, LOW);
      break;
    }
    if (dosage < -(glasgewicht)) {
      digitalWrite(MOT_2, LOW);
      errorvalue = 1;
      display.clear();
      menustr = "STOP";
      display.print(menustr);
      Serial.println(menustr);
      break;
    }
    if (millis() > dosage_timer) {
      digitalWrite(MOT_2, LOW);
      errorvalue = 1;
      display.clear();
      menustr = "STOP";
      display.print(menustr);
      Serial.println(menustr);
      break;
    }
  }

  // dose 3
  tarebalance();
  // der erste Aufruf von Balance nach Tarierung ist noch ungenau
  Serial.println(balance(2000));
  dosage = 0;
  dosage_timer = millis() + (max_dosage * 1000);
  while (true) {
    if (errorvalue == 1) {
      break;
    }
    dosage = balance(timespan);
    digitalWrite(MOT_3, HIGH);
    display.clear();
    display.print(dosage);
    Serial.print("3 ");
    Serial.print(d_3_g);
    Serial.print(" ");
    Serial.println(dosage);
    if (dosage > d_3_g) {
      digitalWrite(MOT_3, LOW);
      break;
    }
    if (dosage < -(glasgewicht)) {
      digitalWrite(MOT_3, LOW);
      errorvalue = 1;
      display.clear();
      menustr = "STOP";
      display.print(menustr);
      Serial.println(menustr);
      break;
    }
    if (millis() > dosage_timer) {
      digitalWrite(MOT_3, LOW);
      errorvalue = 1;
      display.clear();
      menustr = "STOP";
      display.print(menustr);
      Serial.println(menustr);
      break;
    }
  }

  menustr = "SKOL";
  display.print(menustr);
  Serial.println(menustr);
  delay(10000);
}

void serial_purge() {
  //receive from serial terminal
  if (Serial.available() > 0) {
    float i;
    char inByte = Serial.read();
    if (inByte == '1') {
      MOT_1_state = !MOT_1_state;
      Serial.print("MOT_1: ");
      Serial.println(MOT_1_state);
      digitalWrite(MOT_1, MOT_1_state);
    }
    if (inByte == '2') {
      MOT_2_state = !MOT_2_state;
      Serial.print("MOT_2: ");
      Serial.println(MOT_2_state);
      digitalWrite(MOT_2, MOT_2_state);
    }
    if (inByte == '3') {
      MOT_3_state = !MOT_3_state;
      Serial.print("MOT_3: ");
      Serial.println(MOT_3_state);
      digitalWrite(MOT_3, MOT_3_state);
    }
  }
}
