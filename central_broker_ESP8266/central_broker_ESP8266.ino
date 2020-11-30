/*
 * uMQTTBroker demo for Arduino (C++-style)
 * 
 * The program defines a custom broker class with callbacks, 
 * starts it, subscribes locally to anything, and publishs a topic every second.
 * Try to connect from a remote client and publish something - the console will show this as well.
 */

#include <ESP8266WiFi.h>
#include "uMQTTBroker.h"

#define TICK_PERIOD     10 //en ms.


typedef enum{
    TRYING_WIFI,
    WIFI_TIMING_OUT,
    TRYING_MQTT,
    MQTT_TIMING_OUT,
    ALL_CONNECTED,
}T_CONN;

typedef enum{
    AUTO,
    MANUAL,
}T_MODE;

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
public:
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
    }
};



//variables red
char ssid[] = "ESP8266_CU";      // your network SSID (name)
char pass[] = "RJQ-729!!"; // your network password
bool WiFiAP = true;      // Do yo want the ESP as AP? (useless)


myMQTTBroker myBroker;


//variables timer
uint8_t flag_tick = 0;
unsigned long last_tick = 0;

//variables conexion
T_CONN conn_status = TRYING_WIFI;
uint16_t reconnect_time = 0; // 1 * 10ms



void connections_handler() {
    
    switch(conn_status){
        
        case TRYING_WIFI:
            // We start by connecting to a WiFi network
            Serial.println();
            Serial.print("Starting access point.");
            Serial.println(ssid);

            WiFi.mode(WIFI_AP);
            if (!WiFi.mode(WIFI_AP)){
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
  

void setup()
{
	Serial.begin(115200);
	Serial.println();
	Serial.println();

	connections_handler();
}

void loop()
{
	timer_update();
	
	if (flag_tick){
				
		if (reconnect_time) reconnect_time--;
		
		
		
		flag_tick = 0;
	}//end if flag_tick
	
    connections_handler();
	
	//Publish the counter value as String
	//myBroker.publish("broker/counter", (String)counter++);

}//end loop

