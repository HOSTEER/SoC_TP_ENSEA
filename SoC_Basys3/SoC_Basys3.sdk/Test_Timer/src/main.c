#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xgpio.h"
#include "xtmrctr.h"
#include "xintc.h"
#include "sleep.h"

#define MAX 18

XGpio btn;
uint8_t sens=0;
XTmrCtr timer;
XIntc xintc;

uint8_t msg[MAX]={16,16,16,16,10,11,12,12,13,16,2,0,2,3,16,16,16,16};

void Afficher_hello(uint8_t position){//affiche une partie du message avec un d�calage "position"
	volatile uint16_t * ptrSegAn = (uint16_t *)(XPAR_AXI_GPIO_0_BASEADDR + 0x008);
	volatile uint16_t * ptrSegCa = (uint16_t *)(XPAR_AXI_GPIO_1_BASEADDR);
	*(ptrSegAn+0x0002)=0;
	*(ptrSegCa+0x0002)=0;

	static uint8_t displaySelected=0;
	uint8_t data=0;
	switch(displaySelected){
		case 0:{*ptrSegAn=0xF7;displaySelected++;data=msg[position];break;}
		case 1:{*ptrSegAn=0xFB;displaySelected++;data=msg[position+1];break;}
		case 2:{*ptrSegAn=0xFD;displaySelected++;data=msg[position+2];break;}
		default:{*ptrSegAn=0xFE;displaySelected=0;data=msg[position+3];break;}
	}
	switch(data){
		case 0:{*ptrSegCa=0xC0;break;}
		case 1:{*ptrSegCa=0xF9;break;}
		case 2:{*ptrSegCa=0xA4;break;}
		case 3:{*ptrSegCa=0xB0;break;}
		case 4:{*ptrSegCa=0x99;break;}
		case 5:{*ptrSegCa=0x92;break;}
		case 6:{*ptrSegCa=0x82;break;}
		case 7:{*ptrSegCa=0xF8;break;}
		case 8:{*ptrSegCa=0x80;break;}
		case 9:{*ptrSegCa=0x90;break;}
		case 10:{*ptrSegCa=0x89;break;}//H
		case 11:{*ptrSegCa=0x86;break;}//E
		case 12:{*ptrSegCa=0xC7;break;}//L
		case 13:{*ptrSegCa=0xC0;break;}//O
		case 14:{*ptrSegCa=0x86;break;}
		case 15:{*ptrSegCa=0x8E;break;}
		default:{*ptrSegCa=0xFF;break;}
	}
}

void Animer_message(){//sens de d�filement, � gauche 0, � droite 1
	volatile uint16_t * ptrSw = (uint16_t *)(XPAR_AXI_GPIO_2_BASEADDR);
	*(ptrSw+0x0002)=1;
	uint16_t shiftingCount=((*ptrSw&0x000F)<<9)+512;
	static uint16_t countBeforeShifting=0;
	static uint8_t decalage=0;
	if(countBeforeShifting>shiftingCount){
		countBeforeShifting=0;
		switch(sens){
			case 0:{if(decalage==MAX-4){decalage=0;};decalage++;break;}
			default:{if(decalage==0){decalage=MAX-4;};decalage--;break;}
		}
	}
	else{countBeforeShifting++;}
	Afficher_hello(decalage);
}

void Timer_isr(void *CallBackRef, u8 TmrCtrNumber){
	Animer_message();
}

void Btn_isr(void *baseaddr_p){
	//XGpio_InterruptDisable(&btn,XGPIO_IR_CH1_MASK);//Interruption pour le bouton
	if((XGpio_InterruptGetStatus(&btn)&XGPIO_IR_CH1_MASK)!=XGPIO_IR_CH1_MASK){
		return;
	}
	static uint8_t nobounce=2;
	//uint8_t btn_value = XGpio_DiscreteRead(&btn,1);
	if(sens==0&&nobounce==2){
		sens=1;
		nobounce=0;
	}else if(sens==1&&nobounce==2){
		sens=0;
		nobounce=0;
	}
	nobounce++;
	printf("%d\r\n",sens);
	//XGpio_DiscreteWrite(&led,1,led_data);
	(void)XGpio_InterruptClear(&btn, XGPIO_IR_CH1_MASK);
	XGpio_InterruptEnable(&btn, XGPIO_IR_CH1_MASK);

}

int Interrupt_init(){
	int Status = XIntc_Initialize(&xintc, XPAR_INTC_0_DEVICE_ID);
	if (Status != XST_SUCCESS) return XST_FAILURE;

	Status = XIntc_Connect(&xintc, XPAR_INTC_0_TMRCTR_0_VEC_ID,	(XInterruptHandler)XTmrCtr_InterruptHandler,(void *)&timer);
	if (Status != XST_SUCCESS) return XST_FAILURE;
	Status = XIntc_Connect(&xintc, XPAR_INTC_0_GPIO_0_VEC_ID,(XInterruptHandler)Btn_isr,(void *)&btn);
	if (Status != XST_SUCCESS) return XST_FAILURE;

	Status = XIntc_Start(&xintc, XIN_REAL_MODE);
	if (Status != XST_SUCCESS) return XST_FAILURE;


	XGpio_InterruptGlobalEnable(&btn);

	XIntc_Enable(&xintc,XPAR_INTC_0_GPIO_0_VEC_ID);
	XIntc_Enable(&xintc, XPAR_INTC_0_TMRCTR_0_VEC_ID);

	Xil_ExceptionInit();
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler) XIntc_InterruptHandler, &xintc);
	Xil_ExceptionEnable();

	return XST_SUCCESS;
}


int Timer_init(){
	int Status = XTmrCtr_Initialize(&timer,XPAR_TMRCTR_0_DEVICE_ID);
	if (Status != XST_SUCCESS) return XST_FAILURE;
	XTmrCtr_SetHandler(&timer,Timer_isr,&timer);
	XTmrCtr_SetOptions(&timer,0,XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);
	XTmrCtr_SetResetValue(&timer,0,0xFFFFFFFF-12500);//12500 pour 1ms
	return XST_SUCCESS;
}

int main()
{
    init_platform();

    print("Hello World\n\r");

    int Status=XGpio_Initialize(&btn, XPAR_GPIO_0_DEVICE_ID);
    if (Status != XST_SUCCESS) return XST_FAILURE;
    XGpio_SetDataDirection(&btn,1,0xFF);

    if(Timer_init()==XST_FAILURE){
    	print("Timer init error\n\r");
    }else{print("Timer init successful\n\r");}

    if(Interrupt_init()==XST_FAILURE){
        print("Interrupt init error\n\r");
    }else{print("Interrupt init successful\n\r");}

    XTmrCtr_Start(&timer,0);

    while(1){};//tempo car les switch ne sont pu actualis�s apr�s le cleanup_platform()

    cleanup_platform();
    return 0;
}

