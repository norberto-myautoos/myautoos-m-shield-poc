#include "vcu_diagnostics.h"
#include <string.h>

static struct { uint32_t flags, ms_since_bms, slow_ticks; bool rx; } d;

void diag_init(void) { (void)memset(&d, 0, sizeof d); }
void diag_bms_msg_received(void) { d.rx = true; }

void diag_task_10ms(void)
{
    if (d.rx) { d.ms_since_bms = 0U; d.rx = false; }
    else      { d.ms_since_bms += 10U; }
    if (d.ms_since_bms >= DIAG_BMS_TIMEOUT_MS) { d.flags |= DIAG_CAN_TIMEOUT_ERROR; }
}
void diag_task_100ms(void) { d.slow_ticks++; }
void diag_report_fault(uint32_t flag) { d.flags |= flag; }
uint32_t diag_get_flags(void) { return d.flags; }
bool diag_is_critical_fault(void) { return d.flags != 0U; }
bool diag_is_hard_fault(void) { return (d.flags & ~(uint32_t)DIAG_CAN_TIMEOUT_ERROR) != 0U; }
bool diag_is_degradable_fault(void) { return ((d.flags & DIAG_CAN_TIMEOUT_ERROR) != 0U) && !diag_is_hard_fault(); }
