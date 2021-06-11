/*
  OpenRadio Quisk Interface 
  Main
  This is used to provide a serial interface which Quisk SDR can talk to.
  
  Copyright (C) 2014 Mark Jessop <vk5qi@rfhead.net>

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

   Command Reference:
    ------------------
    All commands consist of an identifier, followed by one or more parameters delinieated by commas, then a newline.
    i.e.:
    COMMAND,<param1>,<param2><LF>
    
    A command will return OK<LF> if successful, and ERROR<LF> if not. Some commands (particuarly transmit commands)
    may take a significant amount of time to return.
    
    Sending a command which takes parameters, without sending the parameters, will return the set parameters.
    
    Frequency Settings:
    
    FREQ,<freq> 		- Set the center freq of the receiver, in Hz.
    FREQPSK,<freq>		- Set the transmit frequency, in Hz.

    Modulation:
    All transmissions will time out after 10 minutes. (if we implement it)
    
    CARRIER,<ON/OFF>	- Transmit a carrier.
    PSK,<rate>,message	- Send message using Phase Shift Keying, with the specified symbol rate.
*/

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <avr/pgmspace.h>
#warning __PGMSPACE_H_
#include <EEPROM.h>
#include <Wire.h>
#include "si5351.h"
#include "TimerOne.h"
#include "settings.h"
#include "pins.h"
#include "ring_buffer.h"

#define VERSION "0.3"

// Default output frequency
#define RX_FREQ 27000000
#define TX_FREQ 27000000
#define FREQ_LIMIT_LOWER 100000
#define FREQ_LIMIT_UPPER 30000000


#define SERIAL_BAUD     57600
//#define USE_JOHNSON_COUNTER  // Select this if you have a Johnson counter and need the Si5351 to make signals at 4*freq.

Si5351 si5351;

int tx_relay_state = 0;
int tx_state = 0;

static struct settings settings;

// Ring buffer for use with the interrupt-driven transmit buffer.
struct ring_buffer data_tx_buffer = { { 0 }, 0, 0};

/* Returns true if settings are valid */
static bool read_settings(void)
{
    uint8_t *p = (uint8_t *)&settings;
    int i = 0;

    while(i < sizeof(struct settings))
        *p++ = EEPROM.read(i++);

    return settings.magic == SETTINGS_MAGIC;
}

static void write_settings(void)
{
    uint8_t *p = (uint8_t *)&settings;
    int i = 0;

    // Invalidate checksum to guard against partial writes
    EEPROM.write(0, 0);

    while(i < sizeof(struct settings))
        EEPROM.write(i++, *p++);
}

// Input buffer for reading user input.
#define INPUTBUFLEN	64
//char inputBuffer[INPUTBUFLEN];
int inputBufferPtr = 0;
char tempBuffer[20]; // Buffer for string to int conversion.
String inputBuffer = "";

void setup(){
    // Initialise things.
    pinMode(TX_RX_SWITCH, OUTPUT);
    digitalWrite(TX_RX_SWITCH, LOW);
    pinMode(MODULATION, OUTPUT);
    digitalWrite(MODULATION, LOW);
    pinMode(LED, OUTPUT);
    digitalWrite(LED, LOW);

    Serial.begin(SERIAL_BAUD);
    Serial.setTimeout(1000 * 10);

    uint8_t rev_id = si5351_init();

    // Click the relay as an 'audible' startup indicator.
    digitalWrite(TX_RX_SWITCH, HIGH);
    delay(300);
    digitalWrite(TX_RX_SWITCH, LOW);

    if (!read_settings()) {
        //Serial.println(F("Resetting settings to defaults"));
        settings.magic = SETTINGS_MAGIC;
        settings.rx_freq = RX_FREQ;
        settings.tx_freq = TX_FREQ;
        settings.cal_factor = 1.0f;
    }

    set_rx_freq(settings.rx_freq);
    set_tx_freq(settings.tx_freq);


}

void loop(){
	if(Serial.available()){
		// Read in a byte.
		char inChar = (char)Serial.read();
		
		// Work out what the character is. If letters or numbers, add to
		// the buffer, if not, don't add.
		
		// If character is a newline, we have reached the end of the command.
		if( (inChar == '\n') || (inputBufferPtr>INPUTBUFLEN)){
			// Parse string
			if( parseCommand(inputBuffer)==0 ){
				Serial.println("OK");
			}else{
				Serial.println("ERROR");
			}
			inputBuffer = "";
			inputBufferPtr = 0;
		}else if( isalnum(inChar) || ispunct(inChar) || isblank(inChar) || inChar==4 ){
			// Add to the input buffer if printable and not a line ending.
			inputBuffer += inChar;
			inputBufferPtr++;
		}
	}
}

// CSV parser function. Splits input into arguments.
// I am so sorry...
int parseCommand(String input){
	int params = 0;
	
	if(input.length()<3){
		// No command is shorter than 3 characters. Error.
		return 1;
	}
	// At this point we have found some sort of command (maybe)
	

	// Search through for commas, populate an array of pointers, make pointer = -1 if nothing found.
	// NOTE: This is a horrible bit of code. If theres is a better way to do CSV parsing in Arduino, let me know!
	int separators[] = {-1,-1,-1,-1,-1,-1};

	separators[0] = input.indexOf(",");
	
	if(separators[0] != -1){ // If we find one comma, maybe there are more...
		int i = 1;
		while((separators[i-1] != -1) && i<6){
			params++;
			separators[i] = input.indexOf(",", separators[i-1]+1);
			i++;
		}
	
	}

	if(params==0){ // Either a request command or a command with no parameter
		if(input.startsWith("FREQPSK")){
			Serial.print("FREQPSK,"); Serial.println(settings.tx_freq);
			return 0;
		}else if(input.startsWith("FREQ")){
			Serial.print("FREQ,"); Serial.println(settings.rx_freq);
			return 0;
		}else if(input.startsWith("CAL")){
			Serial.print("CAL,"); Serial.println(settings.cal_factor,10);
			return 0;
		}else if(input.startsWith("VER")){
			Serial.print("VER,"); Serial.println("OpenRadio V1.1");
			return 0;
		}else{
			// No other commands are valid without parameters. Error
			return 1;
		}
	}else if(params==1){ // Commands with one argument.
		String param1 = input.substring(separators[0]+1); // Extract the argument for passing to the function.
		if(input.startsWith("FREQPSK")){
			return parsePSKFreq(param1);
		}else if(input.startsWith("FREQ")){
			return parseFreq(param1);
		}else if(input.startsWith("PSKFREQ")){
			return parsePSKFreq(param1);
		}else if(input.startsWith("CARRIER")){
			return parseCarrier(param1);
		}else if(input.startsWith("CHAN")){
			return set_channel(param1);
		}else{
			// No other commands are valid with only one parameter. Error
			return 1;
		}
	}else if(params==2){ // Commands with 2 parameters (PSK);
		String param1 = input.substring(separators[0]+1, separators[1]);
		String param2 = input.substring(separators[1]+1);
		
		if(input.startsWith("PSK")){
			return psk_transmit(param1,param2);
		}else{
			// No other commands are valid with only one parameter. Error
			return 1;
		}
	}

	return 1; // Cannot parse command, error.
}

int freqValid(uint32_t freq){
	return (freq>FREQ_LIMIT_LOWER) && (freq<FREQ_LIMIT_UPPER);
}

// Parse frequency, program radio.
int parseFreq(String input){
	unsigned long result = 0;
	unsigned long combinedresult = 0;
	char* endptr;
	
	input.toCharArray(tempBuffer,10);
	
	result = strtoul(tempBuffer,&endptr,10);
	// If strtoul fails to convert, *endptr contains the character which conversion failed on.
	// Otherwise, *endptr contains '\0'.
	if(*endptr != 0) return -1;
	
	if(freqValid(result)){
		set_rx_freq(result);
		Serial.print("FREQ,");Serial.println(result);
		return 0;
	}else{
		//Serial.println("Invalid input.");
		return -1;
	}	
}

// Carrier on/off
// This should transmit a carrier at an offset from the rx freq.
// We probably dont need this right now.
int parseCarrier(String carrier_str){
// TODO

}

// Transit a string of PSK (message) at (rate) baud.
// We need to figure out how to decide what the 'offset' from the RF freq is.
// Maybe add another command to set this?
int parsePSK(String rate, String message){
// TODO

}

// Set the frequency of the transmitted PSK signal.
// This is entirely independent of the receive chain, and we should
// be controlling if we want to lock this to within the RX passband 
// from within the client software.
int parsePSKFreq(String input){
	unsigned long result = 0;
	unsigned long combinedresult = 0;
	char* endptr;
	
	input.toCharArray(tempBuffer,10);
	
	result = strtoul(tempBuffer,&endptr,10);
	// If strtoul fails to convert, *endptr contains the character which conversion failed on.
	// Otherwise, *endptr contains '\0'.
	if(*endptr != 0) return -1;
	
	if(freqValid(result)){
		set_tx_freq(result);
		Serial.print("FREQPSK,");Serial.println(result);
		return 0;
	}else{
		//Serial.println("Invalid input.");
		return -1;
	}	

}


//
// Si5351 Helper Functions
//

uint8_t si5351_init(){
    si5351.init(SI5351_CRYSTAL_LOAD_8PF,0,0);
    si5351.set_pll(SI5351_PLL_FIXED, SI5351_PLLA);
    // Set output frequencies
    set_rx_freq(RX_FREQ);
    set_tx_freq(TX_FREQ);

    // Enable RX, and disable TX by default.
    si5351.output_enable(RX_CLOCK,1);
    si5351.output_enable(TX_CLOCK,0);
    si5351.output_enable(SPARE_CLOCK,0);

    // Read the status register and return the chip revision ID.
    si5351.update_status();
    return si5351.dev_status.REVID;

}

static void set_rx_freq(uint32_t freq)
{
    // Save actual value
    settings.rx_freq = freq; // (Hz)

#ifdef USE_JOHNSON_COUNTER
    // This is for models where quadrature mixer requires a 4X LO signal.
    si5351.set_freq(freq * settings.cal_factor * 4, SI5351_PLL_FIXED,
                    RX_CLOCK);
#else
	// This is for models that use the Si5351 to produce the I/Q on CLK0 and CLK1.
	uint64_t pll_freq;
	uint_fast8_t mult;
	// mult must be less than 128 (7 bits) according to documentation
	if (freq >= 26470588){
		mult = 22;
	} else if (freq >= 18000000) {
		mult = 34;
	} else if(freq > 12000000) {
    Serial.println("in the right multiplier.");
		mult = 50;
	} else if (freq >= 8000000) {
		mult = 75;
	} else if (freq > 5357143) {
		mult = 112;
	} else {
		mult = 128;
	}
  freq = (uint64_t) (freq*100ULL*settings.cal_factor);
	pll_freq = (uint64_t)mult*freq;
	si5351.set_freq_manual(freq, pll_freq, SI5351_CLK0);
	si5351.set_freq_manual(freq, pll_freq, SI5351_CLK1);
	// Now we can set CLK1 to have a 90 deg phase shift by entering
	// mult in the CLK1 phase register, since the ratio of the PLL to
	// the clock frequency is mult.
	si5351.set_phase(SI5351_CLK0, 0);
	si5351.set_phase(SI5351_CLK1, mult);
	// We need to reset the PLL before they will be in phase alignment
	si5351.pll_reset(SI5351_PLLA);
#endif
	// This is for switching bands in some models.
	if( freq <= 4000000) {
		digitalWrite(S0, HIGH);
		digitalWrite(S1, HIGH);
	}
	else if (freq <= 8000000) {
		digitalWrite(S0, LOW);
		digitalWrite(S1, HIGH);
	}
	else if (freq <= 16000000) {
		digitalWrite(S0, HIGH);
		digitalWrite(S1, LOW);
	}
	else {
		digitalWrite(S0, LOW);
		digitalWrite(S1, LOW);
	}				
}

static void set_tx_freq(uint32_t freq)
{
    // Save actual value
    settings.tx_freq = freq;

    // Let driver choose PLL settings. May glitch when changing frequencies.
    si5351.set_freq(freq * settings.cal_factor, TX_CLOCK);
}

void tx_enable(){
    si5351.output_enable(TX_CLOCK,1);
    tx_state = 1;
}

void tx_disable(){
    si5351.output_enable(TX_CLOCK,0);
    tx_state = 0;
}

void tx(){
    digitalWrite(TX_RX_SWITCH, HIGH);
    digitalWrite(LED, HIGH);
    tx_relay_state = 1;
}

void rx(){
    digitalWrite(TX_RX_SWITCH, LOW);
    digitalWrite(LED, LOW);
    tx_relay_state = 0;
}

#define MIN_CHANNEL 1
#define MAX_CHANNEL 30


#ifdef USE_FLASH
// From https://docs.google.com/spreadsheets/d/1KP5XsAHPCD2FsUW5RoqCfCJYRt2o03q6RN2TroPgEKk/edit#gid=0
const prog_uint16_t channel_list[] =
#else
const uint16_t channel_list[] =
#endif
        {8986, 8990, 8993, 8996, 9000, 9003, 9006, 9010, 9013, 9016, 9020, 9023, 9026,
        9030, 9033, 9036, 9040, 9043, 9046, 9050, 9053, 9056, 9060, 9063, 9066, 9070,
        9073, 9076, 9080, 9083};

int set_channel(String input)
{
	unsigned long chan = 0;
	char* endptr;
	
	input.toCharArray(tempBuffer,10);
	
	chan = strtoul(tempBuffer,&endptr,10);
	// If strtoul fails to convert, *endptr contains the character which conversion failed on.
	// Otherwise, *endptr contains '\0'.
	if(*endptr != 0) return -1;



    if (chan < MIN_CHANNEL || chan > MAX_CHANNEL) {
        return 1;
    }

#ifdef USE_FLASH
    uint32_t rx = (uint32_t)pgm_read_word_near(channel_list + chan-1) * 1000;
#else
    uint32_t rx = (uint32_t)channel_list[chan-1] * 1000;
#endif
    uint32_t tx = rx * 3 + 1500;

    set_rx_freq(rx);
    set_tx_freq(tx);

    // Save settings
    write_settings();

    Serial.print("CHAN,"); Serial.println(chan);
    return 0;
}

int psk_transmit(String param1, String param2){
	unsigned long baud_rate = 0;
	char* endptr;
	
	param1.toCharArray(tempBuffer,10);
	
	baud_rate = strtoul(tempBuffer,&endptr,10);
	// If strtoul fails to convert, *endptr contains the character which conversion failed on.
	// Otherwise, *endptr contains '\0'.
	if(*endptr != 0) return -1;

    tx();
    tx_enable();
    if (!bpsk_start(baud_rate)){
    	tx_disable();
    	rx();
        return 1;
    }
    delay(300); //  Short delay for RX sync to occur.
    // Push string into ring buffer for transmission.
    for(int i = 0; i<param2.length(); i++){
    	store_char(param2.charAt(i),&data_tx_buffer);
    }
    // Send an EOT control character, then a newline, to signify the end of the message.
    store_char((char)4, &data_tx_buffer);
    store_char('\n', &data_tx_buffer);
    while(data_waiting(&data_tx_buffer)>0){}
    delay(300);
    bpsk_stop();
    tx_disable();
    rx();

    return 0;
}
