# CNS API Reference

## Overview

This document provides complete API documentation for all CNS (Command Nano Stack) components, including function signatures, parameters, return values, and usage examples.

## Core API (cns.h)

### Engine Management

#### `cns_init`
```c
void cns_init(cns_engine_t* engine, cns_cmd_entry_t* commands, 
              uint32_t* hash_table, uint32_t max_commands);
```
**Description**: Initialize a CNS engine with command storage and hash table.

**Parameters**:
- `engine`: Pointer to engine structure to initialize
- `commands`: Array of command entries
- `hash_table`: Hash table for command lookup
- `max_commands`: Maximum number of commands supported

**Example**:
```c
cns_cmd_entry_t commands[10];
uint32_t hash_table[256];
cns_engine_t engine;
cns_init(&engine, commands, hash_table, 10);
```

#### `cns_register`
```c
cns_result_t cns_register(cns_engine_t* engine, const char* name, 
                         cns_handler_t handler, cns_flags_t flags,
                         uint32_t min_args, uint32_t max_args, 
                         const char* help);
```
**Description**: Register a command handler with the engine.

**Parameters**:
- `engine`: Engine to register command with
- `name`: Command name (must be unique)
- `handler`: Function pointer to command handler
- `flags`: Command flags (CNS_FLAG_NONE, CNS_FLAG_ADMIN, etc.)
- `min_args`: Minimum number of arguments required
- `max_args`: Maximum number of arguments allowed
- `help`: Help text for the command

**Returns**: `CNS_OK` on success, error code on failure

**Example**:
```c
cns_result_t result = cns_register(&engine, "echo", echo_handler, 
                                  CNS_FLAG_NONE, 0, 2, "Echo arguments");
```

#### `cns_lookup`
```c
cns_cmd_entry_t* cns_lookup(cns_engine_t* engine, uint32_t hash);
```
**Description**: Look up a command by its hash value.

**Parameters**:
- `engine`: Engine to search in
- `hash`: Hash value of command name

**Returns**: Pointer to command entry if found, NULL otherwise

**Example**:
```c
uint32_t hash = s7t_hash_string("echo", 4);
cns_cmd_entry_t* entry = cns_lookup(&engine, hash);
```

#### `cns_execute`
```c
cns_result_t cns_execute(cns_engine_t* engine, cns_command_t* cmd);
```
**Description**: Execute a parsed command.

**Parameters**:
- `engine`: Engine to execute command in
- `cmd`: Parsed command structure

**Returns**: Result from command handler or error code

**Example**:
```c
cns_command_t cmd;
cns_parse("echo hello", &cmd);
cns_result_t result = cns_execute(&engine, &cmd);
```

### Command Validation

#### `cns_validate_quick`
```c
bool cns_validate_quick(const char* cmd, size_t len);
```
**Description**: Quick validation of command name.

**Parameters**:
- `cmd`: Command name to validate
- `len`: Length of command name

**Returns**: `true` if valid, `false` otherwise

**Example**:
```c
bool valid = cns_validate_quick("echo", 4);
```

## Parser API (cns_parser.h)

### Command Parsing

#### `cns_parse`
```c
cns_result_t cns_parse(const char* input, cns_command_t* cmd);
```
**Description**: Parse a command string into a command structure.

**Parameters**:
- `input`: Command string to parse
- `cmd`: Output command structure

**Returns**: `CNS_OK` on success, error code on failure

**Example**:
```c
cns_command_t cmd;
cns_result_t result = cns_parse("echo \"hello world\"", &cmd);
```

#### `cns_parse_simple`
```c
cns_result_t cns_parse_simple(const char* input, cns_command_t* cmd);
```
**Description**: Parse a simple command (no quotes or escapes).

**Parameters**:
- `input`: Simple command string
- `cmd`: Output command structure

**Returns**: `CNS_OK` on success, error code on failure

**Example**:
```c
cns_command_t cmd;
cns_result_t result = cns_parse_simple("echo hello", &cmd);
```

#### `cns_parse_batch`
```c
uint32_t cns_parse_batch(const char* input, cns_command_t* commands, 
                        uint32_t max_commands);
```
**Description**: Parse multiple commands separated by semicolons.

**Parameters**:
- `input`: Batch command string
- `commands`: Array to store parsed commands
- `max_commands`: Maximum number of commands to parse

**Returns**: Number of commands parsed

**Example**:
```c
cns_command_t commands[10];
uint32_t count = cns_parse_batch("echo hello; echo world", commands, 10);
```

### Utility Functions

#### `cns_skip_whitespace`
```c
const char* cns_skip_whitespace(const char* input);
```
**Description**: Skip leading whitespace in a string.

**Parameters**:
- `input`: Input string

**Returns**: Pointer to first non-whitespace character

**Example**:
```c
const char* result = cns_skip_whitespace("  hello");
// result points to "hello"
```

#### `cns_parse_token`
```c
size_t cns_parse_token(const char** input, char* buffer, size_t buffer_size);
```
**Description**: Parse a single token from input.

**Parameters**:
- `input`: Pointer to input string (updated to point after token)
- `buffer`: Buffer to store token
- `buffer_size`: Size of buffer

**Returns**: Length of parsed token

**Example**:
```c
const char* input = "echo hello";
char buffer[64];
size_t len = cns_parse_token(&input, buffer, sizeof(buffer));
// buffer contains "echo", input points to " hello"
```

#### `cns_normalize_command`
```c
void cns_normalize_command(char* cmd);
```
**Description**: Normalize command name (convert to lowercase).

**Parameters**:
- `cmd`: Command name to normalize

**Example**:
```c
char cmd[] = "ECHO";
cns_normalize_command(cmd);
// cmd now contains "echo"
```

#### `cns_validate_arg`
```c
bool cns_validate_arg(const char* arg, uint32_t index);
```
**Description**: Validate a command argument.

**Parameters**:
- `arg`: Argument to validate
- `index`: Argument index

**Returns**: `true` if valid, `false` otherwise

**Example**:
```c
bool valid = cns_validate_arg("hello", 0);
```

#### `cns_detect_type`
```c
cns_cmd_type_t cns_detect_type(uint32_t hash);
```
**Description**: Detect command type from hash.

**Parameters**:
- `hash`: Command hash value

**Returns**: Command type enum value

**Example**:
```c
uint32_t hash = s7t_hash_string("execute", 7);
cns_cmd_type_t type = cns_detect_type(hash);
```

#### `cns_parse_error_str`
```c
const char* cns_parse_error_str(cns_result_t error);
```
**Description**: Get error string for parse error code.

**Parameters**:
- `error`: Error code

**Returns**: Error message string

**Example**:
```c
const char* msg = cns_parse_error_str(CNS_ERR_INVALID_CMD);
```

## Dispatch API (cns_dispatch.h)

### Dispatch Table Management

#### `cns_dispatch_init`
```c
void cns_dispatch_init(cns_dispatch_table_t* table, 
                      const cns_dispatch_entry_t* entries,
                      uint32_t count, uint32_t* hash_index, 
                      uint32_t hash_size);
```
**Description**: Initialize a dispatch table.

**Parameters**:
- `table`: Dispatch table to initialize
- `entries`: Array of dispatch entries
- `count`: Number of entries
- `hash_index`: Hash index table
- `hash_size`: Size of hash index table

**Example**:
```c
cns_dispatch_entry_t entries[] = {
    {"echo", 0, echo_handler, 0, 2, CNS_FLAG_NONE, "Echo command", "echo [args]"}
};
uint32_t hash_index[256];
cns_dispatch_table_t table;
cns_dispatch_init(&table, entries, 1, hash_index, 256);
```

#### `cns_dispatch_lookup`
```c
const cns_dispatch_entry_t* cns_dispatch_lookup(cns_dispatch_table_t* table, 
                                               uint32_t hash);
```
**Description**: Look up a dispatch entry by hash.

**Parameters**:
- `table`: Dispatch table to search
- `hash`: Command hash value

**Returns**: Pointer to dispatch entry if found, NULL otherwise

**Example**:
```c
uint32_t hash = s7t_hash_string("echo", 4);
const cns_dispatch_entry_t* entry = cns_dispatch_lookup(&table, hash);
```

#### `cns_dispatch_execute`
```c
cns_result_t cns_dispatch_execute(cns_dispatch_table_t* table, 
                                 cns_command_t* cmd, void* context);
```
**Description**: Execute a command through the dispatch table.

**Parameters**:
- `table`: Dispatch table
- `cmd`: Command to execute
- `context`: User context data

**Returns**: Result from command handler

**Example**:
```c
cns_result_t result = cns_dispatch_execute(&table, &cmd, NULL);
```

#### `cns_dispatch_batch`
```c
uint32_t cns_dispatch_batch(cns_dispatch_table_t* table, 
                           cns_command_t* commands, uint32_t count,
                           void* context, cns_result_t* results);
```
**Description**: Execute multiple commands in batch.

**Parameters**:
- `table`: Dispatch table
- `commands`: Array of commands to execute
- `count`: Number of commands
- `context`: User context data
- `results`: Array to store results

**Returns**: Number of successfully executed commands

**Example**:
```c
cns_result_t results[10];
uint32_t success_count = cns_dispatch_batch(&table, commands, 5, NULL, results);
```

### Help and Information

#### `cns_dispatch_help`
```c
const char* cns_dispatch_help(cns_dispatch_table_t* table, const char* name);
```
**Description**: Get help text for a command.

**Parameters**:
- `table`: Dispatch table
- `name`: Command name

**Returns**: Help text string

**Example**:
```c
const char* help = cns_dispatch_help(&table, "echo");
```

#### `cns_dispatch_exists`
```c
bool cns_dispatch_exists(cns_dispatch_table_t* table, const char* name);
```
**Description**: Check if a command exists.

**Parameters**:
- `table`: Dispatch table
- `name`: Command name

**Returns**: `true` if command exists, `false` otherwise

**Example**:
```c
bool exists = cns_dispatch_exists(&table, "echo");
```

#### `cns_dispatch_list`
```c
void cns_dispatch_list(cns_dispatch_table_t* table, 
                      cns_list_callback_t callback, void* user_data);
```
**Description**: List all commands in dispatch table.

**Parameters**:
- `table`: Dispatch table
- `callback`: Function called for each command
- `user_data`: User data passed to callback

**Example**:
```c
void list_callback(const char* name, const char* help, void* user_data) {
    printf("  %s: %s\n", name, help);
}
cns_dispatch_list(&table, list_callback, NULL);
```

### Metrics

#### `cns_dispatch_update_metrics`
```c
void cns_dispatch_update_metrics(cns_dispatch_metrics_t* metrics, 
                                cns_result_t result, uint64_t cycles);
```
**Description**: Update dispatch metrics.

**Parameters**:
- `metrics`: Metrics structure to update
- `result`: Execution result
- `cycles`: Execution cycles

**Example**:
```c
cns_dispatch_metrics_t metrics = {0};
cns_dispatch_update_metrics(&metrics, CNS_OK, 100);
```

## Commands API (cns_commands.h)

### Built-in Commands

#### `cns_cmd_help`
```c
cns_result_t cns_cmd_help(const cns_command_t* cmd, void* context);
```
**Description**: Built-in help command handler.

**Parameters**:
- `cmd`: Command structure
- `context`: Engine context

**Returns**: `CNS_OK` on success

**Example**:
```c
cns_result_t result = cns_cmd_help(&cmd, &engine);
```

#### `cns_cmd_exit`
```c
cns_result_t cns_cmd_exit(const cns_command_t* cmd, void* context);
```
**Description**: Built-in exit command handler.

**Parameters**:
- `cmd`: Command structure
- `context`: Engine context

**Returns**: `CNS_CMD_EXIT` to indicate exit

**Example**:
```c
cns_result_t result = cns_cmd_exit(&cmd, NULL);
```

#### `cns_cmd_echo`
```c
cns_result_t cns_cmd_echo(const cns_command_t* cmd, void* context);
```
**Description**: Built-in echo command handler.

**Parameters**:
- `cmd`: Command structure
- `context`: Engine context

**Returns**: `CNS_OK` on success

**Example**:
```c
cns_result_t result = cns_cmd_echo(&cmd, NULL);
```

#### `cns_cmd_status`
```c
cns_result_t cns_cmd_status(const cns_command_t* cmd, void* context);
```
**Description**: Built-in status command handler.

**Parameters**:
- `cmd`: Command structure
- `context`: Engine context

**Returns**: `CNS_OK` on success

**Example**:
```c
cns_result_t result = cns_cmd_status(&cmd, &engine);
```

### Registration Macros

#### `CNS_HANDLER`
```c
CNS_HANDLER(function_name) {
    // Handler implementation
    return CNS_OK;
}
```
**Description**: Define a command handler function.

**Example**:
```c
CNS_HANDLER(my_handler) {
    printf("Executing my command\n");
    return CNS_OK;
}
```

#### `CNS_REGISTER_CMD`
```c
CNS_REGISTER_CMD(engine, name, handler, min_args, max_args, help)
```
**Description**: Register a command with default flags.

**Example**:
```c
CNS_REGISTER_CMD(&engine, "mycmd", my_handler, 0, 2, "My command");
```

#### `CNS_REGISTER_ADMIN`
```c
CNS_REGISTER_ADMIN(engine, name, handler, min_args, max_args, help)
```
**Description**: Register an admin command.

**Example**:
```c
CNS_REGISTER_ADMIN(&engine, "admin", admin_handler, 1, 3, "Admin command");
```

### Utility Functions

#### `cns_register_builtins`
```c
void cns_register_builtins(cns_engine_t* engine);
```
**Description**: Register all built-in commands.

**Parameters**:
- `engine`: Engine to register commands with

**Example**:
```c
cns_register_builtins(&engine);
```

#### `cns_complete`
```c
uint32_t cns_complete(cns_engine_t* engine, const char* prefix, 
                     const char** matches, uint32_t max_matches);
```
**Description**: Get command completions for a prefix.

**Parameters**:
- `engine`: Engine to search
- `prefix`: Command prefix
- `matches`: Array to store matches
- `max_matches`: Maximum number of matches

**Returns**: Number of matches found

**Example**:
```c
const char* matches[10];
uint32_t count = cns_complete(&engine, "ec", matches, 10);
```

## Benchmark API (cns_benchmark.h)

### Configuration

#### `cns_benchmark_config_t`
```c
typedef struct {
    const char* benchmark_name;
    uint32_t iterations;
    uint32_t warmup_iterations;
    uint32_t batch_size;
    bool verbose;
    bool validate_correctness;
} cns_benchmark_config_t;
```
**Description**: Benchmark configuration structure.

**Example**:
```c
cns_benchmark_config_t config = {
    .benchmark_name = "my_benchmark",
    .iterations = 1000,
    .warmup_iterations = 100,
    .batch_size = 10,
    .verbose = false,
    .validate_correctness = true
};
```

### Results

#### `cns_benchmark_result_t`
```c
typedef struct {
    const char* operation_name;
    const char* component_name;
    uint64_t total_operations;
    uint64_t total_time_ns;
    uint64_t min_time_ns;
    uint64_t max_time_ns;
    double avg_time_ns;
    double throughput_ops_per_sec;
    double p50_time_ns;
    double p95_time_ns;
    double p99_time_ns;
    bool target_achieved;
    const char* performance_tier;
} cns_benchmark_result_t;
```
**Description**: Benchmark result structure.

### Command Interface

#### `cmd_benchmark`
```c
int cmd_benchmark(int argc, char** argv);
```
**Description**: Benchmark command entry point.

**Parameters**:
- `argc`: Argument count
- `argv`: Argument array

**Returns**: Exit code

**Example**:
```c
int result = cmd_benchmark(argc, argv);
```

## Types API (cns/types.h)

### Option Types

#### `CNSOption`
```c
typedef struct {
    const char* name;
    char short_name;
    CNSOptionType type;
    const char* description;
    const char* default_val;
    bool required;
} CNSOption;
```
**Description**: Command option structure.

**Example**:
```c
CNSOption option = {
    .name = "verbose",
    .short_name = 'v',
    .type = CNS_OPT_FLAG,
    .description = "Verbose output",
    .default_val = NULL,
    .required = false
};
```

### Argument Types

#### `CNSArgument`
```c
typedef struct {
    const char* name;
    const char* description;
    bool required;
    bool variadic;
} CNSArgument;
```
**Description**: Command argument structure.

**Example**:
```c
CNSArgument argument = {
    .name = "file",
    .description = "Input file",
    .required = true,
    .variadic = false
};
```

### Command Types

#### `CNSCommand`
```c
typedef struct {
    const char* name;
    const char* description;
    CNSHandler handler;
    CNSOption* options;
    uint32_t option_count;
    CNSArgument* arguments;
    uint32_t argument_count;
} CNSCommand;
```
**Description**: Command structure.

**Example**:
```c
CNSCommand command = {
    .name = "copy",
    .description = "Copy files",
    .handler = copy_handler,
    .options = copy_options,
    .option_count = 2,
    .arguments = copy_arguments,
    .argument_count = 2
};
```

### Domain Types

#### `CNSDomain`
```c
typedef struct {
    const char* name;
    const char* description;
    CNSCommand* commands;
    uint32_t command_count;
} CNSDomain;
```
**Description**: Command domain structure.

**Example**:
```c
CNSDomain domain = {
    .name = "file",
    .description = "File operations",
    .commands = file_commands,
    .command_count = 3
};
```

### Context Types

#### `CNSContext`
```c
typedef struct {
    const char* program_name;
    const char* domain;
    const char* command;
    void* parsed_options;
    void* parsed_arguments;
    bool verbose;
    bool json_output;
    uint64_t start_cycles;
    void* user_data;
} CNSContext;
```
**Description**: Command execution context.

**Example**:
```c
CNSContext context = {
    .program_name = "my_program",
    .domain = "file",
    .command = "copy",
    .verbose = false,
    .json_output = false,
    .start_cycles = 0,
    .user_data = NULL
};
```

### Result Codes

#### `CNSResult`
```c
typedef enum {
    CNS_OK = 0,
    CNS_ERROR = 1,
    CNS_ERROR_ARGS = 2,
    CNS_ERROR_NOT_FOUND = 3,
    CNS_ERROR_PERMISSION = 4,
    CNS_ERROR_IO = 5,
    CNS_ERROR_MEMORY = 6,
    CNS_ERROR_TIMEOUT = 7,
    CNS_ERROR_CYCLES = 8
} CNSResult;
```
**Description**: Result code enumeration.

## CLI API (cns/cli.h)

### Initialization

#### `cns_cli_init`
```c
int cns_cli_init(const char* program_name);
```
**Description**: Initialize the CLI system.

**Parameters**:
- `program_name`: Name of the program

**Returns**: `CNS_OK` on success, error code on failure

**Example**:
```c
int result = cns_cli_init("my_program");
```

#### `cns_cli_cleanup`
```c
void cns_cli_cleanup(void);
```
**Description**: Clean up CLI resources.

**Example**:
```c
cns_cli_cleanup();
```

### Domain Management

#### `cns_cli_register_domain`
```c
int cns_cli_register_domain(const CNSDomain* domain);
```
**Description**: Register a command domain.

**Parameters**:
- `domain`: Domain to register

**Returns**: `CNS_OK` on success, error code on failure

**Example**:
```c
int result = cns_cli_register_domain(&file_domain);
```

### Execution

#### `cns_cli_run`
```c
int cns_cli_run(int argc, char** argv);
```
**Description**: Run the CLI with command line arguments.

**Parameters**:
- `argc`: Argument count
- `argv`: Argument array

**Returns**: Exit code

**Example**:
```c
int exit_code = cns_cli_run(argc, argv);
```

### Help System

#### `cns_cli_print_help`
```c
void cns_cli_print_help(void);
```
**Description**: Print general help information.

**Example**:
```c
cns_cli_print_help();
```

#### `cns_cli_print_domain_help`
```c
void cns_cli_print_domain_help(const char* domain_name);
```
**Description**: Print help for a specific domain.

**Parameters**:
- `domain_name`: Name of domain

**Example**:
```c
cns_cli_print_domain_help("file");
```

#### `cns_cli_print_command_help`
```c
void cns_cli_print_command_help(const char* domain_name, const char* command_name);
```
**Description**: Print help for a specific command.

**Parameters**:
- `domain_name`: Name of domain
- `command_name`: Name of command

**Example**:
```c
cns_cli_print_command_help("file", "copy");
```

#### `cns_cli_print_version`
```c
void cns_cli_print_version(void);
```
**Description**: Print version information.

**Example**:
```c
cns_cli_print_version();
```

### Error Handling

#### `cns_cli_error`
```c
void cns_cli_error(const char* format, ...);
```
**Description**: Print error message.

**Parameters**:
- `format`: Format string
- `...`: Variable arguments

**Example**:
```c
cns_cli_error("Invalid argument: %s", arg);
```

#### `cns_cli_warning`
```c
void cns_cli_warning(const char* format, ...);
```
**Description**: Print warning message.

**Parameters**:
- `format`: Format string
- `...`: Variable arguments

**Example**:
```c
cns_cli_warning("Deprecated option: %s", option);
```

#### `cns_cli_info`
```c
void cns_cli_info(const char* format, ...);
```
**Description**: Print info message.

**Parameters**:
- `format`: Format string
- `...`: Variable arguments

**Example**:
```c
cns_cli_info("Processing %d items", count);
```

#### `cns_cli_success`
```c
void cns_cli_success(const char* format, ...);
```
**Description**: Print success message.

**Parameters**:
- `format`: Format string
- `...`: Variable arguments

**Example**:
```c
cns_cli_success("Operation completed successfully");
```

### Performance Tracking

#### `cns_get_cycles`
```c
uint64_t cns_get_cycles(void);
```
**Description**: Get current cycle count.

**Returns**: Current cycle count

**Example**:
```c
uint64_t start = cns_get_cycles();
// ... perform operation ...
uint64_t end = cns_get_cycles();
uint64_t cycles = end - start;
```

#### `cns_assert_cycles`
```c
void cns_assert_cycles(uint64_t actual, uint64_t expected);
```
**Description**: Assert cycle count is within expected range.

**Parameters**:
- `actual`: Actual cycle count
- `expected`: Expected cycle count

**Example**:
```c
cns_assert_cycles(cycles, 1000);
```

## Error Codes

### Core Error Codes
- `CNS_OK` (0) - Success
- `CNS_ERROR` (1) - General error
- `CNS_ERROR_ARGS` (2) - Invalid arguments
- `CNS_ERROR_NOT_FOUND` (3) - Command not found
- `CNS_ERROR_PERMISSION` (4) - Permission denied
- `CNS_ERROR_IO` (5) - I/O error
- `CNS_ERROR_MEMORY` (6) - Memory error
- `CNS_ERROR_TIMEOUT` (7) - Timeout error
- `CNS_ERROR_CYCLES` (8) - Cycle limit exceeded

### Special Return Values
- `CNS_CMD_EXIT` - Command requests exit
- `CNS_MAX_COMMANDS` - Maximum commands constant

## Constants

### Limits
- `CNS_MAX_CMD_LEN` - Maximum command name length
- `CNS_MAX_ARG_LEN` - Maximum argument length
- `CNS_MAX_ARGS` - Maximum number of arguments
- `CNS_MAX_COMMANDS` - Maximum number of commands

### Flags
- `CNS_FLAG_NONE` - No flags
- `CNS_FLAG_ADMIN` - Admin command
- `CNS_FLAG_LOGGED` - Logged command

### Performance Targets
- `CNS_SEVEN_TICK_TARGET_NS` - 7-tick target (10ns)
- `CNS_L2_TIER_TARGET_NS` - L2 tier target (100ns)
- `CNS_L3_TIER_TARGET_NS` - L3 tier target (1000ns)

---

*This API reference provides complete documentation for all CNS functions, structures, and constants.* 