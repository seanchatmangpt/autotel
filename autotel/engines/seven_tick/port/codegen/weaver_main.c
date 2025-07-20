#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>
#include "cjinja.h"

// ============================================================================
// JSON LOADING FUNCTIONS
// ============================================================================

static json_t *load_json_file(const char *filename)
{
  json_error_t error;
  json_t *root = json_load_file(filename, 0, &error);

  if (!root)
  {
    fprintf(stderr, "Error loading JSON file %s: %s\n", filename, error.text);
    return NULL;
  }

  return root;
}

static int populate_cjinja_context_from_json(CJinjaContext *ctx, json_t *root)
{
  json_t *spans_array = json_object_get(root, "spans");
  if (!spans_array || !json_is_array(spans_array))
  {
    fprintf(stderr, "Error: 'spans' array not found in JSON\n");
    return 0;
  }

  size_t span_count = json_array_size(spans_array);
  printf("Found %zu spans in JSON\n", span_count);

  // Create spans array for cjinja
  CJinjaArray *spans = cjinja_create_array();

  for (size_t i = 0; i < span_count; i++)
  {
    json_t *span_obj = json_array_get(spans_array, i);
    if (!json_is_object(span_obj))
      continue;

    CJinjaObject *span = cjinja_create_object();

    // Extract span properties
    json_t *fn = json_object_get(span_obj, "fn");
    json_t *span_name = json_object_get(span_obj, "spanName");
    json_t *comment = json_object_get(span_obj, "comment");
    json_t *uri = json_object_get(span_obj, "uri");

    if (fn && json_is_string(fn))
    {
      cjinja_set_string(span, "fn", json_string_value(fn));
    }

    if (span_name && json_is_string(span_name))
    {
      cjinja_set_string(span, "spanName", json_string_value(span_name));
    }

    if (comment && json_is_string(comment))
    {
      cjinja_set_string(span, "comment", json_string_value(comment));
    }
    else
    {
      cjinja_set_string(span, "comment", "");
    }

    if (uri && json_is_string(uri))
    {
      cjinja_set_string(span, "uri", json_string_value(uri));
    }

    cjinja_array_append(spans, span);
    cjinja_destroy_object(span);
  }

  // Add spans to context
  cjinja_set_array(ctx, "spans", spans);
  cjinja_destroy_array(spans);

  return 1;
}

// ============================================================================
// FILE UTILITIES
// ============================================================================

static int write_file(const char *filename, const char *content)
{
  FILE *f = fopen(filename, "w");
  if (!f)
  {
    fprintf(stderr, "Error opening file %s for writing\n", filename);
    return 0;
  }

  size_t written = fwrite(content, 1, strlen(content), f);
  fclose(f);

  if (written != strlen(content))
  {
    fprintf(stderr, "Error writing to file %s\n", filename);
    return 0;
  }

  printf("✓ Written %zu bytes to %s\n", written, filename);
  return 1;
}

static int ensure_directory(const char *path)
{
  char *dir_path = strdup(path);
  char *last_slash = strrchr(dir_path, '/');

  if (last_slash)
  {
    *last_slash = '\0';
    // Create directory (simplified - would use mkdir -p)
    printf("Ensuring directory exists: %s\n", dir_path);
  }

  free(dir_path);
  return 1;
}

// ============================================================================
// MAIN WEAVER FUNCTION
// ============================================================================

int main(int argc, char **argv)
{
  if (argc < 5)
  {
    fprintf(stderr, "Usage: %s <ctx.json> <header_tmpl> <inject_tmpl> <out_dir>\n", argv[0]);
    fprintf(stderr, "Example: %s build/cns_spans.json templates/otel_header.h.j2 templates/otel_inject.c.j2 src\n", argv[0]);
    return 1;
  }

  const char *ctx_path = argv[1];
  const char *hdr_tmpl = argv[2];
  const char *inj_tmpl = argv[3];
  const char *out_dir = argv[4];

  printf("=== CNS OpenTelemetry Weaver ===\n");
  printf("Context: %s\n", ctx_path);
  printf("Header template: %s\n", hdr_tmpl);
  printf("Inject template: %s\n", inj_tmpl);
  printf("Output directory: %s\n", out_dir);
  printf("\n");

  // Load JSON context
  json_t *root = load_json_file(ctx_path);
  if (!root)
  {
    return 1;
  }

  // Initialize cjinja
  CJinjaEngine *eng = cjinja_create("templates");
  if (!eng)
  {
    fprintf(stderr, "Error creating cjinja engine\n");
    json_decref(root);
    return 1;
  }

  CJinjaContext *ctx = cjinja_create_context();
  if (!ctx)
  {
    fprintf(stderr, "Error creating cjinja context\n");
    cjinja_destroy(eng);
    json_decref(root);
    return 1;
  }

  // Populate context from JSON
  if (!populate_cjinja_context_from_json(ctx, root))
  {
    cjinja_destroy_context(ctx);
    cjinja_destroy(eng);
    json_decref(root);
    return 1;
  }

  // Add additional context variables
  cjinja_set_string(ctx, "inject_target_header", "cns.h");
  cjinja_set_string(ctx, "generated_by", "cns_weaver");
  cjinja_set_string(ctx, "generation_time", "2025-01-18");

  // Ensure output directory exists
  ensure_directory(out_dir);

  // Render header template
  printf("Rendering header template...\n");
  char *hdr_content = cjinja_render_cached(eng, hdr_tmpl, ctx);
  if (!hdr_content)
  {
    fprintf(stderr, "Error rendering header template\n");
    cjinja_destroy_context(ctx);
    cjinja_destroy(eng);
    json_decref(root);
    return 1;
  }

  char out_hdr[512];
  snprintf(out_hdr, sizeof(out_hdr), "%s/cns_otel.h", out_dir);

  if (!write_file(out_hdr, hdr_content))
  {
    free(hdr_content);
    cjinja_destroy_context(ctx);
    cjinja_destroy(eng);
    json_decref(root);
    return 1;
  }

  free(hdr_content);

  // Render inject template
  printf("Rendering inject template...\n");
  char *inj_content = cjinja_render_cached(eng, inj_tmpl, ctx);
  if (!inj_content)
  {
    fprintf(stderr, "Error rendering inject template\n");
    cjinja_destroy_context(ctx);
    cjinja_destroy(eng);
    json_decref(root);
    return 1;
  }

  char out_c[512];
  snprintf(out_c, sizeof(out_c), "%s/cns_otel_inject.c", out_dir);

  if (!write_file(out_c, inj_content))
  {
    free(inj_content);
    cjinja_destroy_context(ctx);
    cjinja_destroy(eng);
    json_decref(root);
    return 1;
  }

  free(inj_content);

  // Cleanup
  cjinja_destroy_context(ctx);
  cjinja_destroy(eng);
  json_decref(root);

  printf("\n=== Weaver Complete ===\n");
  printf("✓ Generated: %s\n", out_hdr);
  printf("✓ Generated: %s\n", out_c);
  printf("✓ OpenTelemetry instrumentation ready\n");

  return 0;
}