#include <stdio.h>
#include "def_pinos.h"
#include "config.c"

#define R 1
#define W 0
#define EEPROM 0xA0 //Endereco da memoria da placa de desenvolvimento

unsigned char retorno;
unsigned char relogio = 0;

void delay_ms(unsigned int t)
{
TMOD |= 0x01;
TMOD &= ~0x02;
for(;t>0;t--)
	{
	TR0=0;
	TF0=0;
	TL0=0x58;	//1ms para 25000000
	TH0=0x9e;
	TR0=1;
	while(TF0==0);
	}
}

unsigned char escreve_byte_controle(unsigned char endereco_dispositivo, __bit RW){

	STA = 1; // Gera condicao de Start
	SI = 0; 
	while(SI == 0); // Quando sair o Status mudou
	if(SMB0STA != 0x08 && SMB0STA != 0x10){
		return (SMB0STA);
	}
	SMB0DAT = ( endereco_dispositivo & 0xfe) | RW; // Modelo o COntrol Byte
	STA = 0; //
	SI = 0;
	while(SI == 0); // Sai quando houver mudanca no status	
	if(RW == W){ //verifica se e escrita	
		if(SMB0STA != 0x18){ // 0x18 ack recebido em escrita		
			return (SMB0STA);			
		}
	}else{
		if(SMB0STA != 0x40){ // 0x40 ack rebecido em leitura
			return (SMB0STA);
		}
	}	
	return (0);

}
unsigned char escreve_byte_dados(unsigned char dado){
	SMB0DAT = dado;
	SI = 0;
	while(SI == 0);
	if(SMB0STA != 0x28){ // 0x28 byte de dados transmitido e ack recebido	
		return (SMB0STA);
	}
	return (0);
}

void putchar(char c){
    SBUF0 = c;
    while(TI0 == 0); //Trava enquanto SBUF ï¿½ transmitido
    TI0 = 0;
}
void Timer4_ISR (void) interrupt 16{
	SMB0CN &= ~0x40;  // Desabilita SMBus
	SMB0CN |= 0x40;   // Habilita SMBus
	TF4 = 0;  // Zera flag de interrupï¿½ï¿½o do TC4
}

int escreve_eeprom(unsigned char endereco, unsigned char dados){	
	if(!escreve_byte_controle(EEPROM, W)){
		if(escreve_byte_dados(endereco) == 0 ){
			if(escreve_byte_dados(dados) == 0){		
				STO = 1;
				SI = 0;
				while(STO == 1);		
				while(escreve_byte_controle(EEPROM, W) != 0){			
					escreve_byte_controle(EEPROM, W);
				}				
				return (0);							
			}
		}
	}
	return (SMB0STA);

}

unsigned char leitura_eeprom(unsigned char endereco){
	if(!escreve_byte_controle(EEPROM, W) ){
		if(escreve_byte_dados(endereco) == 0 ){
			if(escreve_byte_controle(EEPROM, R) == 0){	
				AA = 0;
				SI = 0;
				while(SI == 0);		
				STO = 1;
				SI = 0;
				while(STO == 1);								
			}			
		}	
	}
	return (SMB0DAT);
}
//O firmare escuta a entrada Uart0
//E Exibe na Uart0
void main(void){
    Init_Device();
    SFRPAGE = LEGACY_PAGE;	
	while(1){
		if(relogio == 0){ // condicional pensado em em caso onde é possivel fazer rest no contador
			if( leitura_eeprom(125) == 1){
				relogio = leitura_eeprom(127);
			}else{
				relogio = 0;
			}
		}
		if(relogio == 255){
			relogio = 0;
		}
		printf_fast_f("Cont: %u \n", relogio);
		escreve_eeprom(127, relogio);
		if(leitura_eeprom(127) != relogio){
			printf_fast_f("Diverge\n");
			while(1);
		}		
		relogio++;	
		delay_ms(50);
		escreve_eeprom(125, 1);
		//escreve_eeprom(125, 0); //zerar contador
		
		
	}

}
