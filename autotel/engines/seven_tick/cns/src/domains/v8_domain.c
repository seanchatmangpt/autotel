#include "cns/cli.h"
#include "cns/v8_physics.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// V8 universe instantiation command
static int cmd_v8_instantiate(CNSContext *ctx, int argc, char **argv)
{
  uint64_t start = cns_get_cycles();

  cns_cli_info("🌌 Instantiating CNS v8 Universe...");

  // Check if instantiation script exists
  const char *script_path = "v8_universe_instantiate.py";
  if (access(script_path, F_OK) != 0)
  {
    cns_cli_error("Instantiation script not found: %s", script_path);
    return CNS_ERR_NOT_FOUND;
  }

  // Run the instantiation script
  char cmd[512];
  snprintf(cmd, sizeof(cmd), "python3 %s", script_path);

  cns_cli_info("Running: %s", cmd);
  int result = system(cmd);

  if (result == 0)
  {
    cns_cli_success("✅ CNS v8 Universe instantiated successfully");
  }
  else
  {
    cns_cli_error("❌ Universe instantiation failed (exit code: %d)", result);
    return CNS_ERR_INTERNAL;
  }

  // Verify 7-tick constraint
  cns_assert_cycles(start, 7);
  return CNS_OK;
}

// V8 physics validation command
static int cmd_v8_validate(CNSContext *ctx, int argc, char **argv)
{
  uint64_t start = cns_get_cycles();

  cns_cli_info("🔬 Validating CNS v8 Physics...");

  // Initialize v8 physics
  if (cns_v8_physics_init() != CNS_V8_OK)
  {
    cns_cli_error("Failed to initialize v8 physics");
    return CNS_ERR_INTERNAL;
  }

  // Run trinity validation
  cns_v8_trinity_t trinity = {0};
  if (cns_v8_trinity_validate(&trinity) != CNS_V8_OK)
  {
    cns_cli_error("Trinity validation failed");
    return CNS_ERR_INTERNAL;
  }

  cns_cli_success("✅ V8 Physics Validation Results:");
  cns_cli_info("   8T (Temporal): %s", trinity.temporal_valid ? "✓" : "✗");
  cns_cli_info("   8H (Cognitive): %s", trinity.cognitive_valid ? "✓" : "✗");
  cns_cli_info("   8M (Memory): %s", trinity.memory_valid ? "✓" : "✗");

  if (trinity.temporal_valid && trinity.cognitive_valid && trinity.memory_valid)
  {
    cns_cli_success("🎉 All Trinity contracts validated successfully");
  }
  else
  {
    cns_cli_error("❌ Some Trinity contracts failed validation");
    return CNS_ERR_INTERNAL;
  }

  // Verify 7-tick constraint
  cns_assert_cycles(start, 7);
  return CNS_OK;
}

// V8 reasoner command
static int cmd_v8_reasoner(CNSContext *ctx, int argc, char **argv)
{
  uint64_t start = cns_get_cycles();

  if (argc < 1)
  {
    cns_cli_error("Usage: v8 reasoner <ttl_file>");
    return CNS_ERR_INVALID_ARG;
  }

  const char *ttl_file = argv[0];
  cns_cli_info("🧠 Running AOT Reasoner on: %s", ttl_file);

  // Initialize reasoner
  cns_v8_reasoner_t reasoner = {0};
  if (cns_v8_reasoner_init(&reasoner) != CNS_V8_OK)
  {
    cns_cli_error("Failed to initialize AOT reasoner");
    return CNS_ERR_INTERNAL;
  }

  // Parse TTL and generate C code
  if (cns_v8_reasoner_compile(&reasoner, ttl_file) != CNS_V8_OK)
  {
    cns_cli_error("Failed to compile TTL to C");
    return CNS_ERR_INTERNAL;
  }

  cns_cli_success("✅ AOT Reasoner completed successfully");

  // Cleanup
  cns_v8_reasoner_cleanup(&reasoner);

  // Verify 7-tick constraint
  cns_assert_cycles(start, 7);
  return CNS_OK;
}

// V8 weaver command
static int cmd_v8_weaver(CNSContext *ctx, int argc, char **argv)
{
  uint64_t start = cns_get_cycles();

  cns_cli_info("🕸️  Running Permutation Weaver...");

  // Initialize weaver
  cns_v8_weaver_t weaver = {0};
  if (cns_v8_weaver_init(&weaver) != CNS_V8_OK)
  {
    cns_cli_error("Failed to initialize permutation weaver");
    return CNS_ERR_INTERNAL;
  }

  // Run invariance validation
  cns_v8_weaver_result_t result = {0};
  if (cns_v8_weaver_validate(&weaver, &result) != CNS_V8_OK)
  {
    cns_cli_error("Weaver validation failed");
    return CNS_ERR_INTERNAL;
  }

  cns_cli_success("✅ Permutation Weaver Results:");
  cns_cli_info("   Permutations tested: %u", result.permutations_tested);
  cns_cli_info("   Invariance verified: %s", result.invariance_verified ? "✓" : "✗");
  cns_cli_info("   Chaos tolerance: %.2f%%", result.chaos_tolerance * 100.0);

  if (result.invariance_verified)
  {
    cns_cli_success("🎉 System invariance validated under controlled chaos");
  }
  else
  {
    cns_cli_error("❌ System failed invariance validation");
    return CNS_ERR_INTERNAL;
  }

  // Cleanup
  cns_v8_weaver_cleanup(&weaver);

  // Verify 7-tick constraint
  cns_assert_cycles(start, 7);
  return CNS_OK;
}

// V8 status command
static int cmd_v8_status(CNSContext *ctx, int argc, char **argv)
{
  uint64_t start = cns_get_cycles();

  cns_cli_success("🌌 CNS v8 Universe Status:");

  // Check physics initialization
  cns_cli_info("   Physics: %s", cns_v8_physics_initialized() ? "✓ Initialized" : "✗ Not initialized");

  // Check reasoner availability
  cns_cli_info("   AOT Reasoner: %s", cns_v8_reasoner_available() ? "✓ Available" : "✗ Not available");

  // Check weaver availability
  cns_cli_info("   Permutation Weaver: %s", cns_v8_weaver_available() ? "✓ Available" : "✗ Not available");

  // Check instantiation script
  const char *script_path = "v8_universe_instantiate.py";
  cns_cli_info("   Instantiation Script: %s", access(script_path, F_OK) == 0 ? "✓ Found" : "✗ Not found");

  // Verify 7-tick constraint
  cns_assert_cycles(start, 7);
  return CNS_OK;
}

// V8 commands
static CNSCommand v8_commands[] = {
    {.name = "instantiate",
     .description = "Instantiate the CNS v8 universe with 8T/8H/8M physics",
     .handler = cmd_v8_instantiate,
     .options = NULL,
     .option_count = 0,
     .arguments = NULL,
     .argument_count = 0},
    {.name = "validate",
     .description = "Validate CNS v8 physics and Trinity contracts",
     .handler = cmd_v8_validate,
     .options = NULL,
     .option_count = 0,
     .arguments = NULL,
     .argument_count = 0},
    {.name = "reasoner",
     .description = "Run AOT reasoner to compile TTL to C code",
     .handler = cmd_v8_reasoner,
     .options = NULL,
     .option_count = 0,
     .arguments = (CNSArgument[]){
         {.name = "ttl_file", .description = "TTL ontology file", .required = true}},
     .argument_count = 1},
    {.name = "weaver", .description = "Run permutation weaver for invariance validation", .handler = cmd_v8_weaver, .options = NULL, .option_count = 0, .arguments = NULL, .argument_count = 0},
    {.name = "status", .description = "Show CNS v8 universe status and component availability", .handler = cmd_v8_status, .options = NULL, .option_count = 0, .arguments = NULL, .argument_count = 0}};

// V8 domain
CNSDomain cns_v8_domain = {
    .name = "v8",
    .description = "CNS v8 Universe with 8T/8H/8M Physics and AOT Reasoner",
    .commands = v8_commands,
    .command_count = sizeof(v8_commands) / sizeof(v8_commands[0])};