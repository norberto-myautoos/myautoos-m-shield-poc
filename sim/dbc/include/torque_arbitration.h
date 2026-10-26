#ifndef TORQUE_ARBITRATION_H
#define TORQUE_ARBITRATION_H
#include <stdint.h>
#include <stdbool.h>
/* SIMULACION DE REFERENCIA - no apto para uso en vehiculo. Valores calibrables. */
#define TORQUE_MAX_NM        250
#define TORQUE_RISE_NM_10MS  25      /* limite de subida; la bajada es inmediata */
#define BOS_BRAKE_PCT        10U     /* Brake Override: freno >= 10 % ...        */
#define BOS_ACCEL_PCT        10U     /* ... con acelerador >= 10 % => torque 0   */
#define BOS_RESET_ACCEL_PCT  5U      /* se libera cuando el acelerador < 5 %     */
#define LIMP_MAX_NM          120     /* limite reducido al degradar */
#define LIMP_RAMP_NM_10MS    2       /* bajada controlada: 120 Nm en ~600 ms */
#define REGEN_MAX_NM         100     /* FSR-06: limite fisico/de calibracion de regeneracion */
#define REGEN_RISE_NM_10MS   5       /* aumento gradual de regeneracion; liberar es inmediato */
#define REGEN_MIN_SPEED_KPH  5U      /* sin regeneracion casi detenido */
#define REGEN_BRAKE_DEADBAND 3U      /* % de freno minimo para solicitar regeneracion */
#define PEDAL_PLAUS_DELTA_PCT    10U /* FSR-02: diferencia maxima entre sensores */
#define PEDAL_PLAUS_TIME_MS      100U/* ... durante mas de este tiempo => torque 0 */
#define PEDAL_RECOVER_TIME_MS    100U/* sensores coherentes este tiempo => se libera */

typedef struct {
    uint8_t accel_pct, accel2_pct, brake_pct;
    bool ready, fault, degraded;
    uint16_t speed_kph, motor_rpm;
    uint16_t bms_max_charge_kw10;   /* limite de potencia de carga del BMS, 0.1 kW */
    bool bms_limits_valid;          /* false si el BMS no esta vigente */
} torque_inputs_t;

void    torque_arb_init(void);
void    torque_arb_task_10ms(const torque_inputs_t *in);
int16_t torque_arb_get_cmd_nm(void);
bool    torque_arb_bos_active(void);
bool    torque_arb_pedal_fault(void);
#endif
