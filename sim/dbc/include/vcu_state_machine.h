#ifndef VCU_STATE_MACHINE_H
#define VCU_STATE_MACHINE_H
#include <stdint.h>
#include <stdbool.h>
/* SIMULACION DE REFERENCIA - no apto para uso en vehiculo. */
#define SM_INIT_TIME_MS            100U
#define SM_START_BRAKE_MIN_PCT     10U
#define SM_PRECHARGE_TIME_MS       500U     /* tiempo minimo configurado */
#define SM_PRECHARGE_VOLT_PCT      95U      /* ... Y bus >= 95 % del pack */
#define SM_PRECHARGE_TIMEOUT_MS    2000U
#define SM_SPEED_STOP_KPH          5U       /* por debajo: se considera detenido */
#define SM_DEGRADED_MAX_MS         2000U
#define SM_FAULT_OPEN_MAX_WAIT_MS  100U     /* apertura de emergencia si el torque no llega a 0 */

typedef enum {
    VEHICLE_STATE_INIT, VEHICLE_STATE_STANDBY, VEHICLE_STATE_PRECHARGE,
    VEHICLE_STATE_READY_TO_DRIVE, VEHICLE_STATE_DEGRADED, VEHICLE_STATE_FAULT
} vehicle_state_t;

typedef struct { bool neg, pos, precharge; } contactor_cmd_t;
typedef struct {
    bool start_button; uint8_t brake_pct;
    uint32_t bus_voltage_mv, pack_voltage_mv;
    int16_t torque_cmd_nm; bool hard_fault, degradable_fault; uint16_t speed_kph;
} vcu_sm_inputs_t;

void            vcu_sm_init(void);
void            vcu_sm_task_20ms(const vcu_sm_inputs_t *in);
vehicle_state_t vcu_sm_get_state(void);
contactor_cmd_t vcu_sm_get_contactors(void);
bool            vcu_sm_get_discharge(void);   /* descarga activa del bus HV (FSR-08) */
#endif
