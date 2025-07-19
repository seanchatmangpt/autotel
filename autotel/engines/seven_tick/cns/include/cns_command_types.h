#ifndef CNS_COMMAND_TYPES_H
#define CNS_COMMAND_TYPES_H

// Command identifiers for CNS
typedef enum {
    CMD_UNKNOWN = 0,
    
    // Core cognitive commands
    CMD_SPIN,
    CMD_THINK,
    CMD_REFLECT,
    CMD_LEARN,
    CMD_ADAPT,
    
    // System commands
    CMD_SYNC,
    CMD_FLOW,
    CMD_MEASURE,
    CMD_QUERY,
    CMD_TRANSFORM,
    CMD_OPTIMIZE,
    CMD_VALIDATE,
    
    // Benchmark and ML commands
    CMD_BENCHMARK,
    CMD_ML,
    CMD_PM,
    
    // Trace/telemetry commands
    CMD_TRACE,
    
    // Help and system
    CMD_HELP,
    CMD_VERSION,
    CMD_EXIT,
    
    CMD_MAX
} cns_command_id_t;

// Command metadata structure (different from parsed command)
typedef struct {
    cns_command_id_t id;
    const char* name;
    int (*handler)(void* ctx, int argc, char** argv, void (*output)(const char*, ...));
    const char* description;
} cns_command_meta_t;

#endif // CNS_COMMAND_TYPES_H