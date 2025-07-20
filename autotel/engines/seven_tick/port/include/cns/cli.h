#ifndef CNS_CLI_H
#define CNS_CLI_H

#include "cns/types.h"

// Initialize the CNS CLI framework
int cns_cli_init(const char* program_name);

// Register a domain with its commands
int cns_cli_register_domain(const CNSDomain* domain);

// Run the CLI with given arguments
int cns_cli_run(int argc, char** argv);

// Cleanup
void cns_cli_cleanup(void);

// Helper functions
void cns_cli_print_help(void);
void cns_cli_print_domain_help(const char* domain);
void cns_cli_print_command_help(const char* domain, const char* command);
void cns_cli_print_version(void);

// Error handling
void cns_cli_error(const char* format, ...);
void cns_cli_warning(const char* format, ...);
void cns_cli_info(const char* format, ...);
void cns_cli_success(const char* format, ...);

// Performance tracking
uint64_t cns_get_cycles(void);
void cns_assert_cycles(uint64_t start, uint64_t limit);

#endif // CNS_CLI_H