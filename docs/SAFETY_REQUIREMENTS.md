# Metas de seguridad y requisitos funcionales (borrador)

Ejemplo didáctico. Las clasificaciones S/E/C, los ASIL y los tiempos (FTTI) son **propuestas** que debe validar un ingeniero de seguridad funcional con la definición real del ítem. No es evidencia de cumplimiento de ISO 26262.

## Metas de seguridad propuestas

| ID | Meta | Peligro | ASIL propuesto | FTTI (supuesto) |
|---|---|---|---|---|
| SG-01 | Evitar torque mayor al solicitado o a un límite plausible | Aceleración no intencionada | C | 100 ms |
| SG-02 | Evitar pérdida abrupta de torque; degradar de forma controlada y abrir HV solo sin carga | Pérdida de torque, apertura bajo carga | B | 200 ms |
| SG-03 | Evitar torque sin demanda o fuera de READY_TO_DRIVE | Avance no deseado | A | 200 ms |
| SG-04 | Limitar la regeneración a lo solicitado y seguro | Frenado regenerativo no solicitado | B | 100 ms |
| SG-05 | Poder desenergizar el alto voltaje ante falla grave o colisión | HV energizado tras incidente | B | por definir |

## Estrategia de estado seguro

- Falla de la cadena de torque, falla dura o colisión: torque a 0 de inmediato.
- Pérdida del BMS en marcha (más de 5 km/h): estado DEGRADED (límite de 120 Nm con rampa de unos 600 ms), luego FAULT y apertura sin carga.
- Detenido: torque 0 y apertura inmediata.

## Requisitos funcionales de seguridad

| FSR | Requisito | Meta | Módulo |
|---|---|---|---|
| FSR-01 | Brake Override: freno ≥ 10 % y acelerador ≥ 10 % → 0 Nm en ≤ 10 ms, con latch | SG-01 | `torque_arbitration` |
| FSR-02 | Plausibilidad de pedal: sensores que difieren > 10 % durante 100 ms anulan el torque | SG-01 | `torque_arbitration` |
| FSR-03 | Torque distinto de cero solo en READY_TO_DRIVE / DEGRADED | SG-03 | `torque_arbitration`, `vcu_state_machine` |
| FSR-04 | Pérdida del BMS (timeout = 5 ciclos) → estado seguro según velocidad | SG-02 | `vcu_diagnostics`, `vcu_state_machine` |
| FSR-05 | Contactores principales solo con precarga validada por tiempo y tensión (≥ 95 %) | SG-02 | `vcu_state_machine` |
| FSR-06 | Regeneración solo si el conductor la solicita, limitada por el BMS (T = P/ω) | SG-04 | `torque_arbitration` |
| FSR-07 | Monitor independiente compara el torque enviado con el esperado y fuerza el estado seguro | SG-01 | `safety_monitor` |
| FSR-08 | Colisión: apertura inmediata de contactores y descarga activa del bus HV | SG-05 | `vcu_state_machine` |

Los valores numéricos son de ejemplo y deben calibrarse con el vehículo real.
