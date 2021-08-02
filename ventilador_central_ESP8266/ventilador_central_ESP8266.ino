#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <uMQTTBroker.h>
#include <MQTT.h>


#define MSG_BUFFER_SIZE 50
#define TICK_PERIOD     10 //en ms.
/*
#define RS 6 //CLK o SK
#define EN 7 //SD0 o S0
#define L_D4 11 //CMD o SC
#define L_D5 8 //SD1 Oo S1
#define L_D6 9 //SD2 o S2
#define L_D7 10 //SD3 o S3
*/

#define IN_VENT0	0 //D3
#define IN_VENT1   13 //D7
#define IN_VENT2   12 //D6
#define IN_VENT3   14 //D5
#define IN_LUZ     2 //D4

#define OUT_VENT1   15 //D8
#define OUT_VENT2   4 //D2
#define OUT_VENT3   5 //D1
#define OUT_LUZ     16 //D0



void callback_MQTT (String, char[]);


typedef enum{
    TRYING_WIFI,
    WIFI_TIMING_OUT,
    TRYING_MQTT,
    MQTT_TIMING_OUT,
    ALL_CONNECTED,
}T_CONN;

typedef enum{
    LOW_L,
    HIGH_L,
    FALL,
    RISE,
}T_INPUT;
    

/*
 * Custom broker class with overwritten callback functions
 */
class myMQTTBroker: public uMQTTBroker
{
	
	void (*MQTTCallBack) (String, char[]);
	
	
public:

	myMQTTBroker ( void (*pFunction) (String, char[]) ){
		
		MQTTCallBack = pFunction;
				
	}

    virtual bool onConnect(IPAddress addr, uint16_t client_count) {
      Serial.println(addr.toString()+" connected");
      return true;
    }
    
    virtual bool onAuth(String username, String password) {
      Serial.println("Username/Password: "+username+"/"+password);
      return true;
    }
    
    virtual void onData(String topic, const char *data, uint32_t length) {
		char data_str[length+1];
		os_memcpy(data_str, data, length);
		data_str[length] = '\0';
      
		Serial.println("received topic '"+topic+"' with data '"+(String)data_str+"'");
		
		//callback_MQTT(topic, data_str);
		MQTTCallBack(topic, data_str);
		
	}//fin onData
};



//variables de entradas
uint8_t flag_lecturas = 0;
const uint8_t pin_boton[5] = {IN_LUZ, IN_VENT0, IN_VENT1, IN_VENT2, IN_VENT3};
uint8_t read_boton[5] = {0, 0, 0, 0, 0}; //luz, vent0, vent1, vent2, vent3.
uint8_t last_boton[5] = {0, 0, 0, 0, 0}; //luz, vent0, vent1, vent2, vent3.
T_INPUT boton[5]; //luz, vent0, vent1, vent2, vent3.


//variables red
/*
char ssid[] = "ESP8266_CU";      // your network SSID (name)
char pass[] = "tclpqtp123456"; // your network password
*/
const char* ssid = "ESP8266_CU";
const char* password = "tclpqtp123456";

bool WiFiAP = true;      // Do yo want the ESP as AP? (useless)

myMQTTBroker myBroker(callback_MQTT);

char msg[MSG_BUFFER_SIZE];

//variables timer
uint8_t flag_tick = 0;
unsigned long last_tick = 0;


//variables conexion
T_CONN conn_status = TRYING_WIFI;
uint16_t reconnect_time = 0; // 1 * 10ms


//variables con topicos MQTT
char 	infoLuz[] = "Info/Nodo_ventilador/Luz", // payloads: 1 = prendida, 0 = apagada.
		infoVent[] = "Info/Nodo_ventilador/Vent",
		
		cmdLuz[] = "Cmd/Nodo_ventilador/Luz",
		cmdVent[] = "Cmd/Nodo_ventilador/Vent",
		cmdAsk[] = "Cmd/Nodo_ventilador/Ask"; // payloads: pregunta por: "S" = sensor, "L" = luz.

//variable de MQTT
uint8_t next_ask = 0;
uint8_t status_askMQTT = 1;

//variables de control
uint8_t luzStatus = 0;

/************************************
*
*		FUNCIONES
*
************************************/

void callback_MQTT (String topic, char data_str[]){
	
	String strComp = infoLuz;
	if (topic.equals(strComp)){//si recibe por MQTT infoLuz
		
		if (data_str[0]=='1'){
			digitalWrite(OUT_LUZ, 0);
			Serial.println("luz prendida");
		}else if (data_str[0]=='0'){
			digitalWrite(OUT_LUZ, 1);
			Serial.println("luz apagada");
		}
		
	} else {//else 1
		
		String strComp = infoVent;
		if (topic.equals(strComp)){//si recibe por MQTT info de la luz
			
			switch (data_str[0]){
				case 0:
					digitalWrite(OUT_VENT1, 1);
					digitalWrite(OUT_VENT2, 1);
					digitalWrite(OUT_VENT3, 1);	
					Serial.println("ventilador apagado");			
				break;
				case 1:
					digitalWrite(OUT_VENT1, 0);
					digitalWrite(OUT_VENT2, 1);
					digitalWrite(OUT_VENT3, 1);	
					Serial.println("ventilador en 1");			
				break;
				case 2:
					digitalWrite(OUT_VENT1, 0);
					digitalWrite(OUT_VENT2, 0);
					digitalWrite(OUT_VENT3, 1);
					Serial.println("ventilador en 2");
				break;
				case 3:
					digitalWrite(OUT_VENT1, 0);
					digitalWrite(OUT_VENT2, 0);
					digitalWrite(OUT_VENT3, 0);
					Serial.println("ventilador en 3");
				default:
				break;				
			} //fin switch
							
		}
		
	}//fin else 1
	
}



void connections_handler() {
    
    switch(conn_status){
        
        case TRYING_WIFI:
            // We start by connecting to a WiFi network
            Serial.println();
            Serial.print("Starting access point.");
            Serial.println(ssid);

            if (WiFi.softAP(ssid, password) != WL_CONNECTED){
                Serial.print(".");
                conn_status = WIFI_TIMING_OUT;
                reconnect_time = 500;
            }else{
                Serial.println("");
                Serial.println("Access point ready");
                Serial.println("IP address: " + WiFi.softAPIP().toString());
                reconnect_time = 0;
                conn_status = TRYING_MQTT;
            }
        break;
        
        case WIFI_TIMING_OUT:
            
            if (!WiFi.mode(WIFI_AP)){            
                if (!reconnect_time){
                    conn_status = TRYING_WIFI;
                }
            }else{
                Serial.println("");
                Serial.println("Access point ready");
                Serial.println("IP address: " + WiFi.softAPIP().toString());
                reconnect_time = 0;
                conn_status = TRYING_MQTT;
            }
                
        break;
        
        case TRYING_MQTT:
            
            if (!WiFi.mode(WIFI_AP)){
                Serial.println("Wifi AP lost. restarting...");
                conn_status = TRYING_WIFI;
                break;
            }
            
			// Starting the broker
			Serial.println("Starting MQTT broker");
			myBroker.init();
            
            //Subscribe to all topics
			myBroker.subscribe("#");
			
			conn_status = ALL_CONNECTED;

        break;
        
        case ALL_CONNECTED:
            if (!WiFi.mode(WIFI_AP)){
                Serial.println("Wifi AP lost. restarting...");
                conn_status = TRYING_WIFI;
                break;
            }
            
        break;
        
        default:
        break;        
        
    }//end switch
}//end connections_handler

void timer_update(void){
    
    unsigned long now = millis();
    
    if(now > last_tick + TICK_PERIOD){
        flag_tick = 1;
        last_tick = now;     
    }
}


void teclas(void){  
    for (uint8_t i = 0; i < 5; i++){
		if(!read_boton[i]){
			if(last_boton[i]){
				boton[i]= FALL;
				Serial.println("fall");
			}else{
				boton[i]= LOW_L;
			}
		}else{
			if(last_boton[i]){
				boton[i] = HIGH_L;
			}else{
				boton[i]= RISE;
			}
		}//end if	
		
		last_boton[i] = read_boton[i];
		
	}//end for
	
}//end teclas


void periodicAskMQTT(){
	
	if (!next_ask){
		Serial.println("preguntando...");
	/*	switch(status_askMQTT){
			case 1: //pregunta por el LDR
				myBroker.publish(cmdAsk, "L");
				next_ask = 200;
				status_askMQTT = 1;
			break;
			
			case 2: //pregunta por la tecla de la Luz.
				myBroker.publish(cmdAsk, "T");
				next_ask = 100;
				status_askMQTT = 3;
			break;
			
			case 3: //pregunta por el modo.
				myBroker.publish(cmdAsk, "M");
				next_ask = 100;
				status_askMQTT = 1;
			break;			
			case 0:
			default:
			break;
		}//fin switch*/
	} else {
		next_ask--;
	}

}//fin periodicAskMQTT




void setup() {

	pinMode (IN_VENT0, INPUT_PULLUP);
	pinMode (IN_VENT1, INPUT_PULLUP);
	pinMode (IN_VENT2, INPUT_PULLUP);
	pinMode (IN_VENT3, INPUT_PULLUP);
	pinMode (IN_LUZ, INPUT_PULLUP);
	
	pinMode(OUT_LUZ, OUTPUT);
    pinMode(OUT_VENT1, OUTPUT);
    pinMode(OUT_VENT2, OUTPUT);
    pinMode(OUT_VENT3, OUTPUT);
    
    digitalWrite(OUT_LUZ, 1);
    digitalWrite(OUT_VENT1, 1);
    digitalWrite(OUT_VENT2, 1);
    digitalWrite(OUT_VENT3, 1);    
    

	Serial.begin(115200);
	Serial.println();
	Serial.println();
	
	connections_handler();
	
}

void loop() {
	
	timer_update();	

	if (flag_tick){
		
				
		if (reconnect_time) reconnect_time--;
		
		
		if (conn_status == ALL_CONNECTED){
			//periodicAskMQTT();
		}
		
        if (!flag_lecturas){
            for (uint8_t i = 0; i < 5; i++){
				read_boton[i] = digitalRead(pin_boton[i]);
				flag_lecturas = 5;
			} //end for
        }else{
            flag_lecturas--;
        }	
		
		flag_tick = 0;
		
	} //end if flag_tick
	
    connections_handler();
    
    teclas();
    
  if (boton[0] == FALL){
		if (!luzStatus){
			myBroker.publish(cmdLuz, "1");
			luzStatus = 1;
		}else{
			myBroker.publish(cmdLuz, "0");
			luzStatus = 0;
		}
	}
	
	if (boton[1] == FALL){
		myBroker.publish(cmdVent, "0");
		digitalWrite(OUT_VENT1, 1);
		digitalWrite(OUT_VENT2, 1);
		digitalWrite(OUT_VENT3, 1);
	}
	
	if (boton[2] == FALL){
		myBroker.publish(cmdVent, "1");
		digitalWrite(OUT_VENT1, 0);
		digitalWrite(OUT_VENT2, 1);
		digitalWrite(OUT_VENT3, 1);
	}	
	
	if (boton[3] == FALL){
		myBroker.publish(cmdVent, "2");
		digitalWrite(OUT_VENT1, 0);
		digitalWrite(OUT_VENT2, 0);
		digitalWrite(OUT_VENT3, 1);
	}	
	
	
	if (boton[4] == FALL){
		myBroker.publish(cmdVent, "3");
		digitalWrite(OUT_VENT1, 0);
		digitalWrite(OUT_VENT2, 0);
		digitalWrite(OUT_VENT3, 0);
	}	
    
	
	if (Serial.available()) {      // If anything comes in Serial (USB),
		
	} //fin if serial
	
    //Serial.write(Serial1.read());   // read it and send it out Serial (USB)
  
}
