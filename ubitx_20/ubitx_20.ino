//Firmware Version
//+ : This symbol identifies the firmware. 
//    It was originally called 'CEC V1.072' but it is too long to waste the LCD window.
//    I do not want to make this Firmware users's uBITX messy with my callsign.
//    Putting one alphabet in front of 'v' has a different meaning.
//    So I put + in the sense that it was improved one by one based on Original Firmware.
//    This firmware has been gradually changed based on the original firmware created by Farhan, Jack, Jerry and others.

#define FIRMWARE_VERSION_INFO F("+v1.094")  
#define FIRMWARE_VERSION_NUM 0x04       //1st Complete Project : 1 (Version 1.061), 2st Project : 2, 1.08: 3, 1.09 : 4

/**
 Cat Suppoort uBITX CEC Version
 This firmware has been gradually changed based on the original firmware created by Farhan, Jack, Jerry and others.
 Most features(TX, Frequency Range, Ham Band, TX Control, CW delay, start Delay... more) have been added by KD8CEC.
 My wish is to keep the original author's Comment as long as the meaning does not change much, even if the code looks a bit long.
 Ian KD8CEC

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
#include "ubitx.h"
#include "ubitx_eemap.h"

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

byte commonOption0 = 0;
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

byte isShiftDisplayCWFreq = 1;  //Display Frequency 
int shiftDisplayAdjustVal = 0;  //

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
                              
byte advancedFreqOption1;     //255 : Bit0: use IFTune_Value, Bit1 : use Stored enabled SDR Mode, Bit2~Bit3 : dynamic sdr frequency,  bit 7: IFTune_Value Reverse for DIY uBITX
byte attLevel = 0;            //ATT : RF Gain Control (Receive) <-- IF1 Shift, 0 : Off, ShiftValue is attLevel * 100; attLevel 150 = 15K
byte if1TuneValue = 0;        //0 : OFF, IF1 + if1TuneValue * 100; // + - 12500;
byte sdrModeOn = 0;           //SDR MODE ON / OFF
unsigned long SDR_Center_Freq; //

unsigned long beforeIdle_ProcessTime = 0; //for check Idle time
byte line2DisplayStatus = 0;  //0:Clear, 1 : menu, 1: DisplayFrom Idle, 
char lcdMeter[17];
byte sMeterLevels[9];

//Current ADC Value for S.Meter, and S Meter Level
int currentSMeter = 0;
byte scaledSMeter = 0;

byte I2C_LCD_MASTER_ADDRESS;        //0x27  //if Set I2C Address by uBITX Manager, read from EEProm
byte I2C_LCD_SECOND_ADDRESS;         //only using Dual LCD Mode

byte KeyValues[16][3];

byte isIFShift = 0;     //1 = ifShift, 2 extend
int ifShiftValue = 0;   //

byte TriggerBySW = 0;   //Action Start from Nextion LCD, Other MCU
                              
/**
 * Below are the basic functions that control the uBitx. Understanding the functions before 
 * you start hacking around
 */

//Ham Band
#define MAX_LIMIT_RANGE 10  //because limited eeprom size
byte useHamBandCount = 0;   //0 use full range frequency
byte tuneTXType = 0;        //0 : use full range, 1 : just Change Dial speed, 2 : just ham band change, but can general band by tune, 3 : only ham band (just support 0, 2 (0.26 version))
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

  byteToMode(loadMode, 1);
  setFrequency(resultFreq);
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

  unsigned long appliedCarrier = ((cwMode == 0 ? usbCarrier : cwmCarrier) + (isIFShift && (inTx == 0) ? ifShiftValue : 0));
  int appliedTuneValue = 0;

  //applied if tune 
  //byte advancedFreqOption1;     //255 : Bit0: use IFTune_Value, Bit1 : use Stored enabled SDR Mode, Bit2 : dynamic sdr frequency0, Bit3 : dynamic sdr frequency1, bit 7: IFTune_Value Reverse for DIY uBITX
  if ((advancedFreqOption1 & 0x01) != 0x00)
  {
    appliedTuneValue = if1TuneValue;

    //In the LSB state, the optimum reception value was found. To apply to USB, 3Khz decrease is required.
    if (sdrModeOn && (inTx == 0))
      appliedTuneValue -= 15; //decrease 1.55Khz
      
    //if (isUSB)
    if (cwMode == 2 || (cwMode == 0 && (isUSB)))    
      appliedTuneValue -= 30; //decrease 3Khz
  }

   //if1Tune RX, TX Enabled, ATT : only RX Mode
  //The IF Tune shall be measured at the LSB. Then, move the 3Khz down for USB.
  long if1AdjustValue = ((inTx == 0) ? (attLevel * 100) : 0) + (appliedTuneValue * 100); //if1Tune RX, TX Enabled, ATT : only RX Mode  //5600

  //for DIY uBITX (custom filter)
  if ((advancedFreqOption1 & 0x80) != 0x00)  //Reverse IF Tune (- Value for DIY uBITX)
    if1AdjustValue *= -1;
    
  if (sdrModeOn && (inTx == 0))  //IF SDR MODE
  {
    //Fixed Frequency SDR (Default Frequency : 32Mhz, available change sdr Frequency by uBITX Manager)
    //Dynamic Frequency is for SWL without cat

    //byte advancedFreqOption1;     //255 : Bit0: use IFTune_Value, Bit1 : use Stored enabled SDR Mode, Bit2 : dynamic sdr frequency0, Bit3 : dynamic sdr frequency1, bit 7: IFTune_Value Reverse for DIY uBITX
    long moveFrequency = 0;
    //7 6 5 4 3 2 1 0
    //        _ _     <-- SDR Freuqncy Option
    byte sdrOption = (advancedFreqOption1 >> 2) & 0x03;

    if (sdrOption == 1) // SDR Frequency + frequenc
    {
      //example : offset Freq : 20 Mhz and frequency = 7.080 => 27.080 Mhz
      //example : offset Freq : 0 Mhz and frequency = 7.080 => 7.080 Mhz
      //for available HF, SDR
      moveFrequency = f;
    }
    else if (sdrOption == 2)  //Mhz move
    {
      //Offset Frequency + Mhz, 
      //Example : Offset Frequency : 30Mhz and current Frequncy is 7.080 => 37.080Mhz
      //          Offset Frequency : 30Mhz and current Frequncy is 14.074 => 34.074Mhz
      moveFrequency = (f % 10000000);
    }
    else if (sdrOption == 3)  //Khz move
    {
      //Offset Frequency + Khz, 
      //Example : Offset Frequency : 30Mhz and current Frequncy is 7.080 => 30.080Mhz
      //          Offset Frequency : 30Mhz and current Frequncy is 14.074 => 30.074Mhz
      moveFrequency = (f % 1000000);
    }

    si5351bx_setfreq(2, 44991500 + if1AdjustValue + f);
    si5351bx_setfreq(1, 44991500 
      + if1AdjustValue 
      + SDR_Center_Freq 
      //+ ((advancedFreqOption1 & 0x04) == 0x00 ? 0 : (f % 10000000))
      + moveFrequency
      + 2390);
  }
  else
  {
    if (cwMode == 1 || (cwMode == 0 && (!isUSB))) //cwl or lsb
    {
      //CWL(cwMode == 1) or LSB (cwMode == 0 && (!isUSB))
      si5351bx_setfreq(2, SECOND_OSC_LSB + if1AdjustValue + appliedCarrier + f);
      si5351bx_setfreq(1, SECOND_OSC_LSB + if1AdjustValue);
    }
    else  //cwu or usb
    {
      //CWU (cwMode == 2) or USB (cwMode == 0 and isUSB)
      si5351bx_setfreq(2, SECOND_OSC_USB + if1AdjustValue - appliedCarrier + f);
      si5351bx_setfreq(1, SECOND_OSC_USB + if1AdjustValue);
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
  else 
  {
    if (splitOn == 1) {
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
    }

    setFrequency(frequency);
  } //end of else

  SetCarrierFreq();

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

void stopTx(void){
  inTx = 0;

  digitalWrite(TX_RX, 0);           //turn off the tx
  SetCarrierFreq();

  if (ritOn)
    setFrequency(ritRxFrequency);
  else
  {
    if (splitOn == 1) {
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
    }
    
    setFrequency(frequency);
  } //end of else
  
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
#ifdef EXTEND_KEY_GROUP1  
void checkButton(){
  char currentBandIndex = -1;
  
  //only if the button is pressed
  int keyStatus = getBtnStatus();
  if (keyStatus == -1)
    return;
    
  delay(50);
  keyStatus = getBtnStatus();   //will be remove 3 lines
  if (keyStatus == -1)
    return;
    
  if (keyStatus == FKEY_PRESS)  //Menu Key
  {
    //for touch screen
#ifdef USE_SW_SERIAL
    SetSWActivePage(1);
    doMenu();

    if (isCWAutoMode == 0)
          SetSWActivePage(0);
#else
    doMenu();
#endif    
  }
  else if (keyStatus <= FKEY_TYPE_MAX)  //EXTEND KEY GROUP #1
  {

    switch(keyStatus)
    {
      case FKEY_MODE :
        if (cwMode == 1)
        {
          cwMode = 2;
        }
        else if (cwMode == 2)
        {
          cwMode = 0;
          isUSB = 0;
        }
        else if (isUSB == 0)
        {
          isUSB = 1;
        }
        else
        {
          cwMode = 1;
        }
        break;
      case FKEY_BANDUP :
      case FKEY_BANDDOWN :
        //Save Band Information
        if (tuneTXType == 2 || tuneTXType == 3 || tuneTXType == 102 || tuneTXType == 103) { //only ham band move
          currentBandIndex = getIndexHambanBbyFreq(frequency);
          
          if (currentBandIndex >= 0) {
            saveBandFreqByIndex(frequency, modeToByte(), currentBandIndex);
          }
        }
        setNextHamBandFreq(frequency, keyStatus == FKEY_BANDDOWN ? -1 : 1);  //Prior Band      
        break;

      case FKEY_STEP :
        if (++tuneStepIndex > 5)
          tuneStepIndex = 1;
  
        EEPROM.put(TUNING_STEP, tuneStepIndex);
        printLine2ClearAndUpdate();
        break;

      case FKEY_VFOCHANGE :
        menuVfoToggle(1); //Vfo Toggle
        break;
      
      case FKEY_SPLIT :
        menuSplitOnOff(1);
        break;
      case  FKEY_TXOFF:
        menuTxOnOff(1, 0x01);
        break;
      case  FKEY_SDRMODE :
         menuSDROnOff(1);
        break;
      case FKEY_RIT :
        menuRitToggle(1);
        break;
    }
    /*
    if (keyStatus == FKEY_MODE) //Press Mode Key
    {
      if (cwMode == 1)
      {
        cwMode = 2;
      }
      else if (cwMode == 2)
      {
        cwMode = 0;
        isUSB = 0;
      }
      else if (isUSB == 0)
      {
        isUSB = 1;
      }
      else
      {
        cwMode = 1;
      }
    }
    else if (keyStatus == FKEY_BANDUP || keyStatus == FKEY_BANDDOWN)  //Press Mode Key
    {

      char currentBandIndex = -1;
      
      //Save Band Information
      if (tuneTXType == 2 || tuneTXType == 3 || tuneTXType == 102 || tuneTXType == 103) { //only ham band move
        currentBandIndex = getIndexHambanBbyFreq(frequency);
        
        if (currentBandIndex >= 0) {
          saveBandFreqByIndex(frequency, modeToByte(), currentBandIndex);
        }
      }
      
      setNextHamBandFreq(frequency, keyStatus == FKEY_BANDDOWN ? -1 : 1);  //Prior Band      
    }
    else if (keyStatus == FKEY_STEP)  //FKEY_BANDUP
    {
      if (++tuneStepIndex > 5)
        tuneStepIndex = 1;

      EEPROM.put(TUNING_STEP, tuneStepIndex);
      printLine2ClearAndUpdate();
    }

    else if (keyStatus == FKEY_VFOCHANGE)
    {
      menuVfoToggle(1); //Vfo Toggle
    }
    else if (keyStatus == FKEY_SPLIT)
    {
      menuSplitOnOff(1);
    }
    else if (keyStatus == FKEY_TXOFF)
    {
      menuTxOnOff(1, 0x01);
    }
    else if (keyStatus == FKEY_SDRMODE)
    {
      menuSDROnOff(1);
    }
    else if (keyStatus == FKEY_RIT)
    {
      menuRitToggle(1);
    }
    */
      
    FrequencyToVFO(1);
    SetCarrierFreq();
    setFrequency(frequency);
    //delay_background(delayTime, 0);
    updateDisplay();
  }
  
  //wait for the button to go up again
  while(keyStatus == getBtnStatus()) {
    delay(10);
    Check_Cat(0);
  }
  //delay(50);//debounce
}

#else
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
  //delay(50);//debounce
}
#endif

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

//#define skipThresholdTime 70
#define encodeTimeOut 1000

void doTuningWithThresHold(){
  int s = 0;
  unsigned long prev_freq;

  if ((vfoActive == VFO_A && ((isDialLock & 0x01) == 0x01)) ||
    (vfoActive == VFO_B && ((isDialLock & 0x02) == 0x02)))
    return;

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
  //not use continues changing by Threshold
  //if ((lastTunetime < (millis() - skipThresholdTime)) && ((encodedSumValue *  encodedSumValue) <= (threshold * threshold)))
  if (((encodedSumValue *  encodedSumValue) <= (threshold * threshold)))
    return;

  lastTunetime = millis();

  //Valid Action without noise
  encodedSumValue = 0;

  prev_freq = frequency;
  //incdecValue = tuningStep * s;
  //frequency += (arTuneStep[tuneStepIndex -1] * s * (s * s < 10 ? 1 : 3));  //appield weight (s is speed)
  frequency += (arTuneStep[tuneStepIndex -1] * s);  //appield weight (s is speed) //if want need more increase size, change step size
    
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
  else if (knob > 0)
    frequency += (arTuneStep[tuneStepIndex -1]);  //
 
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
    for (unsigned int i = 64; i < 1024; i++) //protect Master_cal, usb_cal
      EEPROM.write(i, 0);

    //Write Firmware ID
    EEPROM.write(FIRMWAR_ID_ADDR, 0x59);
    EEPROM.write(FIRMWAR_ID_ADDR + 1, 0x58);
    EEPROM.write(FIRMWAR_ID_ADDR + 2, 0x68);
  }
  
  //Version Write for Memory Management Software
  if (EEPROM.read(VERSION_ADDRESS) != FIRMWARE_VERSION_NUM)
    EEPROM.write(VERSION_ADDRESS, FIRMWARE_VERSION_NUM);

  //Backup Calibration Setting from Factory Setup
  //Check Factory Setting Backup Y/N
  if (EEPROM.read(FACTORY_BACKUP_YN) != 0x13) {
    EEPROM.write(FACTORY_BACKUP_YN, 0x13);  //Set Backup Y/N
    
    for (unsigned int i = 0; i < 32; i++) //factory setting range
      EEPROM.write(FACTORY_VALUES + i, EEPROM.read(i)); //0~31 => 65~96
  }

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

  EEPROM.get(COMMON_OPTION0, commonOption0);
  EEPROM.get(DISPLAY_OPTION1, displayOption1);
  EEPROM.get(DISPLAY_OPTION2, displayOption2);

  for (byte i = 0; i < 8; i++) {
    sMeterLevels[i + 1] = EEPROM.read(S_METER_LEVELS + i);
  }

  //KeyValues
  for (byte i = 0; i < 16; i++) {
    KeyValues[i][0] = EEPROM.read(EXTENDED_KEY_RANGE + (i * 3));        //RANGE : Start Value
    KeyValues[i][1] = EEPROM.read(EXTENDED_KEY_RANGE + (i * 3) + 1);    //RANGE : End Value
    KeyValues[i][2] = EEPROM.read(EXTENDED_KEY_RANGE + (i * 3) + 2);    //KEY TYPE 
  }

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
    arTuneStep[1] = 50;
    arTuneStep[2] = 100;
    arTuneStep[3] = 500;
    arTuneStep[4] = 1000;
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

  //Display Type for CW mode
  isShiftDisplayCWFreq = EEPROM.read(CW_DISPLAY_SHIFT);

  //Enable / Diable Check for CW Display Cofiguration Group 
  if ((commonOption0 & 0x80) != 0x00)
  {
    //Adjust CW Mode Freq
    shiftDisplayAdjustVal = (isShiftDisplayCWFreq & 0x3F) * 10;
  
    //check Minus
    if ((isShiftDisplayCWFreq & 0x40) == 0x40)
      shiftDisplayAdjustVal = shiftDisplayAdjustVal * -1;
  
   //Shift Display Check (Default : 0)
    if ((isShiftDisplayCWFreq & 0x80) == 0)  //Enabled
      isShiftDisplayCWFreq = 1;
    else    //Disabled
      isShiftDisplayCWFreq = 0;
  }

   //Stored IF Shift Option
  if ((commonOption0 & 0x40) != 0x00)
  {
    EEPROM.get(IF_SHIFTVALUE, ifShiftValue);
    isIFShift = ifShiftValue != 0;
  }

  //Advanced Freq control
  EEPROM.get(ADVANCED_FREQ_OPTION1, advancedFreqOption1);

  //byte advancedFreqOption1;     //255 : Bit0: use IFTune_Value, Bit1 : use Stored enabled SDR Mode, Bit2 : dynamic sdr frequency0, Bit3 : dynamic sdr frequency1, bit 7: IFTune_Value Reverse for DIY uBITX
  if ((advancedFreqOption1 & 0x01) != 0x00)
  {
    EEPROM.get(IF1_CAL, if1TuneValue);

    //Stored Enabled SDR Mode
    if ((advancedFreqOption1 & 0x02) != 0x00)
    {
      EEPROM.get(ENABLE_SDR, sdrModeOn);
    }
  }
  
  EEPROM.get(SDR_FREQUNCY, SDR_Center_Freq);
  //if (SDR_Center_Freq == 0)
  //  SDR_Center_Freq = 32000000;

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
    usbCarrier = 11997000l;

  if (cwmCarrier > 12010000l || cwmCarrier < 11990000l)
    cwmCarrier = 11997000l;
    
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

//Recovery Factory Setting Values 
void factory_Recovery()
{
  if (EEPROM.read(FACTORY_BACKUP_YN) != 0x13)
    return;

  if (digitalRead(PTT) == 0)  //Do not proceed if PTT is pressed to prevent malfunction.
    return;
    
  printLineF2(F("Factory Recovery"));
  delay(2000);
  if (!btnDown())
    return;

  printLineF2(F("IF you continue"));
  printLineF1(F("release the key"));
  delay(2000);
  if (btnDown())
    return;
  
  printLineF1(F("Press Key PTT"));
  delay(2000);
  if (digitalRead(PTT) == 0)
  {
    for (unsigned int i = 0; i < 32; i++) //factory setting range
      EEPROM.write(i, EEPROM.read(FACTORY_VALUES + i)); //65~96 => 0~31

    //printLineF2(F("CompleteRecovery"));
    printLineF1(F("Power Reset!"));
    while(1);   //Hold 
  }
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

  //Load I2C LCD Address for I2C LCD 
  //I2C LCD Parametere
#ifdef USE_I2C_LCD  
  EEPROM.get(I2C_LCD_MASTER, I2C_LCD_MASTER_ADDRESS);
  EEPROM.get(I2C_LCD_SECOND, I2C_LCD_SECOND_ADDRESS);

  if (I2C_LCD_MASTER_ADDRESS < 0x10 || I2C_LCD_MASTER_ADDRESS > 0xF0)
    I2C_LCD_MASTER_ADDRESS = I2C_LCD_MASTER_ADDRESS_DEFAULT;
    
  if (I2C_LCD_SECOND_ADDRESS < 0x10 || I2C_LCD_SECOND_ADDRESS > 0xF0)
    I2C_LCD_SECOND_ADDRESS = I2C_LCD_SECOND_ADDRESS_DEFAULT;
#endif  
  
  //Serial.begin(9600);
  LCD_Init();
  //printLineF(1, FIRMWARE_VERSION_INFO);
  DisplayVersionInfo(FIRMWARE_VERSION_INFO);

  Init_Cat(38400, SERIAL_8N1);
  initSettings();
  initPorts();     

  if (userCallsignLength > 0 && ((userCallsignLength & 0x80) == 0x80)) {
    userCallsignLength = userCallsignLength & 0x7F;
    DisplayCallsign(userCallsignLength);
  }
  else {
    printLineF(0, F("uBITX v0.20")); 
    delay(500);
    clearLine2();
  }
  

#ifdef FACTORY_RECOVERY_BOOTUP
  if (btnDown())
    factory_Recovery();
#endif

  byteToMode(vfoA_mode, 0);
  initOscillators();

  frequency = vfoA;
  saveCheckFreq = frequency;  //for auto save frequency
  setFrequency(vfoA);

#ifdef USE_SW_SERIAL
  SendUbitxData();
#endif
  
  updateDisplay();

#ifdef ENABLE_FACTORYALIGN
  if (btnDown())
    factory_alignment();
#endif

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
    if (isCWAutoMode == 0 || cwAutoDialType == 1)
    {
      if (ritOn)
        doRIT();
      else 
        doTuningWithThresHold();
    }

    if (isCWAutoMode == 0 && beforeIdle_ProcessTime < millis() - 250) {
      idle_process();
      checkAutoSaveFreqMode();  //move here form out scope for reduce cpu use rate
      beforeIdle_ProcessTime = millis();
    }
  } //end of check TX Status

  //we check CAT after the encoder as it might put the radio into TX
  Check_Cat(inTx? 1 : 0);

  //for SEND SW Serial
  #ifdef USE_SW_SERIAL
    SWS_Process();
  #endif  
}
