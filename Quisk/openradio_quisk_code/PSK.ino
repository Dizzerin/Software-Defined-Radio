/*
	Multi-mode HF data software for Arduino + Si5351
	
	PSK Module.
	
	Copyright (C) 2014 Mark Jessop <vk5qi@rfhead.net>

    Hacked up to work with the Open Radio Miniconf Transmitter. 2014-09-22

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    For a full copy of the GNU General Public License, 
    see <http://www.gnu.org/licenses/>.
*/

#include <avr/pgmspace.h>

#include "TimerOne.h"

#include "ring_buffer.h"
#include "pins.h"

extern struct ring_buffer data_tx_buffer;

static const uint16_t bpsk_varicode[] = {
	0x0355, // 0x00 =>  1010101011 (NUL)
	0x036d, // 0x01 =>  1011011011 (SOH)
	0x02dd, // 0x02 =>  1011101101 (STX)
	0x03bb, // 0x03 =>  1101110111 (ETX)
	0x035d, // 0x04 =>  1011101011 (EOT)
	0x03eb, // 0x05 =>  1101011111 (ENQ)
	0x03dd, // 0x06 =>  1011101111 (ACK)
	0x02fd, // 0x07 =>  1011111101 (BEL)
	0x03fd, // 0x08 =>  1011111111 (BS)
	0x00f7, // 0x09 =>  11101111 (HT)
	0x0017, // 0x0a =>  11101 (LF)
	0x03db, // 0x0b =>  1101101111 (VT)
	0x02ed, // 0x0c =>  1011011101 (FF)
	0x001f, // 0x0d =>  11111 (CR)
	0x02bb, // 0x0e =>  1101110101 (SO)
	0x0357, // 0x0f =>  1110101011 (SI)
	0x03bd, // 0x10 =>  1011110111 (DLE)
	0x02bd, // 0x11 =>  1011110101 (DC1)
	0x02d7, // 0x12 =>  1110101101 (DC2)
	0x03d7, // 0x13 =>  1110101111 (DC3)
	0x036b, // 0x14 =>  1101011011 (DC4)
	0x035b, // 0x15 =>  1101101011 (NAK)
	0x02db, // 0x16 =>  1101101101 (SYN)
	0x03ab, // 0x17 =>  1101010111 (ETB)
	0x037b, // 0x18 =>  1101111011 (CAN)
	0x02fb, // 0x19 =>  1101111101 (EM)
	0x03b7, // 0x1a =>  1110110111 (SUB)
	0x02ab, // 0x1b =>  1101010101 (ESC)
	0x02eb, // 0x1c =>  1101011101 (FS)
	0x0377, // 0x1d =>  1110111011 (GS)
	0x037d, // 0x1e =>  1011111011 (RS)
	0x03fb, // 0x1f =>  1101111111 (US)
	0x0001, // 0x20 =>  1 ([[Space (punctuation)|SP]])
	0x01ff, // 0x21 =>  111111111 ([[Exclamation mark|!]])
	0x01f5, // 0x22 =>  101011111 ([[Quotation mark|"]])
	0x015f, // 0x23 =>  111110101 ([[Number sign|#]])
	0x01b7, // 0x24 =>  111011011 ([[Dollar sign|$]])
	0x02ad, // 0x25 =>  1011010101 ([[Percent sign|%]])
	0x0375, // 0x26 =>  1010111011 ([[Ampersand|&]])
	0x01fd, // 0x27 =>  101111111 ([[Apostrophe (punctuation)|']])
	0x00df, // 0x28 =>  11111011 ([[Bracket|(]])
	0x00ef, // 0x29 =>  11110111 ([[Bracket|)]])
	0x01ed, // 0x2a =>  101101111 ([[Asterisk|*]])
	0x01f7, // 0x2b =>  111011111 ([[Plus sign|+]])
	0x0057, // 0x2c =>  1110101 ([[Comma (punctuation)|,]])
	0x002b, // 0x2d =>  110101 ([[Hyphen|-]])
	0x0075, // 0x2e =>  1010111 ([[Full stop|.]])
	0x01eb, // 0x2f =>  110101111 ([[Slash (punctuation)|/]])
	0x00ed, // 0x30 =>  10110111 ([[0 (number)|0]])
	0x00bd, // 0x31 =>  10111101 ([[1 (number)|1]])
	0x00b7, // 0x32 =>  11101101 ([[2 (number)|2]])
	0x00ff, // 0x33 =>  11111111 ([[3 (number)|3]])
	0x01dd, // 0x34 =>  101110111 ([[4 (number)|4]])
	0x01b5, // 0x35 =>  101011011 ([[5 (number)|5]])
	0x01ad, // 0x36 =>  101101011 ([[6 (number)|6]])
	0x016b, // 0x37 =>  110101101 ([[7 (number)|7]])
	0x01ab, // 0x38 =>  110101011 ([[8 (number)|8]])
	0x01db, // 0x39 =>  110110111 ([[9 (number)|9]])
	0x00af, // 0x3a =>  11110101 ([[Colon (punctuation)|:]])
	0x017b, // 0x3b =>  110111101 ([[Semicolon|;]])
	0x016f, // 0x3c =>  111101101 ([[Less-than sign|<]])
	0x0055, // 0x3d =>  1010101 ([[Equals sign|=]])
	0x01d7, // 0x3e =>  111010111 ([[Greater-than sign|>]])
	0x03d5, // 0x3f =>  1010101111 ([[Question mark|?]])
	0x02f5, // 0x40 =>  1010111101 ([[@]])
	0x005f, // 0x41 =>  1111101 ([[A]])
	0x00d7, // 0x42 =>  11101011 ([[B]])
	0x00b5, // 0x43 =>  10101101 ([[C]])
	0x00ad, // 0x44 =>  10110101 ([[D]])
	0x0077, // 0x45 =>  1110111 ([[E]])
	0x00db, // 0x46 =>  11011011 ([[F]])
	0x00bf, // 0x47 =>  11111101 ([[G]])
	0x0155, // 0x48 =>  101010101 ([[H]])
	0x007f, // 0x49 =>  1111111 ([[I]])
	0x017f, // 0x4a =>  111111101 ([[J]])
	0x017d, // 0x4b =>  101111101 ([[K]])
	0x00eb, // 0x4c =>  11010111 ([[L]])
	0x00dd, // 0x4d =>  10111011 ([[M]])
	0x00bb, // 0x4e =>  11011101 ([[N]])
	0x00d5, // 0x4f =>  10101011 ([[O]])
	0x00ab, // 0x50 =>  11010101 ([[P]])
	0x0177, // 0x51 =>  111011101 ([[Q]])
	0x00f5, // 0x52 =>  10101111 ([[R]])
	0x007b, // 0x53 =>  1101111 ([[S]])
	0x005b, // 0x54 =>  1101101 ([[T]])
	0x01d5, // 0x55 =>  101010111 ([[U]])
	0x015b, // 0x56 =>  110110101 ([[V]])
	0x0175, // 0x57 =>  101011101 ([[W]])
	0x015d, // 0x58 =>  101110101 ([[X]])
	0x01bd, // 0x59 =>  101111011 ([[Y]])
	0x02d5, // 0x5a =>  1010101101 ([[Z]])
	0x01df, // 0x5b =>  111110111 ([[Bracket|<nowiki>[</nowiki>]])
	0x01ef, // 0x5c =>  111101111 ([[Backslash|\]])
	0x01bf, // 0x5d =>  111111011 ([[Bracket|<nowiki>]</nowiki>]])
	0x03f5, // 0x5e =>  1010111111 ([[Caret|^]])
	0x016d, // 0x5f =>  101101101 ([[Underscore|_]])
	0x03ed, // 0x60 =>  1011011111 ([[Grave accent|`]])
	0x000d, // 0x61 =>  1011 ([[a]])
	0x007d, // 0x62 =>  1011111 ([[b]])
	0x003d, // 0x63 =>  101111 ([[c]])
	0x002d, // 0x64 =>  101101 ([[d]])
	0x0003, // 0x65 =>  11 ([[e]])
	0x002f, // 0x66 =>  111101 ([[f]])
	0x006d, // 0x67 =>  1011011 ([[g]])
	0x0035, // 0x68 =>  101011 ([[h]])
	0x000b, // 0x69 =>  1101 ([[i]])
	0x01af, // 0x6a =>  111101011 ([[j]])
	0x00fd, // 0x6b =>  10111111 ([[k]])
	0x001b, // 0x6c =>  11011 ([[l]])
	0x0037, // 0x6d =>  111011 ([[m]])
	0x000f, // 0x6e =>  1111 ([[n]])
	0x0007, // 0x6f =>  111 ([[o]])
	0x003f, // 0x70 =>  111111 ([[p]])
	0x01fb, // 0x71 =>  110111111 ([[q]])
	0x0015, // 0x72 =>  10101 ([[r]])
	0x001d, // 0x73 =>  10111 ([[s]])
	0x0005, // 0x74 =>  101 ([[t]])
	0x003b, // 0x75 =>  110111 ([[u]])
	0x006f, // 0x76 =>  1111011 ([[v]])
	0x006b, // 0x77 =>  1101011 ([[w]])
	0x00fb, // 0x78 =>  11011111 ([[x]])
	0x005d, // 0x79 =>  1011101 ([[y]])
	0x0157, // 0x7a =>  111010101 ([[z]])
	0x03b5, // 0x7b =>  1010110111 ([[Bracket|&#123;]])
	0x01bb, // 0x7c =>  110111011 ([[Vertical bar|&#124;]])
	0x02b5, // 0x7d =>  1010110101 ([[Bracket|&#125;]])
	0x03ad, // 0x7e =>  1011010111 ([[Tilde|~]])
	0x02b7, // 0x7f =>  1110110101 (DEL)
};

// Phase Reversal routine. Currently this just toggles an IO line.
void bpsk_phase_switch(){
    digitalToggleFast(MODULATION);
}

// Interrupt driven transmit routines

uint16_t bpsk_current_word = 0;
uint8_t current_txbit = 0;
uint8_t bpsk_stopbit = 0;
volatile boolean bpsk_enabled = 0;

// Add characters to the transmit ring buffer
void bpsk_add_data(char* string){
	char c = *string++;
	while (c != 0){
		store_char(c,&data_tx_buffer);
		c = *string++;
	}
}

bool bpsk_start(uint16_t baud_rate){
    // Set up timer interrupt based on baud rate.
    switch(baud_rate){
        case 31:
            Timer1.initialize(32000);
            break;
        case 63:
            Timer1.initialize(15873);
            break;
        case 125:
            Timer1.initialize(8000);
            break;
        case 250:
            Timer1.initialize(4000);
            break;
        case 500:
            Timer1.initialize(2000);
            break;
        case 1000:
            Timer1.initialize(1000);
            break;
        default:
            Serial.print("ERROR: buad rate not supported\n");
            Serial.println(baud_rate);
    }

    Timer1.attachInterrupt(bpsk_isr);
}

void bpsk_stop(){
    Timer1.detachInterrupt();
    bpsk_current_word = 0;
    current_txbit = 0;
}

// Interrupt Driven BPSK
void bpsk_isr(){

    // Check the current bit to be transmitted. 
    if(current_txbit == 0){
        bpsk_phase_switch();
    }
    
	// If we have finished processing a word, try to read in another character.
	if(bpsk_current_word == 0 && (data_waiting(&data_tx_buffer)>0) ){
		unsigned char c = read_char(&data_tx_buffer);
		if(c != 0){
			bpsk_current_word = bpsk_varicode[c];
		}
		bpsk_stopbit = 2; // Transmit 2 phase reversals, to signify the end of a character.
	}
	
    if(bpsk_stopbit==0){
        // If we have finished our between-character phase reverals, start clocking through bits
        current_txbit = bpsk_current_word & 1;
        bpsk_current_word >>= 1;
    }else{
        // Else decrement the stop-bit counter and transmit another phase reversal.
        bpsk_stopbit--;
        current_txbit = 0;
    }
	
} 




