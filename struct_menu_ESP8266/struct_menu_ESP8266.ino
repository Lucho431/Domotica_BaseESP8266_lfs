#include <LiquidCrystal.h>
#define RS 12
#define EN 11
#define D4 10
#define D5 9
#define D6 8
#define D7 7




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



void imprime_pantalla (void){
	
	//Serial.print("\n" + menuActual->nombre + "\n");
	lcd.clear();
	lcd.setCursor(0,0);
	lcd.print(menuActual->nombre);
		
	lcd.setCursor(0,1);
	if (menuActual->firstSubMenu != NULL){
		T_MENU_GENERAL* aux = menuActual->firstSubMenu + menuActual->seleccion;
		//Serial.print(aux->nombre + "\n");
		lcd.print(aux->nombre);
	} else{
		//Serial.print("naranja...      ");
		lcd.print("naranja...      ");
	}
}//fin imprime_pantalla


void navega_menu (void){
	
	char cmd = (char) Serial.read();
		
	switch(cmd){
		
		case '8':
			//Serial.print("arriba\n");	
			if (menuActual->menuPadre != NULL){ 
				
				menuActual = menuActual->menuPadre;
				
				renglon1 = menuActual->nombre;
				T_MENU_GENERAL* aux = menuActual->firstSubMenu + menuActual->seleccion;
				renglon2 = aux->nombre;
				
				flag_imprimir = 1;
			}
		
		break;
		
		case '2':
			//Serial.print("abajo\n");
			if (menuActual->firstSubMenu != NULL){ 
				
				menuActual = menuActual->firstSubMenu + menuActual->seleccion;
				opcion = 0;
				
				renglon1 = menuActual->nombre;
				
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
			
			flag_imprimir = 1;			
		break;
		
		case '6':
			//Serial.print("derecha\n");
			opcion++;
			if (opcion >= menuActual->totalSubMenus) opcion = 0;
			menuActual->seleccion = opcion;
			
			flag_imprimir = 1;			
		break;
		
	}//end switch	
	
}//fin navega_menu


void cmd_modo (void){

	char cmd = (char) Serial.read();
		
	switch(cmd){
		
		case '8':
			//Serial.print("arriba\n");	
			if (!menuNivel) menuNivel--;			
		
		break;
		
		case '2':
			//Serial.print("abajo\n");
			if (menuNivel < MENU_NIEVL_MAX - 1) menuNivel++;
		
		break;
		
		case '4':
			//Serial.print("izquierda\n");
			
		
		break;
		
		case '6':
			//Serial.print("derecha\n");
		
		break;
		
	}//end switch
	
	
}//fin cmd_modo


void cmd_ldr (void){
	
	
}//fin cmd_ldr


void cmd_tecla (void){
	
	
}//fin cmd_tecla

void cmd_contraste (void){
	
	
}//fin cmd_contraste


void cmd_salvaPantalla (void){
	
	
}//fin cmd_salvaPantalla


void setup() {
	
	Serial.begin(115200);
	
	lcd.begin(16, 2);
	
	menuActual = &listaMenus[0];
	
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
