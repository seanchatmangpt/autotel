#include "cns/bitactor.h"
#include <stdint.h>

// Simple mean-reversion strategy: if price > 100, sell; else buy
void on_tick(void *state, const void *msg, size_t msg_len)
{
  const uint8_t *tick = (const uint8_t *)msg;
  uint8_t price = tick[0];
  uint8_t order[8] = {0};
  if (price > 100)
  {
    order[0] = 0x02; // sell
    order[3] = price;
    order[7] = 10; // qty
  }
  else
  {
    order[0] = 0x01; // buy
    order[3] = price;
    order[7] = 10; // qty
  }
  bitactor_pid_t ob_pid = registry_lookup("orderbook");
  bitactor_message_t order_msg = {.type = BITACTOR_CALL, .payload = order, .payload_len = 8};
  bitactor_message_t ob_reply;
  bitactor_call(NULL, ob_pid, &order_msg, &ob_reply);
}

void on_orderbook(void *state, const void *msg, size_t msg_len)
{
  // Handle fills, update state, etc. (omitted for brevity)
}

BITACTOR_BEHAVIOR(strategy, on_tick, on_orderbook)