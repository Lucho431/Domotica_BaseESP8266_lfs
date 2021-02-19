
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define MSG_BUFFER_SIZE 50

#define TICK_PERIOD     10 //en ms.

#define PIN_LED     D7
#define PIN_TECLA_LUZ   D5
#define PIN_AUTO_MAN    D6
#define PIN_RELE    D8
#define PIN_LDR     A0


typedef enum{
    TRYING_WIFI,
    WIFI_TIMING_OUT,
    TRYING_MQTT,
    MQTT_TIMING_OUT,
    ALL_CONNECTED,
}T_CONN;

typedef enum{
    MANUAL = 0,
    AUTO,
}T_MODE;

typedef enum{
    LOW_L,
    HIGH_L,
    FALL,
    RISE,
}T_INPUT;
    


//variables red
/*
const char* ssid = "Arias2547";
const char* password = "1142416109";
const char* mqtt_server = "192.168.100.24";
*/
const char* ssid = "ESP8266_CU";
const char* password = "RJQ-729!!";
const char* mqtt_server = "192.168.4.1";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;

String clientId;
char msg[MSG_BUFFER_SIZE];
int value = 0;


//variables timer
uint8_t flag_tick = 0;
unsigned long last_tick = 0;

//variables conexion
T_CONN conn_status = TRYING_WIFI;
uint16_t reconnect_time = 0; // 1 * 10ms


//variables LDR
const long LDRmax = 1000;     //Resistencia en oscuridad en KΩ
const int LDRmin = 15;        //Resistencia a la luz (10 Lux) en KΩ
const int Rc = 10;       //Resistencia calibracion en KΩ

int LDR_val; //valor de lectura del LDR
int LDR_H = 768; //umbral de valor alto
int LDR_L = 356; //umbral de valor bajo
int ilum; //iluminacion sensada


//variables de funcionamiento
T_MODE op_mode = MANUAL;
volatile uint8_t luz_auto_on = 0;


//variables de entradas
uint8_t flag_lecturas = 0;
uint8_t read_teclaLuz = 0;
uint8_t read_boton_manAuto = 0;
uint8_t last_teclaLuz = 0;
uint8_t last_boton_manAuto = 0;
T_INPUT teclaLuz;
T_INPUT boton_manAuto;


//variables con topicos MQTT
char 	infoLuz[] = "Info/Nodo_luzAfuera/Luz", // payloads: 1 = prendida, 0 = apagada.
		infoLDR_H[] = "Info/Nodo_luzAfuera/LDR_H",
		infoLDR_L[] = "Info/Nodo_luzAfuera/LDR_L",
		infoLDR_val[] = "Info/Nodo_luzAfuera/LDR_val",
		infoModo[] = "Info/Nodo_luzAfuera/Modo", //payloads: 0 = MANUAL; 1 = AUTO.
		
		cmdLuz[] = "Cmd/Nodo_luzAfuera/Luz",
		cmdLRD_H[] = "Cmd/Nodo_luzAfuera/LDR_H",
		cmdLRD_L[] = "Cmd/Nodo_luzAfuera/LDR_L",
		cmdAsk[] = "Cmd/Nodo_luzAfuera/Ask", // payloads: pregunta por: "S" = sensor, "L" = luz.
		cmdModo[] = "Info/Nodo_luzAfuera/Modo";	




void LDR_read (void){
    LDR_val = analogRead(PIN_LDR);         
 
    //ilum = ((long)(1024-V)*LDRmax*10)/((long)LDRmin*Rc*LDR_val);  //usar si LDR entre GND y A0 
    ilum = ((long)LDR_val*LDRmax*10)/((long)LDRmin*Rc*(1024-LDR_val));    //usar si LDR entre A0 y Vcc (como en el esquema anterior)
    
    //Serial.println(String(LDR_val, DEC));
}




void timer_update(void){
    
    unsigned long now = millis();
    
    if(now > last_tick + TICK_PERIOD){
        flag_tick = 1;
        last_tick = now;        
    }
}
        
    



void callback(char* topic, byte* payload, unsigned int length) {
	Serial.print("Message arrived [");
	Serial.print(topic);
	Serial.print("] ");
	for (int i = 0; i < length; i++) {
		Serial.print((char)payload[i]);
	}
	Serial.println();
	
	
	
	String strTopic = topic;
	String strComp = cmdLuz;
	
	if (strTopic.equals(strComp)){//si recibe por MQTT el comando de la luz
		if (op_mode == MANUAL){//si está en MANUAL, entonces opero...
			if ((char)payload[0] == '1'){
				digitalWrite(PIN_RELE, 1);
                client.publish(infoLuz,"1");
			}else if ((char)payload[0] == '0'){
				digitalWrite(PIN_RELE, 0);
                client.publish(infoLuz,"0");
			}
		}	
	} else{//else 1
		
		String strComp = cmdModo;		
		if (strTopic.equals(strComp)){
			if ((char)payload[0] == '0'){
				op_mode = MANUAL;//pongo op_mode en manual
                client.publish(infoModo,"0");
			}else if ((char)payload[0] == '1'){
				op_mode = AUTO;//pongo op_mode en auto
                client.publish(infoModo,"1");
			}
		} else {//else 2
		
			String strComp = cmdLRD_H;		
			if (strTopic.equals(strComp)){
				
				LDR_H = atoi((char*)payload);
			
			} else { //else 3
				
				String strComp = cmdLRD_L;
				if (strTopic.equals(strComp)){
				
				LDR_L = atoi((char*)payload);
				
				} else {//else 4
					
					String strComp = cmdAsk;
					if (strTopic.equals(strComp)){
						
						switch(payload[0]){
								case 'L':
									sprintf(msg, "%d", LDR_val);
									client.publish(infoLDR_val, msg);									
								break;
								
								case 'T':
								default:
								break;
						}//fin switch
						
					}//fin if cmdAsk
				}//fin else 4
			} //fin else 3
		} //fin else 2
	} //fin else 1
	
	
  // Switch on the LED if an 1 was received as first character
  /*if ((char)payload[0] == '1') {
    digitalWrite(BUILTIN_LED, LOW_L);   // Turn the LED on (Note that LOW_L is the voltage level
    // but actually the LED is on; this is because
    // it is active low on the ESP-01)
  } else {
    digitalWrite(BUILTIN_LED, HIGH_L);  // Turn the LED off by making the voltage HIGH_L
  }*/

}//fin callback




void connections_handler() {
    
    switch(conn_status){
        
        case TRYING_WIFI:
            // We start by connecting to a WiFi network
            Serial.println();
            Serial.print("Connecting to ");
            Serial.println(ssid);

            WiFi.mode(WIFI_STA);
            if (WiFi.begin(ssid, password) != WL_CONNECTED){
                Serial.print(".");
                conn_status = WIFI_TIMING_OUT;
                reconnect_time = 500;
            }else{
                Serial.println("");
                Serial.println("WiFi connected");
                Serial.println("IP address: ");
                Serial.println(WiFi.localIP());
                reconnect_time = 0;
                conn_status = TRYING_MQTT;
            }
        break;
        
        case WIFI_TIMING_OUT:
            
            if (WiFi.status() != WL_CONNECTED){            
                if (!reconnect_time){
                    conn_status = TRYING_WIFI;
                }
            }else{
                Serial.println("");
                Serial.println("WiFi connected");
                Serial.println("IP address: ");
                Serial.println(WiFi.localIP());
                reconnect_time = 0;
                conn_status = TRYING_MQTT;
            }
                
        break;
        
        case TRYING_MQTT:
            
            if (WiFi.status() != WL_CONNECTED){
                Serial.println("Wifi connection lost. restarting...");
                conn_status = TRYING_WIFI;
                break;
            }
            
            
            Serial.print("Attempting MQTT connection...");
            // Create a random client ID
            //clientId = "Nodo_luzAfuera";
            clientId += String(random(0xffff), HEX); //must be random because of the uMQTTbroker bug
            
            // Attempt to connect
            if (client.connect(clientId.c_str())) {
                Serial.println("connected");
                // Once connected, publish an announcement...
                client.publish("Hola", "Nodo_luzAfuera");
                // ... and resubscribe
                client.subscribe("Cmd/Nodo_luzAfuera/#");
                
                conn_status = ALL_CONNECTED;
                
            } else {
                Serial.print("failed, rc=");
                Serial.print(client.state());
                Serial.println(" try again in 5 seconds");
                // Wait 5 seconds before retrying
                
                conn_status = MQTT_TIMING_OUT;
                reconnect_time = 500;
            }
        break;
        
        case MQTT_TIMING_OUT:
            
            if (WiFi.status() != WL_CONNECTED){
                Serial.println("Wifi connection lost. restarting...");
                conn_status = TRYING_WIFI;
                break;
            }
            
            if (!reconnect_time){
                conn_status =TRYING_MQTT;
            }
        break;
        
        case ALL_CONNECTED:
            if (WiFi.status() != WL_CONNECTED){
                Serial.println("Wifi connection lost. restarting...");
                conn_status = TRYING_WIFI;
                break;
            }
            
            if (client.connected()){
                client.loop();
            }else{
                conn_status = TRYING_MQTT;
            }
        break;        
        
    }//end switch
}//end connections_handler




void luz_handler(void){
    
    switch (op_mode){
        case AUTO:
            digitalWrite(PIN_LED, 0);
            //digitalWrite(BUILTIN_LED, 0);
            
            if (boton_manAuto == FALL){
                op_mode = MANUAL;
                client.publish(infoModo,"0");
                if (teclaLuz == LOW_L || teclaLuz == FALL){
                    digitalWrite(PIN_RELE, 1);
                    client.publish(infoLuz,"1");
                }else{
                    digitalWrite(PIN_RELE, 0);
                    client.publish(infoLuz,"0");
                }
            }//end if boton_manAuto
            
            if (luz_auto_on){
                if (LDR_val > LDR_H){
					//digitalWrite(BUILTIN_LED, 1);
                    digitalWrite(PIN_RELE, 0);
                    client.publish(infoLuz,"0");
                    luz_auto_on = 0;
                }
            }else{
                if (LDR_val < LDR_L){
                    //digitalWrite(BUILTIN_LED, 0);
                    digitalWrite(PIN_RELE, 1);
                    client.publish(infoLuz,"1");
                    luz_auto_on = 1;
                }
            }//end if luz_auto_on
                    
            
        break;
        
        case MANUAL:
            
            digitalWrite(PIN_LED, 1);
            //digitalWrite(BUILTIN_LED, 1);
            
            if (boton_manAuto == FALL){
                op_mode = AUTO;
                client.publish(infoModo,"1");
                break;
            }
            
            if(teclaLuz == FALL){
                digitalWrite(PIN_RELE, 1);
                client.publish(infoLuz,"1");
            }else if (teclaLuz == RISE){
                digitalWrite(PIN_RELE, 0);
                client.publish(infoLuz,"0");
            }            
        break;
            
    }//end swith op_mode
}//end luz_handler





void teclas(void){
        
    if(!read_boton_manAuto){
        if(last_boton_manAuto){
            boton_manAuto = FALL;
        }else{
            boton_manAuto = LOW_L;
        }
    }else{
        if(last_boton_manAuto){
            boton_manAuto = HIGH_L;
        }else{
            boton_manAuto = RISE;
        }
    }
    
    if(!read_teclaLuz){
        if(last_teclaLuz){
            teclaLuz = FALL;
        }else{
            teclaLuz = LOW_L;
        }
    }else{
        if(last_teclaLuz){
            teclaLuz = HIGH_L;
        }else{
            teclaLuz = RISE;
        }
    }
    
    last_boton_manAuto = read_boton_manAuto;
    last_teclaLuz = read_teclaLuz;
    
}//end teclas




void setup() {
    
    pinMode(BUILTIN_LED, OUTPUT);
    pinMode(PIN_LED, OUTPUT);
    pinMode(PIN_RELE, OUTPUT);
    
    pinMode(PIN_TECLA_LUZ, INPUT_PULLUP);
    pinMode(PIN_AUTO_MAN, INPUT_PULLUP);
    
    digitalWrite(PIN_RELE, 0);
    

    Serial.begin(115200);
    randomSeed(micros());
    
    client.setServer(mqtt_server, 1883);
    client.setCallback(callback);
    
    connections_handler();
}

void loop() {
    
    timer_update();
    
    if (flag_tick){
        
        if (reconnect_time) reconnect_time--;
        
        
        if (conn_status == ALL_CONNECTED){
            /*
            lastMsg++;
            if (lastMsg>200){
                ++value;
                snprintf (msg, MSG_BUFFER_SIZE, "hello world #%ld", value);
                Serial.print("Publish message: ");
                Serial.println(msg);
                client.publish("Hola", msg);
                lastMsg = 0;
            }*/
        }//end if conn_status
        
        if (!flag_lecturas){
            read_teclaLuz = digitalRead(PIN_TECLA_LUZ);
            read_boton_manAuto = digitalRead(PIN_AUTO_MAN);
            LDR_read();
            flag_lecturas = 1;
        }else{
            flag_lecturas=0;
        }
        
        flag_tick = 0;
    }//end if flag_tick
    
    connections_handler();
    
    teclas();
    
    luz_handler();

}//end loop
