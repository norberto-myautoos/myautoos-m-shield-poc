# CONTROL PLAN - MyAutoOS M-SHIELD VCU - FASE 2 HIL 20 VCUs
**Supplier:** Norberto Bravo Tadeo - Cherán, Michoacán - Toluca 2h to Pesquería
**Part:** MYAUTOOS-M-SHIELD-V1-18790-02850 - KIA EV3
**Qty:** 20 VCUs = 0.08% planta = $850k MXN

| # | Process | Characteristic | Spec | Method | Sample | Control | Reaction |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| 10 | PCB Fab Lerma | 4L AEC-Q100 -40/+125°C | IPC-A-600 | Visual + Micro | 20/20 | Supplier cert | Scrap + reorder |
| 20 | SMD Assembly | ESP32-S3 EYE $285 + MCP2515 $125 | Placement +/-0.1mm | AOI | 20/20 | First article | Rework Cherán |
| 30 | Wiring Harness | C5 Yellow CAN_H / C6 W/Y CAN_L | 18790-02850 | Continuity + Color | 20/20 | Blueprint REV B TADEO | Re-crimp |
| 40 | Wiring SPI | SCK18 Orange / MISO19 Green / MOSI23 Blue / CS5 | SPI 8MHz | Loopback test | 20/20 | Color code + pinout | Re-wire |
| 50 | Power | A12 12V + TVS SMAJ12A + Fuse 5A / A1 GND | 12V +/-1V <50ms cut | Surge test ISO7637 | 20/20 | Multimeter + Scope | Replace TVS |
| 60 | Flash Firmware | LAT 53.2ms ID 0x100 avoid BMS 100ms | 53.2ms prime timing | CANoe timestamp | 20/20 | CANoe 10min Log | Re-flash OTA |
| 70 | Jitter Test | Jitter avg 98ms <150ms limit | <150ms | CANoe histogram | 20/20 | Non-blocking timer | Optimize timer ISR |
| 80 | BUS-OFF Test | EFL_TXBO 0x20 Real + Recovery 500ms x3 | <200ms recovery | MCP2515 REG CANINTF | 20/20 | Real fault injection | Firmware patch |
| 90 | SAFE-CUT Test | SOC<20% Cell 34 1A 0.8k + GPIO4 Active LOW | <50ms cut | BMS emulator + Scope | 20/20 | Cell 34 1A real | GPIO4 LOW + OTA config |
| 100 | OTA Test | Dual partition + rollback | 10 cycles no brick | OTA flash + boot | 5/20 | Rollback test | Dual partition |
| 110 | Final HIL | CAN 500Kbps 8MHz HEV/KIA/BMW/BYD | 500Kbps | dSPACE SCALEXIO Pesquería | 20/20 | 8 weeks onsite | Onsite 2h Toluca |
| 120 | Shipping | 20 VCUs boxed + docs | PPAP Level 3 | Visual + PSW | 1 lot | PSW signed | Reship |

**SPECIAL CHARACTERISTICS:**
- CC: SAFE-CUT <50ms SOC<20% / BUS-OFF <200ms / TVS Power
- SC: LAT 53.2ms / Jitter 98ms / Wiring colors

**REACTION PLAN T-MEC:** If failure 3am - Engineer on-site 2h Toluca vs Continental 10,000km ticket 3 days

**Cost $850k MXN:** $380k CANoe + $280k 8 weeks onsite + $120k PPAP docs + $70k Material 20 VCUs
