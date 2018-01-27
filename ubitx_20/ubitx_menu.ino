/** Menus
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
#define printLineF1(x) (printLineF(1, x))
#define printLineF2(x) (printLineF(0, x))

//Ham band move by KD8CEC
void menuBand(int btn){
  int knob = 0;
  int stepChangeCount = 0;
  byte btnPressCount = 0;

  if (!btn){
   printLineF2(F("Band Select?"));
   return;
  }

  printLineF2(F("Press to confirm"));
  //wait for the button menu select button to be lifted)
  while (btnDown()) {
    delay(50);
    Check_Cat(0);  //To prevent disconnections
    if (btnPressCount++ > 20) {
      btnPressCount = 0;
      if (tuneTXType > 0) { //Just toggle 0 <-> 2, if tuneTXType is 100, 100 -> 0 -> 2
        tuneTXType = 0;
        printLineF2(F("Full range mode"));
      }
      else {
        tuneTXType = 2;
        printLineF2(F("Ham band mode"));
      }
      delay_background(1000, 0);
      printLine2ClearAndUpdate();
      printLineF2(F("Press to confirm"));
    }
  }
  
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
  
  delay(50);    
  ritDisable();

  while(!btnDown()){

    knob = enc_read();
    if (knob != 0){
      /*
      if (band > 3 && knob < 0)
        band--;
      if (band < 30 && knob > 0)
        band++; 
      if (band > 10)
        isUSB = true;
      else
        isUSB = false;
      setFrequency(((unsigned long)band * 1000000l) + offset); */
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
      }
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
    
    delay(20);
    Check_Cat(0);  //To prevent disconnections
  }

  while(btnDown()) {
    delay(50);
    Check_Cat(0);  //To prevent disconnections
  }
  
  delay(50);
  
  printLine2ClearAndUpdate();
  menuOn = 0;
}

//Convert Mode, Number by KD8CEC
//0: default, 1:not use, 2:LSB, 3:USB, 4:CW, 5:AM, 6:FM
byte modeToByte(){
  if (isUSB)
    return 3;
  else
    return 2;
}

//Convert Number to Mode by KD8CEC
void byteToMode(byte modeValue){
  if (modeValue == 3)
    isUSB = 1;
  else
    isUSB = 0;
}

//Convert Number to Mode by KD8CEC
void byteWithFreqToMode(byte modeValue){
  if (modeValue == 3)
    isUSB = 1;
  else if (modeValue == 0)  //Not Set
    isUSB = (frequency > 10000000l) ? true : false;
  else
    isUSB = 0;
}

//VFO Toggle and save VFO Information, modified by KD8CEC
void menuVfoToggle(int btn, char isUseDelayTime)
{
  if (!btn){
    if (vfoActive == VFO_A)
      printLineF2(F("Select VFO B?"));
    else
      printLineF2(F("Select VFO A?"));    
  }
  else {
      if (vfoActive == VFO_B){
        vfoB = frequency;
        vfoB_mode = modeToByte();
        storeFrequencyAndMode(2); //vfoB -> eeprom
        
        vfoActive = VFO_A;
        frequency = vfoA;
        saveCheckFreq = frequency;
        byteToMode(vfoA_mode);
        printLineF2(F("Selected VFO A"));
      }
      else {
        vfoA = frequency;
        vfoA_mode = modeToByte();
        storeFrequencyAndMode(1); //vfoA -> eeprom
        
        vfoActive = VFO_B;
        frequency = vfoB;
        saveCheckFreq = frequency;
        byteToMode(vfoB_mode);
        printLineF2(F("Selected VFO B"));
      }

      ritDisable();

      if (isUseDelayTime == 1)        //Found Issue in wsjt-x Linux 32bit 
        delay_background(500, 0);
        
      printLine2ClearAndUpdate();
      //exit the menu
      menuOn = 0;
  }
}

void menuRitToggle(int btn){
  if (!btn){
    if (ritOn == 1)
      printLineF2(F("RIT:On, Off?"));
    else
      printLineF2(F("RIT:Off, On?"));
  }
  else {
      if (ritOn == 0){
        printLineF2(F("RIT is ON"));
        //enable RIT so the current frequency is used at transmit
        ritEnable(frequency);
      }
      else{
        printLineF2(F("RIT is OFF"));
        ritDisable();
      }
      menuOn = 0;
      delay_background(500, 0);
      printLine2ClearAndUpdate();
  }
}

void menuSidebandToggle(int btn){
  if (!btn){
    if (isUSB == true)
      printLineF2(F("Select LSB?"));
    else
      printLineF2(F("Select USB?"));
  }
  else {
      if (isUSB == true){
        isUSB = false;
        printLineF2(F("LSB Selected"));
      }
      else {
        isUSB = true;
        printLineF2(F("USB Selected"));
      }
    setFrequency(frequency);
    delay_background(500, 0);
    printLine2ClearAndUpdate();
    menuOn = 0;
  }
}

//Select CW Key Type by KD8CEC
void menuSetupKeyType(int btn){
  if (!btn && digitalRead(PTT) == HIGH){
      if (Iambic_Key)
        printLineF2(F("Key: Straight?"));
      else
        printLineF2(F("Key: Fn=A, PTT=B"));
  }
  else {
      if (Iambic_Key)
      {
        printLineF2(F("Straight Key!"));
        Iambic_Key = false;
      }
      else
      {
        Iambic_Key = true;
        if (btn)
        {
          keyerControl &= ~IAMBICB;
          printLineF2(F("IAMBICA Key!"));
        }
        else
        {
          keyerControl |= IAMBICB;
          printLineF2(F("IAMBICB Key!"));
        }
      }
      
    delay_background(500, 0);
    printLine2ClearAndUpdate();
    menuOn = 0;
  }
}

//Analog pin monitoring with CW Key and function keys connected.
//by KD8CEC
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
    adcPinA3 = analogRead(A3);  //A3(ORANGE, CW Key)
    adcPinA6 = analogRead(A6);  //A6(BLUE, Ptt)
    adcPinA7 = analogRead(A7);  //A7(VIOLET, Spare)

/*
  sprintf(c, "%4d %4d %4d", adcPinA0, adcPinA1, adcPinA2);
  printLine1(c);
  sprintf(c, "%4d %4d %4d", adcPinA3, adcPinA6, adcPinA7);
  printLine2(c);
*/  
  
    if (adcPinA6 < 10) {
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
      
  printLine2ClearAndUpdate();
  menuOn = 0;
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
        printLineF2(F("TX OFF!"));
      }
      else {
        isTxType &= ~(optionType);
        printLineF2(F("TX ON!"));
      }
    delay_background(500, 0);
    printLine2ClearAndUpdate();
    menuOn = 0;
  }
}

/**
 * The calibration routines are not normally shown in the menu as they are rarely used
 * They can be enabled by choosing this menu option
 */
void menuSetup(int btn){
  if (!btn){
    if (!modeCalibrate)
      printLineF2(F("Setup On?"));
    else
      printLineF2(F("Setup Off?"));
  }else {
    if (!modeCalibrate){
      modeCalibrate = true;
      printLineF2(F("Setup:On"));
    }
    else {
      modeCalibrate = false;
      printLineF2(F("Setup:Off"));
    }
   delay_background(2000, 0);
   printLine2Clear();
   menuOn = 0;
  }
}

void menuExit(int btn){

  if (!btn){
      printLineF2(F("Exit Menu?"));
  }
  else{
      printLine2ClearAndUpdate();
      menuOn = 0;
  }
}

void menuCWSpeed(int btn){
    int knob = 0;
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

    printLineF1(F("Press PTT to set"));
    strcpy(b, "WPM:");
    itoa(wpm,c, 10);
    strcat(b, c);
    printLine2(b);
    delay_background(300, 0);

    while(!btnDown() && digitalRead(PTT) == HIGH){

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
      //abort if this button is down
      if (btnDown())
        //re-enable the clock1 and clock 2
        break;

      Check_Cat(0);  //To prevent disconnections
    }
    
    //save the setting
    if (digitalRead(PTT) == LOW){
      printLineF2(F("CW Speed set!"));
      cwSpeed = 1200/wpm;
      EEPROM.put(CW_SPEED, cwSpeed);
      delay_background(2000, 0);
    }
    printLine2ClearAndUpdate();
    menuOn = 0;
}

//Builtin CW Keyer Logic by KD8CEC
void menuCWAutoKey(int btn){
    if (!btn){
     printLineF2(F("CW AutoKey Mode?"));
     return;
    }
    
    //Check CW_AUTO_MAGIC_KEY and CW Text Count
    EEPROM.get(CW_AUTO_COUNT, cwAutoTextCount);
    if (EEPROM.read(CW_AUTO_MAGIC_KEY) != 0x73 || cwAutoTextCount < 1)
    {
     printLineF2(F("Empty CW data"));
     delay_background(2000, 0);
     return;
    }

    printLineF1(F("Press PTT to Send"));
    delay_background(500, 0);
    updateDisplay();
    beforeCWTextIndex = 255;  //255 value is for start check
    isCWAutoMode = 1;
    menuOn = 0;
}

//Modified by KD8CEC
void menuSetupCwDelay(int btn){
    int knob = 0;
    int tmpCWDelay = cwDelayTime * 10;
     
    if (!btn){
     strcpy(b, "CW TX->RX Delay");
     printLine2(b);
     return;
    }

    printLineF1(F("Press PTT to set"));
    strcpy(b, "DELAY:");
    itoa(tmpCWDelay,c, 10);
    strcat(b, c);
    printLine2(b);
    delay_background(300, 0);

    while(!btnDown() && digitalRead(PTT) == HIGH){
      knob = enc_read();
      if (knob != 0){
        if (tmpCWDelay > 3 && knob < 0)
          tmpCWDelay -= 10;
        if (tmpCWDelay < 2500 && knob > 0)
          tmpCWDelay += 10;

        strcpy(b, "DELAY:");
        itoa(tmpCWDelay,c, 10);
        strcat(b, c);
        printLine2(b);
      }
      //abort if this button is down
      if (btnDown())
        break;

      Check_Cat(0);  //To prevent disconnections
    }
    
    //save the setting
    if (digitalRead(PTT) == LOW){
      printLineF2(F("CW Delay set!"));
      cwDelayTime = tmpCWDelay / 10;
      EEPROM.put(CW_DELAY, cwDelayTime);
      delay_background(2000, 0);
    }
    printLine2ClearAndUpdate();
    menuOn = 0;
}

//CW Time delay by KD8CEC
void menuSetupTXCWInterval(int btn){
    int knob = 0;
    int tmpTXCWInterval = delayBeforeCWStartTime * 2;
     
    if (!btn){
     strcpy(b, "CW Start Delay");
     printLine2(b);
     return;
    }

    printLineF1(F("Press PTT to set"));
    strcpy(b, "Start Delay:");
    itoa(tmpTXCWInterval,c, 10);
    strcat(b, c);
    printLine2(b);
    delay_background(300, 0);

    while(!btnDown() && digitalRead(PTT) == HIGH){
      knob = enc_read();
      if (knob != 0){
        if (tmpTXCWInterval > 0 && knob < 0)
          tmpTXCWInterval -= 2;
        if (tmpTXCWInterval < 500 && knob > 0)
          tmpTXCWInterval += 2;

        strcpy(b, "Start Delay:");
        itoa(tmpTXCWInterval,c, 10);
        strcat(b, c);
        printLine2(b);
      }
      //abort if this button is down
      if (btnDown())
        break;

      Check_Cat(0);  //To prevent disconnections
    }
    
    //save the setting
    if (digitalRead(PTT) == LOW){
      printLineF2(F("CW Start set!"));
      delayBeforeCWStartTime = tmpTXCWInterval / 2;
      EEPROM.put(CW_START, delayBeforeCWStartTime);
      delay_background(2000, 0);
    }
    printLine2ClearAndUpdate();
    menuOn = 0;
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
  delay(100);
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
  printLine2ClearAndUpdate();
  menuOn = 0;
}


void printCarrierFreq(unsigned long freq){

  memset(c, 0, sizeof(c));
  memset(b, 0, sizeof(b));

  ultoa(freq, b, DEC);
  
  strncat(c, b, 2);
  strcat(c, ".");
  strncat(c, &b[2], 3);
  strcat(c, ".");
  strncat(c, &b[5], 1);
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
      usbCarrier -= 50;
    else if (knob < 0)
      usbCarrier += 50;
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

  si5351bx_setfreq(0, usbCarrier);          
  setFrequency(frequency);    
  printLine2ClearAndUpdate();
  menuOn = 0; 
}

//Modified by KD8CEC
void menuSetupCwTone(int btn){
    int knob = 0;
    int prev_sideTone;
     
    if (!btn){
      printLineF2(F("Change CW Tone"));
      return;
    }

    prev_sideTone = sideTone;
    printLineF1(F("Tune CW tone"));
    printLineF2(F("PTT to confirm."));
    delay_background(1000, 0);
    tone(CW_TONE, sideTone);

    //disable all clock 1 and clock 2 
    while (digitalRead(PTT) == HIGH && !btnDown())
    {
      knob = enc_read();

      if (knob > 0 && sideTone < 2000)
        sideTone += 10;
      else if (knob < 0 && sideTone > 100 )
        sideTone -= 10;
      else
        continue; //don't update the frequency or the display
        
      tone(CW_TONE, sideTone);
      itoa(sideTone, b, 10);
      printLine2(b);

      delay(100);
      Check_Cat(0);  //To prevent disconnections
    }
    noTone(CW_TONE);
    //save the setting
    if (digitalRead(PTT) == LOW){
      printLineF2(F("Sidetone set!"));
      EEPROM.put(CW_SIDETONE, usbCarrier);
      delay_background(2000, 0);
    }
    else
      sideTone = prev_sideTone;
    
    printLine2ClearAndUpdate();
    menuOn = 0; 
 }

//Lock Dial move by KD8CEC
void setDialLock(byte tmpLock, byte fromMode) {
  if (tmpLock == 1)
    isDialLock |= (vfoActive == VFO_A ? 0x01 : 0x02);
  else
    isDialLock &= ~(vfoActive == VFO_A ? 0x01 : 0x02);
    
  if (fromMode == 2 || fromMode == 3) return;
  
  if (tmpLock == 1)
    printLineF2(F("Dial Lock ON"));
  else
    printLineF2(F("Dial Lock OFF"));

  delay_background(1000, 0);
  printLine2ClearAndUpdate();
}

unsigned int btnDownTimeCount;

#define PRESS_ADJUST_TUNE 1000
#define PRESS_LOCK_CONTROL 2000

//Modified by KD8CEC
void doMenu(){
  int select=0, i,btnState;
  char isNeedDisplay = 0;
  
  //for DialLock On/Off function
  btnDownTimeCount = 0;
  
  //wait for the button to be raised up

  //Appened Lines by KD8CEC for Adjust Tune step and Set Dial lock
  while(btnDown()){
    delay(50);
    Check_Cat(0);  //To prevent disconnections
    
    if (btnDownTimeCount++ == (PRESS_ADJUST_TUNE / 50)) { //Set Tune Step 
      printLineF2(F("Set Tune Step?"));
    }
    else if (btnDownTimeCount > (PRESS_LOCK_CONTROL / 50)) {  //check long time Down Button -> 2.5 Second => Lock
      if (vfoActive == VFO_A)
        setDialLock((isDialLock & 0x01) == 0x01 ? 0 : 1, 0); //Reverse Dial lock
      else
        setDialLock((isDialLock & 0x02) == 0x02 ? 0 : 1, 0); //Reverse Dial lock
      return;
    }
  }
  delay(50);  //debounce

  //ADJUST TUNE STEP 
  if (btnDownTimeCount > (PRESS_ADJUST_TUNE / 50))
  {
    printLineF1(F("Press Key to set"));
    isNeedDisplay = 1; //check to need display for display current value
    
    while (digitalRead(PTT) == HIGH && !btnDown())
    {
      Check_Cat(0);  //To prevent disconnections
      delay(50);  //debounce    

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

    printLineF2(F("Changed Step!"));
    //SAVE EEPROM
    EEPROM.put(TUNING_STEP, tuneStepIndex);
    delay_background(500, 0);
    printLine2ClearAndUpdate();
    return;
  }   //set tune step

  //Below codes are origial code with modified by KD8CEC
  //Select menu
  menuOn = 2;
  
  while (menuOn){
    i = enc_read();
    btnState = btnDown();

    if (i > 0){
      if (modeCalibrate && select + i < 170)
        select += i;
      if (!modeCalibrate && select + i < 80)
        select += i;
    }
    //if (i < 0 && select - i >= 0)
    if (i < 0 && select - i >= -10)
      select += i;      //caught ya, i is already -ve here, so you add it

    if (select < -5)
      menuExit(btnState);
    else if (select < 10)
      menuBand(btnState);
    else if (select < 20)
      menuRitToggle(btnState);
    else if (select < 30)
      menuVfoToggle(btnState, 1);
    else if (select < 40)
      menuSidebandToggle(btnState);
    else if (select < 50)
      menuCWSpeed(btnState);
    else if (select < 60)
      menuCWAutoKey(btnState);
    else if (select < 70)
      menuSetup(btnState);
    else if (select < 80 && !modeCalibrate)
      menuExit(btnState);
    else if (select < 90 && modeCalibrate)
      menuSetupCalibration(btnState);   //crystal
    else if (select < 100 && modeCalibrate)
      menuSetupCarrier(btnState);       //lsb
    else if (select < 110 && modeCalibrate)
      menuSetupCwTone(btnState);
    else if (select < 120 && modeCalibrate)
      menuSetupCwDelay(btnState);
    else if (select < 130 && modeCalibrate)
      menuSetupTXCWInterval(btnState);
    else if (select < 140 && modeCalibrate)
      menuSetupKeyType(btnState);
    else if (select < 150 && modeCalibrate)
      menuADCMonitor(btnState);
    else if (select < 160 && modeCalibrate)
      menuTxOnOff(btnState, 0x01);      //TX OFF / ON
    else if (select < 170 && modeCalibrate)
      menuExit(btnState);

    Check_Cat(0);  //To prevent disconnections
  }

  //debounce the button
  while(btnDown()){
    delay(50);
    Check_Cat(0);  //To prevent disconnections
  }
  delay(50);
}

