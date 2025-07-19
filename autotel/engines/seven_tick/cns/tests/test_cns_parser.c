/*  ─────────────────────────────────────────────────────────────
    test_cns_parser.c  –  CNS Parser Unit Tests (v1.0)
    7-tick compliant test framework for CNS parser functionality
    ───────────────────────────────────────────────────────────── */

#include "../include/cns_parser.h"
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
  Parser Functionality Tests
  ═══════════════════════════════════════════════════════════════*/

// Test character classification
static void test_char_classification()
{
  TEST_BEGIN("Character Classification");

  // Test whitespace characters
  TEST_ASSERT(CNS_IS_SPACE(' ') == true);
  TEST_ASSERT(CNS_IS_SPACE('\t') == true);
  TEST_ASSERT(CNS_IS_SPACE('\n') == true);
  TEST_ASSERT(CNS_IS_SPACE('\r') == true);

  // Test non-whitespace characters
  TEST_ASSERT(CNS_IS_SPACE('a') == false);
  TEST_ASSERT(CNS_IS_SPACE('1') == false);
  TEST_ASSERT(CNS_IS_SPACE('_') == false);

  // Test quote characters
  TEST_ASSERT(CNS_IS_QUOTE('"') == true);
  TEST_ASSERT(CNS_IS_QUOTE('\'') == true);
  TEST_ASSERT(CNS_IS_QUOTE('a') == false);

  // Test escape character
  TEST_ASSERT(CNS_IS_ESCAPE('\\') == true);
  TEST_ASSERT(CNS_IS_ESCAPE('a') == false);

  // Test terminator
  TEST_ASSERT(CNS_IS_TERM('\0') == true);
  TEST_ASSERT(CNS_IS_TERM('a') == false);

  TEST_ASSERT_CYCLES(50);
  TEST_END();
}

// Test whitespace skipping
static void test_skip_whitespace()
{
  TEST_BEGIN("Whitespace Skipping");

  // Test no whitespace
  const char *input = "command";
  const char *result = cns_skip_whitespace(input);
  TEST_ASSERT(result == input);

  // Test leading whitespace
  input = "  \t\n\rcommand";
  result = cns_skip_whitespace(input);
  TEST_ASSERT(result == input + 6);
  TEST_ASSERT(*result == 'c');

  // Test mixed whitespace
  input = " \t \n \r command";
  result = cns_skip_whitespace(input);
  TEST_ASSERT(result == input + 8);
  TEST_ASSERT(*result == 'c');

  // Test only whitespace
  input = "   \t\n\r";
  result = cns_skip_whitespace(input);
  TEST_ASSERT(*result == '\0');

  TEST_ASSERT_CYCLES(100);
  TEST_END();
}

// Test token parsing
static void test_parse_token()
{
  TEST_BEGIN("Token Parsing");

  char buffer[64];
  size_t len;

  // Test simple token
  const char *input = "command";
  len = cns_parse_token(&input, buffer, sizeof(buffer));
  TEST_ASSERT(len == 7);
  TEST_ASSERT(strcmp(buffer, "command") == 0);
  TEST_ASSERT(*input == '\0');

  // Test token with trailing whitespace
  input = "command  arg1";
  len = cns_parse_token(&input, buffer, sizeof(buffer));
  TEST_ASSERT(len == 7);
  TEST_ASSERT(strcmp(buffer, "command") == 0);
  TEST_ASSERT(*input == ' ');

  // Test quoted token
  input = "\"quoted command\" arg1";
  len = cns_parse_token(&input, buffer, sizeof(buffer));
  TEST_ASSERT(len == 14);
  TEST_ASSERT(strcmp(buffer, "quoted command") == 0);
  TEST_ASSERT(*input == ' ');

  // Test escaped characters in quotes
  input = "\"escaped\\\"quote\" arg1";
  len = cns_parse_token(&input, buffer, sizeof(buffer));
  TEST_ASSERT(len == 13);
  TEST_ASSERT(strcmp(buffer, "escaped\"quote") == 0);

  // Test buffer overflow protection
  input = "very_long_command_name_that_exceeds_buffer";
  len = cns_parse_token(&input, buffer, 20);
  TEST_ASSERT(len == 19);
  TEST_ASSERT(buffer[19] == '\0');

  TEST_ASSERT_CYCLES(200);
  TEST_END();
}

// Test main parser function
static void test_cns_parse()
{
  TEST_BEGIN("CNS Parse Function");

  cns_command_t cmd;
  cns_result_t result;

  // Test simple command
  result = cns_parse("test", &cmd);
  TEST_ASSERT(result == CNS_OK);
  TEST_ASSERT(strcmp(cmd.cmd, "test") == 0);
  TEST_ASSERT(cmd.argc == 0);
  TEST_ASSERT(cmd.hash == s7t_hash_string("test", 4));
  TEST_ASSERT(cmd.timestamp > 0);

  // Test command with arguments
  result = cns_parse("test arg1 arg2", &cmd);
  TEST_ASSERT(result == CNS_OK);
  TEST_ASSERT(strcmp(cmd.cmd, "test") == 0);
  TEST_ASSERT(cmd.argc == 2);
  TEST_ASSERT(strcmp(cmd.args[0], "arg1") == 0);
  TEST_ASSERT(strcmp(cmd.args[1], "arg2") == 0);

  // Test command with quoted arguments
  result = cns_parse("test \"quoted arg\" normal", &cmd);
  TEST_ASSERT(result == CNS_OK);
  TEST_ASSERT(strcmp(cmd.cmd, "test") == 0);
  TEST_ASSERT(cmd.argc == 2);
  TEST_ASSERT(strcmp(cmd.args[0], "quoted arg") == 0);
  TEST_ASSERT(strcmp(cmd.args[1], "normal") == 0);

  // Test command with extra whitespace
  result = cns_parse("  test  arg1  arg2  ", &cmd);
  TEST_ASSERT(result == CNS_OK);
  TEST_ASSERT(strcmp(cmd.cmd, "test") == 0);
  TEST_ASSERT(cmd.argc == 2);
  TEST_ASSERT(strcmp(cmd.args[0], "arg1") == 0);
  TEST_ASSERT(strcmp(cmd.args[1], "arg2") == 0);

  // Test empty command
  result = cns_parse("", &cmd);
  TEST_ASSERT(result == CNS_ERR_INVALID_CMD);

  result = cns_parse("   ", &cmd);
  TEST_ASSERT(result == CNS_ERR_INVALID_CMD);

  // Test command with maximum arguments
  char max_args_cmd[256] = "test";
  for (int i = 0; i < CNS_MAX_ARGS; i++)
  {
    char arg[32];
    snprintf(arg, sizeof(arg), "arg%d", i);
    strcat(max_args_cmd, " ");
    strcat(max_args_cmd, arg);
  }

  result = cns_parse(max_args_cmd, &cmd);
  TEST_ASSERT(result == CNS_OK);
  TEST_ASSERT(cmd.argc == CNS_MAX_ARGS);

  TEST_ASSERT_CYCLES(500);
  TEST_END();
}

// Test simple parser
static void test_cns_parse_simple()
{
  TEST_BEGIN("CNS Simple Parser");

  cns_command_t cmd;
  cns_result_t result;

  // Test simple command without args
  result = cns_parse_simple("test", &cmd);
  TEST_ASSERT(result == CNS_OK);
  TEST_ASSERT(strcmp(cmd.cmd, "test") == 0);
  TEST_ASSERT(cmd.argc == 0);
  TEST_ASSERT(cmd.hash == s7t_hash_string("test", 4));
  TEST_ASSERT(cmd.timestamp > 0);

  // Test command with single argument
  result = cns_parse_simple("test arg1", &cmd);
  TEST_ASSERT(result == CNS_OK);
  TEST_ASSERT(strcmp(cmd.cmd, "test") == 0);
  TEST_ASSERT(cmd.argc == 1);
  TEST_ASSERT(strcmp(cmd.args[0], "arg1") == 0);

  // Test command with extra whitespace
  result = cns_parse_simple("  test  arg1  ", &cmd);
  TEST_ASSERT(result == CNS_OK);
  TEST_ASSERT(strcmp(cmd.cmd, "test") == 0);
  TEST_ASSERT(cmd.argc == 1);
  TEST_ASSERT(strcmp(cmd.args[0], "arg1") == 0);

  // Test empty command
  result = cns_parse_simple("", &cmd);
  TEST_ASSERT(result == CNS_ERR_INVALID_CMD);

  result = cns_parse_simple("   ", &cmd);
  TEST_ASSERT(result == CNS_ERR_INVALID_CMD);

  // Test long command name
  char long_cmd[64];
  memset(long_cmd, 'a', 63);
  long_cmd[63] = '\0';
  strcpy(long_cmd, "very_long_command_name");

  result = cns_parse_simple(long_cmd, &cmd);
  TEST_ASSERT(result == CNS_OK);
  TEST_ASSERT(strcmp(cmd.cmd, "very_long_command_name") == 0);

  TEST_ASSERT_CYCLES(300);
  TEST_END();
}

// Test command normalization
static void test_command_normalization()
{
  TEST_BEGIN("Command Normalization");

  char cmd[64];

  // Test lowercase conversion
  strcpy(cmd, "TEST");
  cns_normalize_command(cmd);
  TEST_ASSERT(strcmp(cmd, "test") == 0);

  // Test mixed case
  strcpy(cmd, "TestCommand");
  cns_normalize_command(cmd);
  TEST_ASSERT(strcmp(cmd, "testcommand") == 0);

  // Test with underscores
  strcpy(cmd, "TEST_COMMAND");
  cns_normalize_command(cmd);
  TEST_ASSERT(strcmp(cmd, "test_command") == 0);

  // Test already normalized
  strcpy(cmd, "test");
  cns_normalize_command(cmd);
  TEST_ASSERT(strcmp(cmd, "test") == 0);

  // Test empty string
  strcpy(cmd, "");
  cns_normalize_command(cmd);
  TEST_ASSERT(strcmp(cmd, "") == 0);

  TEST_ASSERT_CYCLES(200);
  TEST_END();
}

// Test argument validation
static void test_argument_validation()
{
  TEST_BEGIN("Argument Validation");

  // Test valid arguments
  TEST_ASSERT(cns_validate_arg("normal_arg", 0) == true);
  TEST_ASSERT(cns_validate_arg("arg_with_underscores", 0) == true);
  TEST_ASSERT(cns_validate_arg("arg123", 0) == true);
  TEST_ASSERT(cns_validate_arg("a", 0) == true);

  // Test empty argument
  TEST_ASSERT(cns_validate_arg("", 0) == false);

  // Test argument with special characters (if validation is strict)
  // This depends on the specific validation rules implemented

  // Test very long argument
  char long_arg[CNS_MAX_ARG_LEN + 1];
  memset(long_arg, 'a', CNS_MAX_ARG_LEN);
  long_arg[CNS_MAX_ARG_LEN] = '\0';
  TEST_ASSERT(cns_validate_arg(long_arg, 0) == false);

  TEST_ASSERT_CYCLES(100);
  TEST_END();
}

// Test command type detection
static void test_command_type_detection()
{
  TEST_BEGIN("Command Type Detection");

  // Test different command types
  uint32_t exec_hash = s7t_hash_string("execute", 7);
  uint32_t query_hash = s7t_hash_string("query", 5);
  uint32_t config_hash = s7t_hash_string("config", 6);
  uint32_t help_hash = s7t_hash_string("help", 4);
  uint32_t exit_hash = s7t_hash_string("exit", 4);
  uint32_t unknown_hash = s7t_hash_string("unknown", 7);

  // Note: The actual type detection depends on the implementation
  // of cns_detect_type. These tests verify the function works
  // without crashing and returns valid enum values.

  cns_cmd_type_t type;

  type = cns_detect_type(exec_hash);
  TEST_ASSERT(type >= CNS_CMD_EXEC && type <= CNS_CMD_INVALID);

  type = cns_detect_type(query_hash);
  TEST_ASSERT(type >= CNS_CMD_EXEC && type <= CNS_CMD_INVALID);

  type = cns_detect_type(config_hash);
  TEST_ASSERT(type >= CNS_CMD_EXEC && type <= CNS_CMD_INVALID);

  type = cns_detect_type(help_hash);
  TEST_ASSERT(type >= CNS_CMD_EXEC && type <= CNS_CMD_INVALID);

  type = cns_detect_type(exit_hash);
  TEST_ASSERT(type >= CNS_CMD_EXEC && type <= CNS_CMD_INVALID);

  type = cns_detect_type(unknown_hash);
  TEST_ASSERT(type >= CNS_CMD_EXEC && type <= CNS_CMD_INVALID);

  TEST_ASSERT_CYCLES(200);
  TEST_END();
}

// Test batch parsing
static void test_batch_parsing()
{
  TEST_BEGIN("Batch Parsing");

  cns_command_t commands[10];
  uint32_t count;

  // Test single command
  count = cns_parse_batch("test", commands, 10);
  TEST_ASSERT(count == 1);
  TEST_ASSERT(strcmp(commands[0].cmd, "test") == 0);

  // Test multiple commands
  count = cns_parse_batch("test1;test2;test3", commands, 10);
  TEST_ASSERT(count == 3);
  TEST_ASSERT(strcmp(commands[0].cmd, "test1") == 0);
  TEST_ASSERT(strcmp(commands[1].cmd, "test2") == 0);
  TEST_ASSERT(strcmp(commands[2].cmd, "test3") == 0);

  // Test commands with arguments
  count = cns_parse_batch("test1 arg1;test2 arg2 arg3", commands, 10);
  TEST_ASSERT(count == 2);
  TEST_ASSERT(strcmp(commands[0].cmd, "test1") == 0);
  TEST_ASSERT(commands[0].argc == 1);
  TEST_ASSERT(strcmp(commands[0].args[0], "arg1") == 0);
  TEST_ASSERT(strcmp(commands[1].cmd, "test2") == 0);
  TEST_ASSERT(commands[1].argc == 2);
  TEST_ASSERT(strcmp(commands[1].args[0], "arg2") == 0);
  TEST_ASSERT(strcmp(commands[1].args[1], "arg3") == 0);

  // Test with extra semicolons
  count = cns_parse_batch("test1;;test2", commands, 10);
  TEST_ASSERT(count == 2);
  TEST_ASSERT(strcmp(commands[0].cmd, "test1") == 0);
  TEST_ASSERT(strcmp(commands[1].cmd, "test2") == 0);

  // Test limit exceeded
  count = cns_parse_batch("test1;test2;test3;test4", commands, 3);
  TEST_ASSERT(count == 3);

  TEST_ASSERT_CYCLES(400);
  TEST_END();
}

// Test error string conversion
static void test_error_strings()
{
  TEST_BEGIN("Error String Conversion");

  // Test all error codes
  const char *str;

  str = cns_parse_error_str(CNS_OK);
  TEST_ASSERT(str != NULL);
  TEST_ASSERT(strlen(str) > 0);

  str = cns_parse_error_str(CNS_ERR_INVALID_CMD);
  TEST_ASSERT(str != NULL);
  TEST_ASSERT(strlen(str) > 0);

  str = cns_parse_error_str(CNS_ERR_INVALID_ARG);
  TEST_ASSERT(str != NULL);
  TEST_ASSERT(strlen(str) > 0);

  str = cns_parse_error_str(CNS_ERR_PERMISSION);
  TEST_ASSERT(str != NULL);
  TEST_ASSERT(strlen(str) > 0);

  str = cns_parse_error_str(CNS_ERR_TIMEOUT);
  TEST_ASSERT(str != NULL);
  TEST_ASSERT(strlen(str) > 0);

  str = cns_parse_error_str(CNS_ERR_RESOURCE);
  TEST_ASSERT(str != NULL);
  TEST_ASSERT(strlen(str) > 0);

  str = cns_parse_error_str(CNS_ERR_INTERNAL);
  TEST_ASSERT(str != NULL);
  TEST_ASSERT(strlen(str) > 0);

  // Test invalid error code
  str = cns_parse_error_str(999);
  TEST_ASSERT(str != NULL);
  TEST_ASSERT(strlen(str) > 0);

  TEST_ASSERT_CYCLES(100);
  TEST_END();
}

/*═══════════════════════════════════════════════════════════════
  Test Runner
  ═══════════════════════════════════════════════════════════════*/

int main()
{
  printf("CNS Parser Unit Tests\n");
  printf("====================\n\n");

  // Initialize S7T
  s7t_init();

  // Run tests
  test_char_classification();
  test_skip_whitespace();
  test_parse_token();
  test_cns_parse();
  test_cns_parse_simple();
  test_command_normalization();
  test_argument_validation();
  test_command_type_detection();
  test_batch_parsing();
  test_error_strings();

  // Print summary
  printf("\nTest Summary:\n");
  printf("  Total: %u\n", tests_run);
  printf("  Passed: %u\n", tests_passed);
  printf("  Failed: %u\n", tests_failed);
  printf("  Success Rate: %.1f%%\n",
         (float)tests_passed / tests_run * 100.0f);

  return (tests_failed == 0) ? 0 : 1;
}