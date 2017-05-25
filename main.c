/******************************************************************************
*
* Copyright (C) 2009 - 2014 Xilinx, Inc.  All rights reserved.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* Use of the Software is limited solely to applications:
* (a) running on a Xilinx device, or
* (b) that interact with a Xilinx device through a bus or interconnect.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
* XILINX  BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
* OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* Except as contained in this notice, the name of the Xilinx shall not be used
* in advertising or otherwise to promote the sale, use or other dealings in
* this Software without prior written authorization from Xilinx.
*
******************************************************************************/

/*
 * helloworld.c: simple test application
 *
 * This application configures UART 16550 to baud rate 9600.
 * PS7 UART (Zynq) is not initialized by this application, since
 * bootrom/bsp configures it to baud rate 115200
 *
 * ------------------------------------------------
 * | UART TYPE   BAUD RATE                        |
 * ------------------------------------------------
 *   uartns550   9600
 *   uartlite    Configurable only in HW design
 *   ps7_uart    115200 (configured by bootrom/bsp)
 */

#include <stdio.h>
#include <math.h>
#include "platform.h"
#include "xil_printf.h"
#include "xaxidma.h"
#include "xtmrctr.h"
#include "xparameters.h"
#include "stdlib.h"

#define ARRAY_SIZE 10
#define SIZE ARRAY_SIZE

int init_dma(XAxiDma* AxiDma){

	XAxiDma_Config *CfgPtr;

	int status;

	CfgPtr = XAxiDma_LookupConfig(XPAR_AXI_DMA_0_DEVICE_ID);
	if(!CfgPtr){
		print("Error looking for AXI DMA config\n");
		return XST_FAILURE;
	}

	status = XAxiDma_CfgInitialize(AxiDma,CfgPtr);
	if(status != XST_SUCCESS){
		print("Error initializing DMA\n");
		return XST_FAILURE;
	}

	/* Disable interrupts, we use polling mode */
	XAxiDma_IntrDisable(AxiDma, XAXIDMA_IRQ_ALL_MASK,XAXIDMA_DEVICE_TO_DMA);
	XAxiDma_IntrDisable(AxiDma, XAXIDMA_IRQ_ALL_MASK,XAXIDMA_DMA_TO_DEVICE);

	return XST_SUCCESS;
}

int main()
{
 	int i;

 	float A[ARRAY_SIZE];

 	float res_hw;
 	float res_sw;


	XStatus status; // return value of the calls to the memory-mapped devices



    float accel_factor; // speed-up

	unsigned int dma_size = SIZE * sizeof(float);

	XTmrCtr AxiTimer;
	XAxiDma AxiDma;

	unsigned int begin_time, end_time;
	unsigned int run_time_sw, run_time_hw;
	unsigned int time_A;


	init_platform();

	//Init DMA
	status = init_dma(&AxiDma);

	if(status != XST_SUCCESS){
		print("Error: DMA init failed\n");
		return XST_FAILURE;
	}

	// Setup timer
	status = XTmrCtr_Initialize(&AxiTimer, XPAR_AXI_TIMER_0_DEVICE_ID);

	if(status != XST_SUCCESS){
		print("Error: timer setup failed\n");
	}

	XTmrCtr_SetOptions(&AxiTimer, XPAR_AXI_TIMER_0_DEVICE_ID, XTC_ENABLE_ALL_OPTION);

	// input data; Matrix Initiation
	for(i = 0; i<ARRAY_SIZE; i++)
			//A[i] = rand ();
			A[i] = i;

	res_sw = 0;
	res_hw = 0;

	// flush the data caches to update the memory before DMA
	Xil_DCacheFlushRange((unsigned int)A,dma_size);


	// call the software version of the matrix multiplication
	xil_printf("Starting the matrix multiplication on the ARM PS ...\n\n");

	// reset the timer
	XTmrCtr_Reset(&AxiTimer, XPAR_AXI_TIMER_0_DEVICE_ID);

	// capture the start time
	begin_time = XTmrCtr_GetValue(&AxiTimer, XPAR_AXI_TIMER_0_DEVICE_ID);

	// matrix multiplication in software
	for (i=0; i< ARRAY_SIZE; ++i) {
		res_sw += A[i];
	}
	//matrix_multiply_SW(A, B, res_sw);

	// capture end time
	end_time = XTmrCtr_GetValue(&AxiTimer, XPAR_AXI_TIMER_0_DEVICE_ID);

	// software runtime
	run_time_sw = end_time - begin_time;

	xil_printf("Runtime for SW on ARM core is %d cycles.\n", run_time_sw);

	XTmrCtr_Reset(&AxiTimer, XPAR_AXI_TIMER_0_DEVICE_ID);

	begin_time = XTmrCtr_GetValue(&AxiTimer, XPAR_AXI_TIMER_0_DEVICE_ID);

	// pointer to the matrix multiplier IP
	int* accelCtrl = XPAR_FP_ADDER_0_S00_AXI_BASEADDR;

	xil_printf("start to transfer array\n");
	// DMA transfers matrix A to the accelerator
	status = XAxiDma_SimpleTransfer(&AxiDma, (unsigned int) A, dma_size, XAXIDMA_DMA_TO_DEVICE);

	if (status != XST_SUCCESS) {
		xil_printf("Error: DMA transfer matrix A to accelerator failed\n");
		return XST_FAILURE;
	}

	// Wait for transfer of matrix A
	while (XAxiDma_Busy(&AxiDma, XAXIDMA_DMA_TO_DEVICE)) ;
	xil_printf("done\n");
	time_A = XTmrCtr_GetValue(&AxiTimer, XPAR_AXI_TIMER_0_DEVICE_ID);

	// start the accelerator
	*accelCtrl = 1;

	// invalidate the data cache to receive updated values from the memory after DMA
	Xil_DCacheInvalidateRange((float)res_hw,sizeof(float));
	xil_printf("wait for result from hw\n");
	//get results from the accelerator
	status = XAxiDma_SimpleTransfer(&AxiDma, (float) res_hw, sizeof(float), XAXIDMA_DEVICE_TO_DMA);

	if (status != XST_SUCCESS) {
		xil_printf("Error: DMA transfer from accelerator failed\n");
		return XST_FAILURE;
	}

	// Wait for the result matrix
	while (XAxiDma_Busy(&AxiDma, XAXIDMA_DEVICE_TO_DMA)) ;
	xil_printf("done\n");
	end_time = XTmrCtr_GetValue(&AxiTimer, XPAR_AXI_TIMER_0_DEVICE_ID);

	run_time_hw = end_time - begin_time;

	xil_printf("Runtime for DMA + HW accelerator is %d cycles.\n\n", run_time_hw);

	// speed up
	accel_factor = (float) run_time_sw / (float) run_time_hw;
	xil_printf("Speedup: %d.%d \n\n", (int) accel_factor, (int) (accel_factor * 1000) % 1000);

	xil_printf("Cycles spent for matrix A: %d cycles\n", time_A - begin_time);
	xil_printf("Cycles waiting for the results: %d cycles\n\n", end_time - time_A);

	//Compare the results from sw and hw
	xil_printf ("res_sw = %d ", res_sw);
	xil_printf ("res_hw = %d\n\n",res_hw);

	cleanup_platform();
    return 0;
}
