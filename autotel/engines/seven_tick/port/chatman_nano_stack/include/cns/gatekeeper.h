#ifndef CNS_GATEKEEPER_H
#define CNS_GATEKEEPER_H

#include <stdint.h>

// ============================================================================
// GATEKEEPER CONSTANTS
// ============================================================================

#define CNS_GATEKEEPER_CHATMAN_CONSTANT_CYCLES 7
#define CNS_GATEKEEPER_MIN_THROUGHPUT_MOPS 10
#define CNS_GATEKEEPER_MIN_SIGMA_LEVEL 4.0
#define CNS_GATEKEEPER_SAMPLE_SIZE 1000000
#define CNS_GATEKEEPER_PERCENTILE_95 95

// ============================================================================
// GATEKEEPER METRICS STRUCTURE
// ============================================================================

typedef struct
{
  double mean_cycles;
  double std_cycles;
  double p95_cycles;
  double throughput_mops;
  uint64_t total_operations;
  uint64_t total_cycles;
  uint64_t min_cycles;
  uint64_t max_cycles;
  uint32_t histogram[256]; // Cycle distribution
} CnsGatekeeperMetrics;

// ============================================================================
// GATEKEEPER CTQ RESULTS
// ============================================================================

typedef enum
{
  CNS_CTQ_CORRECTNESS = 0,
  CNS_CTQ_CYCLE_BUDGET = 1,
  CNS_CTQ_THROUGHPUT = 2,
  CNS_CTQ_ONTOLOGY_PARSING = 3,
  CNS_CTQ_COUNT
} CnsCtqType;

typedef struct
{
  CnsCtqType type;
  int passed;
  double value;
  double threshold;
  const char *description;
} CnsCtqResult;

typedef struct
{
  CnsCtqResult results[CNS_CTQ_COUNT];
  int all_passed;
  CnsGatekeeperMetrics metrics;
} CnsGatekeeperReport;

// ============================================================================
// GATEKEEPER API FUNCTIONS
// ============================================================================

/**
 * Run all Gatekeeper CTQ tests
 * @return 0 if all tests pass, 1 if any test fails
 */
int cns_gatekeeper_run_all_tests(void);

/**
 * Run individual CTQ test
 * @param ctq_type The type of CTQ test to run
 * @return 0 if test passes, 1 if test fails
 */
int cns_gatekeeper_run_ctq_test(CnsCtqType ctq_type);

/**
 * Generate comprehensive Gatekeeper report
 * @param report Pointer to report structure to fill
 * @return 0 if all tests pass, 1 if any test fails
 */
int cns_gatekeeper_generate_report(CnsGatekeeperReport *report);

/**
 * Print Gatekeeper report to stdout
 * @param report The report to print
 */
void cns_gatekeeper_print_report(const CnsGatekeeperReport *report);

/**
 * Save Gatekeeper report to JSON file
 * @param report The report to save
 * @param filename Output filename
 * @return 0 on success, 1 on failure
 */
int cns_gatekeeper_save_report_json(const CnsGatekeeperReport *report, const char *filename);

// ============================================================================
// SIGMA CALCULATION UTILITIES
// ============================================================================

/**
 * Calculate short-term sigma level for one-sided spec (upper)
 * @param USL Upper specification limit
 * @param mu Mean value
 * @param sigma Standard deviation
 * @return Sigma level
 */
double cns_gatekeeper_sigma(double USL, double mu, double sigma);

/**
 * Convert sigma level to parts-per-million
 * @param sigma_st Short-term sigma level
 * @return Parts per million
 */
double cns_gatekeeper_dpm(double sigma_st);

/**
 * Calculate process capability index (Cpk)
 * @param USL Upper specification limit
 * @param mu Mean value
 * @param sigma Standard deviation
 * @return Cpk value
 */
double cns_gatekeeper_cpk(double USL, double mu, double sigma);

// ============================================================================
// CYCLE MEASUREMENT UTILITIES
// ============================================================================

/**
 * Get current CPU cycle count
 * @return CPU cycles since system start
 */
uint64_t cns_gatekeeper_get_cycles(void);

/**
 * Initialize cycle counter (enables on ARM64)
 */
void cns_gatekeeper_init_cycles(void);

/**
 * Calculate cycle-based metrics from raw data
 * @param metrics Metrics structure to calculate
 */
void cns_gatekeeper_calculate_metrics(CnsGatekeeperMetrics *metrics);

// ============================================================================
// GATEKEEPER CONFIGURATION
// ============================================================================

typedef struct
{
  int chatman_constant_cycles;
  double min_throughput_mops;
  double min_sigma_level;
  uint64_t sample_size;
  int percentile_95;
  int verbose_output;
  int save_reports;
  const char *report_directory;
} CnsGatekeeperConfig;

/**
 * Initialize Gatekeeper with custom configuration
 * @param config Configuration structure
 */
void cns_gatekeeper_init(const CnsGatekeeperConfig *config);

/**
 * Get default Gatekeeper configuration
 * @return Default configuration
 */
CnsGatekeeperConfig cns_gatekeeper_get_default_config(void);

// ============================================================================
// GATEKEEPER UTILITIES
// ============================================================================

/**
 * Format cycle count as human-readable string
 * @param cycles Cycle count
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return Formatted string
 */
const char *cns_gatekeeper_format_cycles(uint64_t cycles, char *buffer, size_t buffer_size);

/**
 * Format throughput as human-readable string
 * @param mops Millions of operations per second
 * @param buffer Output buffer
 * @param buffer_size Buffer size
 * @return Formatted string
 */
const char *cns_gatekeeper_format_throughput(double mops, char *buffer, size_t buffer_size);

/**
 * Get CTQ test name
 * @param ctq_type CTQ type
 * @return Human-readable name
 */
const char *cns_gatekeeper_get_ctq_name(CnsCtqType ctq_type);

/**
 * Get CTQ test description
 * @param ctq_type CTQ type
 * @return Human-readable description
 */
const char *cns_gatekeeper_get_ctq_description(CnsCtqType ctq_type);

#endif // CNS_GATEKEEPER_H