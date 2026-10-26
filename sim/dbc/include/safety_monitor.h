#ifndef SAFETY_MONITOR_H
#define SAFETY_MONITOR_H
#include <stdint.h>
#include <stdbool.h>
/* FSR-07: monitor independiente del torque. SIMULACION: en un diseno real corre en otro
 * nucleo/chip, con reloj y alimentacion propios, y actua sobre una via de corte independiente
 * (p. ej. Safe Torque Off del inversor). Aqui es una re-implementacion DIVERSA de la
 * especificacion: no incluye ni reutiliza el codigo de torque_arbitration. */
#define MON_REASON_OVER_TORQUE  0x01U   /* torque comandado sobre el maximo esperado */
#define MON_REASON_UNDER_TORQUE 0x02U   /* torque (regen) bajo el minimo esperado */
#define MON_REASON_ALIVE_STALL  0x04U   /* la tarea de torque dejo de ejecutarse */

typedef struct {
    uint8_t accel1_pct, accel2_pct, brake_pct;
    bool drive_enabled, degraded;
    uint16_t speed_kph;
    int16_t torque_cmd_nm;          /* lo que realmente se envia al inversor */
} mon_inputs_t;

void    mon_init(void);
void    mon_alive_kick(void);       /* la tarea de torque la llama en cada ejecucion */
void    mon_task_10ms(const mon_inputs_t *in);
bool    mon_tripped(void);          /* latched hasta mon_init() */
uint8_t mon_trip_reason(void);
#endif
