/**********************************************************************************
WSPR SENDER for uBITX  by KD8CEC
Some of the code that sends WSPR referenced the code in G3ZIL.
Thanks to G3ZIL for sharing great code.

Due to the limited memory of uBITX, I have implemented at least only a few of the codes in uBITX.

Thanks for testing
Beta Tester : 

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
**********************************************************************************/

#include <EEPROM.h>
#include "ubitx.h"

//begin of test
byte WsprToneCode[164];

unsigned long lastTime=0;
unsigned long TX_MSNB_P2;              // Si5351 register MSNB_P2  PLLB for Tx
unsigned long TX_P2;    // Variable values for MSNB_P2 which defines the frequencies for the data

extern int enc_read(void);

byte WsprMSGCount = 0;

#define WSPR_BAND1 401

extern uint8_t Wspr_Reg1[8]; //3, 4, 5, 6, 7
extern uint8_t Wspr_Reg2[8]; //2, 3, 4

void SendWSPRManage()
{
  int knob = 0;
  byte knobPosition = 0;
  //char isNeedDisplayInfo = 0;
  char nowSelectedIndex = 0;
  char nowWsprStep = 0; //0 : select Message, 1 : select band, 2 : send
  char selectedWsprMessageIndex = -1;
  char selectedWsprBandIndex = -1;

  unsigned long WsprTXFreq = 0;
  unsigned int WsprMultiChan = 0;
  //unsigned long prevFreq;
  byte loopIndex;

  delay_background(500, 0);
  
  //Readed WsprMSGCount, WsprTone
  while(1)
  {
    knob = enc_read();
    
    if (knobPosition > 0 && knob < 0)
      knobPosition--;
    else if (knob > 0 && (knobPosition <= (nowWsprStep == 0 ? WsprMSGCount : WSPR_BAND_COUNT) * 10 -2))
      knobPosition++;

    nowSelectedIndex = knobPosition / 10;

    if (nowWsprStep == 0) //select Message status
    {
      //printLineF2(F("WSPR:"));
      
      if (selectedWsprMessageIndex != nowSelectedIndex)
      {
        selectedWsprMessageIndex = nowSelectedIndex;
        int wsprMessageBuffIndex = selectedWsprMessageIndex * 46;
        
        printLineF2(F("WSPR:"));
        //Display WSPR Name tag
        printLineFromEEPRom(0, 6, wsprMessageBuffIndex, wsprMessageBuffIndex + 4, 1); 

        //Load WSPR Tonecode
        //Read Tone Code
        for (int i = 0; i < 41; i++)
        {
          byte readData = EEPROM.read(WSPR_MESSAGE1 + 5 + (wsprMessageBuffIndex) + i);  //NAME TAG 5, MESSAGE 41 = 46
          WsprToneCode[i * 4 + 0] = readData & 3;
          WsprToneCode[i * 4 + 1] = (readData >> 2) & 3;
          WsprToneCode[i * 4 + 2] = (readData >> 4) & 3;
          WsprToneCode[i * 4 + 3] = (readData >> 6) & 3;
        }
      }
      else if (btnDown())
      {
        nowWsprStep = 1;  //Change Status to Select Band
        knobPosition = 0;
        nowSelectedIndex = 0;
        delay_background(500, 0);
      }
    }
    else if (nowWsprStep == 1)
    {
      //printLineF2(F("Select Band"));
      if (selectedWsprBandIndex != nowSelectedIndex)
      {
        selectedWsprBandIndex = nowSelectedIndex;
        int bandBuffIndex = WSPR_BAND1 + selectedWsprBandIndex * 14;
        
        EEPROM.get(bandBuffIndex, WsprTXFreq); 
        EEPROM.get(bandBuffIndex + 4, WsprMultiChan); 

        for (loopIndex = 3; loopIndex < 8; loopIndex++)
          Wspr_Reg1[loopIndex] = EEPROM.read(bandBuffIndex + loopIndex + 3);

        //2, 3, 4
        for (loopIndex = 2; loopIndex < 5; loopIndex++)
          Wspr_Reg2[loopIndex] = EEPROM.read(bandBuffIndex + loopIndex + 9);

        TX_MSNB_P2 = ((unsigned long)Wspr_Reg1[5] & 0x0F) << 16 | ((unsigned long)Wspr_Reg1[6]) << 8 | Wspr_Reg1[7];
      }

      if (digitalRead(PTT) == 0)
        strcpy(c, "SEND: ");
      else
        strcpy(c, "PTT-> ");

      //ltoa(WsprTXFreq, b, DEC);
      //strcat(c, b);

      //display frequency, Frequency to String for KD8CEC
      unsigned long tmpFreq = WsprTXFreq;
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

      printLine1(c);

#ifdef USE_SW_SERIAL
      SWS_Process();
      if ((digitalRead(PTT) == 0) || (TriggerBySW == 1))
      {
        TriggerBySW = 0;
#else
      if (digitalRead(PTT) == 0)
      {
#endif
        //SEND WSPR
        //If you need to consider the Rit and Sprite modes, uncomment them below.
        //remark = To reduce the size of the program
        //prevFreq = frequency;
        //frequency = WsprTXFreq;
        startTx(TX_CW, 0);
        setTXFilters(WsprTXFreq);
        
        //Start WSPR
        Set_WSPR_Param();
        digitalWrite(CW_KEY, 1);     
        
        for (int i = 0; i < 162; i++)
        {                                                         // Now this is the message loop
          lastTime = millis();                                    // Store away the time when the last message symbol was sent
          TX_P2 = TX_MSNB_P2 + WsprMultiChan * WsprToneCode[i];   // This represents the 1.46 Hz shift and is correct only for the bands specified in the array
          TXSubFreq(TX_P2);                                       // TX at the appropriate channel frequency for....

          //if (btnDown())
          //  break;

          while (millis() < lastTime + 683){}                     // .... 0,683 seconds
        }
        
        digitalWrite(CW_KEY, 0);
        stopTx(); //call setFrequency -> recovery TX Filter
        //frequency = prevFreq;
        
        selectedWsprBandIndex = -1;
      }     //end of PTT Check
      else if (btnDown())
      {
        return;
      }

    }  //end of status check
    
    //delay_background(50, 1);
  } //end of while
}

