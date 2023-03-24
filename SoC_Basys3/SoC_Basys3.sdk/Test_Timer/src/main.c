#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "xparameters.h"
#include "xtmrctr.h"
#include "xintc.h"
#include "sleep.h"

XTmrCtr timer;
XIntc xintc;

int Interrupt_init(){
	int Status = XIntc_Initialize(&xintc, XPAR_INTC_0_DEVICE_ID);
	if (Status != XST_SUCCESS) return XST_FAILURE;

	Status = XIntc_Connect(&xintc, XPAR_INTC_0_TMRCTR_0_VEC_ID,	(XInterruptHandler)XTmrCtr_InterruptHandler,(void *)&timer);
	if (Status != XST_SUCCESS) return XST_FAILURE;

	Status = XIntc_Start(&xintc, XIN_REAL_MODE);
	if (Status != XST_SUCCESS) return XST_FAILURE;

	XIntc_Enable(&xintc, XPAR_INTC_0_TMRCTR_0_VEC_ID);

	Xil_ExceptionInit();

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler) XIntc_InterruptHandler, &xintc);

	Xil_ExceptionEnable();

	return XST_SUCCESS;
}

void Timer_isr(void *CallBackRef, u8 TmrCtrNumber){
	volatile uint16_t * ptrLed = (uint16_t *)(XPAR_AXI_GPIO_1_BASEADDR + 0x0008);
	*(ptrLed+0x0002)=0;
	static uint8_t cnt=0;
	if(cnt==0){
		*ptrLed=0xFFFF;
		cnt=1;
	}
	else{
		*ptrLed=0x0000;
		cnt=0;
	}

}

int Timer_init(){
	int Status = XTmrCtr_Initialize(&timer,XPAR_TMRCTR_0_DEVICE_ID);
	if (Status != XST_SUCCESS) return XST_FAILURE;
	XTmrCtr_SetHandler(&timer,Timer_isr,&timer);
	XTmrCtr_SetOptions(&timer,0,XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);
	XTmrCtr_SetResetValue(&timer,0,0xFFFFFFFF-100000000/8);
}

int main()
{
    init_platform();

    print("Hello World\n\r");

    if(Timer_init()==XST_FAILURE){
    	print("Timer init error\n\r");
    }else{print("Timer init successful\n\r");}

    if(Interrupt_init()==XST_FAILURE){
        print("Interrupt init error\n\r");
    }else{print("Interrupt init successful\n\r");}

    XTmrCtr_Start(&timer,0);

    cleanup_platform();
    return 0;
}
