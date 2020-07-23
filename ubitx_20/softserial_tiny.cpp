/*
Softserial for Nextion LCD and Control MCU
KD8CEC, Ian Lee
-----------------------------------------------------------------------
It is a library rewritten in C format based on SoftwareSerial.c.
I tried to use as much as possible without modifying the SoftwareSerial. 
But eventually I had to modify the code. 

I rewrote it in C for the following reasons.
 - Problems occurred when increasing Program Size and Program Memory
 - We had to reduce the program size.
   Of course, Software Serial is limited to one.
 - reduce the steps for transmitting and receiving

useage
extern void SWSerial_Begin(long speedBaud);
extern void SWSerial_Write(uint8_t b);
extern int SWSerial_Available(void);
extern int SWSerial_Read(void);
extern void SWSerial_Print(uint8_t *b);

If you use Softwreserial library instead of this library, you can modify the code as shown below.
I kept the function name of SoftwareSerial so you only need to modify a few lines of code.

define top of source code
#include <SoftwareSerial.h>
SoftwareSerial sSerial(10, 11); // RX, TX

replace source code
SWSerial_Begin to sSerial.begin
SWSerial_Write to sSerial.write
SWSerial_Available to sSerial.available
SWSerial_Read to sSerial.read

KD8CEC, Ian Lee
-----------------------------------------------------------------------
License
All licenses for the source code are subject to the license of the original source SoftwareSerial Library.
However, if you use or modify this code, please keep the all comments in this source code.
KD8CEC
-----------------------------------------------------------------------
License from SoftwareSerial
-----------------------------------------------------------------------
SoftwareSerial.cpp (formerly NewSoftSerial.cpp) - 
Multi-instance software serial library for Arduino/Wiring
-- Interrupt-driven receive and other improvements by ladyada
   (http://ladyada.net)
-- Tuning, circular buffer, derivation from class Print/Stream,
   multi-instance support, porting to 8MHz processors,
   various optimizations, PROGMEM delay tables, inverse logic and 
   direct port writing by Mikal Hart (http://www.arduiniana.org)
-- Pin change interrupt macros by Paul Stoffregen (http://www.pjrc.com)
-- 20MHz processor support by Garrett Mace (http://www.macetech.com)
-- ATmega1280/2560 support by Brett Hagman (http://www.roguerobotics.com/)

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

The latest version of this library can always be found at
http://arduiniana.org.
 */
#include <Arduino.h>

//================================================================
//Public Variable
//================================================================
#define TX_PIN 9
#define RX_PIN 8
#define _SS_MAX_RX_BUFF 35 // RX buffer size
#define PRINT_MAX_LENGTH 30

//================================================================
//Internal Variable from SoftwareSerial.c and SoftwareSerial.h
//================================================================
//variable from softwareserial.c and softwareserial.h
static uint8_t swr_receive_buffer[_SS_MAX_RX_BUFF];

volatile uint8_t *_transmitPortRegister;      //Write Port Register
uint8_t transmit_RegMask; //use Mask bit 1
uint8_t transmit_InvMask; //use mask bit 0

volatile uint8_t *_receivePortRegister;       //Read Port Register
uint8_t _receiveBitMask;

//delay value for Bit
uint16_t _tx_delay;

//delay value for Receive
uint16_t _rx_delay_stopbit;
uint16_t _rx_delay_centering;
uint16_t _rx_delay_intrabit;

//Customize for uBITX Protocol
int8_t receiveIndex = 0;
uint8_t receivedCommandLength = 0;
int8_t ffCount = 0;

//Values for Receive Buffer
//uint16_t _buffer_overflow;
//static volatile uint8_t _receive_buffer_head;
//static volatile uint8_t _receive_buffer_tail;

//Values for Interrupt (check Start Bit)
volatile uint8_t *_pcint_maskreg;
uint8_t _pcint_maskvalue;

//================================================================
//Internal Function from SoftwareSerial.c
//================================================================
uint16_t subtract_cap(uint16_t num, uint16_t sub) 
{
  if (num > sub)
    return num - sub;
  else
    return 1;
}

inline void tunedDelay(uint16_t delay) 
{
  _delay_loop_2(delay);
}

void setRxIntMsk(bool enable)
{
    if (enable)
      *_pcint_maskreg |= _pcint_maskvalue;
    else
      *_pcint_maskreg &= ~_pcint_maskvalue;
}

uint8_t rx_pin_read()
{
  return *_receivePortRegister & _receiveBitMask;
}

//
// The receive routine called by the interrupt handler
//
void softSerail_Recv()
{
#if GCC_VERSION < 40302
// Work-around for avr-gcc 4.3.0 OSX version bug
// Preserve the registers that the compiler misses
// (courtesy of Arduino forum user *etracer*)
  asm volatile(
    "push r18 \n\t"
    "push r19 \n\t"
    "push r20 \n\t"
    "push r21 \n\t"
    "push r22 \n\t"
    "push r23 \n\t"
    "push r26 \n\t"
    "push r27 \n\t"
    ::);
#endif  

  uint8_t d = 0;

  // If RX line is high, then we don't see any start bit
  // so interrupt is probably not for us
  if (!rx_pin_read())     //Start Bit
  {
    // Disable further interrupts during reception, this prevents
    // triggering another interrupt directly after we return, which can
    // cause problems at higher baudrates.
    setRxIntMsk(false);

    // Wait approximately 1/2 of a bit width to "center" the sample
    tunedDelay(_rx_delay_centering);

    // Read each of the 8 bits
    for (uint8_t i=8; i > 0; --i)
    {
      tunedDelay(_rx_delay_intrabit);
      d >>= 1;

      if (rx_pin_read())
        d |= 0x80;
    }

    if (receivedCommandLength == 0) //check Already Command
    {
        //Set Received Data
        swr_receive_buffer[receiveIndex++] = d;

        //Finded Command
        if (d == 0x73 && ffCount > 1 && receiveIndex > 6)
        {
          receivedCommandLength = receiveIndex;
          receiveIndex = 0;
          ffCount = 0;
        }
        else if (receiveIndex > _SS_MAX_RX_BUFF)
        {
          //Buffer Overflow
          receiveIndex = 0;
          ffCount = 0;
        }
        else if (d == 0xFF)
        {
          ffCount++;
        }
        else
        {
          ffCount = 0;
        }
    }
    
    // skip the stop bit
    tunedDelay(_rx_delay_stopbit);

    // Re-enable interrupts when we're sure to be inside the stop bit
    setRxIntMsk(true);
  }

#if GCC_VERSION < 40302
// Work-around for avr-gcc 4.3.0 OSX version bug
// Restore the registers that the compiler misses
  asm volatile(
    "pop r27 \n\t"
    "pop r26 \n\t"
    "pop r23 \n\t"
    "pop r22 \n\t"
    "pop r21 \n\t"
    "pop r20 \n\t"
    "pop r19 \n\t"
    "pop r18 \n\t"
    ::);
#endif
}

ISR(PCINT0_vect)
{
 softSerail_Recv();
}

//================================================================
//Public Function from SoftwareSerial.c and modified and create
//================================================================
// Read data from buffer
void SWSerial_Read(uint8_t * receive_cmdBuffer)
{
  for (int i = 0; i < receivedCommandLength; i++)
    receive_cmdBuffer[i] = swr_receive_buffer[i];
}

void SWSerial_Write(uint8_t b)
{
  volatile uint8_t *reg = _transmitPortRegister;
  uint8_t oldSREG = SREG;
  uint16_t delay = _tx_delay;

  cli();  // turn off interrupts for a clean txmit

  // Write the start bit
  *reg &= transmit_InvMask;

  tunedDelay(delay);

  // Write each of the 8 bits
  for (uint8_t i = 8; i > 0; --i)
  {
    if (b & 1) // choose bit
      *reg |= transmit_RegMask; // send 1
    else
      *reg &= transmit_InvMask; // send 0

    tunedDelay(delay);
    b >>= 1;
  }

  // restore pin to natural state
  *reg |= transmit_RegMask;

  SREG = oldSREG; // turn interrupts back on
  tunedDelay(_tx_delay);
}

void SWSerial_Print(uint8_t *b)
{
  for (int i = 0; i < PRINT_MAX_LENGTH; i++)
  {
    if (b[i] == 0x00)
      break;
    else
      SWSerial_Write(b[i]);
  }
}

void SWSerial_Begin(long speedBaud)
{
  //INT TX_PIN
  digitalWrite(TX_PIN, HIGH);
  pinMode(TX_PIN, OUTPUT);
  transmit_RegMask = digitalPinToBitMask(TX_PIN);   //use Bit 1
  transmit_InvMask = ~digitalPinToBitMask(TX_PIN);  //use Bit 0
  _transmitPortRegister = portOutputRegister(digitalPinToPort(TX_PIN));

  //INIT RX_PIN
  pinMode(RX_PIN, INPUT);
  digitalWrite(RX_PIN, HIGH);  // pullup for normal logic!
  _receiveBitMask = digitalPinToBitMask(RX_PIN);
  _receivePortRegister = portInputRegister(digitalPinToPort(RX_PIN));

  //Set Values
  uint16_t bit_delay = (F_CPU / speedBaud) / 4;
  _tx_delay = subtract_cap(bit_delay, 15 / 4);

  if (digitalPinToPCICR(RX_PIN)) 
  {
    _rx_delay_centering = subtract_cap(bit_delay / 2, (4 + 4 + 75 + 17 - 23) / 4);
    _rx_delay_intrabit = subtract_cap(bit_delay, 23 / 4);
    _rx_delay_stopbit = subtract_cap(bit_delay * 3 / 4, (37 + 11) / 4);
    *digitalPinToPCICR(RX_PIN) |= _BV(digitalPinToPCICRbit(RX_PIN));
    _pcint_maskreg = digitalPinToPCMSK(RX_PIN);
    _pcint_maskvalue = _BV(digitalPinToPCMSKbit(RX_PIN));

    tunedDelay(_tx_delay); // if we were low this establishes the end
  }

  //Start Listen
  setRxIntMsk(true);
}
