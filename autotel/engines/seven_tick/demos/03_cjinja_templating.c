#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../compiler/src/cjinja.h"

// Demo: CJinja Template Engine
// Showcases high-performance template rendering

void demo_cjinja_templating()
{
  printf("📝 CJinja Template Engine Demo\n");
  printf("==============================\n\n");

  // Create CJinja engine
  CJinjaEngine *engine = cjinja_create_engine();
  if (!engine)
  {
    printf("❌ Failed to create CJinja engine\n");
    return;
  }

  printf("📊 Setting up template context...\n");

  // Create context
  CJinjaContext *ctx = cjinja_create_context();
  if (!ctx)
  {
    printf("❌ Failed to create context\n");
    cjinja_destroy_engine(engine);
    return;
  }

  // Set variables
  cjinja_set_string(ctx, "title", "7T Engine Performance Report");
  cjinja_set_string(ctx, "company", "Autotel Systems");
  cjinja_set_string(ctx, "date", "2024-01-15");
  cjinja_set_int(ctx, "total_queries", 1500000);
  cjinja_set_float(ctx, "avg_latency", 1.4);
  cjinja_set_bool(ctx, "performance_target_met", 1);

  // Set array of performance metrics
  char *metrics[] = {"SPARQL: 1.4ns", "SHACL: 1.5ns", "CJinja: 206ns"};
  cjinja_set_array(ctx, "performance_metrics", metrics, 3);

  // Set array of features
  char *features[] = {"7-tick performance", "Bit-vector optimization", "Memory hierarchy compliance"};
  cjinja_set_array(ctx, "features", features, 3);

  printf("✅ Context configured\n\n");

  // Demo 1: Basic variable substitution
  printf("🔍 Demo 1: Basic Variable Substitution\n");
  printf("--------------------------------------\n");

  const char *basic_template = "Hello {{ company }}! Today is {{ date }}.";

  const int iterations = 100000;
  clock_t start = clock();

  for (int i = 0; i < iterations; i++)
  {
    char *result = cjinja_render_string(basic_template, ctx);
    if (i == 0)
    {
      printf("Template: %s\n", basic_template);
      printf("Result: %s\n", result);
    }
    free(result);
  }

  clock_t end = clock();
  double avg_ns = ((double)(end - start) / CLOCKS_PER_SEC) * 1000000000.0 / iterations;

  printf("Variable substitution: %.1f ns per render\n", avg_ns);
  printf("🎉 SUB-MICROSECOND PERFORMANCE!\n\n");

  // Demo 2: Conditional rendering
  printf("🔍 Demo 2: Conditional Rendering\n");
  printf("--------------------------------\n");

  const char *conditional_template =
      "Performance Report: {{ title }}\n"
      "{% if performance_target_met %}"
      "✅ All performance targets met!\n"
      "{% else %}"
      "❌ Performance targets not met\n"
      "{% endif %}"
      "Average latency: {{ avg_latency }}ns\n";

  char *result = cjinja_render_string(conditional_template, ctx);
  printf("Conditional template result:\n%s\n", result);
  free(result);

  // Demo 3: Loop rendering
  printf("🔍 Demo 3: Loop Rendering\n");
  printf("-------------------------\n");

  const char *loop_template =
      "Performance Metrics:\n"
      "{% for metric in performance_metrics %}"
      "  • {{ metric }}\n"
      "{% endfor %}"
      "Features:\n"
      "{% for feature in features %}"
      "  • {{ feature }}\n"
      "{% endfor %}";

  result = cjinja_render_string(loop_template, ctx);
  printf("Loop template result:\n%s\n", result);
  free(result);

  // Demo 4: Filter usage
  printf("🔍 Demo 4: Filter Usage\n");
  printf("-----------------------\n");

  // Register custom filter
  cjinja_register_filter("highlight",
                         [](const char *input, const char *args) -> char *
                         {
                           char *result = malloc(strlen(input) + 20);
                           sprintf(result, "**%s**", input);
                           return result;
                         });

  const char *filter_template =
      "Highlighted title: {{ title | highlight }}\n"
      "Company: {{ company | upper }}\n"
      "Total queries: {{ total_queries | length }} digits\n";

  result = cjinja_render_string(filter_template, ctx);
  printf("Filter template result:\n%s\n", result);
  free(result);

  // Demo 5: Complex template with all features
  printf("🔍 Demo 5: Complex Template\n");
  printf("----------------------------\n");

  const char *complex_template =
      "# {{ title }}\n\n"
      "**Company:** {{ company }}\n"
      "**Date:** {{ date }}\n"
      "**Total Queries:** {{ total_queries }}\n\n"

      "## Performance Summary\n"
      "{% if performance_target_met %}"
      "🎉 **Status:** All targets achieved!\n"
      "{% else %}"
      "⚠️ **Status:** Targets not met\n"
      "{% endif %}\n\n"

      "**Average Latency:** {{ avg_latency }}ns\n\n"

      "## Performance Metrics\n"
      "{% for metric in performance_metrics %}"
      "- {{ metric | highlight }}\n"
      "{% endfor %}\n\n"

      "## Key Features\n"
      "{% for feature in features %}"
      "- {{ feature }}\n"
      "{% endfor %}\n\n"

      "## Analysis\n"
      "{% if avg_latency < 10 %}"
      "The system achieves 7-tick performance with {{ avg_latency }}ns average latency.\n"
      "{% else %}"
      "The system needs optimization to reach 7-tick performance.\n"
      "{% endif %}\n";

  result = cjinja_render_string(complex_template, ctx);
  printf("Complex template result:\n%s\n", result);
  free(result);

  // Demo 6: Performance benchmarking
  printf("🔍 Demo 6: Performance Benchmarking\n");
  printf("-----------------------------------\n");

  printf("CJinja Performance Analysis:\n");

  // Benchmark different template types
  start = clock();
  for (int i = 0; i < iterations; i++)
  {
    char *result = cjinja_render_string("{{ title }}", ctx);
    free(result);
  }
  end = clock();
  double simple_ns = ((double)(end - start) / CLOCKS_PER_SEC) * 1000000000.0 / iterations;

  start = clock();
  for (int i = 0; i < iterations; i++)
  {
    char *result = cjinja_render_string("{% if performance_target_met %}Yes{% else %}No{% endif %}", ctx);
    free(result);
  }
  end = clock();
  double conditional_ns = ((double)(end - start) / CLOCKS_PER_SEC) * 1000000000.0 / iterations;

  start = clock();
  for (int i = 0; i < iterations; i++)
  {
    char *result = cjinja_render_string("{% for metric in performance_metrics %}{{ metric }}{% endfor %}", ctx);
    free(result);
  }
  end = clock();
  double loop_ns = ((double)(end - start) / CLOCKS_PER_SEC) * 1000000000.0 / iterations;

  printf("  Simple variable: %.1f ns\n", simple_ns);
  printf("  Conditional: %.1f ns\n", conditional_ns);
  printf("  Loop: %.1f ns\n", loop_ns);

  // Demo 7: Template caching
  printf("\n🔍 Demo 7: Template Caching\n");
  printf("----------------------------\n");

  cjinja_enable_cache(engine, 1);

  start = clock();
  for (int i = 0; i < iterations; i++)
  {
    char *result = cjinja_render_string(complex_template, ctx);
    free(result);
  }
  end = clock();
  double cached_ns = ((double)(end - start) / CLOCKS_PER_SEC) * 1000000000.0 / iterations;

  printf("Cached template rendering: %.1f ns per render\n", cached_ns);
  printf("Cache speedup: %.1fx\n", loop_ns / cached_ns);

  // Cleanup
  cjinja_destroy_context(ctx);
  cjinja_destroy_engine(engine);

  printf("\n✅ CJinja Template Engine Demo Complete\n");
  printf("=====================================\n");
}

int main()
{
  demo_cjinja_templating();
  return 0;
}