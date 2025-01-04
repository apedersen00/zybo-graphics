/************************************************************************/
/*	@file cl.cpp        												*/
/*	@created 30/12/24													*/
/************************************************************************/

/* Includes */
#include "xil_types.h"
#include "cl.h"
#include "xgpu.h"
#include "xil_cache.h"

static u32 cl[NUM_OF_CL][CL_SIZE] __attribute__((aligned(0x20)));
static struct cl_type cl_instances[NUM_OF_CL];
static struct cl_type* cl_instances_ptr[NUM_OF_CL];

struct cl_type** init_cl(void)
{
    /* Zero initialize the CL arrays */
	int i;
    for (i = 0; i < NUM_OF_CL; i++) {
        memset(cl[i], 0, CL_SIZE * sizeof(u32));
    }

    /* Initialize CL structures */
    for (i = 0; i< NUM_OF_CL; i++) {
        cl_instances[i].idx = 0;
        cl_instances[i].array = cl[i];
        cl_instances_ptr[i] = &cl_instances[i];
    }

    return cl_instances_ptr;
}

void clear_cl(struct cl_type *cl)
{
    memset(cl->array, 0, CL_SIZE * sizeof(u32));
    cl->idx = 0;
}

void rewind_cl(struct cl_type *cl)
{
    cl->idx = 0;
}

void draw_rect(struct cl_type *cl, u16 x, u16 y, u16 w, u16 h, u32 color)
{
    cl->array[cl->idx++] = ((x & 0xFFFF) << 16) | (BLIT_RECT_CMD & 0xFFFF);
    cl->array[cl->idx++] = ((w & 0xFFFF) << 16) | (y & 0xFFFF);
    cl->array[cl->idx++] = ((0 & 0xFFFF) << 16) | (h & 0xFFFF);
    cl->array[cl->idx++] = color;
    Xil_DCacheFlushRange((UINTPTR)cl, 256);
}

void set_clip(struct cl_type *cl, u16 x, u16 y, u16 w, u16 h)
{
    cl->array[cl->idx++] = ((x & 0xFFFF) << 16) | (SET_CLIP_CMD & 0xFFFF);
    cl->array[cl->idx++] = ((w & 0xFFFF) << 16) | (y & 0xFFFF);
    cl->array[cl->idx++] = ((0 & 0xFFFF) << 16) | (h & 0xFFFF);
    cl->array[cl->idx++] = 0x00000000;
    Xil_DCacheFlushRange((UINTPTR)cl, 256);
}

void draw_stop(struct cl_type *cl)
{
    cl->array[cl->idx++] = ((0 & 0xFFFF) << 16) | (GPU_STOP_CMD & 0xFFFF);
    cl->array[cl->idx++] = 0;
    cl->array[cl->idx++] = 0;
    cl->array[cl->idx++] = 0;
    Xil_DCacheFlushRange((UINTPTR)cl, 256);
}
