#include <ESP8266WiFi.h>
#include "uMQTTBroker.h"
#include <LiquidCrystal.h>

#define TICK_PERIOD     10 //en ms.
#define RS 6 //CLK o SK
#define EN 7 //SD0 o S0
#define L_D4 11 //CMD o SC
#define L_D5 8 //SD1 Oo S1
#define L_D6 9 //SD2 o S2
#define L_D7 10 //SD3 o S3
#define PIN_UP 14 //D5
#define PIN_DOWN 12 //D6
#define PIN_LEFT 13 //D7
#define PIN_RIGHT 15 //D8


void cmd_modo (void);
void cmd_ldr (void);
void cmd_contraste (void);
void cmd_salvaPantalla (void);
void cmd_tecla (void);



typedef struct t_menu_general {
	String nombre;
	struct t_menu_general* menuPadre;
	struct t_menu_general* firstSubMenu;
	uint8_t totalSubMenus;
	uint8_t seleccion;
	void (*accion) (void);
}T_MENU_GENERAL;

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
    
typedef enum{
	
	NAVEGANDO = 0,
	SETEANDO_CMD,
	SALVA_PANTALLA,
} T_ESTATUS_PANTALLA;
	

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



T_MENU_GENERAL listaMenus[20]= {
									{"      MENU      ", NULL, &listaMenus[1], 2, 0, NULL},  //0
									
									{"   LUZ AFUERA   ", &listaMenus[0], &listaMenus[3], 3, 0, NULL},  //1 
									{" CONFIGURACION  ", &listaMenus[0], &listaMenus[6], 2, 0, NULL},  //2
									
									{"      MODO      ", &listaMenus[1], NULL, 0, 0, cmd_modo},  //3
									{"      LDR       ", &listaMenus[1], NULL, 0, 0, cmd_ldr},  //4
									{"     TECLA      ", &listaMenus[1], NULL, 0, 0, cmd_tecla},  //5
									
									{"   CONTRASTE    ", &listaMenus[2], NULL, 0, 0, cmd_contraste},  //6
									{"SALVA PANTALLAS ", &listaMenus[2], NULL, 0, 0, cmd_salvaPantalla},  //7
};


T_ESTATUS_PANTALLA status_pantalla = NAVEGANDO;	
int8_t opcion = 0;
T_MENU_GENERAL* menuActual;
uint8_t flag_imprimir = 0;


//variables de pantalla
String renglon1, renglon2;

LiquidCrystal lcd(RS,EN,L_D4,L_D5,L_D6,L_D7);


//variables de entradas
uint8_t flag_lecturas = 0;
const uint8_t pin_boton[4] = {PIN_UP, PIN_DOWN, PIN_LEFT, PIN_RIGHT};
uint8_t read_boton[4] = {0, 0, 0, 0}; //up, down, left, right
uint8_t last_boton[4] = {0, 0, 0, 0}; //up, down, left, right
T_INPUT boton[4]; //up, down, left, right


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


//variables de la luz de afuera:
uint8_t mode = 0;
uint8_t mode_selected = 0;
uint8_t teclaLuz = 0;
uint8_t teclaLuz_selected = 0;
int LDR_actual = 512;


/************************************
*
*		FUNCIONES
*
************************************/

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

void imprime_pantalla (void){
	
	lcd.clear();
	lcd.setCursor(0,0);
	lcd.print(renglon1);
	lcd.setCursor(0,1);
	lcd.print(renglon2);
}//fin imprime_pantalla

void teclas(void){
        
    for (uint8_t i = 0; i++; i < 4){
		if(!read_boton[i]){
			if(last_boton[i]){
				boton[i]= FALL;
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

void navega_menu (void){
	
	T_MENU_GENERAL* aux;
	
	char cmd = (char) Serial.read();
		
	switch(cmd){
		
		case '8':
			//Serial.print("arriba\n");	
			if (menuActual->menuPadre != NULL){ 
				
				menuActual = menuActual->menuPadre;
				opcion = menuActual->seleccion;
				
				
				renglon1 = menuActual->nombre;
				aux = menuActual->firstSubMenu + menuActual->seleccion;
				renglon2 = aux->nombre;
				flag_imprimir = 1;
			}
		
		break;
		
		case '2':
			//Serial.print("abajo\n");
			
			menuActual = menuActual->firstSubMenu + menuActual->seleccion;
			opcion = menuActual->seleccion;
			
			if (menuActual->firstSubMenu != NULL){ 
							
				renglon1 = menuActual->nombre;
				aux = menuActual->firstSubMenu + menuActual->seleccion;
				renglon2 = aux->nombre;
				flag_imprimir = 1;
				
			}else if (menuActual->accion != NULL){
				status_pantalla = SETEANDO_CMD;
				menuActual->accion();
				flag_imprimir = 1;
			}	
		
		break;
		
		case '4':
			//Serial.print("izquierda\n");
			opcion--;
			if (opcion < 0) opcion = menuActual->totalSubMenus - 1;
			menuActual->seleccion = opcion;
			
			renglon1 = menuActual->nombre;
			aux = menuActual->firstSubMenu + menuActual->seleccion;
			renglon2 = aux->nombre;
			flag_imprimir = 1;			
		break;
		
		case '6':
			//Serial.print("derecha\n");
			opcion++;
			if (opcion >= menuActual->totalSubMenus) opcion = 0;
			menuActual->seleccion = opcion;
			
			renglon1 = menuActual->nombre;
			aux = menuActual->firstSubMenu + menuActual->seleccion;
			renglon2 = aux->nombre;
			flag_imprimir = 1;			
		break;
		
	}//end switch
	
}//fin navega_menu

void cmd_modo (void){

	//static uint8_t mode = 0;
	//static uint8_t mode_selected = 0;

	char cmd = (char) Serial.read();
		
	renglon1 = "      MODO      ";
	
	switch(cmd){
		default:
			if (mode){				
				if (mode_selected == mode){
					renglon2 = "   AUTOMATICO X ";
				}else{
					renglon2 = "   AUTOMATICO   ";
				}				
			}else{
				if (mode_selected == mode){
					renglon2 = "     MANUAL  X  ";
				}else{
					renglon2 = "     MANUAL     ";
				}
			}
			
			flag_imprimir = 1;
		break;
		
		case '4':
		case '6':
			mode = ~mode;
			
			if (mode){				
				if (mode_selected == mode){
					renglon2 = "   AUTOMATICO X ";
				}else{
					renglon2 = "   AUTOMATICO   ";
				}				
			}else{
				if (mode_selected == mode){
					renglon2 = "     MANUAL  X  ";
				}else{
					renglon2 = "     MANUAL     ";
				}
			}
			
			flag_imprimir = 1;
		break;
		
		case '2':
			if (mode){
				renglon2 = "   AUTOMATICO X ";
				//mando el MQTTcorrespondiente
			}else{
				renglon2 = "     MANUAL  X  ";
				//mando el otro MQTT correspondiente.
			}
			flag_imprimir = 1;
			
			mode_selected = mode;
		break;
		
		case '8':
			//Serial.print("arriba\n");	
			status_pantalla = NAVEGANDO;			
			
			renglon2 = menuActual->nombre;
			menuActual = menuActual->menuPadre;	
			renglon1 = menuActual->nombre;
			opcion = menuActual->seleccion;
			flag_imprimir = 1;
		
		break;
	}
	
}//fin cmd_modo

void cmd_ldr (void){

	renglon1 = "luz actual: " + String(LDR_actual, DEC);
	
	char cmd = (char) Serial.read();
		
	switch(cmd){
		default:
			renglon2 = "Valor: nada";
		break;
		
		case '4':
			flag_imprimir = 1;
			renglon2 = "izquierda";
		break;
		
		case '2':
			flag_imprimir = 1;
			renglon2 = "abajo";
		break;
		
		case '6':
			flag_imprimir = 1;
			renglon2 = "derecha";
		break;
		
		case '8':
			//Serial.print("arriba\n");	
			status_pantalla = NAVEGANDO;			
			
			renglon2 = menuActual->nombre;
			menuActual = menuActual->menuPadre;	
			renglon1 = menuActual->nombre;
			opcion = menuActual->seleccion;
			flag_imprimir = 1;
		
		break;
	}
	
}//fin cmd_ldr

void cmd_tecla (void){
	
	renglon1 = "TECLA DE LA LUZ ";
	
	char cmd = (char) Serial.read();
		
	switch(cmd){
		default:
			if (teclaLuz){				
				renglon2 = "       ON       ";
			}else{
				renglon2 = "       OFF      ";
			}				
						
			flag_imprimir = 1;
		break;
		
		case '4':
		case '6':
		case '2':
			teclaLuz = ~teclaLuz;
			
			if (teclaLuz){				
				renglon2 = "       ON       ";
				//manda el MQTT
			}else{
				renglon2 = "       OFF      ";
				//manda el MQTT
			}				
						
			flag_imprimir = 1;
		break;
		
		case '8':
			//Serial.print("arriba\n");	
			status_pantalla = NAVEGANDO;			
			
			renglon2 = menuActual->nombre;
			menuActual = menuActual->menuPadre;	
			renglon1 = menuActual->nombre;
			opcion = menuActual->seleccion;
			flag_imprimir = 1;
		
		break;
	}
	
}//fin cmd_tecla

void cmd_contraste (void){
	
	renglon1 = "no se puede regular!";
	
	char cmd = (char) Serial.read();
		
	switch(cmd){
		default:
			renglon2 = "Valor: " + cmd;
		break;
		
		case '4':
			flag_imprimir = 1;
			renglon2 = "izquierda";
		break;
		
		case '2':
			flag_imprimir = 1;
			renglon2 = "abajo";
		break;
		
		case '6':
			flag_imprimir = 1;
			renglon2 = "derecha";
		break;
		
		case '8':
			//Serial.print("arriba\n");	
			status_pantalla = NAVEGANDO;			
			
			renglon2 = menuActual->nombre;
			menuActual = menuActual->menuPadre;	
			renglon1 = menuActual->nombre;
			opcion = menuActual->seleccion;
			flag_imprimir = 1;
		
		break;
	}
}//fin cmd_contraste

void cmd_salvaPantalla (void){

	renglon1 = "acá no hay nada...";
	
	char cmd = (char) Serial.read();
		
	switch(cmd){
		default:
			renglon2 = "Valor: " + cmd;
		break;
		
		case '4':
			flag_imprimir = 1;
			renglon2 = "izquierda";
		break;
		
		case '2':
			flag_imprimir = 1;
			renglon2 = "abajo";
		break;
		
		case '6':
			flag_imprimir = 1;
			renglon2 = "derecha";
		break;
		
		case '8':
			//Serial.print("arriba\n");	
			status_pantalla = NAVEGANDO;			
			
			renglon2 = menuActual->nombre;
			menuActual = menuActual->menuPadre;	
			renglon1 = menuActual->nombre;
			opcion = menuActual->seleccion;
			flag_imprimir = 1;
		
		break;
	}	
	
}//fin cmd_salvaPantalla


void setup() {
	
	pinMode (PIN_UP, INPUT_PULLUP);
	pinMode (PIN_DOWN, INPUT_PULLUP);
	pinMode (PIN_LEFT, INPUT_PULLUP);
	pinMode (PIN_RIGHT, INPUT_PULLUP);
	
	Serial.begin(115200);
	Serial.println();
	Serial.println();
	
	connections_handler();
	
	lcd.begin(16, 2);
	
	menuActual = &listaMenus[0];
	
	renglon1 = "      MENU      ";
	renglon2 = "   LUZ AFUERA   ";
	
	imprime_pantalla();
	
}

void loop() {
	
	timer_update();	
	
	if (flag_tick){
				
		if (reconnect_time) reconnect_time--;
		
        if (!flag_lecturas){
            for (uint8_t i = 0; i++; i < 4){
				read_boton[i] = digitalRead(pin_boton[i]);
				flag_lecturas = 1;
			}//end for
        }else{
            flag_lecturas=0;
        }	
		
		flag_tick = 0;
	}//end if flag_tick
	
    connections_handler();
    
    teclas();
	
	if (Serial.available()) {      // If anything comes in Serial (USB),
		
			switch (status_pantalla){
				
				case NAVEGANDO:
					navega_menu();				
				break;
				
				case SETEANDO_CMD:
					menuActual->accion();
				break;
				
				case SALVA_PANTALLA:
				
				break;
				
			}//fin switch
		
		
	}//fin if serial
	
	if (flag_imprimir){
			imprime_pantalla();
			flag_imprimir = 0;
	}
	
    //Serial.write(Serial1.read());   // read it and send it out Serial (USB)
  
}
