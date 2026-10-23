# DFMEA - MyAutoOS M-SHIELD VCU - KIA EV3
**Part:** MYAUTOOS-M-SHIELD-V1-18790-02850 - 20 VCUs HIL

| Item | Failure Mode | Effect | SEV | Cause | OCC | Control | DET | RPN | Action HIL |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| CAN BUS | BUS-OFF EFL_TXBO 0x20 | VCU offline | 9 | Colision ID 0x100 vs BMS 100ms | 4 | LAT 53.2ms prime timing avoid collision | 2 | 72 | CANoe 10min Log 500Kbps |
| CAN BUS | BUS-OFF no recovery | VCU stuck | 9 | No recovery | 3 | Recovery 500ms x3 auto-rejoin <200ms | 2 | 54 | Test real MCP2515 REG |
| JITTER | Jitter >150ms | BMS desync | 8 | Timer bloqueante | 4 | Timer no bloqueante 98ms avg <150ms | 2 | 64 | CANoe histogram |
| SAFE-CUT | SOC<20% no cut | Sobre-descarga Cell 34 | 9 | Logica SOC no leida | 3 | SAFE-CUT Cell 34 1A 0.8k GPIO4 LOW <50ms | 2 | 54 | Test Cell 34 1A real |
| SAFE-CUT | Cut falso SOC>20% | Paro inesperado | 8 | Ruido CAN SOC | 3 | Filtro ID SOC + debounce 3 ciclos + OTA | 3 | 72 | Log 10min sin falso cut |
| WIRING | C5/C6 invertido | CAN short | 7 | Arnes mal ponchado | 2 | Color C5 Yellow / C6 W/Yellow + continuidad | 2 | 28 | Blueprint REV B TADEO |
| WIRING | SCK/MISO/MOSI cruzado | MCP2515 no SPI | 7 | Breadboard error | 3 | Orange/Green/Blue SCK18=18 MISO19=19 MOSI23=23 | 2 | 42 | Test SPI loopback |
| POWER | TVS fail 12V spike | VCU quemado | 9 | Load dump | 2 | SMAJ12A + Fuse 5A + PCB 4L AEC-Q100 | 2 | 36 | Test ISO 7637 |
| OTA | OTA brick | VCU no boot | 8 | Flash corrupt | 2 | Dual partition + rollback + GPIO4 safe | 2 | 32 | OTA 10 ciclos |
