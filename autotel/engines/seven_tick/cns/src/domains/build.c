#include "cns/cli.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

// Build init command handler
static int cmd_build_init(CNSContext* ctx, int argc, char** argv) {
    uint64_t start = cns_get_cycles();
    
    // Check if build directory exists
    struct stat st;
    if (stat("build", &st) == 0) {
        cns_cli_warning("Build directory already exists");
    } else {
        // Create build directory
        if (mkdir("build", 0755) != 0) {
            cns_cli_error("Failed to create build directory");
            return CNS_ERROR_IO;
        }
    }
    
    // Generate CMake configuration
    const char* cmake_cmd = "cmake -S . -B build "
                           "-G Ninja "
                           "-DCMAKE_BUILD_TYPE=Release "
                           "-DCMAKE_C_COMPILER=clang "
                           "-DCMAKE_C_FLAGS=\"-O3 -march=native -flto -ffast-math\" "
                           "-DCMAKE_EXE_LINKER_FLAGS=\"-flto\"";
    
    cns_cli_info("Configuring build with CMake...");
    int result = system(cmake_cmd);
    
    if (result != 0) {
        cns_cli_error("CMake configuration failed");
        return CNS_ERROR;
    }
    
    cns_cli_success("Build system initialized");
    
    // Verify 7-tick constraint
    cns_assert_cycles(start, 7);
    return CNS_OK;
}

// Build all command handler
static int cmd_build_all(CNSContext* ctx, int argc, char** argv) {
    uint64_t start = cns_get_cycles();
    
    // Check if build directory exists
    struct stat st;
    if (stat("build", &st) != 0) {
        cns_cli_error("Build directory not found. Run 'cns build init' first");
        return CNS_ERROR_IO;
    }
    
    // Run ninja build
    cns_cli_info("Building all targets...");
    int result = system("ninja -C build");
    
    if (result != 0) {
        cns_cli_error("Build failed");
        return CNS_ERROR;
    }
    
    cns_cli_success("Build completed successfully");
    
    // Note: Build command is allowed to exceed 7 cycles
    // as it's not a runtime critical path
    return CNS_OK;
}

// Build clean command handler
static int cmd_build_clean(CNSContext* ctx, int argc, char** argv) {
    uint64_t start = cns_get_cycles();
    
    cns_cli_info("Cleaning build directory...");
    int result = system("rm -rf build");
    
    if (result != 0) {
        cns_cli_error("Clean failed");
        return CNS_ERROR;
    }
    
    cns_cli_success("Build directory cleaned");
    
    cns_assert_cycles(start, 7);
    return CNS_OK;
}

// Build debug command handler
static int cmd_build_debug(CNSContext* ctx, int argc, char** argv) {
    uint64_t start = cns_get_cycles();
    
    // Create build directory if needed
    struct stat st;
    if (stat("build", &st) != 0) {
        if (mkdir("build", 0755) != 0) {
            cns_cli_error("Failed to create build directory");
            return CNS_ERROR_IO;
        }
    }
    
    // Generate CMake configuration for debug
    const char* cmake_cmd = "cmake -S . -B build "
                           "-G Ninja "
                           "-DCMAKE_BUILD_TYPE=Debug "
                           "-DCMAKE_C_COMPILER=clang "
                           "-DCMAKE_C_FLAGS=\"-g -O0 -fsanitize=address\" "
                           "-DCMAKE_EXE_LINKER_FLAGS=\"-fsanitize=address\"";
    
    cns_cli_info("Configuring debug build...");
    int result = system(cmake_cmd);
    
    if (result != 0) {
        cns_cli_error("Debug configuration failed");
        return CNS_ERROR;
    }
    
    cns_cli_success("Debug build configured");
    return CNS_OK;
}

// Build commands
static CNSCommand build_commands[] = {
    {
        .name = "init",
        .description = "Initialize build system with CMake",
        .handler = cmd_build_init,
        .options = NULL,
        .option_count = 0,
        .arguments = NULL,
        .argument_count = 0
    },
    {
        .name = "all",
        .description = "Build all targets",
        .handler = cmd_build_all,
        .options = NULL,
        .option_count = 0,
        .arguments = NULL,
        .argument_count = 0
    },
    {
        .name = "clean",
        .description = "Clean build directory",
        .handler = cmd_build_clean,
        .options = NULL,
        .option_count = 0,
        .arguments = NULL,
        .argument_count = 0
    },
    {
        .name = "debug",
        .description = "Configure debug build",
        .handler = cmd_build_debug,
        .options = NULL,
        .option_count = 0,
        .arguments = NULL,
        .argument_count = 0
    }
};

// Build domain
CNSDomain cns_build_domain = {
    .name = "build",
    .description = "Build system management",
    .commands = build_commands,
    .command_count = sizeof(build_commands) / sizeof(build_commands[0])
};