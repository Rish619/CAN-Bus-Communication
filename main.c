//
//  main.cpp
//  
//
//  Created by Ranjan Rishi Chambial.
//  
//

#include <stm32f10x_cl.h>

int main (void)
{
	//////////////////////////////
	//*****DEFINE VARIABLES*****//
	//////////////////////////////
	int ADCvalue;
	int userSwitch;
	int tamperSwitch;
	int led8;
	int led9;
	
	//////////////////////////////
	//*****CONFIGURE CLOCKS*****//
	//////////////////////////////
	RCC->APB2ENR |= 0x279;																			//Enable clocks for Port B/C/D/E, ADC1, AF
	RCC->APB1ENR |= (1<<25);																		//Enable clock for CAN
	RCC->CFGR |= (1<<15);																				//Divide clock by 6 for ADC
	
	////////////////////////////
	//*****CONFIGURE GPIO*****//
	////////////////////////////
	GPIOB->CRL |= (1<<30);																			//Enable User Switch
	GPIOC->CRL &= ~(15<<16);																		//Enable Analog Input for ADC
	GPIOC->CRH |= (1<<22);																			//Enable Tamper Switch
	GPIOD->CRL |= 0xB4;																				//Enable CAN Inputs and Outputs
	GPIOE->CRH = 0x22222222;																		//Enable LEDs
	
	AFIO->MAPR |= (3<<13);																			//Remap CAN to Port D
	
	///////////////////////////
	//*****CONFIGURE ADC*****//
	///////////////////////////
	//ADC1->CR2 |= (1<<2);																				//Enable Calibration
	//while(ADC1->CR2 & (1<<2));																	//Wait for Calibration to finish
	ADC1->CR2 |= (1<<20);																				//Enable Ext Trigger
	ADC1->CR2 |= (7<<17); 																			//Set SWSTART as Ext Trigger
	ADC1->CR2 &= ~(1<<11); 																			//Align data right
	ADC1->CR2 |= (1<<1); 																				//Enable ontinuous conversion
	ADC1->CR2 |= (1<<0); 																				//Turn ADC on
	ADC1->SQR1 = 0x00000000;																		//Set for one conversion
	ADC1->SQR3 = (14<<0); 																			//Channel 14 as first conversation
	ADC1->CR2 |= (1<<22);																				//Start SW conversion
	
	///////////////////////////
	//*****CONFIGURE CAN*****//
	///////////////////////////
	CAN1->MCR |= (1<<15);																				//Reset CAN1
	while(CAN1->MCR & (1<<15));																	//Wait for CAN1 to reset
	CAN1->MCR |= (1<<0);																				//Enable Intialisation
	CAN1->MCR |= (1<<4);																				//Turn NART on
	CAN1->MCR &= ~(1<<1);																				//No sleep
	
	//Bit Timing
	CAN1->BTR |= (1<<3);																				//Set BRP = 0x08
	CAN1->BTR |= (11<<16);																			//Set TS1 = 0x0B
	CAN1->BTR |= (1<<21);																				//Set TS2 = 0x02
	CAN1->BTR |= (1<<25);																				//Set SJW = 0x02
	
	//Filter Setup
	CAN1->FMR |= (1<<0);																				//Enable Filter Initialisation
	CAN1->FM1R |= (3<<0);																				//Enable Filter0 and Filter1 as Identifier List mode
	CAN1->FS1R |= (3<<0);																				//Set Filter0 and Filter1 as 32 bit
	CAN1->FFA1R |= (1<<1);																			//Assign Filter0 to FIFO0 and Filter1 to FIFO1
	CAN1->Filter0->FR1 = (0x01A4F2B<<3)|0x04;										//Set Filter0
	CAN1->Filter1->FR1 = (0x0024FCE<<3)|0x04;										//Set Filter1
	//CAN1->Filter2->FR1 = (0xBADCAFE<<3)|0x04;
	CAN1->FMR &= ~(1<<0);																				//Turn off init.
	CAN1->FA1R |= (3<<0);																				//Turn on Filter0 and Filter1

	//Loopback Mode. Comment out when finished testing
	//CAN1->BTR |= (1<<30);																				//Loopback Mode On	
	
	CAN1->MCR &= ~(1<<0);																				//Turn off initialisation

	//////////////////////////
	//*****MAIN PROGRAM*****//
	//////////////////////////
	while(1)
	{
		//*****Tx ADC*****//
		while(!(ADC1->SR&(1<<1))); 																//Wait for EOC
		ADCvalue = ADC1->DR; 																			//Get ADC value
		ADC1->SR &= ~(1<<1);																			//Reset EOC
		
		CAN1->TxMailBox0->TDTR = (0x104);													//Set Tx Data Length (4 Bytes)
		CAN1->TxMailBox0->TIR = (0xBADCAFE<<3)|0x04;							//Enable Address with Extended ID
		CAN1->TxMailBox0->TDLR = ADCvalue;												//Set Data Bytes
	
		CAN1->TxMailBox0->TIR |= (1<<0);													//Send Data packet
		
		//*****Tx USER SWITCH*****//
		userSwitch = GPIOB->IDR;																	//Get User Switch value
		if ((userSwitch & (1<<7)) == 0)														//If User Switch is pressed
		{
			CAN1->TxMailBox1->TDTR = 0x101;												//Set Tx Data Length (1 Byte)
			CAN1->TxMailBox1->TIR = (0x01AEFCA<<3)|0x04;						//Enable Address with Extended ID
			CAN1->TxMailBox1->TDLR = 0x01;													//Set Data Byte
	
			CAN1->TxMailBox1->TIR |= (1<<0);												//Send Data packet
		}
		/*else
		{
			CAN1->TxMailBox1->TDTR = 0x101;												//Set Tx Data Length (1 Byte)
			CAN1->TxMailBox1->TIR = (0x01AEFCA<<3)|0x04;						//Enable Address with Extended ID
			CAN1->TxMailBox1->TDLR = 0x00;													//Set Data Byte
	
			CAN1->TxMailBox1->TIR |= (1<<0);												//Send Data packet
		}*/
		
		//*****Tx TAMPER SWITCH*****//
		tamperSwitch = GPIOC->IDR;																//Get Tamper Switch value
		if ((tamperSwitch & (1<<13)) == 0)												//If Tamper Switch is pressed
		{
			CAN1->TxMailBox2->TDTR = 0x101;												//Set Tx Data Length (1 Byte)
			CAN1->TxMailBox2->TIR = (0x0002BEF<<3)|0x04;						//Enable Address with Extended ID
			CAN1->TxMailBox2->TDLR = 0x01;													//Set Data Byte
	
			CAN1->TxMailBox2->TIR |= (1<<0);												//Send Data packet
		}
		/*else
		{
			CAN1->TxMailBox2->TDTR = 0x101;												//Set Tx Data Length (1 Byte)
			CAN1->TxMailBox2->TIR = (0x0002BEF<<3)|0x04;						//Enable Address with Extended ID
			CAN1->TxMailBox2->TDLR = 0x00;													//Set Data Byte
	
			CAN1->TxMailBox2->TIR |= (1<<0);												//Send Data packet
		}*/
		
		//*****Rx LED8*****//
		if ((CAN1->RF0R & (3<<0)) != 0)														//If FIFO0 has messages
		{
			led8 = CAN1->FIFOMailBox0->RDLR;												//Get message
			if (led8 == 0x01)																				//If message is 0x01
			{
				GPIOE->ODR |= (1<<8);																	//Turn on LED8
			}
			else if (led8 == 0x00)																	//If message is 0x00
			{
				GPIOE->ODR &= ~(1<<8);																//Turn off LED8
			}
			CAN1->RF0R |= (1<<5);																		//Release Mailbox
		}
		
		//*****Rx LED9*****//
		if ((CAN1->RF1R & (3<<0)) != 0)														//If FIFO1 has messages
		{
			led9 = CAN1->FIFOMailBox1->RDLR;												//Get message
			if (led9 == 0x01)																				//If message is 0x01
			{
				GPIOE->ODR |= (1<<9);																	//Turn on LED9
			}
			else if (led9 == 0x00)																	//If message is 0x00
			{
				GPIOE->ODR &= ~(1<<9);																//Turn off LED9
			}
			CAN1->RF1R |= (1<<5);																		//Release Mailbox
		}
	}
}
