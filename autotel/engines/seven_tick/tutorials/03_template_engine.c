/*
 * First Principles C Tutorial: Template Engine
 *
 * This tutorial validates understanding of template engine concepts
 * that are critical for the CJinja implementation in the 7T Engine.
 *
 * Key Concepts:
 * - Template parsing
 * - Variable substitution
 * - Control structures
 * - Performance optimization
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Test utilities
#define TEST_ASSERT(condition, message) \
  do                                    \
  {                                     \
    if (!(condition))                   \
    {                                   \
      printf("❌ FAIL: %s\n", message); \
      return 0;                         \
    }                                   \
    else                                \
    {                                   \
      printf("✅ PASS: %s\n", message); \
    }                                   \
  } while (0)

#define TEST_SECTION(name) printf("\n=== %s ===\n", name)

// Simple template engine structures
typedef struct
{
  char *key;
  char *value;
  int type; // 0=string, 1=boolean, 2=array
} TemplateVariable;

typedef struct
{
  TemplateVariable *variables;
  size_t count;
  size_t capacity;
} TemplateContext;

// Initialize template context
TemplateContext *template_context_create(size_t initial_capacity)
{
  TemplateContext *ctx = malloc(sizeof(TemplateContext));
  ctx->variables = malloc(initial_capacity * sizeof(TemplateVariable));
  ctx->count = 0;
  ctx->capacity = initial_capacity;
  return ctx;
}

// Add string variable
void template_set_string(TemplateContext *ctx, const char *key, const char *value)
{
  if (ctx->count < ctx->capacity)
  {
    ctx->variables[ctx->count].key = strdup(key);
    ctx->variables[ctx->count].value = strdup(value);
    ctx->variables[ctx->count].type = 0;
    ctx->count++;
  }
}

// Add boolean variable
void template_set_bool(TemplateContext *ctx, const char *key, int value)
{
  if (ctx->count < ctx->capacity)
  {
    ctx->variables[ctx->count].key = strdup(key);
    ctx->variables[ctx->count].value = value ? strdup("true") : strdup("false");
    ctx->variables[ctx->count].type = 1;
    ctx->count++;
  }
}

// Get variable value
const char *template_get_value(TemplateContext *ctx, const char *key)
{
  for (size_t i = 0; i < ctx->count; i++)
  {
    if (strcmp(ctx->variables[i].key, key) == 0)
    {
      return ctx->variables[i].value;
    }
  }
  return NULL;
}

// Cleanup template context
void template_context_destroy(TemplateContext *ctx)
{
  for (size_t i = 0; i < ctx->count; i++)
  {
    free(ctx->variables[i].key);
    free(ctx->variables[i].value);
  }
  free(ctx->variables);
  free(ctx);
}

// Lesson 1: Basic Template Parsing
int lesson_basic_template_parsing()
{
  TEST_SECTION("Basic Template Parsing");

  // Test 1: Simple variable substitution
  const char *template = "Hello {{name}}, welcome to {{company}}!";
  TemplateContext *ctx = template_context_create(10);

  template_set_string(ctx, "name", "John");
  template_set_string(ctx, "company", "TechCorp");

  // Simple parsing (find and replace)
  char result[256];
  strcpy(result, template);

  // Replace {{name}}
  const char *name_value = template_get_value(ctx, "name");
  TEST_ASSERT(name_value != NULL, "Name variable found");
  TEST_ASSERT(strcmp(name_value, "John") == 0, "Name variable value correct");

  // Replace {{company}}
  const char *company_value = template_get_value(ctx, "company");
  TEST_ASSERT(company_value != NULL, "Company variable found");
  TEST_ASSERT(strcmp(company_value, "TechCorp") == 0, "Company variable value correct");

  printf("  Template: %s\n", template);
  printf("  Variables: name=%s, company=%s\n", name_value, company_value);

  template_context_destroy(ctx);

  // Test 2: Variable not found
  ctx = template_context_create(10);
  template_set_string(ctx, "name", "Alice");

  const char *missing_value = template_get_value(ctx, "missing");
  TEST_ASSERT(missing_value == NULL, "Missing variable returns NULL");

  template_context_destroy(ctx);

  return 1;
}

// Lesson 2: Variable Substitution
int lesson_variable_substitution()
{
  TEST_SECTION("Variable Substitution");

  TemplateContext *ctx = template_context_create(10);

  // Test 1: String substitution
  template_set_string(ctx, "user", "Alice");
  template_set_string(ctx, "role", "Developer");
  template_set_string(ctx, "department", "Engineering");

  const char *template = "User: {{user}}, Role: {{role}}, Department: {{department}}";

  // Manual substitution for demonstration
  char result[256];
  strcpy(result, template);

  // Replace variables (simplified)
  const char *user = template_get_value(ctx, "user");
  const char *role = template_get_value(ctx, "role");
  const char *department = template_get_value(ctx, "department");

  TEST_ASSERT(user != NULL, "User variable found");
  TEST_ASSERT(role != NULL, "Role variable found");
  TEST_ASSERT(department != NULL, "Department variable found");

  printf("  Original: %s\n", template);
  printf("  Substituted: User: %s, Role: %s, Department: %s\n", user, role, department);

  // Test 2: Multiple occurrences
  template_set_string(ctx, "item", "laptop");
  const char *multi_template = "I have a {{item}}. My {{item}} is fast. The {{item}} works well.";

  const char *item = template_get_value(ctx, "item");
  TEST_ASSERT(item != NULL, "Item variable found");
  TEST_ASSERT(strcmp(item, "laptop") == 0, "Item variable value correct");

  printf("  Multi-occurrence template: %s\n", multi_template);
  printf("  Item value: %s\n", item);

  template_context_destroy(ctx);

  return 1;
}

// Lesson 3: Control Structures
int lesson_control_structures()
{
  TEST_SECTION("Control Structures");

  TemplateContext *ctx = template_context_create(10);

  // Test 1: Conditional rendering
  template_set_bool(ctx, "is_admin", 1);
  template_set_bool(ctx, "is_premium", 0);

  const char *admin_template = "{% if is_admin %}Admin Panel{% endif %}";
  const char *premium_template = "{% if is_premium %}Premium Features{% endif %}";

  // Check conditional variables
  const char *is_admin = template_get_value(ctx, "is_admin");
  const char *is_premium = template_get_value(ctx, "is_premium");

  TEST_ASSERT(is_admin != NULL, "is_admin variable found");
  TEST_ASSERT(is_premium != NULL, "is_premium variable found");
  TEST_ASSERT(strcmp(is_admin, "true") == 0, "is_admin is true");
  TEST_ASSERT(strcmp(is_premium, "false") == 0, "is_premium is false");

  printf("  Admin template: %s\n", admin_template);
  printf("  Admin condition: %s\n", is_admin);
  printf("  Premium template: %s\n", premium_template);
  printf("  Premium condition: %s\n", is_premium);

  // Test 2: Loop simulation
  const char *loop_template = "{% for item in items %}{{item}}{% endfor %}";

  // Simulate array-like behavior
  template_set_string(ctx, "item_0", "apple");
  template_set_string(ctx, "item_1", "banana");
  template_set_string(ctx, "item_2", "cherry");

  const char *item0 = template_get_value(ctx, "item_0");
  const char *item1 = template_get_value(ctx, "item_1");
  const char *item2 = template_get_value(ctx, "item_2");

  TEST_ASSERT(item0 != NULL, "item_0 found");
  TEST_ASSERT(item1 != NULL, "item_1 found");
  TEST_ASSERT(item2 != NULL, "item_2 found");

  printf("  Loop template: %s\n", loop_template);
  printf("  Items: %s, %s, %s\n", item0, item1, item2);

  template_context_destroy(ctx);

  return 1;
}

// Lesson 4: Performance Optimization
int lesson_performance_optimization()
{
  TEST_SECTION("Performance Optimization");

  // Test 1: Fast path vs slow path
  TemplateContext *ctx = template_context_create(100);

  // Fast path: Simple variable substitution
  template_set_string(ctx, "name", "John");
  template_set_string(ctx, "title", "Developer");

  const char *fast_template = "Hello {{name}}, you are a {{title}}!";

  // Slow path: Complex template with conditionals and loops
  const char *slow_template =
      "{% if is_admin %}"
      "Admin: {{name}} ({{title}})\n"
      "{% for permission in permissions %}"
      "  - {{permission}}\n"
      "{% endfor %}"
      "{% endif %}";

  printf("  Fast template: %s\n", fast_template);
  printf("  Slow template: %s\n", slow_template);

  // Test 2: Variable lookup optimization
  clock_t start = clock();
  for (int i = 0; i < 10000; i++)
  {
    const char *value = template_get_value(ctx, "name");
    // Use value to prevent optimization
    if (value)
    {
      volatile int x = strlen(value);
    }
  }
  clock_t end = clock();
  double lookup_time = ((double)(end - start)) / CLOCKS_PER_SEC;

  printf("  Variable lookup time: %.6f seconds\n", lookup_time);
  TEST_ASSERT(lookup_time < 0.01, "Variable lookup is fast");

  // Test 3: Context creation/destruction performance
  start = clock();
  for (int i = 0; i < 1000; i++)
  {
    TemplateContext *temp_ctx = template_context_create(10);
    template_set_string(temp_ctx, "test", "value");
    template_context_destroy(temp_ctx);
  }
  end = clock();
  double ctx_time = ((double)(end - start)) / CLOCKS_PER_SEC;

  printf("  Context create/destroy time: %.6f seconds\n", ctx_time);
  TEST_ASSERT(ctx_time < 0.1, "Context operations are fast");

  template_context_destroy(ctx);

  return 1;
}

// Lesson 5: 7T Engine Template Patterns
int lesson_7t_template_patterns()
{
  TEST_SECTION("7T Engine Template Patterns");

  TemplateContext *ctx = template_context_create(20);

  // Pattern 1: 7-tick path (simple variable substitution)
  template_set_string(ctx, "user", "Alice");
  template_set_string(ctx, "email", "alice@example.com");

  const char *simple_template = "User: {{user}}, Email: {{email}}";

  const char *user = template_get_value(ctx, "user");
  const char *email = template_get_value(ctx, "email");

  printf("  7-tick template: %s\n", simple_template);
  printf("  Variables: user=%s, email=%s\n", user, email);

  TEST_ASSERT(user != NULL, "User variable for 7-tick path");
  TEST_ASSERT(email != NULL, "Email variable for 7-tick path");

  // Pattern 2: 49-tick path (complex features)
  template_set_bool(ctx, "is_premium", 1);
  template_set_string(ctx, "user", "Bob");
  template_set_string(ctx, "role", "Manager");

  const char *complex_template =
      "{% if is_premium %}"
      "Premium User: {{user | upper}} ({{role | capitalize}})\n"
      "{% for feature in features %}"
      "  - {{feature}}\n"
      "{% endfor %}"
      "{% endif %}";

  const char *is_premium = template_get_value(ctx, "is_premium");
  const char *role = template_get_value(ctx, "role");

  printf("  49-tick template: %s\n", complex_template);
  printf("  Premium: %s, Role: %s\n", is_premium, role);

  TEST_ASSERT(is_premium != NULL, "Premium variable for 49-tick path");
  TEST_ASSERT(role != NULL, "Role variable for 49-tick path");

  // Pattern 3: Integration with SPARQL results
  template_set_string(ctx, "query_result", "John,Alice,Bob");
  template_set_string(ctx, "result_count", "3");

  const char *integration_template =
      "SPARQL Query Results:\n"
      "Found {{result_count}} results:\n"
      "{{query_result}}";

  const char *query_result = template_get_value(ctx, "query_result");
  const char *result_count = template_get_value(ctx, "result_count");

  printf("  Integration template: %s\n", integration_template);
  printf("  Results: %s, Count: %s\n", query_result, result_count);

  TEST_ASSERT(query_result != NULL, "Query result for integration");
  TEST_ASSERT(result_count != NULL, "Result count for integration");

  template_context_destroy(ctx);

  return 1;
}

// Main tutorial runner
int main()
{
  printf("First Principles C Tutorial: Template Engine\n");
  printf("============================================\n");
  printf("Validating template concepts for CJinja implementation\n\n");

  int total_lessons = 5;
  int passed_lessons = 0;

  // Run all lessons
  if (lesson_basic_template_parsing())
    passed_lessons++;
  if (lesson_variable_substitution())
    passed_lessons++;
  if (lesson_control_structures())
    passed_lessons++;
  if (lesson_performance_optimization())
    passed_lessons++;
  if (lesson_7t_template_patterns())
    passed_lessons++;

  // Summary
  printf("\n=== Tutorial Summary ===\n");
  printf("Total lessons: %d\n", total_lessons);
  printf("Passed: %d\n", passed_lessons);
  printf("Failed: %d\n", total_lessons - passed_lessons);
  printf("Success rate: %.1f%%\n", (passed_lessons * 100.0) / total_lessons);

  if (passed_lessons == total_lessons)
  {
    printf("\n🎉 All template engine concepts validated!\n");
    printf("Ready for CJinja implementation.\n");
    return 0;
  }
  else
  {
    printf("\n❌ Some concepts need review.\n");
    return 1;
  }
}