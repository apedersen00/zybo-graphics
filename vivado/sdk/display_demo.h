/************************************************************************/
/*	@file cl.h        												    */
/*	@created 30/12/24													*/
/************************************************************************/

#ifndef DISPLAY_DEMO_H_
#define DISPLAY_DEMO_H_

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------ */
/*				Include File Definitions						*/
/* ------------------------------------------------------------ */

#include "xil_types.h"

/* ------------------------------------------------------------ */
/*					Miscellaneous Declarations					*/
/* ------------------------------------------------------------ */

#define DEMO_PATTERN_0 0
#define DEMO_PATTERN_1 1

#define DEMO_MAX_FRAME (1920*1080*4)
#define DEMO_STRIDE (1920 * 4)

#include "xscutimer.h"
#include "xscugic.h"
#include "xil_exception.h"
#include "xgpio.h"

#define MILLISECOND 325000

/* ------------------------------------------------------------ */
/*					Procedure Declarations						*/
/* ------------------------------------------------------------ */

int main(void);
void InitGPU();
void GPU_BindFrameBuffer(u32 frameBufferAddr);
void GPU_BindCommandList(u32 commandListAddr);
void GPU_Start();
void GPU_WaitForDone();
void gpu_draw(u8 *frame, int frameIndex, u32 move);
void Error_Handler(const char* caller);
void PrintStartup();
u32 RainbowRGB();
int GpioInit(XGpio *GpioInstancePtr, u16 DeviceId);
u8 read_gpio(void);

int ScuTimerInit(XScuTimer *TimerInstancePtr, u16 TimerDeviceId, u32 TimerCounter);
int ScuIntrInit(XScuGic *IntcInstancePtr, u16 GicDeviceId);
int TimerSetupIntr(XScuGic *IntcInstancePtr, XScuTimer *TimerInstancePtr, u16 TimerIntrId);

void TimerStart(XScuTimer *TimerInstancePtr);
void TimerStop(XScuTimer *TimerInstancePtr);
void TimerLoad(XScuTimer *TimerInstancePtr, u32 TimerCounter);
void TimerReinitialize(XScuTimer *TimerInstancePtr, u32 TimerCounter, void (*TimerFunction)(void));

static void TimerIntrHandler(void *CallBackRef);
static void TimerDisableIntrSystem(XScuGic *IntcInstancePtr, u16 TimerIntrId);
static void VtcFrameSyncCallback(void *CallbackRef, u32 Mask);

#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_DEMO_H_ */
