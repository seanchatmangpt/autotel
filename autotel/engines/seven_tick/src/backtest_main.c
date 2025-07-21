#include "cns/bitactor.h"
#include "cns/bitactor_bridge.h"
#include "cns/cns_weaver.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

int main()
{
  // 1. Load compiled manifest
  actor_manifest_t manifest;
  load_actor_manifest_from_bin("build/backtest_manifest.bin", &manifest);

  // 2. Initialize conductor and registry
  bitactor_conductor_t conductor;
  bitactor_conductor_init(&conductor);

  // 3. Register actors
  actor_id_t md_id = registry_register("market_data", manifest.bytecode_buffer, manifest.bytecode_length);
  actor_id_t strat_id = registry_register("strategy", manifest.bytecode_buffer, manifest.bytecode_length);
  actor_id_t ob_id = registry_register("orderbook", manifest.bytecode_buffer, manifest.bytecode_length);
  actor_id_t risk_id = registry_register("risk", manifest.bytecode_buffer, manifest.bytecode_length);
  actor_id_t metrics_id = registry_register("metrics", manifest.bytecode_buffer, manifest.bytecode_length);

  // 4. Spawn actors
  bitactor_pid_t md_pid = bitactor_conductor_spawn(&conductor, md_id);
  bitactor_pid_t strat_pid = bitactor_conductor_spawn(&conductor, strat_id);
  bitactor_pid_t ob_pid = bitactor_conductor_spawn(&conductor, ob_id);
  bitactor_pid_t risk_pid = bitactor_conductor_spawn(&conductor, risk_id);
  bitactor_pid_t metrics_pid = bitactor_conductor_spawn(&conductor, metrics_id);

  // 5. Main backtest loop: stream historical ticks
  FILE *tick_file = fopen("data/historical_ticks.bin", "rb");
  if (!tick_file)
  {
    perror("tick_file");
    exit(1);
  }
  uint8_t tick[8];
  while (fread(tick, 1, 8, tick_file) == 8)
  {
    bitactor_message_t msg = {.type = BITACTOR_CAST, .payload = tick, .payload_len = 8};
    bitactor_cast(&conductor, md_pid, &msg);
    // Optionally throttle for real-time simulation
  }
  fclose(tick_file);

  // 6. Collect and print metrics
  bitactor_message_t metrics_req = {.type = BITACTOR_CALL, .payload = NULL, .payload_len = 0};
  bitactor_message_t metrics_reply;
  bitactor_call(&conductor, metrics_pid, &metrics_req, &metrics_reply);
  printf("Backtest Results: %s\n", (char *)metrics_reply.payload);

  bitactor_conductor_shutdown(&conductor);
  return 0;
}