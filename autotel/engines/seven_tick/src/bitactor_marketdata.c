#include "cns/bitactor.h"

// MarketDataActor: forwards each tick to the strategy actor
void on_tick(void *state, const void *msg, size_t msg_len)
{
  bitactor_pid_t strategy_pid = registry_lookup("strategy");
  bitactor_message_t fwd = {.type = BITACTOR_CAST, .payload = msg, .payload_len = msg_len};
  bitactor_cast(NULL, strategy_pid, &fwd);
}

// Register the behavior
BITACTOR_BEHAVIOR(market_data, on_tick)