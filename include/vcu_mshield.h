#ifndef VCU_MSHIELD_H
#define VCU_MSHIELD_H
#include <stdint.h>

// PINOUT KIA EV3 18790-02850 - Validated
#define PIN_CAN_CS      5
#define PIN_CAN_INT     4  // GPIO4 SAFE-CUT Active LOW
#define PIN_SPI_SCK     18 // Orange
#define PIN_SPI_MISO    19 // Green  
#define PIN_SPI_MOSI    23 // Blue
#define PIN_CAN_C5      21 // Yellow CAN_H
#define PIN_CAN_C6      22 // W/Yellow CAN_L
#define PIN_POWER_A12   12 // 12V + TVS SMAJ12A
#define PIN_GND_A1      1

// TIMING VALIDATED CANoe 10min
#define LATENCY_PRIME_MS    53.2f
#define JITTER_AVG_MS       98.0f
#define JITTER_LIMIT_MS     150.0f
#define CAN_ID_AVOID_BMS    0x100
#define CAN_BMS_ID          0x100
#define CAN_BITRATE         500000
#define SPI_FREQ            8000000

// SAFE-CUT
#define SOC_CUT_THRESHOLD   20
#define CELL_34_CURRENT     1.0f // 1A
#define CELL_34_RESISTOR    0.8f // 0.8k
#define SAFE_CUT_TIME_MS    50
#define BUSOFF_RECOVERY_MS  500
#define BUSOFF_RETRY        3

// MCP2515 REGISTERS
#define REG_CANINTF         0x2C
#define EFL_TXBO            0x20

void can_driver_init(void);
uint8_t can_check_busoff(void);
void safety_cut_init(void);
void safety_cut_check_soc(uint8_t soc);
void can_send_prime_id(void);

#endif
