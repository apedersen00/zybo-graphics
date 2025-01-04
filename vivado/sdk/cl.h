/************************************************************************/
/*	@file cl.h        												    */
/*	@created 30/12/24													*/
/************************************************************************/

#ifndef CL_H_
#define CL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "xil_types.h"

/* GPU Commands */
#define GPU_STOP_CMD   0x0000
#define BLIT_RECT_CMD  0x0001
#define SET_CLIP_CMD   0x0002

#define NUM_OF_CL   2
#define CL_SIZE     256

/* Structs */
struct cl_type {
    u32 idx;
    u32 *array;
};

struct cl_type** init_cl(void);

void clear_cl(struct cl_type *cl);

void rewind_cl(struct cl_type *cl);

void draw_rect(struct cl_type *cl, u16 x, u16 y, u16 w, u16 h, u32 color);

void set_clip(struct cl_type *cl, u16 x, u16 y, u16 w, u16 h);

void draw_stop(struct cl_type *cl);

#ifdef __cplusplus
}
#endif

#endif /* CL_H_ */
