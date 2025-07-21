#include "cns/bitactor.h"
#include <stdint.h>

// Simple orderbook: always fills at requested price
void match_order(void *state, const void *msg, size_t msg_len)
{
  const uint8_t *order = (const uint8_t *)msg;
  uint8_t fill[8] = {0};
  fill[0] = order[0]; // side
  fill[3] = order[3]; // price
  fill[7] = order[7]; // qty
  // Notify strategy
  bitactor_pid_t strat_pid = registry_lookup("strategy");
  bitactor_message_t fill_msg = {.type = BITACTOR_CAST, .payload = fill, .payload_len = 8};
  bitactor_cast(NULL, strat_pid, &fill_msg);
  // Notify metrics
  bitactor_pid_t metrics_pid = registry_lookup("metrics");
  bitactor_cast(NULL, metrics_pid, &fill_msg);
}

BITACTOR_BEHAVIOR(orderbook, match_order)