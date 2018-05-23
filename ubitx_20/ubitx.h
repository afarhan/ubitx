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
#ifndef _UBITX_HEADER__
#define _UBITX_HEADER__

#include <Arduino.h>  //for Linux, On Linux it is case sensitive.

//==============================================================================
// Compile Option
//==============================================================================
//Depending on the type of LCD mounted on the uBITX, uncomment one of the options below.
//You must select only one.
#define UBITX_DISPLAY_LCD1602P        //LCD mounted on unmodified uBITX (Parallel)
//#define UBITX_DISPLAY_LCD1602I        //I2C type 16 x 02 LCD
//#define UBITX_DISPLAY_LCD1602I_DUAL   //I2C type 16 x02 LCD Dual
//#define UBITX_DISPLAY_LCD2004P        //24 x 04 LCD (Parallel)
//#define UBITX_DISPLAY_LCD2004I        //I2C type 24 x 04 LCD

#define I2C_LCD_MASTER_ADDRESS_DEFAULT  0x3F     //0x27  //DEFAULT, if Set I2C Address by uBITX Manager, read from EEProm
#define I2C_LCD_SECOND_ADDRESS_DEFAULT  0x27     //0x27  //only using Dual LCD Mode

#define EXTEND_KEY_GROUP1               //MODE, BAND(-), BAND(+), STEP
//#define EXTEND_KEY_GROUP2             //Numeric (0~9), Point(.), Enter  //Not supported in Version 1.0x

//#define ENABLE_FACTORYALIGN
#define FACTORY_RECOVERY_BOOTUP         //Whether to enter Factory Recovery mode by pressing FKey and turning on power
#define ENABLE_ADCMONITOR               //Starting with Version 1.07, you can read ADC values directly from uBITX Manager. So this function is not necessary.

extern byte I2C_LCD_MASTER_ADDRESS;     //0x27  //if Set I2C Address by uBITX Manager, read from EEProm
extern byte I2C_LCD_SECOND_ADDRESS;     //only using Dual LCD Mode

#define SMeterLatency   3               //1 is 0.25 sec

#ifdef UBITX_DISPLAY_LCD1602I
  #define USE_I2C_LCD
#elif defined(UBITX_DISPLAY_LCD1602I_DUAL)
  #define USE_I2C_LCD
#elif defined(UBITX_DISPLAY_LCD2004I)
  #define USE_I2C_LCD
#endif

//==============================================================================
// Hardware, Define PIN Usage
//==============================================================================
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
#define ENC_A         (A0)
#define ENC_B         (A1)
#define FBUTTON       (A2)
#define PTT           (A3)
#define ANALOG_KEYER  (A6)
#define ANALOG_SPARE  (A7)
#define ANALOG_SMETER (A7)  //by KD8CEC

/** 
 *  The second set of 16 pins on the Raduino's bottom connector are have the three clock outputs and the digital lines to control the rig.
 *  This assignment is as follows :
 *    Pin   1   2    3    4    5    6    7    8    9    10   11   12   13   14   15   16
 *         GND +5V CLK0  GND  GND  CLK1 GND  GND  CLK2  GND  D2   D3   D4   D5   D6   D7  
 *  These too are flexible with what you may do with them, for the Raduino, we use them to :
 *  - TX_RX line : Switches between Transmit and Receive after sensing the PTT or the morse keyer
 *  - CW_KEY line : turns on the carrier for CW
 */
#define TX_RX         (7)   //Relay
#define CW_TONE       (6)
#define TX_LPF_A      (5)   //Relay
#define TX_LPF_B      (4)   //Relay
#define TX_LPF_C      (3)   //Relay
#define CW_KEY        (2)

//==============================================================================
// for public, Variable, functions
//==============================================================================
#define WSPR_BAND_COUNT 3
#define TX_SSB          0
#define TX_CW           1
#define printLineF1(x) (printLineF(1, x))
#define printLineF2(x) (printLineF(0, x))

//0x00 : None, 0x01 : MODE, 0x02:BAND+, 0x03:BAND-, 0x04:TUNE_STEP, 0x05:VFO Toggle, 0x06:SplitOn/Off, 0x07:TX/ON-OFF,  0x08:SDR Mode On / Off, 0x09:Rit Toggle
#define FUNCTION_KEY_ADC  80  //MODE, BAND(-), BAND(+), STEP
#define FKEY_PRESS      0x78
#define FKEY_MODE       0x01
#define FKEY_BANDUP     0x02
#define FKEY_BANDDOWN   0x03
#define FKEY_STEP       0x04
#define FKEY_VFOCHANGE  0x05
#define FKEY_SPLIT      0x06
#define FKEY_TXOFF      0x07
#define FKEY_SDRMODE    0x08
#define FKEY_RIT        0x09

#define FKEY_ENTER      0x0A
#define FKEY_POINT      0x0B
#define FKEY_DELETE     0x0C
#define FKEY_CANCEL     0x0D

#define FKEY_NUM0       0x10
#define FKEY_NUM1       0x11
#define FKEY_NUM2       0x12
#define FKEY_NUM3       0x13
#define FKEY_NUM4       0x14
#define FKEY_NUM5       0x15
#define FKEY_NUM6       0x16
#define FKEY_NUM7       0x17
#define FKEY_NUM8       0x18
#define FKEY_NUM9       0x19

#define FKEY_TYPE_MAX   0x1F

extern unsigned long frequency;
extern byte WsprMSGCount;
extern byte sMeterLevels[9];
extern int currentSMeter;         //ADC Value for S.Meter
extern byte scaledSMeter;         //Calculated S.Meter Level

extern byte KeyValues[16][3];     //Set : Start Value, End Value, Key Type, 16 Set (3 * 16 = 48)

extern void printLine1(const char *c);
extern void printLine2(const char *c);
extern void printLineF(char linenmbr, const __FlashStringHelper *c);
extern void printLineFromEEPRom(char linenmbr, char lcdColumn, byte eepromStartIndex, byte eepromEndIndex, char offsetType);
extern byte delay_background(unsigned delayTime, byte fromType);
extern int btnDown(void);
extern char c[30];
extern char b[30];
extern int enc_read(void);
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
extern char byteToChar(byte srcByte);
extern void DisplayCallsign(byte callSignLength);
extern void DisplayVersionInfo(const char* fwVersionInfo);

#endif    //end of if header define


