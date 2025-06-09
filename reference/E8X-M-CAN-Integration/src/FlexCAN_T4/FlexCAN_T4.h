#ifndef _FLEXCAN_T4_H_
#define _FLEXCAN_T4_H_

#include <stdint.h>
#include "Arduino.h"
#include "circular_buffer.h"
#include "imxrt_flexcan.h"

// CAN device table
typedef enum CAN_DEV_TABLE {
  CAN0 = (uint32_t)0x0,
  CAN1 = (uint32_t)0x401D0000,
  CAN2 = (uint32_t)0x401D4000,
  CAN3 = (uint32_t)0x401D8000
} CAN_DEV_TABLE;

// Queue size definitions
typedef enum FLEXCAN_RXQUEUE_TABLE {
  RX_SIZE_16 = (uint16_t)16,
  RX_SIZE_32 = (uint16_t)32,
  RX_SIZE_64 = (uint16_t)64,
  RX_SIZE_128 = (uint16_t)128
} FLEXCAN_RXQUEUE_TABLE;

typedef enum FLEXCAN_TXQUEUE_TABLE {
  TX_SIZE_16 = (uint16_t)16,
  TX_SIZE_32 = (uint16_t)32,
  TX_SIZE_64 = (uint16_t)64,
  TX_SIZE_128 = (uint16_t)128
} FLEXCAN_TXQUEUE_TABLE;

// CAN message structure
typedef struct CAN_message_t {
  uint32_t id = 0;          // can identifier
  uint16_t timestamp = 0;   // FlexCAN time when message arrived
  uint8_t idhit = 0;       // filter that id came from
  struct {
    bool extended = 0;     // identifier is extended (29-bit)
    bool remote = 0;      // remote transmission request packet type
    bool overrun = 0;     // message overrun
    bool reserved = 0;
  } flags;
  uint8_t len = 8;        // length of data
  uint8_t buf[8] = { 0 }; // data
  int8_t mb = 0;         // used to identify mailbox reception
  uint8_t bus = 0;       // used to identify where the message came from
  bool seq = 0;          // sequential frames
} CAN_message_t;

// FlexCAN class template
template<CAN_DEV_TABLE _bus = CAN1, FLEXCAN_RXQUEUE_TABLE _rxSize = RX_SIZE_16, FLEXCAN_TXQUEUE_TABLE _txSize = TX_SIZE_16>
class FlexCAN_T4 {
public:
  FlexCAN_T4() {}
  void begin();
  void setBaudRate(uint32_t baud);
  void enableFIFO();
  void enableFIFOInterrupt();
  void onReceive(void (*callback)(const CAN_message_t &msg));
  void mailboxStatus();
  bool write(const CAN_message_t &msg);
  bool read(CAN_message_t &msg);

private:
  void (*_rx_callback)(const CAN_message_t &msg) = nullptr;
};

#endif 