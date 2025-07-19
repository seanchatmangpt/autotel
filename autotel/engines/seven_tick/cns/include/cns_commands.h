/*  ─────────────────────────────────────────────────────────────
    cns_commands.h  –  Built-in Command Handlers (v1.0)
    Standard command implementations for CNS
    ───────────────────────────────────────────────────────────── */
#ifndef CNS_COMMANDS_H
#define CNS_COMMANDS_H

#include "cns.h"
#include "cns_parser.h"
#include <stdio.h>

/*═══════════════════════════════════════════════════════════════
  Command Categories
  ═══════════════════════════════════════════════════════════════*/

// Command categories for organization
typedef enum {
    CNS_CAT_SYSTEM = 0,
    CNS_CAT_FILE,
    CNS_CAT_PROCESS,
    CNS_CAT_NETWORK,
    CNS_CAT_DEBUG,
    CNS_CAT_ADMIN
} cns_category_t;

/*═══════════════════════════════════════════════════════════════
  Built-in Command Handlers
  ═══════════════════════════════════════════════════════════════*/

// Help command - list all available commands
CNS_HANDLER(cns_cmd_help) {
    cns_engine_t* engine = (cns_engine_t*)context;
    
    if (cmd->argc == 0) {
        // List all commands
        printf("Available commands:\n");
        for (uint32_t i = 0; i < engine->cmd_count; i++) {
            printf("  %-16s %s\n", 
                engine->commands[i].name,
                engine->commands[i].help);
        }
    } else {
        // Help for specific command
        uint32_t hash = s7t_hash_string(cmd->args[0], strlen(cmd->args[0]));
        cns_cmd_entry_t* entry = cns_lookup(engine, hash);
        if (entry) {
            printf("%s: %s\n", entry->name, entry->help);
            printf("Arguments: %d-%d\n", entry->min_args, entry->max_args);
        } else {
            printf("Unknown command: %s\n", cmd->args[0]);
            return CNS_ERR_INVALID_ARG;
        }
    }
    
    return CNS_OK;
}

// Exit command
CNS_HANDLER(cns_cmd_exit) {
    // Signal exit by returning special code
    return CNS_CMD_EXIT;
}

// Echo command - print arguments
CNS_HANDLER(cns_cmd_echo) {
    for (uint8_t i = 0; i < cmd->argc; i++) {
        printf("%s%s", cmd->args[i], (i < cmd->argc - 1) ? " " : "\n");
    }
    return CNS_OK;
}

// Status command - show engine statistics
CNS_HANDLER(cns_cmd_status) {
    cns_engine_t* engine = (cns_engine_t*)context;
    
    printf("CNS Engine Status:\n");
    printf("  Commands registered: %u/%u\n", 
        engine->cmd_count, engine->max_commands);
    printf("  Performance:\n");
    printf("    Total commands: %lu\n", engine->perf.count);
    if (engine->perf.count > 0) {
        printf("    Avg cycles: %lu\n", 
            engine->perf.cycles / engine->perf.count);
        printf("    Min cycles: %lu\n", engine->perf.min_cycles);
        printf("    Max cycles: %lu\n", engine->perf.max_cycles);
    }
    
    return CNS_OK;
}

// Time command - measure command execution time
CNS_HANDLER(cns_cmd_time) {
    if (cmd->argc < 1) {
        return CNS_ERR_INVALID_ARG;
    }
    
    // Parse and execute the timed command
    cns_command_t timed_cmd;
    
    // Reconstruct command line
    char cmdline[256];
    size_t pos = 0;
    for (uint8_t i = 0; i < cmd->argc && pos < sizeof(cmdline) - 1; i++) {
        size_t len = strlen(cmd->args[i]);
        if (pos + len + 1 < sizeof(cmdline)) {
            memcpy(cmdline + pos, cmd->args[i], len);
            pos += len;
            if (i < cmd->argc - 1) {
                cmdline[pos++] = ' ';
            }
        }
    }
    cmdline[pos] = '\0';
    
    // Parse the command
    cns_result_t result = cns_parse(cmdline, &timed_cmd);
    if (result != CNS_OK) {
        return result;
    }
    
    // Time the execution
    uint64_t start = s7t_cycles();
    cns_engine_t* engine = (cns_engine_t*)context;
    result = cns_execute(engine, &timed_cmd);
    uint64_t cycles = s7t_cycles() - start;
    
    printf("Execution time: %lu cycles (%.2f ns)\n", 
        cycles, cycles * S7T_NS_PER_CYCLE);
    
    return result;
}

// Clear command - clear screen
CNS_HANDLER(cns_cmd_clear) {
    printf("\033[2J\033[H"); // ANSI escape codes
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  Advanced Command Handlers
  ═══════════════════════════════════════════════════════════════*/

// Batch command - execute multiple commands from file
CNS_HANDLER(cns_cmd_batch) {
    if (cmd->argc < 1) {
        return CNS_ERR_INVALID_ARG;
    }
    
    // This is a placeholder - actual implementation would:
    // 1. Open file
    // 2. Read commands line by line
    // 3. Parse and execute each
    // 4. Handle errors appropriately
    
    printf("Batch execution not implemented\n");
    return CNS_OK;
}

// Alias command - create command alias
CNS_HANDLER(cns_cmd_alias) {
    if (cmd->argc < 2) {
        return CNS_ERR_INVALID_ARG;
    }
    
    // This would store alias mapping in engine context
    printf("Alias '%s' -> '%s' (not implemented)\n", 
        cmd->args[0], cmd->args[1]);
    
    return CNS_OK;
}

// History command - show command history
CNS_HANDLER(cns_cmd_history) {
    // This would require maintaining a history buffer
    printf("Command history not implemented\n");
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  Debug Commands
  ═══════════════════════════════════════════════════════════════*/

// Debug command - toggle debug mode
CNS_HANDLER(cns_cmd_debug) {
    cns_engine_t* engine = (cns_engine_t*)context;
    
    if (cmd->argc > 0) {
        if (strcmp(cmd->args[0], "on") == 0) {
            engine->flags |= CNS_FLAG_LOGGED;
            printf("Debug mode enabled\n");
        } else if (strcmp(cmd->args[0], "off") == 0) {
            engine->flags &= ~CNS_FLAG_LOGGED;
            printf("Debug mode disabled\n");
        } else {
            return CNS_ERR_INVALID_ARG;
        }
    } else {
        printf("Debug mode: %s\n", 
            (engine->flags & CNS_FLAG_LOGGED) ? "on" : "off");
    }
    
    return CNS_OK;
}

// Dump command - dump internal state
CNS_HANDLER(cns_cmd_dump) {
    cns_engine_t* engine = (cns_engine_t*)context;
    
    printf("=== CNS Engine Dump ===\n");
    printf("Commands (%u):\n", engine->cmd_count);
    
    for (uint32_t i = 0; i < engine->cmd_count; i++) {
        cns_cmd_entry_t* e = &engine->commands[i];
        printf("  [%u] %s (hash: 0x%08x, flags: 0x%04x, args: %u-%u)\n",
            i, e->name, e->hash, e->flags, e->min_args, e->max_args);
    }
    
    printf("\nHash Table:\n");
    uint32_t used = 0;
    for (uint32_t i = 0; i < engine->table_size; i++) {
        if (engine->hash_table[i] != CNS_MAX_COMMANDS) {
            used++;
        }
    }
    printf("  Size: %u, Used: %u (%.1f%%)\n", 
        engine->table_size, used, (100.0 * used) / engine->table_size);
    
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  External Command Handlers
  ═══════════════════════════════════════════════════════════════*/

// Test command handler (implemented in cmd_test.c)
CNS_HANDLER(cns_cmd_test);

/*═══════════════════════════════════════════════════════════════
  Command Registration Helper
  ═══════════════════════════════════════════════════════════════*/

// Register all built-in commands
S7T_ALWAYS_INLINE void cns_register_builtins(cns_engine_t* engine) {
    // System commands
    CNS_REGISTER_CMD(engine, "help", cns_cmd_help, 0, 1, 
        "Show help for commands");
    CNS_REGISTER_CMD(engine, "exit", cns_cmd_exit, 0, 0, 
        "Exit the command interface");
    CNS_REGISTER_CMD(engine, "quit", cns_cmd_exit, 0, 0, 
        "Exit the command interface");
    CNS_REGISTER_CMD(engine, "echo", cns_cmd_echo, 0, CNS_MAX_ARGS, 
        "Print arguments");
    CNS_REGISTER_CMD(engine, "status", cns_cmd_status, 0, 0, 
        "Show engine status");
    CNS_REGISTER_CMD(engine, "clear", cns_cmd_clear, 0, 0, 
        "Clear screen");
    CNS_REGISTER_CMD(engine, "time", cns_cmd_time, 1, CNS_MAX_ARGS, 
        "Time command execution");
    
    // Advanced commands
    CNS_REGISTER_CMD(engine, "batch", cns_cmd_batch, 1, 1, 
        "Execute commands from file");
    CNS_REGISTER_CMD(engine, "alias", cns_cmd_alias, 2, 2, 
        "Create command alias");
    CNS_REGISTER_CMD(engine, "history", cns_cmd_history, 0, 1, 
        "Show command history");
    
    // Debug commands
    CNS_REGISTER_ADMIN(engine, "debug", cns_cmd_debug, 0, 1, 
        "Toggle debug mode");
    CNS_REGISTER_ADMIN(engine, "dump", cns_cmd_dump, 0, 0, 
        "Dump internal state");
    
    // Test command
    CNS_REGISTER_CMD(engine, "test", cns_cmd_test, 0, CNS_MAX_ARGS, 
        "Run unit tests");
}

/*═══════════════════════════════════════════════════════════════
  Command Handler Templates
  ═══════════════════════════════════════════════════════════════*/

// Template for file operation commands
#define CNS_FILE_HANDLER(name, operation) \
    CNS_HANDLER(name) { \
        if (cmd->argc < 1) return CNS_ERR_INVALID_ARG; \
        /* File operation logic here */ \
        return CNS_OK; \
    }

// Template for network commands
#define CNS_NET_HANDLER(name, operation) \
    CNS_HANDLER(name) { \
        /* Network operation logic here */ \
        return CNS_OK; \
    }

// Template for process commands
#define CNS_PROC_HANDLER(name, operation) \
    CNS_HANDLER(name) { \
        /* Process operation logic here */ \
        return CNS_OK; \
    }

/*═══════════════════════════════════════════════════════════════
  Command Completion Support
  ═══════════════════════════════════════════════════════════════*/

// Find commands matching prefix (for tab completion)
S7T_ALWAYS_INLINE uint32_t cns_complete(
    cns_engine_t* engine,
    const char* prefix,
    const char** matches,
    uint32_t max_matches
) {
    size_t prefix_len = strlen(prefix);
    uint32_t count = 0;
    
    for (uint32_t i = 0; i < engine->cmd_count && count < max_matches; i++) {
        if (strncmp(engine->commands[i].name, prefix, prefix_len) == 0) {
            matches[count++] = engine->commands[i].name;
        }
    }
    
    return count;
}

#endif /* CNS_COMMANDS_H */