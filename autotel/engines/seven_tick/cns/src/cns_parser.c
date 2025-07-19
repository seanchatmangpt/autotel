/*  ─────────────────────────────────────────────────────────────
    cns_parser.c  –  Command Parser Implementation (v1.0)
    7-tick compliant command parsing with OpenTelemetry support
    ───────────────────────────────────────────────────────────── */

#include "../include/cns_parser.h"
#include "../include/cns.h"

#ifdef WITH_OPENTELEMETRY
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/exporters/ostream/span_exporter.h>
#include <opentelemetry/sdk/trace/simple_processor.h>

namespace trace = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;
namespace exporter_ostream = opentelemetry::exporter::trace;
#endif

#ifdef WITH_OPENTELEMETRY
// Global tracer for command parsing
static std::shared_ptr<trace::Tracer> g_parser_tracer;
#endif

/*═══════════════════════════════════════════════════════════════
  OpenTelemetry Initialization
  ═══════════════════════════════════════════════════════════════*/

void cns_parser_init_telemetry(void)
{
#ifdef WITH_OPENTELEMETRY
    // Create OStream exporter
    auto exporter = std::unique_ptr<trace_sdk::SpanExporter>(
        new exporter_ostream::OStreamSpanExporter);

    // Create simple processor
    auto processor = std::unique_ptr<trace_sdk::SpanProcessor>(
        new trace_sdk::SimpleSpanProcessor(std::move(exporter)));

    // Create provider
    auto provider = std::shared_ptr<trace::TracerProvider>(
        new trace_sdk::TracerProvider(std::move(processor)));

    // Set global provider
    trace::Provider::SetTracerProvider(provider);

    // Get tracer
    g_parser_tracer = provider->GetTracer("cns_parser", "1.0.0");
#else
    // No-op when OpenTelemetry is disabled
    (void)0;
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
#ifdef WITH_OPENTELEMETRY
    // Create span for parsing
    auto span = g_parser_tracer->StartSpan(span_name ? span_name : "cns_parse");
    auto scope = trace::Scope(span);

    // Add input as attribute
    span->SetAttribute("cns.input", input);
    span->SetAttribute("cns.operation", "parse");

    // Time the parse operation
    uint64_t start_cycles = s7t_cycles();

    // Perform parse
    cns_result_t result = cns_parse(input, cmd);

    // Calculate parse time
    uint64_t parse_cycles = s7t_cycles() - start_cycles;

    // Add parse results to span
    span->SetAttribute("cns.command", cmd->cmd);
    span->SetAttribute("cns.argc", (int)cmd->argc);
    span->SetAttribute("cns.parse_cycles", (int)parse_cycles);
    span->SetAttribute("cns.parse_ticks", (int)(parse_cycles / S7T_CYCLES_PER_TICK));
    span->SetAttribute("cns.hash", (int)cmd->hash);

    // Add arguments as attributes
    for (int i = 0; i < cmd->argc; i++)
    {
        char attr_name[32];
        snprintf(attr_name, sizeof(attr_name), "cns.arg.%d", i);
        span->SetAttribute(attr_name, cmd->args[i]);
    }

    // Set status based on result
    if (result == CNS_OK)
    {
        span->SetStatus(trace::StatusCode::kOk, "Parse successful");
    }
    else
    {
        span->SetStatus(trace::StatusCode::kError, cns_parse_error_str(result));
        span->SetAttribute("cns.error", (int)result);
    }

    // End span
    span->End();

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
    // Create parent span for batch
    auto batch_span = g_parser_tracer->StartSpan("cns_parse_batch");
    auto scope = trace::Scope(batch_span);

    batch_span->SetAttribute("cns.max_commands", (int)max_commands);

    uint64_t start_cycles = s7t_cycles();
    uint32_t count = 0;
    uint32_t errors = 0;

    // Parse commands
    while (*input && count < max_commands)
    {
        // Find line end
        const char *line_end = input;
        while (*line_end && *line_end != '\n')
            line_end++;

        // Parse line
        size_t line_len = line_end - input;
        if (line_len > 0 && line_len < 256)
        {
            char line_buf[256];
            memcpy(line_buf, input, line_len);
            line_buf[line_len] = '\0';

            // Create child span for each command
            char span_name[64];
            snprintf(span_name, sizeof(span_name), "parse_cmd_%d", count);

            if (cns_parse_with_telemetry(line_buf, &commands[count], span_name) == CNS_OK)
            {
                count++;
            }
            else
            {
                errors++;
            }
        }

        // Next line
        input = (*line_end == '\n') ? line_end + 1 : line_end;
    }

    // Calculate total time
    uint64_t total_cycles = s7t_cycles() - start_cycles;

    // Update batch span
    batch_span->SetAttribute("cns.parsed_count", (int)count);
    batch_span->SetAttribute("cns.error_count", (int)errors);
    batch_span->SetAttribute("cns.total_cycles", (int)total_cycles);
    batch_span->SetAttribute("cns.total_ticks", (int)(total_cycles / S7T_CYCLES_PER_TICK));
    batch_span->SetAttribute("cns.avg_cycles_per_cmd", (int)(count > 0 ? total_cycles / count : 0));

    batch_span->SetStatus(trace::StatusCode::kOk, "Batch parse complete");
    batch_span->End();

    return count;
#else
    // Fall back to regular batch parse when OpenTelemetry is disabled
    uint32_t count = 0;
    while (*input && count < max_commands)
    {
        // Find line end
        const char *line_end = input;
        while (*line_end && *line_end != '\n')
            line_end++;

        // Parse line
        size_t line_len = line_end - input;
        if (line_len > 0 && line_len < 256)
        {
            char line_buf[256];
            memcpy(line_buf, input, line_len);
            line_buf[line_len] = '\0';

            if (cns_parse(line_buf, &commands[count]) == CNS_OK)
            {
                count++;
            }
        }

        // Next line
        input = (*line_end == '\n') ? line_end + 1 : line_end;
    }
    return count;
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
    auto span = g_parser_tracer->StartSpan("cns_validate");
    auto scope = trace::Scope(span);

    span->SetAttribute("cns.command", cmd->cmd);
    span->SetAttribute("cns.flags", (int)flags);

    bool valid = true;

    // Validate command name
    if (strlen(cmd->cmd) == 0 || strlen(cmd->cmd) >= CNS_MAX_CMD_LEN)
    {
        span->AddEvent("Invalid command length");
        valid = false;
    }

    // Validate arguments
    for (int i = 0; i < cmd->argc; i++)
    {
        if (!cns_validate_arg(cmd->args[i], flags))
        {
            char event_msg[64];
            snprintf(event_msg, sizeof(event_msg), "Invalid argument %d", i);
            span->AddEvent(event_msg);
            valid = false;
        }
    }

    span->SetAttribute("cns.valid", valid);
    span->SetStatus(valid ? trace::StatusCode::kOk : trace::StatusCode::kError,
                    valid ? "Validation passed" : "Validation failed");
    span->End();

    return valid;
#else
    // Fall back to regular validation when OpenTelemetry is disabled
    bool valid = true;

    // Validate command name
    if (strlen(cmd->cmd) == 0 || strlen(cmd->cmd) >= CNS_MAX_CMD_LEN)
    {
        valid = false;
    }

    // Validate arguments
    for (int i = 0; i < cmd->argc; i++)
    {
        if (!cns_validate_arg(cmd->args[i], flags))
        {
            valid = false;
        }
    }

    return valid;
#endif
}

/*═══════════════════════════════════════════════════════════════
  Parser Metrics and Analytics
  ═══════════════════════════════════════════════════════════════*/

typedef struct
{
    uint64_t total_parses;
    uint64_t successful_parses;
    uint64_t failed_parses;
    uint64_t total_cycles;
    uint64_t min_cycles;
    uint64_t max_cycles;
    uint64_t commands_by_type[6]; // CNS_CMD_* types
} cns_parser_metrics_t;

static cns_parser_metrics_t g_parser_metrics = {
    .min_cycles = UINT64_MAX};

// Update parser metrics
void cns_parser_update_metrics(
    cns_result_t result,
    uint64_t cycles,
    cns_cmd_type_t type)
{
    g_parser_metrics.total_parses++;
    g_parser_metrics.total_cycles += cycles;

    if (result == CNS_OK)
    {
        g_parser_metrics.successful_parses++;
        if (type < 6)
        {
            g_parser_metrics.commands_by_type[type]++;
        }
    }
    else
    {
        g_parser_metrics.failed_parses++;
    }

    if (cycles < g_parser_metrics.min_cycles)
    {
        g_parser_metrics.min_cycles = cycles;
    }
    if (cycles > g_parser_metrics.max_cycles)
    {
        g_parser_metrics.max_cycles = cycles;
    }
}

// Export parser metrics to telemetry
void cns_parser_export_metrics(void)
{
#ifdef WITH_OPENTELEMETRY
    auto span = g_parser_tracer->StartSpan("cns_parser_metrics");

    span->SetAttribute("cns.metrics.total_parses", (int)g_parser_metrics.total_parses);
    span->SetAttribute("cns.metrics.successful_parses", (int)g_parser_metrics.successful_parses);
    span->SetAttribute("cns.metrics.failed_parses", (int)g_parser_metrics.failed_parses);
    span->SetAttribute("cns.metrics.avg_cycles",
                       (int)(g_parser_metrics.total_parses > 0 ? g_parser_metrics.total_cycles / g_parser_metrics.total_parses : 0));
    span->SetAttribute("cns.metrics.min_cycles", (int)g_parser_metrics.min_cycles);
    span->SetAttribute("cns.metrics.max_cycles", (int)g_parser_metrics.max_cycles);

    // Command type distribution
    span->SetAttribute("cns.metrics.exec_commands", (int)g_parser_metrics.commands_by_type[CNS_CMD_EXEC]);
    span->SetAttribute("cns.metrics.query_commands", (int)g_parser_metrics.commands_by_type[CNS_CMD_QUERY]);
    span->SetAttribute("cns.metrics.config_commands", (int)g_parser_metrics.commands_by_type[CNS_CMD_CONFIG]);
    span->SetAttribute("cns.metrics.help_commands", (int)g_parser_metrics.commands_by_type[CNS_CMD_HELP]);

    span->End();
#else
    // No-op when OpenTelemetry is disabled
    (void)0;
#endif
}

/*═══════════════════════════════════════════════════════════════
  Parser Cleanup
  ═══════════════════════════════════════════════════════════════*/

void cns_parser_cleanup(void)
{
    // Export final metrics
    cns_parser_export_metrics();

#ifdef WITH_OPENTELEMETRY
    // Reset tracer
    g_parser_tracer.reset();
#endif
}