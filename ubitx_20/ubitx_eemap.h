/*************************************************************************
  header file for EEProm Address Map by KD8CEC
  It must be protected to protect the factory calibrated calibration.
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
#ifndef _UBITX_EEPOM_HEADER__
#define _UBITX_EEPOM_HEADER__

//==============================================================================
// Factory-shipped EEProm address
// (factory Firmware)
// Address : 0 ~ 31
//==============================================================================
#define MASTER_CAL            0
#define LSB_CAL               4
#define USB_CAL               8
#define SIDE_TONE             12
//these are ids of the vfos as well as their offset into the eeprom storage, don't change these 'magic' values
#define VFO_A                 16
#define VFO_B                 20
#define CW_SIDETONE           24
#define CW_SPEED              28

//==============================================================================
// The spare space available in the original firmware #1
// Address : 32 ~ 63
//==============================================================================
#define RESERVE_FOR_FACTORY1  32

//==============================================================================
// The spare space available in the original firmware #2
// (Enabled if the EEProm address is insufficient)
// Address : 64 ~ 100
//==============================================================================
#define RESERVE_FOR_FACTORY2  64  //use Factory backup from Version 1.075
#define FACTORY_BACKUP_YN     64  //Check Backup //Magic : 0x13
#define FACTORY_VALUES        65  //65 ~ 65 + 32

//==============================================================================
// KD8CEC EEPROM MAP
// Address : 101 ~ 1023
// 256 is the base address
// 256 ~ 1023 (EEProm Section #1)
// 255 ~ 101  (EEProm Section #2)
//==============================================================================

//0x00 : None, 0x01 : MODE, 0x02:BAND+, 0x03:BAND-, 0x04:TUNE_STEP, 0x05:VFO Toggle, 0x06:SplitOn/Off, 0x07:TX/ON-OFF,  0x08:SDR Mode On / Off, 0x09:Rit Toggle
#define EXTENDED_KEY_RANGE    140 //Extended Key => Set : Start Value, End Value, Key Type, 16 Set (3 * 16 = 48)

#define I2C_LCD_MASTER        190
#define I2C_LCD_SECOND        191

#define S_METER_LEVELS        230 //LEVEL0 ~ LEVEL7

#define ADVANCED_FREQ_OPTION1 240 //Bit0: use IFTune_Value, Bit1 : use Stored enabled SDR Mode, Bit2 : dynamic sdr frequency
#define IF1_CAL               241
#define ENABLE_SDR            242
#define SDR_FREQUNCY          243
#define CW_CAL                252

#define VFO_A_MODE            256
#define VFO_B_MODE            257
#define CW_DELAY              258
#define CW_START              259
#define HAM_BAND_COUNT        260    //
#define TX_TUNE_TYPE          261      //
#define HAM_BAND_RANGE        262    //FROM (2BYTE) TO (2BYTE) * 10 = 40byte
#define HAM_BAND_FREQS        302    //40, 1 BAND = 4Byte most bit is mode
#define TUNING_STEP           342   //TUNING STEP * 6 (index 1 + STEPS 5)  //1STEP : 

//for reduce cw key error, eeprom address
#define CW_ADC_MOST_BIT1      348   //most 2bits of  DOT_TO , DOT_FROM, ST_TO, ST_FROM
#define CW_ADC_ST_FROM        349   //CW ADC Range STRAIGHT KEY from (Lower 8 bit)
#define CW_ADC_ST_TO          350   //CW ADC Range STRAIGHT KEY to   (Lower 8 bit)
#define CW_ADC_DOT_FROM       351   //CW ADC Range DOT  from         (Lower 8 bit)
#define CW_ADC_DOT_TO         352   //CW ADC Range DOT  to           (Lower 8 bit)

#define CW_ADC_MOST_BIT2      353   //most 2bits of BOTH_TO, BOTH_FROM, DASH_TO, DASH_FROM
#define CW_ADC_DASH_FROM      354   //CW ADC Range DASH from         (Lower 8 bit)
#define CW_ADC_DASH_TO        355   //CW ADC Range DASH to           (Lower 8 bit)
#define CW_ADC_BOTH_FROM      356   //CW ADC Range BOTH from         (Lower 8 bit)
#define CW_ADC_BOTH_TO        357   //CW ADC Range BOTH to           (Lower 8 bit)
#define CW_KEY_TYPE           358
#define CW_DISPLAY_SHIFT      359   //Transmits on CWL, CWU Mode, LCD Frequency shifts Sidetone Frequency. 
                                    //(7:Enable / Disable //0: enable, 1:disable, (default is applied shift)
                                    //6 : 0 : Adjust Pulus, 1 : Adjust Minus
                                    //0~5: Adjust Value : * 10 = Adjust Value (0~300)
#define COMMON_OPTION0        360   //0: Confirm : CW Frequency Shift
                                    //1 : IF Shift Save
#define IF_SHIFTVALUE         363

#define DISPLAY_OPTION1       361   //Display Option1
#define DISPLAY_OPTION2       362   //Display Option2

#define WSPR_COUNT            443   //WSPR_MESSAGE_COUNT
#define WSPR_MESSAGE1         444   //
#define WSPR_MESSAGE2         490   //
#define WSPR_MESSAGE3         536   //
#define WSPR_MESSAGE4         582   //

#define CHANNEL_FREQ          630   //Channel 1 ~ 20, 1 Channel = 4 bytes
#define CHANNEL_DESC          710   //Channel 1 ~ 20, 1 Channel = 4 bytes
#define EXTERNAL_DEVICE_OPT1  770   //for External Deivce 4byte
#define EXTERNAL_DEVICE_OPT2  774   //for External Deivce 2byte

//Check Firmware type and version
#define FIRMWAR_ID_ADDR       776 //776 : 0x59, 777 :0x58, 778 : 0x68 : Id Number, if not found id, erase eeprom(32~1023) for prevent system error.
#define VERSION_ADDRESS       779   //check Firmware version
//USER INFORMATION
#define USER_CALLSIGN_KEY     780   //0x59
#define USER_CALLSIGN_LEN     781   //1BYTE (OPTION + LENGTH) + CALLSIGN (MAXIMUM 18)
#define USER_CALLSIGN_DAT     782   //CALL SIGN DATA  //direct EEPROM to LCD basic offset

//AUTO KEY STRUCTURE
//AUTO KEY USE 800 ~ 1023
#define CW_AUTO_MAGIC_KEY     800   //0x73
#define CW_AUTO_COUNT         801   //0 ~ 255
#define CW_AUTO_DATA          803   //[INDEX, INDEX, INDEX,DATA,DATA, DATA (Positon offset is CW_AUTO_DATA
#define CW_DATA_OFSTADJ       CW_AUTO_DATA - USER_CALLSIGN_DAT   //offset adjust for ditect eeprom to lcd (basic offset is USER_CALLSIGN_DAT
#define CW_STATION_LEN        1023  //value range : 4 ~ 30

#endif    //end of if header define

