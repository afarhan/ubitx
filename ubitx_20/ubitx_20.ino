/**
 Since KD8CEC Version 0.29, most of the original code is no longer available.
 Most features(TX, Frequency Range, Ham Band, TX Control, CW delay, start Delay... more) have been added by KD8CEC.
 However, the license rules are subject to the original source rules.
 DE Ian KD8CEC

 Original source comment            -------------------------------------------------------------
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
#define ANALOG_SMETER (A7)  //by KD8CEC

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
#define CW_CAL 252
#define VFO_A_MODE 256
#define VFO_B_MODE 257
#define CW_DELAY 258
#define CW_START 259
#define HAM_BAND_COUNT 260    //
#define TX_TUNE_TYPE 261      //
#define HAM_BAND_RANGE 262    //FROM (2BYTE) TO (2BYTE) * 10 = 40byte
#define HAM_BAND_FREQS 302    //40, 1 BAND = 4Byte most bit is mode
#define TUNING_STEP    342   //TUNING STEP * 6 (index 1 + STEPS 5)  //1STEP : 
  

//for reduce cw key error, eeprom address
#define CW_ADC_MOST_BIT1 348   //most 2bits of  DOT_TO , DOT_FROM, ST_TO, ST_FROM
#define CW_ADC_ST_FROM   349   //CW ADC Range STRAIGHT KEY from (Lower 8 bit)
#define CW_ADC_ST_TO     350   //CW ADC Range STRAIGHT KEY to   (Lower 8 bit)
#define CW_ADC_DOT_FROM  351   //CW ADC Range DOT  from         (Lower 8 bit)
#define CW_ADC_DOT_TO    352   //CW ADC Range DOT  to           (Lower 8 bit)

#define CW_ADC_MOST_BIT2 353   //most 2bits of BOTH_TO, BOTH_FROM, DASH_TO, DASH_FROM
#define CW_ADC_DASH_FROM 354   //CW ADC Range DASH from         (Lower 8 bit)
#define CW_ADC_DASH_TO   355   //CW ADC Range DASH to           (Lower 8 bit)
#define CW_ADC_BOTH_FROM 356   //CW ADC Range BOTH from         (Lower 8 bit)
#define CW_ADC_BOTH_TO   357   //CW ADC Range BOTH to           (Lower 8 bit)
#define CW_KEY_TYPE      358

#define DISPLAY_OPTION1  361   //Display Option1
#define DISPLAY_OPTION2  362   //Display Option2

#define CHANNEL_FREQ    630   //Channel 1 ~ 20, 1 Channel = 4 bytes
#define CHANNEL_DESC    710   //Channel 1 ~ 20, 1 Channel = 4 bytes
#define RESERVE3        770   //Reserve3 between Channel and Firmware id check

//Check Firmware type and version
#define FIRMWAR_ID_ADDR 776 //776 : 0x59, 777 :0x58, 778 : 0x68 : Id Number, if not found id, erase eeprom(32~1023) for prevent system error.
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
unsigned long vfoA=7150000L, vfoB=14200000L, sideTone=800, usbCarrier, cwmCarrier;
unsigned long vfoA_eeprom, vfoB_eeprom; //for protect eeprom life
unsigned long frequency, ritRxFrequency, ritTxFrequency;  //frequency is the current frequency on the dial

unsigned int cwSpeed = 100; //this is actuall the dot period in milliseconds
extern int32_t calibration;

//for store the mode in eeprom
byte vfoA_mode=0, vfoB_mode = 0;          //0: default, 1:not use, 2:LSB, 3:USB, 4:CW, 5:AM, 6:FM
byte vfoA_mode_eeprom, vfoB_mode_eeprom;  //for protect eeprom life

//KD8CEC
//for AutoSave and protect eeprom life
byte saveIntervalSec = 10;  //second
unsigned long saveCheckTime = 0;
unsigned long saveCheckFreq = 0;

byte cwDelayTime = 60;
byte delayBeforeCWStartTime = 50;

//sideTonePitch + sideToneSub = sideTone
byte sideTonePitch=0;
byte sideToneSub = 0;

//DialLock
byte isDialLock = 0;  //000000[0]vfoB [0]vfoA 0Bit : A, 1Bit : B
byte isTxType = 0;    //000000[0 - isSplit] [0 - isTXStop]
long arTuneStep[5];
byte tuneStepIndex; //default Value 0, start Offset is 0 because of check new user

byte displayOption1 = 0;
byte displayOption2 = 0;

//CW ADC Range
int cwAdcSTFrom = 0;
int cwAdcSTTo = 0;
int cwAdcDotFrom = 0;
int cwAdcDotTo = 0;
int cwAdcDashFrom = 0;
int cwAdcDashTo = 0;
int cwAdcBothFrom = 0;
int cwAdcBothTo = 0;
byte cwKeyType = 0; //0: straight, 1 : iambica, 2: iambicb
bool Iambic_Key = true;
#define IAMBICB 0x10 // 0 for Iambic A, 1 for Iambic B
unsigned char keyerControl = IAMBICB;

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
char splitOn = 0;             //working split, uses VFO B as the transmit frequency
char keyDown = 0;             //in cw mode, denotes the carrier is being transmitted
char isUSB = 0;               //upper sideband was selected, this is reset to the default for the 

char cwMode = 0;              //compatible original source, and extend mode //if cwMode == 0, mode check : isUSB, cwMode > 0, mode Check : cwMode
                              //iscwMode = 0 : ssbmode, 1 :cwl, 2 : cwu, 3 : cwn (none tx)
                              
                              //frequency when it crosses the frequency border of 10 MHz
byte menuOn = 0;              //set to 1 when the menu is being displayed, if a menu item sets it to zero, the menu is exited
unsigned long cwTimeout = 0;  //milliseconds to go before the cw transmit line is released and the radio goes back to rx mode
unsigned long dbgCount = 0;   //not used now
unsigned char txFilter = 0;   //which of the four transmit filters are in use
boolean modeCalibrate = false;//this mode of menus shows extended menus to calibrate the oscillators and choose the proper
                              //beat frequency

unsigned long beforeIdle_ProcessTime = 0; //for check Idle time
byte line2DisplayStatus = 0;  //0:Clear, 1 : menu, 1: DisplayFrom Idle, 
char lcdMeter[17];

byte isIFShift = 0;     //1 = ifShift, 2 extend
long ifShiftValue = 0;  //
                              
/**
 * Below are the basic functions that control the uBitx. Understanding the functions before 
 * you start hacking around
 */

//Ham Band
#define MAX_LIMIT_RANGE 10  //because limited eeprom size
byte useHamBandCount = 0;  //0 use full range frequency
byte tuneTXType = 0;      //0 : use full range, 1 : just Change Dial speed, 2 : just ham band change, but can general band by tune, 3 : only ham band (just support 0, 2 (0.26 version))
                          //100 : use full range but not TX on general band, 101 : just change dial speed but.. 2 : jut... but.. 3 : only ham band  (just support 100, 102 (0.26 version))
unsigned int hamBandRange[MAX_LIMIT_RANGE][2];  // =  //Khz because reduce use memory

//-1 : not found, 0 ~ 9 : Hamband index
char getIndexHambanBbyFreq(unsigned long f)
{
  f = f / 1000;
  for (byte i = 0; i < useHamBandCount; i++)
    if (hamBandRange[i][0] <= f && f < hamBandRange[i][1])
      return i;
      
  return -1;
}

//when Band change step = just hamband
//moveDirection : 1 = next, -1 : prior
void setNextHamBandFreq(unsigned long f, char moveDirection)
{
  unsigned long resultFreq = 0;
  byte loadMode = 0;
  char findedIndex = getIndexHambanBbyFreq(f);

  if (findedIndex == -1) {  //out of hamband
    f = f / 1000;
    for (byte i = 0; i < useHamBandCount -1; i++) {
      if (hamBandRange[i][1] <= f && f < hamBandRange[i + 1][0]) {
        findedIndex = i + moveDirection;
        //return (unsigned long)(hamBandRange[i + 1][0]) * 1000;
      }
    } //end of for
  }
  else if (((moveDirection == 1) && (findedIndex < useHamBandCount -1)) ||  //Next
    ((moveDirection == -1) && (findedIndex > 0)) ) {                        //Prior
    findedIndex += moveDirection;
  }
  else
    findedIndex = -1;
    
  if (findedIndex == -1)
    findedIndex = (moveDirection == 1 ? 0 : useHamBandCount -1);

  EEPROM.get(HAM_BAND_FREQS + 4 * findedIndex, resultFreq);
  
  //loadMode = (byte)(resultFreq >> 30);
  //resultFreq = resultFreq & 0x3FFFFFFF;
  loadMode = (byte)(resultFreq >> 29);
  resultFreq = resultFreq & 0x1FFFFFFF;
  
  if ((resultFreq / 1000) < hamBandRange[(unsigned char)findedIndex][0] || (resultFreq / 1000) > hamBandRange[(unsigned char)findedIndex][1])
    resultFreq = (unsigned long)(hamBandRange[(unsigned char)findedIndex][0]) * 1000;

  setFrequency(resultFreq);
  byteToMode(loadMode, 1);
}

void saveBandFreqByIndex(unsigned long f, unsigned long mode, char bandIndex) {
  if (bandIndex >= 0)
    //EEPROM.put(HAM_BAND_FREQS + 4 * bandIndex, (f & 0x3FFFFFFF) | (mode << 30) );
    EEPROM.put(HAM_BAND_FREQS + 4 * bandIndex, (f & 0x1FFFFFFF) | (mode << 29) );
}

/*
  KD8CEC
  When using the basic delay of the Arduino, the program freezes.
  When the delay is used, the program will generate an error because it is not communicating, 
  so Create a new delay function that can do background processing.
 */
 
unsigned long delayBeforeTime = 0;
byte delay_background(unsigned delayTime, byte fromType){ //fromType : 4 autoCWKey -> Check Paddle
  delayBeforeTime = millis();

  while (millis() - delayBeforeTime <= delayTime) {

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
  f = (f / arTuneStep[tuneStepIndex -1]) * arTuneStep[tuneStepIndex -1];
  
  setTXFilters(f);

  if (cwMode == 0)
  {
    if (isUSB){
      si5351bx_setfreq(2, SECOND_OSC_USB - usbCarrier + f  + (isIFShift ? ifShiftValue : 0));
      si5351bx_setfreq(1, SECOND_OSC_USB);
    }
    else{
      si5351bx_setfreq(2, SECOND_OSC_LSB + usbCarrier + f + (isIFShift ? ifShiftValue : 0));
      si5351bx_setfreq(1, SECOND_OSC_LSB);
    }
  }
  else
  {
    if (cwMode == 1){ //CWL
      si5351bx_setfreq(2, SECOND_OSC_LSB + cwmCarrier + f + (isIFShift ? ifShiftValue : 0));
      si5351bx_setfreq(1, SECOND_OSC_LSB);
    }
    else{             //CWU
      si5351bx_setfreq(2, SECOND_OSC_USB - cwmCarrier + f + (isIFShift ? ifShiftValue : 0));
      si5351bx_setfreq(1, SECOND_OSC_USB);
    }
  }
  
  frequency = f;
}

/**
 * startTx is called by the PTT, cw keyer and CAT protocol to
 * put the uBitx in tx mode. It takes care of rit settings, sideband settings
 * Note: In cw mode, doesnt key the radio, only puts it in tx mode
 */
 
void startTx(byte txMode, byte isDisplayUpdate){
  //Check Hamband only TX //Not found Hamband index by now frequency
  if (tuneTXType >= 100 && getIndexHambanBbyFreq(ritOn ? ritTxFrequency :  frequency) == -1) {
    //no message
    return;
  }

  if ((isTxType & 0x01) != 0x01)
    digitalWrite(TX_RX, 1);

  inTx = 1;
  
  if (ritOn){
    //save the current as the rx frequency
    ritRxFrequency = frequency;
    setFrequency(ritTxFrequency);
  }
  else if (splitOn == 1) {
      if (vfoActive == VFO_B) {
        vfoActive = VFO_A;
        frequency = vfoA;
        byteToMode(vfoA_mode, 0);
      }
      else if (vfoActive == VFO_A){
        vfoActive = VFO_B;
        frequency = vfoB;
        byteToMode(vfoB_mode, 0);
      }

      setFrequency(frequency);
  } //end of else
  

  if (txMode == TX_CW){
    //turn off the second local oscillator and the bfo
    si5351bx_setfreq(0, 0);
    si5351bx_setfreq(1, 0);

    //shif the first oscillator to the tx frequency directly
    //the key up and key down will toggle the carrier unbalancing
    //the exact cw frequency is the tuned frequency + sidetone

    if (cwMode == 0)
    {
      if (isUSB)
        si5351bx_setfreq(2, frequency + sideTone);
      else
        si5351bx_setfreq(2, frequency - sideTone); 
    }
    else if (cwMode == 1) //CWL
    {
        si5351bx_setfreq(2, frequency - sideTone); 
    }
    else  //CWU
    {
        si5351bx_setfreq(2, frequency + sideTone);
    }
  }

  //reduce latency time when begin of CW mode
  if (isDisplayUpdate == 1)
    updateDisplay();
}

void stopTx(){
  inTx = 0;

  digitalWrite(TX_RX, 0);           //turn off the tx

  if (cwMode == 0)
    si5351bx_setfreq(0, usbCarrier + (isIFShift ? ifShiftValue : 0));  //set back the carrier oscillator anyway, cw tx switches it off
  else
    si5351bx_setfreq(0, cwmCarrier + (isIFShift ? ifShiftValue : 0));  //set back the carrier oscillator anyway, cw tx switches it off

  if (ritOn)
    setFrequency(ritRxFrequency);
  else if (splitOn == 1) {
      //vfo Change
      if (vfoActive == VFO_B){
        vfoActive = VFO_A;
        frequency = vfoA;
        byteToMode(vfoA_mode, 0);
      }
      else if (vfoActive == VFO_A){
        vfoActive = VFO_B;
        frequency = vfoB;
        byteToMode(vfoB_mode, 0);
      }
      setFrequency(frequency);
  } //end of else
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


/************************************
Replace function by KD8CEC
prevent error controls
applied Threshold for reduct errors,  dial Lock, dynamic Step
 *************************************/
byte threshold = 2;  //noe action for count
unsigned long lastEncInputtime = 0;
int encodedSumValue = 0;
unsigned long lastTunetime = 0; //if continous moving, skip threshold processing
byte lastMovedirection = 0; //0 : stop, 1 : cw, 2 : ccw

#define skipThresholdTime 100
#define encodeTimeOut 1000

void doTuningWithThresHold(){
  int s = 0;
  unsigned long prev_freq;

  if ((vfoActive == VFO_A && ((isDialLock & 0x01) == 0x01)) ||
    (vfoActive == VFO_B && ((isDialLock & 0x02) == 0x02)))
    return;

  if (isCWAutoMode == 0 || cwAutoDialType == 1)
    s = enc_read();

  //if time is exceeded, it is recognized as an error,
  //ignore exists values, because of errors
  if (s == 0) {
    if (encodedSumValue != 0 && (millis() - encodeTimeOut) > lastEncInputtime)
      encodedSumValue = 0;

    lastMovedirection = 0;
    return;
  }
  lastEncInputtime = millis();

  //for check moving direction
  encodedSumValue += (s > 0 ? 1 : -1);

  //check threshold and operator actions (hold dial speed = continous moving, skip threshold check)
  if ((lastTunetime < millis() - skipThresholdTime) && ((encodedSumValue *  encodedSumValue) <= (threshold * threshold)))
    return;

  lastTunetime = millis();

  //Valid Action without noise
  encodedSumValue = 0;

  prev_freq = frequency;
  //incdecValue = tuningStep * s;
  frequency += (arTuneStep[tuneStepIndex -1] * s * (s * s < 10 ? 1 : 3));  //appield weight (s is speed)
    
  if (prev_freq < 10000000l && frequency > 10000000l)
    isUSB = true;
    
  if (prev_freq > 10000000l && frequency < 10000000l)
    isUSB = false;
    
  setFrequency(frequency);
  updateDisplay();
}

/**
 * RIT only steps back and forth by 100 hz at a time
 */
void doRIT(){
  int knob = enc_read();
  unsigned long old_freq = frequency;

  if (knob < 0)
    frequency -= (arTuneStep[tuneStepIndex -1]);  //
    //frequency -= 100l;
  else if (knob > 0)
    frequency += (arTuneStep[tuneStepIndex -1]);  //
    //frequency += 100;
 
  if (old_freq != frequency){
    setFrequency(frequency);
    updateDisplay();
  }
}
/*
 save Frequency and mode to eeprom for Auto Save with protected eeprom cycle, by kd8cec
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

//calculate step size from 1 byte, compatible uBITX Manager, by KD8CEC
unsigned int byteToSteps(byte srcByte) {
    byte powerVal = (byte)(srcByte >> 6);
    unsigned int baseVal = srcByte & 0x3F;

    if (powerVal == 1)
        return baseVal * 10;
    else if (powerVal == 2)
        return baseVal * 100;
    else if (powerVal == 3)
        return baseVal * 1000;
    else
        return baseVal;
}


/**
 * The settings are read from EEPROM. The first time around, the values may not be 
 * present or out of range, in this case, some intelligent defaults are copied into the 
 * variables.
 */
void initSettings(){
  //read the settings from the eeprom and restore them
  //if the readings are off, then set defaults
  //for original source Section ===========================
  EEPROM.get(MASTER_CAL, calibration); 
  EEPROM.get(USB_CAL, usbCarrier);
  EEPROM.get(VFO_A, vfoA);
  EEPROM.get(VFO_B, vfoB);
  EEPROM.get(CW_SIDETONE, sideTone);
  EEPROM.get(CW_SPEED, cwSpeed);
  //End of original code
  
  //----------------------------------------------------------------
  //Add Lines by KD8CEC
  //for custom source Section =============================
  //ID & Version Check from EEProm 
  //if found different firmware, erase eeprom (32
  #define FIRMWAR_ID_ADDR 776 //776 : 0x59, 777 :0x58, 778 : 0x68 : Id Number, if not found id, erase eeprom(32~1023) for prevent system error.
  if (EEPROM.read(FIRMWAR_ID_ADDR) != 0x59 || 
    EEPROM.read(FIRMWAR_ID_ADDR + 1) != 0x58 || 
    EEPROM.read(FIRMWAR_ID_ADDR + 2) != 0x68 ) {
      
    printLineF(1, F("Init EEProm...")); 
      //initial all eeprom 
    for (unsigned int i = 32; i < 1024; i++) //protect Master_cal, usb_cal
      EEPROM.write(i, 0);

    //Write Firmware ID
    EEPROM.write(FIRMWAR_ID_ADDR, 0x59);
    EEPROM.write(FIRMWAR_ID_ADDR + 1, 0x58);
    EEPROM.write(FIRMWAR_ID_ADDR + 2, 0x68);
  }
  
  //Version Write for Memory Management Software
  if (EEPROM.read(VERSION_ADDRESS) != VERSION_NUM)
    EEPROM.write(VERSION_ADDRESS, VERSION_NUM);

  EEPROM.get(CW_CAL, cwmCarrier);

  //for Save VFO_A_MODE to eeprom
  //0: default, 1:not use, 2:LSB, 3:USB, 4:CW, 5:AM, 6:FM
  EEPROM.get(VFO_A_MODE, vfoA_mode);
  EEPROM.get(VFO_B_MODE, vfoB_mode);

  //CW DelayTime
  EEPROM.get(CW_DELAY, cwDelayTime);

  //CW interval between TX and CW Start
  EEPROM.get(CW_START, delayBeforeCWStartTime);
  EEPROM.get(CW_KEY_TYPE, cwKeyType);
  if (cwKeyType > 2)
    cwKeyType = 0;

  if (cwKeyType == 0)
    Iambic_Key = false;
  else
  {
    Iambic_Key = true;
    if (cwKeyType == 1)
      keyerControl &= ~IAMBICB;
    else
      keyerControl |= IAMBICB;
  }
    

  EEPROM.get(DISPLAY_OPTION1, displayOption1);
  EEPROM.get(DISPLAY_OPTION2, displayOption2);

  //User callsign information
  if (EEPROM.read(USER_CALLSIGN_KEY) == 0x59)
    userCallsignLength = EEPROM.read(USER_CALLSIGN_LEN);  //MAXIMUM 18 LENGTH

  //Ham Band Count
  EEPROM.get(HAM_BAND_COUNT, useHamBandCount);
  EEPROM.get(TX_TUNE_TYPE, tuneTXType);

  byte findedValidValueCount = 0;
    
  //Read band Information
  for (byte i = 0; i < useHamBandCount; i++) {
    unsigned int tmpReadValue = 0;
    EEPROM.get(HAM_BAND_RANGE + 4 * i, tmpReadValue);
    hamBandRange[i][0] = tmpReadValue;

    if (tmpReadValue > 1 && tmpReadValue < 55000)
      findedValidValueCount++;

    EEPROM.get(HAM_BAND_RANGE + 4 * i + 2, tmpReadValue);
    hamBandRange[i][1] = tmpReadValue;
  }

  //Check Value Range and default Set for new users
  if ((3 < tuneTXType && tuneTXType < 100) || 103 < tuneTXType || useHamBandCount < 1 || findedValidValueCount < 5)
  {
    tuneTXType = 2;
    //if empty band Information, auto insert default region 2 frequency range
    //This part is made temporary for people who have difficulty setting up, so can remove it when you run out of memory.
    useHamBandCount = 10;
    hamBandRange[0][0] = 1810; hamBandRange[0][1] = 2000; 
    hamBandRange[1][0] = 3500; hamBandRange[1][1] = 3800; 
    hamBandRange[2][0] = 5351; hamBandRange[2][1] = 5367; 
    hamBandRange[3][0] = 7000; hamBandRange[3][1] = 7300;     //region 2
    hamBandRange[4][0] = 10100; hamBandRange[4][1] = 10150; 
    hamBandRange[5][0] = 14000; hamBandRange[5][1] = 14350; 
    hamBandRange[6][0] = 18068; hamBandRange[6][1] = 18168; 
    hamBandRange[7][0] = 21000; hamBandRange[7][1] = 21450; 
    hamBandRange[8][0] = 24890; hamBandRange[8][1] = 24990; 
    hamBandRange[9][0] = 28000; hamBandRange[9][1] = 29700; 
  }
  

  //Read Tuning Step Index, and steps
  findedValidValueCount = 0;
  EEPROM.get(TUNING_STEP, tuneStepIndex);
  for (byte i = 0; i < 5; i++) {
    arTuneStep[i] = byteToSteps(EEPROM.read(TUNING_STEP + i + 1));
    if (arTuneStep[i] >= 1 && arTuneStep[i] <= 60000) //Maximum 650 for check valid Value
       findedValidValueCount++;
  }

  //Check Value Range and default Set for new users
  if (findedValidValueCount < 5)
  {
    //Default Setting
    arTuneStep[0] = 10;
    arTuneStep[1] = 20;
    arTuneStep[2] = 50;
    arTuneStep[3] = 100;
    arTuneStep[4] = 200;
  }

  if (tuneStepIndex == 0) //New User
    tuneStepIndex = 3;

  //CW Key ADC Range ======= adjust set value for reduce cw keying error
  //by KD8CEC
  unsigned int tmpMostBits = 0;
  tmpMostBits = EEPROM.read(CW_ADC_MOST_BIT1);
  cwAdcSTFrom = EEPROM.read(CW_ADC_ST_FROM)   | ((tmpMostBits & 0x03) << 8);
  cwAdcSTTo = EEPROM.read(CW_ADC_ST_TO)       | ((tmpMostBits & 0x0C) << 6);
  cwAdcDotFrom = EEPROM.read(CW_ADC_DOT_FROM) | ((tmpMostBits & 0x30) << 4);
  cwAdcDotTo = EEPROM.read(CW_ADC_DOT_TO)     | ((tmpMostBits & 0xC0) << 2);
  
  tmpMostBits = EEPROM.read(CW_ADC_MOST_BIT2);
  cwAdcDashFrom = EEPROM.read(CW_ADC_DASH_FROM) | ((tmpMostBits & 0x03) << 8);
  cwAdcDashTo = EEPROM.read(CW_ADC_DASH_TO)     | ((tmpMostBits & 0x0C) << 6);
  cwAdcBothFrom = EEPROM.read(CW_ADC_BOTH_FROM) | ((tmpMostBits & 0x30) << 4);
  cwAdcBothTo = EEPROM.read(CW_ADC_BOTH_TO)     | ((tmpMostBits & 0xC0) << 2);

  //default Value (for original hardware)
  if (cwAdcSTFrom >= cwAdcSTTo)
  {
    cwAdcSTFrom = 0;
    cwAdcSTTo = 50;
  }

  if (cwAdcBothFrom >= cwAdcBothTo)
  {
    cwAdcBothFrom = 51;
    cwAdcBothTo = 300;
  }
  
  if (cwAdcDotFrom >= cwAdcDotTo)
  {
    cwAdcDotFrom = 301;
    cwAdcDotTo = 600;
  }
  if (cwAdcDashFrom >= cwAdcDashTo)
  {
    cwAdcDashFrom = 601;
    cwAdcDashTo = 800;
  }
  //end of CW Keying Variables
  
  if (cwDelayTime < 1 || cwDelayTime > 250)
    cwDelayTime = 60;

  if (vfoA_mode < 2)
    vfoA_mode = 2;
  
  if (vfoB_mode < 2)
    vfoB_mode = 3;

  //original code with modified by kd8cec
  if (usbCarrier > 12010000l || usbCarrier < 11990000l)
    usbCarrier = 11995000l;

  if (cwmCarrier > 12010000l || cwmCarrier < 11990000l)
    cwmCarrier = 11995000l;
    
  if (vfoA > 35000000l || 3500000l > vfoA) {
     vfoA = 7150000l;
     vfoA_mode = 2; //LSB
  }
  
  if (vfoB > 35000000l || 3500000l > vfoB) {
     vfoB = 14150000l;  
     vfoB_mode = 3; //USB
  }
  //end of original code section

  //for protect eeprom life by KD8CEC
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
  pinMode(ANALOG_SMETER, INPUT); //by KD8CEC

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
  /*
  //Init EEProm for Fault EEProm TEST and Factory Reset
  //please remove remark for others.
  //for (int i = 0; i < 1024; i++)
  for (int i = 16; i < 1024; i++) //protect Master_cal, usb_cal
    EEPROM.write(i, 0xFF);
  lcd.begin(16, 2);
  printLineF(1, F("Complete Erase")); 
  sleep(1000);
  //while(1);
  //end section of test
  */
  
  //Serial.begin(9600);
  lcd.begin(16, 2);
  printLineF(1, F("CECBT v1.01")); 

  Init_Cat(38400, SERIAL_8N1);
  initMeter(); //not used in this build
  initSettings();

  if (userCallsignLength > 0 && ((userCallsignLength & 0x80) == 0x80)) {
    userCallsignLength = userCallsignLength & 0x7F;
    printLineFromEEPRom(0, 0, 0, userCallsignLength -1); //eeprom to lcd use offset (USER_CALLSIGN_DAT)
    delay(500);
  }
  else {
    printLineF(0, F("uBITX v0.20")); 
    delay(500);
    clearLine2();
  }
  
  initPorts();     

  byteToMode(vfoA_mode, 0);
  initOscillators();

  frequency = vfoA;
  saveCheckFreq = frequency;  //for auto save frequency
  setFrequency(vfoA);
  updateDisplay();

  if (btnDown())
    factory_alignment();
}

//Auto save Frequency and Mode with Protected eeprom life by KD8CEC
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
      FrequencyToVFO(1);
      saveCheckTime = 0;  //for reduce cpu use rate
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
    //else if (isIFShift)
    //  doIFShift();
    else 
      doTuningWithThresHold();

    if (isCWAutoMode == 0 && beforeIdle_ProcessTime < millis() - 250) {
      idle_process();
      checkAutoSaveFreqMode();  //move here form out scope for reduce cpu use rate
      beforeIdle_ProcessTime = millis();
    }
  } //end of check TX Status

  //we check CAT after the encoder as it might put the radio into TX
  Check_Cat(inTx? 1 : 0);
}
