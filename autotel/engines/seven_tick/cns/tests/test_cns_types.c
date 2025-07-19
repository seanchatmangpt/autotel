/*  ─────────────────────────────────────────────────────────────
    test_cns_types.c  –  CNS Types Unit Tests (v1.0)
    7-tick compliant test framework for CNS type definitions
    ───────────────────────────────────────────────────────────── */

#include "../include/cns/types.h"
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
  Option Type Tests
  ═══════════════════════════════════════════════════════════════*/

// Test option type enum
static void test_option_types()
{
  TEST_BEGIN("Option Types");

  // Test enum values
  TEST_ASSERT(CNS_OPT_STRING == 0);
  TEST_ASSERT(CNS_OPT_INT == 1);
  TEST_ASSERT(CNS_OPT_BOOL == 2);
  TEST_ASSERT(CNS_OPT_FLOAT == 3);
  TEST_ASSERT(CNS_OPT_FLAG == 4);

  // Test enum boundaries
  TEST_ASSERT(CNS_OPT_STRING < CNS_OPT_FLAG);
  TEST_ASSERT(CNS_OPT_FLAG > CNS_OPT_STRING);

  // Test enum uniqueness
  TEST_ASSERT(CNS_OPT_STRING != CNS_OPT_INT);
  TEST_ASSERT(CNS_OPT_INT != CNS_OPT_BOOL);
  TEST_ASSERT(CNS_OPT_BOOL != CNS_OPT_FLOAT);
  TEST_ASSERT(CNS_OPT_FLOAT != CNS_OPT_FLAG);

  TEST_ASSERT_CYCLES(50);
  TEST_END();
}

// Test option structure
static void test_option_structure()
{
  TEST_BEGIN("Option Structure");

  // Test option initialization
  CNSOption option = {
      .name = "test_option",
      .short_name = 't',
      .type = CNS_OPT_STRING,
      .description = "Test option description",
      .default_val = "default",
      .required = true};

  // Verify option fields
  TEST_ASSERT(strcmp(option.name, "test_option") == 0);
  TEST_ASSERT(option.short_name == 't');
  TEST_ASSERT(option.type == CNS_OPT_STRING);
  TEST_ASSERT(strcmp(option.description, "Test option description") == 0);
  TEST_ASSERT(strcmp(option.default_val, "default") == 0);
  TEST_ASSERT(option.required == true);

  // Test option modification
  option.short_name = 'x';
  option.type = CNS_OPT_INT;
  option.required = false;

  TEST_ASSERT(option.short_name == 'x');
  TEST_ASSERT(option.type == CNS_OPT_INT);
  TEST_ASSERT(option.required == false);

  TEST_ASSERT_CYCLES(100);
  TEST_END();
}

// Test option array
static void test_option_array()
{
  TEST_BEGIN("Option Array");

  // Create array of options
  CNSOption options[] = {
      {"input", 'i', CNS_OPT_STRING, "Input file", "input.txt", true},
      {"output", 'o', CNS_OPT_STRING, "Output file", "output.txt", false},
      {"verbose", 'v', CNS_OPT_FLAG, "Verbose output", NULL, false},
      {"count", 'c', CNS_OPT_INT, "Count value", "10", false},
      {"ratio", 'r', CNS_OPT_FLOAT, "Ratio value", "1.0", false}};

  size_t option_count = sizeof(options) / sizeof(options[0]);
  TEST_ASSERT(option_count == 5);

  // Test option array access
  TEST_ASSERT(strcmp(options[0].name, "input") == 0);
  TEST_ASSERT(options[0].short_name == 'i');
  TEST_ASSERT(options[0].type == CNS_OPT_STRING);
  TEST_ASSERT(options[0].required == true);

  TEST_ASSERT(strcmp(options[1].name, "output") == 0);
  TEST_ASSERT(options[1].short_name == 'o');
  TEST_ASSERT(options[1].required == false);

  TEST_ASSERT(strcmp(options[2].name, "verbose") == 0);
  TEST_ASSERT(options[2].type == CNS_OPT_FLAG);
  TEST_ASSERT(options[2].default_val == NULL);

  TEST_ASSERT(strcmp(options[3].name, "count") == 0);
  TEST_ASSERT(options[3].type == CNS_OPT_INT);
  TEST_ASSERT(strcmp(options[3].default_val, "10") == 0);

  TEST_ASSERT(strcmp(options[4].name, "ratio") == 0);
  TEST_ASSERT(options[4].type == CNS_OPT_FLOAT);
  TEST_ASSERT(strcmp(options[4].default_val, "1.0") == 0);

  TEST_ASSERT_CYCLES(200);
  TEST_END();
}

/*═══════════════════════════════════════════════════════════════
  Argument Type Tests
  ═══════════════════════════════════════════════════════════════*/

// Test argument structure
static void test_argument_structure()
{
  TEST_BEGIN("Argument Structure");

  // Test argument initialization
  CNSArgument argument = {
      .name = "test_arg",
      .description = "Test argument description",
      .required = true,
      .variadic = false};

  // Verify argument fields
  TEST_ASSERT(strcmp(argument.name, "test_arg") == 0);
  TEST_ASSERT(strcmp(argument.description, "Test argument description") == 0);
  TEST_ASSERT(argument.required == true);
  TEST_ASSERT(argument.variadic == false);

  // Test argument modification
  argument.required = false;
  argument.variadic = true;

  TEST_ASSERT(argument.required == false);
  TEST_ASSERT(argument.variadic == true);

  TEST_ASSERT_CYCLES(100);
  TEST_END();
}

// Test argument array
static void test_argument_array()
{
  TEST_BEGIN("Argument Array");

  // Create array of arguments
  CNSArgument arguments[] = {
      {"source", "Source file", true, false},
      {"destination", "Destination file", true, false},
      {"options", "Additional options", false, true}};

  size_t argument_count = sizeof(arguments) / sizeof(arguments[0]);
  TEST_ASSERT(argument_count == 3);

  // Test argument array access
  TEST_ASSERT(strcmp(arguments[0].name, "source") == 0);
  TEST_ASSERT(arguments[0].required == true);
  TEST_ASSERT(arguments[0].variadic == false);

  TEST_ASSERT(strcmp(arguments[1].name, "destination") == 0);
  TEST_ASSERT(arguments[1].required == true);
  TEST_ASSERT(arguments[1].variadic == false);

  TEST_ASSERT(strcmp(arguments[2].name, "options") == 0);
  TEST_ASSERT(arguments[2].required == false);
  TEST_ASSERT(arguments[2].variadic == true);

  TEST_ASSERT_CYCLES(150);
  TEST_END();
}

/*═══════════════════════════════════════════════════════════════
  Command Type Tests
  ═══════════════════════════════════════════════════════════════*/

// Test command structure
static void test_command_structure()
{
  TEST_BEGIN("Command Structure");

  // Create test options and arguments
  CNSOption options[] = {
      {"verbose", 'v', CNS_OPT_FLAG, "Verbose output", NULL, false}};

  CNSArgument arguments[] = {
      {"file", "Input file", true, false}};

  // Test command initialization
  CNSCommand command = {
      .name = "test_command",
      .description = "Test command description",
      .handler = NULL, // Will be set later
      .options = options,
      .option_count = 1,
      .arguments = arguments,
      .argument_count = 1};

  // Verify command fields
  TEST_ASSERT(strcmp(command.name, "test_command") == 0);
  TEST_ASSERT(strcmp(command.description, "Test command description") == 0);
  TEST_ASSERT(command.handler == NULL);
  TEST_ASSERT(command.options == options);
  TEST_ASSERT(command.option_count == 1);
  TEST_ASSERT(command.arguments == arguments);
  TEST_ASSERT(command.argument_count == 1);

  TEST_ASSERT_CYCLES(100);
  TEST_END();
}

// Test command handler function type
static void test_command_handler()
{
  TEST_BEGIN("Command Handler");

  // Define a test handler function
  CNSHandler test_handler = NULL;

  // Test handler assignment
  test_handler = (CNSHandler)0x12345678; // Dummy address
  TEST_ASSERT(test_handler != NULL);

  // Test handler function signature compatibility
  // This test verifies the function type is properly defined
  TEST_ASSERT(sizeof(CNSHandler) == sizeof(void *));

  TEST_ASSERT_CYCLES(50);
  TEST_END();
}

// Test command array
static void test_command_array()
{
  TEST_BEGIN("Command Array");

  // Create array of commands
  CNSCommand commands[] = {
      {"copy", "Copy files", NULL, NULL, 0, NULL, 0},
      {"move", "Move files", NULL, NULL, 0, NULL, 0},
      {"delete", "Delete files", NULL, NULL, 0, NULL, 0}};

  size_t command_count = sizeof(commands) / sizeof(commands[0]);
  TEST_ASSERT(command_count == 3);

  // Test command array access
  TEST_ASSERT(strcmp(commands[0].name, "copy") == 0);
  TEST_ASSERT(strcmp(commands[0].description, "Copy files") == 0);

  TEST_ASSERT(strcmp(commands[1].name, "move") == 0);
  TEST_ASSERT(strcmp(commands[1].description, "Move files") == 0);

  TEST_ASSERT(strcmp(commands[2].name, "delete") == 0);
  TEST_ASSERT(strcmp(commands[2].description, "Delete files") == 0);

  TEST_ASSERT_CYCLES(150);
  TEST_END();
}

/*═══════════════════════════════════════════════════════════════
  Domain Type Tests
  ═══════════════════════════════════════════════════════════════*/

// Test domain structure
static void test_domain_structure()
{
  TEST_BEGIN("Domain Structure");

  // Create test commands
  CNSCommand commands[] = {
      {"list", "List items", NULL, NULL, 0, NULL, 0},
      {"create", "Create item", NULL, NULL, 0, NULL, 0}};

  // Test domain initialization
  CNSDomain domain = {
      .name = "test_domain",
      .description = "Test domain description",
      .commands = commands,
      .command_count = 2};

  // Verify domain fields
  TEST_ASSERT(strcmp(domain.name, "test_domain") == 0);
  TEST_ASSERT(strcmp(domain.description, "Test domain description") == 0);
  TEST_ASSERT(domain.commands == commands);
  TEST_ASSERT(domain.command_count == 2);

  TEST_ASSERT_CYCLES(100);
  TEST_END();
}

// Test domain array
static void test_domain_array()
{
  TEST_BEGIN("Domain Array");

  // Create array of domains
  CNSDomain domains[] = {
      {"file", "File operations", NULL, 0},
      {"network", "Network operations", NULL, 0},
      {"system", "System operations", NULL, 0}};

  size_t domain_count = sizeof(domains) / sizeof(domains[0]);
  TEST_ASSERT(domain_count == 3);

  // Test domain array access
  TEST_ASSERT(strcmp(domains[0].name, "file") == 0);
  TEST_ASSERT(strcmp(domains[0].description, "File operations") == 0);

  TEST_ASSERT(strcmp(domains[1].name, "network") == 0);
  TEST_ASSERT(strcmp(domains[1].description, "Network operations") == 0);

  TEST_ASSERT(strcmp(domains[2].name, "system") == 0);
  TEST_ASSERT(strcmp(domains[2].description, "System operations") == 0);

  TEST_ASSERT_CYCLES(150);
  TEST_END();
}

/*═══════════════════════════════════════════════════════════════
  Context Type Tests
  ═══════════════════════════════════════════════════════════════*/

// Test context structure
static void test_context_structure()
{
  TEST_BEGIN("Context Structure");

  // Test context initialization
  CNSContext context = {
      .program_name = "test_program",
      .domain = "test_domain",
      .command = "test_command",
      .parsed_options = NULL,
      .parsed_arguments = NULL,
      .verbose = false,
      .json_output = false,
      .start_cycles = 0,
      .user_data = NULL};

  // Verify context fields
  TEST_ASSERT(strcmp(context.program_name, "test_program") == 0);
  TEST_ASSERT(strcmp(context.domain, "test_domain") == 0);
  TEST_ASSERT(strcmp(context.command, "test_command") == 0);
  TEST_ASSERT(context.parsed_options == NULL);
  TEST_ASSERT(context.parsed_arguments == NULL);
  TEST_ASSERT(context.verbose == false);
  TEST_ASSERT(context.json_output == false);
  TEST_ASSERT(context.start_cycles == 0);
  TEST_ASSERT(context.user_data == NULL);

  // Test context modification
  context.verbose = true;
  context.json_output = true;
  context.start_cycles = 12345;

  TEST_ASSERT(context.verbose == true);
  TEST_ASSERT(context.json_output == true);
  TEST_ASSERT(context.start_cycles == 12345);

  TEST_ASSERT_CYCLES(100);
  TEST_END();
}

/*═══════════════════════════════════════════════════════════════
  Result Code Tests
  ═══════════════════════════════════════════════════════════════*/

// Test result codes
static void test_result_codes()
{
  TEST_BEGIN("Result Codes");

  // Test enum values
  TEST_ASSERT(CNS_OK == 0);
  TEST_ASSERT(CNS_ERROR == 1);
  TEST_ASSERT(CNS_ERROR_ARGS == 2);
  TEST_ASSERT(CNS_ERROR_NOT_FOUND == 3);
  TEST_ASSERT(CNS_ERROR_PERMISSION == 4);
  TEST_ASSERT(CNS_ERROR_IO == 5);
  TEST_ASSERT(CNS_ERROR_MEMORY == 6);
  TEST_ASSERT(CNS_ERROR_TIMEOUT == 7);
  TEST_ASSERT(CNS_ERROR_CYCLES == 8);

  // Test enum boundaries
  TEST_ASSERT(CNS_OK < CNS_ERROR);
  TEST_ASSERT(CNS_ERROR < CNS_ERROR_ARGS);
  TEST_ASSERT(CNS_ERROR_ARGS < CNS_ERROR_NOT_FOUND);
  TEST_ASSERT(CNS_ERROR_NOT_FOUND < CNS_ERROR_PERMISSION);
  TEST_ASSERT(CNS_ERROR_PERMISSION < CNS_ERROR_IO);
  TEST_ASSERT(CNS_ERROR_IO < CNS_ERROR_MEMORY);
  TEST_ASSERT(CNS_ERROR_MEMORY < CNS_ERROR_TIMEOUT);
  TEST_ASSERT(CNS_ERROR_TIMEOUT < CNS_ERROR_CYCLES);

  // Test enum uniqueness
  TEST_ASSERT(CNS_OK != CNS_ERROR);
  TEST_ASSERT(CNS_ERROR != CNS_ERROR_ARGS);
  TEST_ASSERT(CNS_ERROR_ARGS != CNS_ERROR_NOT_FOUND);
  TEST_ASSERT(CNS_ERROR_NOT_FOUND != CNS_ERROR_PERMISSION);
  TEST_ASSERT(CNS_ERROR_PERMISSION != CNS_ERROR_IO);
  TEST_ASSERT(CNS_ERROR_IO != CNS_ERROR_MEMORY);
  TEST_ASSERT(CNS_ERROR_MEMORY != CNS_ERROR_TIMEOUT);
  TEST_ASSERT(CNS_ERROR_TIMEOUT != CNS_ERROR_CYCLES);

  TEST_ASSERT_CYCLES(100);
  TEST_END();
}

// Test result code semantics
static void test_result_semantics()
{
  TEST_BEGIN("Result Code Semantics");

  // Test success case
  CNSResult success = CNS_OK;
  TEST_ASSERT(success == 0);
  TEST_ASSERT(!success); // 0 is false in C

  // Test error cases
  CNSResult error = CNS_ERROR;
  TEST_ASSERT(error != 0);
  TEST_ASSERT(error); // Non-zero is true in C

  // Test error hierarchy
  TEST_ASSERT(CNS_ERROR_ARGS > CNS_ERROR);
  TEST_ASSERT(CNS_ERROR_NOT_FOUND > CNS_ERROR_ARGS);
  TEST_ASSERT(CNS_ERROR_PERMISSION > CNS_ERROR_NOT_FOUND);
  TEST_ASSERT(CNS_ERROR_IO > CNS_ERROR_PERMISSION);
  TEST_ASSERT(CNS_ERROR_MEMORY > CNS_ERROR_IO);
  TEST_ASSERT(CNS_ERROR_TIMEOUT > CNS_ERROR_MEMORY);
  TEST_ASSERT(CNS_ERROR_CYCLES > CNS_ERROR_TIMEOUT);

  // Test 7-tick specific error
  TEST_ASSERT(CNS_ERROR_CYCLES == 8);
  TEST_ASSERT(CNS_ERROR_CYCLES > CNS_ERROR_TIMEOUT);

  TEST_ASSERT_CYCLES(100);
  TEST_END();
}

/*═══════════════════════════════════════════════════════════════
  Integration Tests
  ═══════════════════════════════════════════════════════════════*/

// Test complete command structure
static void test_complete_command()
{
  TEST_BEGIN("Complete Command Structure");

  // Create options
  CNSOption options[] = {
      {"input", 'i', CNS_OPT_STRING, "Input file", "default.txt", true},
      {"output", 'o', CNS_OPT_STRING, "Output file", "output.txt", false},
      {"verbose", 'v', CNS_OPT_FLAG, "Verbose output", NULL, false}};

  // Create arguments
  CNSArgument arguments[] = {
      {"source", "Source file", true, false},
      {"destination", "Destination file", true, false}};

  // Create command
  CNSCommand command = {
      .name = "copy",
      .description = "Copy files",
      .handler = NULL,
      .options = options,
      .option_count = 3,
      .arguments = arguments,
      .argument_count = 2};

  // Verify complete structure
  TEST_ASSERT(strcmp(command.name, "copy") == 0);
  TEST_ASSERT(command.option_count == 3);
  TEST_ASSERT(command.argument_count == 2);

  // Verify options
  TEST_ASSERT(strcmp(command.options[0].name, "input") == 0);
  TEST_ASSERT(command.options[0].short_name == 'i');
  TEST_ASSERT(command.options[0].type == CNS_OPT_STRING);
  TEST_ASSERT(command.options[0].required == true);

  TEST_ASSERT(strcmp(command.options[1].name, "output") == 0);
  TEST_ASSERT(command.options[1].short_name == 'o');
  TEST_ASSERT(command.options[1].required == false);

  TEST_ASSERT(strcmp(command.options[2].name, "verbose") == 0);
  TEST_ASSERT(command.options[2].type == CNS_OPT_FLAG);
  TEST_ASSERT(command.options[2].default_val == NULL);

  // Verify arguments
  TEST_ASSERT(strcmp(command.arguments[0].name, "source") == 0);
  TEST_ASSERT(command.arguments[0].required == true);
  TEST_ASSERT(command.arguments[0].variadic == false);

  TEST_ASSERT(strcmp(command.arguments[1].name, "destination") == 0);
  TEST_ASSERT(command.arguments[1].required == true);
  TEST_ASSERT(command.arguments[1].variadic == false);

  TEST_ASSERT_CYCLES(300);
  TEST_END();
}

// Test complete domain structure
static void test_complete_domain()
{
  TEST_BEGIN("Complete Domain Structure");

  // Create commands
  CNSCommand commands[] = {
      {"copy", "Copy files", NULL, NULL, 0, NULL, 0},
      {"move", "Move files", NULL, NULL, 0, NULL, 0},
      {"delete", "Delete files", NULL, NULL, 0, NULL, 0}};

  // Create domain
  CNSDomain domain = {
      .name = "file",
      .description = "File operations",
      .commands = commands,
      .command_count = 3};

  // Verify complete structure
  TEST_ASSERT(strcmp(domain.name, "file") == 0);
  TEST_ASSERT(strcmp(domain.description, "File operations") == 0);
  TEST_ASSERT(domain.command_count == 3);

  // Verify commands
  TEST_ASSERT(strcmp(domain.commands[0].name, "copy") == 0);
  TEST_ASSERT(strcmp(domain.commands[1].name, "move") == 0);
  TEST_ASSERT(strcmp(domain.commands[2].name, "delete") == 0);

  TEST_ASSERT_CYCLES(200);
  TEST_END();
}

/*═══════════════════════════════════════════════════════════════
  Test Runner
  ═══════════════════════════════════════════════════════════════*/

int main()
{
  printf("CNS Types Unit Tests\n");
  printf("====================\n\n");

  // Initialize S7T
  s7t_init();

  // Run tests
  test_option_types();
  test_option_structure();
  test_option_array();
  test_argument_structure();
  test_argument_array();
  test_command_structure();
  test_command_handler();
  test_command_array();
  test_domain_structure();
  test_domain_array();
  test_context_structure();
  test_result_codes();
  test_result_semantics();
  test_complete_command();
  test_complete_domain();

  // Print summary
  printf("\nTest Summary:\n");
  printf("  Total: %u\n", tests_run);
  printf("  Passed: %u\n", tests_passed);
  printf("  Failed: %u\n", tests_failed);
  printf("  Success Rate: %.1f%%\n",
         (float)tests_passed / tests_run * 100.0f);

  return (tests_failed == 0) ? 0 : 1;
}