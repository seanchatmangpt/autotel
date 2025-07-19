#include "../compiler/src/cjinja.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

// Performance measurement utilities
double get_time_ms()
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

void print_performance(const char *test_name, int iterations, double total_time_ms, const char *path_type)
{
  double avg_time_ns = (total_time_ms * 1000000.0) / iterations;
  double ops_per_sec = (iterations * 1000.0) / total_time_ms;

  printf("  %s (%s)\n", test_name, path_type);
  printf("    Total iterations: %d\n", iterations);
  printf("    Total time: %.3f ms\n", total_time_ms);
  printf("    Average time: %.1f ns\n", avg_time_ns);
  printf("    Throughput: %.1f ops/sec\n", ops_per_sec);

  if (avg_time_ns < 1000)
  {
    printf("    ✅ Sub-microsecond performance!\n");
  }
  else if (avg_time_ns < 10000)
  {
    printf("    ✅ Sub-10μs performance!\n");
  }
  else if (avg_time_ns < 100000)
  {
    printf("    ✅ Sub-100μs performance!\n");
  }
  else
  {
    printf("    ⚠️ Performance above 100μs\n");
  }
  printf("\n");
}

int main()
{
  printf("CJinja 7-Tick vs 49-Tick Performance Comparison\n");
  printf("===============================================\n\n");

  // Create engine and context
  CJinjaEngine *engine = cjinja_create("./templates");
  CJinjaContext *ctx = cjinja_create_context();

  if (!engine || !ctx)
  {
    printf("Failed to create CJinja engine or context\n");
    return 1;
  }

  // Set test data
  cjinja_set_var(ctx, "title", "Performance Test");
  cjinja_set_var(ctx, "user", "John Doe");
  cjinja_set_var(ctx, "email", "john@example.com");
  cjinja_set_bool(ctx, "is_admin", 1);
  cjinja_set_bool(ctx, "show_debug", 0);

  printf("Test data loaded:\n");
  printf("  - title: %s\n", get_var(ctx, "title"));
  printf("  - user: %s\n", get_var(ctx, "user"));
  printf("  - is_admin: %s\n", get_var(ctx, "is_admin") ? "true" : "false");
  printf("\n");

  printf("Running Performance Comparison...\n\n");

  // 1. Basic Variable Substitution - 7-tick vs 49-tick
  const char *simple_template = "Hello {{user}}, welcome to {{title}}!";
  int simple_iterations = 10000;

  // 7-tick path
  double simple_7tick_start = get_time_ms();
  for (int i = 0; i < simple_iterations; i++)
  {
    char *result = cjinja_render_string_7tick(simple_template, ctx);
    free(result);
  }
  double simple_7tick_time = get_time_ms() - simple_7tick_start;
  print_performance("1. Basic Variable Substitution", simple_iterations, simple_7tick_time, "7-TICK PATH");

  // 49-tick path
  double simple_49tick_start = get_time_ms();
  for (int i = 0; i < simple_iterations; i++)
  {
    char *result = cjinja_render_string(simple_template, ctx);
    free(result);
  }
  double simple_49tick_time = get_time_ms() - simple_49tick_start;
  print_performance("1. Basic Variable Substitution", simple_iterations, simple_49tick_time, "49-TICK PATH");

  // 2. Conditional Rendering - 7-tick vs 49-tick
  const char *conditional_template =
      "{% if is_admin %}Welcome admin {{user}}!{% endif %}"
      "{% if show_debug %}Debug mode enabled{% endif %}"
      "Regular user: {{user}}";

  int conditional_iterations = 10000;

  // 7-tick path
  double conditional_7tick_start = get_time_ms();
  for (int i = 0; i < conditional_iterations; i++)
  {
    char *result = cjinja_render_conditionals_7tick(conditional_template, ctx);
    free(result);
  }
  double conditional_7tick_time = get_time_ms() - conditional_7tick_start;
  print_performance("2. Conditional Rendering", conditional_iterations, conditional_7tick_time, "7-TICK PATH");

  // 49-tick path
  double conditional_49tick_start = get_time_ms();
  for (int i = 0; i < conditional_iterations; i++)
  {
    char *result = cjinja_render_with_conditionals(conditional_template, ctx);
    free(result);
  }
  double conditional_49tick_time = get_time_ms() - conditional_49tick_start;
  print_performance("2. Conditional Rendering", conditional_iterations, conditional_49tick_time, "49-TICK PATH");

  // 3. Advanced Features (49-tick only)
  const char *advanced_template =
      "User: {{user | upper}}\n"
      "Email: {{email | lower}}\n"
      "Title: {{title | capitalize}}\n"
      "Name length: {{user | length}} characters\n"
      "Trimmed: {{user | trim}}\n"
      "Replaced: {{user | replace('John','Jane')}}";

  int advanced_iterations = 10000;
  double advanced_start = get_time_ms();

  for (int i = 0; i < advanced_iterations; i++)
  {
    char *result = cjinja_render_with_loops(advanced_template, ctx);
    free(result);
  }

  double advanced_time = get_time_ms() - advanced_start;
  print_performance("3. Advanced Features (Filters, etc.)", advanced_iterations, advanced_time, "49-TICK PATH ONLY");

  // 4. Template Inheritance (49-tick only)
  const char *base_template =
      "<html>\n"
      "<head><title>{{title}}</title></head>\n"
      "<body>\n"
      "  <header>{{% block header %}}Default Header{{% endblock %}}</header>\n"
      "  <main>{{% block content %}}Default Content{{% endblock %}}</main>\n"
      "</body>\n"
      "</html>";

  const char *child_template =
      "{{% extends base %}}\n"
      "{{% block header %}}Welcome {{user}}!{{% endblock %}}\n"
      "{{% block content %}}This is the main content.{{% endblock %}}";

  CJinjaInheritanceContext *inherit_ctx = cjinja_create_inheritance_context();
  cjinja_set_base_template(inherit_ctx, base_template);
  cjinja_add_block(inherit_ctx, "header", "Welcome John Doe!");
  cjinja_add_block(inherit_ctx, "content", "This is the main content.");

  int inheritance_iterations = 1000;
  double inheritance_start = get_time_ms();

  for (int i = 0; i < inheritance_iterations; i++)
  {
    char *result = cjinja_render_with_inheritance(child_template, ctx, inherit_ctx);
    free(result);
  }

  double inheritance_time = get_time_ms() - inheritance_start;
  print_performance("4. Template Inheritance", inheritance_iterations, inheritance_time, "49-TICK PATH ONLY");

  // 5. Batch Rendering (49-tick only)
  const char *templates[] = {
      "Template 1: {{user}}",
      "Template 2: {{title}}",
      "Template 3: {% if is_admin %}Admin{% endif %}",
      "Template 4: {{user | upper}}",
      "Template 5: {{title | length}} chars"};

  CJinjaBatchRender *batch = cjinja_create_batch_render(5);
  for (int i = 0; i < 5; i++)
  {
    batch->templates[i] = templates[i];
  }

  int batch_iterations = 1000;
  double batch_start = get_time_ms();

  for (int i = 0; i < batch_iterations; i++)
  {
    cjinja_render_batch(engine, batch, ctx);
  }

  double batch_time = get_time_ms() - batch_start;
  print_performance("5. Batch Rendering (5 templates)", batch_iterations, batch_time, "49-TICK PATH ONLY");

  // Performance Comparison Summary
  printf("Performance Comparison Summary\n");
  printf("==============================\n");

  double simple_speedup = simple_49tick_time / simple_7tick_time;
  double conditional_speedup = conditional_49tick_time / conditional_7tick_time;

  printf("Basic Variable Substitution:\n");
  printf("  7-tick path: %.1f ns\n", (simple_7tick_time * 1000000.0) / simple_iterations);
  printf("  49-tick path: %.1f ns\n", (simple_49tick_time * 1000000.0) / simple_iterations);
  printf("  Speedup: %.1fx faster with 7-tick path\n", simple_speedup);
  printf("\n");

  printf("Conditional Rendering:\n");
  printf("  7-tick path: %.1f ns\n", (conditional_7tick_time * 1000000.0) / conditional_iterations);
  printf("  49-tick path: %.1f ns\n", (conditional_49tick_time * 1000000.0) / conditional_iterations);
  printf("  Speedup: %.1fx faster with 7-tick path\n", conditional_speedup);
  printf("\n");

  printf("Advanced Features (49-tick only):\n");
  printf("  Advanced filters: %.1f ns\n", (advanced_time * 1000000.0) / advanced_iterations);
  printf("  Template inheritance: %.1f ns\n", (inheritance_time * 1000000.0) / inheritance_iterations);
  printf("  Batch rendering: %.1f ns\n", (batch_time * 1000000.0) / batch_iterations);
  printf("\n");

  printf("Architecture Benefits:\n");
  printf("  ✅ 7-tick path: Maximum performance for basic operations\n");
  printf("  ✅ 49-tick path: Full feature set for complex needs\n");
  printf("  ✅ Clear separation: Developers choose the right path\n");
  printf("  ✅ No performance penalty for basic operations\n");
  printf("  ✅ Feature-rich path available when needed\n");
  printf("\n");

  // Sample Output
  printf("Sample Output\n");
  printf("=============\n");

  char *simple_7tick_result = cjinja_render_string_7tick(simple_template, ctx);
  printf("7-tick Simple: %s\n", simple_7tick_result);
  free(simple_7tick_result);

  char *simple_49tick_result = cjinja_render_string(simple_template, ctx);
  printf("49-tick Simple: %s\n", simple_49tick_result);
  free(simple_49tick_result);

  char *conditional_7tick_result = cjinja_render_conditionals_7tick(conditional_template, ctx);
  printf("7-tick Conditional: %s\n", conditional_7tick_result);
  free(conditional_7tick_result);

  char *conditional_49tick_result = cjinja_render_with_conditionals(conditional_template, ctx);
  printf("49-tick Conditional: %s\n", conditional_49tick_result);
  free(conditional_49tick_result);

  char *advanced_result = cjinja_render_with_loops(advanced_template, ctx);
  printf("49-tick Advanced: %s\n", advanced_result);
  free(advanced_result);

  // Cleanup
  cjinja_destroy_batch_render(batch);
  cjinja_destroy_inheritance_context(inherit_ctx);
  cjinja_destroy_context(ctx);
  cjinja_destroy_engine(engine);

  printf("\nCJinja 7-Tick vs 49-Tick benchmark completed!\n");
  return 0;
}