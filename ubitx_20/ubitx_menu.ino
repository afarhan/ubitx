/*
This source code started with Farhan's original source. The license rules are followed as well.
Calibration related functions kept the original source except for the minor ones. 
The part is collected in the last minute of this source.
Ian KD8CEC
 */
 
#include "ubitx.h"
#include "ubitx_eemap.h"

//Current Frequency and mode to active VFO by KD8CEC
void FrequencyToVFO(byte isSaveFreq)
{
  //Save Frequency & Mode Information
  if (vfoActive == VFO_A)
  {
    vfoA = frequency;
    vfoA_mode = modeToByte();

    if (isSaveFreq)
      storeFrequencyAndMode(1);
  }
  else
  {
    vfoB = frequency;
    vfoB_mode = modeToByte();
    
    if (isSaveFreq)
      storeFrequencyAndMode(2);
  }
}

//Commonly called functions when exiting menus by KD8CEC
void menuClearExit(int delayTime)
{
  if (delayTime > 0)
    delay_background(delayTime, 0);
    
  printLine2ClearAndUpdate();
  menuOn = 0;
}

//Ham band or general band movement by KD8CEC
void menuBand(int btn){
  int knob = 0;
  int stepChangeCount = 0;
  byte btnPressCount = 0;

  if (!btn){
   printLineF2(F("Band Select?"));
   return;
  }

  //printLineF2(F("Press to confirm"));
  //wait for the button menu select button to be lifted)
  while (btnDown()) {
     delay_background(50, 0);
    if (btnPressCount++ > 20) {
      btnPressCount = 0;
      if (tuneTXType > 0) { //Just toggle 0 <-> 2, if tuneTXType is 100, 100 -> 0 -> 2
        tuneTXType = 0;
        printLineF2(F("General"));
      }
      else {
        tuneTXType = 2;
        printLineF2(F("Ham band"));
      }
      delay_background(1000, 0);
      printLine2ClearAndUpdate();
    }
  }
  printLineF2(F("Press to confirm"));
  
  char currentBandIndex = -1;
  //Save Band Information
  if (tuneTXType == 2 || tuneTXType == 3 || tuneTXType == 102 || tuneTXType == 103) { //only ham band move
    //Get Now Band Index
    currentBandIndex = getIndexHambanBbyFreq(frequency);
    
    if (currentBandIndex >= 0) {
      //Save Frequency to Band Frequncy Record
      saveBandFreqByIndex(frequency, modeToByte(), currentBandIndex);
    }
  }
  
  //delay(50);    
  ritDisable();

  while(!btnDown()){
    knob = enc_read();
    if (knob != 0){
      if (tuneTXType == 2 || tuneTXType == 3 || tuneTXType == 102 || tuneTXType == 103) { //only ham band move
        if (knob < 0) {
          if (stepChangeCount-- < -3) {
            setNextHamBandFreq(frequency, -1);  //Prior Band
            stepChangeCount = 0;
          }
        }
        else if (knob > 0) {
          if (stepChangeCount++ > 3) {
            setNextHamBandFreq(frequency, 1); //Next Band
            stepChangeCount = 0;
          }
        }
      }       //end of only ham band move
      else {  //original source
        if (knob < 0 && frequency > 3000000l)
          setFrequency(frequency - 200000l);
        if (knob > 0 && frequency < 30000000l)
          setFrequency(frequency + 200000l);

        if (frequency > 10000000l)
          isUSB = true;
        else
          isUSB = false;
      }

      updateDisplay();
    }
    
    delay_background(20, 0);
  }

  FrequencyToVFO(1);
  menuClearExit(500);
}

//Convert Mode, Number by KD8CEC
//0: default, 1:not use, 2:LSB, 3:USB, 4:CWL, 5:CWU, 6:FM
byte modeToByte(){
  if (cwMode == 0)
  {
    if (isUSB)
      return 3;
    else
      return 2;
  }
  else if (cwMode == 1)
  {
    return 4;
  }
  else
  {
    return 5;
  }
}

//Convert Number to Mode by KD8CEC
//autoSetModebyFreq : 0
//autoSetModebyFreq : 1, if (modValue is not set, set mode by frequency)
void byteToMode(byte modeValue, byte autoSetModebyFreq){
  if (modeValue == 4)
    cwMode = 1;
  else if (modeValue == 5)
    cwMode = 2;
  else
  {
    cwMode = 0;
    if (modeValue == 3)
      isUSB = 1;
    else if (autoSetModebyFreq == 1 && (modeValue == 0))
      isUSB = (frequency > 10000000l) ? true : false;
    else
      isUSB = 0;
  }
}


//Memory to VFO, VFO to Memory by KD8CEC
void menuCHMemory(int btn, byte isMemoryToVfo){
  int knob = 0;
  int selectChannel = 0;
  byte isDisplayInfo = 1;
  int moveStep = 0;
  unsigned long resultFreq, tmpFreq = 0;
  byte loadMode = 0;
  
  if (!btn){
    if (isMemoryToVfo == 1)
      printLineF2(F("Channel To VFO?"));
   else 
      printLineF2(F("VFO To Channel?"));
  }
  else {
    delay_background(500, 0);

    while(!btnDown()){
      if (isDisplayInfo == 1) {
        //Display Channel info *********************************
        memset(c, 0, sizeof(c));

        if (selectChannel >= 20 || selectChannel <=-1)
        {
          //strcpy(c, "Exit setup?");
          strcpy(c, "Exit?");
        }
        else
        {
          //Read Frequency from eeprom
          EEPROM.get(CHANNEL_FREQ + 4 * selectChannel, resultFreq);
          
          loadMode = (byte)(resultFreq >> 29);
          resultFreq = resultFreq & 0x1FFFFFFF;

          //display channel description
          if (selectChannel < 10 && EEPROM.read(CHANNEL_DESC + 6 * selectChannel) == 0x03) {  //0x03 is display Chnnel Name
            //display Channel Name
            for (int i = 0; i < 5; i++)
              c[i] = EEPROM.read(CHANNEL_DESC + 6 * selectChannel + i + 1);

           c[5] = ':';
          }
          else {
            //Display frequency
            //1 LINE : Channel Information : CH00
            strcpy(c, "CH");
            if (selectChannel < 9)
              c[2] = '0';
            
            ltoa(selectChannel + 1, b, 10);
            strcat(c, b); //append channel Number;
            strcat(c, " :"); //append channel Number;
          }
  
          //display frequency
          tmpFreq = resultFreq;
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
        }

        printLine2(c);
        
        isDisplayInfo = 0;
      }

      knob = enc_read();

      if (knob != 0)
      {
        moveStep += (knob > 0 ? 1 : -1);
        if (moveStep < -3) {
          if (selectChannel > -1)
            selectChannel--;

          isDisplayInfo = 1;
          moveStep = 0;
        }
        else if (moveStep > 3) {
          if (selectChannel < 20)
            selectChannel++;
            
          isDisplayInfo = 1;
          moveStep = 0;
        }
      }

      Check_Cat(0);  //To prevent disconnections
    } //end of while (knob)

    if (selectChannel < 20 && selectChannel >= 0)
    {
      if (isMemoryToVfo == 1)
      {
        if (resultFreq > 3000 && resultFreq < 60000000)
        {
          byteToMode(loadMode, 1);
          setFrequency(resultFreq);
        }
      }
      else
      {
        //Save current Frequency to Channel (selectChannel)
        EEPROM.put(CHANNEL_FREQ + 4 * selectChannel, (frequency & 0x1FFFFFFF) | (((unsigned long)modeToByte()) << 29) );
        printLine2("Saved Frequency");
      }
    }
    
    menuClearExit(500);
  }
}

//Analog pin monitoring with CW Key and function keys connected.
//by KD8CEC
#ifdef ENABLE_ADCMONITOR        
void menuADCMonitor(int btn){
  int adcPinA0 = 0;  //A0(BLACK, EncoderA)
  int adcPinA1 = 0;  //A1(BROWN, EncoderB)
  int adcPinA2 = 0;  //A2(RED, Function Key)
  int adcPinA3 = 0;  //A3(ORANGE, CW Key)
  int adcPinA6 = 0;  //A6(BLUE, Ptt)
  int adcPinA7 = 0;  //A7(VIOLET, Spare)
  unsigned long pressKeyTime = 0;
  
  if (!btn){
        printLineF2(F("ADC Line Monitor"));
        return;
  }
  
  printLineF2(F("Exit:Long PTT"));
  delay_background(2000, 0);
  printLineF1(F("A0   A1   A2"));
  printLineF2(F("A3   A6   A7"));
  delay_background(3000, 0);
  
  while (true) {
    adcPinA0 = analogRead(A0);  //A0(BLACK, EncoderA)
    adcPinA1 = analogRead(A1);  //A1(BROWN, EncoderB)
    adcPinA2 = analogRead(A2);  //A2(RED, Function Key)
    adcPinA3 = analogRead(A3);  //A3(PTT)
    adcPinA6 = analogRead(A6);  //A6(KEYER)
    adcPinA7 = analogRead(A7);  //A7(VIOLET, Spare)

    if (adcPinA3 < 50) {
      if (pressKeyTime == 0)
        pressKeyTime = millis();
      else if (pressKeyTime < (millis() - 3000))
          break;
    }
    else
      pressKeyTime = 0;
    
    ltoa(adcPinA0, c, 10);
    //strcat(b, c);
    strcpy(b, c);
    strcat(b, ", ");
    
    ltoa(adcPinA1, c, 10);
    strcat(b, c);
    strcat(b, ", ");
    
    ltoa(adcPinA2, c, 10);
    strcat(b, c);

    printLine1(b);

    //strcpy(b, " ");
    ltoa(adcPinA3, c, 10);
    strcpy(b, c);
    strcat(b, ", ");

    ltoa(adcPinA6, c, 10);
    strcat(b, c);
    strcat(b, ", ");
    
    ltoa(adcPinA7, c, 10);
    strcat(b, c);
    printLine2(b);
    
    delay_background(200, 0);
  } //end of while
      
  menuClearExit(0);
}
#endif

//VFO Toggle and save VFO Information, modified by KD8CEC
void menuVfoToggle(int btn)
{
  if (!btn){
    if (vfoActive == VFO_A)
      printLineF2(F("Select VFO B?"));
    else
      printLineF2(F("Select VFO A?"));    
  }
  else {
      FrequencyToVFO(1);
    
      if (vfoActive == VFO_B){
        vfoActive = VFO_A;
        frequency = vfoA;
        saveCheckFreq = frequency;
        byteToMode(vfoA_mode, 0);
      }
      else {
        vfoActive = VFO_B;
        frequency = vfoB;
        saveCheckFreq = frequency;
        byteToMode(vfoB_mode, 0);
      }

      ritDisable();
      setFrequency(frequency);

#ifdef USE_SW_SERIAL
    menuOn = 0;
#else
    //Only Clear And Delay for Character LCD
    menuClearExit(0);
#endif    
      
  }
}


//Split communication using VFOA and VFOB by KD8CEC
void menuSplitOnOff(int btn){
  if (!btn){
    if (splitOn == 0)
      printLineF2(F("Split On?"));
    else
      printLineF2(F("Split Off?"));
  }
  else {
      if (splitOn == 1){
        splitOn = 0;
        printLineF2(F("SPT Off"));
      }
      else {
        splitOn = 1;
        if (ritOn == 1)
          ritOn = 0;
        printLineF2(F("SPT On"));
      }

#ifdef USE_SW_SERIAL
    menuOn = 0;
#else
//Only Clear And Delay for Character LCD
    menuClearExit(500);
#endif    
  }
}

//Function to disbled transmission
//by KD8CEC
void menuTxOnOff(int btn, byte optionType){
  if (!btn){
    if ((isTxType & optionType) == 0)
      printLineF2(F("TX OFF?"));
    else
      printLineF2(F("TX ON?"));
  }
  else {
      if ((isTxType & optionType) == 0){
        isTxType |= optionType;
        printLineF2(F("TX OFF"));
      }
      else {
        isTxType &= ~(optionType);
        printLineF2(F("TX ON"));
      }

#ifdef USE_SW_SERIAL
    menuOn = 0;
#else
    //Only Clear And Delay for Character LCD
    menuClearExit(500);
#endif
  }
}

//Toggle SDR MODE
void menuSDROnOff(int btn)
{
  if (!btn){
    if (sdrModeOn == 0)
      printLineF2(F("SDR Mode On?"));
    else
      printLineF2(F("SDR Mode Off?"));
  }
  else {
      if (sdrModeOn == 1){
        sdrModeOn = 0;
        printLineF2(F("SPK MODE"));
      }
      else {
        sdrModeOn = 1;
        
        if (ritOn == 1)
          ritOn = 0;

        if (splitOn == 1)
          splitOn = 0;
          
        printLineF2(F("SDR MODE"));
      }

    EEPROM.put(ENABLE_SDR, sdrModeOn);
    setFrequency(frequency);
    SetCarrierFreq();

#ifdef USE_SW_SERIAL
    menuOn = 0;
#else
    //Only Clear And Delay for Character LCD
    menuClearExit(500);
#endif
  }
}

void displayEmptyData(void){
  printLineF2(F("Empty data"));
  delay_background(2000, 0);
}

//Builtin CW Keyer Logic by KD8CEC
void menuCWAutoKey(int btn){
    if (!btn){
      printLineF2(F("Memory Keyer"));
      return;
    }
    
    //Check CW_AUTO_MAGIC_KEY and CW Text Count
    EEPROM.get(CW_AUTO_COUNT, cwAutoTextCount);
    if (EEPROM.read(CW_AUTO_MAGIC_KEY) != 0x73 || cwAutoTextCount < 1)
    {
      displayEmptyData();
     return;
    }

    printLineF1(F("PTT to Send"));
    delay_background(500, 0);
    beforeCWTextIndex = 255;  //255 value is for start check
    isCWAutoMode = 1;
    updateDisplay();
    menuOn = 0;
}

//Standalone WSPR Beacone
void menuWSPRSend(int btn){
  if (!btn){
     printLineF2(F("WSPR Beacon"));
     return;
  }

  WsprMSGCount = EEPROM.read(WSPR_COUNT);

  if (WsprMSGCount < 1)
  {
    displayEmptyData();
    return;
  }

  SendWSPRManage();
  menuClearExit(1000);
}

//Append by KD8CEC
void menuSetupCWCarrier(int btn){
  int knob = 0;
  unsigned long prevCarrier;
   
  if (!btn){
      printLineF2(F("Set CW RX BFO"));
    return;
  }

  prevCarrier = cwmCarrier;
  printLineF1(F("PTT to confirm. "));
  delay_background(1000, 0);

  si5351bx_setfreq(0, cwmCarrier);
  printCarrierFreq(cwmCarrier);

  //disable all clock 1 and clock 2 
  while (digitalRead(PTT) == HIGH && !btnDown())
  {
    knob = enc_read();

    if (knob > 0)
      cwmCarrier -= 5;
    else if (knob < 0)
      cwmCarrier += 5;
    else
      continue; //don't update the frequency or the display
      
    si5351bx_setfreq(0, cwmCarrier);
    printCarrierFreq(cwmCarrier);

    delay_background(100, 0);
  }

  //save the setting
  if (digitalRead(PTT) == LOW){
    printLineF2(F("Carrier set!"));
    EEPROM.put(CW_CAL, cwmCarrier);
    delay_background(1000, 0);
  }
  else 
    cwmCarrier = prevCarrier;

  if (cwMode == 0)
    si5351bx_setfreq(0, usbCarrier);  //set back the carrier oscillator anyway, cw tx switches it off
  else
    si5351bx_setfreq(0, cwmCarrier);  //set back the carrier oscillator anyway, cw tx switches it off
  
  setFrequency(frequency);
  menuClearExit(0);
}

//=======================================================
//BEGIN OF STANDARD TUNE SETUP for reduce Program Memory
// by KD8CEC
//=======================================================
//valueType 0 : Normal
//          1 : CW Change -> Generate Tone
//          2 : IF Shift Setup -> SetFrequency, Set SideTone
//          5 : ATT
//          11 : Select Mode (different display type)
//knobSensitivity : 1 ~ 
int getValueByKnob(int valueType, int targetValue, int minKnobValue, int maxKnobValue, int incStep, const char* displayTitle, int knobSensitivity)
{
    int knob;
    int moveDetectStep = 0;
    char isInitDisplay = 1;
    delay_background(300, 0); //Default Delay

    if (valueType < 10)
    {
      strcpy(b, "Press, set ");
      strcat(b, displayTitle);
      printLine1(b);
    }
    
    while(!btnDown())
    {
      knob = enc_read();
      if (knob != 0 || isInitDisplay == 1)
      {
        isInitDisplay = 0;
        
        /*
        //Program Size : 29424 (95%)
        if (targetValue > minKnobValue && knob < 0)
          targetValue -= incStep;
        if (targetValue < maxKnobValue && knob > 0)
          targetValue += incStep;
        */

        //Program Size : 29560 (increase 135 byte from avobe codes), but a lot of usable functions
        moveDetectStep += (knob > 0 ? 1 : -1);
        if (moveDetectStep < (knobSensitivity * -1)) {
          if (targetValue > minKnobValue)
            targetValue -= incStep;
            
          moveDetectStep = 0;
        }
        else if (moveDetectStep > knobSensitivity) {
          if (targetValue < maxKnobValue)
            targetValue += incStep;
            
          moveDetectStep = 0;
        }

        strcpy(b, displayTitle);

        if (valueType == 11)   //Mode Select
        {
          b[targetValue * 4] = '>';
        }
        /*
        else if (valueType == 4) //CW Key Type  Select
        {
          if (targetValue == 0)
            strcat(b, "Straight");
          else if (targetValue == 1)
            strcat(b, "IAMBICA");
          else if (targetValue == 2)
            strcat(b, "IAMBICB");
        }
        */
        else
        {
          strcat(b, ":");
          itoa(targetValue,c, 10);
          strcat(b, c);
        }
        
        printLine2(b);

        if (valueType == 1) //Generate Side Tone
        {
          tone(CW_TONE, targetValue);          
        }
        else if (valueType == 2 || valueType == 5 ) // 2:IFS, 5:ATT
        {
          if (valueType == 2)
            ifShiftValue = targetValue;
          else 
            attLevel = targetValue;

#ifdef USE_SW_SERIAL
  menuOn=2;
  updateDisplay();
#endif
          setFrequency(frequency);
          SetCarrierFreq();
        }
      }

      Check_Cat(0);  //To prevent disconnections
    }

    return targetValue;
}

void menuCWSpeed(int btn){
    int wpm;

    wpm = 1200/cwSpeed;
     
    if (!btn){
     strcpy(b, "CW:");
     itoa(wpm,c, 10);
     strcat(b, c);
     strcat(b, "WPM Change?");
     printLine2(b);
     return;
    }

    //printLineF1(F("Press to set WPM"));
    //strcpy(b, "WPM:");
    //itoa(wpm,c, 10);
    //strcat(b, c);
    //printLine2(b);
    //delay_background(300, 0);
    
    wpm = getValueByKnob(0, wpm, 3, 50, 1, "WPM", 3);
    
    /*
    while(!btnDown()){

      knob = enc_read();
      if (knob != 0){
        if (wpm > 3 && knob < 0)
          wpm--;
        if (wpm < 50 && knob > 0)
          wpm++;

        strcpy(b, "WPM:");
        itoa(wpm,c, 10);
        strcat(b, c);
        printLine2(b);
      }

      Check_Cat(0);  //To prevent disconnections
    }
    */

    
  //save the setting
  //printLineF2(F("CW Speed set!"));
  cwSpeed = 1200 / wpm;
  EEPROM.put(CW_SPEED, cwSpeed);
  //menuClearExit(1000);
#ifdef USE_SW_SERIAL
    menuOn = 0;
#else
    //Only Clear And Delay for Character LCD
    menuClearExit(1000);
#endif
  
}

//Modified by KD8CEC
void menuSetupCwTone(int btn){
    //int prev_sideTone;
     
    if (!btn){
      printLineF2(F("Change CW Tone"));
      return;
    }

    //prev_sideTone = sideTone;
    //printLineF1(F("Tune CW tone"));
    //printLineF2(F("PTT to confirm."));
    //printLineF1(F("Press to set WPM"));
    //delay_background(1000, 0);
    //tone(CW_TONE, sideTone);

    sideTone = getValueByKnob(1, sideTone, 100, 2000, 10, "Tone", 2); //1 : Generate Tone, targetValue, minKnobValue, maxKnobValue, stepSize

    noTone(CW_TONE);
    
    printLineF2(F("Sidetone set!"));
    EEPROM.put(CW_SIDETONE, sideTone);

    //delay_background(2000, 0);
    //menuClearExit(0);
#ifdef USE_SW_SERIAL
    menuOn = 0;
#else
    //Only Clear And Delay for Character LCD
    delay_background(2000, 0);
    menuClearExit(0);
#endif
 }

//Modified by KD8CEC
void menuSetupCwDelay(int btn){
    //int knob = 0;
    int tmpCWDelay = cwDelayTime * 10;
     
    if (!btn){
      printLineF2(F("CW TX->RX Delay"));
     return;
    }

    tmpCWDelay = getValueByKnob(0, tmpCWDelay, 3, 2500, 10, "Delay", 2); //0 : Generate Tone, targetValue, minKnobValue, maxKnobValue, stepSize

    //save the setting
    cwDelayTime = tmpCWDelay / 10;
    EEPROM.put(CW_DELAY, cwDelayTime);
   //menuClearExit(1000);
#ifdef USE_SW_SERIAL
    menuOn = 0;
#else
    //Only Clear And Delay for Character LCD
    menuClearExit(1000);
#endif
}

//CW Time delay by KD8CEC
void menuSetupTXCWInterval(int btn){
    //char needDisplayInformation = 1;
    //int knob = 0;
    int tmpTXCWInterval = delayBeforeCWStartTime * 2;
     
    if (!btn){
      printLineF2(F("CW Start Delay"));
     return;
    }

    //printLineF1(F("Press, set Delay"));
    //delay_background(300, 0);

    tmpTXCWInterval = getValueByKnob(0, tmpTXCWInterval, 0, 500, 2, "Delay", 2); //0 : Generate Tone, targetValue, minKnobValue, maxKnobValue, stepSize

   delayBeforeCWStartTime = tmpTXCWInterval / 2;
   EEPROM.put(CW_START, delayBeforeCWStartTime);
   //menuClearExit(1000);

#ifdef USE_SW_SERIAL
    menuOn = 0;
#else
    //Only Clear And Delay for Character LCD
    menuClearExit(1000);
#endif
   
}

//IF Shift function, BFO Change like RIT, by KD8CEC
void menuIFSSetup(int btn){
  //int knob = 0;
  //char needApplyChangeValue = 1;
  
  if (!btn){
    if (isIFShift == 1)
      printLineF2(F("IF Shift Change?"));
    else
      printLineF2(F("IF Shift On?"));
  }
  else 
  {
      isIFShift = 1;

      ifShiftValue = getValueByKnob(2, ifShiftValue, -20000, 20000, 50, "IFS", 2); //2 : IF Setup (updateLine2Buffer(1), SetFrequency), targetValue, minKnobValue, maxKnobValue, stepSize
      delay_background(500, 0); //for check Long Press function key
      
      if (btnDown() || ifShiftValue == 0)
      {
        isIFShift = 0;
        ifShiftValue = 0;
        setFrequency(frequency);
        SetCarrierFreq();
      }

      //Store IF Shiift
      EEPROM.put(IF_SHIFTVALUE, ifShiftValue);
      //menuClearExit(0);
#ifdef USE_SW_SERIAL
    menuOn = 0;
#else
    //Only Clear And Delay for Character LCD
    menuClearExit(0);
#endif
  }
}

//ATT SETUP (IF1(45MHZ) SHIFT), by KD8CEC
void menuATTSetup(int btn){
  //int knob = 0;
  //char needApplyChangeValue = 1;
  
  if (!btn){
    if (attLevel != 0)
      printLineF2(F("ATT Change?"));
    else
      printLineF2(F("ATT On?"));
  }
  else 
  {
      attLevel = getValueByKnob(5, attLevel, 0, 250, 5, "ATT", 2); //2 : (SetFrequency), targetValue, minKnobValue, maxKnobValue, stepSize
      delay_background(500, 0); //for check Long Press function key
      
      if (btnDown() || attLevel == 0)
      {
        attLevel = 0;
        setFrequency(frequency);
        //SetCarrierFreq();
      }
      //menuClearExit(0);

#ifdef USE_SW_SERIAL
      menuOn = 0;
#else
      //Only Clear And Delay for Character LCD
      menuClearExit(0);
#endif
      
  }
}

//Functions for CWL and CWU by KD8CEC
void menuSelectMode(int btn){
  //int knob = 0;
  int selectModeType = 0;
  int beforeMode = 0;
  //int moveStep = 0;
  
  if (!btn){
      printLineF2(F("Select Mode?"));
  }
  else 
  {
    //LSB, USB, CWL, CWU
    if (cwMode == 0 && isUSB == 0)
      selectModeType = 0;
    else if (cwMode == 0 && isUSB == 1)
      selectModeType = 1;
    else if (cwMode == 1)
      selectModeType = 2;
    else
      selectModeType = 3;

    beforeMode = selectModeType;
    selectModeType = getValueByKnob(11, selectModeType, 0, 3, 1, " LSB USB CWL CWU", 4); //3 : Select Mode, targetValue, minKnobValue, maxKnobValue, stepSize

    if (beforeMode != selectModeType) 
    {
      //printLineF1(F("Changed Mode"));
      if (selectModeType == 0) {
        cwMode = 0; isUSB = 0;
      }
      else if (selectModeType == 1) {
        cwMode = 0; isUSB = 1;
      }
      else if (selectModeType == 2) {
        cwMode = 1;
      }
      else if (selectModeType == 3) {
        cwMode = 2;
      }

      FrequencyToVFO(1);
    }

    SetCarrierFreq();
    setFrequency(frequency);
    //menuClearExit(500);
#ifdef USE_SW_SERIAL
    menuOn = 0;
#else
    //Only Clear And Delay for Character LCD
    menuClearExit(500);
#endif
  }
}

//Select CW Key Type by KD8CEC
void menuSetupKeyType(int btn){
  //int knob = 0;
  int selectedKeyType = 0;
  //int moveStep = 0;
  if (!btn){
        printLineF2(F("Change Key Type?"));
  }
  else {
    selectedKeyType = cwKeyType;

    //selectedKeyType = getValueByKnob(12, selectedKeyType, 0, 2, 1, " KEY:", 5); //4 : Select Key Type, targetValue, minKnobValue, maxKnobValue, stepSize
    selectedKeyType = getValueByKnob(11, selectedKeyType, 0, 2, 1, " ST  IA  IB", 5); //4 : Select Key Type, targetValue, minKnobValue, maxKnobValue, stepSize

    printLineF2(F("CW Key Type set!"));
    cwKeyType = selectedKeyType;
    EEPROM.put(CW_KEY_TYPE, cwKeyType);

    if (cwKeyType == 0)
      Iambic_Key = false;
    else
    {
      Iambic_Key = true;
      if (cwKeyType == 1)
        keyerControl &= ~IAMBICB;
      else
        keyerControl |= IAMBICB;
    }
    
    //menuClearExit(1000);
#ifdef USE_SW_SERIAL
    menuOn = 0;
#else
    //Only Clear And Delay for Character LCD
    menuClearExit(1000);
#endif
    
  }
}

//=====================================================
//END OF STANDARD Set by Knob for reduce Program Memory
//=====================================================
 

//Lock Dial move by KD8CEC
void setDialLock(byte tmpLock, byte fromMode) {
  if (tmpLock == 1)
    isDialLock |= (vfoActive == VFO_A ? 0x01 : 0x02);
  else
    isDialLock &= ~(vfoActive == VFO_A ? 0x01 : 0x02);
    
  if (fromMode == 2 || fromMode == 3) return;

  //delay_background(1000, 0);
  printLine2ClearAndUpdate();
}

byte btnDownTimeCount;

#define PRESS_ADJUST_TUNE 20 //1000msec 20 * 50 = 1000milisec
#define PRESS_LOCK_CONTROL 40 //2000msec 40 * 50 = 2000milisec

//Modified by KD8CEC
void doMenu(){
  int select=0, i,btnState;
  char isNeedDisplay = 0;
  
  //for DialLock On/Off function
  btnDownTimeCount = 0;
  
  //wait for the button to be raised up

  //Appened Lines by KD8CEC for Adjust Tune step and Set Dial lock
  while(btnDown()){
    delay_background(50, 0);
    
    if (btnDownTimeCount++ == (PRESS_ADJUST_TUNE)) { //Set Tune Step 
      printLineF2(F("Set Tune Step?"));
    }
    else if (btnDownTimeCount > (PRESS_LOCK_CONTROL)) {  //check long time Down Button -> 2.5 Second => Lock
      if (vfoActive == VFO_A)
        setDialLock((isDialLock & 0x01) == 0x01 ? 0 : 1, 0); //Reverse Dial lock
      else
        setDialLock((isDialLock & 0x02) == 0x02 ? 0 : 1, 0); //Reverse Dial lock
      return;
    }
  }
  delay(50);  //debounce

  //ADJUST TUNE STEP 
  if (btnDownTimeCount > PRESS_ADJUST_TUNE)
  {
    printLineF1(F("Press to set"));
    isNeedDisplay = 1; //check to need display for display current value
    
    while (!btnDown())
    {
      delay_background(50, 0);

      if (isNeedDisplay) {
        strcpy(b, "Tune Step:");
        itoa(arTuneStep[tuneStepIndex -1], c, 10);
        strcat(b, c);
        printLine2(b);
        isNeedDisplay = 0;
      }
        
      i = enc_read();

      if (i != 0) {
        select += (i > 0 ? 1 : -1);

        if (select * select >= 25) {  //Threshold 5 * 5 = 25
          if (select < 0) {
            if (tuneStepIndex > 1)
              tuneStepIndex--;
          }
          else {
            if (tuneStepIndex < 5)
              tuneStepIndex++;
          }
          select = 0;
          isNeedDisplay = 1;
        }
      }
    } //end of while

    EEPROM.put(TUNING_STEP, tuneStepIndex);
    delay_background(500, 0);
    printLine2ClearAndUpdate();
    return;
  }   //set tune step

  //Below codes are origial code with modified by KD8CEC
  menuOn = 2;
  TriggerBySW = 0;  //Nextion LCD and Other MCU
  
  while (menuOn){
    i = enc_read();
    btnState = btnDown();

    if (i > 0){
      if (modeCalibrate && select + i < 250)
        select += i;
      else if (!modeCalibrate && select + i < 150)
        select += i;
    }
    else if (i < 0 && select - i >= -10)
      select += i;

    //if -> switch : reduce program memory 200byte
    switch (select / 10)
    {
      case 0 : 
        menuBand(btnState); 
        break;
      case 1 : 
        menuVfoToggle(btnState); 
        break;
      case 2 : 
        menuSelectMode(btnState); 
        break;
      case 3 : 
        menuRitToggle(btnState); 
        break;
      case 4 : 
        menuIFSSetup(btnState); 
        break;
      case 5 : 
        menuATTSetup(btnState); 
        break;
      case 6 : 
        menuCWSpeed(btnState); 
        break;
      case 7 : 
        menuSplitOnOff(btnState);        //SplitOn / off
        break;
      case 8 : 
        menuCHMemory(btnState, 0);       //VFO to Memroy
        break;
      case 9 : 
        menuCHMemory(btnState, 1);       //Memory to VFO
        break;
      case 10 : 
        menuCWAutoKey(btnState);  
        break;
      case 11 : 
        menuWSPRSend(btnState);
        break;
      case 12 : 
        menuSDROnOff(btnState);
        break;
      case 13 : 
        menuSetup(btnState);
        break;
      case 14 : 
        menuExit(btnState);
        break;
      case 15 : 
        menuSetupCalibration(btnState);  //crystal
        break;
      case 16 : 
        menuSetupCarrier(btnState);      //ssb
        break;
      case 17 : 
        menuSetupCWCarrier(btnState);    //cw
        break;
      case 18 : 
        menuSetupCwTone(btnState);  
        break;
      case 19 : 
        menuSetupCwDelay(btnState);  
        break;
      case 20 : 
        menuSetupTXCWInterval(btnState);  
        break;
      case 21 :
        menuSetupKeyType(btnState);  
        break;
#ifdef ENABLE_ADCMONITOR        
      case 22 :
        menuADCMonitor(btnState);  
        break;
      case 23 :
#else      
      case 22 :
#endif      
        menuTxOnOff(btnState, 0x01);       //TX OFF / ON
        break;
      default :
        menuExit(btnState);  break;
    }    
    Check_Cat(0);  //To prevent disconnections
  }
}

//*************************************************************************************
//Original Source Part
//The code below is the original source part that I kept unchanged for compatibility.
//By KD8CEC
//*************************************************************************************

/** 
 Original source comment 
 *  Menus
 *  The Radio menus are accessed by tapping on the function button. 
 *  - The main loop() constantly looks for a button press and calls doMenu() when it detects
 *  a function button press. 
 *  - As the encoder is rotated, at every 10th pulse, the next or the previous menu
 *  item is displayed. Each menu item is controlled by it's own function.
 *  - Eache menu function may be called to display itself
 *  - Each of these menu routines is called with a button parameter. 
 *  - The btn flag denotes if the menu itme was clicked on or not.
 *  - If the menu item is clicked on, then it is selected,
 *  - If the menu item is NOT clicked on, then the menu's prompt is to be displayed
 */

/**
 * The calibration routines are not normally shown in the menu as they are rarely used
 * They can be enabled by choosing this menu option
 */
void menuSetup(int btn){
  if (!btn)
  {
    if (!modeCalibrate)
      printLineF2(F("Setup On?"));
    else
      printLineF2(F("Setup Off?"));
  }
  else 
  {
    modeCalibrate = ! modeCalibrate;
    //menuClearExit(1000);

#ifdef USE_SW_SERIAL
    menuOn = 0;
#else
    //Only Clear And Delay for Character LCD
    menuClearExit(1000);
#endif
  }
}

void menuExit(int btn){
  if (!btn){
      printLineF2(F("Exit Menu?"));
  }
  else
   menuClearExit(0);
}

//modified for reduce used flash memory by KD8CEC
void menuRitToggle(int btn){
  if (!btn){
    if (ritOn == 1)
      printLineF2(F("RIT Off?"));
    else
      printLineF2(F("RIT On?"));
  }
  else {
      if (ritOn == 0){
        //printLineF2(F("RIT is ON"));
        //enable RIT so the current frequency is used at transmit
        ritEnable(frequency);
      }
      else{
        //printLineF2(F("RIT is OFF"));
        ritDisable();
      }
      
      //menuClearExit(500);
#ifdef USE_SW_SERIAL
      menuOn = 0;
#else
      //Only Clear And Delay for Character LCD
      menuClearExit(500);
#endif
      
  }
}

/**
 * Take a deep breath, math(ematics) ahead
 * The 25 mhz oscillator is multiplied by 35 to run the vco at 875 mhz
 * This is divided by a number to generate different frequencies.
 * If we divide it by 875, we will get 1 mhz signal
 * So, if the vco is shifted up by 875 hz, the generated frequency of 1 mhz is shifted by 1 hz (875/875)
 * At 12 Mhz, the carrier will needed to be shifted down by 12 hz for every 875 hz of shift up of the vco
 * 
 */

 //this is used by the si5351 routines in the ubitx_5351 file
extern int32_t calibration;
extern uint32_t si5351bx_vcoa;

void factoryCalibration(int btn){
  int knob = 0;

  //keep clear of any previous button press
  while (btnDown())
    delay(100);
  delay(100);
   
  if (!btn){
    printLineF2(F("Set Calibration?"));
    return;
  }

  calibration = 0;

  cwMode = 0;
  isUSB = true;

  //turn off the second local oscillator and the bfo
  si5351_set_calibration(calibration);
  startTx(TX_CW, 1);
  si5351bx_setfreq(2, 10000000l); 
  
  strcpy(b, "#1 10 MHz cal:");
  ltoa(calibration/8750, c, 10);
  strcat(b, c);
  printLine2(b);     

  while (!btnDown())
  {

    if (digitalRead(PTT) == LOW && !keyDown)
      cwKeydown();
    if (digitalRead(PTT)  == HIGH && keyDown)
      cwKeyUp();
      
    knob = enc_read();

    if (knob > 0)
      calibration += 875;
    else if (knob < 0)
      calibration -= 875;
    else 
      continue; //don't update the frequency or the display
      
    si5351_set_calibration(calibration);
    si5351bx_setfreq(2, 10000000l);
    strcpy(b, "#1 10 MHz cal:");
    ltoa(calibration/8750, c, 10);
    strcat(b, c);
    printLine2(b);     
  }

  cwTimeout = 0;
  keyDown = 0;
  stopTx();

  printLineF2(F("Calibration set!"));
  EEPROM.put(MASTER_CAL, calibration);
  initOscillators();
  setFrequency(frequency);
  updateDisplay();

  while(btnDown())
    delay(50);

  menuClearExit(100);
}

void menuSetupCalibration(int btn){
  int knob = 0;
  int32_t prev_calibration;
   
  if (!btn){
    printLineF2(F("Set Calibration?"));
    return;
  }

  printLineF1(F("Set to Zero-beat,"));
  printLineF2(F("press PTT to save"));
  delay_background(1000, 0);
  
  prev_calibration = calibration;
  calibration = 0;
  si5351_set_calibration(calibration);
  setFrequency(frequency);    
  
  strcpy(b, "cal:");
  ltoa(calibration/8750, c, 10);
  strcat(b, c);
  printLine2(b);     

  while (digitalRead(PTT) == HIGH && !btnDown())
  {
    knob = enc_read();

    if (knob > 0){
      calibration += 8750;
      usbCarrier += 120;
    }
    else if (knob < 0){
      calibration -= 8750;
      usbCarrier -= 120;
    }
    else
      continue; //don't update the frequency or the display

    si5351_set_calibration(calibration);
    si5351bx_setfreq(0, usbCarrier);
    setFrequency(frequency);    

    strcpy(b, "cal:");
    ltoa(calibration/8750, c, 10);
    strcat(b, c);
    printLine2(b);     
  }
  
  //save the setting
  if (digitalRead(PTT) == LOW){
    printLineF1(F("Calibration set!"));
    printLineF2(F("Set Carrier now"));
    EEPROM.put(MASTER_CAL, calibration);
    delay_background(2000, 0);
  }
  else
    calibration = prev_calibration;

  initOscillators();
  //si5351_set_calibration(calibration);
  setFrequency(frequency);    
  //printLine2ClearAndUpdate();
  //menuOn = 0;
  menuClearExit(0);
}

void printCarrierFreq(unsigned long freq){

  memset(c, 0, sizeof(c));
  memset(b, 0, sizeof(b));

  ultoa(freq, b, DEC);
  
  strncat(c, b, 2);
  strcat(c, ".");
  strncat(c, &b[2], 3);
  strcat(c, ".");
  strncat(c, &b[5], 3);
  printLine2(c);    
}

//modified by KD8CEC (just 1 line remarked //usbCarrier = ...
void menuSetupCarrier(int btn){
  int knob = 0;
  unsigned long prevCarrier;
   
  if (!btn){
      printLineF2(F("Set the BFO"));
    return;
  }

  prevCarrier = usbCarrier;
  printLineF1(F("Tune to best Signal"));  
  printLineF1(F("PTT to confirm. "));
  delay_background(1000, 0);

  //usbCarrier = 11995000l; //Remarked by KD8CEC, Suggest from many user, if entry routine factoryrest
  
  si5351bx_setfreq(0, usbCarrier);
  printCarrierFreq(usbCarrier);

  //disable all clock 1 and clock 2 
  while (digitalRead(PTT) == HIGH && !btnDown())
  {
    knob = enc_read();

    if (knob > 0)
      usbCarrier -= 5;
    else if (knob < 0)
      usbCarrier += 5;
    else
      continue; //don't update the frequency or the display
      
    si5351bx_setfreq(0, usbCarrier);
    printCarrierFreq(usbCarrier);

    Check_Cat(0);  //To prevent disconnections
    delay(100);
  }

  //save the setting
  if (digitalRead(PTT) == LOW){
    printLineF2(F("Carrier set!"));
    EEPROM.put(USB_CAL, usbCarrier);
    delay_background(1000, 0);
  }
  else 
    usbCarrier = prevCarrier;

  //si5351bx_setfreq(0, usbCarrier);          
  if (cwMode == 0)
    si5351bx_setfreq(0, usbCarrier);  //set back the carrier oscillator anyway, cw tx switches it off
  else
    si5351bx_setfreq(0, cwmCarrier);  //set back the carrier oscillator anyway, cw tx switches it off
  
  setFrequency(frequency);    
  //printLine2ClearAndUpdate();
  //menuOn = 0; 
  menuClearExit(0);
}

