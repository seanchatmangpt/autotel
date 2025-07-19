/*  ─────────────────────────────────────────────────────────────
    cns_main.c  –  CNS Main Entry Point with OpenTelemetry (v1.0)
    7-tick compliant command dispatch with telemetry
    ───────────────────────────────────────────────────────────── */

#include "../include/cns.h"
#include "../include/cns_parser.h"
#include "../include/cns_commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <opentelemetry/trace/provider.h>
#include <opentelemetry/sdk/trace/tracer_provider.h>
#include <opentelemetry/exporters/ostream/span_exporter.h>
#include <opentelemetry/sdk/trace/simple_processor.h>

namespace trace = opentelemetry::trace;
namespace trace_sdk = opentelemetry::sdk::trace;
namespace exporter_ostream = opentelemetry::exporter::trace;

// External parser telemetry init
extern void cns_parser_init_telemetry(void);
extern cns_result_t cns_parse_with_telemetry(const char*, cns_command_t*, const char*);

// Global CNS engine instance
static cns_engine_t g_cns_engine;
static cns_cmd_entry_t g_commands[CNS_MAX_COMMANDS];
static uint32_t g_hash_table[1 << CNS_HASH_BITS];
static std::shared_ptr<trace::Tracer> g_cns_tracer;

/*═══════════════════════════════════════════════════════════════
  Command Handler Declarations
  ═══════════════════════════════════════════════════════════════*/

// Core command handlers
CNS_HANDLER(cmd_spin_handler);
CNS_HANDLER(cmd_think_handler);
CNS_HANDLER(cmd_reflect_handler);
CNS_HANDLER(cmd_learn_handler);
CNS_HANDLER(cmd_adapt_handler);
CNS_HANDLER(cmd_benchmark_handler);
CNS_HANDLER(cmd_help_handler);
CNS_HANDLER(cmd_version_handler);
CNS_HANDLER(cmd_exit_handler);

/*═══════════════════════════════════════════════════════════════
  OpenTelemetry Initialization
  ═══════════════════════════════════════════════════════════════*/

static void cns_init_telemetry(void) {
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
    g_cns_tracer = provider->GetTracer("cns_main", "1.0.0");
    
    // Initialize parser telemetry
    cns_parser_init_telemetry();
}

/*═══════════════════════════════════════════════════════════════
  Command Registration
  ═══════════════════════════════════════════════════════════════*/

static void cns_register_commands(void) {
    // Initialize engine
    cns_init(&g_cns_engine, g_commands, g_hash_table, CNS_MAX_COMMANDS);
    
    // Register core commands
    CNS_REGISTER_CMD(&g_cns_engine, "spin", cmd_spin_handler, 0, 2, 
                     "Start cognitive processing");
    CNS_REGISTER_CMD(&g_cns_engine, "think", cmd_think_handler, 1, 3,
                     "Execute reasoning operations");
    CNS_REGISTER_CMD(&g_cns_engine, "reflect", cmd_reflect_handler, 0, 2,
                     "Analyze and introspect");
    CNS_REGISTER_CMD(&g_cns_engine, "learn", cmd_learn_handler, 1, 4,
                     "Pattern recognition and learning");
    CNS_REGISTER_CMD(&g_cns_engine, "adapt", cmd_adapt_handler, 1, 3,
                     "Modify behavior based on feedback");
    CNS_REGISTER_CMD(&g_cns_engine, "benchmark", cmd_benchmark_handler, 0, 10,
                     "Run 7-tick performance benchmarks");
    
    // System commands
    CNS_REGISTER_CMD(&g_cns_engine, "help", cmd_help_handler, 0, 1,
                     "Show help information");
    CNS_REGISTER_CMD(&g_cns_engine, "version", cmd_version_handler, 0, 0,
                     "Show version information");
    CNS_REGISTER_CMD(&g_cns_engine, "exit", cmd_exit_handler, 0, 0,
                     "Exit CNS");
    CNS_REGISTER_CMD(&g_cns_engine, "quit", cmd_exit_handler, 0, 0,
                     "Exit CNS");
}

/*═══════════════════════════════════════════════════════════════
  Command Execution with Telemetry
  ═══════════════════════════════════════════════════════════════*/

static cns_result_t cns_execute_with_telemetry(
    const char* input,
    const char* source
) {
    // Create execution span
    auto exec_span = g_cns_tracer->StartSpan("cns_execute");
    auto scope = trace::Scope(exec_span);
    
    exec_span->SetAttribute("cns.input", input);
    exec_span->SetAttribute("cns.source", source ? source : "cli");
    
    // Parse command
    cns_command_t cmd;
    cns_result_t parse_result = cns_parse_with_telemetry(input, &cmd, "parse_input");
    
    if (parse_result != CNS_OK) {
        exec_span->SetStatus(trace::StatusCode::kError, "Parse failed");
        exec_span->SetAttribute("cns.error", (int)parse_result);
        exec_span->End();
        return parse_result;
    }
    
    // Execute command
    uint64_t exec_start = s7t_cycles();
    cns_result_t exec_result = cns_execute(&g_cns_engine, &cmd);
    uint64_t exec_cycles = s7t_cycles() - exec_start;
    
    // Add execution metrics to span
    exec_span->SetAttribute("cns.exec_cycles", (int)exec_cycles);
    exec_span->SetAttribute("cns.exec_ticks", (int)(exec_cycles / S7T_CYCLES_PER_TICK));
    exec_span->SetAttribute("cns.result", (int)exec_result);
    
    // Check 7-tick constraint
    if (exec_cycles > 7 * S7T_CYCLES_PER_TICK) {
        exec_span->AddEvent("7-tick constraint violated");
        exec_span->SetAttribute("cns.constraint_violated", true);
    }
    
    // Set final status
    if (exec_result == CNS_OK) {
        exec_span->SetStatus(trace::StatusCode::kOk, "Command executed successfully");
    } else {
        exec_span->SetStatus(trace::StatusCode::kError, "Command execution failed");
    }
    
    exec_span->End();
    return exec_result;
}

/*═══════════════════════════════════════════════════════════════
  Command Handlers Implementation
  ═══════════════════════════════════════════════════════════════*/

CNS_HANDLER(cmd_help_handler) {
    printf("CNS - Cognitive Nano Stack (7-tick optimized)\n");
    printf("Usage: cns <command> [options]\n\n");
    printf("Commands:\n");
    
    // List all registered commands
    for (uint32_t i = 0; i < g_cns_engine.cmd_count; i++) {
        printf("  %-12s %s\n", 
               g_cns_engine.commands[i].name,
               g_cns_engine.commands[i].help);
    }
    
    printf("\nExamples:\n");
    printf("  cns spin -n 1000        # Start cognitive processing\n");
    printf("  cns think \"pattern\"     # Execute reasoning\n");
    printf("  cns learn data.txt      # Learn from data\n");
    
    return CNS_OK;
}

CNS_HANDLER(cmd_version_handler) {
    printf("CNS version 1.0.0 (7-tick)\n");
    printf("OpenTelemetry enabled\n");
    printf("Build: %s %s\n", __DATE__, __TIME__);
    return CNS_OK;
}

CNS_HANDLER(cmd_exit_handler) {
    // Clean shutdown
    exit(0);
    return CNS_OK;
}

// Forward to actual implementations
CNS_HANDLER(cmd_spin_handler) {
    // Create span for spin command
    auto span = g_cns_tracer->StartSpan("cmd_spin");
    span->SetAttribute("cns.argc", (int)cmd->argc);
    
    // Call actual implementation
    extern int cmd_spin(int argc, char** argv);
    char* argv[CNS_MAX_ARGS + 1];
    argv[0] = (char*)cmd->cmd;
    for (int i = 0; i < cmd->argc; i++) {
        argv[i + 1] = (char*)cmd->args[i];
    }
    
    int result = cmd_spin(cmd->argc + 1, argv);
    
    span->SetAttribute("cns.result", result);
    span->End();
    
    return result == 0 ? CNS_OK : CNS_ERR_INTERNAL;
}

CNS_HANDLER(cmd_think_handler) {
    auto span = g_cns_tracer->StartSpan("cmd_think");
    span->SetAttribute("cns.argc", (int)cmd->argc);
    
    extern int cmd_think(int argc, char** argv);
    char* argv[CNS_MAX_ARGS + 1];
    argv[0] = (char*)cmd->cmd;
    for (int i = 0; i < cmd->argc; i++) {
        argv[i + 1] = (char*)cmd->args[i];
    }
    
    int result = cmd_think(cmd->argc + 1, argv);
    
    span->SetAttribute("cns.result", result);
    span->End();
    
    return result == 0 ? CNS_OK : CNS_ERR_INTERNAL;
}

CNS_HANDLER(cmd_reflect_handler) {
    auto span = g_cns_tracer->StartSpan("cmd_reflect");
    span->SetAttribute("cns.argc", (int)cmd->argc);
    
    extern int cmd_reflect(int argc, char** argv);
    char* argv[CNS_MAX_ARGS + 1];
    argv[0] = (char*)cmd->cmd;
    for (int i = 0; i < cmd->argc; i++) {
        argv[i + 1] = (char*)cmd->args[i];
    }
    
    int result = cmd_reflect(cmd->argc + 1, argv);
    
    span->SetAttribute("cns.result", result);
    span->End();
    
    return result == 0 ? CNS_OK : CNS_ERR_INTERNAL;
}

CNS_HANDLER(cmd_learn_handler) {
    auto span = g_cns_tracer->StartSpan("cmd_learn");
    span->SetAttribute("cns.argc", (int)cmd->argc);
    
    extern int cmd_learn(int argc, char** argv);
    char* argv[CNS_MAX_ARGS + 1];
    argv[0] = (char*)cmd->cmd;
    for (int i = 0; i < cmd->argc; i++) {
        argv[i + 1] = (char*)cmd->args[i];
    }
    
    int result = cmd_learn(cmd->argc + 1, argv);
    
    span->SetAttribute("cns.result", result);
    span->End();
    
    return result == 0 ? CNS_OK : CNS_ERR_INTERNAL;
}

CNS_HANDLER(cmd_adapt_handler) {
    auto span = g_cns_tracer->StartSpan("cmd_adapt");
    span->SetAttribute("cns.argc", (int)cmd->argc);
    
    extern int cmd_adapt(int argc, char** argv);
    char* argv[CNS_MAX_ARGS + 1];
    argv[0] = (char*)cmd->cmd;
    for (int i = 0; i < cmd->argc; i++) {
        argv[i + 1] = (char*)cmd->args[i];
    }
    
    int result = cmd_adapt(cmd->argc + 1, argv);
    
    span->SetAttribute("cns.result", result);
    span->End();
    
    return result == 0 ? CNS_OK : CNS_ERR_INTERNAL;
}

CNS_HANDLER(cmd_benchmark_handler) {
    auto span = g_cns_tracer->StartSpan("cmd_benchmark");
    span->SetAttribute("cns.argc", (int)cmd->argc);
    
    extern int cmd_benchmark(int argc, char** argv);
    char* argv[CNS_MAX_ARGS + 1];
    argv[0] = (char*)cmd->cmd;
    for (int i = 0; i < cmd->argc; i++) {
        argv[i + 1] = (char*)cmd->args[i];
    }
    
    int result = cmd_benchmark(cmd->argc + 1, argv);
    
    span->SetAttribute("cns.result", result);
    span->End();
    
    return result == 0 ? CNS_OK : CNS_ERR_INTERNAL;
}

/*═══════════════════════════════════════════════════════════════
  Main Entry Point
  ═══════════════════════════════════════════════════════════════*/

int main(int argc, char** argv) {
    // Initialize telemetry
    cns_init_telemetry();
    
    // Create main span
    auto main_span = g_cns_tracer->StartSpan("cns_main");
    auto scope = trace::Scope(main_span);
    
    main_span->SetAttribute("cns.argc", argc);
    
    // Register commands
    cns_register_commands();
    
    // Handle command line arguments
    if (argc < 2) {
        cmd_help_handler(NULL, NULL);
        main_span->SetStatus(trace::StatusCode::kOk, "Displayed help");
        main_span->End();
        return 0;
    }
    
    // Check for special flags
    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        cmd_help_handler(NULL, NULL);
        main_span->SetStatus(trace::StatusCode::kOk, "Displayed help");
        main_span->End();
        return 0;
    }
    
    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-v") == 0) {
        cmd_version_handler(NULL, NULL);
        main_span->SetStatus(trace::StatusCode::kOk, "Displayed version");
        main_span->End();
        return 0;
    }
    
    // Build command string from arguments
    char command_line[1024] = {0};
    size_t offset = 0;
    
    for (int i = 1; i < argc && offset < sizeof(command_line) - 1; i++) {
        size_t len = strlen(argv[i]);
        
        // Add quotes if argument contains spaces
        bool needs_quotes = strchr(argv[i], ' ') != NULL;
        
        if (needs_quotes) {
            command_line[offset++] = '"';
        }
        
        size_t copy_len = (offset + len < sizeof(command_line) - 2) ? 
                          len : sizeof(command_line) - offset - 2;
        memcpy(command_line + offset, argv[i], copy_len);
        offset += copy_len;
        
        if (needs_quotes && offset < sizeof(command_line) - 1) {
            command_line[offset++] = '"';
        }
        
        if (i < argc - 1 && offset < sizeof(command_line) - 1) {
            command_line[offset++] = ' ';
        }
    }
    
    // Execute command
    cns_result_t result = cns_execute_with_telemetry(command_line, "cli");
    
    // Map result to exit code
    int exit_code = 0;
    switch (result) {
        case CNS_OK:
            exit_code = 0;
            break;
        case CNS_ERR_INVALID_CMD:
            fprintf(stderr, "Error: Invalid command\n");
            exit_code = 1;
            break;
        case CNS_ERR_INVALID_ARG:
            fprintf(stderr, "Error: Invalid arguments\n");
            exit_code = 2;
            break;
        case CNS_ERR_PERMISSION:
            fprintf(stderr, "Error: Permission denied\n");
            exit_code = 3;
            break;
        case CNS_ERR_TIMEOUT:
            fprintf(stderr, "Error: Command timeout\n");
            exit_code = 4;
            break;
        case CNS_ERR_RESOURCE:
            fprintf(stderr, "Error: Resource exhausted\n");
            exit_code = 5;
            break;
        case CNS_ERR_INTERNAL:
        default:
            fprintf(stderr, "Error: Internal error\n");
            exit_code = 127;
            break;
    }
    
    main_span->SetAttribute("cns.exit_code", exit_code);
    main_span->SetStatus(exit_code == 0 ? trace::StatusCode::kOk : trace::StatusCode::kError);
    main_span->End();
    
    return exit_code;
}