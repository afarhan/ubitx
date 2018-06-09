/*************************************************************************
  KD8CEC's uBITX Display Routine for LCD2004 Parrel
  1.This is the display code for the default LCD mounted in uBITX.
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
#include "ubitx.h"
#include "ubitx_lcd.h"

//======================================================================== 
//Begin of Nextion LCD Library by KD8CEC
//========================================================================
#ifdef UBITX_DISPLAY_NEXTION
/*************************************************************************
  Nextion Library for 20 x 4 LCD
  KD8CEC
**************************************************************************/
//#include <SoftwareSerial.h>
//SoftwareSerial softSerial(8, 9); // RX, TX
extern void SWSerial_Begin(long speedBaud);
extern void SWSerial_Write(uint8_t b);
extern int SWSerial_Available(void);
extern int SWSerial_Read(void);
extern void SWSerial_Print(uint8_t *b);

#define TEXT_LINE_LENGTH 20
char softBuffLines[2][TEXT_LINE_LENGTH + 1];
char softBuffSended[2][TEXT_LINE_LENGTH + 1];
char softBuffTemp[TEXT_LINE_LENGTH + 1];    //for STR Command

char c[30], b[30];
char softBuff[20];
char softTemp[20];

void LCD2004_Init()
{
  SWSerial_Begin(9600);
  memset(softBuffLines[0], ' ', TEXT_LINE_LENGTH); 
  softBuffLines[0][TEXT_LINE_LENGTH + 1] = 0x00;
  memset(softBuffLines[1], ' ', TEXT_LINE_LENGTH);
  softBuffLines[1][TEXT_LINE_LENGTH + 1] = 0x00;
}

void LCD_CreateChar(uint8_t location, uint8_t charmap[]) 
{
}

void LCD_Init(void)
{
  LCD2004_Init();  
  initMeter(); //for Meter Display
}

//===================================================================
//Begin of Nextion LCD Protocol
//
// v0~v9, va~vz : Numeric (Transceiver -> Nextion LCD)
// s0~s9  : String (Text) (Transceiver -> Nextion LCD)
// vlSendxxx, vloxxx: Reserve for Nextion (Nextion LCD -> Transceiver)
//
//===================================================================
#define CMD_NOW_DISP      '0' //c0
char L_nowdisp = -1;          //Sended nowdisp

#define CMD_VFO_TYPE      'v' //cv
char L_vfoActive;             //vfoActive

#define CMD_CURR_FREQ     'c' //vc
unsigned long L_vfoCurr;      //vfoA
#define CMD_CURR_MODE     'c' //cc
byte L_vfoCurr_mode;          //vfoA_mode

#define CMD_VFOA_FREQ     'a' //va
unsigned long L_vfoA;         //vfoA
#define CMD_VFOA_MODE     'a' //ca
byte L_vfoA_mode;             //vfoA_mode

#define CMD_VFOB_FREQ     'b' //vb
unsigned long L_vfoB;         //vfoB
#define CMD_VFOB_MODE     'b' //cb
byte L_vfoB_mode;             //vfoB_mode

#define CMD_IS_RIT        'r' //cr
char L_ritOn;
#define CMD_RIT_FREQ      'r' //vr
unsigned long L_ritTxFrequency; //ritTxFrequency

#define CMD_IS_TX         't' //ct
char L_inTx;

#define CMD_IS_DIALLOCK   'l' //cl
byte L_isDialLock;            //byte isDialLock

#define CMD_IS_SPLIT      's' //cs
byte  L_Split;            //isTxType
#define CMD_IS_TXSTOP     'x' //cx
byte  L_TXStop;           //isTxType

#define CMD_TUNEINDEX     'n' //cn
byte L_tuneStepIndex;     //byte tuneStepIndex

#define CMD_SMETER        'p' //cs
byte L_scaledSMeter;      //scaledSMeter

#define CMD_SIDE_TONE     't' //vt
unsigned long L_sideTone; //sideTone
#define CMD_KEY_TYPE      'k' //ck
byte L_cwKeyType;          //L_cwKeyType 0: straight, 1 : iambica, 2: iambicb

#define CMD_CW_SPEED      's' //vs
unsigned int L_cwSpeed;   //cwSpeed

#define CMD_CW_DELAY      'y' //vy
byte L_cwDelayTime;       //cwDelayTime

#define CMD_CW_STARTDELAY 'e' //ve
byte L_delayBeforeCWStartTime;  //byte delayBeforeCWStartTime

#define CMD_ATT_LEVEL     'f' //vf
byte L_attLevel;

byte L_isIFShift;             //1 = ifShift, 2 extend
#define CMD_IS_IFSHIFT    'i' //ci

int L_ifShiftValue;
#define CMD_IFSHIFT_VALUE 'i' //vi

byte L_sdrModeOn;
#define CMD_SDR_MODE      'j' //cj

#define CMD_UBITX_INFO     'm' //cm  Complete Send uBITX Information

//Once Send Data, When boot
//arTuneStep, When boot, once send
//long arTuneStep[5];
#define CMD_AR_TUNE1      '1' //v1
#define CMD_AR_TUNE2      '2' //v2
#define CMD_AR_TUNE3      '3' //v3
#define CMD_AR_TUNE4      '4' //v4
#define CMD_AR_TUNE5      '5' //v5


#define CMD_IS_CW_SHIFT_DISPLAY 'h' //ch
byte L_isShiftDisplayCWFreq;  //byte isShiftDisplayCWFreq

#define CMD_CW_SHIFT_ADJUST     'h' //vh
int L_shiftDisplayAdjustVal;        //int shiftDisplayAdjustVal

//0:CW Display Shift Confirm, 1 : IFshift save
#define CMD_COMM_OPTION     'o'     //vo
byte L_commonOption0;         //byte commonOption0

//0:Line Toggle, 1 : Always display Callsign, 2 : scroll display, 3 : s.meter
#define CMD_DISP_OPTION1    'p'   //vp
byte L_displayOption1;            //byte displayOption1
#define CMD_DISP_OPTION2    'q'   //vq
byte L_displayOption2;            //byte displayOption2 (Reserve)

#define CMD_TEXT_LINE0      '0'   //s0
#define CMD_TEXT_LINE1      '1'   //s1

#define CMD_CW_TEXT         'a'   //sa
#define CMD_CALLSIGN        'c'   //sc
#define CMD_VERSION         'v'   //sv

char nowdisp = 0;

#define SWS_HEADER_CHAR_TYPE 'c'  //1Byte Protocol Prefix
#define SWS_HEADER_INT_TYPE  'v'  //Numeric Protocol Prefex
#define SWS_HEADER_STR_TYPE  's'  //for TEXT Line compatiable Character LCD Control

//Control must have prefix 'v' or 's'
char softSTRHeader[14] = {'p', 'a', 'g', 'e', '0', '.', 's', '0', '.', 't', 'x', 't', '=', '\"'};
char softINTHeader[13] = {'p', 'a', 'g', 'e', '0', '.', 'v', '0', '.', 'v', 'a', 'l', '='};

//send data for Nextion LCD
void SendHeader(char varType, char varIndex)
{
  if (varType == SWS_HEADER_STR_TYPE)
  {
    softSTRHeader[7] = varIndex;
    for (int i = 0; i < 14; i++)
      SWSerial_Write(softSTRHeader[i]);
  }
  else
  {
    softINTHeader[7] = varIndex;
    for (int i = 0; i < 13; i++)
      SWSerial_Write(softINTHeader[i]);
  }
}

void SendCommandUL(char varIndex, unsigned long sendValue)
{
  SendHeader(SWS_HEADER_INT_TYPE, varIndex);

  memset(softTemp, 0, 20);
  ultoa(sendValue, softTemp, DEC);
  SWSerial_Print(softTemp);
  SWSerial_Write(0xff);
  SWSerial_Write(0xff);
  SWSerial_Write(0xff);  
}

void SendCommandL(char varIndex, long sendValue)
{
  SendHeader(SWS_HEADER_INT_TYPE, varIndex);

  memset(softTemp, 0, 20);
  ltoa(sendValue, softTemp, DEC);
  SWSerial_Print(softTemp);
  SWSerial_Write(0xff);
  SWSerial_Write(0xff);
  SWSerial_Write(0xff);  
}

void SendCommandStr(char varIndex, char* sendValue)
{
  SendHeader(SWS_HEADER_STR_TYPE, varIndex);
  
  SWSerial_Print(sendValue);
  SWSerial_Write('\"');
  SWSerial_Write(0xFF);
  SWSerial_Write(0xFF);
  SWSerial_Write(0xFF);
}

//Send String data with duplicate check
void SendTextLineBuff(char lineNumber)
{
  //Check Duplicated data
  if (strcmp(softBuffLines[lineNumber], softBuffSended[lineNumber]))
  {
    SendHeader(SWS_HEADER_STR_TYPE, lineNumber + 0x30);  //s0.txt, s1.txt
  
    SWSerial_Print(softBuffLines[lineNumber]);
    SWSerial_Write('\"');
    SWSerial_Write(0xFF);
    SWSerial_Write(0xFF);
    SWSerial_Write(0xFF);
    
    strcpy(softBuffSended[lineNumber], softBuffLines[lineNumber]);
  }
}

void SendTextLineStr(char lineNumber, char* sendValue)
{
  int i = 0;
  for (i = 0; i < 16; i++)
  {
    if (sendValue[i] == 0x00)
      break;
    else
      softBuffLines[lineNumber][i] = sendValue[i];
  }
  
  for (;i < 20; i++)
  {
    softBuffLines[lineNumber][i] = ' ';
  }

  softBuffLines[lineNumber][TEXT_LINE_LENGTH + 1] = 0x00;
  SendTextLineBuff(lineNumber);
}

void SendEEPromData(char varIndex, int eepromStartIndex, int eepromEndIndex, char offsetTtype) 
{
  SendHeader(SWS_HEADER_STR_TYPE, varIndex);
  
  for (int i = eepromStartIndex; i <= eepromEndIndex; i++)
  {
      SWSerial_Write(EEPROM.read((offsetTtype == 0 ? USER_CALLSIGN_DAT : WSPR_MESSAGE1) + i));
  }

  SWSerial_Write('\"');
  SWSerial_Write(0xFF);
  SWSerial_Write(0xFF);
  SWSerial_Write(0xFF);
}

char softBuff1Num[17] = {'p', 'a', 'g', 'e', '0', '.', 'c', '0', '.', 'v', 'a', 'l', '=', 0, 0xFF, 0xFF, 0xFF};
void SendCommand1Num(char varType, char sendValue) //0~9 : Mode, nowDisp, ActiveVFO, IsDialLock, IsTxtType, IsSplitType
{
  softBuff1Num[7] = varType;
  softBuff1Num[13] = sendValue + 0x30;

  for (int i = 0; i < 17; i++)
    SWSerial_Write(softBuff1Num[i]);
}

void SetSWActivePage(char newPageIndex)
{
    if (L_nowdisp != newPageIndex)
    {
      L_nowdisp = newPageIndex;
      SendCommand1Num(CMD_NOW_DISP, L_nowdisp);
    }
}
//===================================================================
//End of Nextion LCD Protocol
//===================================================================

// The generic routine to display one line on the LCD 
void printLine(unsigned char linenmbr, const char *c) {
  SendTextLineStr(linenmbr, c);
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
void printLineFromEEPRom(char linenmbr, char lcdColumn, byte eepromStartIndex, byte eepromEndIndex, char offsetTtype) 
{
  int colIndex = lcdColumn;
  for (byte i = eepromStartIndex; i <= eepromEndIndex; i++)
  {
    if (++lcdColumn <= LCD_MAX_COLUMN)
      softBuffLines[linenmbr][colIndex++] = EEPROM.read((offsetTtype == 0 ? USER_CALLSIGN_DAT : WSPR_MESSAGE1) + i);
    else
      break;
  }

  SendTextLineBuff(linenmbr);
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
//Main Display for Nextion LCD
//unsigned long 
byte nowPageIndex = 0;

//sendType == 1 not check different 
void sendUIData(int sendType)
{
  char nowActiveVFO = vfoActive == VFO_A ? 0 : 1;

  //#define CMD_VFO_TYPE      'v' //cv
  if (L_vfoActive != nowActiveVFO)
  {
    L_vfoActive = nowActiveVFO;
    SendCommand1Num(CMD_VFO_TYPE, L_vfoActive);
  }

  //#define CMD_CURR_FREQ     'c' //vc
  if (L_vfoCurr != frequency)
  {
    L_vfoCurr = frequency;
    SendCommandUL(CMD_CURR_FREQ, frequency);
  }

  //#define CMD_CURR_MODE     'c' //cc
  byte vfoCurr_mode = modeToByte();
  if (L_vfoCurr_mode != vfoCurr_mode)
  {
    L_vfoCurr_mode = vfoCurr_mode;
    SendCommand1Num(CMD_CURR_MODE, L_vfoCurr_mode);
  }

  //if auto cw key mode, exit
  //if (isCWAutoMode != 0 || menuOn != 0)
  if (isCWAutoMode != 0)
    return;

  //nowPageIndex = 0;
  if (menuOn==0)
  {
    if (sendType == 0)
    {
      SetSWActivePage(0);
    }
    else
    {
      SetSWActivePage(0);
    }
  }
  else
  {
    //Text Line Mode
      SetSWActivePage(1);
  }

  //#define CMD_VFOA_FREQ     'a' //va
  //VFOA
  if (L_vfoA != vfoA)
  {
    L_vfoA = vfoA;
    SendCommandUL(CMD_VFOA_FREQ, L_vfoA);
  }

  //#define CMD_VFOA_MODE     'a' //ca
  if (L_vfoA_mode != vfoA_mode)
  {
    L_vfoA_mode = vfoA_mode;
    SendCommand1Num(CMD_VFOA_MODE, L_vfoA_mode);
  }

  //#define CMD_VFOB_FREQ     'b' //vb
  //VFOB
  if (L_vfoB != vfoB)
  {
    L_vfoB = vfoB;
    SendCommandUL(CMD_VFOB_FREQ, L_vfoB);
  }

  //#define CMD_VFOB_MODE     'b' //cb
  if (L_vfoB_mode != vfoB_mode)
  {
    L_vfoB_mode = vfoB_mode;
    SendCommand1Num(CMD_VFOB_MODE, L_vfoB_mode);  
  }

  //byte isDialLock = ((isTxType & 0x01) == 0x01) ? 1 : 0;
  if (L_isDialLock != isDialLock)
  {
    L_isDialLock = isDialLock;
    SendCommand1Num(CMD_IS_DIALLOCK, L_isDialLock);  
  }

  //#define CMD_IS_RIT        'r' //cr
  if (L_ritOn != ritOn)
  {
    L_ritOn = ritOn;
    SendCommand1Num(CMD_IS_RIT, L_ritOn);  
  }
  
  //#define CMD_RIT_FREQ      'r' //vr
  //unsigned long L_ritTxFrequency; //ritTxFrequency
  if (L_ritTxFrequency != ritTxFrequency)
  {
    L_ritTxFrequency = ritTxFrequency;
    SendCommandUL(CMD_RIT_FREQ, L_ritTxFrequency);  
  }

  //#define CMD_IS_TX         't' //ct
  //char L_inTx;
  if (L_inTx != inTx)
  {
    L_inTx = inTx;
    SendCommand1Num(CMD_IS_TX, L_inTx);  
  }

  //#define CMD_IS_DIALLOCK   'l' //cl
  //byte L_isDialLock;            //byte isDialLock
  if (L_isDialLock != isDialLock)
  {
    L_isDialLock = isDialLock;
    SendCommand1Num(CMD_IS_DIALLOCK, L_isDialLock);  
  }

  //#define CMD_IS_SPLIT      's' //cs
  //byte  L_Split;            //isTxType
  if (L_Split != splitOn)
  {
    L_Split = splitOn;
    SendCommand1Num(CMD_IS_SPLIT, L_Split);  
  }
  

  //#define CMD_IS_TXSTOP     'x' //cx
  byte isTXStop = ((isTxType & 0x01) == 0x01);
  if (L_TXStop != isTXStop)
  {
    L_TXStop = isTXStop;
    SendCommand1Num(CMD_IS_TXSTOP, L_TXStop);
  }

  //#define CMD_TUNEINDEX     'n' //cn
  if (L_tuneStepIndex != tuneStepIndex)
  {
    L_tuneStepIndex = tuneStepIndex;
    SendCommand1Num(CMD_TUNEINDEX, L_tuneStepIndex);
  }

  //#define CMD_SMETER        'p' //cp
  if (L_scaledSMeter != scaledSMeter)
  {
    L_scaledSMeter = scaledSMeter;
    SendCommand1Num(CMD_SMETER, L_scaledSMeter);  
  }

  //#define CMD_SIDE_TONE     't' //vt
  if (L_sideTone != sideTone)
  {
    L_sideTone = sideTone;
    SendCommandL(CMD_SIDE_TONE, L_sideTone);
  }

  //#define CMD_KEY_TYPE      'k' //ck
  if (L_cwKeyType != cwKeyType)
  {
    L_cwKeyType = cwKeyType;
    SendCommand1Num(CMD_KEY_TYPE, L_cwKeyType);  
  }

  //#define CMD_CW_SPEED      's' //vs
  if (L_cwSpeed != cwSpeed)
  {
    L_cwSpeed = cwSpeed;
    SendCommandL(CMD_CW_SPEED, L_cwSpeed);  
  }

  //#define CMD_CW_DELAY      'y' //vy
  if (L_cwDelayTime != cwDelayTime)
  {
    L_cwDelayTime = cwDelayTime;
    SendCommandL(CMD_CW_DELAY, L_cwDelayTime);  
  }

  //#define CMD_CW_STARTDELAY 'e' //ve
  if (L_delayBeforeCWStartTime != delayBeforeCWStartTime)
  {
    L_delayBeforeCWStartTime = delayBeforeCWStartTime;
    SendCommandL(CMD_CW_STARTDELAY, L_delayBeforeCWStartTime);
  }

  //#define CMD_ATT_LEVEL     'f' //vf
  if (L_attLevel != attLevel)
  {
    L_attLevel = attLevel;
    SendCommandL(CMD_ATT_LEVEL, L_attLevel);
  }

  //#define CMD_IS_IFSHIFT    'i'
  if (L_isIFShift != isIFShift)
  {
    L_isIFShift = isIFShift;
    SendCommand1Num(CMD_IS_IFSHIFT, L_isIFShift);
  }

  //#define CMD_IFSHIFT_VALUE 'i'
  if (L_ifShiftValue != ifShiftValue)
  {
    L_ifShiftValue = ifShiftValue;
    SendCommandL(CMD_IFSHIFT_VALUE, L_ifShiftValue);
  }

  //#define CMD_SDR_MODE      'j' //cj
  if (L_sdrModeOn != sdrModeOn)
  {
    L_sdrModeOn = sdrModeOn;
    SendCommand1Num(CMD_SDR_MODE, L_sdrModeOn);
  }
}

void updateDisplay() {
  //clearLine1();
  sendUIData(0);  //UI 

  /*
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
    */
}

// tern static uint8_t swr_receive_buffer[20];
extern uint8_t receivedCommandLength;
extern void SWSerial_Read(uint8_t * receive_cmdBuffer);
//extern void byteToMode(byte modeValue, byte autoSetModebyFreq);
uint8_t swr_buffer[20];

#define TS_CMD_MODE      1
#define TS_CMD_FREQ      2
#define TS_CMD_BAND      3
#define TS_CMD_VFO       4
#define TS_CMD_SPLIT     5
#define TS_CMD_RIT       6
#define TS_CMD_TXSTOP    7
#define TS_CMD_SDR       8
#define TS_CMD_LOCK      9  //Dial Lock
#define TS_CMD_ATT      10  //ATT
#define TS_CMD_IFS      11  //IFS Enabled
#define TS_CMD_IFSVALUE 12  //IFS VALUE

//SoftwareSerial_Process
void SWS_Process(void)
{
  //Received Command from touch screen
  if (receivedCommandLength > 0)
  {
    SWSerial_Read(swr_buffer);

    int8_t comandLength = receivedCommandLength;
    int8_t commandStartIndex = -1;
    receivedCommandLength = 0;

    //Data Process
    //comandLength //Find start Length
    for (int i = 0; i < comandLength - 3; i++)
    {
      if (swr_buffer[i] == 0x59 && swr_buffer[i+ 1] == 0x58 && swr_buffer[i + 2] == 0x68)
      {
        commandStartIndex = i;
        break;
      }
    } //end of for

    if (commandStartIndex != -1)
    {
      //Complete received command from touch screen
      uint8_t commandType = swr_buffer[commandStartIndex + 3];

/*
#define TS_CMD_MODE     1
#define TS_CMD_FREQ     2
#define TS_CMD_BAND     3
 */
#define TS_CMD_BAND     3

      if (commandType == TS_CMD_MODE)
      {
        byteToMode(swr_buffer[commandStartIndex + 4], 1);
      }
      else if (commandType == TS_CMD_FREQ)
      {
        unsigned long *tempFreq;
        tempFreq = (unsigned long *)(&swr_buffer[commandStartIndex + 4]);
        frequency = *tempFreq;
      }
      else if (commandType == TS_CMD_BAND)
      {
        char currentBandIndex = -1;
        if (tuneTXType == 2 || tuneTXType == 3 || tuneTXType == 102 || tuneTXType == 103) { //only ham band move
          currentBandIndex = getIndexHambanBbyFreq(frequency);
          
          if (currentBandIndex >= 0) {
            saveBandFreqByIndex(frequency, modeToByte(), currentBandIndex);
          }
        }
        setNextHamBandFreq(frequency, swr_buffer[commandStartIndex + 4] == 1 ? -1 : 1);  //Prior Band      
      }
      else if (commandType == TS_CMD_VFO)
      {
        menuVfoToggle(1); //Vfo Toggle        
      }
      else if (commandType == TS_CMD_SPLIT)
      {
        menuSplitOnOff(1);
      }
      else if (commandType == TS_CMD_RIT)
      {
        menuRitToggle(1);
      }
      else if (commandType == TS_CMD_TXSTOP)
      {
        menuTxOnOff(1, 0x01);
      }
      else if (commandType == TS_CMD_SDR)
      {
        menuSDROnOff(1);
      }
      else if (commandType == TS_CMD_LOCK)
      {
        if (vfoActive == VFO_A)
          setDialLock((isDialLock & 0x01) == 0x01 ? 0 : 1, 0); //Reverse Dial lock
        else
          setDialLock((isDialLock & 0x02) == 0x02 ? 0 : 1, 0); //Reverse Dial lock
      }
      else if (commandType == TS_CMD_ATT)
      {
        attLevel = swr_buffer[commandStartIndex + 4];
      }
      else if (commandType == TS_CMD_IFS)
      {
        isIFShift = isIFShift ? 0 : 1;  //Toggle
      }
      else if (commandType == TS_CMD_IFSVALUE)
      {
        ifShiftValue = *(long *)(&swr_buffer[commandStartIndex + 4]);
      }

      setFrequency(frequency);
      SetCarrierFreq();
      updateDisplay(); 
    }
  }
}

void updateLine2Buffer(char displayType)
{

}

/*
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
  return;
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

*/

char checkCount = 0;
char checkCountSMeter = 0;

//execute interval : 0.25sec
void idle_process()
{
  sendUIData(1);

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
  
    checkCountSMeter = 0; //Reset Latency time
  } //end of S-Meter
  
  /*
    return;
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
  
      checkCountSMeter = 0; //Reset Latency time
    } //end of S-Meter
  }
  */
}

//When boot time, send data
void SendUbitxData(void)
{
  SendCommandL(CMD_AR_TUNE1, arTuneStep[0]);
  SendCommandL(CMD_AR_TUNE2, arTuneStep[1]);
  SendCommandL(CMD_AR_TUNE3, arTuneStep[2]);
  SendCommandL(CMD_AR_TUNE4, arTuneStep[3]);
  SendCommandL(CMD_AR_TUNE5, arTuneStep[4]);

  SendCommandL(CMD_IS_CW_SHIFT_DISPLAY, isShiftDisplayCWFreq);
  SendCommandL(CMD_CW_SHIFT_ADJUST, shiftDisplayAdjustVal);
  SendCommandL(CMD_COMM_OPTION, commonOption0);
  SendCommandL(CMD_DISP_OPTION1, displayOption1);  
  SendCommandL(CMD_DISP_OPTION2, displayOption2);

  SendCommandStr(CMD_VERSION, "+v1.0N1"); //Version
  SendEEPromData(CMD_CALLSIGN, 0, userCallsignLength -1, 0);

  //Complte Send Info
  SendCommand1Num(CMD_UBITX_INFO, 1);

  //Page Init
  L_nowdisp = 0;
  SendCommand1Num(CMD_NOW_DISP, L_nowdisp);
}


//AutoKey LCD Display Routine
void Display_AutoKeyTextIndex(byte textIndex)
{
  byte diplayAutoCWLine = 0;
  
  if ((displayOption1 & 0x01) == 0x01)
    diplayAutoCWLine = 1;
  //LCD_SetCursor(0, diplayAutoCWLine);

  softBuffLines[diplayAutoCWLine][0] = byteToChar(textIndex);
  softBuffLines[diplayAutoCWLine][1] = ':';

  SendTextLineBuff(diplayAutoCWLine);
  //LCD_Write(byteToChar(textIndex));
  //LCD_Write(':');
}

//not use with Nextion LCD
void DisplayCallsign(byte callSignLength)
{
}

//Not use with Nextion LCD
void DisplayVersionInfo(const __FlashStringHelper * fwVersionInfo)
{
}

#endif
