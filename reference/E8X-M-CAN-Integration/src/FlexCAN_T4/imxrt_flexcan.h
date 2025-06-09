#ifndef __IMXRT_FLEXCAN_H
#define __IMXRT_FLEXCAN_H

#include <stdint.h>

// FlexCAN module base addresses
#define FLEXCAN1_BASE      (0x401D0000)
#define FLEXCAN2_BASE      (0x401D4000)

// Register definitions
#define FLEXCANb_MCR(b)           (*(volatile uint32_t*)(b))
#define FLEXCANb_CTRL1(b)         (*(volatile uint32_t*)(b+4))
#define FLEXCANb_TIMER(b)         (*(volatile uint32_t*)(b+8))
#define FLEXCANb_RXMGMASK(b)      (*(volatile uint32_t*)(b+0x10))
#define FLEXCANb_RX14MASK(b)      (*(volatile uint32_t*)(b+0x14))
#define FLEXCANb_RX15MASK(b)      (*(volatile uint32_t*)(b+0x18))
#define FLEXCANb_ECR(b)           (*(volatile uint32_t*)(b+0x1C))
#define FLEXCANb_ESR1(b)          (*(volatile uint32_t*)(b+0x20))
#define FLEXCANb_IMASK2(b)        (*(volatile uint32_t*)(b+0x24))
#define FLEXCANb_IMASK1(b)        (*(volatile uint32_t*)(b+0x28))
#define FLEXCANb_IFLAG2(b)        (*(volatile uint32_t*)(b+0x2C))
#define FLEXCANb_IFLAG1(b)        (*(volatile uint32_t*)(b+0x30))
#define FLEXCANb_CTRL2(b)         (*(volatile uint32_t*)(b+0x34))
#define FLEXCANb_ESR2(b)          (*(volatile uint32_t*)(b+0x38))

// Bit definitions for Control Register
#define FLEXCAN_CTRL_PROPSEG(x)   (((x)&0x07)<<0)
#define FLEXCAN_CTRL_LOM          (0x08)
#define FLEXCAN_CTRL_LBUF         (0x10)
#define FLEXCAN_CTRL_TSYNC        (0x20)
#define FLEXCAN_CTRL_BOFF_REC     (0x40)
#define FLEXCAN_CTRL_SMP          (0x80)
#define FLEXCAN_CTRL_RWRN_MSK     (0x400)
#define FLEXCAN_CTRL_TWRN_MSK     (0x800)
#define FLEXCAN_CTRL_LPB          (0x1000)
#define FLEXCAN_CTRL_CLK_SRC      (0x2000)
#define FLEXCAN_CTRL_ERR_MSK      (0x4000)
#define FLEXCAN_CTRL_BOFF_MSK     (0x8000)
#define FLEXCAN_CTRL_PSEG2(x)     (((x)&0x07)<<16)
#define FLEXCAN_CTRL_PSEG1(x)     (((x)&0x07)<<19)
#define FLEXCAN_CTRL_RJW(x)       (((x)&0x03)<<22)
#define FLEXCAN_CTRL_PRESDIV(x)   (((x)&0xFF)<<24)

// Bit definitions for MCR Register
#define FLEXCAN_MCR_MAXMB(x)      ((x)&0x7F)
#define FLEXCAN_MCR_IDAM(x)       (((x)&0x03)<<8)
#define FLEXCAN_MCR_AEN           (0x1000)
#define FLEXCAN_MCR_LPRIO_EN      (0x2000)
#define FLEXCAN_MCR_IRMQ          (0x10000)
#define FLEXCAN_MCR_SRX_DIS       (0x20000)
#define FLEXCAN_MCR_DOZE          (0x40000)
#define FLEXCAN_MCR_WAK_MSK       (0x80000)
#define FLEXCAN_MCR_SOFT_RST      (0x2000000)
#define FLEXCAN_MCR_FRZ           (0x40000000)
#define FLEXCAN_MCR_MDIS          (0x80000000)

#endif 