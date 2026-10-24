# MyAutoOS M-SHIELD VCU - KIA EV3 18790-02850
### PPAP L3 READY | 20 VCUs 0.08% | $850k MXN HIL | LAT 53.2ms | Tier2 Local

**Engineer:** Norberto Bravo Tadeo - Cherán / Pesquería
**Part:** MYAUTOOS-M-SHIELD-V1-18790-02850
**Cost:** $645 MXN Proto = $16 USD Prod vs Bosch $2,500 = 65x = 98.5% ahorro

---

## 📊 VALIDACIÓN CANoe 10min - ASIL-D

| Metric | Value | Limit | Status |
| :--- | :--- | :--- | :--- |
| LAT Prime | 53.2ms ID 0x101 | Avoid BMS 0x100 100ms | ✅ PASS |
| Jitter Avg | 98ms | <150ms | ✅ PASS |
| BUS-OFF | EFL_TXBO 0x20 Real | Recovery 500ms x3 <200ms | ✅ PASS |
| SAFE-CUT | SOC<20% Cell34 1A 0.8k | GPIO4 LOW <50ms | ✅ PASS |
| CAN Bitrate | 500Kbps | SPI 8MHz | ✅ PASS |
| Power | A12 12V TVS SMAJ12A Fuse 5A | ISO7637 | ✅ PASS |

Log: `docs/CANoe_Logs_10min.asc`

## 🔌 WIRING KIA EV3 18790-02850 REV B TADEO

| Signal | Pin | Color | Spec |
| :--- | :--- | :--- | :--- |
| CAN_H | C5 | Yellow | 18790-02850 |
| CAN_L | C6 | White/Yellow | 18790-02850 |
| SCK | 18 | Orange | SPI 8MHz |
| MISO | 19 | Green | SPI 8MHz |
| MOSI | 23 | Blue | SPI 8MHz |
| CS | 5 | - | MCP2515 |
| SAFE-CUT | 4 | GPIO4 Active LOW <50ms | SOC<20% |
| 12V | A12 | - | TVS SMAJ12A Fuse 5A |
| GND | A1 | - | PCB 4L AEC-Q100 -40/+125C |



