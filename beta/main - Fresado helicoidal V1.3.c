//#include <pic.h> // Archivo de cabecera propio del pic usado
#include <htc.h>

__CONFIG(WDTE_OFF & FOSC_HS & CP_OFF & PWRTE_ON & DEBUG_OFF & BOREN_ON & LVP_OFF & CPD_OFF); // Bits de configuracion del pic

#define _XTAL_FREQ		12000000
#define rebote 			__delay_ms(20)
#define O_bcd 			PORTA
#define O_1_bcd			RA0
#define O_led_prog		RA0
#define O_2_bcd			RA1
#define O_led_clear		RA1
#define O_3_bcd			RA2
#define O_led_step_miss	RA2
#define O_4_bcd			RA3
#define O_led_error		RA3
#define I_enc_1			RA4
#define I_enc_2			RA5
#define I_enc_3			RE0
#define I_enc_4			RE1
#define I_buttons		RC1
// #define 		RB0
#define O_disp_1		RD1
#define O_disp_2		RD0
#define O_disp_3		RC3
#define O_disp_4		RC2
#define O_leds			RD2
#define butt_modo		RD1
#define butt_1			RD0
#define butt_2			RD2
#define butt_3			RC2
#define butt_4			RD2
#define O_dir			RB7
#define O_step			RB6
#define O_half			RB5
#define O_enable		RB4
#define I_vref			RB3
#define I_sync			RB2

#define _max_step_speed			1 				// #Pasos por milisegundo
#define _step_accel				0.01 			// (#Pasos por milisegundo) por milisegundo
#define _digit_speed			0.01  			// #Digitos por milisegundo
#define _ms_per_overflow		0.085 			// #miliSegundos por cada desbordamiento de TMR0
#define _enc_step_delay 		1 				// milisegundos de retraso entre pasos en la fase de corte
#define _steps_per_revolution	28800			// Pasos para dar una vuelta completa al divisor			

// Declaracion de variables


bit dir;
bit saved_dir;
bit ms_flag;
bit TMR0_overflow_flag;
bit holding;
bit holded;
unsigned char modo; 		// || 0 = Fase de corte || 1 = Fase de conf. de angulo || 2 = Fase de conf. # de dientes ||
unsigned char disp1;
unsigned char disp2;
unsigned char active_button;
unsigned char char_temp;
unsigned char last_enc;
unsigned char now_enc;
unsigned char leds;
unsigned char ms_cont;
float steps_per_diente;
float x;
float i;
float actual_speed;
float run1_actual_speed;
float run2_actual_speed;
float enc_cont;
float step_cont;
float O_steps_per_I_enc; 	// Pasos por cada entrada de encoder
double enc_step_cont;
/*float*/ unsigned long ms_elapsed;
/*float A_by_ms_per_overflow;*/
unsigned int wait_time;
unsigned int run_time;
unsigned int butt_time;
unsigned int miss_time;
unsigned int error_time;
unsigned int clear_time;
unsigned int int_temp;
signed int num_dientes;
signed int int_enc_step_cont;
eeprom signed int EEPROM_num_dientes;
eeprom signed long EEPROM_enc_cont;
eeprom signed long EEPROM_step_cont;
eeprom float EEPROM_O_steps_per_I_enc;
eeprom float EEPROM_steps_per_diente;
unsigned long run1_ms_elapsed;
unsigned long run2_ms_elapsed;

typedef struct {
        unsigned        b0:1, b1:1, b2:1, b3:1, b4:1, b5:1, b6:1, b7:1;
}       bitv;  

#define led_error 		(((bitv *)&leds)->b3)
#define led_step_miss 	(((bitv *)&leds)->b2)
#define led_clear   	(((bitv *)&leds)->b1)
#define led_prog   		(((bitv *)&leds)->b0)
#define now_enc2   		(((bitv *)&now_enc)->b1)
#define now_enc1   		(((bitv *)&now_enc)->b0)
// Fin deeclaracion de variables

// Declaracion de funciones
void move (unsigned char d) {
	O_dir = d;
	O_step = 1;
	__delay_us(3);
	O_step = 0;
};
void check_buttons (void) {
	if (!butt_modo) {
		if ( (active_button == 0) && (!I_buttons) ) {
			rebote;
			active_button = 1;
		}
		if (active_button == 1) {
			holding = (!I_buttons);
		}
	}
	else if (!butt_1) {
		if ( (active_button == 0) && (!I_buttons) ) {
			rebote;
			active_button = 2;
			dir = 0;
		}
		if (active_button == 2) {
			holding = (!I_buttons);
		}
	}
	else if (!butt_2) {
		if ( (active_button == 0) && (!I_buttons) ) {
			rebote;
			active_button = 3;
			dir = 1;
		}
		if (active_button == 3) {
			holding = (!I_buttons);
		}
	}		
};
void update_disp (signed int show) {
	char_temp = show % 100;
	disp1 = char_temp % 10;
	disp2 = (char_temp - disp1) / 10;
}

bit wait_move (float V, float A) {
	if (A == 0) {actual_speed = V;}
	if (ms_flag /*TMR0_overflow_flag*/) {
		/*ms_elapsed += _ms_per_overflow;*/
		if (actual_speed < V) {
			actual_speed += A /* * _ms_per_overflow*/;
		}
		x += actual_speed;
	}
	/*x = actual_speed * ms_elapsed;*/
	if (x >= 1) {		
		x -= 1; /*ms_elapsed = (x - 1) / actual_speed;*/
		return 1;
	} else {return 0;}
}

bit wait_ms (int ms) {
	if (wait_time <= ms) {
		if (ms_flag) {++wait_time;}
		return 0;
	} 
	else {return 1;}
}


// Inicio del programa
void main(void) {

// Configuracion inicial del PIC
	rebote;
	ADCON1 = 0b111;			// CONFIGURA QUE TODOS LOS PINES ANALÓGICOS FUNCIONEN COMO DIGITALES.
	TRISA = 0b110000;		// CONFIGURA RA4 y RA5 COMO ENTRADA. Y LAS DEMAS COMO SALIDAS.
	TRISB = 0b00001111;		// CONFIGURA RB0-RB3 COMO ENTRADA. Y LAS DEMAS COMO SALIDAS.
	TRISC = 0b0011;
	TRISD = 0b11110000;
	TRISE = 0b111;			// CONFIGURA PORTE COMO ENTRADA. Y LAS DEMAS COMO SALIDAS.
	OPTION_REG = 0b11001000;
// Fin de configuracion

	leds = 0b0111;
	now_enc1 = I_enc_1;
	now_enc2 = I_enc_2;
	O_half = 1;
	O_enable = 1;

	enc_cont = EEPROM_enc_cont;
	step_cont = EEPROM_step_cont;
	num_dientes = EEPROM_num_dientes;
	steps_per_diente = EEPROM_steps_per_diente;
	O_steps_per_I_enc = EEPROM_O_steps_per_I_enc;

	while (1) {

		ms_flag = 0;
		TMR0_overflow_flag = 0;
		if (T0IF) {
			T0IF = 0;
			TMR0_overflow_flag = 1;

			if (++ms_cont == 12) {
				ms_cont = 0;
				ms_flag = 1;

				if (!led_step_miss) {
					wait_time = miss_time;
					if (wait_ms(1000)) {
						led_step_miss = 1;
						wait_time = 0;
					}
					miss_time = wait_time;
				}

				if (!led_error) {
					if (!I_buttons) {led_error = 1;}
					/*wait_time = error_time;
					if (wait_ms(1000)) {
						led_error = 1;
						wait_time = 0;
					}
					error_time = wait_time;*/
				}

				if (!led_clear) {
					wait_time = clear_time;
					if (wait_ms(1000)) {
						led_clear = 1;
						wait_time = 0;
					}
					clear_time = wait_time;
				}

				if (!O_disp_1){
					O_disp_1 = 1;
					O_bcd = disp2;
					O_disp_2 = 0;
				} 
				else if (!O_disp_2) {
					O_disp_2 = 1;
					O_bcd = leds;
					O_leds = 0;
				}
				else if (!O_leds) {
					O_leds = 1;
					O_bcd = disp1;
					O_disp_1 = 0;
				}
			}

			check_buttons();

		}

		last_enc = now_enc;
		now_enc1 = I_enc_1;
		now_enc2 = I_enc_2;

		if (modo == 0) {

			if (last_enc != now_enc) {
				if ( ( (last_enc == 0b00) && (now_enc == 0b01) ) || ( (last_enc == 0b01) && (now_enc == 0b11) ) || ( (last_enc == 0b11) && (now_enc == 0b10) ) || ( (last_enc == 0b10) && (now_enc == 0b00) ) ) {
					enc_step_cont += O_steps_per_I_enc;
					// update_disp(enc_step_cont);
				}
				else if ( ( (now_enc == 0b00) && (last_enc == 0b01) ) || ( (now_enc == 0b01) && (last_enc == 0b11) ) || ( (now_enc == 0b11) && (last_enc == 0b10) ) || ( (now_enc == 0b10) && (last_enc == 0b00) ) ) {
					enc_step_cont -= O_steps_per_I_enc;
					// update_disp(enc_step_cont);
				}
				else {led_step_miss = 0;}
			}

			if (enc_step_cont >= 1) {
				++int_enc_step_cont;
				enc_step_cont -= 1;
			}
			else if (enc_step_cont <= -1) {
				--int_enc_step_cont;
				enc_step_cont += 1;
			}

			wait_time = run_time;
			if (ms_flag /*wait_ms(_enc_step_delay)*/) {
				if (int_enc_step_cont >= 1) {
					move(0);
					int_enc_step_cont -= 1;
				}
				else if (int_enc_step_cont <= -1) {
					move(1);
					int_enc_step_cont += 1;
				}
				wait_time = 0;
			}
			run_time = wait_time;

			// Funcion de los botones
			if (active_button > 0) {
				if (!holding) {
					if (active_button == 1) {
						modo = 1;
						O_enable = 1;
						led_prog = 0;
						active_button = 0;
						actual_speed = 0;
						ms_elapsed = 0;
						run1_ms_elapsed = 0;
						run1_actual_speed = 0;
						run2_ms_elapsed = 0;
						run2_actual_speed = 0;
						rebote;
					}
					else if ( (active_button == 2) || (active_button == 3) ) {
						actual_speed = run2_actual_speed;
						ms_elapsed = run2_ms_elapsed;
						if (wait_move(_max_step_speed, _step_accel)) {
							if (dir != saved_dir) {i = -i;}
							saved_dir = dir;
							if (++i > steps_per_diente) {
								active_button = 0;
								actual_speed = 0;
								ms_elapsed = 0;
								int_temp = i; 	// Guarda la parte entera de "i" en "int_temp", ya que "int_temp" esta declarada como int
								i -= int_temp;
							}
							else {move(dir);}
						}
						run2_ms_elapsed = ms_elapsed;
						run2_actual_speed = actual_speed;
					}
				}					
			}

		}

		else if (modo == 1) {

			if ( ( (last_enc == 0b00) && (now_enc == 0b01) ) || ( (last_enc == 0b01) && (now_enc == 0b11) ) || ( (last_enc == 0b11) && (now_enc == 0b10) ) || ( (last_enc == 0b10) && (now_enc == 0b00) ) ) {
				enc_cont += 1;
				update_disp(enc_cont);
			} 
			else if ( ( (now_enc == 0b00) && (last_enc == 0b01) ) || ( (now_enc == 0b01) && (last_enc == 0b11) ) || ( (now_enc == 0b11) && (last_enc == 0b10) ) || ( (now_enc == 0b10) && (last_enc == 0b00) ) ) {
				enc_cont -= 1;
				update_disp(enc_cont);
			} 
			else if (last_enc != now_enc) {
				led_step_miss = 0;
			}

			// Funcion de los botones
			if (active_button > 0) {
				if (holding) {	

					if (active_button == 1) {
						if (!holded) {
							wait_time = butt_time;
							if (wait_ms(600)) { 
								led_clear = 0;							
								enc_cont = 0;
								step_cont = 0;
								num_dientes = 0;
								holded = 1;
							}
							butt_time = wait_time;
						}
					}
					else if ( (active_button == 2) || (active_button == 3) ) {
						wait_time = butt_time;
						if (wait_ms(600)) { 
							if (wait_move(_max_step_speed, _step_accel)) {
								move(dir);
								if (dir) {--step_cont;}
								else {++step_cont;}
								update_disp(step_cont);
							}
						}
						butt_time = wait_time;
					}
				} else {
					if (active_button == 1) {
						if (holded) {
							holded = 0;
							active_button = 0;
						} else {
							modo = 2;
							O_enable = 0;
							O_steps_per_I_enc = step_cont / enc_cont;
							EEPROM_enc_cont = enc_cont;
							EEPROM_step_cont = step_cont;
							EEPROM_O_steps_per_I_enc = O_steps_per_I_enc;
							if (O_steps_per_I_enc == 0) {led_clear = 0;}
						}
					}
					else if ( (active_button == 2) || (active_button == 3) ) {
						move(dir);
						if (dir) {--step_cont;}
						else {++step_cont;}
						update_disp(step_cont);
					}
					rebote;
					active_button = 0;	
					actual_speed = 0;
					ms_elapsed = 0;
					butt_time = 0;
				}					
			}

		}

		else if (modo == 2) {

			wait_time = prog_time;
			if (wait_ms(200)) {
				if (!led_prog) {led_prog = 1;}
				else {led_prog = 0;}
				wait_time = 0;
			}
			prog_time = wait_time;

			// Funcion de los botones
			if (active_button > 0) {			
				if (holding) {
					if ( (active_button == 2) || (active_button == 3) ) {
						wait_time = butt_time;
						if (wait_ms(600)) { 
							if (wait_move(_digit_speed, 0)) {
								if (active_button == 2) {++num_dientes;}
								else {--num_dientes;}
								update_disp(num_dientes);
							}
						}
						butt_time = wait_time;
					}
				}
				else {
					if (active_button == 1) {
						modo = 0;
						O_enable = 1;
						i = 0;
						led_prog = 1;
						steps_per_diente = _steps_per_revolution / num_dientes;
						EEPROM_num_dientes = num_dientes;
						EEPROM_steps_per_diente = steps_per_diente;
					}
					else if (active_button == 2) {
						++num_dientes;
						update_disp(num_dientes);
					}
					else if (active_button == 3) {
						--num_dientes;
						update_disp(num_dientes);
					}
					rebote;
					active_button = 0;	
					actual_speed = 0;
					ms_elapsed =0;
					butt_time = 0;
				}			
			}

		};

	}
}