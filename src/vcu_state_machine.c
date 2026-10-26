#include "vcu_state_machine.h"
#include "vcu_diagnostics.h"
#include <string.h>

static struct { vehicle_state_t st; uint32_t t_ms, t_state0; bool prev_start, discharge; contactor_cmd_t c; } s;

static void enter(vehicle_state_t n) { s.st = n; s.t_state0 = s.t_ms; }
static uint32_t in_state_ms(void) { return s.t_ms - s.t_state0; }

void vcu_sm_init(void) { (void)memset(&s, 0, sizeof s); s.st = VEHICLE_STATE_INIT; }

void vcu_sm_task_20ms(const vcu_sm_inputs_t *in)
{
    bool edge = in->start_button && !s.prev_start;
    s.prev_start = in->start_button;
    s.t_ms += 20U;

    if (in->hard_fault && s.st != VEHICLE_STATE_FAULT) { enter(VEHICLE_STATE_FAULT); }
    else if (in->degradable_fault && s.st != VEHICLE_STATE_FAULT && s.st != VEHICLE_STATE_DEGRADED) {
        if (s.st == VEHICLE_STATE_READY_TO_DRIVE && in->speed_kph > SM_SPEED_STOP_KPH) { enter(VEHICLE_STATE_DEGRADED); }
        else { enter(VEHICLE_STATE_FAULT); }       /* detenido o no operativo: estado seguro inmediato */
    }

    switch (s.st) {
    case VEHICLE_STATE_INIT:
        if (in_state_ms() >= SM_INIT_TIME_MS) { enter(VEHICLE_STATE_STANDBY); }
        break;
    case VEHICLE_STATE_STANDBY:
        if (edge && in->brake_pct >= SM_START_BRAKE_MIN_PCT) {   /* sujecion segura */
            s.c.neg = true; s.c.precharge = true; enter(VEHICLE_STATE_PRECHARGE);
        }
        break;
    case VEHICLE_STATE_PRECHARGE: {
        uint32_t need = (in->pack_voltage_mv * SM_PRECHARGE_VOLT_PCT) / 100U;
        if (in_state_ms() >= SM_PRECHARGE_TIME_MS && in->bus_voltage_mv >= need) {
            s.c.pos = true; s.c.precharge = false; enter(VEHICLE_STATE_READY_TO_DRIVE);
        } else if (in_state_ms() >= SM_PRECHARGE_TIMEOUT_MS) {
            diag_report_fault(DIAG_PRECHARGE_TIMEOUT); enter(VEHICLE_STATE_FAULT);
        }
        break; }
    case VEHICLE_STATE_READY_TO_DRIVE:
        break;
    case VEHICLE_STATE_DEGRADED:   /* contactores cerrados mientras el torque baja en rampa */
        if (in->torque_cmd_nm == 0 || in_state_ms() >= SM_DEGRADED_MAX_MS) { enter(VEHICLE_STATE_FAULT); }
        break;
    case VEHICLE_STATE_FAULT:   /* abrir contactores solo sin carga (torque 0) o tras espera maxima */
        {   bool crash = (diag_get_flags() & DIAG_CRASH_SIGNAL) != 0U;
            if (crash) { s.discharge = true; }                     /* FSR-08: descarga activa, latched */
            if (crash || in->torque_cmd_nm == 0 || in_state_ms() >= SM_FAULT_OPEN_MAX_WAIT_MS) {
                s.c.pos = false; s.c.neg = false; s.c.precharge = false;   /* colision: apertura inmediata */
            }
        }
        break;
    default: break;
    }
}
vehicle_state_t vcu_sm_get_state(void) { return s.st; }
contactor_cmd_t vcu_sm_get_contactors(void) { return s.c; }
bool vcu_sm_get_discharge(void) { return s.discharge; }
