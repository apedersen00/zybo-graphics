// ==============================================================
// File generated by Vivado(TM) HLS - High-Level Synthesis from C, C++ and SystemC
// Version: 2017.2
// Copyright (C) 1986-2017 Xilinx, Inc. All Rights Reserved.
// 
// ==============================================================

// control
// 0x00 : Control signals
//        bit 0  - ap_start (Read/Write/COH)
//        bit 1  - ap_done (Read/COR)
//        bit 2  - ap_idle (Read)
//        bit 3  - ap_ready (Read)
//        bit 7  - auto_restart (Read/Write)
//        others - reserved
// 0x04 : Global Interrupt Enable Register
//        bit 0  - Global Interrupt Enable (Read/Write)
//        others - reserved
// 0x08 : IP Interrupt Enable Register (Read/Write)
//        bit 0  - Channel 0 (ap_done)
//        bit 1  - Channel 1 (ap_ready)
//        others - reserved
// 0x0c : IP Interrupt Status Register (Read/TOW)
//        bit 0  - Channel 0 (ap_done)
//        bit 1  - Channel 1 (ap_ready)
//        others - reserved
// 0x10 : Data signal of frameBuffer_V
//        bit 31~0 - frameBuffer_V[31:0] (Read/Write)
// 0x14 : reserved
// 0x18 : Data signal of status_V
//        bit 7~0 - status_V[7:0] (Read/Write)
//        others  - reserved
// 0x1c : reserved
// 0x20 : Data signal of cl_V
//        bit 31~0 - cl_V[31:0] (Read/Write)
// 0x24 : reserved
// (SC = Self Clear, COR = Clear on Read, TOW = Toggle on Write, COH = Clear on Handshake)

#define XGPU_CONTROL_ADDR_AP_CTRL            0x00
#define XGPU_CONTROL_ADDR_GIE                0x04
#define XGPU_CONTROL_ADDR_IER                0x08
#define XGPU_CONTROL_ADDR_ISR                0x0c
#define XGPU_CONTROL_ADDR_FRAMEBUFFER_V_DATA 0x10
#define XGPU_CONTROL_BITS_FRAMEBUFFER_V_DATA 32
#define XGPU_CONTROL_ADDR_STATUS_V_DATA      0x18
#define XGPU_CONTROL_BITS_STATUS_V_DATA      8
#define XGPU_CONTROL_ADDR_CL_V_DATA          0x20
#define XGPU_CONTROL_BITS_CL_V_DATA          32

