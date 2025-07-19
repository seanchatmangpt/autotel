#include "cns/cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Dashboard up command handler
static int cmd_dashboard_up(CNSContext* ctx, int argc, char** argv) {
    cns_cli_info("Starting CNS performance dashboard...");
    
    // Check for Docker
    if (system("docker --version > /dev/null 2>&1") != 0) {
        cns_cli_error("Docker not found. Please install Docker first");
        return CNS_ERROR;
    }
    
    // Start Prometheus
    cns_cli_info("Starting Prometheus...");
    int result = system(
        "docker run -d --name cns-prometheus "
        "-p 9090:9090 "
        "-v $(pwd)/prometheus.yml:/etc/prometheus/prometheus.yml "
        "prom/prometheus"
    );
    
    if (result != 0) {
        cns_cli_warning("Failed to start Prometheus");
    }
    
    // Start Grafana
    cns_cli_info("Starting Grafana...");
    result = system(
        "docker run -d --name cns-grafana "
        "-p 3000:3000 "
        "grafana/grafana"
    );
    
    if (result != 0) {
        cns_cli_warning("Failed to start Grafana");
    }
    
    cns_cli_success("Dashboard started!");
    cns_cli_info("Prometheus: http://localhost:9090");
    cns_cli_info("Grafana: http://localhost:3000 (admin/admin)");
    
    return CNS_OK;
}

// Dashboard down command handler
static int cmd_dashboard_down(CNSContext* ctx, int argc, char** argv) {
    cns_cli_info("Stopping CNS dashboard...");
    
    system("docker stop cns-prometheus cns-grafana 2>/dev/null");
    system("docker rm cns-prometheus cns-grafana 2>/dev/null");
    
    cns_cli_success("Dashboard stopped");
    return CNS_OK;
}

// Dashboard status command handler
static int cmd_dashboard_status(CNSContext* ctx, int argc, char** argv) {
    cns_cli_info("Checking dashboard status...");
    
    // Check Prometheus
    int prom_running = system("docker ps | grep cns-prometheus > /dev/null 2>&1") == 0;
    printf("  Prometheus: %s\n", prom_running ? "✓ Running" : "✗ Stopped");
    
    // Check Grafana
    int graf_running = system("docker ps | grep cns-grafana > /dev/null 2>&1") == 0;
    printf("  Grafana:    %s\n", graf_running ? "✓ Running" : "✗ Stopped");
    
    if (prom_running && graf_running) {
        cns_cli_success("Dashboard is running");
    } else {
        cns_cli_warning("Dashboard is not fully running");
    }
    
    return CNS_OK;
}

// Dashboard commands
static CNSCommand dashboard_commands[] = {
    {
        .name = "up",
        .description = "Start monitoring dashboard",
        .handler = cmd_dashboard_up,
        .options = NULL,
        .option_count = 0,
        .arguments = NULL,
        .argument_count = 0
    },
    {
        .name = "down",
        .description = "Stop monitoring dashboard",
        .handler = cmd_dashboard_down,
        .options = NULL,
        .option_count = 0,
        .arguments = NULL,
        .argument_count = 0
    },
    {
        .name = "status",
        .description = "Check dashboard status",
        .handler = cmd_dashboard_status,
        .options = NULL,
        .option_count = 0,
        .arguments = NULL,
        .argument_count = 0
    }
};

// Dashboard domain
CNSDomain cns_dashboard_domain = {
    .name = "dashboard",
    .description = "Performance monitoring dashboard",
    .commands = dashboard_commands,
    .command_count = sizeof(dashboard_commands) / sizeof(dashboard_commands[0])
};