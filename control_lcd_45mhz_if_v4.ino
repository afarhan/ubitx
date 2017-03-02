/**
 * This source file is under General Public License version 3.
 * 
 * Most source code are meant to be understood by the compilers and the computers. 
 * Code that has to be hackable needs to be well understood and properly documented. 
 * Donald Knuth coined the term Literate Programming to indicate code that is written be 
 * easily read and understood.
 * 
 * The Raduino is a small board that includes the Arduin Nano, a 16x2 LCD display and
 * an Si5351a frequency synthesizer. This board is manufactured by Paradigm Ecomm Pvt Ltd
 * 
 * To learn more about Arduino you may visit www.arduino.cc. 
 * 
 * The Arduino works by firt executing the code in a function called setup() and then it 
 * repeatedly keeps calling loop() forever. All the initialization code is kept in setup()
 * and code to continuously sense the tuning knob, the function button, transmit/receive,
 * etc is all in the loop() function. If you wish to study the code top down, then scroll
 * to the bottom of this file and read your way up.
 * 
 * Below are the libraries to be included for building the Raduino 
 * The EEPROM library is used to store settings like the frequency memory, caliberation data, 
 * callsign etc .
 */

#include <EEPROM.h>

/** 
 *  The main chip which generates upto three oscillators of various frequencies in the
 *  Raduino is the Si5351a. To learn more about Si5351a you can download the datasheet 
 *  from www.silabs.com although, strictly speaking it is not a requirment to understand this code. 
 *  Instead, you can look up the Si5351 library written by xxx, yyy. You can download and 
 *  install it from www.url.com to complile this file.
 *  The Wire.h library is used to talk to the Si5351 and we also declare an instance of 
 *  Si5351 object to control the clocks.
 */
#include <Wire.h>
#include <si5351.h>
Si5351 si5351;
/** 
 * The Radiono board is the size of a standard 16x2 LCD panel. It has three connectors:
 * 
 * First, is an 8 pin connector that provides +5v, GND and six analog input pins that can also be 
 * configured to be used as digital input or output pins. These are referred to as A0,A1,A2,
 * A3,A6 and A7 pins. The A4 and A5 pins are missing from this connector as they are used to 
 * talk to the Si5351 over I2C protocol. 
 * 
 * Second is a 16 pin LCD connector. This connector is meant specifically for the standard 16x2
 * LCD display in 4 bit mode. The 4 bit mode requires 4 data lines and two control lines to work:
 * Lines used are : RESET, ENABLE, D4, D5, D6, D7 
 * We include the library and declare the configuration of the LCD panel too
 */

#include <LiquidCrystal.h>
LiquidCrystal lcd(8,9,10,11,12,13);

/**
 * The Arduino, unlike C/C++ on a regular computer with gigabytes of RAM, has very little memory.
 * We have to be very careful with variables that are declared inside the functions as they are 
 * created in a memory region called the stack. The stack has just a few bytes of space on the Arduino
 * if you declare large strings inside functions, they can easily exceed the capacity of the stack
 * and mess up your programs. 
 * We circumvent this by declaring a few global buffers as  kitchen counters where we can 
 * slice and dice our strings. These strings are mostly used to control the display or handle
 * the input and output from the USB port. We must keep a count of the bytes used while reading
 * the serial port as we can easily run out of buffer space. This is done in the serial_in_count variable.
 */
char serial_in[32], c[30], b[30], printBuff[32];
int count = 0;
unsigned char serial_in_count = 0;

/**
 * We need to carefully pick assignment of pin for various purposes.
 * There are two sets of completely programmable pins on the Raduino.
 * First, on the top of the board, in line with the LCD connector is an 8-pin connector
 * that is largely meant for analog inputs and front-panel control. It has a regulated 5v output,
 * ground and six pins. Each of these six pins can be individually programmed 
 * either as an analog input, a digital input or a digital output. 
 * The pins are assigned as follows: 
 *      A0, A1, A2, A3, +5v, GND, A6, A7 
 *      (while holding the board up so that back of the board faces you)
 *      
 * Though, this can be assigned anyway, for this application of the Arduino, we will make the following
 * assignment
 * A2 will connect to the PTT line, which is the usually a part of the mic connector
 * A3 is connected to a push button that can momentarily ground this line. This will be used for RIT/Bandswitching, etc.
 * A6 is to implement a keyer, it is reserved and not yet implemented
 * A7 is connected to a center pin of good quality 100K or 10K linear potentiometer with the two other ends connected to
 * ground and +5v lines available on the connector. This implments the tuning mechanism
 */
#define S_METER (A0)
#define PTT   (A1)
#define FBUTTON (A3)
#define ANALOG_KEYER (A7)
#define ANALOG_TUNING (A6)

/** 
 *  The second set of 16 pins on the bottom connector are have the three clock outputs and the digital lines to control the rig.
 *  This assignment is as follows :
 *    Pin   1   2    3    4    5    6    7    8    9    10   11   12   13   14   15   16
 *         GND +5V CLK0  GND  GND  CLK1 GND  GND  CLK2  GND  D2   D3   D4   D5   D6   D7  
 *  These too are flexible with what you may do with them, for the Raduino, we use them to :
 *  - TX_RX line : Switches between Transmit and Receive after sensing the PTT or the morse keyer
 *  - CW_KEY line : turns on the carrier for CW
 */

#define TX_RX (7)
#define CW_TONE (6)
#define TX_LPF_SEL (5)
#define CW_KEY (4)

/**
 *  The raduino has a number of timing parameters, all specified in milliseconds 
 *  CW_TIMEOUT : how many milliseconds between consecutive keyup and keydowns before switching back to receive?
 *  The next set of three parameters determine what is a tap, a double tap and a hold time for the funciton button
 *  TAP_DOWN_MILLIS : upper limit of how long a tap can be to be considered as a button_tap
 *  TAP_UP_MILLIS : upper limit of how long a gap can be between two taps of a button_double_tap
 *  TAP_HOLD_MILIS : many milliseconds of the buttonb being down before considering it to be a button_hold
 */
 
#define TAP_UP_MILLIS (500)
#define TAP_DOWN_MILLIS (600)
#define TAP_HOLD_MILLIS (2000)
#define CW_TIMEOUT (2000l) // in milliseconds, this is the parameter that determines how long the tx will hold between cw key downs

/**
 * The Raduino supports two VFOs : A and B and receiver incremental tuning (RIT). 
 * we define a variables to hold the frequency of the two VFOs, RITs
 * the rit offset as well as status of the RIT
 */
#define VFO_A 0
#define VFO_B 1
char ritOn = 0;
char vfoActive = VFO_A;
unsigned long vfoA=7100000L, vfoB=14200000L, ritA, ritB, sideTone=800, lsbCarrier, usbCarrier;

#define MASTER_CAL 0
#define LSB_CAL 4
#define USB_CAL 8
#define SIDE_TONE 12

/**
 * Raduino needs to keep track of current state of the transceiver. These are a few variables that do it
 */
char inTx = 0;
char keyDown = 0;
char isUSB = 0;
unsigned long cwTimeout = 0;
unsigned char txFilter = 0;

/** Tuning Mechanism of the Raduino
 *  We use a linear pot that has two ends connected to +5 and the ground. the middle wiper
 *  is connected to ANALOG_TUNNING pin. Depending upon the position of the wiper, the
 *  reading can be anywhere from 0 to 1024.
 *  The tuning control works in steps of 50Hz each for every increment between 50 and 950.
 *  Hence the turning the pot fully from one end to the other will cover 50 x 900 = 45 KHz.
 *  At the two ends, that is, the tuning starts slowly stepping up or down in 10 KHz steps 
 *  To stop the scanning the pot is moved back from the edge. 
 *  To rapidly change from one band to another, you press the function button and then
 *  move the tuning pot. Now, instead of 50 Hz, the tuning is in steps of 50 KHz allowing you
 *  rapidly use it like a 'bandset' control.
 *  To implement this, we fix a 'base frequency' to which we add the offset that the pot 
 *  points to. We also store the previous position to know if we need to wake up and change
 *  the frequency.
 */

unsigned long baseTune = 7100000L;
int  old_knob = 0;

#define FIRST_IF   (45000000l)
#define SECOND_OSC (57000000l)
#define INIT_USB_FREQ   (11996500l)
#define INIT_LSB_FREQ   (11998500l)
#define LOWEST_FREQ  (3000000l)
#define HIGHEST_FREQ (30000000l)

long frequency, stepSize=100000;

/**
 * The raduino can be booted into multiple modes:
 * MODE_NORMAL : works the radio  normally
 * MODE_CALIBRATION : used to calibrate Raduino.
 * To enter this mode, hold the function button down and power up. Tune to exactly 10 MHz on clock0 and release the function button
 */
 #define MODE_NORMAL (0)
 #define MODE_CALIBRATE (1)
 char mode = MODE_NORMAL;

char meter[17];
byte s_meter[] = {
  B0,B0,B0,B0,B0,B00000,B0,B10101,
  B0,B0,B0,B0,B10000,B10000,B0,B10101,
  B0,B0,B0,B0,B11000,B11000,B0,B10101,
  B0,B0,B0,B0,B11100,B11100,B0,B10101,
  B0,B0,B0,B0,B11110,B11110,B0,B10101,
  B0,B0,B0,B0,B11111,B11111,B0,B10101
};

void setupSmeter(){
  lcd.createChar(1, s_meter);
  lcd.createChar(2, s_meter + 8);
  lcd.createChar(3, s_meter + 16);
  lcd.createChar(4, s_meter + 24);
  lcd.createChar(5, s_meter + 32);
  lcd.createChar(6, s_meter + 40);
}

/* display routines */
void printLine1(char *c){
  if (strcmp(c, printBuff)){
    lcd.setCursor(0, 0);
    lcd.print(c);
    strcpy(printBuff, c);
    count++;
  }
}

void printLine2(char *c){
  if (strlen(c) > 16)
    c[16] = 0;
  lcd.setCursor(0, 1);
  lcd.print(c);
}

void displayFrequency(unsigned long f){
  int mhz, khz, hz;
  
  mhz = f / 1000000l;
  khz = (f % 1000000l)/1000;
  hz = f % 1000l;
  sprintf(b, "[%02d.%03d.%03d]", mhz, khz, hz);
  printLine1(b);
}

void updateMeter(){
  int16_t best, i, s;

   best = 0;
  //take 100 readings, take the peaks
  for (i = 0; i < 100; i++){
    s = analogRead(S_METER);
    if (s > best)
      best = s;
  }
  //now, use the s to get the signal
  s = best *2;
  sprintf(meter, "%3d", s); 
  for (i = 3; i < 14; i++){
    if (s >= 5)
      meter[i] = 6;
    else if (s > 0)
      meter[i] = 1 + s;
    else
      meter[i] = 1;
    s = s - 5;
  }
  meter[i] = 0; 
  printLine2(meter);
}

/**
 * Defines for menus
 */
byte menuOn = 0;

int ritToggle(int btn){
  if (!btn){
    if (ritOn == 1)
      printLine2("RIT:On, Off?   ");
    else
      printLine2("RIT:Off, On?   ");
  }
  else {
      if (ritOn == 0){
        ritOn = 1;
        printLine2("RIT is On.       ");
      }
      else {
        ritOn = 0;
        printLine2("RIT Is Off.      ");      
      }
  }
}

int vfoToggle(int btn){
  
  if (!btn){
    if (vfoActive == VFO_A)
      printLine2("Select VFO B?   ");
    else
      printLine2("Select VFO A?   ");    
  }
  else {
      if (vfoActive == VFO_A){
        vfoActive = VFO_A;
        printLine1("Selected VFO A  ");
        frequency = vfoA;
      }
      else {
        vfoActive = VFO_B;
        printLine1("Selected VFO B  ");      
        frequency = vfoB;
      }
      setFrequency(frequency);
      updateDisplay();
      resetBasefrequency();
  }
}

/**
 * Load the new correction and resets the clock 0 to 10 MHz
 */

void recalibrate(int32_t correction){
  
    si5351.set_correction(correction);
    
    si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
    si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLB);
    
    si5351.output_enable(SI5351_CLK0, 1);
    si5351.output_enable(SI5351_CLK1, 0);
    si5351.output_enable(SI5351_CLK2, 0);
    
    si5351.set_freq(1000000000ULL,  SI5351_CLK0);     
}

void calibrateMaster(int btn){
    int knob = 0;
    int32_t correction;
     
    if (!btn){
      printLine2("Set Calibration?");
      return;
    }
    printLine1("Set to 10.000.000");
    printLine2("PTT to confirm.  ");
    delay(2000);

    recalibrate(0);

    //disable all clock 1 and clock 2 
    while(digitalRead(PTT) == HIGH && !btnDown()){
      knob = analogRead(ANALOG_TUNING);
      correction = (knob - 500) * 500ULL;
      recalibrate(correction);
      //abort if this button is down
      if (btnDown()){
        //re-enable the clock1 and clock 2
        break;
      }
      sprintf(c, "%3d ", knob);
      printLine2(c);
    }
    
    //save the setting
    if (digitalRead(PTT) == LOW){
      printLine2("Calibration set!");
      EEPROM.put(MASTER_CAL, correction);
      delay(2000);
    }
    else {
      EEPROM.get(MASTER_CAL, correction);
    }
    
    si5351.set_correction(correction); 
    si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
    si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLB);
    
    si5351.output_enable(SI5351_CLK0, 1);
    si5351.output_enable(SI5351_CLK1, 1);
    si5351.output_enable(SI5351_CLK2, 1);
         
    resetBasefrequency();
    setFrequency(frequency);    
    updateDisplay();
    menuOn = 0;
}

void setBFO(int btn, byte isLSB){
    int knob = 0;
    int32_t lsb_Freq;
     
    if (!btn){
      if (isLSB)
        printLine2("Set LSB Carrier ");
      else
        printLine2("Set USB Carrier ");      
      return;
    }
    if (isLSB)
      printLine1("Tune to best LSB");
    else
      printLine1("Tune to best USB");    
    printLine2("PTT to confirm. ");
    delay(2000);

    //disable all clock 1 and clock 2 
    while(digitalRead(PTT) == HIGH && !btnDown()){
      knob = analogRead(ANALOG_TUNING);
      if (isLSB){
        lsbCarrier = 12000000l - knob * 10;
        si5351.set_freq(lsbCarrier * 100ULL, SI5351_CLK0); 
      }
      else {
        usbCarrier = 12000000l - knob * 10;
        si5351.set_freq(usbCarrier * 100ULL, SI5351_CLK0); 
      }
      //abort if this button is down
      if (btnDown()){
        break;
      }
      sprintf(c, "%3d ", knob);
      printLine2(c);
    }
    //save the setting
    if (digitalRead(PTT) == LOW){
      printLine2("Carrier set!    ");
      if (isLSB)
        EEPROM.put(LSB_CAL, lsbCarrier);
      else
        EEPROM.put(USB_CAL, usbCarrier);
      delay(2000);
    }
    else {
      EEPROM.get(LSB_CAL, lsbCarrier);
      EEPROM.get(USB_CAL, usbCarrier);      
    }
            
    resetBasefrequency();
    setFrequency(frequency);    
    updateDisplay();
    menuOn = 0;
}

void resetBasefrequency(){
  int knob = analogRead(ANALOG_TUNING);
  baseTune = frequency - (50l * knob);
}

int exitMenu(int btn){
  
  if (!btn){
      printLine2("Exit Menu?      ");
  }
  else{
      menuOn = 0;
      resetBasefrequency();
  }
}

void doMenu(){
  int select, btnState;
  menuOn = 1;
  
  while(menuOn){
    select = analogRead(ANALOG_TUNING);
    //downscale the selection
    select = (select-50)/50;
    btnState = btnDown();
     delay(200);
     switch(select){
      case 0:
        ritToggle(btnState);
        break;
      case 1:
        vfoToggle(btnState);
        break;
      case 2:
        calibrateMaster(btnState);
        break;
      case 3:
        setBFO(btnState, 1);
        break;
      case 4:
        setBFO(btnState, 0);
        break;
        
      default:
        exitMenu(btnState);
        break;
     } 
  }
}

void updateDisplay(){
    sprintf(b, "%8ld", frequency);      
    sprintf(c, "%s:%.2s.%.3s.%1s", vfoActive == VFO_A ? "A" : "B" , b,  b+2, b+5);
    if (isUSB)
      strcat(c, " USB");
    else
      strcat(c, " LSB");
      
    if (inTx)
      strcat(c, " TX");
    else if (ritOn)
      strcat(c, " +R");
    else
      strcat(c, "   ");
    
      
    printLine1(c);
/*    sprintf(c, "%s %s %d", isLSB ? "LSB" : "USB", inTx ? " TX" : " RX", digitalRead(FBUTTON));
    printLine2(c); */
}

void setSidebandAndTXFilters(unsigned long freq){
  
  if (freq > 10000000L){
    isUSB = 1;
    si5351.set_freq(usbCarrier * 100ULL, SI5351_CLK0); 
    digitalWrite(TX_LPF_SEL, 1);
  }
  else{
    isUSB = 0;
    digitalWrite(TX_LPF_SEL, 0);
    si5351.set_freq(lsbCarrier * 100ULL, SI5351_CLK0); 
  }
}

void setFrequency(unsigned long f){
  uint64_t osc_f;
  
  setSidebandAndTXFilters(f);

  if (isUSB)
    si5351.set_freq((SECOND_OSC - usbCarrier + f) * 100ULL, SI5351_CLK2);
  else
    si5351.set_freq((SECOND_OSC - lsbCarrier + f) * 100ULL, SI5351_CLK2);

  frequency = f;
}

void checkTX(){
	
  //we don't check for ptt when transmitting cw
  if (cwTimeout > 0)
    return;
    
  if (digitalRead(PTT) == 0 && inTx == 0){
    inTx = 1;
    digitalWrite(TX_RX, 1);
    updateDisplay();
  }
	
  if (digitalRead(PTT) == 1 && inTx == 1){
    inTx = 0;
    digitalWrite(TX_RX, 0);
    updateDisplay();
  }
}

/*
byte prevWasDot = 0;

void keyer(){
    int key = analogRead(ANALOG_KEYER);

    if (key < 50)         //straight key
      keyDown();
    else if (key < 300)   // both
      if (prevWasDot){
         keyDown(dotPeriod * 3);
         prevWasDot = 0;
      }
      else {
        keyDown(dotPeriod);
        prevWasDot = 1;
      }
    else if (key < 600){   //dash
      keyDown(dotPeriod * 3);
      prevWasDot = 0;
    }
    else if (key > 900){   //dot
      keyUp();
    }
}
*/


void checkCW2(){

  if (keyDown == 0 && analogRead(ANALOG_KEYER) < 50){
    //switch to transmit mode if we are not already in it
    if (inTx == 0){
      digitalWrite(TX_RX, 1);      
//      selectTransmitFilter();
      //give the relays a few ms to settle the T/R relays
      delay(50);
    }
    inTx = 1;
    keyDown = 1;
    tone(CW_TONE, (int)sideTone);
    updateDisplay();
  }

  //reset the timer as long as the key is down
  if (keyDown == 1){
     cwTimeout = CW_TIMEOUT + millis();
  }

  //if we have a keyup
  if (keyDown == 1 && analogRead(ANALOG_KEYER) > 150){
    keyDown = 0;
    noTone(CW_TONE);
    cwTimeout = millis() + CW_TIMEOUT;
  }

  //if we are in cw-mode and have a keyuup for a longish time
  if (cwTimeout > 0 && inTx == 1 && cwTimeout < millis()){
    //move the radio back to receive
    digitalWrite(TX_RX, 0);
    inTx = 0;
    cwTimeout = 0;
    updateDisplay();
  }
}

void checkCW3(){

  if (keyDown == 0 && analogRead(ANALOG_KEYER) < 50){
    //switch to transmit mode if we are not already in it
    if (inTx == 0){

      if (isUSB)
        si5351.set_freq((frequency + sideTone) * 100ULL, SI5351_CLK2);
      else
        si5351.set_freq((frequency - sideTone) * 100ULL, SI5351_CLK2); 
      //switch off the second oscillator and the bfo     
      si5351.output_enable(SI5351_CLK0, 0);
      si5351.output_enable(SI5351_CLK1, 0);
      si5351.output_enable(SI5351_CLK2, 1);

      digitalWrite(TX_RX, 1);      
//      selectTransmitFilter();
      //give the relays a few ms to settle the T/R relays
      delay(50);
    }
    inTx = 1;
    keyDown = 1;
    tone(CW_TONE, (int)sideTone);
    digitalWrite(CW_KEY, 1);      

    updateDisplay();
  }

  //reset the timer as long as the key is down
  if (keyDown == 1){
     cwTimeout = CW_TIMEOUT + millis();
  }

  //if we have a keyup
  if (keyDown == 1 && analogRead(ANALOG_KEYER) > 150){
    keyDown = 0;
    noTone(CW_TONE);
    digitalWrite(CW_KEY, 0);    
    cwTimeout = millis() + CW_TIMEOUT;
  }

  //if we are in cw-mode and have a keyuup for a longish time
  if (cwTimeout > 0 && inTx == 1 && cwTimeout < millis()){
    //move the radio back to receive
    digitalWrite(TX_RX, 0);
    inTx = 0;
    cwTimeout = 0;
    updateDisplay();

    //switch off the second oscillator and the bfo     
    si5351.output_enable(SI5351_CLK0, 1);
    si5351.output_enable(SI5351_CLK1, 1);
    si5351.output_enable(SI5351_CLK2, 1);
    setFrequency(frequency);
  }
}


void checkCW(){

  if (keyDown == 0 && analogRead(ANALOG_KEYER) < 50){
    //switch to transmit mode if we are not already in it
    if (inTx == 0){
      digitalWrite(TX_RX, 1);
      delay(50);
      inTx = 1;
      keyDown = 1;
    }
    if (isUSB)
      si5351.set_freq((frequency + sideTone) * 100ULL, SI5351_CLK2);
    else
      si5351.set_freq((frequency - sideTone) * 100ULL, SI5351_CLK2); 
    //switch off the second oscillator and the bfo     
    si5351.output_enable(SI5351_CLK0, 0);
    si5351.output_enable(SI5351_CLK1, 0);
    si5351.output_enable(SI5351_CLK2, 1);
    
    digitalWrite(CW_KEY, 1);
    tone(CW_TONE, sideTone);
    updateDisplay();
  }

  //reset the timer as long as the key is down
  if (keyDown == 1){
     cwTimeout = CW_TIMEOUT + millis();
  }

  //if we have a keyup
  if (keyDown == 1 && analogRead(ANALOG_KEYER) > 150){
    keyDown = 0;
    noTone(CW_TONE);
    digitalWrite(CW_KEY, 0);
    cwTimeout = millis() + CW_TIMEOUT;
  }

  //if we are in cw-mode and have a keyuup for a longish time
  if (cwTimeout > 0 && inTx == 1 && cwTimeout < millis()){
    //move the radio back to receive
    digitalWrite(TX_RX, 0);
    inTx = 0;
    cwTimeout = 0;
    //switch off the second oscillator and the bfo     
    si5351.output_enable(SI5351_CLK0, 1);
    si5351.output_enable(SI5351_CLK1, 1);
    si5351.output_enable(SI5351_CLK2, 1);
    setFrequency(frequency);
    updateDisplay();
  }
}

int btnDown(){
  if (digitalRead(FBUTTON) == HIGH)
    return 0;
  else
    return 1;
}

void checkButton(){
  int i, t1, t2, knob, new_knob, duration;

  //only if the button is pressed
  if (!btnDown())
    return;

  //wait for 50 ms before declaring the button to be really down 
  delay(50);
  if (!btnDown())
    return;
    
  t1 = millis();
  knob = analogRead(ANALOG_TUNING);
  duration = 0;
  
  /* keep measuring how long the duration of btn down has been to a max of 3 seconds */
  while (btnDown() && duration < 3000){
        /* if the tuning knob is moved while the btn is down, 
           then track the bandset until the button is up and return
        */
      new_knob = analogRead(ANALOG_TUNING);
      if (abs(new_knob - knob) > 10){
        int count = 0;
        /* track the tuning and return */
        while (btnDown()){
          frequency = baseTune = ((analogRead(ANALOG_TUNING) * 30000l) + 1000000l);
          setFrequency(frequency);
          updateDisplay();
          count++;
          delay(200);
        }
        delay(1000);
        return;
      } /* end of handling the bandset */
      
     delay(100);
     duration += 100;
  }

  if  (duration < 1000) {
    printLine2("Menu.");
    doMenu();
  }    
}

void doTuning(){
 unsigned long newFreq;
 
 int knob = analogRead(ANALOG_TUNING);
 unsigned long old_freq = frequency;

 if (knob < 10 && frequency > LOWEST_FREQ) {
      baseTune = baseTune - 1000l;
      frequency = baseTune;
      updateDisplay();
      setFrequency(frequency);
      delay(50);
  } 
  else if (knob > 1010 && frequency < HIGHEST_FREQ) {
     baseTune = baseTune + 1000l; 
     frequency = baseTune + 50000l;
     setFrequency(frequency);
     updateDisplay();
     delay(50);
  }
  // in the middle, it is business as usual 
  else if (knob != old_knob){
     frequency = baseTune + (50l * knob);
     old_knob = knob;
     setFrequency(frequency);
     updateDisplay();
  }
}


void setup()
{
  int32_t cal;
  
  lcd.begin(16, 2);
  setupSmeter();
  printBuff[0] = 0;
  printLine1("HFuino v0.01 "); 
  printLine2("             "); 
  
  EEPROM.get(MASTER_CAL, cal);
  EEPROM.get(LSB_CAL, lsbCarrier);
  EEPROM.get(USB_CAL, usbCarrier);
  //set the lsb and usb to defaults
  if (lsbCarrier == 0)
      lsbCarrier = INIT_LSB_FREQ;
  if (usbCarrier == 0)
      usbCarrier = INIT_USB_FREQ;
  
  // Start serial and initialize the Si5351
  Serial.begin(9600);
  analogReference(DEFAULT);
  Serial.println("*HFuino v0.01\n");
  Serial.println("*Searching Si5351\n");

  //configure the function button to use the external pull-up
  pinMode(FBUTTON, INPUT);
  digitalWrite(FBUTTON, HIGH);

  pinMode(PTT, INPUT);
  digitalWrite(PTT, HIGH);

  digitalWrite(ANALOG_KEYER, HIGH);

  pinMode(CW_TONE, OUTPUT);  
  digitalWrite(CW_TONE, 0);
  pinMode(TX_RX,OUTPUT);
  digitalWrite(TX_RX, 0);
  pinMode(TX_LPF_SEL, OUTPUT);
  digitalWrite(TX_LPF_SEL, 0);
  pinMode(CW_KEY, OUTPUT);
  digitalWrite(CW_KEY, 0);
  EEPROM.get(0,cal);
  
  si5351.init(SI5351_CRYSTAL_LOAD_8PF,25000000l,0);
 
  si5351.set_correction(cal);
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
  si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLB);
  
  si5351.output_enable(SI5351_CLK0, 1);
  si5351.output_enable(SI5351_CLK1, 1);
  si5351.output_enable(SI5351_CLK2, 1);
 // printLine1("check freq ");
 // si5351.set_freq(1000000000l, SI5351_CLK2); 

 // delay(20000);
   
  si5351.set_freq(lsbCarrier * 100ULL,  SI5351_CLK0); 
  si5351.set_freq(SECOND_OSC * 100ULL, SI5351_CLK1); 
  si5351.set_freq(5900000000l, SI5351_CLK2); 
  printLine2(b);
  delay(2000);
  
  // Set CLK0 to output 7 MHz with a fixed PLL frequency
  
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_8MA);
  si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_8MA);
  si5351.drive_strength(SI5351_CLK2, SI5351_DRIVE_8MA);
  
 
  Serial.println("*Si5350 ON");       
  delay(10);
}

void loop(){

  //generateCW(10000);
  //the order of testing first for cw and then for ptt is important.
  checkCW3();
  checkTX();
  checkButton();
  //tune only when not tranmsitting
  if (!inTx)
    doTuning(); 
  updateMeter();
  delay(50);
}

