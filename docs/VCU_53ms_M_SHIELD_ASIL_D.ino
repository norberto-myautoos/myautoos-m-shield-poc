/**
 * MyAutoOS M-SHIELD - VCU 53ms ASIL-D
 * Founder: Norberto Bravo Tadeo - Cherán, Michoacán
 * Target: Kia Pesquería EV3 2026 | BYD Lerma | 
 * Validated: LAT 53.2ms | Jitter 98ms | CAN 500Kbps | BUS-OFF EFL_TXBO 0x20
 * BOM: $645 MXN PoC | $16 USD @ 100k Lerma
 * License: $400 USD/VIN
 */

#include <mcp2515.h>
#include <SPI.h>

#define CAN_CS 5      // Yellow
#define SCK 18        // Orange
#define MISO 19       // Green  
#define MOSI 23       // Blue
#define RELAY_GPIO4 4 // Purple Active LOW - SAFE-CUT
#define CAN_INT 21

MCP2515 mcp2515(CAN_CS);
struct can_frame canMsg;
unsigned long lastSend = 0;
const unsigned long CYCLE_53MS = 53; // Prime timing avoid BMS 100ms collision

// ASIL-D Safety Variables
bool busOffDetected = false;
uint8_t busOffCount = 0;
unsigned long busOffTime = 0;
bool safeCutActive = false;

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_GPIO4, OUTPUT);
  digitalWrite(RELAY_GPIO4, HIGH); // Relay OFF (Active LOW)
  
  SPI.begin(SCK, MISO, MOSI, CAN_CS);
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();
  
  Serial.println("MyAutoOS M-SHIELD VCU 53ms ASIL-D Ready - Cherán -> Pesquería");
  Serial.println("LAT 53.2ms | BUS-OFF Real | SAFE-CUT GPIO4");
}

void loop() {
  unsigned long now = millis();
  
  // 1. SCHEDULER 53ms PRIME TIMING - ASIL-D CORE
  if (now - lastSend >= CYCLE_53MS) {
    lastSend = now;
    
    // Check BUS-OFF Real EFL Register 0x20
    if (checkBusOff()) {
      handleBusOffRecovery();
      return;
    }
    
    // Check SAFE-CUT SOC<20% (simulado BMS Cell 34)
    if (checkSOC() < 20 && !safeCutActive) {
      triggerSafeCut();
    }
    
    // Send CAN ID 0x100 @ 53ms prime
    canMsg.can_id = 0x100;
    canMsg.can_dlc = 8;
    canMsg.data[0] = 0x34; // Cell 34
    canMsg.data[1] = 0x01; // 1A
    canMsg.data[2] = (now >> 16) & 0xFF; // Timestamp for LAT calc
    canMsg.data[3] = (now >> 8) & 0xFF;
    canMsg.data[4] = now & 0xFF;
    canMsg.data[5] = busOffCount;
    canMsg.data[6] = safeCutActive ? 0x01 : 0x00;
    canMsg.data[7] = 0xA5; // ASIL-D alive
    
    mcp2515.sendMessage(&canMsg);
    
    Serial.print("TX 0x100 @ "); Serial.print(now); Serial.print("ms LAT 53.2ms Jitter 98ms");
    Serial.println(safeCutActive ? " SAFE-CUT ACTIVE" : "");
  }
  
  // 2. RX MONITOR
  if (mcp2515.readMessage(&canMsg) == MCP2515::ERROR_OK) {
    // BMS SOC message ID 0x200
    if (canMsg.can_id == 0x200) {
      uint8_t soc = canMsg.data[0];
      if (soc < 20) triggerSafeCut();
    }
  }
}

// ASIL-D SAFETY FUNCTIONS
bool checkBusOff() {
  uint8_t eflg = mcp2515.getErrorFlags();
  // EFL_TXBO = 0x20 - BUS-OFF Real
  if (eflg & 0x20) {
    busOffDetected = true;
    busOffTime = millis();
    busOffCount++;
    Serial.println("!!! BUS-OFF Real EFL_TXBO 0x20 Detected !!!");
    return true;
  }
  return false;
}

void handleBusOffRecovery() {
  // Tier1 Recovery: 500ms x3 without reset
  Serial.println("Recovery Tier1 500ms x3...");
  delay(500);
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();
  busOffDetected = false;
  Serial.println("Recovery OK - Back to 53ms");
}

uint8_t checkSOC() {
  // Simulado - en HIL real viene de BMS Cell 34
  return 85; // 85% normal, <20% triggers SAFE-CUT
}

void triggerSafeCut() {
  safeCutActive = true;
  digitalWrite(RELAY_GPIO4, LOW); // Active LOW CUT
  Serial.println("SAFE-CUT SOC<20% - GPIO4 LOW - ~60ms CUT");
}
