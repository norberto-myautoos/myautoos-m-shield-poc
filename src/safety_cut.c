#include "vcu_mshield.h"
#include <Arduino.h>

void safety_cut_init(void) {
  pinMode(PIN_CAN_INT, OUTPUT); // GPIO4 SAFE-CUT
  digitalWrite(PIN_CAN_INT, HIGH); // Active LOW
}

void safety_cut_check_soc(uint8_t soc) {
  if (soc < SOC_CUT_THRESHOLD) {
    // Cell 34 1A 0.8k cut <50ms
    digitalWrite(PIN_CAN_INT, LOW); // SAFE-CUT
    // OTA config dual partition safe
  }
}
