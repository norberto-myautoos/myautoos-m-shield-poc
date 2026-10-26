#ifndef VCU_DIAGNOSTICS_H
#define VCU_DIAGNOSTICS_H
#include <stdint.h>
#include <stdbool.h>
/* SIMULACION DE REFERENCIA - no apto para uso en vehiculo. */
#define BMS_CYCLE_MS             10U     /* periodo de BMS_Status (ver DBC); multiplo de 10 */
#define BMS_TIMEOUT_CYCLES       5U      /* ciclos perdidos antes de declarar falla */
#define DIAG_BMS_TIMEOUT_MS      (BMS_CYCLE_MS * BMS_TIMEOUT_CYCLES)
#if (BMS_CYCLE_MS % 10U) != 0U
#error "BMS_CYCLE_MS debe ser multiplo de 10 (resolucion de la tarea de diagnostico)"
#endif
#if BMS_TIMEOUT_CYCLES < 3U
#error "Timeout menor a 3 ciclos del BMS: falsas alarmas por jitter"
#endif
#define DIAG_CAN_TIMEOUT_ERROR   0x01U   /* perdida de mensaje BMS */
#define DIAG_PRECHARGE_TIMEOUT   0x02U
#define DIAG_MONITOR_TRIP        0x08U   /* disparo del monitor independiente (FSR-07) */
#define DIAG_CRASH_SIGNAL        0x04U   /* senal de colision (FSR-08), falla dura */

void     diag_init(void);
void     diag_bms_msg_received(void);    /* llamar al recibir la trama BMS */
void     diag_task_10ms(void);           /* vigilancia de timeout */
void     diag_task_100ms(void);          /* tareas lentas (contador de vida) */
void     diag_report_fault(uint32_t flag);
uint32_t diag_get_flags(void);
bool     diag_is_critical_fault(void);   /* cualquier falla, latched hasta diag_init() */
bool     diag_is_hard_fault(void);       /* fallas que exigen estado seguro inmediato */
bool     diag_is_degradable_fault(void); /* solo perdida de BMS: permite degradacion controlada */
#endif
