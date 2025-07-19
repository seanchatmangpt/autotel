#include "cns/cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

// Validate documentation
static bool validate_docs(const char* path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        cns_cli_warning("Documentation not found: %s", path);
        return false;
    }
    
    // Check for required files
    const char* required[] = {
        "README.md",
        "docs/api.md",
        "docs/architecture.md",
        "docs/quickstart.md"
    };
    
    bool all_found = true;
    for (size_t i = 0; i < sizeof(required)/sizeof(required[0]); i++) {
        char full_path[256];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, required[i]);
        
        if (stat(full_path, &st) != 0) {
            cns_cli_warning("Missing: %s", required[i]);
            all_found = false;
        } else {
            printf("  ✓ %s\n", required[i]);
        }
    }
    
    return all_found;
}

// Docs validate command handler
static int cmd_docs_validate(CNSContext* ctx, int argc, char** argv) {
    cns_cli_info("Validating documentation...");
    
    // Validate main docs
    bool main_valid = validate_docs(".");
    
    // Validate man pages
    cns_cli_info("Checking man pages...");
    struct stat st;
    bool man_valid = stat("man/cns.1", &st) == 0;
    printf("  %s man/cns.1\n", man_valid ? "✓" : "✗");
    
    // Validate Doxygen
    cns_cli_info("Checking Doxygen configuration...");
    bool doxy_valid = stat("Doxyfile", &st) == 0;
    printf("  %s Doxyfile\n", doxy_valid ? "✓" : "✗");
    
    // Validate sequence diagrams
    cns_cli_info("Checking sequence diagrams...");
    bool seq_valid = stat("docs/diagrams", &st) == 0;
    printf("  %s docs/diagrams/\n", seq_valid ? "✓" : "✗");
    
    if (main_valid && man_valid && doxy_valid && seq_valid) {
        cns_cli_success("Documentation validation passed");
        return CNS_OK;
    } else {
        cns_cli_error("Documentation validation failed");
        return CNS_ERROR;
    }
}

// Docs generate command handler
static int cmd_docs_generate(CNSContext* ctx, int argc, char** argv) {
    cns_cli_info("Generating documentation...");
    
    // Generate Doxygen docs
    if (system("which doxygen > /dev/null 2>&1") == 0) {
        cns_cli_info("Running Doxygen...");
        int result = system("doxygen Doxyfile");
        if (result == 0) {
            cns_cli_success("Generated HTML docs in docs/html/");
        }
    } else {
        cns_cli_warning("Doxygen not found - skipping API docs");
    }
    
    // Generate man pages
    cns_cli_info("Generating man pages...");
    system("mkdir -p man");
    
    // Create basic man page
    FILE* man = fopen("man/cns.1", "w");
    if (man) {
        fprintf(man, ".TH CNS 1 \"2025-01-19\" \"1.0\" \"CNS Manual\"\n");
        fprintf(man, ".SH NAME\n");
        fprintf(man, "cns \\- CHATMAN NANO-STACK performance-first CLI\n");
        fprintf(man, ".SH SYNOPSIS\n");
        fprintf(man, ".B cns\n");
        fprintf(man, "\\fI<domain>\\fP \\fI<command>\\fP [options] [arguments]\n");
        fprintf(man, ".SH DESCRIPTION\n");
        fprintf(man, "CNS is a 7-tick performance-guaranteed CLI tool.\n");
        fclose(man);
        
        cns_cli_success("Generated man page: man/cns.1");
    }
    
    return CNS_OK;
}

// Docs serve command handler
static int cmd_docs_serve(CNSContext* ctx, int argc, char** argv) {
    cns_cli_info("Starting documentation server...");
    
    // Check if docs exist
    struct stat st;
    if (stat("docs/html/index.html", &st) != 0) {
        cns_cli_error("No HTML docs found. Run 'cns docs generate' first");
        return CNS_ERROR_IO;
    }
    
    // Start simple HTTP server
    cns_cli_info("Serving docs at http://localhost:8000");
    system("cd docs/html && python3 -m http.server 8000");
    
    return CNS_OK;
}

// Docs commands
static CNSCommand docs_commands[] = {
    {
        .name = "validate",
        .description = "Validate documentation completeness",
        .handler = cmd_docs_validate,
        .options = NULL,
        .option_count = 0,
        .arguments = NULL,
        .argument_count = 0
    },
    {
        .name = "generate",
        .description = "Generate documentation",
        .handler = cmd_docs_generate,
        .options = NULL,
        .option_count = 0,
        .arguments = NULL,
        .argument_count = 0
    },
    {
        .name = "serve",
        .description = "Serve documentation locally",
        .handler = cmd_docs_serve,
        .options = NULL,
        .option_count = 0,
        .arguments = NULL,
        .argument_count = 0
    }
};

// Docs domain
CNSDomain cns_docs_domain = {
    .name = "docs",
    .description = "Documentation management",
    .commands = docs_commands,
    .command_count = sizeof(docs_commands) / sizeof(docs_commands[0])
};