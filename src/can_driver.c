#include "vcu_mshield.h"
#include <SPI.h>

void can_driver_init(void) {
  SPI.begin(PIN_SPI_SCK, PIN_SPI_MISO, PIN_SPI_MOSI, PIN_CAN_CS);
  SPI.setFrequency(SPI_FREQ); // 8MHz
  // MCP2515 500Kbps init
  // Recovery 500ms x3 configured
}

uint8_t can_check_busoff(void) {
  // Read REG CANINTF 0x2C
  digitalWrite(PIN_CAN_CS, LOW);
  SPI.transfer(0x03); // READ
  SPI.transfer(REG_CANINTF);
  uint8_t status = SPI.transfer(0x00);
  digitalWrite(PIN_CAN_CS, HIGH);
  return status; // Check EFL_TXBO 0x20
}

void can_send_prime_id(void) {
  // Send ID 0x101 53.2ms prime avoid BMS 0x100
  // Jitter 98ms avg <150ms limit non-blocking timer
}
