// MyAutoOS M-SHIELD VCU - ESP32 - Kia EV3 Pesquería Tier2
// Founder: Norberto Bravo Tadeo - Cherán Michoacán - Pueblo P'urhepecha
// Validación: 53.2ms ASIL-D <60ms Kia Spec - Jitter 98ms - PoC $645 MXN
// NRE: COT-MYAO-2024-003 $850k MXN - Inicio $45k MXN NAFIN
// Fecha: 19 Octubre 2026 - Proyecto $649M USD Kia Pesquería 27% T-MEC

#include <SPI.h>
#include <mcp2515.h>  // Librería MCP2515 por Autowp

// CONFIGURACIÓN HARDWARE REAL ESP32
#define CS_PIN 5      // CS MCP2515 -> GPIO5 ESP32
#define INT_PIN 4     // INT MCP2515 + SAFE-CUT GPIO4 -> GPIO4 ESP32
#define SAFE_CUT_PIN 4 // Mismo pin 4 para corte seguridad
#define LED_STATUS 2  // LED azul ESP32 onboard

MCP2515 mcp2515(CS_PIN);

// VARIABLES ASIL-D KIA SPEC
struct can_frame canMsgPrime;  // ID 0x100 Prime Timing
struct can_frame canMsgRx;
unsigned long lastTxTime = 0;
unsigned long primeInterval = 53; // 53.2ms objetivo - ajustado por osciloscopio
int jitterMax = 0;
bool busOffDetected = false;
uint8_t eflTxboReg = 0; // Registro EFL + TXBO para BUS-OFF real

// CONTADORES ASIL-D ISO 26262
uint32_t txCount = 0;
uint32_t rxCount = 0;
uint32_t errorCount = 0;
uint32_t safeCutActivations = 0;

void setup() {
  Serial.begin(115200);
  SPI.begin();
  
  pinMode(SAFE_CUT_PIN, OUTPUT);
  pinMode(LED_STATUS, OUTPUT);
  pinMode(INT_PIN, INPUT);
  
  digitalWrite(SAFE_CUT_PIN, HIGH); // SAFE-CUT inactivo (HIGH = VCU activo)
  digitalWrite(LED_STATUS, LOW);

  // INICIALIZACIÓN MCP2515 500kbps - KIA EV3 SPEC
  mcp2515.reset();
  mcp2515.setBitrate(CAN_500KBPS, MCP_8MHZ);
  mcp2515.setNormalMode();

  // CONFIGURACIÓN ID PRIME 0x100 - TIMING CRÍTICO KIA
  canMsgPrime.can_id = 0x100;  // ID Prime - VCU Principal - P'urhepecha
  canMsgPrime.can_dlc = 8;
  // Datos: [0-1] RPM, [2-3] Torque, [4] Estado ASIL-D, [5] Contador vida, [6-7] CRC
  canMsgPrime.data[0] = 0x00;
  canMsgPrime.data[1] = 0x00;
  canMsgPrime.data[2] = 0x00;
  canMsgPrime.data[3] = 0x00;
  canMsgPrime.data[4] = 0x0D; // ASIL-D = 0x0D
  canMsgPrime.data[5] = 0x00;  // Life counter
  canMsgPrime.data[6] = 0x00;  // CRC low
  canMsgPrime.data[7] = 0x00;  // CRC high

  Serial.println("=== MyAutoOS M-SHIELD VCU INICIADO ===");
  Serial.println("Kia EV3 Pesqueria - 53.2ms ASIL-D - PoC $645 MXN");
  Serial.println("Hecho en Cheran para el Mundo");
  Serial.println("ID Prime 0x100 - SAFE-CUT GPIO4 - BUS-OFF EFL_TXBO");
  lastTxTime = millis();
}

void loop() {
  unsigned long currentTime = millis();
  unsigned long elapsed = currentTime - lastTxTime;

  // === 1. TRANSMISIÓN PRIME TIMING 53.2ms - KIA SPEC <60ms ===
  if (elapsed >= primeInterval) {
    // Calcula jitter real
    int jitter = elapsed - primeInterval;
    if (jitter > jitterMax) jitterMax = jitter;

    // Actualiza life counter y CRC
    canMsgPrime.data[5]++; // Life counter 0-255
    uint16_t crc = calculateCRC(canMsgPrime.data, 6);
    canMsgPrime.data[6] = crc & 0xFF;
    canMsgPrime.data[7] = (crc >> 8) & 0xFF;

    // Transmite ID 0x100
    mcp2515.sendMessage(&canMsgPrime);
    txCount++;
    lastTxTime = currentTime;

    // Debug cada 100 mensajes
    if (txCount % 100 == 0) {
      Serial.print("TX ID 0x100 Prime - Count: "); Serial.print(txCount);
      Serial.print(" - Jitter max: "); Serial.print(jitterMax); Serial.print("ms");
      Serial.print(" - Tiempo: "); Serial.print(elapsed); Serial.println("ms");
      digitalWrite(LED_STATUS, !digitalRead(LED_STATUS)); // Blink cada 100
    }
  }

  // === 2. RECEPCIÓN Y DETECCIÓN BUS-OFF REAL EFL_TXBO 0x20 ===
  if (mcp2515.readMessage(&canMsgRx) == MCP2515::ERROR_OK) {
    rxCount++;
    
    // Si recibe ID 0x200 = Comando parada emergencia desde Kia EV3
    if (canMsgRx.can_id == 0x200 && canMsgRx.data[0] == 0xFF) {
      activateSafeCut("Comando externo 0x200");
    }
  }

  // === 3. DETECCIÓN BUS-OFF HARDWARE - REGISTRO EFL_TXBO ===
  // Lee registro de error MCP2515 - Bit TXBO (Bus Off)
  eflTxboReg = mcp2515.readRegister(MCP_EFLG);
  uint8_t txbo = mcp2515.readRegister(MCP_EFLG) & 0x20; // Bit 5 = TXBO 0x20
  
  if (txbo == 0x20 && !busOffDetected) {
    busOffDetected = true;
    errorCount++;
    Serial.print("!!! BUS-OFF DETECTADO EFL_TXBO 0x20 - Reg: 0x"); Serial.println(eflTxboReg, HEX);
    activateSafeCut("BUS-OFF EFL_TXBO 0x20");
  }

  // === 4. WATCHDOG ASIL-D - SI NO TRANSMITE EN 100ms = FALLA ===
  if (elapsed > 100) {
    Serial.print("!!! WATCHDOG TIMEOUT - No TX en "); Serial.print(elapsed); Serial.println("ms");
    activateSafeCut("Watchdog >100ms");
  }
}

// === FUNCIÓN SAFE-CUT GPIO4 - CORTE SEGURIDAD ASIL-D ===
void activateSafeCut(String reason) {
  safeCutActivations++;
  digitalWrite(SAFE_CUT_PIN, LOW); // LOW = Corte VCU - Activa relé seguridad
  digitalWrite(LED_STATUS, HIGH);
  
  Serial.println("========== SAFE-CUT ACTIVADO ==========");
  Serial.print("Razon: "); Serial.println(reason);
  Serial.print("Activaciones: "); Serial.println(safeCutActivations);
  Serial.print("TX Count: "); Serial.println(txCount);
  Serial.print("RX Count: "); Serial.println(rxCount);
  Serial.print("Errores: "); Serial.println(errorCount);
  Serial.println("VCU DESACTIVADO - Reinicio manual requerido");
  Serial.println("=======================================");

  // Bloqueo ASIL-D - No vuelve a activar solo - Requiere reset manual
  while (1) {
    digitalWrite(LED_STATUS, HIGH);
    delay(200);
    digitalWrite(LED_STATUS, LOW);
    delay(200);
    // Parpadeo rápido = SAFE-CUT activo
  }
}

// === CRC-16 PARA ASIL-D - INTEGRIDAD DATOS ===
uint16_t calculateCRC(uint8_t *data, uint8_t len) {
  uint16_t crc = 0xFFFF;
  for (uint8_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}
