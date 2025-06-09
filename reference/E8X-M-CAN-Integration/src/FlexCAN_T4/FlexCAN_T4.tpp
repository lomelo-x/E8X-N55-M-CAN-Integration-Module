#ifndef _FLEXCAN_T4_TPP_
#define _FLEXCAN_T4_TPP_

#include "FlexCAN_T4.h"

template<CAN_DEV_TABLE _bus, FLEXCAN_RXQUEUE_TABLE _rxSize, FLEXCAN_TXQUEUE_TABLE _txSize>
void FlexCAN_T4<_bus, _rxSize, _txSize>::begin() {
    // Enable CAN module clock
    FLEXCANb_MCR(_bus) &= ~FLEXCAN_MCR_MDIS;
    
    // Wait for clock to stabilize
    while (FLEXCANb_MCR(_bus) & FLEXCAN_MCR_LPMACK);
    
    // Soft reset
    FLEXCANb_MCR(_bus) |= FLEXCAN_MCR_SOFT_RST;
    while (FLEXCANb_MCR(_bus) & FLEXCAN_MCR_SOFT_RST);
    
    // Wait for reset to complete
    while (FLEXCANb_MCR(_bus) & FLEXCAN_MCR_SOFT_RST);
}

template<CAN_DEV_TABLE _bus, FLEXCAN_RXQUEUE_TABLE _rxSize, FLEXCAN_TXQUEUE_TABLE _txSize>
void FlexCAN_T4<_bus, _rxSize, _txSize>::setBaudRate(uint32_t baud) {
    // Enter freeze mode
    FLEXCANb_MCR(_bus) |= FLEXCAN_MCR_FRZ;
    FLEXCANb_MCR(_bus) |= FLEXCAN_MCR_HALT;
    while (!(FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK));
    
    // Configure baud rate
    uint32_t clockSpeed = 24000000; // 24MHz clock
    uint32_t prescaler = clockSpeed / baud / 20;
    
    FLEXCANb_CTRL1(_bus) = FLEXCAN_CTRL_PROPSEG(2) | 
                           FLEXCAN_CTRL_RJW(1) |
                           FLEXCAN_CTRL_PSEG1(7) |
                           FLEXCAN_CTRL_PSEG2(3) |
                           FLEXCAN_CTRL_PRESDIV(prescaler);
    
    // Exit freeze mode
    FLEXCANb_MCR(_bus) &= ~FLEXCAN_MCR_HALT;
    FLEXCANb_MCR(_bus) &= ~FLEXCAN_MCR_FRZ;
    while (FLEXCANb_MCR(_bus) & FLEXCAN_MCR_FRZ_ACK);
}

template<CAN_DEV_TABLE _bus, FLEXCAN_RXQUEUE_TABLE _rxSize, FLEXCAN_TXQUEUE_TABLE _txSize>
void FlexCAN_T4<_bus, _rxSize, _txSize>::enableFIFO() {
    FLEXCANb_MCR(_bus) |= FLEXCAN_MCR_FEN;
}

template<CAN_DEV_TABLE _bus, FLEXCAN_RXQUEUE_TABLE _rxSize, FLEXCAN_TXQUEUE_TABLE _txSize>
void FlexCAN_T4<_bus, _rxSize, _txSize>::enableFIFOInterrupt() {
    FLEXCANb_IMASK1(_bus) |= FLEXCAN_IMASK1_BUF5M;
}

template<CAN_DEV_TABLE _bus, FLEXCAN_RXQUEUE_TABLE _rxSize, FLEXCAN_TXQUEUE_TABLE _txSize>
void FlexCAN_T4<_bus, _rxSize, _txSize>::onReceive(void (*callback)(const CAN_message_t &msg)) {
    _rx_callback = callback;
}

template<CAN_DEV_TABLE _bus, FLEXCAN_RXQUEUE_TABLE _rxSize, FLEXCAN_TXQUEUE_TABLE _txSize>
void FlexCAN_T4<_bus, _rxSize, _txSize>::mailboxStatus() {
    // Read mailbox status (implementation specific)
}

template<CAN_DEV_TABLE _bus, FLEXCAN_RXQUEUE_TABLE _rxSize, FLEXCAN_TXQUEUE_TABLE _txSize>
bool FlexCAN_T4<_bus, _rxSize, _txSize>::write(const CAN_message_t &msg) {
    // Find free mailbox
    uint32_t mb;
    for (mb = 0; mb < 16; mb++) {
        if ((FLEXCANb_MCR(_bus) & FLEXCAN_MCR_MAXMB(mb)) == 0) break;
    }
    if (mb >= 16) return false;
    
    // Write message to mailbox
    volatile uint32_t *mbxAddr = &(FLEXCANb_MCR(_bus)) + 0x80 + (mb * 0x10);
    mbxAddr[0] = (msg.len << 16) | (msg.flags.extended ? 0x600000 : 0x400000);
    mbxAddr[1] = msg.id;
    mbxAddr[2] = (uint32_t)msg.buf[3] << 24 | (uint32_t)msg.buf[2] << 16 |
                 (uint32_t)msg.buf[1] << 8 | (uint32_t)msg.buf[0];
    mbxAddr[3] = (uint32_t)msg.buf[7] << 24 | (uint32_t)msg.buf[6] << 16 |
                 (uint32_t)msg.buf[5] << 8 | (uint32_t)msg.buf[4];
    
    return true;
}

template<CAN_DEV_TABLE _bus, FLEXCAN_RXQUEUE_TABLE _rxSize, FLEXCAN_TXQUEUE_TABLE _txSize>
bool FlexCAN_T4<_bus, _rxSize, _txSize>::read(CAN_message_t &msg) {
    // Check if message available in FIFO
    if (!(FLEXCANb_IFLAG1(_bus) & FLEXCAN_IFLAG1_BUF5I)) return false;
    
    // Read message from FIFO
    volatile uint32_t *mbxAddr = &(FLEXCANb_MCR(_bus)) + 0x80;
    uint32_t code = mbxAddr[0];
    msg.len = (code >> 16) & 0xF;
    msg.flags.extended = (code & 0x600000) == 0x600000;
    msg.id = mbxAddr[1];
    uint32_t word0 = mbxAddr[2];
    uint32_t word1 = mbxAddr[3];
    
    msg.buf[0] = word0 & 0xFF;
    msg.buf[1] = (word0 >> 8) & 0xFF;
    msg.buf[2] = (word0 >> 16) & 0xFF;
    msg.buf[3] = (word0 >> 24) & 0xFF;
    msg.buf[4] = word1 & 0xFF;
    msg.buf[5] = (word1 >> 8) & 0xFF;
    msg.buf[6] = (word1 >> 16) & 0xFF;
    msg.buf[7] = (word1 >> 24) & 0xFF;
    
    // Clear interrupt flag
    FLEXCANb_IFLAG1(_bus) = FLEXCAN_IFLAG1_BUF5I;
    
    // Call receive callback if registered
    if (_rx_callback) _rx_callback(msg);
    
    return true;
}

#endif 