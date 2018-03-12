/*************************************************************************
  header file for C++ by KD8CEC
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
#define WSPR_COUNT       443   //WSPR_MESSAGE_COUNT
#define WSPR_MESSAGE1    444   //
#define WSPR_MESSAGE2    490   //
#define WSPR_MESSAGE3    536   //
#define WSPR_MESSAGE4    582   //

#define WSPR_BAND_COUNT 3

#define TX_SSB 0
#define TX_CW 1


extern void printLine1(const char *c);
extern void printLine2(const char *c);
extern void printLineF(char linenmbr, const __FlashStringHelper *c);
extern void printLineFromEEPRom(char linenmbr, char lcdColumn, byte eepromStartIndex, byte eepromEndIndex, char offsetType);
extern byte delay_background(unsigned delayTime, byte fromType);
extern int btnDown(void);
extern char c[30];
extern char b[30];

extern unsigned long frequency;

#define printLineF1(x) (printLineF(1, x))
#define printLineF2(x) (printLineF(0, x))


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

//we directly generate the CW by programmin the Si5351 to the cw tx frequency, hence, both are different modes
//these are the parameter passed to startTx
#define TX_SSB 0
#define TX_CW 1

extern void si5351bx_init(void);
extern void si5351bx_setfreq(uint8_t clknum, uint32_t fout);
extern void si5351_set_calibration(int32_t cal);
extern void initOscillators(void);
extern void Set_WSPR_Param(void);
extern void TXSubFreq(unsigned long P2);

extern void startTx(byte txMode, byte isDisplayUpdate);
extern void stopTx(void);
extern void setTXFilters(unsigned long freq);

extern void SendWSPRManage(void);
extern byte WsprMSGCount;


