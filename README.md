#uBITX
uBITX firmware, written for the Raduino/Arduino control of uBITX transceivers
This project is based on https://github.com/afarhan/ubitx and all copyright is inherited.
The copyright information of the original is below.

KD8CEC
----------------------------------------------------------------------------
Prepared or finished tasks for the next version
  - Prevent overflow bugs [from pullrequest, history check] : complete
  - Hamlib bug (raspberry pi), It was perfect for the 0.224 version, but there was a problem for the 0.25 version.
    Found by Beta Tester very thanks.
    On Windows, ham deluxe, wsjt-x, jt65-hf, and fldigi were successfully run. Problem with Raspberry pi.
    As a result of the analysis, when the serial port is initialized and used immediately, problems occur in Linux and Raspberry pi. -> Resolution (Complete)
    
  - No TX on non-ham band request - This may be a prohibited item depending on the country.
    Plan to change for uBITX Manager for free countries - Icom, yaesu, kenwood are mostly jumper in circuit.
    Only those who need to lock themselves, Other users remain unchanged
    so, Available in most countries around the world. (Complete)
  - I have heard that Beta testers want DialLock to distinguish between VFOA and VFOB (Complete)
  - Convenience of band movement added (ing - need idea...)

  - User Interface on LCD -> Option by user (yet - need idea)
  - Include WSPR Beacone function - (considerd about include functions or create other version)
    complete experiment
    need solve : Big code size (over 100%, then remove some functions for experment)
                 need replace Si5351 Library for multisynth (increase risk and need more beta tester)
                 W3PM sent me his wonderful source - using BITX, GPS
                  
----------------------------------------------------------------------------
## REVISION RECORD
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
