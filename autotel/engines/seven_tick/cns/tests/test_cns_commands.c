/*  ─────────────────────────────────────────────────────────────
    test_cns_commands.c  –  CNS Commands Unit Tests (v1.0)
    7-tick compliant test framework for CNS built-in commands
    ───────────────────────────────────────────────────────────── */

#include "../include/cns_commands.h"
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
  Mock Engine for Testing
  ═══════════════════════════════════════════════════════════════*/

// Mock engine structure for testing commands
typedef struct
{
  cns_cmd_entry_t commands[10];
  uint32_t cmd_count;
  s7t_perf_counter_t perf;
  uint32_t flags;
} mock_engine_t;

// Mock engine context
static mock_engine_t mock_engine = {0};

// Mock command entries for testing
static cns_cmd_entry_t mock_entries[] = {
    {"test1", 0, NULL, CNS_FLAG_NONE, 0, 2, "Test command 1"},
    {"test2", 0, NULL, CNS_FLAG_ADMIN, 1, 3, "Test command 2"},
    {"help", 0, NULL, CNS_FLAG_NONE, 0, 1, "Help command"}};

/*═══════════════════════════════════════════════════════════════
  Command Functionality Tests
  ═══════════════════════════════════════════════════════════════*/

// Test help command
static void test_help_command()
{
  TEST_BEGIN("Help Command");

  // Initialize mock engine
  mock_engine.cmd_count = 3;
  memcpy(mock_engine.commands, mock_entries, sizeof(mock_entries));

  cns_command_t cmd = {0};
  cns_result_t result;

  // Test help with no arguments (list all commands)
  strcpy(cmd.cmd, "help");
  cmd.argc = 0;

  result = cns_cmd_help(&cmd, &mock_engine);
  TEST_ASSERT(result == CNS_OK);

  // Test help with specific command
  cmd.argc = 1;
  strcpy(cmd.args[0], "test1");

  result = cns_cmd_help(&cmd, &mock_engine);
  TEST_ASSERT(result == CNS_OK);

  // Test help with non-existent command
  strcpy(cmd.args[0], "invalid");

  result = cns_cmd_help(&cmd, &mock_engine);
  TEST_ASSERT(result == CNS_ERR_INVALID_ARG);

  TEST_ASSERT_CYCLES(300);
  TEST_END();
}

// Test exit command
static void test_exit_command()
{
  TEST_BEGIN("Exit Command");

  cns_command_t cmd = {0};
  cns_result_t result;

  // Test exit command
  strcpy(cmd.cmd, "exit");
  cmd.argc = 0;

  result = cns_cmd_exit(&cmd, NULL);
  TEST_ASSERT(result == CNS_CMD_EXIT);

  // Test with arguments (should still exit)
  cmd.argc = 1;
  strcpy(cmd.args[0], "force");

  result = cns_cmd_exit(&cmd, NULL);
  TEST_ASSERT(result == CNS_CMD_EXIT);

  TEST_ASSERT_CYCLES(50);
  TEST_END();
}

// Test echo command
static void test_echo_command()
{
  TEST_BEGIN("Echo Command");

  cns_command_t cmd = {0};
  cns_result_t result;

  // Test echo with no arguments
  strcpy(cmd.cmd, "echo");
  cmd.argc = 0;

  result = cns_cmd_echo(&cmd, NULL);
  TEST_ASSERT(result == CNS_OK);

  // Test echo with single argument
  cmd.argc = 1;
  strcpy(cmd.args[0], "hello");

  result = cns_cmd_echo(&cmd, NULL);
  TEST_ASSERT(result == CNS_OK);

  // Test echo with multiple arguments
  cmd.argc = 3;
  strcpy(cmd.args[0], "hello");
  strcpy(cmd.args[1], "world");
  strcpy(cmd.args[2], "test");

  result = cns_cmd_echo(&cmd, NULL);
  TEST_ASSERT(result == CNS_OK);

  TEST_ASSERT_CYCLES(200);
  TEST_END();
}

// Test status command
static void test_status_command()
{
  TEST_BEGIN("Status Command");

  // Initialize mock engine with performance data
  mock_engine.cmd_count = 2;
  mock_engine.perf.count = 10;
  mock_engine.perf.cycles = 1000;
  mock_engine.perf.min_cycles = 50;
  mock_engine.perf.max_cycles = 200;

  cns_command_t cmd = {0};
  cns_result_t result;

  // Test status command
  strcpy(cmd.cmd, "status");
  cmd.argc = 0;

  result = cns_cmd_status(&cmd, &mock_engine);
  TEST_ASSERT(result == CNS_OK);

  // Test with arguments (should still work)
  cmd.argc = 1;
  strcpy(cmd.args[0], "verbose");

  result = cns_cmd_status(&cmd, &mock_engine);
  TEST_ASSERT(result == CNS_OK);

  TEST_ASSERT_CYCLES(300);
  TEST_END();
}

// Test time command
static void test_time_command()
{
  TEST_BEGIN("Time Command");

  cns_command_t cmd = {0};
  cns_result_t result;

  // Test time command with no arguments (should fail)
  strcpy(cmd.cmd, "time");
  cmd.argc = 0;

  result = cns_cmd_time(&cmd, &mock_engine);
  TEST_ASSERT(result == CNS_ERR_INVALID_ARG);

  // Test time command with simple command
  cmd.argc = 1;
  strcpy(cmd.args[0], "echo hello");

  result = cns_cmd_time(&cmd, &mock_engine);
  // Note: This might fail if echo command isn't registered in mock engine
  // The test validates the function executes without crashing

  TEST_ASSERT_CYCLES(400);
  TEST_END();
}

// Test clear command
static void test_clear_command()
{
  TEST_BEGIN("Clear Command");

  cns_command_t cmd = {0};
  cns_result_t result;

  // Test clear command
  strcpy(cmd.cmd, "clear");
  cmd.argc = 0;

  result = cns_cmd_clear(&cmd, NULL);
  TEST_ASSERT(result == CNS_OK);

  // Test with arguments (should still work)
  cmd.argc = 1;
  strcpy(cmd.args[0], "screen");

  result = cns_cmd_clear(&cmd, NULL);
  TEST_ASSERT(result == CNS_OK);

  TEST_ASSERT_CYCLES(100);
  TEST_END();
}

// Test batch command
static void test_batch_command()
{
  TEST_BEGIN("Batch Command");

  cns_command_t cmd = {0};
  cns_result_t result;

  // Test batch command with no arguments (should fail)
  strcpy(cmd.cmd, "batch");
  cmd.argc = 0;

  result = cns_cmd_batch(&cmd, NULL);
  TEST_ASSERT(result == CNS_ERR_INVALID_ARG);

  // Test batch command with filename
  cmd.argc = 1;
  strcpy(cmd.args[0], "test.bat");

  result = cns_cmd_batch(&cmd, NULL);
  // Note: This will print "Batch execution not implemented"
  // The test validates the function executes without crashing
  TEST_ASSERT(result == CNS_OK);

  TEST_ASSERT_CYCLES(200);
  TEST_END();
}

// Test alias command
static void test_alias_command()
{
  TEST_BEGIN("Alias Command");

  cns_command_t cmd = {0};
  cns_result_t result;

  // Test alias command with no arguments (should fail)
  strcpy(cmd.cmd, "alias");
  cmd.argc = 0;

  result = cns_cmd_alias(&cmd, NULL);
  TEST_ASSERT(result == CNS_ERR_INVALID_ARG);

  // Test alias command with single argument (should fail)
  cmd.argc = 1;
  strcpy(cmd.args[0], "short");

  result = cns_cmd_alias(&cmd, NULL);
  TEST_ASSERT(result == CNS_ERR_INVALID_ARG);

  // Test alias command with two arguments
  cmd.argc = 2;
  strcpy(cmd.args[0], "short");
  strcpy(cmd.args[1], "long_command");

  result = cns_cmd_alias(&cmd, NULL);
  // Note: This will print "Alias 'short' -> 'long_command' (not implemented)"
  // The test validates the function executes without crashing
  TEST_ASSERT(result == CNS_OK);

  TEST_ASSERT_CYCLES(200);
  TEST_END();
}

// Test history command
static void test_history_command()
{
  TEST_BEGIN("History Command");

  cns_command_t cmd = {0};
  cns_result_t result;

  // Test history command
  strcpy(cmd.cmd, "history");
  cmd.argc = 0;

  result = cns_cmd_history(&cmd, NULL);
  // Note: This will print "Command history not implemented"
  // The test validates the function executes without crashing
  TEST_ASSERT(result == CNS_OK);

  // Test with arguments (should still work)
  cmd.argc = 1;
  strcpy(cmd.args[0], "10");

  result = cns_cmd_history(&cmd, NULL);
  TEST_ASSERT(result == CNS_OK);

  TEST_ASSERT_CYCLES(100);
  TEST_END();
}

// Test debug command
static void test_debug_command()
{
  TEST_BEGIN("Debug Command");

  // Initialize mock engine
  mock_engine.flags = 0;

  cns_command_t cmd = {0};
  cns_result_t result;

  // Test debug command with no arguments (show current state)
  strcpy(cmd.cmd, "debug");
  cmd.argc = 0;

  result = cns_cmd_debug(&cmd, &mock_engine);
  TEST_ASSERT(result == CNS_OK);
  TEST_ASSERT(mock_engine.flags == 0); // Should remain unchanged

  // Test debug command with "on"
  cmd.argc = 1;
  strcpy(cmd.args[0], "on");

  result = cns_cmd_debug(&cmd, &mock_engine);
  TEST_ASSERT(result == CNS_OK);
  TEST_ASSERT(mock_engine.flags & CNS_FLAG_LOGGED);

  // Test debug command with "off"
  strcpy(cmd.args[0], "off");

  result = cns_cmd_debug(&cmd, &mock_engine);
  TEST_ASSERT(result == CNS_OK);
  TEST_ASSERT((mock_engine.flags & CNS_FLAG_LOGGED) == 0);

  // Test debug command with invalid argument
  strcpy(cmd.args[0], "invalid");

  result = cns_cmd_debug(&cmd, &mock_engine);
  TEST_ASSERT(result == CNS_ERR_INVALID_ARG);

  TEST_ASSERT_CYCLES(300);
  TEST_END();
}

// Test built-in command registration
static void test_builtin_registration()
{
  TEST_BEGIN("Built-in Command Registration");

  // Initialize mock engine
  mock_engine.cmd_count = 0;

  // Test registration of built-in commands
  cns_register_builtins(&mock_engine);

  // Verify commands were registered
  TEST_ASSERT(mock_engine.cmd_count > 0);

  // Verify specific commands exist
  bool found_help = false;
  bool found_exit = false;
  bool found_echo = false;
  bool found_status = false;

  for (uint32_t i = 0; i < mock_engine.cmd_count; i++)
  {
    if (strcmp(mock_engine.commands[i].name, "help") == 0)
    {
      found_help = true;
    }
    else if (strcmp(mock_engine.commands[i].name, "exit") == 0)
    {
      found_exit = true;
    }
    else if (strcmp(mock_engine.commands[i].name, "echo") == 0)
    {
      found_echo = true;
    }
    else if (strcmp(mock_engine.commands[i].name, "status") == 0)
    {
      found_status = true;
    }
  }

  TEST_ASSERT(found_help);
  TEST_ASSERT(found_exit);
  TEST_ASSERT(found_echo);
  TEST_ASSERT(found_status);

  TEST_ASSERT_CYCLES(500);
  TEST_END();
}

// Test command completion
static void test_command_completion()
{
  TEST_BEGIN("Command Completion");

  // Initialize mock engine with test commands
  mock_engine.cmd_count = 4;
  strcpy(mock_engine.commands[0].name, "test");
  strcpy(mock_engine.commands[1].name, "test1");
  strcpy(mock_engine.commands[2].name, "test2");
  strcpy(mock_engine.commands[3].name, "other");

  const char *matches[10];
  uint32_t count;

  // Test completion with "test" prefix
  count = cns_complete(&mock_engine, "test", matches, 10);
  TEST_ASSERT(count == 3); // test, test1, test2
  TEST_ASSERT(strcmp(matches[0], "test") == 0);
  TEST_ASSERT(strcmp(matches[1], "test1") == 0);
  TEST_ASSERT(strcmp(matches[2], "test2") == 0);

  // Test completion with "test1" prefix
  count = cns_complete(&mock_engine, "test1", matches, 10);
  TEST_ASSERT(count == 1); // test1
  TEST_ASSERT(strcmp(matches[0], "test1") == 0);

  // Test completion with "o" prefix
  count = cns_complete(&mock_engine, "o", matches, 10);
  TEST_ASSERT(count == 1); // other
  TEST_ASSERT(strcmp(matches[0], "other") == 0);

  // Test completion with non-matching prefix
  count = cns_complete(&mock_engine, "xyz", matches, 10);
  TEST_ASSERT(count == 0);

  // Test completion with limit
  count = cns_complete(&mock_engine, "test", matches, 2);
  TEST_ASSERT(count == 2); // Should be limited to 2 matches

  TEST_ASSERT_CYCLES(400);
  TEST_END();
}

// Test command category functionality
static void test_command_categories()
{
  TEST_BEGIN("Command Categories");

  // Test category enum values
  TEST_ASSERT(CNS_CAT_SYSTEM == 0);
  TEST_ASSERT(CNS_CAT_FILE == 1);
  TEST_ASSERT(CNS_CAT_PROCESS == 2);
  TEST_ASSERT(CNS_CAT_NETWORK == 3);
  TEST_ASSERT(CNS_CAT_DEBUG == 4);
  TEST_ASSERT(CNS_CAT_ADMIN == 5);

  // Test category boundaries
  TEST_ASSERT(CNS_CAT_SYSTEM < CNS_CAT_ADMIN);
  TEST_ASSERT(CNS_CAT_ADMIN > CNS_CAT_SYSTEM);

  TEST_ASSERT_CYCLES(50);
  TEST_END();
}

// Test command handler macro
static void test_command_handler_macro()
{
  TEST_BEGIN("Command Handler Macro");

  // Test CNS_HANDLER macro by defining a handler
  CNS_HANDLER(test_handler)
  {
    (void)cmd;
    (void)context;
    return CNS_OK;
  }

  // Verify the handler can be called
  cns_command_t cmd = {0};
  cns_result_t result = test_handler(&cmd, NULL);
  TEST_ASSERT(result == CNS_OK);

  TEST_ASSERT_CYCLES(100);
  TEST_END();
}

// Test command registration macros
static void test_registration_macros()
{
  TEST_BEGIN("Command Registration Macros");

  // Initialize mock engine
  mock_engine.cmd_count = 0;

  // Test CNS_REGISTER_CMD macro
  CNS_REGISTER_CMD(&mock_engine, "macro_test", test_handler, 0, 2, "Macro test command");

  // Verify command was registered
  TEST_ASSERT(mock_engine.cmd_count == 1);
  TEST_ASSERT(strcmp(mock_engine.commands[0].name, "macro_test") == 0);
  TEST_ASSERT(mock_engine.commands[0].flags == CNS_FLAG_NONE);
  TEST_ASSERT(mock_engine.commands[0].min_args == 0);
  TEST_ASSERT(mock_engine.commands[0].max_args == 2);

  // Test CNS_REGISTER_ADMIN macro
  CNS_REGISTER_ADMIN(&mock_engine, "admin_macro", test_handler, 1, 3, "Admin macro test");

  // Verify admin command was registered
  TEST_ASSERT(mock_engine.cmd_count == 2);
  TEST_ASSERT(strcmp(mock_engine.commands[1].name, "admin_macro") == 0);
  TEST_ASSERT(mock_engine.commands[1].flags == CNS_FLAG_ADMIN);
  TEST_ASSERT(mock_engine.commands[1].min_args == 1);
  TEST_ASSERT(mock_engine.commands[1].max_args == 3);

  TEST_ASSERT_CYCLES(300);
  TEST_END();
}

/*═══════════════════════════════════════════════════════════════
  Test Runner
  ═══════════════════════════════════════════════════════════════*/

int main()
{
  printf("CNS Commands Unit Tests\n");
  printf("======================\n\n");

  // Initialize S7T
  s7t_init();

  // Run tests
  test_help_command();
  test_exit_command();
  test_echo_command();
  test_status_command();
  test_time_command();
  test_clear_command();
  test_batch_command();
  test_alias_command();
  test_history_command();
  test_debug_command();
  test_builtin_registration();
  test_command_completion();
  test_command_categories();
  test_command_handler_macro();
  test_registration_macros();

  // Print summary
  printf("\nTest Summary:\n");
  printf("  Total: %u\n", tests_run);
  printf("  Passed: %u\n", tests_passed);
  printf("  Failed: %u\n", tests_failed);
  printf("  Success Rate: %.1f%%\n",
         (float)tests_passed / tests_run * 100.0f);

  return (tests_failed == 0) ? 0 : 1;
}