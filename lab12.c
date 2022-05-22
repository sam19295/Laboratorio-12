/* 
 * File:   lab12.c
 * Author: Melanie Samayoa
 */

// CONFIG1
#pragma config FOSC = INTRC_NOCLKOUT// Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled and can be enabled by SWDTEN bit of the WDTCON register)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RE3/MCLR pin function select bit (RE3/MCLR pin function is digital input, MCLR internally tied to VDD)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = OFF      // Brown Out Reset Selection bits (BOR disabled)
#pragma config IESO = OFF       // Internal External Switchover bit (Internal/External Switchover mode is disabled)
#pragma config FCMEN = OFF      // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is disabled)
#pragma config LVP = OFF        // Low Voltage Programming Enable bit (RB3 pin has digital I/O, HV on MCLR must be used for programming)

// CONFIG2
#pragma config BOR4V = BOR40V   // Brown-out Reset Selection bit (Brown-out Reset set to 4.0V)
#pragma config WRT = OFF        // Flash Program Memory Self Write Enable bits (Write protection off)

#include <xc.h>
#include <stdint.h>

#define _XTAL_FREQ 4000000             // Oscilador
#define address_POT 1                  // Constante para EEPROM


uint8_t bandera_write = 0;            
uint8_t bandera_read = 0;              
uint8_t valor_pot = 0;                  
uint8_t bandera_sleep = 0;             


void setup(void);
uint8_t read_EEPROM(uint8_t address);
void write_EEPROM(uint8_t address, uint8_t data);

// interrupciones

void __interrupt() isr (void){
    
    if(PIR1bits.ADIF){                  // Fue interrupci?n del ADC
        if(ADCON0bits.CHS == 0){        // Verificamos sea AN0 el canal seleccionado
            valor_pot = ADRESH;         // Guardar ADRESH
            PORTC = valor_pot;          // Mostrar el valor en el puerto C
        }
        PIR1bits.ADIF = 0;              // Limpiamos bandera de interrupci?n
    }
    
    else if (INTCONbits.RBIF){          // Fue interrupcion del Puerto B 
        if (PORTBbits.RB0 == 0){        // Fue RB0 
            if (bandera_sleep ==0){     // Est? despierto
                bandera_sleep =1; }     // Mandarlo a dormir
        }
        
        if (PORTBbits.RB1 == 0){        // Fue RB1 
            if (bandera_sleep ==1){     // Est? dormido
                bandera_sleep =0; }     // Mandarlo a despertar
        }
        
        if (PORTBbits.RB2 == 0){        // Fue RB2 
            bandera_sleep =0;           // Mandarlo a despertar
            bandera_write = 1; }        // Guardar valor en EEPROM 
        INTCONbits.RBIF = 0;            // Limpar la bandera de interrupcion
    }
    
    return;
}

// ciclo principal

void main(void) {
    setup();
    while(1){
        if(ADCON0bits.GO == 0){                 // No hay proceso de conversion
            ADCON0bits.GO = 1;}                 // Iniciamos proceso de conversi?n
       
        
        if (bandera_sleep==1){                  // Mandarlo a dormir
            PIE1bits.ADIE =0 ;                  // Desactivar interrupciones
            SLEEP ();                           // Mandarlo a dormir
        }
        else if (bandera_sleep ==0){            // Mandarlo a despertar
            PIE1bits.ADIE =1 ;                  // Activar interrupciones
        }
        
        if(bandera_write == 1){                 // Mandar a escribir en EEPROM
            write_EEPROM(address_POT,valor_pot);// Guardar el valor
            __delay_ms(10);                     // Esperar a que se haya guardado
            bandera_write = 0;                  // Dejar de escribir
        }
        
        PORTD = read_EEPROM(address_POT);       // PORTD con valor leido siempre
        PORTEbits.RE0 = 1;                      // Enncender RE0
        __delay_ms(250);                        
        PORTEbits.RE0 = 0;                      // Apagar RE0
        __delay_ms(250);                        
    }
    return;
}

// configuracion

void setup(void){
    
    // Configuraci?n de entradas y salidas
    ANSEL = 0b01;               // AN0 como entrada anal?gica
    ANSELH = 0;                 // I/O digitales
    
    TRISC = 0;                  // PORTC como salida
    PORTC = 0;                  // Limpiar PORTC
         
    TRISD = 0;                  // PORTD como salida
    PORTD = 0;                  // Limpiar PORTD
    
    TRISE = 0;                  // PORTE como salida
    PORTE = 0;                  // Limpiar PORTE
    
        
    // Configuraci?n reloj interno
    OSCCONbits.IRCF = 0b0110;   // 4MHz
    OSCCONbits.SCS = 1;         // Oscilador interno
    
    // Configuraci?n ADC
    ADCON0bits.ADCS = 0b01;     // Fosc/8
    ADCON1bits.VCFG0 = 0;       // VDD
    ADCON1bits.VCFG1 = 0;       // VSS
    ADCON0bits.CHS = 0b0000;    // Seleccionamos el AN0
    ADCON1bits.ADFM = 0;        // Justificado a la izquierda
    ADCON0bits.ADON = 1;        // Habilitamos modulo ADC
    __delay_us(40);             // Sample time
    
    //Interrupcion del puerto B
    OPTION_REGbits.nRBPU = 0;   // Habilitamos resistencias de pull-up del PORTB

    WPUBbits.WPUB0  = 1;        // Habilitamos resistencia de pull-up de RB0
    WPUBbits.WPUB1  = 1;        // Resistencia resistencia de pull-up de RB1
    WPUBbits.WPUB2  = 1;        // Resistencia resistencia de pull-up de RB2
    INTCONbits.RBIE = 1;        // Habilitamos interrupciones del PORTB
    
    IOCBbits.IOCB0  = 1;        // Habilitamos interrupci?n por cambio de estado para RB0
    IOCBbits.IOCB1  = 1;        // Habilitamos interrupci?n por cambio de estado para RB1
    IOCBbits.IOCB2  = 1;        // Habilitamos interrupci?n por cambio de estado para RB2
    INTCONbits.RBIF = 0;        // Limpiamos bandera de interrupci?n
    
    // Configuracion interrupciones
    PIR1bits.ADIF = 0;          // Limpiamos bandera de ADC
    PIE1bits.ADIE = 1;          // Habilitamos interrupci?n de ADC
    INTCONbits.PEIE = 1;        // Habilitamos interrupci?n de perifericos
    INTCONbits.GIE = 1;         // Habilitamos interrupci?nes globales
    
}

// Leer EEPROM
uint8_t read_EEPROM(uint8_t address){
    EEADR = address;            // Guardamos direcci?n a leer
    EECON1bits.EEPGD = 0;       // Lectura a la EEPROM
    EECON1bits.RD = 1;          // Obtenemos dato de la EEPROM
    return EEDAT;               // Regresamos dato 
}

// Escribir en la EEPROM
void write_EEPROM(uint8_t address, uint8_t data){
    EEADR = address;            // Guardamos direcci?n a escribir
    EEDAT = data;               // Guardar el dato que queremos mandar en EEDAT
    EECON1bits.EEPGD = 0;       // Escritura a la EEPROM
    EECON1bits.WREN = 1;        // Habilitamos escritura en la EEPROM
    
    INTCONbits.GIE = 0;         // Deshabilitamos interrupciones para escribir
    EECON2 = 0x55;              // Registro de control
    EECON2 = 0xAA;              // Registro de control
    
    EECON1bits.WR = 1;          // Iniciamos escritura
    
    EECON1bits.WREN = 0;        // Deshabilitamos escritura en la EEPROM
    INTCONbits.RBIF = 0;        // Limpiar bandera de interrupcion
    INTCONbits.GIE = 1;         // Habilitamos interrupciones
}