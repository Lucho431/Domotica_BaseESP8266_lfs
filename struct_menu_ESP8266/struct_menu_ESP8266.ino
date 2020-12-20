#include <LiquidCrystal.h>
#define RS 12
#define EN 11
#define D4 10
#define D5 9
#define D6 8
#define D7 7


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
	
	NAVEGANDO = 0,
	SETEANDO_CMD,
	SALVA_PANTALLA,
} T_ESTATUS_PANTALLA;
	

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

LiquidCrystal lcd(RS,EN,D4,D5,D6,D7);

String renglon1, renglon2;

//variables de la luz de afuera:
uint8_t mode = 0;
uint8_t mode_selected = 0;
uint8_t teclaLuz = 0;
uint8_t teclaLuz_selected = 0;
int LDR_actual = 512;


void imprime_pantalla (void){
	
	lcd.clear();
	lcd.setCursor(0,0);
	lcd.print(renglon1);
	lcd.setCursor(0,1);
	lcd.print(renglon2);
	
}//fin imprime_pantalla


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
	
	Serial.begin(115200);
	
	lcd.begin(16, 2);
	
	menuActual = &listaMenus[0];
	
	renglon1 = "      MENU      ";
	renglon2 = "   LUZ AFUERA   ";
	
	imprime_pantalla();
	
}

void loop() {
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
