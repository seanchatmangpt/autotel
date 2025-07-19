/*  ─────────────────────────────────────────────────────────────
    test_cns_dispatch.c  –  CNS Dispatch Unit Tests (v1.0)
    7-tick compliant test framework for CNS dispatch functionality
    ───────────────────────────────────────────────────────────── */

#include "../include/cns_dispatch.h"
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

// Argument counting handler
static cns_result_t mock_arg_handler(const cns_command_t *cmd, void *context)
{
  (void)context;
  return (cns_result_t)cmd->argc; // Return argc as result for testing
}

/*═══════════════════════════════════════════════════════════════
  Dispatch Functionality Tests
  ═══════════════════════════════════════════════════════════════*/

// Test dispatch table initialization
static void test_dispatch_init()
{
  TEST_BEGIN("Dispatch Table Initialization");

  // Create test dispatch entries
  static const cns_dispatch_entry_t test_entries[] = {
      {"test", 0, mock_echo_handler, 0, 2, CNS_FLAG_NONE, "Test command", "test [arg1] [arg2]"},
      {"admin", 0, mock_admin_handler, 1, 3, CNS_FLAG_ADMIN, "Admin command", "admin arg1 [arg2] [arg3]"},
      {"error", 0, mock_error_handler, 0, 1, CNS_FLAG_NONE, "Error command", "error [arg1]"}};

  uint32_t hash_index[256];
  cns_dispatch_table_t table;

  // Initialize dispatch table
  cns_dispatch_init(&table, test_entries, 3, hash_index, 256);

  // Verify initialization
  TEST_ASSERT(table.entries == test_entries);
  TEST_ASSERT(table.count == 3);
  TEST_ASSERT(table.hash_index == hash_index);
  TEST_ASSERT(table.hash_size == 256);

  // Verify hash index cleared
  bool all_cleared = true;
  for (uint32_t i = 0; i < table.hash_size; i++)
  {
    if (hash_index[i] != 0xFFFFFFFF)
    {
      all_cleared = false;
      break;
    }
  }
  TEST_ASSERT(all_cleared);

  TEST_ASSERT_CYCLES(200);
  TEST_END();
}

// Test dispatch lookup
static void test_dispatch_lookup()
{
  TEST_BEGIN("Dispatch Lookup");

  // Create test dispatch entries with pre-computed hashes
  static const cns_dispatch_entry_t test_entries[] = {
      {"test", s7t_hash_string("test", 4), mock_echo_handler, 0, 2, CNS_FLAG_NONE, "Test command", "test [arg1] [arg2]"},
      {"admin", s7t_hash_string("admin", 5), mock_admin_handler, 1, 3, CNS_FLAG_ADMIN, "Admin command", "admin arg1 [arg2] [arg3]"},
      {"error", s7t_hash_string("error", 5), mock_error_handler, 0, 1, CNS_FLAG_NONE, "Error command", "error [arg1]"}};

  uint32_t hash_index[256];
  cns_dispatch_table_t table;
  cns_dispatch_init(&table, test_entries, 3, hash_index, 256);

  // Lookup existing commands
  const cns_dispatch_entry_t *entry;

  entry = cns_dispatch_lookup(&table, s7t_hash_string("test", 4));
  TEST_ASSERT(entry != NULL);
  TEST_ASSERT(entry->handler == mock_echo_handler);
  TEST_ASSERT(strcmp(entry->name, "test") == 0);

  entry = cns_dispatch_lookup(&table, s7t_hash_string("admin", 5));
  TEST_ASSERT(entry != NULL);
  TEST_ASSERT(entry->handler == mock_admin_handler);
  TEST_ASSERT(strcmp(entry->name, "admin") == 0);
  TEST_ASSERT(entry->flags == CNS_FLAG_ADMIN);

  entry = cns_dispatch_lookup(&table, s7t_hash_string("error", 5));
  TEST_ASSERT(entry != NULL);
  TEST_ASSERT(entry->handler == mock_error_handler);
  TEST_ASSERT(strcmp(entry->name, "error") == 0);

  // Lookup non-existent command
  entry = cns_dispatch_lookup(&table, s7t_hash_string("invalid", 7));
  TEST_ASSERT(entry == NULL);

  TEST_ASSERT_CYCLES(300);
  TEST_END();
}

// Test dispatch execution
static void test_dispatch_execute()
{
  TEST_BEGIN("Dispatch Execute");

  // Create test dispatch entries
  static const cns_dispatch_entry_t test_entries[] = {
      {"test", s7t_hash_string("test", 4), mock_echo_handler, 0, 2, CNS_FLAG_NONE, "Test command", "test [arg1] [arg2]"},
      {"admin", s7t_hash_string("admin", 5), mock_admin_handler, 1, 3, CNS_FLAG_ADMIN, "Admin command", "admin arg1 [arg2] [arg3]"},
      {"args", s7t_hash_string("args", 4), mock_arg_handler, 0, 5, CNS_FLAG_NONE, "Args command", "args [arg1] [arg2] [arg3] [arg4] [arg5]"}};

  uint32_t hash_index[256];
  cns_dispatch_table_t table;
  cns_dispatch_init(&table, test_entries, 3, hash_index, 256);

  // Test successful execution
  cns_command_t cmd = {0};
  strcpy(cmd.cmd, "test");
  cmd.hash = s7t_hash_string("test", 4);
  cmd.argc = 1;
  strcpy(cmd.args[0], "arg1");
  cmd.flags = CNS_FLAG_NONE;

  cns_result_t result = cns_dispatch_execute(&table, &cmd, NULL);
  TEST_ASSERT(result == CNS_OK);

  // Test argument counting handler
  cmd.hash = s7t_hash_string("args", 4);
  cmd.argc = 3;
  strcpy(cmd.args[0], "arg1");
  strcpy(cmd.args[1], "arg2");
  strcpy(cmd.args[2], "arg3");

  result = cns_dispatch_execute(&table, &cmd, NULL);
  TEST_ASSERT(result == 3); // Should return argc

  // Test invalid command
  cmd.hash = s7t_hash_string("invalid", 7);
  result = cns_dispatch_execute(&table, &cmd, NULL);
  TEST_ASSERT(result == CNS_ERR_INVALID_CMD);

  TEST_ASSERT_CYCLES(400);
  TEST_END();
}

// Test argument validation in dispatch
static void test_dispatch_arg_validation()
{
  TEST_BEGIN("Dispatch Argument Validation");

  // Create test dispatch entries with specific arg requirements
  static const cns_dispatch_entry_t test_entries[] = {
      {"test", s7t_hash_string("test", 4), mock_echo_handler, 1, 3, CNS_FLAG_NONE, "Test command", "test arg1 [arg2] [arg3]"},
      {"admin", s7t_hash_string("admin", 5), mock_admin_handler, 2, 4, CNS_FLAG_ADMIN, "Admin command", "admin arg1 arg2 [arg3] [arg4]"}};

  uint32_t hash_index[256];
  cns_dispatch_table_t table;
  cns_dispatch_init(&table, test_entries, 2, hash_index, 256);

  cns_command_t cmd = {0};
  cns_result_t result;

  // Test too few arguments
  strcpy(cmd.cmd, "test");
  cmd.hash = s7t_hash_string("test", 4);
  cmd.argc = 0;
  cmd.flags = CNS_FLAG_NONE;

  result = cns_dispatch_execute(&table, &cmd, NULL);
  TEST_ASSERT(result == CNS_ERR_INVALID_ARG);

  // Test too many arguments
  cmd.argc = 4;
  strcpy(cmd.args[0], "arg1");
  strcpy(cmd.args[1], "arg2");
  strcpy(cmd.args[2], "arg3");
  strcpy(cmd.args[3], "arg4");

  result = cns_dispatch_execute(&table, &cmd, NULL);
  TEST_ASSERT(result == CNS_ERR_INVALID_ARG);

  // Test valid argument count
  cmd.argc = 2;
  result = cns_dispatch_execute(&table, &cmd, NULL);
  TEST_ASSERT(result == CNS_OK);

  // Test admin command with insufficient args
  strcpy(cmd.cmd, "admin");
  cmd.hash = s7t_hash_string("admin", 5);
  cmd.argc = 1;
  cmd.flags = CNS_FLAG_ADMIN;

  result = cns_dispatch_execute(&table, &cmd, NULL);
  TEST_ASSERT(result == CNS_ERR_INVALID_ARG);

  // Test admin command with valid args
  cmd.argc = 2;
  result = cns_dispatch_execute(&table, &cmd, NULL);
  TEST_ASSERT(result == CNS_OK);

  TEST_ASSERT_CYCLES(400);
  TEST_END();
}

// Test permission validation in dispatch
static void test_dispatch_permission_validation()
{
  TEST_BEGIN("Dispatch Permission Validation");

  // Create test dispatch entries
  static const cns_dispatch_entry_t test_entries[] = {
      {"normal", s7t_hash_string("normal", 6), mock_echo_handler, 0, 1, CNS_FLAG_NONE, "Normal command", "normal [arg1]"},
      {"admin", s7t_hash_string("admin", 5), mock_admin_handler, 0, 1, CNS_FLAG_ADMIN, "Admin command", "admin [arg1]"}};

  uint32_t hash_index[256];
  cns_dispatch_table_t table;
  cns_dispatch_init(&table, test_entries, 2, hash_index, 256);

  cns_command_t cmd = {0};
  cns_result_t result;

  // Test normal command without admin flag
  strcpy(cmd.cmd, "normal");
  cmd.hash = s7t_hash_string("normal", 6);
  cmd.argc = 0;
  cmd.flags = CNS_FLAG_NONE;

  result = cns_dispatch_execute(&table, &cmd, NULL);
  TEST_ASSERT(result == CNS_OK);

  // Test admin command without admin flag
  strcpy(cmd.cmd, "admin");
  cmd.hash = s7t_hash_string("admin", 5);
  cmd.flags = CNS_FLAG_NONE;

  result = cns_dispatch_execute(&table, &cmd, NULL);
  TEST_ASSERT(result == CNS_ERR_PERMISSION);

  // Test admin command with admin flag
  cmd.flags = CNS_FLAG_ADMIN;
  result = cns_dispatch_execute(&table, &cmd, NULL);
  TEST_ASSERT(result == CNS_OK);

  TEST_ASSERT_CYCLES(300);
  TEST_END();
}

// Test batch dispatch
static void test_dispatch_batch()
{
  TEST_BEGIN("Dispatch Batch Execution");

  // Create test dispatch entries
  static const cns_dispatch_entry_t test_entries[] = {
      {"test1", s7t_hash_string("test1", 5), mock_echo_handler, 0, 1, CNS_FLAG_NONE, "Test1 command", "test1 [arg1]"},
      {"test2", s7t_hash_string("test2", 5), mock_echo_handler, 0, 1, CNS_FLAG_NONE, "Test2 command", "test2 [arg1]"},
      {"error", s7t_hash_string("error", 5), mock_error_handler, 0, 1, CNS_FLAG_NONE, "Error command", "error [arg1]"}};

  uint32_t hash_index[256];
  cns_dispatch_table_t table;
  cns_dispatch_init(&table, test_entries, 3, hash_index, 256);

  // Create batch of commands
  cns_command_t commands[4];
  cns_result_t results[4];

  // Command 1: valid
  strcpy(commands[0].cmd, "test1");
  commands[0].hash = s7t_hash_string("test1", 5);
  commands[0].argc = 0;
  commands[0].flags = CNS_FLAG_NONE;

  // Command 2: valid
  strcpy(commands[1].cmd, "test2");
  commands[1].hash = s7t_hash_string("test2", 5);
  commands[1].argc = 0;
  commands[1].flags = CNS_FLAG_NONE;

  // Command 3: error handler
  strcpy(commands[2].cmd, "error");
  commands[2].hash = s7t_hash_string("error", 5);
  commands[2].argc = 0;
  commands[2].flags = CNS_FLAG_NONE;

  // Command 4: invalid
  strcpy(commands[3].cmd, "invalid");
  commands[3].hash = s7t_hash_string("invalid", 7);
  commands[3].argc = 0;
  commands[3].flags = CNS_FLAG_NONE;

  // Execute batch
  uint32_t success_count = cns_dispatch_batch(&table, commands, 4, NULL, results);

  // Verify results
  TEST_ASSERT(success_count == 2); // test1 and test2 should succeed
  TEST_ASSERT(results[0] == CNS_OK);
  TEST_ASSERT(results[1] == CNS_OK);
  TEST_ASSERT(results[2] == CNS_ERR_INTERNAL);    // error handler returns error
  TEST_ASSERT(results[3] == CNS_ERR_INVALID_CMD); // invalid command

  TEST_ASSERT_CYCLES(500);
  TEST_END();
}

// Test dispatch help functions
static void test_dispatch_help()
{
  TEST_BEGIN("Dispatch Help Functions");

  // Create test dispatch entries
  static const cns_dispatch_entry_t test_entries[] = {
      {"test", s7t_hash_string("test", 4), mock_echo_handler, 0, 2, CNS_FLAG_NONE, "Test command help", "test [arg1] [arg2]"},
      {"admin", s7t_hash_string("admin", 5), mock_admin_handler, 1, 3, CNS_FLAG_ADMIN, "Admin command help", "admin arg1 [arg2] [arg3]"}};

  uint32_t hash_index[256];
  cns_dispatch_table_t table;
  cns_dispatch_init(&table, test_entries, 2, hash_index, 256);

  // Test help lookup
  const char *help = cns_dispatch_help(&table, "test");
  TEST_ASSERT(help != NULL);
  TEST_ASSERT(strcmp(help, "Test command help") == 0);

  help = cns_dispatch_help(&table, "admin");
  TEST_ASSERT(help != NULL);
  TEST_ASSERT(strcmp(help, "Admin command help") == 0);

  // Test help for non-existent command
  help = cns_dispatch_help(&table, "invalid");
  TEST_ASSERT(help != NULL);
  TEST_ASSERT(strcmp(help, "Unknown command") == 0);

  // Test command existence
  TEST_ASSERT(cns_dispatch_exists(&table, "test") == true);
  TEST_ASSERT(cns_dispatch_exists(&table, "admin") == true);
  TEST_ASSERT(cns_dispatch_exists(&table, "invalid") == false);

  TEST_ASSERT_CYCLES(300);
  TEST_END();
}

// Test dispatch list callback
static void test_dispatch_list()
{
  TEST_BEGIN("Dispatch List Callback");

  // Create test dispatch entries
  static const cns_dispatch_entry_t test_entries[] = {
      {"test1", s7t_hash_string("test1", 5), mock_echo_handler, 0, 1, CNS_FLAG_NONE, "Test1 help", "test1 [arg1]"},
      {"test2", s7t_hash_string("test2", 5), mock_echo_handler, 0, 1, CNS_FLAG_NONE, "Test2 help", "test2 [arg1]"}};

  uint32_t hash_index[256];
  cns_dispatch_table_t table;
  cns_dispatch_init(&table, test_entries, 2, hash_index, 256);

  // Callback data structure
  typedef struct
  {
    const char *names[10];
    const char *helps[10];
    uint32_t count;
  } callback_data_t;

  callback_data_t data = {0};

  // Define callback function
  void test_callback(const char *name, const char *help, void *user_data)
  {
    callback_data_t *cb_data = (callback_data_t *)user_data;
    if (cb_data->count < 10)
    {
      cb_data->names[cb_data->count] = name;
      cb_data->helps[cb_data->count] = help;
      cb_data->count++;
    }
  }

  // Test list callback
  cns_dispatch_list(&table, test_callback, &data);

  // Verify callback was called for all commands
  TEST_ASSERT(data.count == 2);
  TEST_ASSERT(strcmp(data.names[0], "test1") == 0);
  TEST_ASSERT(strcmp(data.helps[0], "Test1 help") == 0);
  TEST_ASSERT(strcmp(data.names[1], "test2") == 0);
  TEST_ASSERT(strcmp(data.helps[1], "Test2 help") == 0);

  TEST_ASSERT_CYCLES(200);
  TEST_END();
}

// Test dispatch metrics
static void test_dispatch_metrics()
{
  TEST_BEGIN("Dispatch Metrics");

  // Create test dispatch entries
  static const cns_dispatch_entry_t test_entries[] = {
      {"test", s7t_hash_string("test", 4), mock_echo_handler, 0, 1, CNS_FLAG_NONE, "Test command", "test [arg1]"}};

  uint32_t hash_index[256];
  cns_dispatch_table_t table;
  cns_dispatch_init(&table, test_entries, 1, hash_index, 256);

  // Initialize metrics
  cns_dispatch_metrics_t metrics = {0};

  // Execute command and update metrics
  cns_command_t cmd = {0};
  strcpy(cmd.cmd, "test");
  cmd.hash = s7t_hash_string("test", 4);
  cmd.argc = 0;
  cmd.flags = CNS_FLAG_NONE;

  uint64_t start = s7t_cycles();
  cns_result_t result = cns_dispatch_execute(&table, &cmd, NULL);
  uint64_t cycles = s7t_cycles() - start;

  cns_dispatch_update_metrics(&metrics, result, cycles);

  // Verify metrics updated
  TEST_ASSERT(metrics.total_dispatches == 1);
  TEST_ASSERT(metrics.successful_dispatches == 1);
  TEST_ASSERT(metrics.failed_dispatches == 0);
  TEST_ASSERT(metrics.total_cycles == cycles);
  TEST_ASSERT(metrics.min_cycles == cycles);
  TEST_ASSERT(metrics.max_cycles == cycles);

  // Execute error command
  strcpy(cmd.cmd, "invalid");
  cmd.hash = s7t_hash_string("invalid", 7);

  start = s7t_cycles();
  result = cns_dispatch_execute(&table, &cmd, NULL);
  cycles = s7t_cycles() - start;

  cns_dispatch_update_metrics(&metrics, result, cycles);

  // Verify metrics updated for error
  TEST_ASSERT(metrics.total_dispatches == 2);
  TEST_ASSERT(metrics.successful_dispatches == 1);
  TEST_ASSERT(metrics.failed_dispatches == 1);
  TEST_ASSERT(metrics.total_cycles > 0);

  TEST_ASSERT_CYCLES(400);
  TEST_END();
}

/*═══════════════════════════════════════════════════════════════
  Test Runner
  ═══════════════════════════════════════════════════════════════*/

int main()
{
  printf("CNS Dispatch Unit Tests\n");
  printf("======================\n\n");

  // Initialize S7T
  s7t_init();

  // Run tests
  test_dispatch_init();
  test_dispatch_lookup();
  test_dispatch_execute();
  test_dispatch_arg_validation();
  test_dispatch_permission_validation();
  test_dispatch_batch();
  test_dispatch_help();
  test_dispatch_list();
  test_dispatch_metrics();

  // Print summary
  printf("\nTest Summary:\n");
  printf("  Total: %u\n", tests_run);
  printf("  Passed: %u\n", tests_passed);
  printf("  Failed: %u\n", tests_failed);
  printf("  Success Rate: %.1f%%\n",
         (float)tests_passed / tests_run * 100.0f);

  return (tests_failed == 0) ? 0 : 1;
}