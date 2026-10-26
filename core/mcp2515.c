/* mcp2515.c - driver minimo. TXBO vive en EFLG (0x2D), NO en CANINTF (0x2C).
 * Bitrate 500 kbps con cristal de 8 MHz: CNF1=0x00 CNF2=0x90 CNF3=0x02. */
#include "vcu_mshield.h"

#define OP_RESET 0xC0U
#define OP_READ  0x03U
#define OP_WRITE 0x02U
#define OP_BITM  0x05U
#define OP_RTS0  0x81U
#define R_CANSTAT 0x0EU
#define R_CANCTRL 0x0FU
#define R_CNF3 0x28U
#define R_CNF2 0x29U
#define R_CNF1 0x2AU
#define R_CANINTF 0x2CU
#define R_EFLG 0x2DU
#define R_TXB0CTRL 0x30U
#define R_TXB0SIDH 0x31U
#define R_RXB0CTRL 0x60U
#define R_RXB0SIDH 0x61U
#define MODE_MASK 0xE0U

static uint8_t rd(uint8_t reg)
{ uint8_t t[3] = { OP_READ, 0, 0 }, r[3] = { 0, 0, 0 }; t[1] = reg; hal_spi_xfer(t, r, 3U); return r[2]; }
static void wr(uint8_t reg, uint8_t v)
{ uint8_t t[3] = { OP_WRITE, 0, 0 }; t[1] = reg; t[2] = v; hal_spi_xfer(t, 0, 3U); }
static void bm(uint8_t reg, uint8_t mask, uint8_t v)
{ uint8_t t[4] = { OP_BITM, 0, 0, 0 }; t[1] = reg; t[2] = mask; t[3] = v; hal_spi_xfer(t, 0, 4U); }

/* Solo en arranque (fuera del lazo ciclico): espera acotada por iteraciones. */
bool mcp2515_init(void)
{
    uint8_t r = OP_RESET; uint16_t n;
    hal_spi_xfer(&r, 0, 1U);
    for (n = 0U; n < 1000U; n++) { if ((rd(R_CANSTAT) & MODE_MASK) == 0x80U) { break; } }
    if ((rd(R_CANSTAT) & MODE_MASK) != 0x80U) { return false; }
    wr(R_CNF1, 0x00U); wr(R_CNF2, 0x90U); wr(R_CNF3, 0x02U);
    wr(R_RXB0CTRL, 0x60U);                    /* acepta todo (filtros a definir) */
    bm(R_CANCTRL, MODE_MASK, 0x00U);          /* solicitar modo normal */
    for (n = 0U; n < 1000U; n++) { if ((rd(R_CANSTAT) & MODE_MASK) == 0x00U) { return true; } }
    return false;
}

bool hal_mcp_read_eflg(uint8_t *e) { *e = rd(R_EFLG); return true; }

bool hal_mcp_request_normal_mode(void)
{
    bm(R_CANCTRL, MODE_MASK, 0x00U);
    return (rd(R_CANSTAT) & MODE_MASK) == 0x00U;
}

bool hal_can_tx(uint16_t id, const uint8_t *d, uint8_t len)
{
    uint8_t t[16]; uint8_t i; uint8_t rts = OP_RTS0;
    if (len > 8U) { return false; }
    if ((rd(R_TXB0CTRL) & 0x08U) != 0U) { return false; }       /* TXREQ: ocupado */
    t[0] = OP_WRITE; t[1] = R_TXB0SIDH;
    t[2] = (uint8_t)(id >> 3); t[3] = (uint8_t)((id & 7U) << 5);
    t[4] = 0U; t[5] = 0U; t[6] = len;                           /* EID8, EID0, DLC */
    for (i = 0U; i < len; i++) { t[7U + i] = d[i]; }
    hal_spi_xfer(t, 0, (uint8_t)(7U + len));
    hal_spi_xfer(&rts, 0, 1U);
    return true;
}

bool hal_can_rx_poll(uint16_t *id, uint32_t *ts_ms)
{
    uint8_t h, l;
    if ((rd(R_CANINTF) & 0x01U) == 0U) { return false; }        /* RX0IF */
    h = rd(R_RXB0SIDH); l = rd((uint8_t)(R_RXB0SIDH + 1U));
    *id = (uint16_t)(((uint16_t)h << 3) | (uint16_t)(l >> 5));
    *ts_ms = hal_millis();       /* resolucion = periodo de polling; ideal: timestamp en ISR de INT */
    bm(R_CANINTF, 0x01U, 0x00U);
    return true;
}
