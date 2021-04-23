//RECEPTOR
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>
#include <SPI.h>


#define TICK_PERIOD     10 //en ms.

//#define PIN_VENT0	8
#define PIN_VENT1   7
#define PIN_VENT2   6
#define PIN_VENT3   5
#define PIN_LUZ     4


//typedefs:
typedef enum{
    CMD_NULL = -1,
	VENT_0 = 0,	//velocidad del ventilador 0
    VENT_1, 	//velocidad del ventilador 0
    VENT_2, 	//velocidad del ventilador 0
	VENT_3, 	//velocidad del ventilador 0
	LUZ,		//Luz
	SIZEOF_CMD_ENUM,
}T_CMD_ENUM;


//datos de la radio:
const int pinCE = 9;
const int pinCSN = 10;
RF24 radio(pinCE, pinCSN);


//variables timer
uint8_t flag_tick = 0;
unsigned long last_tick = 0;


//variables cmd:
T_CMD_ENUM id_cmd = CMD_NULL;
T_CMD_ENUM sw_turnOFF = CMD_NULL;
T_CMD_ENUM sw_turnON = CMD_NULL;
uint8_t status_output[SIZEOF_CMD_ENUM];
uint8_t status_luz = 0;
uint8_t vent_pin[SIZEOF_CMD_ENUM - 1] = {PIN_VENT1, PIN_VENT2, PIN_VENT3}; // no hay pin para VENT_0
uint8_t flag_cmd = 0;
uint8_t toogle_timeout = 0; // 1 * 10ms



// Single radio pipe address for the 2 nodes to communicate.
const uint64_t pipe = 0xE8E8F0F0E1LL;

//char data[16];
char data;


void timer_update(void){
    
    unsigned long now = millis();
    
    if(now > last_tick + TICK_PERIOD){
        flag_tick = 1;
        last_tick = now;        
    }
}//end timer_update


void setup(void)
{
   Serial.begin(9600);
   radio.begin();
   radio.openReadingPipe(1,pipe);
   radio.startListening();
   
   //pinMode(PIN_VENT0, OUTPUT);
   pinMode(PIN_VENT1, OUTPUT);
   pinMode(PIN_VENT2, OUTPUT);
   pinMode(PIN_VENT3, OUTPUT);
   pinMode(PIN_LUZ, OUTPUT);
   
   digitalWrite(PIN_LUZ, 0);
   digitalWrite(PIN_VENT1, 0);
   digitalWrite(PIN_VENT2, 0);
   digitalWrite(PIN_VENT3, 0);
   
}
 
void loop(void)
{
	if (radio.available())
	{
		int done = radio.read(data, sizeof data); 
		//Serial.println(data);
		
		switch (data){
			case 'L':
				status_luz = !status_luz;
				digitalWrite(PIN_LUZ, status_luz);
			break;
			case 0xF0:
				
			break;
			case 0xF1:
				sw_turnON
			break;
			case 0xF2:
			
			break;
			case 0xF3:
			
			default:
			break;
			
		
		
	}
   
	timer_update();
   
	if (flag_tick){
		
		if (toogle_timeout){
			
		
		
		flag_tick = 0;
	}//end flag_tick
   
   
}