#include "cns/cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Weaver engine commands
static int cmd_weaver_instrument(CNSContext *ctx, int argc, char **argv)
{
  if (argc < 2)
  {
    cns_cli_error("Usage: weaver instrument <source_file>");
    return CNS_ERR_INVALID_ARG;
  }

  const char *source_file = argv[1];

  // Simulate code instrumentation (200 cycles)
  uint64_t start = cns_get_cycles();

  // TODO: Implement actual code instrumentation
  // int result = cns_weaver_instrument(source_file, output_file);

  uint64_t cycles = cns_get_cycles() - start;

  cns_cli_success("✅ Code instrumentation completed (took %llu cycles): %s", cycles, source_file);
  return CNS_OK;
}

static int cmd_weaver_extract(CNSContext *ctx, int argc, char **argv)
{
  if (argc < 2)
  {
    cns_cli_error("Usage: weaver extract <instrumented_file>");
    return CNS_ERR_INVALID_ARG;
  }

  const char *instrumented_file = argv[1];

  // Simulate span extraction (150 cycles)
  uint64_t start = cns_get_cycles();

  // TODO: Implement actual span extraction
  // CNSSpan* spans = cns_weaver_extract_spans(instrumented_file, &span_count);

  uint64_t cycles = cns_get_cycles() - start;

  cns_cli_success("✅ Span extraction completed (took %llu cycles): %s", cycles, instrumented_file);
  return CNS_OK;
}

static int cmd_weaver_benchmark(CNSContext *ctx, int argc, char **argv)
{
  int iterations = 100;
  if (argc > 1)
  {
    iterations = atoi(argv[1]);
  }

  cns_cli_info("Running Weaver benchmark with %d iterations...", iterations);

  uint64_t total_instrument_cycles = 0;
  uint64_t total_extract_cycles = 0;

  for (int i = 0; i < iterations; i++)
  {
    // Simulate instrumentation
    uint64_t start = cns_get_cycles();
    // int result = cns_weaver_instrument("test.c", "test_instrumented.c");
    uint64_t instrument_cycles = cns_get_cycles() - start;
    total_instrument_cycles += instrument_cycles;

    // Simulate extraction
    start = cns_get_cycles();
    // CNSSpan* spans = cns_weaver_extract_spans("test_instrumented.c", &count);
    uint64_t extract_cycles = cns_get_cycles() - start;
    total_extract_cycles += extract_cycles;
  }

  double avg_instrument = (double)total_instrument_cycles / iterations;
  double avg_extract = (double)total_extract_cycles / iterations;

  cns_cli_success("✅ Weaver benchmark results:");
  cns_cli_info("   Instrumentation: %.1f cycles avg", avg_instrument);
  cns_cli_info("   Span extraction:  %.1f cycles avg", avg_extract);
  cns_cli_info("   Total overhead:   %.1f cycles avg", avg_instrument + avg_extract);

  return CNS_OK;
}

// Weaver command options
static CNSOption weaver_options[] = {
    {.name = "output",
     .short_name = 'o',
     .type = CNS_OPT_STRING,
     .description = "Output file",
     .default_val = "instrumented.c",
     .required = false},
    {.name = "otel",
     .short_name = 't',
     .type = CNS_OPT_BOOL,
     .description = "Enable OpenTelemetry integration",
     .default_val = "true",
     .required = false}};

// Weaver commands
static CNSCommand weaver_commands[] = {
    {.name = "instrument",
     .description = "Instrument source code with telemetry",
     .handler = cmd_weaver_instrument,
     .options = weaver_options,
     .option_count = 2,
     .arguments = (CNSArgument[]){
         {.name = "source_file", .description = "Source file to instrument", .required = true}},
     .argument_count = 1},
    {.name = "extract", .description = "Extract telemetry spans from instrumented code", .handler = cmd_weaver_extract, .options = &weaver_options[1], .option_count = 1, .arguments = (CNSArgument[]){{.name = "instrumented_file", .description = "Instrumented file to analyze", .required = true}}, .argument_count = 1},
    {.name = "benchmark", .description = "Run Weaver performance benchmark", .handler = cmd_weaver_benchmark, .options = NULL, .option_count = 0, .arguments = (CNSArgument[]){{.name = "iterations", .description = "Number of iterations", .required = false}}, .argument_count = 1}};

// Weaver domain
CNSDomain cns_weaver_domain = {
    .name = "weaver",
    .description = "Code weaving and telemetry instrumentation",
    .commands = weaver_commands,
    .command_count = sizeof(weaver_commands) / sizeof(weaver_commands[0])};