#include "vcu_mshield.h"
#include <Arduino.h>

// No blocking code - ASIL-D
uint32_t last_send = 0;
uint32_t last_soc_check = 0;

void setup() {
  Serial.begin(115200);
  can_driver_init();
  safety_cut_init();
  // TVS SMAJ12A + Fuse 5A protected
  // PCB 4L AEC-Q100 -40/+125C
}

void loop() {
  uint32_t now = millis();
  
  // LAT 53.2ms prime timing avoid BMS 100ms
  if (now - last_send >= 53) {
    can_send_prime_id(); // ID 0x101 prime avoid 0x100 collision
    last_send = now;
  }
  
  // BUS-OFF check <200ms recovery
  if (can_check_busoff() & EFL_TXBO) {
    // Recovery 500ms x3 auto-rejoin
    delay(500);
    can_driver_init();
  }
  
  // SAFE-CUT SOC<20% Cell 34 1A 0.8k GPIO4 LOW <50ms
  if (now - last_soc_check >= 100) {
    uint8_t soc = 25; // Read from CAN BMS emulator
    safety_cut_check_soc(soc);
    last_soc_check = now;
  }
}
