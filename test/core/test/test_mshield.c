/* Pruebas unitarias en host con HAL simulado. Cada test lleva su [REQ-xx]. */
#include "vcu_mshield.h"
#include <stdio.h>
#include <string.h>

static uint32_t now; static uint8_t eflg; static bool gpio4, nm_ok, bms_new, stationary, clear_on_nm;
static int nm_calls, tx_n, dtc_n, fails, checks; static uint32_t nm_first_at;
static struct { uint16_t id; uint32_t ts; bool v; } rxq;
static uint32_t tx_at[64]; static uint16_t tx_id[64]; static bms_sample_t bms;

uint32_t hal_millis(void) { return now; }
bool hal_mcp_read_eflg(uint8_t *e) { *e = eflg; return true; }
bool hal_mcp_request_normal_mode(void) { if (nm_calls++ == 0) { nm_first_at = now; } if (clear_on_nm) { eflg = 0; } return nm_ok; }
bool hal_can_rx_poll(uint16_t *id, uint32_t *ts) { if (!rxq.v) { return false; } *id = rxq.id; *ts = rxq.ts; rxq.v = false; return true; }
bool hal_can_tx(uint16_t id, const uint8_t *d, uint8_t l) { (void)d; (void)l; if (tx_n < 64) { tx_id[tx_n] = id; tx_at[tx_n++] = now; } return true; }
bool hal_bms_get(bms_sample_t *s) { if (!bms_new) { return false; } *s = bms; return true; }
bool hal_vehicle_stationary(void) { return stationary; }
void hal_gpio4_write(bool l) { gpio4 = l; }
void dtc_set(uint16_t c) { (void)c; dtc_n++; }
void build_vcu_frame(uint8_t o[8]) { (void)memset(o, 0, 8); }
void hal_spi_xfer(const uint8_t *t, uint8_t *r, uint8_t n) { (void)t; (void)r; (void)n; }

#define CHECK(c) do { checks++; if (!(c)) { fails++; printf("  FAIL %s:%d  %s\n", __FILE__, __LINE__, #c); } } while (0)

static void reset(uint32_t start)
{
    now = start; eflg = 0; gpio4 = true; nm_ok = true; bms_new = true; stationary = true;
    clear_on_nm = false; nm_calls = 0; tx_n = 0; dtc_n = 0; nm_first_at = 0; rxq.v = false;
    bms.soc_pct = 50; bms.cell_ma = 0; bms.valid = true;
    mshield_init();
}
/* avanza 1 ms por paso; alimenta 0x100 cada 100 ms si feed */
static void run(uint32_t start, uint32_t ms, bool feed)
{
    uint32_t i;
    for (i = 0; i < ms; i++) {
        now++;
        if (feed && ((uint32_t)(now - start) % 100U) == 0U) { rxq.id = ID_BMS; rxq.ts = now; rxq.v = true; }
        mshield_step();
    }
}

static void t_busoff(uint32_t start)   /* [REQ-01] */
{
    uint32_t t0; int k;
    printf("T-BO-1 reintentos a 500 ms x3 -> FAILED (start=%lu)\n", (unsigned long)start);
    reset(start); eflg = 0x20U;
    run(start, 1, true); t0 = now;
    CHECK(mshield_bus_state() == MS_BO_WAIT);
    run(start, 498, true);  CHECK(nm_calls == 0);                 /* antes de 500 ms: nada */
    run(start, 5, true);    CHECK(nm_calls == 1);
    CHECK((uint32_t)(nm_first_at - t0) >= BO_RETRY_MS && (uint32_t)(nm_first_at - t0) <= BO_RETRY_MS + 2U);
    for (k = 0; k < 4; k++) { run(start, 700, true); }
    CHECK(nm_calls == (int)BO_MAX_RETRIES);                       /* maximo 3 */
    CHECK(mshield_bus_state() == MS_BO_FAILED);
    CHECK(gpio4 == GPIO4_CUT_LEVEL);                              /* falla latched => corte */
}
static void t_stable(void)             /* [REQ-01] */
{
    printf("T-BO-2 estabiliza -> OK sin corte\n");
    reset(1000); eflg = 0x20U; clear_on_nm = true;   /* el bus se recupera tras el 1er reintento */
    run(1000, 2700, true);
    CHECK(nm_calls == 1); CHECK(mshield_bus_state() == MS_BO_OK);
    CHECK(gpio4 == GPIO4_RUN_LEVEL); CHECK(!mshield_safecut_active());
}
static void t_slot(void)               /* [REQ-02] */
{
    mshield_stats_t s; uint32_t i;
    printf("T-TX-1 0x101 a +20 ms de 0x100\n");
    reset(1000); now = 1000; rxq.id = ID_BMS; rxq.ts = 1000; rxq.v = true;
    mshield_step();
    for (i = 1; i < 20; i++) { now++; mshield_step(); }
    CHECK(tx_n == 0);
    now++; mshield_step();
    CHECK(tx_n == 1); CHECK(tx_id[0] == ID_VCU); CHECK(tx_at[0] == 1020U);
    s = mshield_get_stats(); CHECK(s.lat_last_ms == TX_OFFSET_MS);
}
static void t_bmsloss(void)            /* [REQ-02][REQ-03] */
{
    printf("T-TX-2 perdida de BMS -> DTC + corte (decision de diseno)\n");
    reset(1000); run(1000, BMS_LOSS_MS + 5U, false);
    CHECK(dtc_n >= 1); CHECK(gpio4 == GPIO4_CUT_LEVEL);
}
static void t_soc(void)                /* [REQ-03][REQ-04] */
{
    printf("T-SC-1 SOC<20 debounce, latch, histeresis\n");
    reset(1000); bms.soc_pct = 19; run(1000, 2, true); CHECK(gpio4 == GPIO4_RUN_LEVEL);
    run(1000, 1, true); CHECK(gpio4 == GPIO4_CUT_LEVEL);
    bms.soc_pct = 21; run(1000, 10, true); CHECK(gpio4 == GPIO4_CUT_LEVEL);   /* latch */
    bms.soc_pct = 25; run(1000, 2, true); CHECK(gpio4 == GPIO4_CUT_LEVEL);
    run(1000, 1, true); CHECK(gpio4 == GPIO4_RUN_LEVEL);
    printf("T-SC-2 dato invalido / sobrecorriente de celda\n");
    reset(1000); bms.valid = false; run(1000, 1, true); CHECK(gpio4 == GPIO4_CUT_LEVEL);
    reset(1000); bms.cell_ma = (int16_t)(CELL_I_LIMIT_MA + 1); run(1000, 1, true); CHECK(gpio4 == GPIO4_CUT_LEVEL);
}
static void t_ota(void)                /* [REQ-05] */
{
    printf("T-OTA-1 compuerta OTA\n");
    reset(1000); run(1000, 5, true); CHECK(mshield_ota_allowed());
    stationary = false; CHECK(!mshield_ota_allowed());
    stationary = true; bms.soc_pct = 10; run(1000, 5, true); CHECK(!mshield_ota_allowed());
}

int main(void)
{
    t_busoff(1000U); t_busoff(0xFFFFFF00U);    /* incluye wrap-around de millis */
    t_stable(); t_slot(); t_bmsloss(); t_soc(); t_ota();
    printf("\n%d checks, %d fallos\n", checks, fails);
    return fails ? 1 : 0;
}
