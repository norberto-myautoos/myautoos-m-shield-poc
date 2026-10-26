/* vcu_mshield.h - MyAutoOS M-SHIELD PoC: configuracion + API + HAL.
 * ESTADO: PoC. Verificado en host con HAL simulado; sin evidencia HIL en este repo. */
#ifndef VCU_MSHIELD_H
#define VCU_MSHIELD_H
#include <stdint.h>
#include <stdbool.h>

/* ---- Pines (PoC en ESP32 clasico). NO validos en ESP32-S3:
 *      el S3 no tiene GPIO22-25 y GPIO19/20 son USB. Remapear con GPIO matrix. ---- */
#define PIN_SPI_SCK   18
#define PIN_SPI_MISO  19
#define PIN_SPI_MOSI  23
#define PIN_CAN_CS     5
#define PIN_SAFE_CUT   4      /* Active LOW = corte [REQ-04] */
#define SPI_FREQ_HZ   8000000UL

/* ---- Parametros calibrables (override con -D) ---- */
#ifndef BO_RETRY_MS
#define BO_RETRY_MS       500U   /* [REQ-01] */
#endif
#ifndef BO_MAX_RETRIES
#define BO_MAX_RETRIES    3U     /* [REQ-01] */
#endif
#ifndef BO_STABLE_MS
#define BO_STABLE_MS      2000U  /* [REQ-01] ventana de estabilidad (supuesto) */
#endif
#define ID_BMS            0x100U /* [REQ-02] */
#define ID_VCU            0x101U
#define TX_OFFSET_MS      20U    /* [REQ-02] slot tras trama BMS */
#define BMS_PERIOD_MS     100U
#define BMS_LOSS_MS       250U
#define SOC_CUT_PCT       20U    /* [REQ-03] */
#define SOC_RELEASE_PCT   25U
#define SOC_DEBOUNCE_N    3U
#define CELL_I_LIMIT_MA   1000   /* [REQ-03] confirmar unidad/valor de "Cell 34 1A 0.8k" */
#define GPIO4_CUT_LEVEL   false
#define GPIO4_RUN_LEVEL   true

typedef struct { uint8_t soc_pct; int16_t cell_ma; bool valid; } bms_sample_t;
typedef enum { MS_BO_OK, MS_BO_WAIT, MS_BO_RECOVER, MS_BO_VERIFY, MS_BO_FAILED } ms_bo_state_t;
typedef struct {
    uint32_t tx_count, lat_last_ms, lat_max_ms;      /* rx 0x100 -> tx 0x101 */
    uint32_t period_min_ms, period_max_ms;           /* periodo entre 0x101 */
    uint32_t busoff_events, cut_events;
} mshield_stats_t;

/* ---- API publica ---- */
void            mshield_init(void);
void            mshield_step(void);                  /* llamar cada 1 ms, no desde ISR */
bool            mshield_ota_allowed(void);           /* [REQ-05] */
bool            mshield_safecut_active(void);
ms_bo_state_t   mshield_bus_state(void);
mshield_stats_t mshield_get_stats(void);

/* ---- HAL: todas NO bloqueantes (false = no listo / cola llena) ---- */
extern uint32_t hal_millis(void);
extern bool hal_mcp_read_eflg(uint8_t *eflg);
extern bool hal_mcp_request_normal_mode(void);
extern bool hal_can_rx_poll(uint16_t *id, uint32_t *ts_ms);
extern bool hal_can_tx(uint16_t id, const uint8_t *d, uint8_t len);
extern bool hal_bms_get(bms_sample_t *s);
extern bool hal_vehicle_stationary(void);
/* Implementacion: fijar el latch en nivel RUN ANTES de configurar el pin como salida
 * (evita pulso LOW = corte espurio) y poner pull-up externo para el periodo de reset. */
extern void hal_gpio4_write(bool level);
extern void dtc_set(uint16_t code);
extern void build_vcu_frame(uint8_t out[8]);
/* SPI: CS incluido, transferencia corta y acotada (<=16 bytes @8 MHz) */
extern void hal_spi_xfer(const uint8_t *tx, uint8_t *rx, uint8_t len);

enum { DTC_BUSOFF = 0x0100, DTC_BUSOFF_LATCH, DTC_BMS_LOSS, DTC_SAFECUT };
#endif
