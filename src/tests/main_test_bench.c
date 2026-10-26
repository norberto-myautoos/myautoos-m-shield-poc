/* main_test_bench.c - Banco SIL (Software-in-the-Loop) para la VCU. C99 portable.
 * Tareas periodicas: 10 ms (diagnostico + torque), 20 ms (maquina de estados), 100 ms (lentas).
 * Resolucion de simulacion: 1 ms. SIMULACION: no es evidencia HIL ni apta para vehiculo. */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "vcu_state_machine.h"
#include "torque_arbitration.h"
#include "vcu_diagnostics.h"
#include "safety_monitor.h"

/* ---------- Estimulos globales (entradas "fisicas" del coche) ---------- */
uint8_t  g_stim_accel_pct;
uint8_t  g_stim_brake_pct;
bool     g_stim_start_button;
uint32_t g_stim_pack_voltage_mv;
bool     g_stim_bms_msg_present;      /* true = el BMS publica cada 10 ms */
int16_t  g_inj_torque_offset_nm;       /* inyeccion de falla: corrompe el torque comandado */
bool     g_stim_freeze_torque_task;   /* inyeccion de falla: la tarea de torque se cuelga */
bool     g_stim_crash_signal;         /* senal de colision (FSR-08) */
uint16_t g_stim_bms_max_charge_kw10;  /* limite de carga del BMS (0.1 kW) */
uint16_t g_stim_speed_kph;            /* velocidad del vehiculo (viene del inversor) */
int8_t   g_stim_accel2_offset_pct;    /* segundo sensor = acelerador + offset (FSR-02) */

/* ---------- Estado del banco / modelo de planta ---------- */
static uint32_t g_sim_time_ms;
static uint32_t g_bus_voltage_mv;     /* tension del bus HV (modelo RC simple) */
static bool     g_prev_pos;
static int16_t g_min_torque;
static uint32_t g_violation_under_load, g_violation_torque_not_ready;
static bool     g_seen_precharge, g_seen_ready;
static uint32_t g_t_precharge, g_t_ready;
static vehicle_state_t g_last_state;

static int g_checks, g_fails, g_tests_failed, g_tests_run;

static const char *state_name(vehicle_state_t s)
{
    switch (s) {
    case VEHICLE_STATE_INIT: return "INIT";
    case VEHICLE_STATE_STANDBY: return "STANDBY";
    case VEHICLE_STATE_PRECHARGE: return "PRECHARGE";
    case VEHICLE_STATE_READY_TO_DRIVE: return "READY_TO_DRIVE";
    case VEHICLE_STATE_DEGRADED: return "DEGRADED";
    case VEHICLE_STATE_FAULT: return "VEHICLE_STATE_FAULT";
    default: return "?";
    }
}

static int16_t commanded_torque_nm(void)     /* lo que la VCU envia al inversor */
{
    int32_t v = (int32_t)torque_arb_get_cmd_nm() + (int32_t)g_inj_torque_offset_nm;
    if (v > 32000) { v = 32000; } if (v < -32000) { v = -32000; }
    return (int16_t)v;
}
static int16_t output_torque_nm(void)        /* lo que el inversor recibe: via de corte del monitor */
{ return mon_tripped() ? (int16_t)0 : commanded_torque_nm(); }

static void snapshot(const char *label)
{
    contactor_cmd_t c = vcu_sm_get_contactors();
    printf("    [%s] t=%5lums state=%-19s torque=%4dNm contactors(neg,pos,pre)=%d%d%d bus=%6lumV flags=0x%02lX mon=%d\n",
           label, (unsigned long)g_sim_time_ms, state_name(vcu_sm_get_state()), (int)output_torque_nm(),
           c.neg, c.pos, c.precharge, (unsigned long)g_bus_voltage_mv, (unsigned long)diag_get_flags(), (int)mon_tripped());
}

#define CHECK(cond, msg) do { g_checks++; \
    if (cond) { printf("    PASS: %s\n", msg); } \
    else { g_fails++; printf("    FAIL: %s  (%s:%d)\n", msg, __FILE__, __LINE__); } } while (0)

/* ---------- Planta: contactores -> bus HV (1 ms) ---------- */
static void plant_1ms(void)
{
    contactor_cmd_t c = vcu_sm_get_contactors();
    if (c.neg && c.pos)              { g_bus_voltage_mv = g_stim_pack_voltage_mv; }
    else if (c.neg && c.precharge)   {
        int32_t diff = (int32_t)g_stim_pack_voltage_mv - (int32_t)g_bus_voltage_mv;
        g_bus_voltage_mv = (uint32_t)((int32_t)g_bus_voltage_mv + diff / 150);   /* tau ~150 ms */
    } else if (vcu_sm_get_discharge()) { g_bus_voltage_mv -= g_bus_voltage_mv / 1000U;   /* descarga activa: tau ~1 s */
    } else                         { g_bus_voltage_mv -= g_bus_voltage_mv / 60000U;  /* pasivo: tau ~60 s */ }
    if (g_prev_pos && !c.pos && output_torque_nm() != 0) { g_violation_under_load++; }
    g_prev_pos = c.pos;
    if (output_torque_nm() < g_min_torque) { g_min_torque = output_torque_nm(); }
    if (vcu_sm_get_state() != VEHICLE_STATE_READY_TO_DRIVE && vcu_sm_get_state() != VEHICLE_STATE_DEGRADED && output_torque_nm() != 0) { g_violation_torque_not_ready++; }
}

static void task_10ms(void)
{
    torque_inputs_t ti;
    if (g_stim_bms_msg_present && (g_sim_time_ms % BMS_CYCLE_MS) == 0U) { diag_bms_msg_received(); }
    if (g_stim_crash_signal) { diag_report_fault(DIAG_CRASH_SIGNAL); }
    diag_task_10ms();
    { int a2 = (int)g_stim_accel_pct + (int)g_stim_accel2_offset_pct;
      if (a2 < 0) { a2 = 0; } if (a2 > 100) { a2 = 100; }
      ti.accel2_pct = (uint8_t)a2; }
    ti.accel_pct = g_stim_accel_pct; ti.brake_pct = g_stim_brake_pct;
    { bool moving = g_stim_speed_kph > SM_SPEED_STOP_KPH;
      ti.ready = (vcu_sm_get_state() == VEHICLE_STATE_READY_TO_DRIVE) || (vcu_sm_get_state() == VEHICLE_STATE_DEGRADED);
      ti.fault = diag_is_hard_fault() || (diag_is_degradable_fault() && !moving);   /* detenido: corte inmediato */
      ti.degraded = diag_is_degradable_fault() && moving; }
    ti.speed_kph = g_stim_speed_kph; ti.motor_rpm = (uint16_t)(g_stim_speed_kph * 70U);   /* ~70 rpm por km/h */
    ti.bms_max_charge_kw10 = g_stim_bms_max_charge_kw10; ti.bms_limits_valid = !diag_is_critical_fault();                          /* en marcha: rampa */
    if (!g_stim_freeze_torque_task) { torque_arb_task_10ms(&ti); mon_alive_kick(); }
    {   mon_inputs_t mi;                                      /* el monitor ve lo que realmente se envia */
        mi.accel1_pct = g_stim_accel_pct; mi.accel2_pct = ti.accel2_pct; mi.brake_pct = g_stim_brake_pct;
        mi.drive_enabled = ti.ready; mi.degraded = ti.degraded; mi.speed_kph = g_stim_speed_kph;
        mi.torque_cmd_nm = commanded_torque_nm();
        mon_task_10ms(&mi);
    }
    if (mon_tripped()) { diag_report_fault(DIAG_MONITOR_TRIP); }
}
static void task_20ms(void)
{
    vcu_sm_inputs_t in;
    in.start_button = g_stim_start_button; in.brake_pct = g_stim_brake_pct;
    in.bus_voltage_mv = g_bus_voltage_mv; in.pack_voltage_mv = g_stim_pack_voltage_mv;
    in.torque_cmd_nm = output_torque_nm(); in.hard_fault = diag_is_hard_fault();
    in.degradable_fault = diag_is_degradable_fault(); in.speed_kph = g_stim_speed_kph;
    vcu_sm_task_20ms(&in);
}

/* Avanza la simulacion delta_time_ms (pasos de 1 ms) invocando las tareas periodicas. */
void run_simulation_step(uint32_t delta_time_ms)
{
    uint32_t i;
    for (i = 0U; i < delta_time_ms; i++) {
        g_sim_time_ms++;
        plant_1ms();
        if ((g_sim_time_ms % 10U)  == 0U) { task_10ms(); }
        if ((g_sim_time_ms % 20U)  == 0U) { task_20ms(); }
        if ((g_sim_time_ms % 100U) == 0U) { diag_task_100ms(); }
        if (vcu_sm_get_state() != g_last_state) {
            g_last_state = vcu_sm_get_state();
            if (g_last_state == VEHICLE_STATE_PRECHARGE && !g_seen_precharge) { g_seen_precharge = true; g_t_precharge = g_sim_time_ms; }
            if (g_last_state == VEHICLE_STATE_READY_TO_DRIVE && !g_seen_ready) { g_seen_ready = true; g_t_ready = g_sim_time_ms; }
        }
    }
}

static void bench_reset(void)
{
    g_sim_time_ms = 0U; g_bus_voltage_mv = 0U; g_prev_pos = false; g_violation_under_load = 0U; g_violation_torque_not_ready = 0U; g_stim_accel2_offset_pct = 0; g_stim_speed_kph = 0U; g_stim_bms_max_charge_kw10 = 500U; g_min_torque = 0; g_stim_crash_signal = false;
    mon_init(); g_inj_torque_offset_nm = 0; g_stim_freeze_torque_task = false;
    g_seen_precharge = g_seen_ready = false; g_t_precharge = g_t_ready = 0U;
    g_stim_accel_pct = 0U; g_stim_brake_pct = 0U; g_stim_start_button = false;
    g_stim_pack_voltage_mv = 400000U; g_stim_bms_msg_present = true;
    diag_init(); torque_arb_init(); vcu_sm_init(); g_last_state = vcu_sm_get_state();
}
static void tap_start(void) { g_stim_start_button = true; run_simulation_step(60U); g_stim_start_button = false; }
static bool run_until(vehicle_state_t st, uint32_t max_ms)
{
    uint32_t t;
    for (t = 0U; t < max_ms; t += 10U) { if (vcu_sm_get_state() == st) { return true; } run_simulation_step(10U); }
    return vcu_sm_get_state() == st;
}
static void go_ready(void)
{
    run_simulation_step(200U);
    g_stim_brake_pct = 50U; tap_start();
    (void)run_until(VEHICLE_STATE_READY_TO_DRIVE, 1500U);
    g_stim_brake_pct = 0U; run_simulation_step(40U);
}
static void begin(const char *name) { g_tests_run++; printf("\n== %s ==\n", name); g_fails = 0; }
static void end(void) { if (g_fails) { g_tests_failed++; } printf("  => %s\n", g_fails ? "FAILED" : "PASSED"); }

/* ---------- Casos de prueba ---------- */
static void TEST_CASE_01_Sujecion_Segura_Encendido(void)
{
    begin("TEST_CASE_01_Sujecion_Segura_Encendido");
    bench_reset();
    CHECK(vcu_sm_get_state() == VEHICLE_STATE_INIT, "arranca en INIT");
    run_simulation_step(200U);
    CHECK(vcu_sm_get_state() == VEHICLE_STATE_STANDBY, "INIT -> STANDBY");
    g_stim_brake_pct = 0U; tap_start(); run_simulation_step(1000U);
    snapshot("sin freno");
    CHECK(vcu_sm_get_state() == VEHICLE_STATE_STANDBY, "Start SIN freno: permanece en STANDBY");
    CHECK(!g_seen_precharge && !g_seen_ready, "nunca entro a PRECHARGE ni READY");
    g_stim_brake_pct = 5U; tap_start(); run_simulation_step(1000U);   /* freno insuficiente */
    CHECK(vcu_sm_get_state() == VEHICLE_STATE_STANDBY, "freno por debajo del umbral: sigue en STANDBY");
    { contactor_cmd_t c = vcu_sm_get_contactors();
      CHECK(!c.neg && !c.pos && !c.precharge, "contactores abiertos"); }
    end();
}
static void TEST_CASE_02_Secuencia_Precarga_Exitosa(void)
{
    contactor_cmd_t c;
    begin("TEST_CASE_02_Secuencia_Precarga_Exitosa");
    bench_reset(); run_simulation_step(200U);
    g_stim_brake_pct = 50U; tap_start();
    run_simulation_step(40U);
    c = vcu_sm_get_contactors(); snapshot("precarga");
    CHECK(vcu_sm_get_state() == VEHICLE_STATE_PRECHARGE, "freno + Start -> PRECHARGE");
    CHECK(c.neg && c.precharge && !c.pos, "en precarga: neg + resistencia cerrados, positivo abierto");
    CHECK(run_until(VEHICLE_STATE_READY_TO_DRIVE, 1500U), "alcanza READY_TO_DRIVE");
    c = vcu_sm_get_contactors(); snapshot("ready");
    CHECK(g_t_ready - g_t_precharge >= SM_PRECHARGE_TIME_MS, "espero al menos el tiempo de precarga configurado");
    CHECK(g_t_ready - g_t_precharge <= SM_PRECHARGE_TIME_MS + 100U, "sin demoras excesivas");
    CHECK(c.neg && c.pos && !c.precharge, "contactores principales cerrados, precarga abierta");
    CHECK(g_bus_voltage_mv >= (g_stim_pack_voltage_mv / 100U) * SM_PRECHARGE_VOLT_PCT, "bus >= 95 % del pack");
    end();
}
static void TEST_CASE_03_Brake_Override_Safety(void)
{
    begin("TEST_CASE_03_Brake_Override_Safety");
    bench_reset(); go_ready();
    CHECK(vcu_sm_get_state() == VEHICLE_STATE_READY_TO_DRIVE, "en READY_TO_DRIVE");
    g_stim_accel_pct = 100U; run_simulation_step(1000U); snapshot("acel 100%");
    CHECK(torque_arb_get_cmd_nm() == TORQUE_MAX_NM, "acelerador 100 % => torque maximo");
    g_stim_brake_pct = 50U; run_simulation_step(10U); snapshot("freno 50%");
    CHECK(torque_arb_get_cmd_nm() == 0, "freno 50 % con acel 100 %: torque 0 Nm en <= 10 ms");
    CHECK(torque_arb_bos_active(), "Brake Override activo");
    g_stim_brake_pct = 0U; run_simulation_step(100U);
    CHECK(torque_arb_get_cmd_nm() == 0, "al soltar el freno con acelerador a fondo: sigue en 0 (latch BOS)");
    g_stim_accel_pct = 0U; run_simulation_step(40U);
    CHECK(!torque_arb_bos_active(), "BOS se libera con acelerador < 5 %");
    g_stim_accel_pct = 40U; run_simulation_step(500U);
    CHECK(torque_arb_get_cmd_nm() == 100, "torque se recupera (40 % => 100 Nm)");
    end();
}
static void TEST_CASE_04_Mitigacion_Falla_Critica(void)
{
    uint32_t T; contactor_cmd_t c;
    begin("TEST_CASE_04_Mitigacion_Falla_Critica");   /* velocidad < 5 km/h: estado seguro inmediato */
    bench_reset(); go_ready();
    g_stim_accel_pct = 60U; run_simulation_step(500U); snapshot("en marcha");
    CHECK(torque_arb_get_cmd_nm() == 150, "en marcha con torque activo (150 Nm)");
    CHECK((diag_get_flags() & DIAG_CAN_TIMEOUT_ERROR) == 0U, "sin fallas antes de perder el BMS");
    g_stim_bms_msg_present = false; T = g_sim_time_ms;                /* ultimo mensaje BMS en T */
    run_simulation_step(40U);                       /* requisito: timeout = 50 ms (literal, no la macro) */
    CHECK((diag_get_flags() & DIAG_CAN_TIMEOUT_ERROR) == 0U, "antes de 50 ms: aun no hay CAN_TIMEOUT_ERROR");
    run_simulation_step(20U); snapshot("T+60ms");
    CHECK((diag_get_flags() & DIAG_CAN_TIMEOUT_ERROR) != 0U, "CAN_TIMEOUT_ERROR levantado tras el timeout");
    CHECK(vcu_sm_get_state() == VEHICLE_STATE_FAULT, "salta a VEHICLE_STATE_FAULT");
    CHECK(torque_arb_get_cmd_nm() == 0, "torque forzado a 0 Nm");
    run_simulation_step(100U); c = vcu_sm_get_contactors(); snapshot("T+160ms");
    CHECK(!c.neg && !c.pos && !c.precharge, "contactores de alto voltaje abiertos");
    CHECK(g_violation_under_load == 0U, "contactor abierto SIN carga (torque ya en 0)");
    CHECK(g_sim_time_ms - T <= 200U, "reaccion completa dentro de 200 ms");
    g_stim_bms_msg_present = true; g_stim_accel_pct = 60U; run_simulation_step(500U);
    CHECK(vcu_sm_get_state() == VEHICLE_STATE_FAULT && torque_arb_get_cmd_nm() == 0, "FAULT latched: no se reanuda solo");
    end();
}

static void TEST_CASE_05_Plausibilidad_Pedal(void)   /* FSR-02 */
{
    uint32_t T;
    begin("TEST_CASE_05_Plausibilidad_Pedal");
    bench_reset(); go_ready();
    g_stim_accel_pct = 50U; run_simulation_step(600U);
    CHECK(torque_arb_get_cmd_nm() == 125, "sensores coherentes: 50 % => 125 Nm");
    g_stim_accel2_offset_pct = 30; T = g_sim_time_ms;       /* sensor 2 diverge 30 % */
    run_simulation_step(90U);
    CHECK(torque_arb_get_cmd_nm() > 0 && !torque_arb_pedal_fault(), "antes de 100 ms: aun sin falla");
    run_simulation_step(20U); snapshot("T+110ms");
    CHECK(torque_arb_pedal_fault(), "tras 100 ms de discrepancia: falla de plausibilidad");
    CHECK(torque_arb_get_cmd_nm() == 0, "torque anulado (0 Nm)");
    CHECK(g_sim_time_ms - T <= 120U, "reaccion dentro de 120 ms");
    run_simulation_step(300U);
    CHECK(torque_arb_get_cmd_nm() == 0, "mientras la discrepancia persiste: sigue en 0");
    g_stim_accel2_offset_pct = 0; run_simulation_step(400U); snapshot("recuperado");
    CHECK(!torque_arb_pedal_fault() && torque_arb_get_cmd_nm() > 0, "sensores coherentes de nuevo: se libera y recupera torque");
    end();
}
static void TEST_CASE_06_Torque_Solo_En_READY(void)  /* FSR-03 */
{
    uint32_t t;
    begin("TEST_CASE_06_Torque_Solo_En_READY");
    bench_reset(); g_stim_accel_pct = 100U;                  /* acelerador a fondo desde el inicio */
    run_simulation_step(400U);
    CHECK(vcu_sm_get_state() == VEHICLE_STATE_STANDBY && torque_arb_get_cmd_nm() == 0, "INIT/STANDBY con acelerador a fondo: 0 Nm");
    g_stim_accel_pct = 0U; g_stim_brake_pct = 50U; tap_start(); g_stim_brake_pct = 0U; g_stim_accel_pct = 100U;
    for (t = 0U; t < 1500U && vcu_sm_get_state() != VEHICLE_STATE_READY_TO_DRIVE; t += 10U) { run_simulation_step(10U); }
    CHECK(vcu_sm_get_state() == VEHICLE_STATE_READY_TO_DRIVE, "llega a READY_TO_DRIVE");
    CHECK(g_violation_torque_not_ready == 0U, "0 Nm durante PRECHARGE y antes de READY (0 violaciones)");
    run_simulation_step(500U); snapshot("ready");
    CHECK(torque_arb_get_cmd_nm() > 0, "ya en READY: el torque se habilita");
    end();
}

static void TEST_CASE_07_Falla_Comunicacion_En_Marcha(void)   /* SG-02 */
{
    uint32_t T, t, start_ramp, steps; int16_t prev, cur; bool monotonic = true, smooth = true; contactor_cmd_t c;
    begin("TEST_CASE_07_Falla_Comunicacion_En_Marcha");
    bench_reset(); go_ready();
    g_stim_speed_kph = 60U; g_stim_accel_pct = 60U; run_simulation_step(500U);
    CHECK(torque_arb_get_cmd_nm() == 150, "en marcha a 60 km/h con 150 Nm");
    g_stim_bms_msg_present = false; T = g_sim_time_ms;
    run_simulation_step(60U); snapshot("T+60ms");
    CHECK((diag_get_flags() & DIAG_CAN_TIMEOUT_ERROR) != 0U, "CAN_TIMEOUT_ERROR levantado");
    CHECK(vcu_sm_get_state() == VEHICLE_STATE_DEGRADED, "a velocidad: pasa a DEGRADED, no a FAULT inmediato");
    CHECK(torque_arb_get_cmd_nm() > 0 && torque_arb_get_cmd_nm() <= LIMP_MAX_NM, "torque limitado al limite reducido, no cortado de golpe");
    c = vcu_sm_get_contactors();
    CHECK(c.neg && c.pos, "contactores siguen cerrados durante la rampa");
    prev = torque_arb_get_cmd_nm(); start_ramp = g_sim_time_ms;
    for (t = 0U; t < 900U && vcu_sm_get_state() == VEHICLE_STATE_DEGRADED; t += 10U) {
        run_simulation_step(10U); cur = torque_arb_get_cmd_nm();
        if (cur > prev) { monotonic = false; }
        if ((prev - cur) > LIMP_RAMP_NM_10MS) { smooth = false; }
        prev = cur;
    }
    steps = g_sim_time_ms - start_ramp;
    CHECK(monotonic, "el torque solo baja durante la rampa");
    CHECK(smooth, "sin escalones mayores a la rampa configurada");
    CHECK(steps >= 400U, "la degradacion dura >= 400 ms (no es un corte abrupto)");
    run_simulation_step(100U); c = vcu_sm_get_contactors(); snapshot("fin rampa");
    CHECK(vcu_sm_get_state() == VEHICLE_STATE_FAULT && torque_arb_get_cmd_nm() == 0, "al terminar: FAULT con 0 Nm");
    CHECK(!c.neg && !c.pos && !c.precharge, "contactores abiertos al final");
    CHECK(g_violation_under_load == 0U, "apertura sin carga (0 violaciones)");
    (void)T;
    end();
}

static void TEST_CASE_08_Regen_Limitada(void)   /* FSR-06 */
{
    int16_t prev, cur; uint32_t i; bool smooth = true; contactor_cmd_t c; uint32_t T;
    begin("TEST_CASE_08_Regen_Limitada");
    bench_reset(); go_ready();
    g_stim_speed_kph = 60U;                                   /* 4200 rpm en el modelo */
    run_simulation_step(200U);
    CHECK(torque_arb_get_cmd_nm() == 0, "sin freno ni acelerador: 0 Nm (sin regeneracion no solicitada)");
    g_stim_bms_max_charge_kw10 = 200U; g_min_torque = 0;      /* 20 kW a 4200 rpm = 45 Nm */
    g_stim_brake_pct = 100U; prev = 0;
    for (i = 0U; i < 40U; i++) {
        run_simulation_step(10U); cur = torque_arb_get_cmd_nm();
        if ((prev - cur) > REGEN_RISE_NM_10MS) { smooth = false; }
        prev = cur;
    }
    snapshot("regen max");
    CHECK(torque_arb_get_cmd_nm() == -45, "freno 100 %: regeneracion limitada por el BMS (-45 Nm)");
    CHECK(g_min_torque >= -45, "nunca supera el limite del BMS");
    CHECK(smooth, "la regeneracion aumenta gradualmente (sin escalones)");
    g_stim_brake_pct = 30U; run_simulation_step(40U);
    CHECK(torque_arb_get_cmd_nm() == -30, "freno 30 %: regeneracion = lo solicitado (-30 Nm)");
    g_stim_bms_max_charge_kw10 = 0U; run_simulation_step(20U);
    CHECK(torque_arb_get_cmd_nm() == 0, "BMS no admite carga (bateria llena): 0 Nm de inmediato");
    g_stim_bms_max_charge_kw10 = 200U; g_stim_speed_kph = 3U; g_stim_brake_pct = 50U; run_simulation_step(60U);
    CHECK(torque_arb_get_cmd_nm() == 0, "velocidad < 5 km/h: sin regeneracion");
    g_stim_speed_kph = 60U; g_stim_accel_pct = 100U; g_stim_brake_pct = 50U; run_simulation_step(20U);
    CHECK(torque_arb_get_cmd_nm() == 0 && torque_arb_bos_active(), "freno + acelerador: Brake Override, sin traccion ni regeneracion");
    g_stim_accel_pct = 0U; g_stim_brake_pct = 100U; run_simulation_step(400U);
    CHECK(torque_arb_get_cmd_nm() == -45, "de nuevo regenerando (-45 Nm)");
    g_stim_bms_msg_present = false; T = g_sim_time_ms;
    run_simulation_step(60U); snapshot("BMS perdido");
    CHECK(torque_arb_get_cmd_nm() >= 0, "BMS perdido: se retira la regeneracion (limites no vigentes)");
    run_simulation_step(300U); c = vcu_sm_get_contactors();
    CHECK(vcu_sm_get_state() == VEHICLE_STATE_FAULT && !c.neg && !c.pos, "termina en FAULT con contactores abiertos");
    CHECK(g_violation_under_load == 0U, "apertura sin carga (0 violaciones)");
    (void)T;
    end();
}

static void TEST_CASE_09_Colision_Descarga_Activa(void)   /* FSR-08 */
{
    uint32_t T, t; contactor_cmd_t c; bool under60 = false; uint32_t t_under60 = 0U;
    begin("TEST_CASE_09_Colision_Descarga_Activa");
    bench_reset(); go_ready();
    g_stim_speed_kph = 80U; g_stim_accel_pct = 60U; run_simulation_step(500U);
    CHECK(torque_arb_get_cmd_nm() == 150 && g_bus_voltage_mv > 300000U, "en marcha, bus HV energizado");
    g_stim_crash_signal = true; T = g_sim_time_ms;
    run_simulation_step(30U); c = vcu_sm_get_contactors(); snapshot("T+30ms");
    CHECK(torque_arb_get_cmd_nm() == 0, "torque a 0 Nm en <= 30 ms");
    CHECK(vcu_sm_get_state() == VEHICLE_STATE_FAULT, "FAULT inmediato (sin degradacion por velocidad)");
    CHECK(!c.neg && !c.pos && !c.precharge, "contactores abiertos en <= 30 ms");
    CHECK(vcu_sm_get_discharge(), "descarga activa comandada");
    CHECK(g_violation_under_load == 0U, "contactores abiertos sin carga (torque ya en 0)");
    for (t = 0U; t < 6000U; t += 10U) {
        run_simulation_step(10U);
        if (!under60 && g_bus_voltage_mv < 60000U) { under60 = true; t_under60 = g_sim_time_ms - T; }
    }
    snapshot("T+6s");
    CHECK(under60 && t_under60 <= 5000U, "bus HV < 60 V en <= 5 s (criterio tipico; verificar norma aplicable)");
    CHECK(t_under60 >= 500U, "la descarga no es instantanea (modelo RC): se mide, no se asume");
    g_stim_crash_signal = false; run_simulation_step(200U);
    CHECK(vcu_sm_get_state() == VEHICLE_STATE_FAULT, "FAULT latched aunque la senal desaparezca");
    printf("    t(bus < 60 V) = %lu ms\n", (unsigned long)t_under60);
    end();
}
static void TEST_CASE_10_Colision_Durante_Precarga(void)   /* FSR-08 */
{
    contactor_cmd_t c;
    begin("TEST_CASE_10_Colision_Durante_Precarga");
    bench_reset(); run_simulation_step(200U);
    g_stim_brake_pct = 50U; tap_start(); run_simulation_step(100U);
    CHECK(vcu_sm_get_state() == VEHICLE_STATE_PRECHARGE, "en PRECHARGE");
    g_stim_crash_signal = true; run_simulation_step(40U); c = vcu_sm_get_contactors(); snapshot("colision");
    CHECK(vcu_sm_get_state() == VEHICLE_STATE_FAULT, "colision en precarga: FAULT");
    CHECK(!c.neg && !c.pos && !c.precharge, "contactores abiertos");
    CHECK(vcu_sm_get_discharge(), "descarga activa comandada");
    end();
}

static void TEST_CASE_11_Monitor_Independiente(void)   /* FSR-07 */
{
    uint32_t T; contactor_cmd_t c;
    begin("TEST_CASE_11_Monitor_Independiente");

    /* a) operacion normal: ningun falso disparo */
    bench_reset(); go_ready();
    g_stim_accel_pct = 20U; run_simulation_step(500U);
    g_stim_accel_pct = 80U; run_simulation_step(500U);
    g_stim_speed_kph = 60U; g_stim_accel_pct = 100U; g_stim_brake_pct = 50U; run_simulation_step(200U);   /* BOS */
    g_stim_accel_pct = 0U; g_stim_brake_pct = 0U; run_simulation_step(100U);
    g_stim_brake_pct = 100U; run_simulation_step(500U);                                                    /* regeneracion */
    g_stim_brake_pct = 0U; run_simulation_step(100U);
    CHECK(!mon_tripped(), "traccion, Brake Override y regeneracion: sin falsos disparos");

    /* b) glitch de un solo ciclo: el filtro lo ignora */
    bench_reset(); go_ready(); g_stim_accel_pct = 20U; run_simulation_step(500U);
    g_inj_torque_offset_nm = 100; run_simulation_step(10U); g_inj_torque_offset_nm = 0; run_simulation_step(200U);
    CHECK(!mon_tripped(), "glitch de 10 ms: no dispara (debounce de 30 ms)");

    /* c) torque excesivo persistente (error de software o bit flip) */
    bench_reset(); go_ready(); g_stim_accel_pct = 20U; run_simulation_step(500U);
    CHECK(torque_arb_get_cmd_nm() == 50, "acelerador 20 % => 50 Nm");
    g_inj_torque_offset_nm = 100; T = g_sim_time_ms;                 /* el torque enviado seria 150 Nm */
    run_simulation_step(20U);
    CHECK(!mon_tripped(), "a los 20 ms aun no dispara (debounce)");
    run_simulation_step(20U); snapshot("T+40ms");
    CHECK(mon_tripped() && (mon_trip_reason() & MON_REASON_OVER_TORQUE) != 0U, "dispara por torque excesivo en <= 40 ms");
    CHECK(output_torque_nm() == 0, "la via independiente fuerza 0 Nm hacia el inversor");
    CHECK(g_sim_time_ms - T <= 40U, "reaccion dentro del FTTI de SG-01 (100 ms)");
    run_simulation_step(100U); c = vcu_sm_get_contactors();
    CHECK(vcu_sm_get_state() == VEHICLE_STATE_FAULT && !c.neg && !c.pos, "VCU en FAULT con contactores abiertos");
    CHECK(g_violation_under_load == 0U, "apertura sin carga (0 violaciones)");

    /* d) par negativo no solicitado (regeneracion/frenado fantasma) */
    bench_reset(); go_ready(); run_simulation_step(100U);
    g_inj_torque_offset_nm = -80; run_simulation_step(60U);
    CHECK(mon_tripped() && (mon_trip_reason() & MON_REASON_UNDER_TORQUE) != 0U, "par negativo sin solicitud: dispara");
    CHECK(output_torque_nm() == 0, "salida a 0 Nm");

    /* e) la tarea de torque se cuelga con el conductor ya frenando */
    bench_reset(); go_ready();
    g_stim_speed_kph = 60U; g_stim_accel_pct = 60U; run_simulation_step(500U);
    CHECK(torque_arb_get_cmd_nm() == 150, "en marcha con 150 Nm");
    g_stim_freeze_torque_task = true; g_stim_accel_pct = 0U; g_stim_brake_pct = 50U; T = g_sim_time_ms;
    run_simulation_step(40U); snapshot("tarea colgada");
    CHECK(mon_tripped() && (mon_trip_reason() & MON_REASON_ALIVE_STALL) != 0U, "detecta tarea de torque detenida (alive)");
    CHECK(output_torque_nm() == 0, "el inversor deja de recibir los 150 Nm obsoletos");
    CHECK(g_sim_time_ms - T <= 40U, "dentro de 40 ms");
    end();
}

int main(void)
{
    printf("MyAutoOS - Banco SIL de la VCU (simulacion; no apto para uso en vehiculo)\n");
    TEST_CASE_01_Sujecion_Segura_Encendido();
    TEST_CASE_02_Secuencia_Precarga_Exitosa();
    TEST_CASE_03_Brake_Override_Safety();
    TEST_CASE_04_Mitigacion_Falla_Critica();
    TEST_CASE_05_Plausibilidad_Pedal();
    TEST_CASE_06_Torque_Solo_En_READY();
    TEST_CASE_07_Falla_Comunicacion_En_Marcha();
    TEST_CASE_08_Regen_Limitada();
    TEST_CASE_09_Colision_Descarga_Activa();
    TEST_CASE_10_Colision_Durante_Precarga();
    TEST_CASE_11_Monitor_Independiente();
    printf("\n==== RESUMEN: %d/%d pruebas PASSED, %d FAILED ====\n", g_tests_run - g_tests_failed, g_tests_run, g_tests_failed);
    return g_tests_failed ? 1 : 0;
}
