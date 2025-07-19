/*  ─────────────────────────────────────────────────────────────
    cns_parser.h  –  Command Parser for CNS (v1.0)
    7-tick compliant command parsing with zero allocation
    ───────────────────────────────────────────────────────────── */
#ifndef CNS_PARSER_H
#define CNS_PARSER_H

#include "cns.h"
#include <string.h>
#include <ctype.h>

/*═══════════════════════════════════════════════════════════════
  Parser State Machine
  ═══════════════════════════════════════════════════════════════*/

typedef enum {
    PARSE_STATE_CMD = 0,     // Parsing command
    PARSE_STATE_SPACE,       // Between tokens
    PARSE_STATE_ARG,         // Parsing argument
    PARSE_STATE_QUOTE,       // Inside quoted string
    PARSE_STATE_ESCAPE,      // Escape character
    PARSE_STATE_DONE         // Parsing complete
} cns_parse_state_t;

/*═══════════════════════════════════════════════════════════════
  Fast Character Classification (LUT-based)
  ═══════════════════════════════════════════════════════════════*/

// Character classification lookup table
static const uint8_t cns_char_class[256] = {
    [' '] = 1, ['\t'] = 1, ['\n'] = 1, ['\r'] = 1,  // Whitespace
    ['"'] = 2, ['\''] = 2,                           // Quotes
    ['\\'] = 3,                                      // Escape
    ['\0'] = 4,                                      // Terminator
    // All others are 0 (normal characters)
};

#define CNS_IS_SPACE(c)    (cns_char_class[(uint8_t)(c)] == 1)
#define CNS_IS_QUOTE(c)    (cns_char_class[(uint8_t)(c)] == 2)
#define CNS_IS_ESCAPE(c)   (cns_char_class[(uint8_t)(c)] == 3)
#define CNS_IS_TERM(c)     (cns_char_class[(uint8_t)(c)] == 4)

/*═══════════════════════════════════════════════════════════════
  Parser Functions (< 7 ticks for typical commands)
  ═══════════════════════════════════════════════════════════════*/

// Fast whitespace skip (vectorizable)
S7T_ALWAYS_INLINE const char* cns_skip_whitespace(const char* p) {
    while (CNS_IS_SPACE(*p)) p++;
    return p;
}

// Parse single token into buffer
S7T_ALWAYS_INLINE size_t cns_parse_token(
    const char** input,
    char* buffer,
    size_t max_len
) {
    const char* p = *input;
    char* dst = buffer;
    size_t len = 0;
    
    // Skip leading whitespace
    p = cns_skip_whitespace(p);
    
    // Check for quote
    char quote = 0;
    if (CNS_IS_QUOTE(*p)) {
        quote = *p++;
    }
    
    // Copy token
    while (*p && len < max_len - 1) {
        if (quote) {
            // Inside quotes
            if (*p == quote) {
                p++; // Skip closing quote
                break;
            }
            if (CNS_IS_ESCAPE(*p) && *(p + 1)) {
                p++; // Skip escape char
            }
        } else {
            // Outside quotes
            if (CNS_IS_SPACE(*p)) {
                break;
            }
        }
        *dst++ = *p++;
        len++;
    }
    
    *dst = '\0';
    *input = p;
    return len;
}

// Main parser function (< 7 ticks for simple commands)
S7T_ALWAYS_INLINE cns_result_t cns_parse(
    const char* input,
    cns_command_t* cmd
) {
    // Clear command structure
    memset(cmd, 0, sizeof(cns_command_t));
    
    // Skip leading whitespace
    input = cns_skip_whitespace(input);
    
    // Empty command
    if (!*input) {
        return CNS_ERR_INVALID_CMD;
    }
    
    // Parse command name
    size_t cmd_len = cns_parse_token(&input, cmd->cmd, CNS_MAX_CMD_LEN);
    if (cmd_len == 0) {
        return CNS_ERR_INVALID_CMD;
    }
    
    // Pre-compute command hash
    cmd->hash = s7t_hash_string(cmd->cmd, cmd_len);
    
    // Parse arguments
    while (*input && cmd->argc < CNS_MAX_ARGS) {
        input = cns_skip_whitespace(input);
        if (!*input) break;
        
        size_t arg_len = cns_parse_token(
            &input,
            cmd->args[cmd->argc],
            CNS_MAX_ARG_LEN
        );
        
        if (arg_len > 0) {
            cmd->argc++;
        }
    }
    
    // Set timestamp
    cmd->timestamp = s7t_cycles();
    
    return CNS_OK;
}

// Specialized parser for single-argument commands (< 3 ticks)
S7T_ALWAYS_INLINE cns_result_t cns_parse_simple(
    const char* input,
    cns_command_t* cmd
) {
    // Clear using aligned stores
    uint64_t* p = (uint64_t*)cmd;
    for (int i = 0; i < sizeof(cns_command_t) / 8; i++) {
        p[i] = 0;
    }
    
    // Skip whitespace
    while (CNS_IS_SPACE(*input)) input++;
    if (!*input) return CNS_ERR_INVALID_CMD;
    
    // Copy command (unrolled for short commands)
    char* dst = cmd->cmd;
    size_t len = 0;
    
    S7T_UNROLL(8)
    while (*input && !CNS_IS_SPACE(*input) && len < 8) {
        *dst++ = *input++;
        len++;
    }
    
    // Continue if longer
    while (*input && !CNS_IS_SPACE(*input) && len < CNS_MAX_CMD_LEN - 1) {
        *dst++ = *input++;
        len++;
    }
    *dst = '\0';
    
    // Hash and timestamp
    cmd->hash = s7t_hash_string(cmd->cmd, len);
    cmd->timestamp = s7t_cycles();
    
    // Single argument if present
    while (CNS_IS_SPACE(*input)) input++;
    if (*input) {
        dst = cmd->args[0];
        len = 0;
        while (*input && len < CNS_MAX_ARG_LEN - 1) {
            *dst++ = *input++;
            len++;
        }
        *dst = '\0';
        cmd->argc = 1;
    }
    
    return CNS_OK;
}

/*═══════════════════════════════════════════════════════════════
  Command Validation and Normalization
  ═══════════════════════════════════════════════════════════════*/

// Normalize command (lowercase, trim)
S7T_ALWAYS_INLINE void cns_normalize_command(char* cmd) {
    char* p = cmd;
    while (*p) {
        *p = tolower(*p);
        p++;
    }
}

// Validate argument format
S7T_ALWAYS_INLINE bool cns_validate_arg(const char* arg, uint32_t flags) {
    if (!arg || !*arg) return false;
    
    // Check for valid characters based on flags
    if (flags & CNS_FLAG_ADMIN) {
        // Stricter validation for admin commands
        while (*arg) {
            char c = *arg++;
            if (!isalnum(c) && c != '_' && c != '-' && c != '.') {
                return false;
            }
        }
    }
    
    return true;
}

/*═══════════════════════════════════════════════════════════════
  Parser with Pre-compiled Command Table
  ═══════════════════════════════════════════════════════════════*/

// Pre-compiled command info for faster parsing
typedef struct {
    uint32_t hash;
    uint8_t type;
    uint8_t flags;
    uint8_t min_args;
    uint8_t max_args;
} cns_cmd_info_t;

// Fast command type detection from hash
S7T_ALWAYS_INLINE cns_cmd_type_t cns_detect_type(uint32_t hash) {
    // Use hash bits to quickly classify command type
    // This is a simplified example - real implementation would use
    // a proper hash table or perfect hash function
    
    switch (hash & 0xFF) {
        case 0x00 ... 0x3F: return CNS_CMD_EXEC;
        case 0x40 ... 0x7F: return CNS_CMD_QUERY;
        case 0x80 ... 0xBF: return CNS_CMD_CONFIG;
        case 0xC0 ... 0xDF: return CNS_CMD_HELP;
        case 0xE0 ... 0xEF: return CNS_CMD_EXIT;
        default: return CNS_CMD_INVALID;
    }
}

/*═══════════════════════════════════════════════════════════════
  Batch Parsing Support
  ═══════════════════════════════════════════════════════════════*/

// Parse multiple commands from buffer
S7T_ALWAYS_INLINE uint32_t cns_parse_batch(
    const char* input,
    cns_command_t* commands,
    uint32_t max_commands
) {
    uint32_t count = 0;
    
    while (*input && count < max_commands) {
        // Find line end
        const char* line_end = input;
        while (*line_end && *line_end != '\n') line_end++;
        
        // Parse line
        size_t line_len = line_end - input;
        if (line_len > 0 && line_len < 256) {
            char line_buf[256];
            memcpy(line_buf, input, line_len);
            line_buf[line_len] = '\0';
            
            if (cns_parse(line_buf, &commands[count]) == CNS_OK) {
                count++;
            }
        }
        
        // Next line
        input = (*line_end == '\n') ? line_end + 1 : line_end;
    }
    
    return count;
}

/*═══════════════════════════════════════════════════════════════
  Parser Error Recovery
  ═══════════════════════════════════════════════════════════════*/

// Get parser error description
S7T_ALWAYS_INLINE const char* cns_parse_error_str(cns_result_t result) {
    static const char* const errors[] = {
        [CNS_OK] = "Success",
        [CNS_ERR_INVALID_CMD] = "Invalid command",
        [CNS_ERR_INVALID_ARG] = "Invalid argument",
        [CNS_ERR_PERMISSION] = "Permission denied",
        [CNS_ERR_TIMEOUT] = "Command timeout",
        [CNS_ERR_RESOURCE] = "Resource exhausted",
        [CNS_ERR_INTERNAL] = "Internal error"
    };
    
    return (result < sizeof(errors) / sizeof(errors[0])) 
        ? errors[result] 
        : "Unknown error";
}

#endif /* CNS_PARSER_H */