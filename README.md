#IMPORTANT INFORMATION
----------------------------------------------------------------------------
- 0.33 Version Test only download. almost complete
- Beta 0.26 and Beta 0.261, Beta 0.262,0.27 is complete test, 0.28 is tested.
- 0.31 is tested but has not critical bug
- You can download and use it (Release section).

#NOTICE
----------------------------------------------------------------------------
I received uBITX a month ago and found that many features are required, and began coding with the idea of implementing minimal functionality as a general hf transceiver rather than an experimental device.

Most of the basic functions of the HF transceiver I thought were implemented.
The minimum basic specification for uBITX to operate as a radio, I think it is finished.
So I will release the 0.27 version and if I do not see the bug anymore, I will try to change the version name to 1.0.
Now uBITX is an HF radio and will be able to join you in your happy hams life.
Based on this source, you can use it by adding functions.

I am going to do a new project based on this source, linking with WSPR, WSJT-X and so on.
Of course, this repository is still running. If you have any bugs or ideas, please feel free to email me.

http://www.hamskey.com

DE KD8CEC
kd8cec@gmail.com

#uBITX
uBITX firmware, written for the Raduino/Arduino control of uBITX transceivers
This project is based on https://github.com/afarhan/ubitx and all copyright is inherited.
The copyright information of the original is below.

KD8CEC
----------------------------------------------------------------------------
Prepared or finished tasks for the next version
  - Most of them are implemented and included in version 0.27.
  - User Interface on LCD -> Option by user (not need)
  - Include WSPR Beacone function - (implement other new repository)
    complete experiment
    need solve : Big code size (over 100%, then remove some functions for experment)
                 need replace Si5351 Library (increase risk and need more beta tester)
                 W3PM sent me his wonderful source - using BITX, GPS 
                  
----------------------------------------------------------------------------
## REVISION RECORD
0.33
  - Added CWL, CWU Mode, (dont complete test yet)
  - fixed VFO changed bug.
  - Added Additional BFO for CWL, CWL
  - Added IF Shift
  - Change confirmation key PTT -> function key (not critical menus)
  - Change CW Key Select type, (toggle -> select by dial)
  
0.32
  - Added function Scroll Frequencty on upper line
  - Added Example code for Draw meter and remarked (you can see and use this code in source codes)
  - Added Split function, just toggle VFOs when TX/RX

0.31
  - Fixed CW ADC Range error
  - Display Message on Upper Line (anothor VFO Frequency, Tune Step, Selected Key Type)

0.30
 - implemented the function to monitor the value of all analog inputs. This allows you to monitor the status of the CW keys connected to your uBITX. 
 - possible to set the ADC range for CW Keying. If no setting is made, it will have the same range as the original code. If you set the CW Keying ADC Values using uBITX Manager 0.3, you can reduce the key error.
 - Added the function to select Straight Key, IAMBICA, IAMBICB key from the menu.
 - default Band select is Ham Band mode, if you want common type, long press function key at band select menu, uBITX Manager can be used to modify frequencies to suit your country.

0.29
 - Remove the use of initialization values in BFO settings - using crruent value, if factory reset
 - Select Tune Step, default 0, 20, 50, 100, 200, Use the uBITX Manager to set the steps value you want. You can select Step by pressing and holding the Function Key (1sec ~ 2sec).
 - Modify Dial Lock Function, Press the Function key for more than 3 seconds to toggle dial lock.
 - created a new frequency tune method. remove original source codes, Threshold has been applied to reduce malfunction. checked the continuity of the user operating to make natural tune possible.
 - stabilize and remove many warning messages - by Pullrequest and merge
 - Changed cw keying method. removed the original code and applied Ron's code and Improved compatibility with original hardware and CAT commnication. It can be used without modification of hardware.
 
0.28
 - Fixed CAT problem with hamlib on Linux
 - restore Protocol autorecovery logic

0.27 
   (First alpha test version, This will be renamed to the major version 1.0)
 - Dual VFO Dial Lock (vfoA Dial lock)
 - Support Ham band on uBITX
   default Hamband is regeion1 but customize by uBITX Manager Software
 - Advanced ham band options (Tx control) for use in all countries. You can adjust it yourself.   
 - Convenience of band movement

0.26
  - only Beta tester released & source code share
  - find a bug on none initial eeprom uBITX - Fixed (Check -> initialized & compatible original source code)
  - change the version number 0.26 -> 0.27
  - Prevent overflow bugs
  - bug with linux based Hamlib (raspberry pi), It was perfect for the 0.224 version, but there was a problem for the 0.25 version.  
    On Windows, ham deluxe, wsjt-x, jt65-hf, and fldigi were successfully run. Problem with Raspberry pi.

0.25
  - Beta Version Released
    http://www.hamskey.com/2018/01/release-beta-version-of-cat-support.html
  - Added CAT Protocol for uBITX
  - Modified the default usb carrier value used when the setting is wrong.
  - Fixed a routine to repair when the CAT protocol was interrupted.

0.24
  - Program optimization
    reduce usage ram rate (string with M() optins)
  - Optimized CAT protocol for wsjt-x, fldigi

0.23
  - added delay_background() , replace almost delay() to delay_background for prevent timeout
  - cat library compatible with FT-817 Command 
    switch VFOA / VFOB, 
    Read Write CW Speed
    Read Write CW Delay Time
    Read Write CW Pitch (with sidetone)
    All of these can be controlled by Hamradio deluxe.

  - modified cat libray function for protocol for CAT communication is not broken in CW or TX mode
  - Ability to change CW Delay
  - Added Dial Lock function
  - Add functions CW Start dely (TX -> CW interval)
  - Automatic storage of VFO frequency
    It was implemented by storing it only once when the frequency stays 10 seconds or more after the change.
    (protect eeprom life)

  
0.22
  - fixed screen Update Problem
  - Frequency Display Problem - Problems occur below 1Mhz
  - added function Enhanced CAT communication
  - replace ubitx_cat.ino to cat_libs.ino
  - Save mode when switching to VFOA / VFOB


0.21
  - fixed the cw side tone configuration.
  - Fix the error that the frequency is over.
  - fixed frequency display (alignment, point) 


0.20 
  - original uBITX software (Ashhar Farhan)

## Original README.md
uBITX firmware, written for the Raduino/Arduino control of uBITX transceigers

Copyright (C) 2017,  Ashhar Farhan

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
