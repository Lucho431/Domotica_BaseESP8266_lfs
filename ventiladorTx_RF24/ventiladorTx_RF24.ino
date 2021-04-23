//EMISOR
#include <nRF24L01.h>
#include <RF24.h>
#include <RF24_config.h>
#include <SPI.h>


#define TICK_PERIOD     10 //en ms.

#define PIN_VENT0	8
#define PIN_VENT1   7
#define PIN_VENT2   6
#define PIN_VENT3   5
#define PIN_LUZ     4


//typedefs:
typedef enum{
    LOW_L,
    HIGH_L,
    FALL,
    RISE,
}T_INPUT;

typedef enum{
    VENT_0 = 0,	//velocidad del ventilador 0
    VENT_1, 	//velocidad del ventilador 0
    VENT_2, 	//velocidad del ventilador 0
	VENT_3, 	//velocidad del ventilador 0
	LUZ,		//Luz
	SIZEOF_BOTON_ENUM,
}T_BOTON_ENUM;


//datos de la radio:
const int pinCE = 9;
const int pinCSN = 10;
RF24 radio(pinCE, pinCSN);


//variables de entradas:
T_BOTON_ENUM index_boton = VENT_0;

uint8_t read_boton[SIZEOF_BOTON_ENUM];
uint8_t last_boton[SIZEOF_BOTON_ENUM];

T_INPUT status_boton[SIZEOF_BOTON_ENUM];


//variables timer
uint8_t flag_tick = 0;
unsigned long last_tick = 0;


// Single radio pipe address for the 2 nodes to communicate.
const uint64_t pipe = 0xE8E8F0F0E1LL;
 
//char data[16]="Hola mundo" ;
char data;

void teclas(void){
        
    for(T_BOTON_ENUM i = VENT_0; i < SIZEOF_BOTON_ENUM; i++){
	
		if(!read_boton[i]){
			if(last_boton[i]){
				status_boton[i] = FALL;
			}else{
				boton_manAuto = LOW_L;
			}
		}else{
			if(last_boton_manAuto){
				boton_manAuto = HIGH_L;
			}else{
				boton_manAuto = RISE;
			}
		} //end if read_boton
		
		last_boton[i] = read_boton[i];
	} //end for    
}//end teclas

void timer_update(void){
    
    unsigned long now = millis();
    
    if(now > last_tick + TICK_PERIOD){
        flag_tick = 1;
        last_tick = now;        
    }
}//end timer_update

void cmd_handler (void){
	
	if(status_boton[LUZ] == FALL){
		data = 'L';
		radio.write(data, sizeof data);
	}
	
	if (status_boton[VENT_0] == FALL){
		data = 0xF0;
		radio.write(data, sizeof data);
	}
	
	if (status_boton[VENT_1] == FALL){
		data = 0xF1;
		radio.write(data, sizeof data);
	}
	
	if (status_boton[VENT_2] == FALL){
		data = 0xF2;
		radio.write(data, sizeof data);
	}
	
	if (status_boton[VENT_3] == FALL){
		data = 0xF3;
		radio.write(data, sizeof data);
	}
}//end cmd_handler
	


void setup(void)
{
   radio.begin();
   radio.openWritingPipe(pipe);
   
   pinMode(PIN_VENT0, INPUT_PULLUP);
   pinMode(PIN_VENT1, INPUT_PULLUP);
   pinMode(PIN_VENT2, INPUT_PULLUP);
   pinMode(PIN_VENT3, INPUT_PULLUP);
   pinMode(PIN_LUZ, INPUT_PULLUP);
}
 
void loop(void)
{
	timer_update();
	
	if (flag_tick){
		
		if (!flag_lecturas){
            read_boton[VENT_0] = digitalRead(PIN_VENT0);
			read_boton[VENT_1] = digitalRead(PIN_VENT1);
			read_boton[VENT_2] = digitalRead(PIN_VENT2);
			read_boton[VENT_3] = digitalRead(PIN_VENT3);
			read_boton[LUZ] = digitalRead(PIN_LUZ);
            flag_lecturas = 1;
        }else{
            flag_lecturas=0;
        }//end if flag_lecturas
		
		flag_tick = 0;
	}//end if flag_tick
	
	teclas();
	
   //radio.write(data, sizeof data);
   //delay(1000);
}