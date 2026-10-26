# MyAutoOS M-SHIELD

Firmware de referencia en C para una unidad de control de vehículo (VCU) eléctrico: gestión de fallas en bus CAN y una simulación de la lógica de control con pruebas automatizadas.

> **Estado: prueba de concepto y simulación.** No está certificado (ISO 26262, IATF 16949, PPAP), no ha sido aprobado por ningún fabricante y **no es apto para uso en vehículos**. Las clasificaciones de seguridad (HARA) son un borrador para validar con un ingeniero de seguridad funcional.

## Contenido

| Carpeta | Qué es |
|---|---|
| `core/` | Núcleo para ESP32 + MCP2515: Bus-Off (EFLG TXBO 0x20) con reintentos, slot de TX sincronizado con el BMS, SAFE-CUT latcheado, compuerta OTA, driver MCP2515 |
| `sim/` | Simulación SIL de la VCU: máquina de estados con precarga, arbitraje de torque, diagnóstico, regeneración limitada, colisión con descarga activa y monitor independiente |
| `sim/dbc/` | DBC de ejemplo didáctico (no corresponde a ningún vehículo real) |
| `docs/` | Requisitos de seguridad, trazabilidad y decisiones abiertas |

## Cómo ejecutar

```
make test       # pruebas del núcleo (HAL simulada) y banco SIL
make coverage   # cobertura de líneas y ramas con gcov (módulos de sim/)
```
Opciones: `CC=clang`, `EXTRA=-Werror`, `BUILD=/ruta`. Requiere un compilador C99 y `make`.

## Qué está verificado hoy

- `core/`: 35 verificaciones unitarias en host con HAL simulada.
- `sim/`: 11 casos de prueba automatizados (encendido seguro, precarga, Brake Override, pérdida de BMS detenido y en marcha, plausibilidad de pedal, torque solo en READY, regeneración limitada, colisión con descarga activa, monitor independiente). Cobertura de líneas entre 95 % y 100 %.
- Los casos se validaron también con **pruebas de mutación**: al romper cada función de seguridad a propósito, las pruebas fallan.

## Qué NO está verificado

- Ninguna prueba en hardware real (HIL) ni en vehículo. La simulación no cubre temporización real, ruido eléctrico ni el comportamiento de los contactores.
- Las mediciones de latencia y jitter del PoC se publicarán con su log CANoe y su método; mientras tanto no se citan cifras aquí.
- El monitor independiente (FSR-07) es una re-implementación diversa **en el mismo programa**. Un diseño real lo ejecuta en otro núcleo o chip con vía de corte propia.
- No implementado: diagnóstico UDS con DTCs, térmico y carga, OTA seguro (solo existe la compuerta), secuencia de apagado, ciberseguridad.

## Pines (ESP32 clásico, PoC)

SCK 18, MISO 19, MOSI 23, CS 5, SAFE-CUT GPIO4 (activo en BAJO). **No válidos en ESP32-S3**: allí no existen los GPIO22–25 y los GPIO19/20 son USB.

## Documentación

- [`docs/SAFETY_REQUIREMENTS.md`](docs/SAFETY_REQUIREMENTS.md): metas de seguridad y requisitos funcionales
- [`docs/TRACEABILITY.md`](docs/TRACEABILITY.md): requisito → código → prueba
- [`docs/DECISIONS.md`](docs/DECISIONS.md): decisiones abiertas (ciclo del BMS, licencia, criterios de descarga, etc.)

## Política de afirmaciones

Toda cifra o cumplimiento citado en este proyecto debe tener detrás un log, un reporte o un certificado con su método. Lo que es objetivo se marca como objetivo.



