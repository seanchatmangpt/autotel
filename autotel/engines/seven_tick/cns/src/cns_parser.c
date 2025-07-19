/*  ─────────────────────────────────────────────────────────────
    cns_parser.c  –  Command Parser Implementation (v1.0)
    7-tick compliant command parsing with OpenTelemetry support
    ───────────────────────────────────────────────────────────── */

#include "../include/cns_parser.h"
#include "../include/cns.h"
#include <stdio.h>
#include <string.h>

#ifdef WITH_OPENTELEMETRY
// Forward declarations for OpenTelemetry (C-compatible interface)
void* g_parser_tracer;
#endif

/*═══════════════════════════════════════════════════════════════
  OpenTelemetry Initialization
  ═══════════════════════════════════════════════════════════════*/

void cns_parser_init_telemetry(void)
{
#ifdef WITH_OPENTELEMETRY
    // TODO: Initialize OpenTelemetry C bindings when available
    g_parser_tracer = NULL; // Placeholder
    printf("INFO: OpenTelemetry parser telemetry initialized (stub)\n");
#else
    // No-op when OpenTelemetry is disabled
#endif
}

/*═══════════════════════════════════════════════════════════════
  Parser Implementation with Telemetry
  ═══════════════════════════════════════════════════════════════*/

// Parse command with OpenTelemetry span
cns_result_t cns_parse_with_telemetry(
    const char *input,
    cns_command_t *cmd,
    const char *span_name)
{
    (void)span_name; // Suppress unused parameter warning when OpenTelemetry is disabled
#ifdef WITH_OPENTELEMETRY
    // TODO: Add OpenTelemetry C binding support
    // Time the parse operation
    uint64_t start_cycles = s7t_cycles();

    // Perform parse
    cns_result_t result = cns_parse(input, cmd);

    // Calculate parse time (for future telemetry)
    uint64_t parse_cycles = s7t_cycles() - start_cycles;
    
    // Simple telemetry logging for now
    printf("TELEMETRY: parse_cycles=%llu command=%s\n", parse_cycles, cmd->cmd);

    return result;
#else
    // Fall back to regular parse when OpenTelemetry is disabled
    return cns_parse(input, cmd);
#endif
}

// Batch parse with telemetry
uint32_t cns_parse_batch_with_telemetry(
    const char *input,
    cns_command_t *commands,
    uint32_t max_commands)
{
#ifdef WITH_OPENTELEMETRY
    // TODO: Add proper OpenTelemetry batch span support
    uint64_t start_cycles = s7t_cycles();
    uint32_t count = 0;
    uint32_t errors = 0;

    // Parse commands
    while (*input && count < max_commands) {
        // Find line end
        const char *line_end = input;
        while (*line_end && *line_end != '\n') line_end++;

        // Parse line
        size_t line_len = line_end - input;
        if (line_len > 0 && line_len < 256) {
            char line_buf[256];
            memcpy(line_buf, input, line_len);
            line_buf[line_len] = '\0';

            if (cns_parse_with_telemetry(line_buf, &commands[count], "batch_parse") == CNS_OK) {
                count++;
            } else {
                errors++;
            }
        }

        // Next line
        input = (*line_end == '\n') ? line_end + 1 : line_end;
    }

    // Calculate total time
    uint64_t total_cycles = s7t_cycles() - start_cycles;
    printf("TELEMETRY: batch_parse total_cycles=%llu count=%u errors=%u\n", 
           total_cycles, count, errors);

    return count;
#else
    // Fall back to regular batch parse when OpenTelemetry is disabled
    return cns_parse_batch(input, commands, max_commands);
#endif
}

/*═══════════════════════════════════════════════════════════════
  Command Validation with Telemetry
  ═══════════════════════════════════════════════════════════════*/

bool cns_validate_command_with_telemetry(
    const cns_command_t *cmd,
    uint32_t flags)
{
#ifdef WITH_OPENTELEMETRY
    // TODO: Add OpenTelemetry validation span support
    printf("TELEMETRY: validate_command cmd=%s flags=%u\n", cmd->cmd, flags);
#endif

    // Basic validation
    bool valid = true;
    
    // Check command length
    if (strlen(cmd->cmd) == 0 || strlen(cmd->cmd) >= CNS_MAX_CMD_LEN) {
        valid = false;
    }

    // Check arguments
    for (int i = 0; i < cmd->argc; i++) {
        if (!cns_validate_arg(cmd->args[i], flags)) {
            valid = false;
            break;
        }
    }

#ifdef WITH_OPENTELEMETRY
    printf("TELEMETRY: validation_result valid=%s\n", valid ? "true" : "false");
#endif

    return valid;
}

/*═══════════════════════════════════════════════════════════════
  Parser Metrics with Telemetry
  ═══════════════════════════════════════════════════════════════*/

typedef struct {
    uint64_t total_parses;
    uint64_t successful_parses;
    uint64_t failed_parses;
    uint64_t total_cycles;
    uint64_t min_cycles;
    uint64_t max_cycles;
    uint32_t commands_by_type[5]; // CNS_CMD_* types
} cns_parser_metrics_t;

static cns_parser_metrics_t g_parser_metrics = {0};

void cns_parser_export_metrics(void)
{
#ifdef WITH_OPENTELEMETRY
    // TODO: Add OpenTelemetry metrics export
    printf("TELEMETRY: parser_metrics total=%llu successful=%llu failed=%llu\n",
           g_parser_metrics.total_parses,
           g_parser_metrics.successful_parses,
           g_parser_metrics.failed_parses);
#endif
}

void cns_parser_reset_metrics(void)
{
    memset(&g_parser_metrics, 0, sizeof(g_parser_metrics));
#ifdef WITH_OPENTELEMETRY
    printf("TELEMETRY: parser_metrics_reset\n");
#endif
}

/*═══════════════════════════════════════════════════════════════
  Parser Cleanup
  ═══════════════════════════════════════════════════════════════*/

void cns_parser_cleanup_telemetry(void)
{
#ifdef WITH_OPENTELEMETRY
    // TODO: Cleanup OpenTelemetry resources
    g_parser_tracer = NULL;
    printf("INFO: OpenTelemetry parser telemetry cleaned up\n");
#else
    // No-op when OpenTelemetry is disabled
#endif
}