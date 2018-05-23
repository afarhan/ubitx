/*************************************************************************
  KD8CEC's uBITX Display Routine for LCD2004 Parrel & I2C
  1.This is the display code for the 20x04 LCD
  2.Some functions moved from uBITX_Ui.
-----------------------------------------------------------------------------
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

**************************************************************************/
#include "ubitx.h"
#include "ubitx_lcd.h"

//========================================================================
//Begin of TinyLCD Library by KD8CEC
//========================================================================

#ifdef UBITX_DISPLAY_LCD2004P
/*************************************************************************
  LCD2004TINY Library for 20 x 4 LCD
  Referecnce Source : LiquidCrystal.cpp 
  KD8CEC

  This source code is modified version for small program memory 
  from Arduino LiquidCrystal Library

  I wrote this code myself, so there is no license restriction. 
  So this code allows anyone to write with confidence.
  But keep it as long as the original author of the code.
  DE Ian KD8CEC
**************************************************************************/
#define LCD_Command(x)  (LCD_Send(x, LOW))
#define LCD_Write(x)    (LCD_Send(x, HIGH))

#define UBITX_DISPLAY_LCD2004_BASE

//Define  connected PIN
#define LCD_PIN_RS 8
#define LCD_PIN_EN 9
uint8_t LCD_PIN_DAT[4] = {10, 11, 12, 13};

void write4bits(uint8_t value) 
{
  for (int i = 0; i < 4; i++) 
    digitalWrite(LCD_PIN_DAT[i], (value >> i) & 0x01);

  digitalWrite(LCD_PIN_EN, LOW);
  delayMicroseconds(1);    
  digitalWrite(LCD_PIN_EN, HIGH);
  delayMicroseconds(1);    // enable pulse must be >450ns
  digitalWrite(LCD_PIN_EN, LOW);
  delayMicroseconds(100);   // commands need > 37us to settle
}

void LCD_Send(uint8_t value, uint8_t mode)
{
    digitalWrite(LCD_PIN_RS, mode);
    write4bits(value>>4);
    write4bits(value);
}

void LCD2004_Init()
{
  pinMode(LCD_PIN_RS, OUTPUT);
  pinMode(LCD_PIN_EN, OUTPUT);
  for (int i = 0; i < 4; i++)
    pinMode(LCD_PIN_DAT[i], OUTPUT);

  delayMicroseconds(50); 
 
  // Now we pull both RS and R/W low to begin commands
  digitalWrite(LCD_PIN_RS, LOW);
  digitalWrite(LCD_PIN_EN, LOW);

  // we start in 8bit mode, try to set 4 bit mode
  write4bits(0x03);
  delayMicroseconds(4500); // wait min 4.1ms
  
  // second try
  write4bits(0x03);
  delayMicroseconds(4500); // wait min 4.1ms
  
  // third go!
  write4bits(0x03); 
  delayMicroseconds(150);
  
  // finally, set to 4-bit interface
  write4bits(0x02);

  // finally, set # lines, font size, etc.
  LCD_Command(LCD_FUNCTIONSET | LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS | LCD_2LINE);  

  // turn the display on with no cursor or blinking default
  LCD_Command(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF);

  // clear it off
  LCD_Command(LCD_CLEARDISPLAY);  // clear display, set cursor position to zero
  delayMicroseconds(2000);  // this command takes a long time!

  LCD_Command(LCD_ENTRYMODESET | LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT);
}
#endif
//========================================================================
//End of TinyLCD Library by KD8CEC
//========================================================================



//========================================================================
//Begin of I2CTinyLCD Library by KD8CEC
//========================================================================
#ifdef UBITX_DISPLAY_LCD2004I

#include <Wire.h>
/*************************************************************************
  I2C Tiny LCD Library
  Referecnce Source : LiquidCrystal_I2C.cpp // Based on the work by DFRobot
  KD8CEC

  This source code is modified version for small program memory 
  from Arduino LiquidCrystal_I2C Library

  I wrote this code myself, so there is no license restriction. 
  So this code allows anyone to write with confidence.
  But keep it as long as the original author of the code.
  Ian KD8CEC
**************************************************************************/
#define UBITX_DISPLAY_LCD2004_BASE

#define En B00000100  // Enable bit
#define Rw B00000010  // Read/Write bit
#define Rs B00000001  // Register select bit

#define LCD_Command(x)  (LCD_Send(x, 0))
#define LCD_Write(x)    (LCD_Send(x, Rs))

uint8_t _Addr;
uint8_t _displayfunction;
uint8_t _displaycontrol;
uint8_t _displaymode;
uint8_t _numlines;
uint8_t _cols;
uint8_t _rows;
uint8_t _backlightval;

#define printIIC(args)  Wire.write(args)

void expanderWrite(uint8_t _data)
{
  Wire.beginTransmission(_Addr);
  printIIC((int)(_data) | _backlightval);
  Wire.endTransmission();   
}

void pulseEnable(uint8_t _data){
  expanderWrite(_data | En);  // En high
  delayMicroseconds(1);       // enable pulse must be >450ns
  
  expanderWrite(_data & ~En); // En low
  delayMicroseconds(50);      // commands need > 37us to settle
}

void write4bits(uint8_t value) 
{
  expanderWrite(value);
  pulseEnable(value);
}

void LCD_Send(uint8_t value, uint8_t mode)
{
  uint8_t highnib=value&0xf0;
  uint8_t lownib=(value<<4)&0xf0;
       write4bits((highnib)|mode);
  write4bits((lownib)|mode); 
}


// Turn the (optional) backlight off/on
void noBacklight(void) {
  _backlightval=LCD_NOBACKLIGHT;
  expanderWrite(0);
}

void backlight(void) {
  _backlightval=LCD_BACKLIGHT;
  expanderWrite(0);
}

void LCD2004_Init()
{
  //I2C Init
  _Addr = I2C_LCD_MASTER_ADDRESS;
  _cols = 20;
  _rows = 4;
  _backlightval = LCD_NOBACKLIGHT;
  Wire.begin();

  delay(50);
  
  // Now we pull both RS and R/W low to begin commands
  expanderWrite(_backlightval); // reset expanderand turn backlight off (Bit 8 =1)
  delay(1000);
      //put the LCD into 4 bit mode
  // this is according to the hitachi HD44780 datasheet
  // figure 24, pg 46
  
    // we start in 8bit mode, try to set 4 bit mode
   write4bits(0x03 << 4);
   delayMicroseconds(4500); // wait min 4.1ms
   
   // second try
   write4bits(0x03 << 4);
   delayMicroseconds(4500); // wait min 4.1ms
   
   // third go!
   write4bits(0x03 << 4); 
   delayMicroseconds(150);
   
   // finally, set to 4-bit interface
   write4bits(0x02 << 4); 

  // finally, set # lines, font size, etc.
  LCD_Command(LCD_FUNCTIONSET | LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS | LCD_2LINE);  

  // turn the display on with no cursor or blinking default
  LCD_Command(LCD_DISPLAYCONTROL | LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF);

  // clear it off
  LCD_Command(LCD_CLEARDISPLAY);  // clear display, set cursor position to zero
  //delayMicroseconds(2000);  // this command takes a long time!
  delayMicroseconds(1000);  // this command takes a long time!

  LCD_Command(LCD_ENTRYMODESET | LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT);

  backlight();
}
#endif
//========================================================================
//End of I2CTinyLCD Library by KD8CEC
//========================================================================


//========================================================================
// 20 X 04 LCD Routines
//Begin of Display Base Routines (Init, printLine..)
//========================================================================
#ifdef UBITX_DISPLAY_LCD2004_BASE

void LCD_Print(const char *c) 
{
  for (uint8_t i = 0; i < strlen(c); i++)
  {
    if (*(c + i) == 0x00) return;
    LCD_Write(*(c + i));
  }
}

const int row_offsets[] = { 0x00, 0x40, 0x14, 0x54 };
void LCD_SetCursor(uint8_t col, uint8_t row)
{
  LCD_Command(LCD_SETDDRAMADDR | (col + row_offsets[row]));  //0 : 0x00, 1 : 0x40, only for 20 x 4 lcd
}

void LCD_CreateChar(uint8_t location, uint8_t charmap[]) 
{
  location &= 0x7; // we only have 8 locations 0-7
  LCD_Command(LCD_SETCGRAMADDR | (location << 3));
  for (int i=0; i<8; i++)
    LCD_Write(charmap[i]);
}

//SWR GRAPH,  DrawMeter and drawingMeter Logic function by VK2ETA 
//#define OPTION_SKINNYBARS

char c[30], b[30];
char printBuff[4][21];  //mirrors what is showing on the two lines of the display

void LCD_Init(void)
{
  LCD2004_Init();  
  initMeter(); //for Meter Display
}


// The generic routine to display one line on the LCD 
void printLine(unsigned char linenmbr, const char *c) {
  if ((displayOption1 & 0x01) == 0x01)
    linenmbr = (linenmbr == 0 ? 1 : 0); //Line Toggle
  if (strcmp(c, printBuff[linenmbr])) {     // only refresh the display when there was a change
    LCD_SetCursor(0, linenmbr);             // place the cursor at the beginning of the selected line
    LCD_Print(c);
    strcpy(printBuff[linenmbr], c);

    for (byte i = strlen(c); i < 20; i++) { // add white spaces until the end of the 20 characters line is reached
      LCD_Write(' ');
    }
  }
}

void printLineF(char linenmbr, const __FlashStringHelper *c)
{
  int i;
  char tmpBuff[21];
  PGM_P p = reinterpret_cast<PGM_P>(c);  

  for (i = 0; i < 21; i++){
    unsigned char fChar = pgm_read_byte(p++);
    tmpBuff[i] = fChar;
    if (fChar == 0)
      break;
  }

  printLine(linenmbr, tmpBuff);
}

#define LCD_MAX_COLUMN 20
void printLineFromEEPRom(char linenmbr, char lcdColumn, byte eepromStartIndex, byte eepromEndIndex, char offsetTtype) {
  if ((displayOption1 & 0x01) == 0x01)
    linenmbr = (linenmbr == 0 ? 1 : 0); //Line Toggle
  
  LCD_SetCursor(lcdColumn, linenmbr);

  for (byte i = eepromStartIndex; i <= eepromEndIndex; i++)
  {
    if (++lcdColumn <= LCD_MAX_COLUMN)
      LCD_Write(EEPROM.read((offsetTtype == 0 ? USER_CALLSIGN_DAT : WSPR_MESSAGE1) + i));
    else
      break;
  }
  
  for (byte i = lcdColumn; i < 20; i++) //Right Padding by Space
      LCD_Write(' ');
}

//  short cut to print to the first line
void printLine1(const char *c)
{
  printLine(1,c);
}
//  short cut to print to the first line
void printLine2(const char *c)
{
  printLine(0,c);
}

void clearLine2()
{
  printLine2("");
  line2DisplayStatus = 0;
}

//  short cut to print to the first line
void printLine1Clear(){
  printLine(1,"");
}
//  short cut to print to the first line
void printLine2Clear(){
  printLine(0, "");
}

void printLine2ClearAndUpdate(){
  printLine(0, "");
  line2DisplayStatus = 0;  
  updateDisplay();
}

//==================================================================================
//End of Display Base Routines
//==================================================================================


//==================================================================================
//Begin of User Interface Routines
//==================================================================================

//Main Display
// this builds up the top line of the display with frequency and mode
void updateDisplay() {
  // tks Jack Purdum W8TEE
  // replaced fsprint commmands by str commands for code size reduction
  // replace code for Frequency numbering error (alignment, point...) by KD8CEC
  // i also Very TNX Purdum for good source code
  int i;
  unsigned long tmpFreq = frequency; //
  
  memset(c, 0, sizeof(c));

  if (inTx){
    if (isCWAutoMode == 2) {
      for (i = 0; i < 4; i++)
        c[3-i] = (i < autoCWSendReservCount ? byteToChar(autoCWSendReserv[i]) : ' ');

      //display Sending Index
      c[4] = byteToChar(sendingCWTextIndex);
      c[5] = '=';
    }
    else {
      if (cwTimeout > 0)
        strcpy(c, "   CW:");
      else
        strcpy(c, "   TX:");
    }
  }
  else {
    if (ritOn)
      strcpy(c, "RIT ");
    else {
      if (cwMode == 0)
      {
        if (isUSB)
          strcpy(c, "USB ");
        else
          strcpy(c, "LSB ");
      }
      else if (cwMode == 1)
      {
          strcpy(c, "CWL ");
      }
      else
      {
          strcpy(c, "CWU ");
      }
    }
    
    if (vfoActive == VFO_A) // VFO A is active
      strcat(c, "A:");
    else
      strcat(c, "B:");
  }

  //Fixed by Mitani Massaru (JE4SMQ)
  if (isShiftDisplayCWFreq == 1)
  {
    if (cwMode == 1)        //CWL
        tmpFreq = tmpFreq - sideTone + shiftDisplayAdjustVal;
    else if (cwMode == 2)   //CWU
        tmpFreq = tmpFreq + sideTone + shiftDisplayAdjustVal;
  }

  //display frequency
  for (int i = 15; i >= 6; i--) {
    if (tmpFreq > 0) {
      if (i == 12 || i == 8) c[i] = '.';
      else {
        c[i] = tmpFreq % 10 + 0x30;
        tmpFreq /= 10;
      }
    }
    else
      c[i] = ' ';
  }

  if (sdrModeOn)
    strcat(c, " SDR");
  else
    strcat(c, " SPK");

  //remarked by KD8CEC
  //already RX/TX status display, and over index (20 x 4 LCD)
  //if (inTx)
  //  strcat(c, " TX");
  printLine(1, c);

  byte diplayVFOLine = 1;
  if ((displayOption1 & 0x01) == 0x01)
    diplayVFOLine = 0;

  if ((vfoActive == VFO_A && ((isDialLock & 0x01) == 0x01)) ||
    (vfoActive == VFO_B && ((isDialLock & 0x02) == 0x02))) {
    LCD_SetCursor(5,diplayVFOLine);
    LCD_Write((uint8_t)0);
  }
  else if (isCWAutoMode == 2){
    LCD_SetCursor(5,diplayVFOLine);
    LCD_Write(0x7E);
  }
  else
  {
    LCD_SetCursor(5,diplayVFOLine);
    LCD_Write(':');
  }
}



char line2Buffer[20];
//KD8CEC 200Hz ST
//L14.150 200Hz ST
//U14.150 +150khz
int freqScrollPosition = 0;

//Example Line2 Optinal Display
//immediate execution, not call by scheulder
//warning : unused parameter 'displayType' <-- ignore, this is reserve
void updateLine2Buffer(char displayType)
{
  unsigned long tmpFreq = 0;
  if (ritOn)
  {
    strcpy(line2Buffer, "RitTX:");

    //display frequency
    tmpFreq = ritTxFrequency;

    //Fixed by Mitani Massaru (JE4SMQ)
    if (isShiftDisplayCWFreq == 1)
    {
      if (cwMode == 1)        //CWL
          tmpFreq = tmpFreq - sideTone + shiftDisplayAdjustVal;
      else if (cwMode == 2)   //CWU
          tmpFreq = tmpFreq + sideTone + shiftDisplayAdjustVal;
    }
    
    for (int i = 15; i >= 6; i--) {
      if (tmpFreq > 0) {
        if (i == 12 || i == 8) line2Buffer[i] = '.';
        else {
          line2Buffer[i] = tmpFreq % 10 + 0x30;
          tmpFreq /= 10;
        }
      }
      else
        line2Buffer[i] = ' ';
    }

    return;
  } //end of ritOn display

  //other VFO display
  if (vfoActive == VFO_B)
  {
    tmpFreq = vfoA;
  }
  else 
  {
    tmpFreq = vfoB;
  }

  // EXAMPLE 1 & 2
  //U14.150.100
  //display frequency
  for (int i = 9; i >= 0; i--) {
    if (tmpFreq > 0) {
      if (i == 2 || i == 6) line2Buffer[i] = '.';
      else {
        line2Buffer[i] = tmpFreq % 10 + 0x30;
        tmpFreq /= 10;
      }
    }
    else
      line2Buffer[i] = ' ';
  }
  
  memset(&line2Buffer[10], ' ', 10);
  
  if (isIFShift)
  {
    line2Buffer[6] = 'M';
    line2Buffer[7] = ' ';
    //IFShift Offset Value 
    line2Buffer[8] = 'I';
    line2Buffer[9] = 'F';

    line2Buffer[10] = ifShiftValue >= 0 ? '+' : 0;
    line2Buffer[11] = 0;
    line2Buffer[12] = ' ';
  
    //11, 12, 13, 14, 15
    memset(b, 0, sizeof(b));
    ltoa(ifShiftValue, b, DEC);
    strncat(line2Buffer, b, 5);

    for (int i = 12; i < 17; i++)
    {
      if (line2Buffer[i] == 0)
        line2Buffer[i] = ' ';
    }
  }       // end of display IF
  else    // step & Key Type display
  {
    //Step
    long tmpStep = arTuneStep[tuneStepIndex -1];
    
    byte isStepKhz = 0;
    if (tmpStep >= 1000)
    {
      isStepKhz = 2;
    }
      
    for (int i = 14; i >= 12 - isStepKhz; i--) {
      if (tmpStep > 0) {
          line2Buffer[i + isStepKhz] = tmpStep % 10 + 0x30;
          tmpStep /= 10;
      }
      else
        line2Buffer[i +isStepKhz] = ' ';
    }

    if (isStepKhz == 0)
    {
      line2Buffer[15] = 'H';
      line2Buffer[16] = 'z';
    }
  }

  line2Buffer[17] = ' ';
  
  //Check CW Key cwKeyType = 0; //0: straight, 1 : iambica, 2: iambicb
  if (cwKeyType == 0)
  {
    line2Buffer[18] = 'S';
    line2Buffer[19] = 'T';
  }
  else if (cwKeyType == 1)
  {
    line2Buffer[18] = 'I';
    line2Buffer[19] = 'A';
  }
  else
  {
    line2Buffer[18] = 'I';
    line2Buffer[19] = 'B';
  }

}

//meterType : 0 = S.Meter, 1 : P.Meter
void DisplayMeter(byte meterType, byte meterValue, char drawPosition)
{
  if (meterType == 0 || meterType == 1 || meterType == 2)
  {
    drawMeter(meterValue);
    
    LCD_SetCursor(drawPosition, 2);
    LCD_Write('S');
    LCD_Write(':');
    for (int i = 0; i < 7; i++) //meter 5 + +db 1 = 6
      LCD_Write(lcdMeter[i]);
  }
}

char checkCount = 0;
char checkCountSMeter = 0;

//execute interval : 0.25sec
void idle_process()
{
  //space for user graphic display
  if (menuOn == 0)
  {
    if ((displayOption1 & 0x10) == 0x10)    //always empty topline
      return;
      
    //if line2DisplayStatus == 0 <-- this condition is clear Line, you can display any message
    if (line2DisplayStatus == 0 || (((displayOption1 & 0x04) == 0x04) && line2DisplayStatus == 2)) {
      if (checkCount++ > 1)
      {
        updateLine2Buffer(0); //call by scheduler
        printLine2(line2Buffer);
        line2DisplayStatus = 2;
        checkCount = 0;
      }
    }

    //EX for Meters
    /*
    DisplayMeter(0, testValue++, 0);
    if (testValue > 30)
      testValue = 0;
    */

    //Sample
    //DisplayMeter(0, analogRead(ANALOG_SMETER) / 30, 0);
    //DisplayMeter(0, analogRead(ANALOG_SMETER) / 10, 0);
    //delay_background(10, 0);
    //DisplayMeter(0, analogRead(ANALOG_SMETER), 0);
    //if (testValue > 30)
    //  testValue = 0;

    //S-Meter Display
    if (((displayOption1 & 0x08) == 0x08 && (sdrModeOn == 0)) && (++checkCountSMeter > SMeterLatency))
    {
      int newSMeter;
      
      //VK2ETA S-Meter from MAX9814 TC pin
      newSMeter = analogRead(ANALOG_SMETER) / 4;
  
      //Faster attack, Slower release
      //currentSMeter = (newSMeter > currentSMeter ? ((currentSMeter * 3 + newSMeter * 7) + 5) / 10 : ((currentSMeter * 7 + newSMeter * 3) + 5) / 10);
      //currentSMeter = ((currentSMeter * 7 + newSMeter * 3) + 5) / 10;
      currentSMeter = newSMeter;
  
      scaledSMeter = 0;
      for (byte s = 8; s >= 1; s--) {
        if (currentSMeter > sMeterLevels[s]) {
          scaledSMeter = s;
          break;
        }
      }
  
      DisplayMeter(0, scaledSMeter, 0);
      checkCountSMeter = 0; //Reset Latency time
    } //end of S-Meter
    
  }
}

//AutoKey LCD Display Routine
void Display_AutoKeyTextIndex(byte textIndex)
{
  byte diplayAutoCWLine = 0;
  
  if ((displayOption1 & 0x01) == 0x01)
    diplayAutoCWLine = 1;
  LCD_SetCursor(0, diplayAutoCWLine);
  LCD_Write(byteToChar(textIndex));
  LCD_Write(':');
}

void DisplayCallsign(byte callSignLength)
{
  printLineFromEEPRom(3, 20 - userCallsignLength, 0, userCallsignLength -1, 0); //eeprom to lcd use offset (USER_CALLSIGN_DAT)
}

void DisplayVersionInfo(const __FlashStringHelper * fwVersionInfo)
{
  printLineF(3, fwVersionInfo);
}

#endif
