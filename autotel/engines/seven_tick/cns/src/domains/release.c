#include "cns/cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Get current version from VERSION file
static char* get_current_version(void) {
    static char version[32] = "0.0.0";
    FILE* f = fopen("VERSION", "r");
    if (f) {
        fscanf(f, "%31s", version);
        fclose(f);
    }
    return version;
}

// Increment version number
static void increment_version(char* version, const char* bump_type) {
    int major = 0, minor = 0, patch = 0;
    sscanf(version, "%d.%d.%d", &major, &minor, &patch);
    
    if (strcmp(bump_type, "major") == 0) {
        major++;
        minor = 0;
        patch = 0;
    } else if (strcmp(bump_type, "minor") == 0) {
        minor++;
        patch = 0;
    } else {
        patch++;
    }
    
    snprintf(version, 32, "%d.%d.%d", major, minor, patch);
}

// Release patch command handler
static int cmd_release_patch(CNSContext* ctx, int argc, char** argv) {
    return cmd_release_version(ctx, 1, (char*[]){"patch"});
}

// Release minor command handler
static int cmd_release_minor(CNSContext* ctx, int argc, char** argv) {
    return cmd_release_version(ctx, 1, (char*[]){"minor"});
}

// Release major command handler
static int cmd_release_major(CNSContext* ctx, int argc, char** argv) {
    return cmd_release_version(ctx, 1, (char*[]){"major"});
}

// Forward declaration
static int cmd_release_version(CNSContext* ctx, int argc, char** argv);

// Release patch command handler
static int cmd_release_patch(CNSContext* ctx, int argc, char** argv) {
    return cmd_release_version(ctx, 1, (char*[]){"patch"});
}

// Release minor command handler
static int cmd_release_minor(CNSContext* ctx, int argc, char** argv) {
    return cmd_release_version(ctx, 1, (char*[]){"minor"});
}

// Release major command handler
static int cmd_release_major(CNSContext* ctx, int argc, char** argv) {
    return cmd_release_version(ctx, 1, (char*[]){"major"});
}

// Generic release version handler
static int cmd_release_version(CNSContext* ctx, int argc, char** argv) {
    const char* bump_type = argc > 0 ? argv[0] : "patch";
    
    // Get current version
    char* current = get_current_version();
    char new_version[32];
    strcpy(new_version, current);
    
    // Increment version
    increment_version(new_version, bump_type);
    
    cns_cli_info("Creating release %s → %s", current, new_version);
    
    // Run gatekeeper first
    cns_cli_info("Running gatekeeper validation...");
    if (system("./cns gate run") != 0) {
        cns_cli_error("Gatekeeper failed - cannot release");
        return CNS_ERROR;
    }
    
    // Update VERSION file
    FILE* f = fopen("VERSION", "w");
    if (f) {
        fprintf(f, "%s\n", new_version);
        fclose(f);
    }
    
    // Generate changelog
    cns_cli_info("Generating changelog...");
    time_t now = time(NULL);
    struct tm* tm = localtime(&now);
    char date[32];
    strftime(date, sizeof(date), "%Y-%m-%d", tm);
    
    f = fopen("CHANGELOG.md", "a");
    if (f) {
        fprintf(f, "\n## [%s] - %s\n\n", new_version, date);
        fprintf(f, "### Added\n- ...\n\n");
        fprintf(f, "### Changed\n- ...\n\n");
        fprintf(f, "### Fixed\n- ...\n\n");
        fclose(f);
    }
    
    // Create git tag
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "git tag -a v%s -m \"Release v%s\"", 
             new_version, new_version);
    
    cns_cli_info("Creating git tag v%s", new_version);
    system(cmd);
    
    // Create GitHub release draft
    cns_cli_info("Creating GitHub release draft...");
    snprintf(cmd, sizeof(cmd), 
             "gh release create v%s --draft --title \"v%s\" "
             "--notes \"Release v%s\n\nSee CHANGELOG.md for details.\"",
             new_version, new_version, new_version);
    system(cmd);
    
    cns_cli_success("Release v%s created (draft)", new_version);
    cns_cli_info("Review and publish: gh release edit v%s --draft=false", new_version);
    
    return CNS_OK;
}

// Release commands
static CNSCommand release_commands[] = {
    {
        .name = "patch",
        .description = "Create patch release (0.0.x)",
        .handler = cmd_release_patch,
        .options = NULL,
        .option_count = 0,
        .arguments = NULL,
        .argument_count = 0
    },
    {
        .name = "minor",
        .description = "Create minor release (0.x.0)",
        .handler = cmd_release_minor,
        .options = NULL,
        .option_count = 0,
        .arguments = NULL,
        .argument_count = 0
    },
    {
        .name = "major",
        .description = "Create major release (x.0.0)",
        .handler = cmd_release_major,
        .options = NULL,
        .option_count = 0,
        .arguments = NULL,
        .argument_count = 0
    }
};

// Release domain
CNSDomain cns_release_domain = {
    .name = "release",
    .description = "Release management",
    .commands = release_commands,
    .command_count = sizeof(release_commands) / sizeof(release_commands[0])
};