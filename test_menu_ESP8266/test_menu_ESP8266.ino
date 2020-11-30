
#define MENU_NIEVL_MAX 3

typedef struct {
	String nombre;
	uint8_t item;
	String subNombre;
	uint8_t itemMin;
	uint8_t itemMax;	
}T_MENU_GENERAL;


T_MENU_GENERAL menuImpreso;

uint8_t recorridoMenu = 0x1;
uint32_t menuNivel = 0;
uint8_t item = 0;


String menuTexto[4][8] = 	{
								{"        MENU        "},
								{"     LUZ AFUERA     ","   CONFIGURACION    "},
								{"        MODO        ","        LDR         ","       TECLA       ","     CONTRASTE      ","  SALVA PANTALLAS   "},
								{"       MANUAL       ","     AUTOMATICO     ","        min        ","        max         ","         ON         ", "         OFF        ","     PORCENTAJE     ","       TIMEPO       "},
							};


uint8_t menuItem_MinMax[4][8] = {
									{0x01},
									{0x02, 0x34},
									{0x01, 0x23, 0x45, 0x66, 0x77},
									{0xFF, 0xff,0xFF, 0xff,0xFF, 0xff,0xFF, 0xff},
								};




void imprimir_menu(){
	
	//menuImpreso.nombre =  menuTexto[menuNivel][ (recorridoMenu >> (menuNivel)) & 0xFF ];
	//menuImpreso.subNombre = menuTexto[menuNivel + 1][ (recorridoMenu >> menuNivel) & 0xFF ];
	
	Serial.print("\n\n" + menuImpreso.nombre + "\n");
	Serial.print(menuImpreso.subNombre + "\n");
	
	
}

void menu_handler(){
	
		
	item++;
	
	recorridoMenu &= ~(0xffffff00 << (4 * menuNivel));
	
	
	
	menuImpreso.nombre =  menuTexto[menuNivel][ (recorridoMenu >> (4 * menuNivel)) & 0x0F ];
	menuImpreso.subNombre = menuTexto[menuNivel + 1][ (recorridoMenu >> (4 * (menuNivel + 1))) ];
	
}


void setup() {
	
	Serial.begin(115200);
	
	
	
	imprimir_menu();
}

void loop() {
	if (Serial.available()) {      // If anything comes in Serial (USB),
		
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
		
		imprimir_menu();

  }


    //Serial.write(Serial1.read());   // read it and send it out Serial (USB)

  
  
}
