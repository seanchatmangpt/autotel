#include "../compiler/src/cjinja.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Demo 4: Configuration File Generation
// Use Case: Generating complex configuration files using template inheritance and includes

int main()
{
  printf("Demo 4: Configuration File Generation (49-Tick Path)\n");
  printf("===================================================\n\n");

  // Create engine and context
  CJinjaEngine *engine = cjinja_create("./templates");
  CJinjaContext *ctx = cjinja_create_context();

  if (!engine || !ctx)
  {
    printf("Failed to create CJinja engine or context\n");
    return 1;
  }

  // Set up configuration context
  cjinja_set_var(ctx, "app_name", "WebPortal");
  cjinja_set_var(ctx, "version", "2.1.0");
  cjinja_set_var(ctx, "environment", "production");
  cjinja_set_var(ctx, "host", "webportal.example.com");
  cjinja_set_var(ctx, "port", "8080");
  cjinja_set_var(ctx, "db_host", "db.example.com");
  cjinja_set_var(ctx, "db_port", "5432");
  cjinja_set_var(ctx, "db_name", "webportal_prod");
  cjinja_set_var(ctx, "redis_host", "redis.example.com");
  cjinja_set_var(ctx, "redis_port", "6379");

  cjinja_set_bool(ctx, "enable_ssl", 1);
  cjinja_set_bool(ctx, "enable_caching", 1);
  cjinja_set_bool(ctx, "enable_monitoring", 1);
  cjinja_set_bool(ctx, "enable_debug", 0);
  cjinja_set_bool(ctx, "enable_analytics", 1);

  // Set up feature flags
  char *features[] = {"user_management", "file_upload", "real_time_chat", "api_gateway"};
  cjinja_set_array(ctx, "enabled_features", features, 4);

  // Set up security settings
  char *allowed_origins[] = {"https://webportal.example.com", "https://admin.example.com"};
  cjinja_set_array(ctx, "allowed_origins", allowed_origins, 2);

  printf("Generating configuration files with template inheritance...\n\n");

  // 1. Base Configuration Template
  const char *base_config_template =
      "# {{app_name}} Configuration File\n"
      "# Generated on {{timestamp}}\n"
      "# Version: {{version}}\n"
      "# Environment: {{environment}}\n"
      "\n"
      "# {{% block config_header %}}Base Configuration{{% endblock %}}\n"
      "{{% block config_content %}}\n"
      "  # Default configuration content\n"
      "{{% endblock %}}\n"
      "\n"
      "# {{% block config_footer %}}End of Configuration{{% endblock %}}";

  // 2. Main Application Configuration
  const char *app_config_template =
      "{{% extends base_config %}}\n"
      "{{% block config_header %}}Application Configuration{{% endblock %}}\n"
      "{{% block config_content %}}\n"
      "  [application]\n"
      "  name = {{app_name}}\n"
      "  version = {{version}}\n"
      "  environment = {{environment}}\n"
      "  host = {{host}}\n"
      "  port = {{port}}\n"
      "  \n"
      "  [application.features]\n"
      "  {% for feature in enabled_features %}\n"
      "  {{feature | replace('_', '.')}} = true\n"
      "  {% endfor %}\n"
      "  \n"
      "  [application.security]\n"
      "  enable_ssl = {{enable_ssl | lower}}\n"
      "  allowed_origins = {{allowed_origins | join(', ')}}\n"
      "  \n"
      "  [application.monitoring]\n"
      "  enable_monitoring = {{enable_monitoring | lower}}\n"
      "  enable_analytics = {{enable_analytics | lower}}\n"
      "  enable_debug = {{enable_debug | lower}}\n"
      "{{% endblock %}}\n"
      "{{% block config_footer %}}Application Configuration Complete{{% endblock %}}";

  // 3. Database Configuration
  const char *db_config_template =
      "{{% extends base_config %}}\n"
      "{{% block config_header %}}Database Configuration{{% endblock %}}\n"
      "{{% block config_content %}}\n"
      "  [database]\n"
      "  host = {{db_host}}\n"
      "  port = {{db_port}}\n"
      "  name = {{db_name}}\n"
      "  \n"
      "  [database.pool]\n"
      "  min_connections = 5\n"
      "  max_connections = 20\n"
      "  connection_timeout = 30\n"
      "  \n"
      "  [database.backup]\n"
      "  enabled = true\n"
      "  schedule = \"0 2 * * *\"\n"
      "  retention_days = 30\n"
      "{{% endblock %}}\n"
      "{{% block config_footer %}}Database Configuration Complete{{% endblock %}}";

  // 4. Cache Configuration
  const char *cache_config_template =
      "{{% extends base_config %}}\n"
      "{{% block config_header %}}Cache Configuration{{% endblock %}}\n"
      "{{% block config_content %}}\n"
      "  [cache]\n"
      "  enabled = {{enable_caching | lower}}\n"
      "  \n"
      "  [cache.redis]\n"
      "  host = {{redis_host}}\n"
      "  port = {{redis_port}}\n"
      "  database = 0\n"
      "  \n"
      "  [cache.settings]\n"
      "  default_ttl = 3600\n"
      "  max_memory = \"256mb\"\n"
      "  eviction_policy = \"lru\"\n"
      "{{% endblock %}}\n"
      "{{% block config_footer %}}Cache Configuration Complete{{% endblock %}}";

  // Create inheritance context
  CJinjaInheritanceContext *inherit_ctx = cjinja_create_inheritance_context();
  cjinja_set_base_template(inherit_ctx, base_config_template);

  // Add timestamp
  char timestamp[64];
  time_t now = time(NULL);
  strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
  cjinja_set_var(ctx, "timestamp", timestamp);

  printf("1. Application Configuration:\n");
  printf("=============================\n");

  char *app_config = cjinja_render_with_inheritance(app_config_template, ctx, inherit_ctx);
  printf("%s\n", app_config);
  free(app_config);

  printf("\n2. Database Configuration:\n");
  printf("==========================\n");

  char *db_config = cjinja_render_with_inheritance(db_config_template, ctx, inherit_ctx);
  printf("%s\n", db_config);
  free(db_config);

  printf("\n3. Cache Configuration:\n");
  printf("========================\n");

  char *cache_config = cjinja_render_with_inheritance(cache_config_template, ctx, inherit_ctx);
  printf("%s\n", cache_config);
  free(cache_config);

  // 5. Environment-specific configuration generation
  printf("\n4. Environment-Specific Configurations:\n");
  printf("=======================================\n");

  const char *environments[] = {"development", "staging", "production"};
  const char *env_hosts[] = {"localhost", "staging.example.com", "webportal.example.com"};
  const char *env_ports[] = {"3000", "8080", "443"};

  for (int i = 0; i < 3; i++)
  {
    printf("\n--- %s Environment ---\n", environments[i]);

    // Update context for this environment
    cjinja_set_var(ctx, "environment", environments[i]);
    cjinja_set_var(ctx, "host", env_hosts[i]);
    cjinja_set_var(ctx, "port", env_ports[i]);
    cjinja_set_bool(ctx, "enable_debug", (i == 0)); // Only dev has debug
    cjinja_set_bool(ctx, "enable_ssl", (i == 2));   // Only prod has SSL

    char *env_config = cjinja_render_with_inheritance(app_config_template, ctx, inherit_ctx);
    printf("%s\n", env_config);
    free(env_config);
  }

  // 6. Batch configuration generation
  printf("\n5. Batch Configuration Generation:\n");
  printf("==================================\n");

  const char *config_types[] = {"app", "database", "cache"};
  const char *batch_templates[] = {
      app_config_template,
      db_config_template,
      cache_config_template};

  CJinjaBatchRender *batch = cjinja_create_batch_render(3);

  for (int i = 0; i < 3; i++)
  {
    batch->templates[i] = batch_templates[i];
  }

  int batch_result = cjinja_render_batch(engine, batch, ctx);
  if (batch_result == 0)
  {
    printf("Generated %d configuration files:\n", 3);
    for (int i = 0; i < 3; i++)
    {
      printf("  %s_config.ini (%d bytes)\n", config_types[i],
             (int)strlen(batch->results[i]));
    }
  }

  // 7. Advanced configuration with filters
  printf("\n6. Advanced Configuration with Filters:\n");
  printf("=======================================\n");

  const char *advanced_template =
      "# Advanced Configuration\n"
      "APP_NAME={{app_name | upper}}\n"
      "APP_VERSION={{version}}\n"
      "ENVIRONMENT={{environment | upper}}\n"
      "HOST={{host | lower}}\n"
      "PORT={{port}}\n"
      "FEATURES={{enabled_features | join('|')}}\n"
      "ORIGINS={{allowed_origins | join(';')}}\n"
      "DEBUG={{enable_debug | lower}}\n"
      "SSL={{enable_ssl | lower}}\n"
      "CACHE={{enable_caching | lower}}\n"
      "MONITORING={{enable_monitoring | lower}}\n"
      "ANALYTICS={{enable_analytics | lower}}";

  char *advanced_config = cjinja_render_with_loops(advanced_template, ctx);
  printf("%s\n", advanced_config);
  free(advanced_config);

  // Cleanup
  cjinja_destroy_batch_render(batch);
  cjinja_destroy_inheritance_context(inherit_ctx);
  cjinja_destroy_context(ctx);
  cjinja_destroy_engine(engine);

  printf("\nDemo 4 completed! Configuration generation with template inheritance.\n");
  return 0;
}