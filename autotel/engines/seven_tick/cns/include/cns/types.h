#ifndef CNS_TYPES_H
#define CNS_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Forward declarations
typedef struct CNSContext CNSContext;
typedef struct CNSCommand CNSCommand;
typedef struct CNSDomain CNSDomain;
typedef struct CNSOption CNSOption;
typedef struct CNSArgument CNSArgument;

// Command handler function type
typedef int (*CNSHandler)(CNSContext* ctx, int argc, char** argv);

// Option types
typedef enum {
    CNS_OPT_STRING,
    CNS_OPT_INT,
    CNS_OPT_BOOL,
    CNS_OPT_FLOAT,
    CNS_OPT_FLAG
} CNSOptionType;

// Option structure
struct CNSOption {
    const char* name;        // Long name (e.g., "input")
    char short_name;         // Short name (e.g., 'i')
    CNSOptionType type;      // Option type
    const char* description; // Help text
    const char* default_val; // Default value
    bool required;           // Is required?
};

// Argument structure
struct CNSArgument {
    const char* name;        // Argument name
    const char* description; // Help text
    bool required;           // Is required?
    bool variadic;           // Accept multiple values?
};

// Command structure
struct CNSCommand {
    const char* name;        // Command name (verb)
    const char* description; // Help text
    CNSHandler handler;      // Function to execute
    CNSOption* options;      // Array of options
    size_t option_count;     // Number of options
    CNSArgument* arguments;  // Array of arguments
    size_t argument_count;   // Number of arguments
};

// Domain structure (groups related commands)
struct CNSDomain {
    const char* name;        // Domain name
    const char* description; // Help text
    CNSCommand* commands;    // Array of commands
    size_t command_count;    // Number of commands
};

// Context for command execution
struct CNSContext {
    const char* program_name;     // Program name (cns)
    const char* domain;           // Current domain
    const char* command;          // Current command
    void* parsed_options;         // Parsed options
    void* parsed_arguments;       // Parsed arguments
    bool verbose;                 // Verbose output
    bool json_output;             // JSON output mode
    uint64_t start_cycles;        // Performance tracking
    void* user_data;              // User-defined data
};

// Result codes
typedef enum {
    CNS_OK = 0,
    CNS_ERROR = 1,
    CNS_ERROR_ARGS = 2,
    CNS_ERROR_NOT_FOUND = 3,
    CNS_ERROR_PERMISSION = 4,
    CNS_ERROR_IO = 5,
    CNS_ERROR_MEMORY = 6,
    CNS_ERROR_TIMEOUT = 7,
    CNS_ERROR_CYCLES = 8  // 7-tick violation
} CNSResult;

#endif // CNS_TYPES_H