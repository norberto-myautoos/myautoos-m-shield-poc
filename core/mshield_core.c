/* mshield_core.c - Bus-Off FSM, slot TX sincrono con BMS, SAFE-CUT latcheado, gate OTA.
 * Sin delays, sin malloc, sin bucles no acotados. */
#include "vcu_mshield.h"
#include <string.h>

#define EFLG_TXBO   0x20U   /* MCP2515 EFLG (0x2D) bit5 [REQ-01] */
#define RX_BUDGET   8U

static struct { ms_bo_state_t st; uint32_t t0; uint8_t retries; } bo;
static struct { uint32_t last_bms, tx_due, last_tx; bool pending, bms_lost, have_tx; } tx;
static struct { bool latched; uint8_t low_cnt, ok_cnt; } sc;
static mshield_stats_t stats;

static bool elapsed(uint32_t now, uint32_t t0, uint32_t dt) { return (uint32_t)(now - t0) >= dt; }
static bool reached(uint32_t now, uint32_t t) { return (int32_t)(now - t) >= 0; }

/* ---------- [REQ-01] Bus-Off ---------- */
static void busoff_step(uint32_t now)
{
    uint8_t ef;
    switch (bo.st) {
    case MS_BO_OK:
        if (hal_mcp_read_eflg(&ef) && ((ef & EFLG_TXBO) != 0U)) {
            bo.st = MS_BO_WAIT; bo.t0 = now; bo.retries = 0U;
            stats.busoff_events++; dtc_set(DTC_BUSOFF);
        }
        break;
    case MS_BO_WAIT:
        if (elapsed(now, bo.t0, BO_RETRY_MS)) { bo.retries++; bo.st = MS_BO_RECOVER; }
        break;
    case MS_BO_RECOVER:
        if (hal_mcp_request_normal_mode()) { bo.st = MS_BO_VERIFY; bo.t0 = now; }
        break;
    case MS_BO_VERIFY:
        if (hal_mcp_read_eflg(&ef)) {
            if ((ef & EFLG_TXBO) != 0U) {
                bo.t0 = now;
                if (bo.retries >= BO_MAX_RETRIES) { bo.st = MS_BO_FAILED; dtc_set(DTC_BUSOFF_LATCH); }
                else { bo.st = MS_BO_WAIT; }
            } else if (elapsed(now, bo.t0, BO_STABLE_MS)) {
                bo.st = MS_BO_OK; bo.retries = 0U;      /* estable: sin reset del MCU */
            }
        }
        break;
    default: break;                                      /* FAILED: latched */
    }
}

/* ---------- [REQ-02] Slot TX sincrono con 0x100 ---------- */
static void tx_step(uint32_t now)
{
    uint16_t id; uint32_t ts; uint8_t i; uint8_t f[8];
    for (i = 0U; i < RX_BUDGET; i++) {
        if (!hal_can_rx_poll(&id, &ts)) { break; }
        if (id == ID_BMS) {
            tx.last_bms = ts; tx.tx_due = ts + TX_OFFSET_MS;
            tx.pending = true; tx.bms_lost = false;
        }
    }
    if (!tx.bms_lost && elapsed(now, tx.last_bms, BMS_LOSS_MS)) {
        tx.bms_lost = true; dtc_set(DTC_BMS_LOSS);
    }
    if (bo.st != MS_BO_OK) { return; }
    if (tx.bms_lost && elapsed(now, tx.last_tx, BMS_PERIOD_MS)) { tx.pending = true; tx.tx_due = now; }
    if (tx.pending && reached(now, tx.tx_due)) {
        build_vcu_frame(f);
        if (hal_can_tx(ID_VCU, f, 8U)) {
            uint32_t lat = (uint32_t)(now - tx.last_bms);
            stats.tx_count++; stats.lat_last_ms = lat;
            if (lat > stats.lat_max_ms) { stats.lat_max_ms = lat; }
            if (tx.have_tx) {
                uint32_t p = (uint32_t)(now - tx.last_tx);
                if (p < stats.period_min_ms) { stats.period_min_ms = p; }
                if (p > stats.period_max_ms) { stats.period_max_ms = p; }
            }
            tx.have_tx = true; tx.pending = false; tx.last_tx = now;
        }
    }
}

/* ---------- [REQ-03][REQ-04] SAFE-CUT latcheado ---------- */
static void safecut_step(void)
{
    bms_sample_t s;
    bool fault = (bo.st == MS_BO_FAILED) || tx.bms_lost;
    if (hal_bms_get(&s)) {
        if (!s.valid) { fault = true; }
        else {
            if (s.soc_pct < SOC_CUT_PCT) { if (sc.low_cnt < SOC_DEBOUNCE_N) { sc.low_cnt++; } }
            else { sc.low_cnt = 0U; }
            if (s.soc_pct >= SOC_RELEASE_PCT) { if (sc.ok_cnt < SOC_DEBOUNCE_N) { sc.ok_cnt++; } }
            else { sc.ok_cnt = 0U; }
            if (s.cell_ma > CELL_I_LIMIT_MA || s.cell_ma < -CELL_I_LIMIT_MA) { fault = true; }
        }
    }
    if (fault || sc.low_cnt >= SOC_DEBOUNCE_N) {
        if (!sc.latched) { sc.latched = true; stats.cut_events++; dtc_set(DTC_SAFECUT); }
        sc.ok_cnt = 0U;
    } else if (sc.latched && sc.ok_cnt >= SOC_DEBOUNCE_N) {
        sc.latched = false;
    }
    hal_gpio4_write(sc.latched ? GPIO4_CUT_LEVEL : GPIO4_RUN_LEVEL);
}

bool mshield_ota_allowed(void)   /* [REQ-05] firma y rollback A/B: responsabilidad del bootloader */
{ return (bo.st == MS_BO_OK) && !sc.latched && !tx.bms_lost && hal_vehicle_stationary(); }
bool mshield_safecut_active(void) { return sc.latched; }
ms_bo_state_t mshield_bus_state(void) { return bo.st; }
mshield_stats_t mshield_get_stats(void) { return stats; }

void mshield_init(void)
{
    (void)memset(&bo, 0, sizeof bo); (void)memset(&tx, 0, sizeof tx);
    (void)memset(&sc, 0, sizeof sc); (void)memset(&stats, 0, sizeof stats);
    stats.period_min_ms = 0xFFFFFFFFUL;
    bo.st = MS_BO_OK; tx.last_bms = hal_millis();
    hal_gpio4_write(GPIO4_RUN_LEVEL);
}

void mshield_step(void)
{
    uint32_t now = hal_millis();
    busoff_step(now); tx_step(now); safecut_step();
}
