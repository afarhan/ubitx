/*************************************************************************
  KD8CEC, _______
  uBITX Display Routine for LCD1602 I2C

  1.Code for 16 x 2 LCD for I2C.
  2.Display related functions of uBITX.  Some functions moved from uBITX_Ui.
  3.uBITX Idle time Processing
    Functions that run at times that do not affect TX, CW, and CAT
    It is called in 1/10 time unit.
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
#ifdef UBITX_DISPLAY_LCD1602I


//========================================================================
//Begin of LCD Hardware define
//========================================================================
#include <LiquidCrystal.h>
LiquidCrystal lcd(8,9,10,11,12,13);


//========================================================================
//End of LCD Hardware define
//========================================================================

//========================================================================
//Begin of Display Base Routines (Init, printLine..)
//========================================================================
char c[30], b[30];
char printBuff[2][17];  //mirrors what is showing on the two lines of the display

const PROGMEM uint8_t meters_bitmap[] = {
  B10000,  B10000,  B10000,  B10000,  B10000,  B10000,  B10000,  B10000 ,   //custom 1
  B11000,  B11000,  B11000,  B11000,  B11000,  B11000,  B11000,  B11000 ,   //custom 2
  B11100,  B11100,  B11100,  B11100,  B11100,  B11100,  B11100,  B11100 ,   //custom 3
  B11110,  B11110,  B11110,  B11110,  B11110,  B11110,  B11110,  B11110 ,   //custom 4
  B11111,  B11111,  B11111,  B11111,  B11111,  B11111,  B11111,  B11111 ,   //custom 5
  B01000,  B11100,  B01000,  B00000,  B10111,  B10101,  B10101,  B10111     //custom 6
};

PGM_P p_metes_bitmap = reinterpret_cast<PGM_P>(meters_bitmap);

const PROGMEM uint8_t lock_bitmap[8] = {
  0b01110,
  0b10001,
  0b10001,
  0b11111,
  0b11011,
  0b11011,
  0b11111,
  0b00000};
PGM_P plock_bitmap = reinterpret_cast<PGM_P>(lock_bitmap);


// initializes the custom characters
// we start from char 1 as char 0 terminates the string!
void initMeter(){
  uint8_t tmpbytes[8];
  byte i;

  for (i = 0; i < 8; i++)
    tmpbytes[i] = pgm_read_byte(plock_bitmap + i);
  lcd.createChar(0, tmpbytes);
  
  for (i = 0; i < 8; i++)
    tmpbytes[i] = pgm_read_byte(p_metes_bitmap + i);
  lcd.createChar(1, tmpbytes);

  for (i = 0; i < 8; i++)
    tmpbytes[i] = pgm_read_byte(p_metes_bitmap + i + 8);
  lcd.createChar(2, tmpbytes);
  
  for (i = 0; i < 8; i++)
    tmpbytes[i] = pgm_read_byte(p_metes_bitmap + i + 16);
  lcd.createChar(3, tmpbytes);
  
  for (i = 0; i < 8; i++)
    tmpbytes[i] = pgm_read_byte(p_metes_bitmap + i + 24);
  lcd.createChar(4, tmpbytes);
  
  for (i = 0; i < 8; i++)
    tmpbytes[i] = pgm_read_byte(p_metes_bitmap + i + 32);
  lcd.createChar(5, tmpbytes);
  
  for (i = 0; i < 8; i++)
    tmpbytes[i] = pgm_read_byte(p_metes_bitmap + i + 40);
  lcd.createChar(6, tmpbytes);
}

void LCD_Init(void)
{
  lcd.begin(16, 2);
  initMeter(); //for Meter Display
}

//by KD8CEC
//0 ~ 25 : 30 over : + 10
void drawMeter(int needle) {
  //5Char + O over
  int i;

  for (i = 0; i < 5; i++) {
    if (needle >= 5)
      lcdMeter[i] = 5; //full
    else if (needle > 0)
      lcdMeter[i] = needle; //full
    else  //0
      lcdMeter[i] = 0x20;
    
    needle -= 5;
  }

  if (needle > 0)
    lcdMeter[5] = 6;
  else
    lcdMeter[5] = 0x20;
}



// The generic routine to display one line on the LCD 
void printLine(unsigned char linenmbr, const char *c) {
  if ((displayOption1 & 0x01) == 0x01)
    linenmbr = (linenmbr == 0 ? 1 : 0); //Line Toggle
    
  if (strcmp(c, printBuff[linenmbr])) {     // only refresh the display when there was a change
    lcd.setCursor(0, linenmbr);             // place the cursor at the beginning of the selected line
    lcd.print(c);
    strcpy(printBuff[linenmbr], c);

    for (byte i = strlen(c); i < 16; i++) { // add white spaces until the end of the 16 characters line is reached
      lcd.write(' ');
    }
  }
}

void printLineF(char linenmbr, const __FlashStringHelper *c)
{
  int i;
  char tmpBuff[17];
  PGM_P p = reinterpret_cast<PGM_P>(c);  

  for (i = 0; i < 17; i++){
    unsigned char fChar = pgm_read_byte(p++);
    tmpBuff[i] = fChar;
    if (fChar == 0)
      break;
  }

  printLine(linenmbr, tmpBuff);
}

#define LCD_MAX_COLUMN 16
void printLineFromEEPRom(char linenmbr, char lcdColumn, byte eepromStartIndex, byte eepromEndIndex, char offsetTtype) {
  if ((displayOption1 & 0x01) == 0x01)
    linenmbr = (linenmbr == 0 ? 1 : 0); //Line Toggle
  
  lcd.setCursor(lcdColumn, linenmbr);

  for (byte i = eepromStartIndex; i <= eepromEndIndex; i++)
  {
    if (++lcdColumn <= LCD_MAX_COLUMN)
      lcd.write(EEPROM.read((offsetTtype == 0 ? USER_CALLSIGN_DAT : WSPR_MESSAGE1) + i));
    else
      break;
  }
  
  for (byte i = lcdColumn; i < 16; i++) //Right Padding by Space
      lcd.write(' ');
}

//  short cut to print to the first line
void printLine1(const char *c){
  printLine(1,c);
}
//  short cut to print to the first line
void printLine2(const char *c){
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
//===================================================================================
//End of Display Base Routines
//===================================================================================

//===================================================================================
//Begin of User Interface Routines
//===================================================================================

// this builds up the top line of the display with frequency and mode
void updateDisplay() {
  // tks Jack Purdum W8TEE
  // replaced fsprint commmands by str commands for code size reduction
  // replace code for Frequency numbering error (alignment, point...) by KD8CEC
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

  //remarked by KD8CEC
  //already RX/TX status display, and over index (16 x 2 LCD)
  //if (inTx)
  //  strcat(c, " TX");
  printLine(1, c);

  byte diplayVFOLine = 1;
  if ((displayOption1 & 0x01) == 0x01)
    diplayVFOLine = 0;

  if ((vfoActive == VFO_A && ((isDialLock & 0x01) == 0x01)) ||
    (vfoActive == VFO_B && ((isDialLock & 0x02) == 0x02))) {
    lcd.setCursor(5,diplayVFOLine);
    lcd.write((uint8_t)0);
  }
  else if (isCWAutoMode == 2){
    lcd.setCursor(5,diplayVFOLine);
    lcd.write(0x7E);
  }
  else
  {
    lcd.setCursor(5,diplayVFOLine);
    lcd.write(':');
  }
}



char line2Buffer[16];
//KD8CEC 200Hz ST
//L14.150 200Hz ST
//U14.150 +150khz
int freqScrollPosition = 0;
//Example Line2 Optinal Display
//immediate execution, not call by scheulder
void updateLine2Buffer(char displayType)
{
  unsigned long tmpFreq = 0;
  if (ritOn)
  {
    strcpy(line2Buffer, "RitTX:");

    //display frequency
    tmpFreq = ritTxFrequency;
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

  //======================================================
  //other VFO display
  //======================================================
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

  //EXAMPLE #1
  if ((displayOption1 & 0x04) == 0x00)  //none scroll display
    line2Buffer[6] = 'k';
  else
  {
    //example #2
    if (freqScrollPosition++ > 18)    //none scroll display time
    {
      line2Buffer[6] = 'k';
      if (freqScrollPosition > 25)
        freqScrollPosition = -1;
    }
    else                              //scroll frequency 
    {
      line2Buffer[10] = 'H';
      line2Buffer[11] = 'z';
  
      if (freqScrollPosition < 7)   
      {
        for (int i = 11; i >= 0; i--)
          if (i - (7 - freqScrollPosition) >= 0)
            line2Buffer[i] = line2Buffer[i - (7 - freqScrollPosition)];
          else
            line2Buffer[i] = ' ';
      }
      else
      {
        for (int i = 0; i < 11; i++)
          if (i + (freqScrollPosition - 7) <= 11)
            line2Buffer[i] = line2Buffer[i + (freqScrollPosition - 7)];
          else
            line2Buffer[i] = ' ';
      }
    }
  } //scroll
  
  line2Buffer[7] = ' ';
  
  if (isIFShift)
  {
//    if (isDirectCall == 1)
//      for (int i = 0; i < 16; i++)
//        line2Buffer[i] = ' ';
      
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
    
    //if (isDirectCall == 1)  //if call by encoder (not scheduler), immediate print value
    printLine2(line2Buffer);    
  }       // end of display IF
  else    // step & Key Type display
  {
    //if (isDirectCall != 0)
    //  return;

    memset(&line2Buffer[8], ' ', 8);
    //Step
    long tmpStep = arTuneStep[tuneStepIndex -1];
    
    byte isStepKhz = 0;
    if (tmpStep >= 1000)
    {
      isStepKhz = 2;
    }
      
    for (int i = 10; i >= 8 - isStepKhz; i--) {
      if (tmpStep > 0) {
          line2Buffer[i + isStepKhz] = tmpStep % 10 + 0x30;
          tmpStep /= 10;
      }
      else
        line2Buffer[i +isStepKhz] = ' ';
    }

    if (isStepKhz == 0)
    {
      line2Buffer[11] = 'H';
      line2Buffer[12] = 'z';
    }
  
    line2Buffer[13] = ' ';
    
    //Check CW Key cwKeyType = 0; //0: straight, 1 : iambica, 2: iambicb
    if (cwKeyType == 0)
    {
      line2Buffer[14] = 'S';
      line2Buffer[15] = 'T';
    }
    else if (cwKeyType == 1)
    {
      line2Buffer[14] = 'I';
      line2Buffer[15] = 'A';
    }
    else
    {
      line2Buffer[14] = 'I';
      line2Buffer[15] = 'B';
    }    
  }
}

//meterType : 0 = S.Meter, 1 : P.Meter
void DisplayMeter(byte meterType, byte meterValue, char drawPosition)
{
  if (meterType == 0 || meterType == 1 || meterType == 2)
  {
    drawMeter(meterValue);  //call original source code
    int lineNumber = 0;
    if ((displayOption1 & 0x01) == 0x01)
      lineNumber = 1;
    
    lcd.setCursor(drawPosition, lineNumber);
  
    for (int i = 0; i < 6; i++) //meter 5 + +db 1 = 6
      lcd.write(lcdMeter[i]);
  }
}

byte testValue = 0;
char checkCount = 0;
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

      //EX for Meters
      /*
      DisplayMeter(0, testValue++, 7);
      if (testValue > 30)
        testValue = 0;
      */
    }
  }
}

void Display_AutoKeyTextIndex(char textIndex)
{
  byte diplayAutoCWLine = 0;
  
  if ((displayOption1 & 0x01) == 0x01)
    diplayAutoCWLine = 1;
  lcd.setCursor(0, diplayAutoCWLine);
  lcd.write(byteToChar(selectedCWTextIndex));
  lcd.write(':');
}


#endif
