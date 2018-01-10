/**
 * This source file is under General Public License version 3.
 * 
 * This verision uses a built-in Si5351 library
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
 * The Arduino works by starts executing the code in a function called setup() and then it 
 * repeatedly keeps calling loop() forever. All the initialization code is kept in setup()
 * and code to continuously sense the tuning knob, the function button, transmit/receive,
 * etc is all in the loop() function. If you wish to study the code top down, then scroll
 * to the bottom of this file and read your way up.
 * 
 * Below are the libraries to be included for building the Raduino 
 * The EEPROM library is used to store settings like the frequency memory, caliberation data, 
 * callsign etc .
 *
 *  The main chip which generates upto three oscillators of various frequencies in the
 *  Raduino is the Si5351a. To learn more about Si5351a you can download the datasheet 
 *  from www.silabs.com although, strictly speaking it is not a requirment to understand this code. 
 *  Instead, you can look up the Si5351 library written by xxx, yyy. You can download and 
 *  install it from www.url.com to complile this file.
 *  The Wire.h library is used to talk to the Si5351 and we also declare an instance of 
 *  Si5351 object to control the clocks.
 */
#include <Wire.h>
#include <EEPROM.h>

/**
    The main chip which generates upto three oscillators of various frequencies in the
    Raduino is the Si5351a. To learn more about Si5351a you can download the datasheet
    from www.silabs.com although, strictly speaking it is not a requirment to understand this code.

    We no longer use the standard SI5351 library because of its huge overhead due to many unused
    features consuming a lot of program space. Instead of depending on an external library we now use
    Jerry Gaffke's, KE7ER, lightweight standalone mimimalist "si5351bx" routines (see further down the
    code). Here are some defines and declarations used by Jerry's routines:
*/


/**
 * We need to carefully pick assignment of pin for various purposes.
 * There are two sets of completely programmable pins on the Raduino.
 * First, on the top of the board, in line with the LCD connector is an 8-pin connector
 * that is largely meant for analog inputs and front-panel control. It has a regulated 5v output,
 * ground and six pins. Each of these six pins can be individually programmed 
 * either as an analog input, a digital input or a digital output. 
 * The pins are assigned as follows (left to right, display facing you): 
 *      Pin 1 (Violet), A7, SPARE
 *      Pin 2 (Blue),   A6, KEYER (DATA)
 *      Pin 3 (Green), +5v 
 *      Pin 4 (Yellow), Gnd
 *      Pin 5 (Orange), A3, PTT
 *      Pin 6 (Red),    A2, F BUTTON
 *      Pin 7 (Brown),  A1, ENC B
 *      Pin 8 (Black),  A0, ENC A
 *Note: A5, A4 are wired to the Si5351 as I2C interface 
 *       *     
 * Though, this can be assigned anyway, for this application of the Arduino, we will make the following
 * assignment
 * A2 will connect to the PTT line, which is the usually a part of the mic connector
 * A3 is connected to a push button that can momentarily ground this line. This will be used for RIT/Bandswitching, etc.
 * A6 is to implement a keyer, it is reserved and not yet implemented
 * A7 is connected to a center pin of good quality 100K or 10K linear potentiometer with the two other ends connected to
 * ground and +5v lines available on the connector. This implments the tuning mechanism
 */

#define ENC_A (A0)
#define ENC_B (A1)
#define FBUTTON (A2)
#define PTT   (A3)
#define ANALOG_KEYER (A6)
#define ANALOG_SPARE (A7)

/** 
 * The Raduino board is the size of a standard 16x2 LCD panel. It has three connectors:
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

#define VERSION_NUM 0x01  //for KD8CEC'S firmware and for memory management software

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
char c[30], b[30];      
char printBuff[2][17];  //mirrors what is showing on the two lines of the display
int count = 0;          //to generally count ticks, loops, etc

/** 
 *  The second set of 16 pins on the Raduino's bottom connector are have the three clock outputs and the digital lines to control the rig.
 *  This assignment is as follows :
 *    Pin   1   2    3    4    5    6    7    8    9    10   11   12   13   14   15   16
 *         GND +5V CLK0  GND  GND  CLK1 GND  GND  CLK2  GND  D2   D3   D4   D5   D6   D7  
 *  These too are flexible with what you may do with them, for the Raduino, we use them to :
 *  - TX_RX line : Switches between Transmit and Receive after sensing the PTT or the morse keyer
 *  - CW_KEY line : turns on the carrier for CW
 */

#define TX_RX (7)
#define CW_TONE (6)
#define TX_LPF_A (5)
#define TX_LPF_B (4)
#define TX_LPF_C (3)
#define CW_KEY (2)

/**
 * These are the indices where these user changable settinngs are stored  in the EEPROM
 */
#define MASTER_CAL 0
#define LSB_CAL 4
#define USB_CAL 8
#define SIDE_TONE 12
//these are ids of the vfos as well as their offset into the eeprom storage, don't change these 'magic' values
#define VFO_A 16
#define VFO_B 20
#define CW_SIDETONE 24
#define CW_SPEED 28

//AT328 has 1KBytes EEPROM
#define VFO_A_MODE 256
#define VFO_B_MODE 257
#define CW_DELAY 258
#define CW_START 259

//
#define VERSION_ADDRESS 779   //check Firmware version
//USER INFORMATION
#define USER_CALLSIGN_KEY 780   //0x59
#define USER_CALLSIGN_LEN 781   //1BYTE (OPTION + LENGTH) + CALLSIGN (MAXIMUM 18)
#define USER_CALLSIGN_DAT 782   //CALL SIGN DATA  //direct EEPROM to LCD basic offset

//AUTO KEY STRUCTURE
//AUTO KEY USE 800 ~ 1023
#define CW_AUTO_MAGIC_KEY 800   //0x73
#define CW_AUTO_COUNT     801   //0 ~ 255
#define CW_AUTO_DATA      803   //[INDEX, INDEX, INDEX,DATA,DATA, DATA (Positon offset is CW_AUTO_DATA
#define CW_DATA_OFSTADJ   CW_AUTO_DATA - USER_CALLSIGN_DAT   //offset adjust for ditect eeprom to lcd (basic offset is USER_CALLSIGN_DAT
#define CW_STATION_LEN    1023  //value range : 4 ~ 30
/**
 * The uBITX is an upconnversion transceiver. The first IF is at 45 MHz.
 * The first IF frequency is not exactly at 45 Mhz but about 5 khz lower,
 * this shift is due to the loading on the 45 Mhz crystal filter by the matching
 * L-network used on it's either sides.
 * The first oscillator works between 48 Mhz and 75 MHz. The signal is subtracted
 * from the first oscillator to arriive at 45 Mhz IF. Thus, it is inverted : LSB becomes USB
 * and USB becomes LSB.
 * The second IF of 12 Mhz has a ladder crystal filter. If a second oscillator is used at 
 * 57 Mhz, the signal is subtracted FROM the oscillator, inverting a second time, and arrives 
 * at the 12 Mhz ladder filter thus doouble inversion, keeps the sidebands as they originally were.
 * If the second oscillator is at 33 Mhz, the oscilaltor is subtracated from the signal, 
 * thus keeping the signal's sidebands inverted. The USB will become LSB.
 * We use this technique to switch sidebands. This is to avoid placing the lsbCarrier close to
 * 12 MHz where its fifth harmonic beats with the arduino's 16 Mhz oscillator's fourth harmonic
 */

// the second oscillator should ideally be at 57 MHz, however, the crystal filter's center frequency 
// is shifted down a little due to the loading from the impedance matching L-networks on either sides
#define SECOND_OSC_USB (56995000l)
#define SECOND_OSC_LSB (32995000l) 
//these are the two default USB and LSB frequencies. The best frequencies depend upon your individual taste and filter shape
#define INIT_USB_FREQ   (11996500l)
// limits the tuning and working range of the ubitx between 3 MHz and 30 MHz
#define LOWEST_FREQ  (3000000l)
#define HIGHEST_FREQ (30000000l)

//When the frequency is moved by the dial, the maximum value by KD8CEC
#define LOWEST_FREQ_DIAL  (3000l)
#define HIGHEST_FREQ_DIAL (60000000l)

//we directly generate the CW by programmin the Si5351 to the cw tx frequency, hence, both are different modes
//these are the parameter passed to startTx
#define TX_SSB 0
#define TX_CW 1

char ritOn = 0;
char vfoActive = VFO_A;
int8_t meter_reading = 0; // a -1 on meter makes it invisible
unsigned long vfoA=7150000L, vfoB=14200000L, sideTone=800, usbCarrier;
unsigned long vfoA_eeprom, vfoB_eeprom; //for protect eeprom life
unsigned long frequency, ritRxFrequency, ritTxFrequency;  //frequency is the current frequency on the dial

int cwSpeed = 100; //this is actuall the dot period in milliseconds
extern int32_t calibration;

//for store the mode in eeprom
byte vfoA_mode=0, vfoB_mode = 0;          //0: default, 1:not use, 2:LSB, 3:USB, 4:CW, 5:AM, 6:FM
byte vfoA_mode_eeprom, vfoB_mode_eeprom;  //for protect eeprom life

//KD8CEC
//for AutoSave and protect eeprom life
byte saveIntervalSec = 10;  //second
unsigned long saveCheckTime = 0;
unsigned long saveCheckFreq = 0;

bool isSplitOn = false;
byte cwDelayTime = 60;
byte delayBeforeCWStartTime = 50;

//sideTonePitch + sideToneSub = sideTone
byte sideTonePitch=0;
byte sideToneSub = 0;

//DialLock
byte isDialLock = 0;
byte isTxOff = 0;

//Variables for auto cw mode
byte isCWAutoMode = 0;          //0 : none, 1 : CW_AutoMode_Menu_Selection, 2 : CW_AutoMode Sending
byte cwAutoTextCount = 0;       //cwAutoText Count
byte beforeCWTextIndex = 255;   //when auto cw start, always beforeCWTextIndex = 255, (for first time check)
byte cwAutoDialType = 0;        //0 : CW Text Change, 1 : Frequency Tune

#define AUTO_CW_RESERVE_MAX 3
byte autoCWSendReserv[AUTO_CW_RESERVE_MAX]; //Reserve CW Auto Send
byte autoCWSendReservCount = 0;             //Reserve CW Text Cound
byte sendingCWTextIndex = 0;                //cw auto seding Text Index

byte userCallsignLength = 0;    //7 : display callsign at system startup, 6~0 : callsign length (range : 1~18)

/**
 * Raduino needs to keep track of current state of the transceiver. These are a few variables that do it
 */
boolean txCAT = false;        //turned on if the transmitting due to a CAT command
char inTx = 0;                //it is set to 1 if in transmit mode (whatever the reason : cw, ptt or cat)
char splitOn = 0;             //working split, uses VFO B as the transmit frequency, (NOT IMPLEMENTED YET)
char keyDown = 0;             //in cw mode, denotes the carrier is being transmitted
char isUSB = 0;               //upper sideband was selected, this is reset to the default for the 
                              //frequency when it crosses the frequency border of 10 MHz
byte menuOn = 0;              //set to 1 when the menu is being displayed, if a menu item sets it to zero, the menu is exited
unsigned long cwTimeout = 0;  //milliseconds to go before the cw transmit line is released and the radio goes back to rx mode
unsigned long dbgCount = 0;   //not used now
unsigned char txFilter = 0;   //which of the four transmit filters are in use
boolean modeCalibrate = false;//this mode of menus shows extended menus to calibrate the oscillators and choose the proper
                              //beat frequency
/**
 * Below are the basic functions that control the uBitx. Understanding the functions before 
 * you start hacking around
 */

/*
  KD8CEC
  When using the basic delay of the Arduino, the program freezes.
  When the delay is used, the program will generate an error because it is not communicating, 
  so Create a new delay function that can do background processing.
 */
 
unsigned long delayBeforeTime = 0;
byte delay_background(unsigned delayTime, byte fromType){ //fromType : 4 autoCWKey -> Check Paddle
  delayBeforeTime = millis();

  while (millis() <= delayBeforeTime + delayTime) {

    if (fromType == 4)
    {
      //CHECK PADDLE
      if (getPaddle() != 0) //Interrupt : Stop cw Auto mode by Paddle -> Change Auto to Manual
        return 1;
        
      //Check PTT while auto Sending
      autoSendPTTCheck();
      
      Check_Cat(3);
    }
    else
    {
      //Background Work      
      Check_Cat(fromType);
    }
  }

  return 0;
}
 

/**
 * Select the properly tx harmonic filters
 * The four harmonic filters use only three relays
 * the four LPFs cover 30-21 Mhz, 18 - 14 Mhz, 7-10 MHz and 3.5 to 5 Mhz
 * Briefly, it works like this, 
 * - When KT1 is OFF, the 'off' position routes the PA output through the 30 MHz LPF
 * - When KT1 is ON, it routes the PA output to KT2. Which is why you will see that
 *   the KT1 is on for the three other cases.
 * - When the KT1 is ON and KT2 is off, the off position of KT2 routes the PA output
 *   to 18 MHz LPF (That also works for 14 Mhz) 
 * - When KT1 is On, KT2 is On, it routes the PA output to KT3
 * - KT3, when switched on selects the 7-10 Mhz filter
 * - KT3 when switched off selects the 3.5-5 Mhz filter
 * See the circuit to understand this
 */

void setTXFilters(unsigned long freq){
  
  if (freq > 21000000L){  // the default filter is with 35 MHz cut-off
    digitalWrite(TX_LPF_A, 0);
    digitalWrite(TX_LPF_B, 0);
    digitalWrite(TX_LPF_C, 0);
  }
  else if (freq >= 14000000L){ //thrown the KT1 relay on, the 30 MHz LPF is bypassed and the 14-18 MHz LPF is allowd to go through
    digitalWrite(TX_LPF_A, 1);
    digitalWrite(TX_LPF_B, 0);
    digitalWrite(TX_LPF_C, 0);
  }
  else if (freq > 7000000L){
    digitalWrite(TX_LPF_A, 1);
    digitalWrite(TX_LPF_B, 1);
    digitalWrite(TX_LPF_C, 0);    
  }
  else {
    digitalWrite(TX_LPF_A, 1);
    digitalWrite(TX_LPF_B, 1);
    digitalWrite(TX_LPF_C, 1);    
  }
}

/**
 * This is the most frequently called function that configures the 
 * radio to a particular frequeny, sideband and sets up the transmit filters
 * 
 * The transmit filter relays are powered up only during the tx so they dont
 * draw any current during rx. 
 * 
 * The carrier oscillator of the detector/modulator is permanently fixed at
 * uppper sideband. The sideband selection is done by placing the second oscillator
 * either 12 Mhz below or above the 45 Mhz signal thereby inverting the sidebands 
 * through mixing of the second local oscillator.
 */
 
void setFrequency(unsigned long f){
  uint64_t osc_f;

  //1 digits discarded
  f = (f / 50) * 50;
  
  setTXFilters(f);

  if (isUSB){
    si5351bx_setfreq(2, SECOND_OSC_USB - usbCarrier + f);
    si5351bx_setfreq(1, SECOND_OSC_USB);
  }
  else{
    si5351bx_setfreq(2, SECOND_OSC_LSB + usbCarrier + f);
    si5351bx_setfreq(1, SECOND_OSC_LSB);
  }
  
  frequency = f;
}

/**
 * startTx is called by the PTT, cw keyer and CAT protocol to
 * put the uBitx in tx mode. It takes care of rit settings, sideband settings
 * Note: In cw mode, doesnt key the radio, only puts it in tx mode
 */
 
void startTx(byte txMode, byte isDisplayUpdate){
  unsigned long tx_freq = 0;

  if (isTxOff != 1)
    digitalWrite(TX_RX, 1);
    
  inTx = 1;
  
  if (ritOn){
    //save the current as the rx frequency
    ritRxFrequency = frequency;
    setFrequency(ritTxFrequency);
  }

  if (txMode == TX_CW){
    //turn off the second local oscillator and the bfo
    si5351bx_setfreq(0, 0);
    si5351bx_setfreq(1, 0);

    //shif the first oscillator to the tx frequency directly
    //the key up and key down will toggle the carrier unbalancing
    //the exact cw frequency is the tuned frequency + sidetone
    if (isUSB)
      si5351bx_setfreq(2, frequency + sideTone);
    else
      si5351bx_setfreq(2, frequency - sideTone); 
  }

  //reduce latency time when begin of CW mode
  if (isDisplayUpdate == 1)
    updateDisplay();
}

void stopTx(){
  inTx = 0;

  digitalWrite(TX_RX, 0);           //turn off the tx
  si5351bx_setfreq(0, usbCarrier);  //set back the carrier oscillator anyway, cw tx switches it off

  if (ritOn)
    setFrequency(ritRxFrequency);
  else
    setFrequency(frequency);
  
  updateDisplay();
}

/**
 * ritEnable is called with a frequency parameter that determines
 * what the tx frequency will be
 */
void ritEnable(unsigned long f){
  ritOn = 1;
  //save the non-rit frequency back into the VFO memory
  //as RIT is a temporary shift, this is not saved to EEPROM
  ritTxFrequency = f;
}

// this is called by the RIT menu routine
void ritDisable(){
  if (ritOn){
    ritOn = 0;
    setFrequency(ritTxFrequency);
    updateDisplay();
  }
}

/**
 * Basic User Interface Routines. These check the front panel for any activity
 */

/**
 * The PTT is checked only if we are not already in a cw transmit session
 * If the PTT is pressed, we shift to the ritbase if the rit was on
 * flip the T/R line to T and update the display to denote transmission
 */

void checkPTT(){	
  //we don't check for ptt when transmitting cw
  if (cwTimeout > 0)
    return;
    
  if (digitalRead(PTT) == 0 && inTx == 0){
    startTx(TX_SSB, 1);
    delay(50); //debounce the PTT
  }
	
  if (digitalRead(PTT) == 1 && inTx == 1)
    stopTx();
}

void checkButton(){
  int i, t1, t2, knob, new_knob;

  //only if the button is pressed
  if (!btnDown())
    return;
  delay(50);
  if (!btnDown()) //debounce
    return;
 
  doMenu();
  
  //wait for the button to go up again
  while(btnDown()) {
    delay(10);
    Check_Cat(0);
  }
  delay(50);//debounce
}


/**
 * The tuning jumps by 50 Hz on each step when you tune slowly
 * As you spin the encoder faster, the jump size also increases 
 * This way, you can quickly move to another band by just spinning the 
 * tuning knob
 */

void doTuning(){
  int s = 0;
  unsigned long prev_freq;
  int incdecValue = 0;

  if (isDialLock == 1)
    return;

  if (isCWAutoMode == 0 || cwAutoDialType == 1)
    s = enc_read();

  if (s){
    prev_freq = frequency;
    
    if (s > 10)
      incdecValue = 200000l;
    if (s > 7)
      incdecValue = 10000l;
    else if (s > 4)
      incdecValue = 1000l;
    else if (s > 2)
      incdecValue = 500;
    else if (s > 0)
      incdecValue =  50l;
    else if (s > -2)
      incdecValue = -50l;
    else if (s > -4)
      incdecValue = -500l;
    else if (s > -7)
      incdecValue = -1000l;
    else if (s > -9)
      incdecValue = -10000l;
    else
      incdecValue = -200000l;

    if (incdecValue > 0 && frequency + incdecValue > HIGHEST_FREQ_DIAL)
        frequency = HIGHEST_FREQ_DIAL;      
    else if (incdecValue < 0 && frequency < -incdecValue + LOWEST_FREQ_DIAL)  //for compute and compare based integer type.
      frequency = LOWEST_FREQ_DIAL;
    else
      frequency += incdecValue;
      
    if (prev_freq < 10000000l && frequency > 10000000l)
      isUSB = true;
      
    if (prev_freq > 10000000l && frequency < 10000000l)
      isUSB = false;

    setFrequency(frequency);
    updateDisplay();
  }
}

/**
 * RIT only steps back and forth by 100 hz at a time
 */
void doRIT(){
  unsigned long newFreq;
 
  int knob = enc_read();
  unsigned long old_freq = frequency;

  if (knob < 0)
    frequency -= 100l;
  else if (knob > 0)
    frequency += 100;
 
  if (old_freq != frequency){
    setFrequency(frequency);
    updateDisplay();
  }
}

/**
 save Frequency and mode to eeprom
 */
void storeFrequencyAndMode(byte saveType)
{
  //freqType : 0 Both (vfoA and vfoB), 1 : vfoA, 2 : vfoB
  if (saveType == 0 || saveType == 1) //vfoA
  {
      if (vfoA != vfoA_eeprom) {
        EEPROM.put(VFO_A, vfoA);
        vfoA_eeprom = vfoA;
      }
      
      if (vfoA_mode != vfoA_mode_eeprom) {
        EEPROM.put(VFO_A_MODE, vfoA_mode);
        vfoA_mode_eeprom = vfoA_mode;
      }
  }
  
  if (saveType == 0 || saveType == 2) //vfoB
  {
      if (vfoB != vfoB_eeprom) {
        EEPROM.put(VFO_B, vfoB);
        vfoB_eeprom = vfoB;
      }
      
      if (vfoB_mode != vfoB_mode_eeprom) {
          EEPROM.put(VFO_B_MODE, vfoB_mode);
          vfoB_mode_eeprom = vfoB_mode;
      }
  }
}

/**
 * The settings are read from EEPROM. The first time around, the values may not be 
 * present or out of range, in this case, some intelligent defaults are copied into the 
 * variables.
 */
void initSettings(){
  //read the settings from the eeprom and restore them
  //if the readings are off, then set defaults
  EEPROM.get(MASTER_CAL, calibration); 
  EEPROM.get(USB_CAL, usbCarrier);
  EEPROM.get(VFO_A, vfoA);
  EEPROM.get(VFO_B, vfoB);
  EEPROM.get(CW_SIDETONE, sideTone);
  EEPROM.get(CW_SPEED, cwSpeed);

  //for Save VFO_A_MODE to eeprom
  //0: default, 1:not use, 2:LSB, 3:USB, 4:CW, 5:AM, 6:FM
  EEPROM.get(VFO_A_MODE, vfoA_mode);
  EEPROM.get(VFO_B_MODE, vfoB_mode);

  //CW DelayTime
  EEPROM.get(CW_DELAY, cwDelayTime);

  //CW interval between TX and CW Start
  EEPROM.get(CW_START, delayBeforeCWStartTime);

  //User callsign information
  if (EEPROM.read(USER_CALLSIGN_KEY) == 0x59)
    userCallsignLength = EEPROM.read(USER_CALLSIGN_LEN);  //MAXIMUM 18 LENGTH

  //Version Write for Memory Management Software
  if (EEPROM.read(VERSION_ADDRESS) != VERSION_NUM)
    EEPROM.write(VERSION_ADDRESS, VERSION_NUM);
  
  if (cwDelayTime < 1 || cwDelayTime > 250)
    cwDelayTime = 60;

  if (vfoA_mode < 2)
    vfoA_mode = 2;
  
  if (vfoB_mode < 2)
    vfoB_mode = 3;

  if (usbCarrier > 12010000l || usbCarrier < 11990000l)
    usbCarrier = 11995000l;
    
  if (vfoA > 35000000l || 3500000l > vfoA) {
     vfoA = 7150000l;
     vfoA_mode = 2;
  }
  
  if (vfoB > 35000000l || 3500000l > vfoB) {
     vfoB = 14150000l;  
     vfoB_mode = 3;
  }

  //for protect eeprom life
  vfoA_eeprom = vfoA;
  vfoB_eeprom = vfoB;
  vfoA_mode_eeprom = vfoA_mode;
  vfoB_mode_eeprom = vfoB_mode;

  if (sideTone < 100 || 2000 < sideTone) 
    sideTone = 800;
  if (cwSpeed < 10 || 1000 < cwSpeed) 
    cwSpeed = 100;

  if (sideTone < 300 || sideTone > 1000) {
    sideTonePitch = 0;
    sideToneSub = 0;;
  }
  else{
    sideTonePitch = (sideTone - 300) / 50;
    sideToneSub = sideTone % 50;
  }
}

void initPorts(){

  analogReference(DEFAULT);

  //??
  pinMode(ENC_A, INPUT_PULLUP);
  pinMode(ENC_B, INPUT_PULLUP);
  pinMode(FBUTTON, INPUT_PULLUP);
  
  //configure the function button to use the external pull-up
//  pinMode(FBUTTON, INPUT);
//  digitalWrite(FBUTTON, HIGH);

  pinMode(PTT, INPUT_PULLUP);
  pinMode(ANALOG_KEYER, INPUT_PULLUP);

  pinMode(CW_TONE, OUTPUT);  
  digitalWrite(CW_TONE, 0);
  
  pinMode(TX_RX,OUTPUT);
  digitalWrite(TX_RX, 0);

  pinMode(TX_LPF_A, OUTPUT);
  pinMode(TX_LPF_B, OUTPUT);
  pinMode(TX_LPF_C, OUTPUT);
  digitalWrite(TX_LPF_A, 0);
  digitalWrite(TX_LPF_B, 0);
  digitalWrite(TX_LPF_C, 0);

  pinMode(CW_KEY, OUTPUT);
  digitalWrite(CW_KEY, 0);
}

void setup()
{
  //Init EEProm for Fault EEProm TEST and Factory Reset
  /*
  for (int i = 0; i < 1024; i++)
    EEPROM.write(i, 0);
  */
  //Serial.begin(9600);
  lcd.begin(16, 2);

  Init_Cat(38400, SERIAL_8N1);
  initMeter(); //not used in this build
  initSettings();

  printLineF(1, F("CECBT v0.25")); 
  if (userCallsignLength > 0 && ((userCallsignLength & 0x80) == 0x80))
  {
    userCallsignLength = userCallsignLength & 0x7F;
    printLineFromEEPRom(0, 0, 0, userCallsignLength -1); //eeprom to lcd use offset (USER_CALLSIGN_DAT)
  }
  else
  {
    printLineF(0, F("uBITX v0.20")); 
    delay_background(500, 0);
    printLine2(""); 
  }
  
  initPorts();     
  initOscillators();

  frequency = vfoA;
  saveCheckFreq = frequency;  //for auto save frequency
  byteToMode(vfoA_mode);
  setFrequency(vfoA);
  updateDisplay();

  if (btnDown())
    factory_alignment();

/*
   //This is for auto key test
  EEPROM.put(CW_AUTO_MAGIC_KEY, 0x73);        //MAGIC KEY
  EEPROM.put(CW_AUTO_COUNT, 3);               //WORD COUNT
  EEPROM.put(CW_AUTO_DATA + 0, 6);        // 0 word begin postion / CQCQ TEST K
  EEPROM.put(CW_AUTO_DATA + 1, 33);       // 0 word end postion / CQCQ TEST K
  EEPROM.put(CW_AUTO_DATA + 2, 34);       //1 word begin position / LOL LOL
  EEPROM.put(CW_AUTO_DATA + 3, 40);       //1 word end position / LOL LOL
  EEPROM.put(CW_AUTO_DATA + 4, 41);       //2 word begin position / /?![]789
  EEPROM.put(CW_AUTO_DATA + 5, 48);       //2 word end position / /?![]789
  
  EEPROM.put(CW_AUTO_DATA + 6, 'C');      //
  EEPROM.put(CW_AUTO_DATA + 7, 'Q');      //
  EEPROM.put(CW_AUTO_DATA + 8, 'C');      //
  EEPROM.put(CW_AUTO_DATA + 9, 'Q');      //
  EEPROM.put(CW_AUTO_DATA + 10, ' ');      //
  EEPROM.put(CW_AUTO_DATA + 11, 'D');      //
  EEPROM.put(CW_AUTO_DATA + 12, 'E');      //
  EEPROM.put(CW_AUTO_DATA + 13, ' ');      //
  EEPROM.put(CW_AUTO_DATA + 14, 'K');      //
  EEPROM.put(CW_AUTO_DATA + 15, 'D');      //
  EEPROM.put(CW_AUTO_DATA + 16, '8');      //
  EEPROM.put(CW_AUTO_DATA + 17, 'C');      //
  EEPROM.put(CW_AUTO_DATA + 18, 'E');      //
  EEPROM.put(CW_AUTO_DATA + 19, 'C');      //
  EEPROM.put(CW_AUTO_DATA + 20, ' ');      //
  EEPROM.put(CW_AUTO_DATA + 21, 'E');      //
  EEPROM.put(CW_AUTO_DATA + 22, 'M');      //
  EEPROM.put(CW_AUTO_DATA + 23, '3');      //
  EEPROM.put(CW_AUTO_DATA + 24, '7');      //
  EEPROM.put(CW_AUTO_DATA + 25, ' ');      //
  EEPROM.put(CW_AUTO_DATA + 26, 'D');      //
  EEPROM.put(CW_AUTO_DATA + 27, 'E');      //
  EEPROM.put(CW_AUTO_DATA + 28, ' ');      //
  EEPROM.put(CW_AUTO_DATA + 29, 'C');      //
  EEPROM.put(CW_AUTO_DATA + 30, 'E');      //
  EEPROM.put(CW_AUTO_DATA + 31, 'C');      //
  EEPROM.put(CW_AUTO_DATA + 32, ' ');      //
  EEPROM.put(CW_AUTO_DATA + 33, 'K');      //
*/

/*
  EEPROM.put(CW_AUTO_DATA + 34, '<');      //
  EEPROM.put(CW_AUTO_DATA + 35, ' ');      //
  EEPROM.put(CW_AUTO_DATA + 36, '>');      //
  EEPROM.put(CW_AUTO_DATA + 37, ' ');      //
  EEPROM.put(CW_AUTO_DATA + 38, '7');      //
  EEPROM.put(CW_AUTO_DATA + 39, '3');      //
  EEPROM.put(CW_AUTO_DATA + 40, 'K');      //

  EEPROM.put(CW_AUTO_DATA + 41, 'C');      //
  EEPROM.put(CW_AUTO_DATA + 42, 'Q');      //
  EEPROM.put(CW_AUTO_DATA + 43, ' ');      //
  EEPROM.put(CW_AUTO_DATA + 44, '>');      // start "
  EEPROM.put(CW_AUTO_DATA + 45, ' ');      // end "
  EEPROM.put(CW_AUTO_DATA + 46, '>');      //
  EEPROM.put(CW_AUTO_DATA + 47, ' ');      //
  EEPROM.put(CW_AUTO_DATA + 48, 'K');      //
*/

/*
  //This is for auto key test2
  //USER CALL SIGN
  EEPROM.put(USER_CALLSIGN_KEY, 0x59);     //MAGIC KEY
  //EEPROM.put(USER_CALLSIGN_LEN, 10);           //WORD COUNT
  EEPROM.put(USER_CALLSIGN_LEN, 10 + 0x80);           //WORD COUNT
  
  EEPROM.put(USER_CALLSIGN_DAT + 1, 'K');      //
  EEPROM.put(USER_CALLSIGN_DAT + 2, 'D');      //
  EEPROM.put(USER_CALLSIGN_DAT + 3, '8');      //
  EEPROM.put(USER_CALLSIGN_DAT + 4, 'C');      //
  EEPROM.put(USER_CALLSIGN_DAT + 5, 'E');      //
  EEPROM.put(USER_CALLSIGN_DAT + 6, 'C');      //
  EEPROM.put(USER_CALLSIGN_DAT + 7, '/');      //
  EEPROM.put(USER_CALLSIGN_DAT + 8, 'A');      //
  EEPROM.put(USER_CALLSIGN_DAT + 9, 'B');      //
  EEPROM.put(USER_CALLSIGN_DAT + 10, 'C');      //

  //CW QSO CALLSIGN
  EEPROM.put(CW_STATION_LEN, 6);                //
  EEPROM.put(CW_STATION_LEN - 6 + 0 , 'A');     //
  EEPROM.put(CW_STATION_LEN - 6 + 1 , 'B');     //
  EEPROM.put(CW_STATION_LEN - 6 + 2 , '1');     //
  EEPROM.put(CW_STATION_LEN - 6 + 3 , 'C');     //
  EEPROM.put(CW_STATION_LEN - 6 + 4 , 'D');     //
  EEPROM.put(CW_STATION_LEN - 6 + 5 , 'E');     // 
*/
  
}


/**
 * The loop checks for keydown, ptt, function button and tuning.
 */
//for debug
int dbgCnt = 0;
byte flasher = 0;

void checkAutoSaveFreqMode()
{
  //when tx or ritOn, disable auto save
  if (inTx || ritOn)
    return;

  //detect change frequency
  if (saveCheckFreq != frequency)
  {
    saveCheckTime = millis();
    saveCheckFreq = frequency;
  }
  else if (saveCheckTime != 0)
  {
    //check time for Frequency auto save
    if (millis() - saveCheckTime > saveIntervalSec * 1000)
    {
      if (vfoActive == VFO_A)
      {
        vfoA = frequency;
        vfoA_mode = modeToByte();
        storeFrequencyAndMode(1);
      }
      else
      {
        vfoB = frequency;
        vfoB_mode = modeToByte();
        storeFrequencyAndMode(2);
      }
    }
  }
}

void loop(){ 
  if (isCWAutoMode == 0){  //when CW AutoKey Mode, disable this process
    if (!txCAT)
      checkPTT();
    checkButton();
  }
 else
    controlAutoCW();

  cwKeyer(); 
  
  //tune only when not tranmsitting 
  if (!inTx){
    if (ritOn)
      doRIT();
    else 
      doTuning();
  }

  //we check CAT after the encoder as it might put the radio into TX
  Check_Cat(inTx? 1 : 0);
  checkAutoSaveFreqMode();
}
