/**
 * The user interface of the ubitx consists of the encoder, the push-button on top of it
 * and the 16x2 LCD display.
 * The upper line of the display is constantly used to display frequency and status
 * of the radio. Occasionally, it is used to provide a two-line information that is 
 * quickly cleared up.
 */

//returns true if the button is pressed
int btnDown(){
  if (digitalRead(FBUTTON) == HIGH)
    return 0;
  else
    return 1;
}

/**
 * Meter (not used in this build for anything)
 * the meter is drawn using special characters. Each character is composed of 5 x 8 matrix.
 * The  s_meter array holds the definition of the these characters. 
 * each line of the array is is one character such that 5 bits of every byte 
 * makes up one line of pixels of the that character (only 5 bits are used)
 * The current reading of the meter is assembled in the string called meter
 */

char meter[17];

byte s_meter_bitmap[] = {
  B00000,B00000,B00000,B00000,B00000,B00100,B00100,B11011,
  B10000,B10000,B10000,B10000,B10100,B10100,B10100,B11011,
  B01000,B01000,B01000,B01000,B01100,B01100,B01100,B11011,
  B00100,B00100,B00100,B00100,B00100,B00100,B00100,B11011,
  B00010,B00010,B00010,B00010,B00110,B00110,B00110,B11011,
  B00001,B00001,B00001,B00001,B00101,B00101,B00101,B11011
};



// initializes the custom characters
// we start from char 1 as char 0 terminates the string!
void initMeter(){
  lcd.createChar(1, s_meter_bitmap);
  lcd.createChar(2, s_meter_bitmap + 8);
  lcd.createChar(3, s_meter_bitmap + 16);
  lcd.createChar(4, s_meter_bitmap + 24);
  lcd.createChar(5, s_meter_bitmap + 32);
  lcd.createChar(6, s_meter_bitmap + 40);
}

/**
 * The meter is drawn with special characters.
 * character 1 is used to simple draw the blocks of the scale of the meter
 * characters 2 to 6 are used to draw the needle in positions 1 to within the block
 * This displays a meter from 0 to 100, -1 displays nothing
 */
void drawMeter(int8_t needle){
  int16_t best, i, s;

  if (needle < 0)
    return;

  s = (needle * 4)/10;
  for (i = 0; i < 8; i++){
    if (s >= 5)
      meter[i] = 1;
    else if (s >= 0)
      meter[i] = 2 + s;
    else
      meter[i] = 1;
    s = s - 5;
  }
  if (needle >= 40)
    meter[i-1] = 6;
  meter[i] = 0;
}

// The generic routine to display one line on the LCD 
void printLine(char linenmbr, char *c) {
  if (strcmp(c, printBuff[linenmbr])) {     // only refresh the display when there was a change
    lcd.setCursor(0, linenmbr);             // place the cursor at the beginning of the selected line
    lcd.print(c);
    strcpy(printBuff[linenmbr], c);

    for (byte i = strlen(c); i < 16; i++) { // add white spaces until the end of the 16 characters line is reached
      lcd.print(' ');
    }
  }
}

//  short cut to print to the first line
void printLine1(char *c){
  printLine(1,c);
}
//  short cut to print to the first line
void printLine2(char *c){
  printLine(0,c);
}

// this builds up the top line of the display with frequency and mode
void updateDisplay() {
  // tks Jack Purdum W8TEE
  // replaced fsprint commmands by str commands for code size reduction

  memset(c, 0, sizeof(c));
  memset(b, 0, sizeof(b));

  ultoa(frequency, b, DEC);

  if (inTx){
    if (cwTimeout > 0)
      strcpy(c, "   CW:");
    else
      strcpy(c, "   TX:");
  }
  else {
    if (ritOn)
      strcpy(c, "RIT ");
    else {
      if (isUSB)
        strcpy(c, "USB ");
      else
        strcpy(c, "LSB ");
    }
    if (vfoActive == VFO_A) // VFO A is active
      strcat(c, "A:");
    else
      strcat(c, "B:");
  }



  //one mhz digit if less than 10 M, two digits if more
  if (frequency < 10000000l){
    c[6] = ' ';
    c[7]  = b[0];
    strcat(c, ".");
    strncat(c, &b[1], 3);    
    strcat(c, ".");
    strncat(c, &b[4], 3);
  }
  else {
    strncat(c, b, 2);
    strcat(c, ".");
    strncat(c, &b[2], 3);
    strcat(c, ".");
    strncat(c, &b[5], 3);    
  }

  if (inTx)
    strcat(c, " TX");
  printLine(1, c);

/*
  //now, the second line
  memset(c, 0, sizeof(c));
  memset(b, 0, sizeof(b));

  if (inTx)
    strcat(c, "TX ");
  else if (ritOn)
    strcpy(c, "RIT");

  strcpy(c, "      \xff");
  drawMeter(meter_reading);
  strcat(c, meter);
  strcat(c, "\xff");
  printLine2(c);*/
}

int enc_prev_state = 3;

/**
 * The A7 And A6 are purely analog lines on the Arduino Nano
 * These need to be pulled up externally using two 10 K resistors
 * 
 * There are excellent pages on the Internet about how these encoders work
 * and how they should be used. We have elected to use the simplest way
 * to use these encoders without the complexity of interrupts etc to 
 * keep it understandable.
 * 
 * The enc_state returns a two-bit number such that each bit reflects the current
 * value of each of the two phases of the encoder
 * 
 * The enc_read returns the number of net pulses counted over 50 msecs. 
 * If the puluses are -ve, they were anti-clockwise, if they are +ve, the
 * were in the clockwise directions. Higher the pulses, greater the speed
 * at which the enccoder was spun
 */

byte enc_state (void) {
    return (analogRead(ENC_A) > 500 ? 1 : 0) + (analogRead(ENC_B) > 500 ? 2: 0);
}

int enc_read(void) {
  int result = 0; 
  byte newState;
  int enc_speed = 0;
  
  long stop_by = millis() + 50;
  
  while (millis() < stop_by) { // check if the previous state was stable
    newState = enc_state(); // Get current state  
    
    if (newState != enc_prev_state)
      delay (1);
    
    if (enc_state() != newState || newState == enc_prev_state)
      continue; 
    //these transitions point to the encoder being rotated anti-clockwise
    if ((enc_prev_state == 0 && newState == 2) || 
      (enc_prev_state == 2 && newState == 3) || 
      (enc_prev_state == 3 && newState == 1) || 
      (enc_prev_state == 1 && newState == 0)){
        result--;
      }
    //these transitions point o the enccoder being rotated clockwise
    if ((enc_prev_state == 0 && newState == 1) || 
      (enc_prev_state == 1 && newState == 3) || 
      (enc_prev_state == 3 && newState == 2) || 
      (enc_prev_state == 2 && newState == 0)){
        result++;
      }
    enc_prev_state = newState; // Record state for next pulse interpretation
    enc_speed++;
    delay(1);
  }
  return(result);
}


