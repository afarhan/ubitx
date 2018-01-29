/*************************************************************************
  KD8CEC's uBITX Idle time Processing
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
byte line2Buffer[16];
//KD8CEC 200Hz ST
//L14.150 200Hz ST
//U14.150 +150khz
int freqScrollPosition = 0;
//Example Line2 Optinal Display
void updateLine2Buffer()
{
  unsigned long tmpFreq = 0;

  if (ritOn)
  {
    line2Buffer[0] = 'R';
    line2Buffer[1] = 'i';
    line2Buffer[2] = 't';
    line2Buffer[3] = 'T';
    line2Buffer[4] = 'X';
    line2Buffer[5] = ':';

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
  }
  
  if (vfoActive == VFO_B)
  {
    tmpFreq = vfoA;
    //line2Buffer[0] = 'A';
  }
  else 
  {
    tmpFreq = vfoB;
    //line2Buffer[0] = 'B';
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
  if ((displayOption1 & 0x04) == 0x00)
    line2Buffer[6] = 'k';
  else
  {
    //example #2
    if (freqScrollPosition++ > 18)
    {
      line2Buffer[6] = 'k';
      if (freqScrollPosition > 25)
        freqScrollPosition = -1;
    }
    else
    {
      line2Buffer[10] = 'H';
      line2Buffer[11] = 'z';
  
      if (freqScrollPosition < 7)
      {
        for (int i = 11; i > 0; i--)
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
  }
  line2Buffer[7] = ' ';

  //Step
  byte tmpStep = arTuneStep[tuneStepIndex -1];
  for (int i = 10; i >= 8; i--) {
    if (tmpStep > 0) {
        line2Buffer[i] = tmpStep % 10 + 0x30;
        tmpStep /= 10;
    }
    else
      line2Buffer[i] = ' ';
  }
  line2Buffer[11] = 'H';
  line2Buffer[12] = 'z';

  line2Buffer[13] = ' ';
  //if (
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

void idle_process()
{
  //space for user graphic display
  if (menuOn == 0)
  {
    //if line2DisplayStatus == 0 <-- this condition is clear Line, you can display any message
    if (line2DisplayStatus == 0 || (((displayOption1 & 0x04) == 0x04) && line2DisplayStatus == 2)) {
      updateLine2Buffer();
      printLine2(line2Buffer);
      line2DisplayStatus = 2;
    }
  }
}

