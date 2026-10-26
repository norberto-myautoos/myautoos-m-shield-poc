# Decisiones abiertas

1. **Ciclo real del BMS.** `sim/` asume 10 ms (timeout = 5 ciclos = 50 ms, parametrizado en `vcu_diagnostics.h`); `core/` asume 100 ms (pérdida a 250 ms). Confirmar con la especificación del BMS y unificar. El compilador rechaza timeouts menores a 3 ciclos.
2. **Celda 34 "1 A 0.8 k".** Se usa 1000 mA como límite provisional (`CELL_I_LIMIT_MA`). Confirmar significado y unidad.
3. **Ventana de estabilidad del Bus-Off:** 2000 ms (supuesto).
4. **Reacción ante pérdida de la trama 0x100:** corte seguro (supuesto de diseño).
5. **Criterio de descarga del bus HV:** el modelo mide 1.9 s hasta < 60 V; el criterio típico es 60 V en 5 s. Verificar con la norma aplicable y el cliente.
6. **Plataforma de hardware:** ESP32 clásico (PoC) frente a ESP32-S3; pines del README solo válidos en el clásico. Un diseño automotriz real requiere un MCU de seguridad (lockstep, ECC).
7. **Licencia:** el repositorio no incluye archivo LICENSE; por defecto, todos los derechos reservados. Decidir entre una licencia abierta (por ejemplo MIT o Apache-2.0) o una comercial. Conviene consultar a un abogado antes de publicar.
8. **Valores de calibración** (torque máximo, umbrales, rampas): de ejemplo.
9. **Validación del HARA** con un ingeniero de seguridad funcional.
