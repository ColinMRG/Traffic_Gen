#include <stdio.h>
#include "platform.h"
#include "xil_printf.h"
#include "ps7_init.h"
#include <xil_io.h>
#include "xscugic.h"
#include "xparameters.h"
#include "addressparams.h"
#include "xaxidma_hw.h"
#include "xil_cache.h"

u32 dmabuf[NUM_OF_WORDS];
XScuGic InterruptController;
static XScuGic_Config *GicConfig;
unsigned int frame_count = 0;

void InterruptHandler(void)
{
	//xil_printf("Interrupt triggered\n\r");
	// Clear the interrupt
	Xil_Out32(XPAR_AXI_DMA_0_BASEADDR+OFFSET_S2MM_DMASR, Xil_In32(XPAR_AXI_DMA_0_BASEADDR+OFFSET_S2MM_DMASR) | 0x1000);
	if (++frame_count>FRAME_COUNT_MAX) return;

	//Reprogram DMA transfer parameters
	Xil_Out32(XPAR_AXI_DMA_0_BASEADDR+OFFSET_S2MMDA, OFFSET_MEM_WRITE+BUF_LEN*frame_count);
	Xil_Out32(XPAR_AXI_DMA_0_BASEADDR+OFFSET_S2MM_LENGTH, BUF_LEN);
}

int SetupInterruptSystem(XScuGic *xScuGicInstancePtr)
{
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler, xScuGicInstancePtr);
	Xil_ExceptionEnable();
	return XST_SUCCESS;
}

int interrupt(){
	int status = XScuGic_CfgInitialize(&InterruptController, GicConfig, GicConfig -> CpuBaseAddress);
	if (status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}
	status = SetupInterruptSystem(&InterruptController);
	if (status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}
	status = XScuGic_Connect(&InterruptController, XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR, (Xil_ExceptionHandler)InterruptHandler, NULL);
	if (status != XST_SUCCESS)
	{
		return XST_FAILURE;
	}
	XScuGic_Enable(&InterruptController, XPAR_FABRIC_AXI_DMA_0_S2MM_INTROUT_INTR);
	return XST_SUCCESS;
}

u32 *dma()
{

    	// Initialize DMA (Set bits 0 and 12 of the DMA control register)
    	Xil_Out32(XPAR_AXI_DMA_0_BASEADDR + OFFSET_S2MM_DMACR, Xil_In32(XPAR_AXI_DMA_0_BASEADDR + OFFSET_S2MM_DMACR) | 0x1001);

    	//Interrupt system and interrupt handling
    	GicConfig = XScuGic_LookupConfig(XPAR_PS7_SCUGIC_0_DEVICE_ID);
    	if (NULL == GicConfig)
    	{
    		return NULL;
    	}

    	memset(dmabuf, 0, sizeof(dmabuf));
    	Xil_DCacheFlush();
		Xil_DCacheInvalidate();
    	//Program DMA transfer parameters (i) destination address (ii) length
		Xil_Out32(XPAR_AXI_DMA_0_BASEADDR+OFFSET_S2MMDA, (u32)dmabuf);
		Xil_Out32(XPAR_AXI_DMA_0_BASEADDR+OFFSET_S2MM_LENGTH, BUF_LEN);


		Xil_Out32(XPAR_TRAFFICGEN_0_S00_AXI_BASEADDR, 1);
		Xil_Out32(XPAR_TRAFFICGEN_0_S00_AXI_BASEADDR+0x4, NUM_OF_WORDS);

		return dmabuf;

}

#define XAXIDMA_SR_IDLE_MASK 0x00000002

int wait_for_dma_done()
{
	u32 status;
	while (1) {
		status = Xil_In32(XPAR_AXI_DMA_0_BASEADDR + XAXIDMA_RX_OFFSET + XAXIDMA_SR_OFFSET);
		int idle = status & XAXIDMA_SR_IDLE_MASK;
		if (idle)
			break;
	}
	return 0;
}

int dma_recvd_length()
{
	int i;
	for (i = 0; i < sizeof(dmabuf)/sizeof(u32); i++) {
		if (dmabuf[i] == 0)
			break;
	}
	xil_printf("first zero in dmabuf at %d\r\n", i);
	u32 word = Xil_In32(XPAR_AXI_DMA_0_BASEADDR + XAXIDMA_RX_OFFSET + XAXIDMA_BUFFLEN_OFFSET);
	int length = word & 0x000FFFFF;
	return length;
}
