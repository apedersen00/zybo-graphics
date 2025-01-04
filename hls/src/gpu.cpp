#include <stdio.h>
#include <string.h>
#include <ap_int.h>

/* Debug */
#define DEBUG_GPU

/* Defines */
#define FB_SIZE 1920*1080

/* FIFO */
#define CMD_FIFO_WRITE_ADDR     0x00
#define CMD_FIFO_STATUS_ADDR    0x04
#define FIFO_DEPTH              256
#define DATA_WIDTH              32

/* Commands */
#define CMD_NONE        0x0000
#define BLIT_RECT_CMD   0x0001
#define SET_CLIP_CMD    0x0002

/* Prototypes */
void gpu_blit_rect(
    ap_uint<32> frameBuffer[FB_SIZE],
    ap_uint<16> arg0,
    ap_uint<16> arg1,
    ap_uint<16> arg2,
    ap_uint<16> arg3,
    ap_uint<16> arg4,
    ap_uint<16> arg5,
    ap_uint<16> arg6
    );

/**
 * The clipping rectangle is stored in static variables.
 * Initialized to full screen by default.
 */
static ap_uint<16> clip_x = 0;
static ap_uint<16> clip_y = 0;
static ap_uint<16> clip_w = 1920;
static ap_uint<16> clip_h = 1080;

void gpu_set_clip(
    ap_uint<16> arg0,
    ap_uint<16> arg1,
    ap_uint<16> arg2,
    ap_uint<16> arg3
    )
{
#pragma HLS INLINE off
    clip_x = arg0;
    clip_y = arg1;
    clip_w = arg2;
    clip_h = arg3;
}

/**
 * The GPU function processes commands from the command FIFO.
 * 
 * @param fb_addr The address of the framebuffer
 * @param status The status flag to enable/disable processing
 * @param cmd_fifo The command FIFO
 * 
 * During runtime the CPU will set the address of the framebuffer to be drawn.
 * The CPU will calculate the scene and send the commands to the GPU.
 * The status flag will enable/disable processing.
 * 
 * The CMD FIFO has a depth of 256 with each entry being 32 bits.
 * A single command takes a fixed size of 4 entries i.e. 128 bits.
 */
void gpu(ap_uint<32> frameBuffer[FB_SIZE], ap_uint<8> status, ap_uint<32> cl[256])
{
#pragma HLS INTERFACE s_axilite port=return bundle=control
#pragma HLS INTERFACE s_axilite port=status bundle=control
#pragma HLS INTERFACE m_axi port=frameBuffer offset=slave
#pragma HLS INTERFACE m_axi port=cl offset=slave

    // Check if processing is enabled via the status flag
    if (status & 1) {
        // Create a buffer to store the commands
        static ap_uint<32> rowBuffer[256];
        #pragma HLS RESOURCE variable=rowBuffer core=RAM_2P_BRAM
        /* Burst read */
        READ_ROW: for (int i = 0; i < 256; i++) {
        #pragma HLS PIPELINE II=1
            rowBuffer[i] = cl[i];
        }

        // Process commands from the FIFO
        for (int i = 0; i < 256/4; i++) {
        #pragma HLS PIPELINE off
            ap_uint<32> dword_0 = rowBuffer[i*4];
            ap_uint<32> dword_1 = rowBuffer[i*4+1];
            ap_uint<32> dword_2 = rowBuffer[i*4+2];
            ap_uint<32> dword_3 = rowBuffer[i*4+3];
            ap_uint<16> cmd  = dword_0 & 0xFFFF;    // Extract command type
            ap_uint<16> arg0 = dword_0 >> 16;       // Extract arguments
            ap_uint<16> arg1 = dword_1 & 0xFFFF;
            ap_uint<16> arg2 = dword_1 >> 16;
            ap_uint<16> arg3 = dword_2 & 0xFFFF;
            ap_uint<16> arg4 = dword_2 >> 16;
            ap_uint<16> arg5 = dword_3 & 0xFFFF;
            ap_uint<16> arg6 = dword_3 >> 16;

            if (cmd == CMD_NONE) {
                break;
            }

            // Handle the command
            switch (cmd) {
                case BLIT_RECT_CMD:
                    std::cout << "Processing BLIT_RECT_CMD" << std::endl;
                    gpu_blit_rect(frameBuffer, arg0, arg1, arg2, arg3, arg4, arg5, arg6);
                    break;
                case SET_CLIP_CMD:
                    std::cout << "Processing SET_CLIP_CMD" << std::endl;
                    gpu_set_clip(arg0, arg1, arg2, arg3);
                    break;
                default:
                    std::cout << "Unknown command: " << cmd << std::endl;
                    break;
            }
        }
    }
    status = 0;
}

void gpu_blit_rect(
    ap_uint<32> frameBuffer[FB_SIZE],
    ap_uint<16> arg0,
    ap_uint<16> arg1,
    ap_uint<16> arg2,
    ap_uint<16> arg3,
    ap_uint<16> arg4,
    ap_uint<16> arg5,
    ap_uint<16> arg6
    )
{
#pragma HLS INLINE off

    /* Parse arguments */
    ap_uint<16> x = arg0;
    ap_uint<16> y = arg1;
    ap_uint<16> w = arg2;
    ap_uint<16> h = arg3;
    ap_uint<32> color = (ap_uint<32>(arg6) << 16) | arg5;

    /* Extract color channels for blending */
    ap_uint<8> newA = color.range(31, 24);
    ap_uint<8> newR = color.range(23, 16);
    ap_uint<8> newG = color.range(15, 8);
    ap_uint<8> newB = color.range(7, 0);

#ifdef DEBUG_GPU
    std::cout << "newB: " << std::hex << newB << std::endl;
    std::cout << "newG: " << std::hex << newG << std::endl;
    std::cout << "newR: " << std::hex << newR << std::endl;
    std::cout << "newA: " << std::hex << newA << std::endl;
#endif

    /* Apply clipping */
    int start_x = (x < clip_x) ? (int)clip_x : (int)x;
    int start_y = (y < clip_y) ? (int)clip_y : (int)y;
    int end_x   = ((int)x + (int)w > (int)clip_x + (int)clip_w) ? (int)(clip_x + clip_w) : (int)(x + w);
    int end_y   = ((int)y + (int)h > (int)clip_y + (int)clip_h) ? (int)(clip_y + clip_h) : (int)(y + h);

    /* If the clipped region is empty, do nothing */
    if (end_x <= start_x || end_y <= start_y) {
        return;
    }

    static ap_uint<32> rowBuffer[1920];
    #pragma HLS RESOURCE variable=rowBuffer core=RAM_2P_BRAM

    int clipped_w = end_x - start_x;
    int clipped_h = end_y - start_y;

    for (int row = 0; row < clipped_h; row++) {
    #pragma HLS LOOP_TRIPCOUNT max=1080
        int y_idx = start_y + row;
        int row_offset = y_idx * 1920 + start_x;

        //=============
        // BURST READ
        //=============
        READ_ROW: for (int col = 0; col < clipped_w; col++) {
        #pragma HLS PIPELINE II=1
            rowBuffer[col] = frameBuffer[row_offset + col];
        }

        //=============
        // BLEND
        //=============
        BLEND_ROW: for (int col = 0; col < clipped_w; col++) {
        #pragma HLS PIPELINE II=1
            ap_uint<32> oldColor = rowBuffer[col];

            ap_uint<8> oldA = oldColor.range(31, 24);
            ap_uint<8> oldR = oldColor.range(23, 16);
            ap_uint<8> oldG = oldColor.range(15, 8);
            ap_uint<8> oldB = oldColor.range(7, 0);

            // If alpha == 100%: overwrite
            if (newA == 255) {
                rowBuffer[col] = color;
            }
            // If alpha == 0%: keep old color
            else if (newA == 0) {
                // do nothing
            }
            // Blend otherwise
            else {
                ap_uint<16> outB_16 = (((255 - newA) * oldB) + (newA * newB)) >> 8;
                ap_uint<16> outG_16 = (((255 - newA) * oldG) + (newA * newG)) >> 8;
                ap_uint<16> outR_16 = (((255 - newA) * oldR) + (newA * newR)) >> 8;

                // Simple alpha strategy: max or other logic
                ap_uint<8> outA = (oldA > newA) ? oldA : newA;

                ap_uint<32> outColor = 
                      (ap_uint<32>(outA) << 24)
                    | (ap_uint<32>(outR_16) << 16)
                    | (ap_uint<32>(outG_16) <<  8)
                    | (ap_uint<32>(outB_16) <<  0);

                rowBuffer[col] = outColor;
            }
        }

        //=============
        // BURST WRITE
        //=============
        WRITE_ROW: for (int col = 0; col < clipped_w; col++) {
        #pragma HLS PIPELINE II=1
            frameBuffer[row_offset + col] = rowBuffer[col];
        }
    }
}
