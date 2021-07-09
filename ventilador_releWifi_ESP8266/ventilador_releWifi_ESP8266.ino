
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

#define MSG_BUFFER_SIZE 50

#define TICK_PERIOD     10 //en ms.

#define PIN_LUZ     D7
#define PIN_VENT1   D5
#define PIN_VENT2    D6
#define PIN_VENT3   D8
//#define PIN_LDR     A0


typedef enum{
    TRYING_WIFI,
    WIFI_TIMING_OUT,
    TRYING_MQTT,
    MQTT_TIMING_OUT,
    ALL_CONNECTED,
}T_CONN;


//variables red

const char* ssid = "ESP8266_CU";
const char* password = "tclpqtp123456";
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



//variables cmd:
uint8_t status_luz = 0;
uint8_t turnON_delay = 0;
uint8_t turnON_pin = 0;


//variables con topicos MQTT
char 	infoLuz[] = "Info/Nodo_ventilador/Luz", // payloads: 1 = prendida, 0 = apagada.
		infoVent[] = "Info/Nodo_ventilador/Vent",
		
		cmdLuz[] = "Cmd/Nodo_ventilador/Luz",
		cmdVent[] = "Cmd/Nodo_ventilador/Vent", // payloads: pregunta por: "S" = sensor, "L" = luz.




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
		if ((char)payload[0] == '1'){
			digitalWrite(PIN_LUZ, 1);
			client.publish(infoLuz,"1");
		}else if ((char)payload[0] == '0'){
			digitalWrite(PIN_LUZ, 0);
			client.publish(infoLuz,"0");
		}	
	} else{//else 1
		
		String strComp = cmdVent;		
		if (strTopic.equals(strComp)){
			switch((char)payload[0]){
				case '0':
					digitalWrite(PIN_VENT1, 0);
					digitalWrite(PIN_VENT2, 0);
					digitalWrite(PIN_VENT3, 0);
				break;
				case '1':
					digitalWrite(PIN_VENT2, 0);
					digitalWrite(PIN_VENT3, 0);
					
					turnON_delay = 10; // 100 ms
					turnON_pin = PIN_VENT1;
				break;
				case '2':
					digitalWrite(PIN_VENT1, 0);
					digitalWrite(PIN_VENT3, 0);
					
					turnON_delay = 10; // 100 ms
					turnON_pin = PIN_VENT2;
				break;
				case '3':
					digitalWrite(PIN_VENT1, 0);
					digitalWrite(PIN_VENT2, 0);
					
					turnON_delay = 10; // 100 ms
					turnON_pin = PIN_VENT3;
				default:
				break;
			} //fin switch		
		} //fin if 
			
	} //fin else 1

} //fin callback




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
            //clientId = "Nodo_ventilador";
            clientId += String(random(0xffff), HEX); //must be random because of the uMQTTbroker bug
            
            // Attempt to connect
            if (client.connect(clientId.c_str())) {
                Serial.println("connected");
                // Once connected, publish an announcement...
                client.publish("Hola", "Nodo_ventilador");
                // ... and resubscribe
                client.subscribe("Cmd/Nodo_ventilador/#");
                
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



void setup() {
    
    pinMode(BUILTIN_LED, OUTPUT);
    pinMode(PIN_LUZ, OUTPUT);
    pinMode(PIN_VENT1, OUTPUT);
    pinMode(PIN_VENT2, OUTPUT);
    pinMode(PIN_VENT3, OUTPUT);
    
    digitalWrite(PIN_VENT1, 0);
    digitalWrite(PIN_VENT2, 0);
    digitalWrite(PIN_VENT3, 0);
    

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
        
		if (turnON_pin != 0){
			if (turnON_delay){
				turnON_delay--;
			}else{
				digitalWrite(turnON_pin, 1);
				turnON_pin = 0;
			}//end if turnON_delay
		}//end if turnON_pin
        
        flag_tick = 0;
    }//end if flag_tick
    
    connections_handler();

}//end loop
