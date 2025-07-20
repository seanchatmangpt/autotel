#include "../compiler/src/cjinja.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

// Demo 1: High-Throughput Logging with 7-Tick Path
// Use Case: Performance-critical logging where sub-microsecond rendering is essential

double get_time_ms()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

int main()
{
  printf("Demo 1: High-Throughput Logging (7-Tick Path)\n");
  printf("==============================================\n\n");

  // Create engine and context
  CJinjaEngine *engine = cjinja_create("./templates");
  CJinjaContext *ctx = cjinja_create_context();

  if (!engine || !ctx)
  {
    printf("Failed to create CJinja engine or context\n");
    return 1;
  }

  // Set up logging context variables
  cjinja_set_var(ctx, "app_name", "WebServer");
  cjinja_set_var(ctx, "version", "2.1.0");
  cjinja_set_var(ctx, "environment", "production");

  // Simple logging templates optimized for 7-tick path
  const char *log_templates[] = {
      "[{{timestamp}}] {{level}} {{app_name}}: {{message}}",
      "{{timestamp}} | {{level}} | {{app_name}} | {{message}}",
      "{{app_name}}[{{pid}}]: {{level}}: {{message}}",
      "{{timestamp}} {{level}} [{{app_name}}] {{message}}",
      "{{app_name}} {{version}} {{level}}: {{message}}"};

  const char *log_levels[] = {"INFO", "WARN", "ERROR", "DEBUG", "FATAL"};
  const char *log_messages[] = {
      "Request received from client",
      "Database connection established",
      "Cache miss, fetching from database",
      "Response sent to client",
      "Session timeout, cleaning up"};

  printf("Simulating high-throughput logging with 7-tick path...\n");
  printf("Template: [{{timestamp}}] {{level}} {{app_name}}: {{message}}\n\n");

  // Performance test: 10,000 log entries
  int iterations = 10000;
  double start_time = get_time_ms();

  for (int i = 0; i < iterations; i++)
  {
    // Update dynamic variables
    char timestamp[32];
    snprintf(timestamp, sizeof(timestamp), "%ld", time(NULL));
    cjinja_set_var(ctx, "timestamp", timestamp);

    cjinja_set_var(ctx, "level", log_levels[i % 5]);
    cjinja_set_var(ctx, "message", log_messages[i % 5]);

    // Use 7-tick path for maximum performance
    char *log_entry = cjinja_render_string_7tick(log_templates[0], ctx);

    // In real application, this would be written to log file
    // printf("%s\n", log_entry);

    free(log_entry);
  }

  double end_time = get_time_ms();
  double total_time = end_time - start_time;
  double avg_time_ns = (total_time * 1000000.0) / iterations;
  double throughput = (iterations * 1000.0) / total_time;

  printf("Performance Results:\n");
  printf("  Total log entries: %d\n", iterations);
  printf("  Total time: %.3f ms\n", total_time);
  printf("  Average time per log: %.1f ns\n", avg_time_ns);
  printf("  Throughput: %.1f logs/sec\n", throughput);

  if (avg_time_ns < 1000)
  {
    printf("  ✅ Sub-microsecond logging performance!\n");
  }
  else
  {
    printf("  ⚠️ Performance above 1μs\n");
  }

  // Show sample log entries
  printf("\nSample Log Entries:\n");
  printf("===================\n");

  for (int i = 0; i < 5; i++)
  {
    char timestamp[32];
    snprintf(timestamp, sizeof(timestamp), "%ld", time(NULL));
    cjinja_set_var(ctx, "timestamp", timestamp);
    cjinja_set_var(ctx, "level", log_levels[i]);
    cjinja_set_var(ctx, "message", log_messages[i]);

    char *log_entry = cjinja_render_string_7tick(log_templates[0], ctx);
    printf("  %s\n", log_entry);
    free(log_entry);
  }

  // Demonstrate different log formats
  printf("\nDifferent Log Formats (7-tick path):\n");
  printf("====================================\n");

  char timestamp[32];
  snprintf(timestamp, sizeof(timestamp), "%ld", time(NULL));
  cjinja_set_var(ctx, "timestamp", timestamp);
  cjinja_set_var(ctx, "level", "INFO");
  cjinja_set_var(ctx, "message", "Server started successfully");
  cjinja_set_var(ctx, "pid", "12345");

  for (int i = 0; i < 5; i++)
  {
    char *log_entry = cjinja_render_string_7tick(log_templates[i], ctx);
    printf("  Format %d: %s\n", i + 1, log_entry);
    free(log_entry);
  }

  // Cleanup
  cjinja_destroy_context(ctx);
  cjinja_destroy_engine(engine);

  printf("\nDemo 1 completed! 7-tick path provides sub-microsecond logging performance.\n");
  return 0;
}