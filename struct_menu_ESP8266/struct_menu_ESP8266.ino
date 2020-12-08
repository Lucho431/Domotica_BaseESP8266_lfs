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
	void (*accion) (void);
}T_MENU_GENERAL;


T_MENU_GENERAL listaMenus[20]= {
									{"      MENU      ", NULL, &listaMenus[1], 2, NULL},  //0
									
									{"   LUZ AFUERA   ", &listaMenus[0], &listaMenus[3], 3, NULL},  //1 
									{" CONFIGURACION  ", &listaMenus[0], &listaMenus[6], 2, NULL},  //2
									
									{"      MODO      ", &listaMenus[1], NULL, 0, NULL},  //3
									{"      LDR       ", &listaMenus[1], NULL, 0, NULL},  //4
									{"     TECLA      ", &listaMenus[1], NULL, 0, NULL},  //5
									
									{"   CONTRASTE    ", &listaMenus[2], NULL, 0, NULL},  //6
									{"SALVA PANTALLAS ", &listaMenus[2], NULL, 0, NULL},  //7
};
	
	
int8_t opcion = 0;
T_MENU_GENERAL* menuActual;
uint8_t flag_imprimir = 0;

LiquidCrystal lcd(RS,EN,D4,D5,D6,D7);



void imprime_pantalla (void){
	
	//Serial.print("\n" + menuActual->nombre + "\n");
	lcd.clear();
	lcd.setCursor(0,0);
	lcd.print(menuActual->nombre);
		
	lcd.setCursor(0,1);
	if (menuActual->firstSubMenu != NULL){
		T_MENU_GENERAL* aux = menuActual->firstSubMenu +opcion;
		//Serial.print(aux->nombre + "\n");
		lcd.print(aux->nombre);
	} else{
		//Serial.print("naranja...      ");
		lcd.print("naranja...      ");
	}
}


void setup() {
	
	Serial.begin(115200);
	
	lcd.begin(16, 2);
	
	menuActual = &listaMenus[0];
	
	imprime_pantalla();
	
}

void loop() {
	if (Serial.available()) {      // If anything comes in Serial (USB),
		
		char cmd = (char) Serial.read();
		
		switch(cmd){
			
			case '8':
				//Serial.print("arriba\n");	
				if (menuActual->menuPadre != NULL){ 
					menuActual = menuActual->menuPadre;
					flag_imprimir = 1;
				}
			
			break;
			
			case '2':
				//Serial.print("abajo\n");
				if (menuActual->firstSubMenu != NULL){ 
					menuActual = menuActual->firstSubMenu +opcion;
					opcion = 0;
					flag_imprimir = 1;
				}
			
			break;
			
			case '4':
				//Serial.print("izquierda\n");
				opcion--;
				if (opcion < 0) opcion = menuActual->totalSubMenus - 1;
				
				flag_imprimir = 1;			
			break;
			
			case '6':
				//Serial.print("derecha\n");
				opcion++;
				if (opcion >= menuActual->totalSubMenus) opcion = 0;
				
				flag_imprimir = 1;			
			break;
			
		}//end switch
		
	}
	
	if (flag_imprimir){
			imprime_pantalla();
			flag_imprimir = 0;
	}
	
    //Serial.write(Serial1.read());   // read it and send it out Serial (USB)
  
}
