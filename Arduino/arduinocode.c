//libraries used for nRF24L01+
#include <SPI.h> 
#include <nRF24L01.h>
#include <RF24.h>
//AES Encryption
#include <AESLib.h>

//CE, CSN (pin 8, pin 9)
RF24 radio(8, 9);

//specified receiver address equals receive address data pipe 0
//has to match with transmitter address for communication!
const uint64_t address = 0xE7E7E7E7E7LL;

//16 byte payload array							
uint8_t payload[16]={0,0,0,0,
                     0,0,0,0,
                     0,0,0,0,
                     0,0,0,0};	
					 
//16 byte aes encryption key					 
uint8_t key[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15};

//indicates if interrupt occurred
bool flag=0;

void setup() 
{
	//set UART baudrate (9.600 bit/s)
	Serial.begin(9600);
	//open connection to nRF24L01+
	radio.begin();
	//setup receiving pipe and address
	radio.openReadingPipe(0, address);
	//setup RF channel frequency (2402 MHz)
	radio.setChannel(2);
	//setup payload size (16 byte)
	radio.setPayloadSize(16);
	//setup RF data rate (250kbps)
	radio.setDataRate(RF24_250KBPS);
	//setup IRQ(tx, rt, rx)
	radio.maskIRQ(1,1,0);
	//setup CRC length (16 bit/2 byte)
	radio.setCRCLength(RF24_CRC_16);
	//you can set this as minimum or maximum depending
	//on the distance between the transmitter and receiver
	radio.setPALevel(RF24_PA_MIN);
	//this sets the module as receiver
	radio.startListening();
	//Create interrupt: 0 for pin 2 or 1 for pin 3, the name of the interrupt function or ISR, and condition to trigger interrupt
	attachInterrupt(0, interruptFunction, FALLING);  
	Serial.println("started");
}

void loop()
{
	if(flag>0)
	{
		radio.available();
		radio.read(&payload, sizeof(payload)); //Reading the data
		Serial.println("receive");
		aes128_dec_single(key, payload);
		for(int k=0; k<16; ++k)
		{
			Serial.println(payload[k]);
		}
		flag=0;
	}
}

//This is the function called when the interrupt occurs (pin 2 goes high)
//this is often referred to as the interrupt service routine or ISR
//This cannot take any input arguments or return anything
void interruptFunction() 
{
  flag=1;
}