/**
 * The user interface of the ubitx consists of the encoder, the push-button on top of it
 * and the 16x2 LCD display.
 * The upper line of the display is constantly used to display frequency and status
 * of the radio. Occasionally, it is used to provide a two-line information that is 
 * quickly cleared up.
 */
//#define printLineF1(x) (printLineF(1, x))
//#define printLineF2(x) (printLineF(0, x))

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

//char meter[17];

const PROGMEM uint8_t s_meter_bitmap[] = {
  B00000,B00000,B00000,B00000,B00000,B00100,B00100,B11011,
  B10000,B10000,B10000,B10000,B10100,B10100,B10100,B11011,
  B01000,B01000,B01000,B01000,B01100,B01100,B01100,B11011,
  B00100,B00100,B00100,B00100,B00100,B00100,B00100,B11011,
  B00010,B00010,B00010,B00010,B00110,B00110,B00110,B11011,
  B00001,B00001,B00001,B00001,B00101,B00101,B00101,B11011
};
PGM_P ps_meter_bitmap = reinterpret_cast<PGM_P>(s_meter_bitmap);

const PROGMEM uint8_t lock_bitmap[8] = {
  0b01110,
  0b10001,
  0b10001,
  0b11111,
  0b11011,
  0b11011,
  0b11111,
  0b00000};
PGM_P plock_bitmap = reinterpret_cast<PGM_P>(lock_bitmap);


// initializes the custom characters
// we start from char 1 as char 0 terminates the string!
void initMeter(){
  uint8_t tmpbytes[8];
  byte i;

  for (i = 0; i < 8; i++)
    tmpbytes[i] = pgm_read_byte(plock_bitmap + i);
  lcd.createChar(0, tmpbytes);
  
  for (i = 0; i < 8; i++)
    tmpbytes[i] = pgm_read_byte(ps_meter_bitmap + i);
  lcd.createChar(1, tmpbytes);

  for (i = 0; i < 8; i++)
    tmpbytes[i] = pgm_read_byte(ps_meter_bitmap + i + 8);
  lcd.createChar(2, tmpbytes);
  
  for (i = 0; i < 8; i++)
    tmpbytes[i] = pgm_read_byte(ps_meter_bitmap + i + 16);
  lcd.createChar(3, tmpbytes);
  
  for (i = 0; i < 8; i++)
    tmpbytes[i] = pgm_read_byte(ps_meter_bitmap + i + 24);
  lcd.createChar(4, tmpbytes);
  
  for (i = 0; i < 8; i++)
    tmpbytes[i] = pgm_read_byte(ps_meter_bitmap + i + 28);
  lcd.createChar(5, tmpbytes);
  
  for (i = 0; i < 8; i++)
    tmpbytes[i] = pgm_read_byte(ps_meter_bitmap + i + 32);
  lcd.createChar(6, tmpbytes);
}

/**
 * The meter is drawn with special characters.
 * character 1 is used to simple draw the blocks of the scale of the meter
 * characters 2 to 6 are used to draw the needle in positions 1 to within the block
 * This displays a meter from 0 to 100, -1 displays nothing
 */

 /*
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
*/

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

void printLineF(char linenmbr, const __FlashStringHelper *c)
{
  int i;
  char tmpBuff[17];
  PGM_P p = reinterpret_cast<PGM_P>(c);  

  for (i = 0; i < 17; i++){
    unsigned char fChar = pgm_read_byte(p++);
    tmpBuff[i] = fChar;
    if (fChar == 0)
      break;
  }

  printLine(linenmbr, tmpBuff);
}

#define LCD_MAX_COLUMN 16
void printLineFromEEPRom(char linenmbr, char lcdColumn, byte eepromStartIndex, byte eepromEndIndex) {
  lcd.setCursor(lcdColumn, linenmbr);

  for (byte i = eepromStartIndex; i <= eepromEndIndex; i++)
  {
    if (++lcdColumn <= LCD_MAX_COLUMN)
      lcd.write(EEPROM.read(USER_CALLSIGN_DAT + i));
    else
      break;
  }
  
  for (byte i = lcdColumn; i < 16; i++) //Right Padding by Space
      lcd.write(' ');
}

//  short cut to print to the first line
void printLine1(char *c){
  printLine(1,c);
}
//  short cut to print to the first line
void printLine2(char *c){
  printLine(0,c);
}

//  short cut to print to the first line
void printLine1Clear(){
  printLine(1,"");
}
//  short cut to print to the first line
void printLine2Clear(){
  printLine(0, "");
}

void printLine2ClearAndUpdate(){
  printLine(0, "");
  updateDisplay();
}

//012...89ABC...Z
char byteToChar(byte srcByte){
  if (srcByte < 10)
    return 0x30 + srcByte;
 else
    return 'A' + srcByte - 10;
}

// this builds up the top line of the display with frequency and mode
void updateDisplay() {
  // tks Jack Purdum W8TEE
  // replaced fsprint commmands by str commands for code size reduction
  
  // replace code for Frequency numbering error (alignment, point...) by KD8CEC
  int i;
  unsigned long tmpFreq = frequency; //
  
  memset(c, 0, sizeof(c));

  if (inTx){
    if (isCWAutoMode == 2) {
      for (i = 0; i < 4; i++)
        c[3-i] = (i < autoCWSendReservCount ? byteToChar(autoCWSendReserv[i]) : ' ');

      //display Sending Index
      c[4] = byteToChar(sendingCWTextIndex);
      c[5] = '=';
    }
    else {
      if (cwTimeout > 0)
        strcpy(c, "   CW:");
      else
        strcpy(c, "   TX:");
    }
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

  //display frequency
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

  //remarked by KD8CEC
  //already RX/TX status display, and over index (16 x 2 LCD)
  //if (inTx)
  //  strcat(c, " TX");
  printLine(1, c);

  if (isDialLock == 1) {
    lcd.setCursor(5,1);
    lcd.write((uint8_t)0);
  }
  else if (isCWAutoMode == 2){
    lcd.setCursor(5,1);
    lcd.write(0x7E);
  }
  else
  {
    lcd.setCursor(5,1);
    lcd.write(":");
  }

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


