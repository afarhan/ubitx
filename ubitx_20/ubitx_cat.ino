/**
 * The CAT protocol is used by many radios to provide remote control to comptuers through
 * the serial port.
 * 
 * This is very much a work in progress. Parts of this code have been liberally
 * borrowed from other GPLicensed works like hamlib.
 * 
 * WARNING : This is an unstable version and it has worked with fldigi, 
 * it gives time out error with WSJTX 1.8.0  
 */

// The next 4 functions are needed to implement the CAT protocol, which
// uses 4-bit BCD formatting.
//
byte setHighNibble(byte b,byte v) {
  // Clear the high nibble
  b &= 0x0f;
  // Set the high nibble
  return b | ((v & 0x0f) << 4);
}

byte setLowNibble(byte b,byte v) {
  // Clear the low nibble
  b &= 0xf0;
  // Set the low nibble
  return b | (v & 0x0f);
}

byte getHighNibble(byte b) {
  return (b >> 4) & 0x0f;
}

byte getLowNibble(byte b) {
  return b & 0x0f;
}

// Takes a number and produces the requested number of decimal digits, staring
// from the least significant digit.  
//
void getDecimalDigits(unsigned long number,byte* result,int digits) {
  for (int i = 0; i < digits; i++) {
    // "Mask off" (in a decimal sense) the LSD and return it
    result[i] = number % 10;
    // "Shift right" (in a decimal sense)
    number /= 10;
  }
}

// Takes a frequency and writes it into the CAT command buffer in BCD form.
//
void writeFreq(unsigned long freq,byte* cmd) {
  // Convert the frequency to a set of decimal digits. We are taking 9 digits
  // so that we can get up to 999 MHz. But the protocol doesn't care about the
  // LSD (1's place), so we ignore that digit.
  byte digits[9];
  getDecimalDigits(freq,digits,9);
  // Start from the LSB and get each nibble 
  cmd[3] = setLowNibble(cmd[3],digits[1]);
  cmd[3] = setHighNibble(cmd[3],digits[2]);
  cmd[2] = setLowNibble(cmd[2],digits[3]);
  cmd[2] = setHighNibble(cmd[2],digits[4]);
  cmd[1] = setLowNibble(cmd[1],digits[5]);
  cmd[1] = setHighNibble(cmd[1],digits[6]);
  cmd[0] = setLowNibble(cmd[0],digits[7]);
  cmd[0] = setHighNibble(cmd[0],digits[8]);  
}

// This function takes a frquency that is encoded using 4 bytes of BCD
// representation and turns it into an long measured in Hz.
//
// [12][34][56][78] = 123.45678? Mhz
//
unsigned long readFreq(byte* cmd) {
    // Pull off each of the digits
    byte d7 = getHighNibble(cmd[0]);
    byte d6 = getLowNibble(cmd[0]);
    byte d5 = getHighNibble(cmd[1]);
    byte d4 = getLowNibble(cmd[1]); 
    byte d3 = getHighNibble(cmd[2]);
    byte d2 = getLowNibble(cmd[2]); 
    byte d1 = getHighNibble(cmd[3]);
    byte d0 = getLowNibble(cmd[3]); 
    return  
      (unsigned long)d7 * 100000000L +
      (unsigned long)d6 * 10000000L +
      (unsigned long)d5 * 1000000L + 
      (unsigned long)d4 * 100000L + 
      (unsigned long)d3 * 10000L + 
      (unsigned long)d2 * 1000L + 
      (unsigned long)d1 * 100L + 
      (unsigned long)d0 * 10L; 
}

/**
 * Responds to all the cat commands, emulates FT-817
 */
  
void processCATCommand(byte* cmd) {
  byte response[5];

  // Debugging code, enable it to fix the cat implementation
  
  count++;
  if (cmd[4] == 0x00){
    response[0]=0;
    Serial.write(response, 1);
  }
  else if (cmd[4] == 0x01) {
    unsigned long f = readFreq(cmd);
    setFrequency(f);   
    updateDisplay();
    //sprintf(b, "set:%ld", f); 
    //printLine2(b);

  }
  // Get frequency
  else if (cmd[4] == 0x03){
    writeFreq(frequency,response); // Put the frequency into the buffer
    if (isUSB)
      response[4] = 0x01; //USB
    else
      response[4] = 0x00; //LSB
    Serial.write(response,5);
    printLine2("cat:getfreq");
  }
  else if (cmd[4] == 0x07){ // set mode
    if (cmd[0] == 0x00 || cmd[0] == 0x03)
      isUSB = 0;
    else
      isUSB = 1;
    response[0] = 0x00;
    Serial.write(response, 1);
    setFrequency(frequency);
    //printLine2("cat: mode changed");
    //updateDisplay();
  }
  else if (cmd[4] == 0x88){
    if (inTx){
      stopTx();
      txCAT = false;
    }
    else
      response[0] = 0xf0;
    printLine2("tx > rx");
    Serial.write(response,1);
  }
  else if (cmd[4] == 0x08) { // PTT On
    if (!inTx) {
      response[0] = 0;
      txCAT = true;
      startTx(TX_SSB);
      updateDisplay();
    } else {
      response[0] = 0xf0;
    } 
    Serial.write(response,1);
    printLine2("rx > tx");
  }
  // Read TX keyed state
  else if (cmd[4] == 0x10) {
    if (!inTx) {
      response[0] = 0;
    } else {
      response[0] = 0xf0;
    } 
    Serial.write(response,1);
    printLine2("cat;0x10");
  }
  // PTT Off
  else if (cmd[4] == 0x88) {
    byte resBuf[0];
    if (inTx) {
      response[0] = 0;
    } else {
      response[0] = 0xf0;
    }
    Serial.write(response,1);
    printLine2("cat;0x88");
    //keyed = false;
    //digitalWrite(13,LOW);
  }
  // Read receiver status
  else if (cmd[4] == 0xe7) {
    response[0] = 0x09;
    Serial.write(response,1);
    printLine2("cat;0xe7");
  }
  else if (cmd[4] == 0xf5){
    
  }
  // Read receiver status
  else if (cmd[4] == 0xf7) {
    response[0] = 0x00;
    if (inTx) {
      response[0] = response[0] | 0xf0;
    }
    Serial.write(response,1);
    printLine2("cat;0xf7");
  }
  else {
    //somehow, get this to print the four bytes
    ultoa(*((unsigned long *)cmd), c, 16);
    itoa(cmd[4], b, 16);
    strcat(b, ":");
    strcat(b, c);
    printLine2(b);
    response[0] = 0x00;
    Serial.write(response[0]);
  }

}



void checkCAT(){
  static byte cat[5]; 
  byte i;

  if (Serial.available() < 5)
    return;

  cat[4] = cat[3];
  cat[3] = cat[2];
  cat[2] = cat[0];
  for (i = 0; i < 5; i++)
    cat[i] = Serial.read();

  processCATCommand(cat);
}


