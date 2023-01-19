/*
 * nRF24L01 BMP180 and ADC tutorial
 * Created: 08.09.2022
 * Author: Markus Kollarik
 */ 

#ifndef F_CPU				//Define F_CPU if not done 
#define F_CPU 8000000UL
#endif

#include <stdbool.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include <stdlib.h>
#include "spi.h"
#include "wl_module.h"
#include "nRF24L01.h"
#include "aes.h"
#include "bmp085/bmp085.h"

#define SENDRATE 1  //Send data every x*8s
					//1...8sec
					//5...40sec
					//15...2min
					//60...10min

volatile uint8_t timercounter=0;

int main(void)
{
	uint8_t payload[wl_module_PAYLOAD];	//payload array
	uint8_t maincounter = 0;			//track packages id
	uint8_t k;							//for loop variable
	uint8_t key[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15}; //aes encryption key
	uint8_t expandedKey[240]; //aes encryption help variable
	uint8_t tmpOutput[16]; //encrypted data
	
	union Temp
	{
		double dtemp;
		uint8_t itemp[4];
	}temp;
	
	union Press
	{
		long lpress;
		uint8_t ipress[4];
	}press;
	
	union Adcdata
	{
		uint16_t valadc16;
		uint8_t valadc8[2];
	}adcdata;
	
	DDRC |= (1 << PC1)|(1 << PC3);           // Setup PC1 (LED) as output ; PC3 (ADC Switch on/off)
	
	ADMUX = (1<<REFS1)|(1<<REFS0)|(1<<MUX1); //Internal 2.56V Voltage Reference with external capacitor at AREF pin and ADC2
	ADCSRA = (1<<ADEN)|(1<<ADPS2);	// Enable ADC, set prescaler to 16
												// Fadc=Fcpu/prescaler=1000000/16=62.5kHz
												// Fadc=Fcpu/prescaler=8000000/64=125kHz
												// Fadc should be between 50kHz and 200kHz
									
	//ACSR = 0x80; // disable analog comparator
	
	wl_module_init();	//initialize nRF24L01+ module
	_delay_ms(50);		//wait 50ms for nRF24L01+ module to stabilize signals
	
	ASSR |= (1<<AS2); //enable asynchron mode - clock timer/counter2 from crystal oscillator connected to the TOSC1 pin
	TCCR2 |= (1<<CS22)|(1<<CS21)|(1<<CS20); //set prescaler to 1024 
	while((ASSR & (1<< TCR2UB))); //wait for end of access
	TIFR   = (1<<TOV2); //clear interrupts (*) datasheet: "Alternatively, TOV2 is cleared by writing a logic one to the flag."
	TIMSK |= (1 << TOIE2); //enable timer compare match interrupt - if overflow in timer/counter2 occurs -> interrupt
	TCNT2 = 0; //initialize the timer/counter2 with 0
	
	_delay_ms(1000); //wait 1 second to stabilize signals on the 32.768kHz crystal oscillator
	
	sei(); //set enable interrupts
	
	wl_module_tx_config(wl_module_TX_NR_0);	//config nRF24L01+ module as transmitter
	
	bmp085_init(); //initialize BMP180
	
    while(1)
    {
		OCR2 = 0;                       // dummy access
		while((ASSR & (1<< OCR2UB)));   // wait for end of access
			
		set_sleep_mode(SLEEP_MODE_PWR_SAVE); //set sleep mode power-save
		sleep_enable();	//enable sleep mode
		sleep_mode(); //change into sleep mode
		sleep_disable(); //first thing after waking from sleep:
		
		if(timercounter>(SENDRATE-1)) //check if enough time has passed to send next package			
		{
			timercounter = 0; //reset timercounter
			
			//twi module reset otherwise twi doesnt work after power save
			TWCR &= ~((1 << TWSTO) | (1 << TWEN));
			TWCR |= (1 << TWEN);
			
			for (k=0; k<=wl_module_PAYLOAD-1; k++) //initialize payload with dummy data
			{
				payload[k] = k;
			}
			
			temp.dtemp = bmp085_gettemperature(); //[degC]
			press.lpress = bmp085_getpressure(); //[Pa]
			
			//********adclines*********
			PORTC &= ~(1 << PC3); //switch on power supply for trimmer
			ADCSRA |= (1<<ADEN)|(1<<ADSC);	//start first conversion
			while(ADCSRA &(1<<ADSC));     //wait until conversion is finished
			//ACSR = 0x80;
			ADCSRA &= ~(1<<ADEN); //switch off ADC
			PORTC |= (1<<PC3); //switch off power supply for trimmer
			
			adcdata.valadc16=ADC;
			//*************************
			
			payload[0] = maincounter; //update id, to show which package send
			
			for(uint8_t i=0; i<4; ++i) //payload data 1:4 represents temperature / 4=MSB : little-endian
			{
				payload[i+1]=temp.itemp[i];
			} 
			for(uint8_t i=0; i<4; ++i) //payload data 5:8 represents pressure / 8=MSB : little-endian
			{
				payload[i+5]=press.ipress[i];
			}
			for(uint8_t i=0; i<2; ++i) //payload data 9:10 represents adc value / 10=MSB : little-endian
			{
				payload[i+9]=adcdata.valadc8[i];
			}
			
			//aes encryption
			aes_expandKey(key, expandedKey, sizeof(expandedKey), AES_TYPE_128);
			aes_encryptWithExpandedKey(tmpOutput, payload, expandedKey, AES_TYPE_128);
			
			wl_module_power_up(); //wake up the nRF24L01+ module from power_down
			wl_module_send(tmpOutput,wl_module_PAYLOAD); //transmit payload
			wl_module_power_down(); //enter power_down
			
			maincounter++; //increment package id
			
			if (maincounter >250) //reset package id after 250
			{
				maincounter = 0;
			}
		}
	}
}

ISR(TIMER2_OVF_vect) { //increment timercounter every timer2 overflow 
	timercounter++;
}

//code copied and improved from the: nRF24L01_Tutorial_Sender.c file; Created: 06.01.2012 20:15:04 by Author: Ernst Buchmann
//(IRQ: Package has not been sent, send again) didn't work -> got stuck -> improved version works (old code commented)
ISR(INT0_vect)
{
	uint8_t status;
	
	// Read wl_module status
	wl_module_CSN_lo;                               // Pull down chip select
	status = spi_fast_shift(NOP);					// Read status register
	wl_module_CSN_hi;                               // Pull up chip select
	
	
	if (status & (1<<TX_DS))							// IRQ: Package has been sent
	{
		wl_module_config_register(STATUS, (1<<TX_DS));	//Clear Interrupt Bit
		PTX=0;
	}
	
	if (status & (1<<MAX_RT))							// IRQ: Package has not been sent, send again
	{
		/*
		wl_module_config_register(STATUS, (1<<MAX_RT));	// Clear Interrupt Bit
		wl_module_CE_hi;								// Start transmission
		_delay_us(10);
		wl_module_CE_lo;
		*/
		wl_module_config_register(STATUS, (1<<MAX_RT));	// Clear Interrupt Bit
		wl_module_config_register(STATUS, (1<<TX_DS));	//Clear Interrupt Bit
		PTX=0;
	}
	
	if (status & (1<<TX_FULL))							//TX_FIFO Full <-- this is not an IRQ
	{
		wl_module_CSN_lo;                               // Pull down chip select
		spi_fast_shift(FLUSH_TX);						// Flush TX-FIFO
		wl_module_CSN_hi;                               // Pull up chip select
	}
	
}
	//splitting double format in 4 uint8_t parts (start with array[3])
	//wiki ieee 754 18.4 = 01000001 10010011 00110011 00110011
	//                     65       147      51       51
	//                     a[3]     a[2]     a[1]     a[0]