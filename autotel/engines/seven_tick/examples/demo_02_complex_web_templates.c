#include "../compiler/src/cjinja.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Demo 2: Complex Web Templates with 49-Tick Path
// Use Case: Feature-rich web applications requiring template inheritance, includes, and advanced filters

int main()
{
  printf("Demo 2: Complex Web Templates (49-Tick Path)\n");
  printf("=============================================\n\n");

  // Create engine and context
  CJinjaEngine *engine = cjinja_create("./templates");
  CJinjaContext *ctx = cjinja_create_context();

  if (!engine || !ctx)
  {
    printf("Failed to create CJinja engine or context\n");
    return 1;
  }

  // Set up web application context
  cjinja_set_var(ctx, "site_title", "TechCorp Portal");
  cjinja_set_var(ctx, "user_name", "Alice Johnson");
  cjinja_set_var(ctx, "user_email", "alice@techcorp.com");
  cjinja_set_var(ctx, "user_role", "Senior Developer");
  cjinja_set_var(ctx, "current_page", "dashboard");
  cjinja_set_var(ctx, "version", "2.1.0");

  cjinja_set_bool(ctx, "is_admin", 1);
  cjinja_set_bool(ctx, "is_premium", 1);
  cjinja_set_bool(ctx, "show_analytics", 1);
  cjinja_set_bool(ctx, "dark_mode", 0);

  // Set up navigation items
  char *nav_items[] = {"Dashboard", "Projects", "Team", "Settings", "Analytics"};
  cjinja_set_array(ctx, "navigation", nav_items, 5);

  // Set up user projects
  char *projects[] = {"Web Portal", "Mobile App", "API Gateway", "Database Migration"};
  cjinja_set_array(ctx, "projects", projects, 4);

  printf("Creating complex web templates with 49-tick features...\n\n");

  // 1. Base Template with Inheritance
  const char *base_template =
      "<!DOCTYPE html>\n"
      "<html lang=\"en\">\n"
      "<head>\n"
      "    <meta charset=\"UTF-8\">\n"
      "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
      "    <title>{{% block title %}}{{site_title}}{{% endblock %}}</title>\n"
      "    <link rel=\"stylesheet\" href=\"/css/main.css\">\n"
      "    {{% block head_extra %}}{{% endblock %}}\n"
      "</head>\n"
      "<body class=\"{{% block body_class %}}default{{% endblock %}}\">\n"
      "    <header class=\"main-header\">\n"
      "        {{% block header %}}\n"
      "        <nav class=\"main-nav\">\n"
      "            <div class=\"logo\">{{site_title}}</div>\n"
      "            <ul class=\"nav-menu\">\n"
      "                {% for item in navigation %}\n"
      "                <li><a href=\"/{{item | lower}}\">{{item}}</a></li>\n"
      "                {% endfor %}\n"
      "            </ul>\n"
      "            <div class=\"user-info\">\n"
      "                <span>{{user_name | capitalize}}</span>\n"
      "                <span class=\"role\">{{user_role | lower}}</span>\n"
      "            </div>\n"
      "        </nav>\n"
      "        {{% endblock %}}\n"
      "    </header>\n"
      "    \n"
      "    <main class=\"main-content\">\n"
      "        {{% block content %}}\n"
      "        <p>Default content</p>\n"
      "        {{% endblock %}}\n"
      "    </main>\n"
      "    \n"
      "    <footer class=\"main-footer\">\n"
      "        {{% block footer %}}\n"
      "        <p>&copy; 2024 {{site_title}}. Version {{version}}.</p>\n"
      "        {{% endblock %}}\n"
      "    </footer>\n"
      "    \n"
      "    <script src=\"/js/main.js\"></script>\n"
      "    {{% block scripts %}}{{% endblock %}}\n"
      "</body>\n"
      "</html>";

  // 2. Dashboard Page Template (extends base)
  const char *dashboard_template =
      "{{% extends base %}}\n"
      "{{% block title %}}Dashboard - {{site_title}}{{% endblock %}}\n"
      "{{% block body_class %}}dashboard-page{{% endblock %}}\n"
      "{{% block content %}}\n"
      "    <div class=\"dashboard-container\">\n"
      "        <h1>Welcome, {{user_name | capitalize}}!</h1>\n"
      "        \n"
      "        <div class=\"user-stats\">\n"
      "            <div class=\"stat-card\">\n"
      "                <h3>Your Projects</h3>\n"
      "                <div class=\"stat-value\">{{projects | length}}</div>\n"
      "            </div>\n"
      "            <div class=\"stat-card\">\n"
      "                <h3>Role</h3>\n"
      "                <div class=\"stat-value\">{{user_role | upper}}</div>\n"
      "            </div>\n"
      "        </div>\n"
      "        \n"
      "        {% if is_admin %}\n"
      "        <div class=\"admin-section\">\n"
      "            <h2>Administrator Tools</h2>\n"
      "            <div class=\"admin-actions\">\n"
      "                <button class=\"btn btn-primary\">User Management</button>\n"
      "                <button class=\"btn btn-secondary\">System Settings</button>\n"
      "                <button class=\"btn btn-warning\">Analytics Dashboard</button>\n"
      "            </div>\n"
      "        </div>\n"
      "        {% endif %}\n"
      "        \n"
      "        {% if is_premium %}\n"
      "        <div class=\"premium-section\">\n"
      "            <h2>Premium Features</h2>\n"
      "            <ul class=\"premium-features\">\n"
      "                <li>Advanced Analytics</li>\n"
      "                <li>Priority Support</li>\n"
      "                <li>Custom Themes</li>\n"
      "                <li>API Access</li>\n"
      "            </ul>\n"
      "        </div>\n"
      "        {% endif %}\n"
      "        \n"
      "        <div class=\"projects-section\">\n"
      "            <h2>Your Active Projects</h2>\n"
      "            <div class=\"project-grid\">\n"
      "                {% for project in projects %}\n"
      "                <div class=\"project-card\">\n"
      "                    <h3>{{project | capitalize}}</h3>\n"
      "                    <p>Project description and status</p>\n"
      "                    <div class=\"project-actions\">\n"
      "                        <button class=\"btn btn-sm\">View</button>\n"
      "                        <button class=\"btn btn-sm\">Edit</button>\n"
      "                    </div>\n"
      "                </div>\n"
      "                {% endfor %}\n"
      "            </div>\n"
      "        </div>\n"
      "        \n"
      "        {% if show_analytics %}\n"
      "        <div class=\"analytics-section\">\n"
      "            <h2>Analytics Overview</h2>\n"
      "            <div class=\"analytics-grid\">\n"
      "                <div class=\"metric\">\n"
      "                    <span class=\"metric-label\">Active Users</span>\n"
      "                    <span class=\"metric-value\">1,247</span>\n"
      "                </div>\n"
      "                <div class=\"metric\">\n"
      "                    <span class=\"metric-label\">Page Views</span>\n"
      "                    <span class=\"metric-value\">45,892</span>\n"
      "                </div>\n"
      "                <div class=\"metric\">\n"
      "                    <span class=\"metric-label\">Conversion Rate</span>\n"
      "                    <span class=\"metric-value\">3.2%%</span>\n"
      "                </div>\n"
      "            </div>\n"
      "        </div>\n"
      "        {% endif %}\n"
      "    </div>\n"
      "{{% endblock %}}\n"
      "{{% block scripts %}}\n"
      "    <script src=\"/js/dashboard.js\"></script>\n"
      "    <script src=\"/js/analytics.js\"></script>\n"
      "{{% endblock %}}";

  // Create inheritance context
  CJinjaInheritanceContext *inherit_ctx = cjinja_create_inheritance_context();
  cjinja_set_base_template(inherit_ctx, base_template);

  printf("Rendering complex dashboard template with 49-tick features...\n\n");

  // Render the dashboard template
  char *dashboard_html = cjinja_render_with_inheritance(dashboard_template, ctx, inherit_ctx);

  if (dashboard_html)
  {
    printf("Dashboard HTML Generated Successfully!\n");
    printf("=====================================\n");
    printf("%s\n", dashboard_html);
    free(dashboard_html);
  }
  else
  {
    printf("Failed to render dashboard template\n");
  }

  // 3. Demonstrate advanced filters
  printf("\nAdvanced Filter Examples:\n");
  printf("=========================\n");

  const char *filter_template =
      "User Information:\n"
      "  Name: {{user_name | trim | capitalize}}\n"
      "  Email: {{user_email | lower}}\n"
      "  Role: {{user_role | upper}}\n"
      "  Projects: {{projects | join(', ')}}\n"
      "  Project Count: {{projects | length}}\n"
      "  Default Value: {{missing_var | default('Not Set')}}\n"
      "  Sliced Name: {{user_name | slice(0,5)}}\n"
      "  Replaced: {{user_role | replace('Developer','Engineer')}}";

  char *filtered_result = cjinja_render_with_loops(filter_template, ctx);
  printf("%s\n", filtered_result);
  free(filtered_result);

  // 4. Demonstrate batch rendering
  printf("\nBatch Rendering Example:\n");
  printf("========================\n");

  const char *batch_templates[] = {
      "<div class=\"user-card\">{{user_name | capitalize}}</div>",
      "<div class=\"role-badge\">{{user_role | upper}}</div>",
      "<div class=\"email-link\">{{user_email | lower}}</div>",
      "<div class=\"project-count\">{{projects | length}} projects</div>",
      "<div class=\"admin-status\">{% if is_admin %}Admin{% else %}User{% endif %}</div>"};

  CJinjaBatchRender *batch = cjinja_create_batch_render(5);
  for (int i = 0; i < 5; i++)
  {
    batch->templates[i] = batch_templates[i];
  }

  int batch_result = cjinja_render_batch(engine, batch, ctx);
  if (batch_result == 0)
  {
    printf("Batch rendering completed successfully!\n");
    for (int i = 0; i < 5; i++)
    {
      printf("  Template %d: %s\n", i + 1, batch->results[i]);
    }
  }

  // Cleanup
  cjinja_destroy_batch_render(batch);
  cjinja_destroy_inheritance_context(inherit_ctx);
  cjinja_destroy_context(ctx);
  cjinja_destroy_engine(engine);

  printf("\nDemo 2 completed! 49-tick path provides full template engine features.\n");
  return 0;
}