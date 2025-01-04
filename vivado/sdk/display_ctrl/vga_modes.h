/************************************************************************/
/*	@file vga_modes.h													*/
/*	@created 01/01/24													*/
/************************************************************************/

#ifndef VGA_MODES_H_
#define VGA_MODES_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	char label[64]; // Label describing the resolution
	u32 width; 		// Width of the active video frame
	u32 height; 	// Height of the active video frame
	u32 hps; 		// Start time of Horizontal sync pulse, in pixel clocks (active width + H. front porch)
	u32 hpe; 		// End time of Horizontal sync pulse, in pixel clocks (active width + H. front porch + H. sync width)
	u32 hmax; 		// Total number of pixel clocks per line (active width + H. front porch + H. sync width + H. back porch) 
	u32 hpol; 		// hsync pulse polarity
	u32 vps; 		// Start time of Vertical sync pulse, in lines (active height + V. front porch)
	u32 vpe; 		// End time of Vertical sync pulse, in lines (active height + V. front porch + V. sync width)
	u32 vmax; 		// Total number of lines per frame (active height + V. front porch + V. sync width + V. back porch) 
	u32 vpol; 		// vsync pulse polarity
	double freq; 	// Pixel Clock frequency
} VideoMode;

static const VideoMode VMODE_1920x1080 = {
	"1920x1080@60Hz", // label
	1920,              // width
	1080,              // height
	2008,              // hps
	2052,              // hpe
	2199,              // hmax
	1,                 // hpol
	1084,              // vps
	1089,              // vpe
	1124,              // vmax
	1,                 // vpol
	148.5              // freq
};

#ifdef __cplusplus
}
#endif

#endif /* VGA_MODES_H_ */
