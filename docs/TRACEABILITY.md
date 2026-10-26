# Trazabilidad requisito → código → prueba

Estado de cada prueba: **Host/SIL** = pasa en esta repo (simulación). **HIL** = evidencia en banco real: **pendiente** en todos los casos; enlazar aquí el log y su método cuando exista.

## Núcleo (`core/`)

| REQ | Requisito | Código | Prueba | Host | HIL |
|---|---|---|---|---|---|
| REQ-01 | Bus-Off (EFLG TXBO 0x20), reintento cada 500 ms, máx. 3, sin reset si estabiliza | `mshield_core.c: busoff_step`, `mcp2515.c` | T-BO-1, T-BO-2 | OK | pendiente |
| REQ-02 | TX 0x101 sincronizado con el ciclo del BMS | `tx_step` | T-TX-1, T-TX-2 | OK | pendiente |
| REQ-03 | SAFE-CUT con SOC < 20 % y celda 34 | `safecut_step` | T-SC-1, T-SC-2 | OK | pendiente |
| REQ-04 | GPIO4 activo en BAJO | `GPIO4_CUT_LEVEL` | T-SC-1 | OK (lógica) | pendiente (nivel eléctrico, estado en reset) |
| REQ-05 | OTA seguro | `mshield_ota_allowed` (solo compuerta) | T-OTA-1 | OK (compuerta) | firma y rollback: no implementado |

## Simulación (`sim/`)

| FSR | Prueba (banco SIL) | Estado SIL | HIL |
|---|---|---|---|
| FSR-01 | TEST_CASE_03 | OK | pendiente |
| FSR-02 | TEST_CASE_05 | OK | pendiente |
| FSR-03 | TEST_CASE_01, TEST_CASE_06 | OK | pendiente |
| FSR-04 | TEST_CASE_04 (detenido), TEST_CASE_07 (en marcha) | OK | pendiente |
| FSR-05 | TEST_CASE_02 | OK | pendiente |
| FSR-06 | TEST_CASE_08 | OK | pendiente |
| FSR-07 | TEST_CASE_11 | OK en simulación; falta hardware independiente | pendiente |
| FSR-08 | TEST_CASE_09, TEST_CASE_10 | OK (descarga modelada) | pendiente |

## Fuera de alcance de este repositorio
HARA/FMEDA validados, safety case, métricas SPFM/LFM/PMHF, cobertura MC/DC, análisis estático con perfil MISRA acordado, proceso ASPICE, ciberseguridad (ISO 21434).
