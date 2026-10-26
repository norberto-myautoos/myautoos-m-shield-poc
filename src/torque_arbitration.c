#include "torque_arbitration.h"

static int16_t cmd_nm;
static bool    bos_latched, pedal_fault, limp_active;
static uint8_t mism_ticks, ok_ticks;
static int16_t limp_nm;

void torque_arb_init(void)
{
    cmd_nm = 0; bos_latched = false; pedal_fault = false; mism_ticks = 0U; ok_ticks = 0U;
    limp_active = false; limp_nm = 0;
}

/* FSR-06: tope de regeneracion segun BMS (P = T x w  =>  T = P / w). 0 si no se puede garantizar. */
static int16_t regen_cap_nm(const torque_inputs_t *in)
{
    uint32_t t;
    if (!in->bms_limits_valid || in->speed_kph <= REGEN_MIN_SPEED_KPH || in->motor_rpm == 0U) { return 0; }
    t = ((uint32_t)in->bms_max_charge_kw10 * 955U) / (uint32_t)in->motor_rpm;   /* 0.1 kW -> Nm */
    return (t > (uint32_t)REGEN_MAX_NM) ? (int16_t)REGEN_MAX_NM : (int16_t)t;
}

void torque_arb_task_10ms(const torque_inputs_t *in)
{
    int16_t target;
    uint8_t a_min = (in->accel_pct < in->accel2_pct) ? in->accel_pct : in->accel2_pct;   /* conservador */
    uint8_t a_max = (in->accel_pct > in->accel2_pct) ? in->accel_pct : in->accel2_pct;
    uint8_t diff  = (uint8_t)(a_max - a_min);

    /* FSR-02: plausibilidad de pedal (dos sensores) */
    if (diff > PEDAL_PLAUS_DELTA_PCT) {
        ok_ticks = 0U;
        if (mism_ticks < 250U) { mism_ticks++; }
        if (((uint32_t)mism_ticks * 10U) >= PEDAL_PLAUS_TIME_MS) { pedal_fault = true; }
    } else {
        mism_ticks = 0U;
        if (pedal_fault) {
            if (ok_ticks < 250U) { ok_ticks++; }
            if (((uint32_t)ok_ticks * 10U) >= PEDAL_RECOVER_TIME_MS) { pedal_fault = false; ok_ticks = 0U; }
        }
    }

    /* FSR-01: Brake Override (usa el mayor de los dos sensores, conservador) */
    if (in->brake_pct >= BOS_BRAKE_PCT && a_max >= BOS_ACCEL_PCT) { bos_latched = true; }
    else if (bos_latched && a_max < BOS_RESET_ACCEL_PCT)          { bos_latched = false; }

    /* SG-02: degradacion controlada (limite reducido que baja en rampa; nunca negativo) */
    if (in->degraded) {
        if (!limp_active) {
            limp_active = true;
            limp_nm = (cmd_nm < 0) ? 0 : ((cmd_nm < LIMP_MAX_NM) ? cmd_nm : (int16_t)LIMP_MAX_NM);
        } else if (limp_nm > LIMP_RAMP_NM_10MS) { limp_nm = (int16_t)(limp_nm - LIMP_RAMP_NM_10MS); }
        else { limp_nm = 0; }
    } else { limp_active = false; }

    if (in->fault || !in->ready || bos_latched || pedal_fault) {   /* seguridad primero, sin rampa */
        cmd_nm = 0;
        return;
    }

    if (a_max < BOS_ACCEL_PCT && in->brake_pct >= REGEN_BRAKE_DEADBAND) {
        /* FSR-06: regeneracion solo si el conductor la solicita (freno) y dentro de limites */
        int16_t req = (int16_t)(((int32_t)in->brake_pct * REGEN_MAX_NM) / 100);
        int16_t cap = regen_cap_nm(in);
        target = (int16_t)(-((req < cap) ? req : cap));
    } else {
        target = (int16_t)(((int32_t)a_min * TORQUE_MAX_NM) / 100);
    }
    if (limp_active && target > limp_nm) { target = limp_nm; }

    if (target < 0) {                                  /* regeneracion */
        if (cmd_nm > 0) { cmd_nm = 0; }                /* primero se quita la traccion */
        else if (target < cmd_nm) {                    /* aumenta gradualmente */
            int16_t dn = (int16_t)(cmd_nm - REGEN_RISE_NM_10MS);
            cmd_nm = (dn > target) ? dn : target;
        } else { cmd_nm = target; }                    /* reducir regeneracion: inmediato */
    } else if (target > cmd_nm) {                      /* traccion con limite de subida */
        int16_t base = (cmd_nm < 0) ? 0 : cmd_nm;
        int16_t up = (int16_t)(base + TORQUE_RISE_NM_10MS);
        cmd_nm = (up < target) ? up : target;
    } else {
        cmd_nm = target;
    }
}
int16_t torque_arb_get_cmd_nm(void) { return cmd_nm; }
bool torque_arb_bos_active(void) { return bos_latched; }
bool torque_arb_pedal_fault(void) { return pedal_fault; }
