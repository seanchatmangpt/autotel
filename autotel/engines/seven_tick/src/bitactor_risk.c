#include "cns/bitactor.h"
#include <stdint.h>

// Simple risk check: always approve
void check_order(void *state, const void *msg, size_t msg_len)
{
  // In production, check position, exposure, etc.
  // For now, always approve and forward to orderbook
  bitactor_pid_t ob_pid = registry_lookup("orderbook");
  bitactor_message_t order_msg = {.type = BITACTOR_CALL, .payload = msg, .payload_len = msg_len};
  bitactor_message_t ob_reply;
  bitactor_call(NULL, ob_pid, &order_msg, &ob_reply);
}

BITACTOR_BEHAVIOR(risk, check_order)