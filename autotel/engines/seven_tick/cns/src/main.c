#include "cns/cli.h"
#include <stdio.h>
#include <stdlib.h>

// Domain implementations
extern CNSDomain cns_build_domain;
extern CNSDomain cns_bench_domain;
extern CNSDomain cns_gate_domain;
extern CNSDomain cns_parse_domain;
extern CNSDomain cns_dashboard_domain;
extern CNSDomain cns_docs_domain;
extern CNSDomain cns_release_domain;
extern CNSDomain cns_profile_domain;
extern CNSDomain cns_deploy_domain;
extern CNSDomain cns_sigma_domain;
extern CNSDomain cns_sparql_domain;
extern CNSDomain cns_shacl_domain;
extern CNSDomain cns_cjinja_domain;
extern CNSDomain cns_telemetry_domain;
extern CNSDomain cns_ml_domain;
extern CNSDomain cns_benchmark_domain;
extern CNSDomain cns_owl_domain;
extern CNSDomain cns_sql_domain;
extern CNSDomain cns_weaver_domain;

int main(int argc, char **argv)
{
    // Initialize CLI framework
    if (cns_cli_init("cns") != CNS_OK)
    {
        fprintf(stderr, "Failed to initialize CNS CLI\n");
        return 1;
    }

    // Register all domains
    cns_cli_register_domain(&cns_build_domain);
    cns_cli_register_domain(&cns_bench_domain);
    cns_cli_register_domain(&cns_gate_domain);
    cns_cli_register_domain(&cns_parse_domain);
    cns_cli_register_domain(&cns_dashboard_domain);
    cns_cli_register_domain(&cns_docs_domain);
    cns_cli_register_domain(&cns_release_domain);
    cns_cli_register_domain(&cns_profile_domain);
    cns_cli_register_domain(&cns_deploy_domain);
    cns_cli_register_domain(&cns_sigma_domain);
    cns_cli_register_domain(&cns_sparql_domain);
    cns_cli_register_domain(&cns_shacl_domain);
    cns_cli_register_domain(&cns_cjinja_domain);
    cns_cli_register_domain(&cns_telemetry_domain);
    cns_cli_register_domain(&cns_ml_domain);
    cns_cli_register_domain(&cns_benchmark_domain);
    cns_cli_register_domain(&cns_owl_domain);
    cns_cli_register_domain(&cns_sql_domain);
    cns_cli_register_domain(&cns_weaver_domain);

    // Run CLI
    int result = cns_cli_run(argc, argv);

    // Cleanup
    cns_cli_cleanup();

    return result;
}