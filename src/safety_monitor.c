#include "safety_monitor.h"

/* Valores tomados del REQUISITO (HARA/FSR), no del codigo del arbitraje. */
#define MON_TORQUE_MAX_NM   250
#define MON_REGEN_MAX_NM    100
#define MON_LIMP_MAX_NM     120
#define MON_TOL_NM          15      /* tolerancia de rampas y cuantizacion */
#define MON_BOS_BRAKE_PCT   10U
#define MON_BOS_ACCEL_PCT   10U
#define MON_REGEN_BRAKE_PCT 3U
#define MON_STOP_KPH        5U
#define MON_DEBOUNCE_TICKS  3U      /* 30 ms: FTTI de SG-01 = 100 ms */
#define MON_ALIVE_TICKS     3U

static struct { bool tripped; uint8_t reason, viol_ticks, stall_ticks; uint32_t kicks, last_kicks; } m;

void mon_init(void) { m.tripped = false; m.reason = 0U; m.viol_ticks = 0U; m.stall_ticks = 0U; m.kicks = 0U; m.last_kicks = 0U; }
void mon_alive_kick(void) { m.kicks++; }
bool mon_tripped(void) { return m.tripped; }
uint8_t mon_trip_reason(void) { return m.reason; }

void mon_task_10ms(const mon_inputs_t *in)
{
    int32_t hi = 0, lo = 0;
    uint8_t a_min = (in->accel1_pct < in->accel2_pct) ? in->accel1_pct : in->accel2_pct;
    uint8_t a_max = (in->accel1_pct > in->accel2_pct) ? in->accel1_pct : in->accel2_pct;
    uint8_t r = 0U;

    if (in->drive_enabled) {
        hi = ((int32_t)a_min * MON_TORQUE_MAX_NM) / 100;
        if (in->brake_pct >= MON_BOS_BRAKE_PCT && a_max >= MON_BOS_ACCEL_PCT) { hi = 0; }   /* Brake Override */
        if (in->degraded && hi > MON_LIMP_MAX_NM) { hi = MON_LIMP_MAX_NM; }
        if (!in->degraded && a_max < MON_BOS_ACCEL_PCT && in->brake_pct >= MON_REGEN_BRAKE_PCT
            && in->speed_kph > MON_STOP_KPH) {
            lo = -(((int32_t)in->brake_pct * MON_REGEN_MAX_NM) / 100);                       /* regen pedida */
        }
    }
    hi += MON_TOL_NM; lo -= MON_TOL_NM;

    if (in->torque_cmd_nm > hi) { r |= MON_REASON_OVER_TORQUE; }
    if (in->torque_cmd_nm < lo) { r |= MON_REASON_UNDER_TORQUE; }
    if (r != 0U) { if (m.viol_ticks < 250U) { m.viol_ticks++; } } else { m.viol_ticks = 0U; }

    if (m.kicks == m.last_kicks) { if (m.stall_ticks < 250U) { m.stall_ticks++; } } else { m.stall_ticks = 0U; }
    m.last_kicks = m.kicks;
    if (m.stall_ticks >= MON_ALIVE_TICKS) { r |= MON_REASON_ALIVE_STALL; }

    if (!m.tripped && (m.viol_ticks >= MON_DEBOUNCE_TICKS || m.stall_ticks >= MON_ALIVE_TICKS)) {
        m.tripped = true; m.reason = r;
    }
}
