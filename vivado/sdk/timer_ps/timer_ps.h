/************************************************************************/
/*	@file timer_ps.h													*/
/*	@created 01/01/24													*/
/************************************************************************/
#ifndef TIMER_PS_H_
#define TIMER_PS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "xil_types.h"
#include "xparameters.h"

/* ------------------------------------------------------------ */
/*					Miscellaneous Declarations					*/
/* ------------------------------------------------------------ */

#define TIMER_FREQ_HZ (XPAR_CPU_CORTEXA9_0_CPU_CLK_FREQ_HZ / 2)

/* ------------------------------------------------------------ */
/*					Procedure Declarations						*/
/* ------------------------------------------------------------ */

int TimerInitialize(u16 TimerDeviceId);
void TimerDelay(u32 uSDelay);

#ifdef __cplusplus
}
#endif

#endif /* TIMER_H_ */
