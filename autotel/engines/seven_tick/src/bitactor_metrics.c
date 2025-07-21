#include "cns/bitactor.h"
#include <stdint.h>
#include <stdio.h>

static int64_t pnl = 0;

void on_event(void *state, const void *msg, size_t msg_len)
{
  const uint8_t *fill = (const uint8_t *)msg;
  int side = fill[0];
  int price = fill[3];
  int qty = fill[7];
  if (side == 0x01)
    pnl -= price * qty; // buy
  else if (side == 0x02)
    pnl += price * qty; // sell
}

void on_call(void *state, const void *msg, size_t msg_len, bitactor_message_t *reply)
{
  static char result[64];
  snprintf(result, sizeof(result), "PnL: %lld", (long long)pnl);
  reply->payload = result;
  reply->payload_len = strlen(result) + 1;
}

BITACTOR_BEHAVIOR(metrics, on_event, on_call)