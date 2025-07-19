/*  ─────────────────────────────────────────────────────────────
    test_cns_core.c  –  CNS Core Unit Tests (v1.0)
    7-tick compliant test framework for CNS core functionality
    ───────────────────────────────────────────────────────────── */

#include "../include/cns.h"
#include "../include/s7t.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/*═══════════════════════════════════════════════════════════════
  Test Infrastructure
  ═══════════════════════════════════════════════════════════════*/

// Test context
typedef struct
{
  uint64_t test_start;
  uint64_t test_cycles;
  bool test_passed;
  const char *test_name;
} test_context_t;

// Test result tracking
static uint32_t tests_run = 0;
static uint32_t tests_passed = 0;
static uint32_t tests_failed = 0;

// Test macros
#define TEST_BEGIN(name)          \
  test_context_t ctx = {          \
      .test_start = s7t_cycles(), \
      .test_name = name,          \
      .test_passed = true};       \
  printf("TEST: %s\n", name)

#define TEST_END()                                        \
  ctx.test_cycles = s7t_cycles() - ctx.test_start;        \
  tests_run++;                                            \
  if (ctx.test_passed)                                    \
  {                                                       \
    tests_passed++;                                       \
    printf("  ✓ PASSED (%lu cycles)\n", ctx.test_cycles); \
  }                                                       \
  else                                                    \
  {                                                       \
    tests_failed++;                                       \
    printf("  ✗ FAILED (%lu cycles)\n", ctx.test_cycles); \
  }

#define TEST_ASSERT(condition)                     \
  if (!(condition))                                \
  {                                                \
    ctx.test_passed = false;                       \
    printf("    ASSERT FAILED: %s\n", #condition); \
  }

#define TEST_ASSERT_CYCLES(max_cycles)                                       \
  if (ctx.test_cycles > max_cycles)                                          \
  {                                                                          \
    ctx.test_passed = false;                                                 \
    printf("    CYCLES EXCEEDED: %lu > %lu\n", ctx.test_cycles, max_cycles); \
  }

/*═══════════════════════════════════════════════════════════════
  Mock Command Handlers
  ═══════════════════════════════════════════════════════════════*/

// Simple echo handler
static cns_result_t mock_echo_handler(const cns_command_t *cmd, void *context)
{
  (void)cmd;
  (void)context;
  return CNS_OK;
}

// Admin handler
static cns_result_t mock_admin_handler(const cns_command_t *cmd, void *context)
{
  (void)cmd;
  (void)context;
  return CNS_OK;
}

// Error handler
static cns_result_t mock_error_handler(const cns_command_t *cmd, void *context)
{
  (void)cmd;
  (void)context;
  return CNS_ERR_INTERNAL;
}

/*═══════════════════════════════════════════════════════════════
  Core Functionality Tests
  ═══════════════════════════════════════════════════════════════*/

// Test CNS engine initialization
static void test_cns_init()
{
  TEST_BEGIN("CNS Engine Initialization");

  // Allocate test structures
  cns_cmd_entry_t commands[10];
  uint32_t hash_table[256];
  cns_engine_t engine;

  // Initialize engine
  cns_init(&engine, commands, hash_table, 10);

  // Verify initialization
  TEST_ASSERT(engine.commands == commands);
  TEST_ASSERT(engine.hash_table == hash_table);
  TEST_ASSERT(engine.cmd_count == 0);
  TEST_ASSERT(engine.max_commands == 10);
  TEST_ASSERT(engine.table_size == 256);

  // Verify hash table cleared
  bool all_cleared = true;
  for (uint32_t i = 0; i < engine.table_size; i++)
  {
    if (hash_table[i] != CNS_MAX_COMMANDS)
    {
      all_cleared = false;
      break;
    }
  }
  TEST_ASSERT(all_cleared);

  // Verify performance counter initialized
  TEST_ASSERT(engine.perf.min_cycles == UINT64_MAX);
  TEST_ASSERT(engine.perf.max_cycles == 0);

  TEST_ASSERT_CYCLES(100); // Should be very fast
  TEST_END();
}

// Test command registration
static void test_cns_register()
{
  TEST_BEGIN("CNS Command Registration");

  cns_cmd_entry_t commands[10];
  uint32_t hash_table[256];
  cns_engine_t engine;
  cns_init(&engine, commands, hash_table, 10);

  // Register a command
  cns_result_t result = cns_register(
      &engine, "test", mock_echo_handler,
      CNS_FLAG_NONE, 0, 2, "Test command");

  TEST_ASSERT(result == CNS_OK);
  TEST_ASSERT(engine.cmd_count == 1);
  TEST_ASSERT(commands[0].handler == mock_echo_handler);
  TEST_ASSERT(strcmp(commands[0].name, "test") == 0);
  TEST_ASSERT(strcmp(commands[0].help, "Test command") == 0);
  TEST_ASSERT(commands[0].min_args == 0);
  TEST_ASSERT(commands[0].max_args == 2);
  TEST_ASSERT(commands[0].flags == CNS_FLAG_NONE);

  // Register admin command
  result = cns_register(
      &engine, "admin", mock_admin_handler,
      CNS_FLAG_ADMIN, 1, 3, "Admin command");

  TEST_ASSERT(result == CNS_OK);
  TEST_ASSERT(engine.cmd_count == 2);
  TEST_ASSERT(commands[1].flags == CNS_FLAG_ADMIN);

  TEST_ASSERT_CYCLES(200);
  TEST_END();
}

// Test command lookup
static void test_cns_lookup()
{
  TEST_BEGIN("CNS Command Lookup");

  cns_cmd_entry_t commands[10];
  uint32_t hash_table[256];
  cns_engine_t engine;
  cns_init(&engine, commands, hash_table, 10);

  // Register commands
  cns_register(&engine, "test", mock_echo_handler, CNS_FLAG_NONE, 0, 2, "Test");
  cns_register(&engine, "admin", mock_admin_handler, CNS_FLAG_ADMIN, 1, 3, "Admin");

  // Lookup existing commands
  uint32_t test_hash = s7t_hash_string("test", 4);
  uint32_t admin_hash = s7t_hash_string("admin", 5);

  cns_cmd_entry_t *entry = cns_lookup(&engine, test_hash);
  TEST_ASSERT(entry != NULL);
  TEST_ASSERT(entry->handler == mock_echo_handler);

  entry = cns_lookup(&engine, admin_hash);
  TEST_ASSERT(entry != NULL);
  TEST_ASSERT(entry->handler == mock_admin_handler);

  // Lookup non-existent command
  uint32_t invalid_hash = s7t_hash_string("invalid", 7);
  entry = cns_lookup(&engine, invalid_hash);
  TEST_ASSERT(entry == NULL);

  TEST_ASSERT_CYCLES(300);
  TEST_END();
}

// Test command execution
static void test_cns_execute()
{
  TEST_BEGIN("CNS Command Execution");

  cns_cmd_entry_t commands[10];
  uint32_t hash_table[256];
  cns_engine_t engine;
  cns_init(&engine, commands, hash_table, 10);

  // Register command
  cns_register(&engine, "test", mock_echo_handler, CNS_FLAG_NONE, 0, 2, "Test");

  // Create test command
  cns_command_t cmd = {0};
  strcpy(cmd.cmd, "test");
  cmd.hash = s7t_hash_string("test", 4);
  cmd.argc = 1;
  strcpy(cmd.args[0], "arg1");
  cmd.flags = CNS_FLAG_NONE;

  // Execute command
  cns_result_t result = cns_execute(&engine, &cmd);
  TEST_ASSERT(result == CNS_OK);

  // Verify performance tracking
  TEST_ASSERT(engine.perf.count > 0);
  TEST_ASSERT(engine.perf.cycles > 0);

  TEST_ASSERT_CYCLES(500);
  TEST_END();
}

// Test argument validation
static void test_cns_arg_validation()
{
  TEST_BEGIN("CNS Argument Validation");

  cns_cmd_entry_t commands[10];
  uint32_t hash_table[256];
  cns_engine_t engine;
  cns_init(&engine, commands, hash_table, 10);

  // Register command with specific arg requirements
  cns_register(&engine, "test", mock_echo_handler, CNS_FLAG_NONE, 1, 3, "Test");

  // Test too few arguments
  cns_command_t cmd = {0};
  strcpy(cmd.cmd, "test");
  cmd.hash = s7t_hash_string("test", 4);
  cmd.argc = 0;
  cmd.flags = CNS_FLAG_NONE;

  cns_result_t result = cns_execute(&engine, &cmd);
  TEST_ASSERT(result == CNS_ERR_INVALID_ARG);

  // Test too many arguments
  cmd.argc = 4;
  strcpy(cmd.args[0], "arg1");
  strcpy(cmd.args[1], "arg2");
  strcpy(cmd.args[2], "arg3");
  strcpy(cmd.args[3], "arg4");

  result = cns_execute(&engine, &cmd);
  TEST_ASSERT(result == CNS_ERR_INVALID_ARG);

  // Test valid argument count
  cmd.argc = 2;
  result = cns_execute(&engine, &cmd);
  TEST_ASSERT(result == CNS_OK);

  TEST_ASSERT_CYCLES(400);
  TEST_END();
}

// Test permission validation
static void test_cns_permission_validation()
{
  TEST_BEGIN("CNS Permission Validation");

  cns_cmd_entry_t commands[10];
  uint32_t hash_table[256];
  cns_engine_t engine;
  cns_init(&engine, commands, hash_table, 10);

  // Register admin command
  cns_register(&engine, "admin", mock_admin_handler, CNS_FLAG_ADMIN, 0, 2, "Admin");

  // Test without admin flag
  cns_command_t cmd = {0};
  strcpy(cmd.cmd, "admin");
  cmd.hash = s7t_hash_string("admin", 5);
  cmd.argc = 0;
  cmd.flags = CNS_FLAG_NONE;

  cns_result_t result = cns_execute(&engine, &cmd);
  TEST_ASSERT(result == CNS_ERR_PERMISSION);

  // Test with admin flag
  cmd.flags = CNS_FLAG_ADMIN;
  result = cns_execute(&engine, &cmd);
  TEST_ASSERT(result == CNS_OK);

  TEST_ASSERT_CYCLES(300);
  TEST_END();
}

// Test quick validation
static void test_cns_validate_quick()
{
  TEST_BEGIN("CNS Quick Validation");

  // Test valid commands
  TEST_ASSERT(cns_validate_quick("test", 4) == true);
  TEST_ASSERT(cns_validate_quick("a", 1) == true);
  TEST_ASSERT(cns_validate_quick("very_long_command_name", 22) == true);

  // Test invalid commands
  TEST_ASSERT(cns_validate_quick("", 0) == false);
  TEST_ASSERT(cns_validate_quick(NULL, 0) == false);

  // Test boundary conditions
  char long_cmd[CNS_MAX_CMD_LEN + 1];
  memset(long_cmd, 'a', CNS_MAX_CMD_LEN);
  long_cmd[CNS_MAX_CMD_LEN] = '\0';
  TEST_ASSERT(cns_validate_quick(long_cmd, CNS_MAX_CMD_LEN) == false);

  TEST_ASSERT_CYCLES(50); // Should be very fast
  TEST_END();
}

// Test resource limits
static void test_cns_resource_limits()
{
  TEST_BEGIN("CNS Resource Limits");

  cns_cmd_entry_t commands[2];
  uint32_t hash_table[256];
  cns_engine_t engine;
  cns_init(&engine, commands, hash_table, 2);

  // Register up to limit
  cns_result_t result = cns_register(&engine, "cmd1", mock_echo_handler, CNS_FLAG_NONE, 0, 1, "Cmd1");
  TEST_ASSERT(result == CNS_OK);

  result = cns_register(&engine, "cmd2", mock_echo_handler, CNS_FLAG_NONE, 0, 1, "Cmd2");
  TEST_ASSERT(result == CNS_OK);

  // Try to exceed limit
  result = cns_register(&engine, "cmd3", mock_echo_handler, CNS_FLAG_NONE, 0, 1, "Cmd3");
  TEST_ASSERT(result == CNS_ERR_RESOURCE);

  TEST_ASSERT_CYCLES(200);
  TEST_END();
}

// Test performance tracking
static void test_cns_performance_tracking()
{
  TEST_BEGIN("CNS Performance Tracking");

  cns_cmd_entry_t commands[10];
  uint32_t hash_table[256];
  cns_engine_t engine;
  cns_init(&engine, commands, hash_table, 10);

  cns_register(&engine, "test", mock_echo_handler, CNS_FLAG_NONE, 0, 1, "Test");

  // Execute multiple commands
  for (int i = 0; i < 5; i++)
  {
    cns_command_t cmd = {0};
    strcpy(cmd.cmd, "test");
    cmd.hash = s7t_hash_string("test", 4);
    cmd.argc = 0;
    cmd.flags = CNS_FLAG_NONE;

    cns_execute(&engine, &cmd);
  }

  // Verify performance stats
  TEST_ASSERT(engine.perf.count == 5);
  TEST_ASSERT(engine.perf.cycles > 0);
  TEST_ASSERT(engine.perf.min_cycles > 0);
  TEST_ASSERT(engine.perf.max_cycles > 0);
  TEST_ASSERT(engine.perf.max_cycles >= engine.perf.min_cycles);

  TEST_ASSERT_CYCLES(1000);
  TEST_END();
}

/*═══════════════════════════════════════════════════════════════
  Test Runner
  ═══════════════════════════════════════════════════════════════*/

int main()
{
  printf("CNS Core Unit Tests\n");
  printf("==================\n\n");

  // Initialize S7T
  s7t_init();

  // Run tests
  test_cns_init();
  test_cns_register();
  test_cns_lookup();
  test_cns_execute();
  test_cns_arg_validation();
  test_cns_permission_validation();
  test_cns_validate_quick();
  test_cns_resource_limits();
  test_cns_performance_tracking();

  // Print summary
  printf("\nTest Summary:\n");
  printf("  Total: %u\n", tests_run);
  printf("  Passed: %u\n", tests_passed);
  printf("  Failed: %u\n", tests_failed);
  printf("  Success Rate: %.1f%%\n",
         (float)tests_passed / tests_run * 100.0f);

  return (tests_failed == 0) ? 0 : 1;
}