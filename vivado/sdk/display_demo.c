/************************************************************************/
/*	@file display_demo.c												*/
/*	@created 29/12/24													*/
/************************************************************************/

/* Includes */
#include "display_demo.h"
#include "display_ctrl/display_ctrl.h"
#include <stdio.h>
#include "xuartps.h"
#include "math.h"
#include <ctype.h>
#include <stdlib.h>
#include "xil_types.h"
#include "xil_cache.h"
#include "timer_ps/timer_ps.h"
#include "xparameters.h"
#include "xgpu.h"
#include "xgpio.h"
#include "xscutimer.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "xvtc.h"
#include "cl.h"
#include "xgpio.h"

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

#include "engine/pong.h"

static void prvAppTask( void *pvParameters );
static TaskHandle_t xAppTask;
static QueueHandle_t xQueue = NULL;
void swap_framebuffers(void);

/* XPAR redefines */
#define DYNCLK_BASEADDR     XPAR_AXI_DYNCLK_0_S_AXI_LITE_BASEADDR
#define VGA_VDMA_ID         XPAR_AXIVDMA_0_DEVICE_ID
#define DISP_VTC_ID         XPAR_VTC_0_DEVICE_ID
// #define VID_VTC_IRPT_ID     XPS_FPGA3_INT_ID
#define VID_VTC_IRPT_ID XPAR_FABRIC_V_TC_0_IRQ_INTR
#define VID_GPIO_IRPT_ID    XPS_FPGA4_INT_ID
#define SCU_TIMER_ID        XPAR_SCUTIMER_DEVICE_ID
#define UART_BASEADDR       XPAR_PS7_UART_1_BASEADDR
#define XGPU_DEVICE_ID      XPAR_XGPU_0_DEVICE_ID

/* Instances */
DisplayCtrl dispCtrl;
XAxiVdma vdma;
XGpu GpuInstance;
XScuTimer TimerInstance;	/* Cortex A9 Scu Private Timer Instance	*/
XScuGic IntcInstance;		/* Interrupt Controller Instance 		*/
XGpio PushInstance;			/* Push buttons Instance 				*/

extern bool upSignal1;
extern bool downSignal1;
extern bool upSignal2;
extern bool downSignal2;

/* Framebuffers */
u8  frameBuf[DISPLAY_NUM_FRAMES][DEMO_MAX_FRAME] __attribute__((aligned(0x20)));
u8 *pFrames[DISPLAY_NUM_FRAMES];

#define STILL 		0x00
#define MOVE_UP 	0x01
#define MOVE_DOWN 	0x02

char HWstring[15] = "Hello World";
struct cl_type** cls;

void VtcFrameSyncCallback(void *CallbackRef, u32 Mask)
{
	static u32 count = 0;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    XVtc_IntrClear(&dispCtrl.vtc, 0xFFFFFFFF);
	// xil_printf("Task start!\r\n");
	xQueueSendFromISR( xQueue, HWstring, &xHigherPriorityTaskWoken );
	portYIELD_FROM_ISR( xHigherPriorityTaskWoken );
    count += 1;
}

int main(void)
{
    int Status;
    XAxiVdma_Config *vdmaConfig;
    XGpu_Config 	*gpuConfig;

    /* Initialize CLs to all zeros */
    cls = init_cl();

    /* Initialize frame buffers to black */
    memset(frameBuf[0], 0xFF000000, sizeof(frameBuf[0]));
    memset(frameBuf[1], 0xFF000000, sizeof(frameBuf[1]));

    /* Pointers to both frame buffers and command lists  */
    int i;
    for (i = 0; i < DISPLAY_NUM_FRAMES; i++) {
        pFrames[i] = frameBuf[i];
    }

    /* Peripherals */
	ScuIntrInit(&IntcInstance, XPAR_SCUGIC_SINGLE_DEVICE_ID);
    GpioInit(&PushInstance, XPAR_AXI_GPIO_BTN_DEVICE_ID);

    vdmaConfig = XAxiVdma_LookupConfig(VGA_VDMA_ID);
    if (!vdmaConfig) {
        Error_Handler("XAxiVdma_LookupConfig");
    }

    Status = XAxiVdma_CfgInitialize(&vdma, vdmaConfig, vdmaConfig->BaseAddress);
    if (Status != XST_SUCCESS) {
        Error_Handler("XAxiVdma_CfgInitialize");
    }

    Status = DisplayInitialize(&dispCtrl, &vdma, DISP_VTC_ID, DYNCLK_BASEADDR, pFrames, DEMO_STRIDE);
    if (Status != XST_SUCCESS) {
        Error_Handler("DisplayInitialize");
    }

    Status = DisplayStart(&dispCtrl);
    if (Status != XST_SUCCESS) {
        Error_Handler("DisplayStart");
    }

    gpuConfig = XGpu_LookupConfig(XGPU_DEVICE_ID);
    if (gpuConfig == NULL) {
        Error_Handler("XGpu_LookupConfig");
    }

    Status = XGpu_CfgInitialize(&GpuInstance, gpuConfig);
    if (Status != XST_SUCCESS) {
        Error_Handler("XGpu_CfgInitialize");
    }

    PrintStartup();

    /* Register the application task */
    xTaskCreate(prvAppTask,
                ( const char * ) "AppTask",
                configMINIMAL_STACK_SIZE,
                NULL,
                tskIDLE_PRIORITY + 1,
                &xAppTask );

    /* Create queue for starting task from Vsync */
    xQueue = xQueueCreate(1, sizeof(HWstring));
    configASSERT( xQueue );

    /* Start FreeRTOS */
    vTaskStartScheduler();

    /* We should never get here */
    for ( ;; );
}

static void prvAppTask( void *pvParameters )
{
    /**
     * Connect the Vsync interrupt to the callback
     * Do this here since vTaskStartSchedular() resets interrupts
     */
    XScuGic_Connect(&IntcInstance, VID_VTC_IRPT_ID, (Xil_ExceptionHandler)VtcFrameSyncCallback, (void *)&dispCtrl.vtc);
    XScuGic_Enable(&IntcInstance, VID_VTC_IRPT_ID);
    Xil_ExceptionEnable();
    char Recdstring[15] = "";
    u8 move;

    initializeGame();

	for( ;; )
	{
        /* Wait for Vsync */
		xQueueReceive( 	xQueue,
						Recdstring,
						portMAX_DELAY );

        if (XGpu_IsIdle(&GpuInstance)) {
            /* Swap the framebuffers */
            swap_framebuffers();

            /* Read player input */
            u8 input = read_gpio();
            if (input & 0b00000001) {
                upSignal1 = 1;
                downSignal1 = 0;
            }
            if (input & 0b00000010) {
                upSignal1 = 0;
                downSignal1 = 1;
            }
            if (input & 0b00000100) {
                upSignal2 = 1;
                downSignal2 = 0;
            }
            if (input & 0b00001000) {
                upSignal2 = 0;
                downSignal2 = 1;
            }
            if (input == 0) {
                upSignal1 = 0;
                downSignal1 = 0;
                upSignal2 = 0;
                downSignal2 = 0;s16
            }
            /* Tick the game engine */
            updateGame();
        }
	}
}

u8 read_gpio()
{
    u32 data = XGpio_DiscreteRead(&PushInstance, 1);
    return (u8)data;
}

void swap_framebuffers()
{
    dispCtrl.curFrame = dispCtrl.curFrame == 0 ? 1 : 0;
    DisplayChangeFrame(&dispCtrl, dispCtrl.curFrame);
}

void GPU_BindFrameBuffer(u32 frameBufferAddr)
{
    XGpu_Set_frameBuffer_V(&GpuInstance, frameBufferAddr);
}

void GPU_BindCommandList(u32 commandListAddr)
{
    XGpu_Set_cl_V(&GpuInstance, commandListAddr);
}

void GPU_Start()
{
    XGpu_Set_status_V(&GpuInstance, 0xFFFFFFFF);
    XGpu_Start(&GpuInstance);
}

void GPU_WaitForDone()
{
    while (!XGpu_IsDone(&GpuInstance)) {}
}

u32 RainbowRGB()
{
    static u8 val = 0;
    static u8 r = 0;
    static u8 g = 0;
    static u8 b = 0;
    u8 speed = 32;

    switch (val) {
        case 0: // Red -> Yellow: Increase G
            if (g + speed >= 255) { g = 255; val = 1; } else { g += speed; }
            break;
        case 1: // Yellow -> Green: Decrease R
            if (r <= speed) { r = 0; val = 2; } else { r -= speed; }
            break;
        case 2: // Green -> Cyan: Increase B
            if (b + speed >= 255) { b = 255; val = 3; } else { b += speed; }
            break;
        case 3: // Cyan -> Blue: Decrease G
            if (g <= speed) { g = 0; val = 4; } else { g -= speed; }
            break;
        case 4: // Blue -> Magenta: Increase R
            if (r + speed >= 255) { r = 255; val = 5; } else { r += speed; }
            break;
        case 5: // Magenta -> Red: Decrease B
            if (b <= speed) { b = 0; val = 0; } else { b -= speed; }
            break;
        default:
            val = 0;
            break;
    }

    u32 color = (0xFF << 24) | (r << 16) | (g << 8) | b;

    return color;
}

void Error_Handler(const char* caller)
{
    if (caller != NULL) {
        xil_printf("Error occured during: %s\r\n", caller);
    }
    else {
    xil_printf("Error occurred.\r\n");
    }
    while (1) {}
}

void PrintStartup()
{
    xil_printf("\x1B[H"); 	// Set cursor to top left of terminal
    xil_printf("\x1B[2J"); 	// Clear terminal
    xil_printf("**************************************************\n\r");
    xil_printf("*                  ZYBO CONSOLE	                *\n\r");
    xil_printf("**************************************************\n\r");
    xil_printf("*Display Resolution: %28s*\n\r", dispCtrl.vMode.label);
    printf("*Display Pixel Clock Freq. (MHz): %15.3f*\n\r", dispCtrl.pxlFreq);
}

/*****************************************************************************/
/**
* Initializes the general interrupt controller.
*
* @param IntcInstancePtr is a pointer to the instance of XScuGic driver.
* @param GicDeviceId is the device Id of the XScuGic device.
*
* @return	XST_SUCCESS if successful, otherwise XST_FAILURE.
******************************************************************************/
int ScuIntrInit(XScuGic *IntcInstancePtr, u16 GicDeviceId)
{
	XScuGic_Config *IntcConfig;
	int Status;

	IntcConfig = XScuGic_LookupConfig(GicDeviceId);
	if (NULL == IntcConfig) {
		xil_printf("Interrupt controller lookup failed.");
		return XST_FAILURE;
	}

	Status = XScuGic_CfgInitialize(IntcInstancePtr, IntcConfig, IntcConfig->CpuBaseAddress);
	if (Status != XST_SUCCESS) {
		xil_printf("Interrupt controller initialization failed.");
		return XST_FAILURE;
	}

	Xil_ExceptionInit();

	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_IRQ_INT, (Xil_ExceptionHandler)XScuGic_InterruptHandler, IntcInstancePtr);

	return XST_SUCCESS;
}

int GpioInit(XGpio *GpioInstancePtr, u16 DeviceId)
{
	XGpio_Initialize(GpioInstancePtr, DeviceId);
	XGpio_SetDataDirection(GpioInstancePtr, 1, 0xffffffff);

	return XST_SUCCESS;
}
