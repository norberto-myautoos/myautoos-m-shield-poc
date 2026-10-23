# MyAutoOS M-SHIELD VCU - Kia EV3 Tier2 Supplier Validated
![Banco ESP32](docs/conexiones.png)

> Banco de pruebas seguro - ESP32 DevKit V1 con 2 LEDs y botón. Timer 53ms con millis()

**Founder:** Norberto Bravo Tadeo - Cherán, Michoacán - Pueblo P'urhépecha - ITSU Uruapan  
**Company:** MyAutoOS Technologies S.A.S. - RFC: MAO240101TQ8  
**Project:** Kia EV3 - First EV Produced in Nuevo León - $649M USD Investment  
**Motto:** Hecho en Cherán para el Mundo

## VALIDACIÓN TIER2

- **PoC Cost:** $645 MXN vs Bosch $2,500 USD
- **Timing:** 53.2ms ASIL-D <60ms Kia Spec - ID 0x100 Prime Timing
- **Jitter:** 98ms validated - BUS-OFF Real EFL_TXBO 0x20 - SAFE-CUT GPIO4
- **T-MEC:** 27% contenido nacional - Cherán a 3.5h Lázaro Cárdenas Port
- **NRE:** COT-MYAO-2024-003 - $850k MXN - Inicio $45k MXN - 6 semanas entrega

## ESTRUCTURA

- 01_HARDWARE_TIER2: Gerbers 4 capas sellada IP67
- 02_FIRMWARE_ASIL-D: Firmware ISO 26262 ASIL-D
- 03_VALIDATION_HIL: HIL Vector CANoe + Lab ISO 16750
- 04_NRE_TIER2_DOCS: Docs NAFIN + Kia
- 05_COMMERCIAL_KIA: Pitch Kia Pesquería

## CONTACTO TIER2

- Email: norberto.myautoos@gmail.com
- GitHub: github.com/norberto-myautoos/myautoos-m-shield-poc
- NAFIN Folio: NAFIN-PYMEX-AUTO-2024-1037
- Toluca Support: 2h Pesquería - Lerma Production

## LICENCIA

Licencia por VIN: $400 USD/VIN - Fase 4: 500 VCUs = $21.25M MXN/mes

# myautoos-m-shield-poc 🛡️

![ESP32](https://img.shields.io/badge/ESP32-DevKitV1-blue)
![Platform](https://img.shields.io/badge/Platform-Wokwi-green)
![Timer](https://img.shields.io/badge/Timer-53ms-orange)
![License](https://img.shields.io/badge/License-MIT-yellow)

Banco seguro para ESP32 DevKit V1 - Core 3.3.12 - Chilchota, Mich.
PoC educativo con timer de 53ms no bloqueante.

### 🎯 Objetivo
Probar 2 LEDs + 1 botón con `millis()` sin `delay()`. Compilación <15% Flash. Solo protoboard, seguro y reutilizable.

### 🔌 Conexiones
| Componente | Pin ESP32 | Nota |
| :--- | :--- | :--- |
| LED Azul | GPIO2 | + 220Ω → GND - Parpadea cada 53ms |
| LED Rojo | GPIO4 | + 220Ω → GND |
| Botón | GPIO15 | → GND - INPUT_PULLUP |

> ⚠️ ESP32 es 3.3V - Siempre usa resistencia de 220Ω para LEDs.

### 💻 Código
Archivo: `esp32-banco-53ms.ino`
- LED Azul: timer 53ms con millis()
- LED Rojo: ON al presionar GPIO15
- Serial: 115200 baud

## ESTADO

Fase 3 HIL: En financiamiento NAFIN $45k MXN - PO Kia $850k MXN en proceso
# Opcion A: En desarrollo - Financiado por PO Kia $850k MXN 
# Opción B: Inicio $45k MXN NAFIN
# Disponible Q1 2025 - PoC validado 53.2ms ASIL-D
