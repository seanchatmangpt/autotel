/**
 * @file nanoregex.h
 * @brief NanoRegex - Ultra-Fast Signal Pattern Matching for BitActors
 * @version 1.0.0
 * 
 * NanoRegex provides sub-microsecond pattern matching for BitActor signals.
 * Optimized for financial news, market data, and real-time signal processing.
 * 
 * Performance Targets:
 * - Pattern matching: <50ns per signal
 * - Compilation: <1μs per pattern
 * - Memory: <1KB per compiled pattern
 * 
 * @author Sean Chatman - Architect of the Fifth Epoch
 * @date 2024-01-15
 */

#ifndef NANOREGEX_H
#define NANOREGEX_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// =============================================================================
// NANOREGEX CONSTANTS
// =============================================================================

#define NANOREGEX_VERSION "1.0.0"
#define MAX_PATTERN_LENGTH 256          // Maximum pattern length
#define MAX_SIGNAL_LENGTH 1024          // Maximum signal length
#define MAX_COMPILED_SIZE 512           // Maximum compiled pattern size
#define MAX_CAPTURE_GROUPS 8            // Maximum capture groups

// Pattern types optimized for different signal categories
#define NANOREGEX_NEWS 0x01             // News text patterns
#define NANOREGEX_PRICE 0x02            // Price/numeric patterns
#define NANOREGEX_SYMBOL 0x04           // Trading symbol patterns
#define NANOREGEX_TIME 0x08             // Timestamp patterns
#define NANOREGEX_SENTIMENT 0x10        // Sentiment patterns
#define NANOREGEX_VOLUME 0x20           // Volume patterns
#define NANOREGEX_CUSTOM 0x80           // Custom patterns

// Compilation flags
#define NANOREGEX_OPTIMIZE_SPEED 0x01   // Optimize for speed
#define NANOREGEX_OPTIMIZE_MEMORY 0x02  // Optimize for memory
#define NANOREGEX_SIMD_ENABLED 0x04     // Enable SIMD instructions
#define NANOREGEX_BRANCH_HINTS 0x08     // Enable branch prediction hints

// =============================================================================
// NANOREGEX CORE TYPES
// =============================================================================

/**
 * @brief Compiled nanoregex pattern
 * 
 * Represents a compiled regular expression optimized for BitActor signal matching.
 * Uses finite state machine with lookup tables for maximum speed.
 */
typedef struct {
    uint8_t compiled_code[MAX_COMPILED_SIZE];   // Compiled state machine
    uint32_t code_size;                         // Size of compiled code
    uint8_t pattern_type;                       // Pattern type flags
    uint8_t optimization_flags;                 // Optimization flags
    uint16_t state_count;                       // Number of states
    uint64_t pattern_hash;                      // Hash of original pattern
    uint64_t compile_time_ns;                   // Compilation time
    bool case_sensitive;                        // Case sensitivity
    bool multiline;                             // Multiline mode
} NanoRegexPattern;

/**
 * @brief Pattern match result
 */
typedef struct {
    bool matched;                               // Pattern matched
    uint16_t match_start;                       // Start position
    uint16_t match_length;                      // Match length
    uint16_t capture_starts[MAX_CAPTURE_GROUPS]; // Capture group starts
    uint16_t capture_lengths[MAX_CAPTURE_GROUPS]; // Capture group lengths
    uint8_t capture_count;                      // Number of captures
    uint64_t match_time_ns;                     // Matching time
} NanoRegexMatch;

/**
 * @brief NanoRegex engine context
 */
typedef struct {
    NanoRegexPattern* patterns;                 // Compiled patterns array
    uint32_t pattern_count;                     // Number of patterns
    uint32_t pattern_capacity;                  // Pattern array capacity
    uint64_t total_matches;                     // Total matches performed
    uint64_t total_match_time_ns;               // Total matching time
    uint8_t default_flags;                      // Default compilation flags
    bool simd_available;                        // SIMD instructions available
} NanoRegexEngine;

// =============================================================================
// NANOREGEX ENGINE API
// =============================================================================

/**
 * @brief Create NanoRegex engine
 * @return Initialized NanoRegex engine
 */
NanoRegexEngine* nanoregex_create(void);

/**
 * @brief Destroy NanoRegex engine
 * @param engine Engine to destroy
 */
void nanoregex_destroy(NanoRegexEngine* engine);

/**
 * @brief Compile regular expression pattern
 * @param engine NanoRegex engine
 * @param pattern Regular expression pattern
 * @param pattern_type Pattern type flags
 * @param optimization_flags Optimization flags
 * @return Pattern ID or 0 on failure
 */
uint32_t nanoregex_compile(NanoRegexEngine* engine, 
                          const char* pattern,
                          uint8_t pattern_type,
                          uint8_t optimization_flags);

/**
 * @brief Match pattern against signal
 * @param engine NanoRegex engine
 * @param pattern_id Compiled pattern ID
 * @param signal Input signal data
 * @param signal_length Signal length
 * @param[out] match Match result
 * @return true if pattern matched
 */
bool nanoregex_match(NanoRegexEngine* engine,
                    uint32_t pattern_id,
                    const uint8_t* signal,
                    uint16_t signal_length,
                    NanoRegexMatch* match);

/**
 * @brief Match all patterns against signal (batch mode)
 * @param engine NanoRegex engine
 * @param signal Input signal data
 * @param signal_length Signal length
 * @param[out] matches Array of match results
 * @param max_matches Maximum matches to return
 * @return Number of matches found
 */
uint32_t nanoregex_match_all(NanoRegexEngine* engine,
                            const uint8_t* signal,
                            uint16_t signal_length,
                            NanoRegexMatch* matches,
                            uint32_t max_matches);

/**
 * @brief Get pattern by ID
 * @param engine NanoRegex engine
 * @param pattern_id Pattern ID
 * @return Pointer to pattern or NULL
 */
const NanoRegexPattern* nanoregex_get_pattern(NanoRegexEngine* engine, uint32_t pattern_id);

// =============================================================================
// OPTIMIZED PATTERN TEMPLATES
// =============================================================================

/**
 * @brief Compile common financial news patterns
 * @param engine NanoRegex engine
 * @return Number of patterns compiled
 */
uint32_t nanoregex_compile_financial_patterns(NanoRegexEngine* engine);

/**
 * @brief Compile common price/numeric patterns
 * @param engine NanoRegex engine
 * @return Number of patterns compiled
 */
uint32_t nanoregex_compile_price_patterns(NanoRegexEngine* engine);

/**
 * @brief Compile common trading symbol patterns
 * @param engine NanoRegex engine
 * @return Number of patterns compiled
 */
uint32_t nanoregex_compile_symbol_patterns(NanoRegexEngine* engine);

/**
 * @brief Compile common timestamp patterns
 * @param engine NanoRegex engine
 * @return Number of patterns compiled
 */
uint32_t nanoregex_compile_time_patterns(NanoRegexEngine* engine);

// =============================================================================
// SIGNAL PROCESSING INTEGRATION
// =============================================================================

/**
 * @brief Extract BitActor signal mask from pattern match
 * @param match Pattern match result
 * @param signal_type Signal type
 * @return 64-bit signal mask for BitActor
 */
uint64_t nanoregex_extract_signal_mask(const NanoRegexMatch* match, uint8_t signal_type);

/**
 * @brief Convert pattern match to BitActor trigger
 * @param match Pattern match result
 * @param[out] trigger_bits BitActor trigger bits
 * @return true if conversion successful
 */
bool nanoregex_to_bitactor_trigger(const NanoRegexMatch* match, uint8_t* trigger_bits);

/**
 * @brief Process news signal for financial triggers
 * @param engine NanoRegex engine
 * @param news_text News text to process
 * @param text_length Text length
 * @param[out] financial_triggers Array of financial triggers
 * @param max_triggers Maximum triggers to return
 * @return Number of triggers found
 */
uint32_t nanoregex_process_news_signal(NanoRegexEngine* engine,
                                      const char* news_text,
                                      uint16_t text_length,
                                      uint64_t* financial_triggers,
                                      uint32_t max_triggers);

/**
 * @brief Process price signal for trading triggers
 * @param engine NanoRegex engine
 * @param price_data Price data to process
 * @param data_length Data length
 * @param[out] trading_triggers Array of trading triggers
 * @param max_triggers Maximum triggers to return
 * @return Number of triggers found
 */
uint32_t nanoregex_process_price_signal(NanoRegexEngine* engine,
                                       const uint8_t* price_data,
                                       uint16_t data_length,
                                       uint64_t* trading_triggers,
                                       uint32_t max_triggers);

// =============================================================================
// PERFORMANCE OPTIMIZATION
// =============================================================================

/**
 * @brief Enable SIMD optimization for pattern matching
 * @param engine NanoRegex engine
 * @param enable Enable SIMD optimization
 * @return true if SIMD available and enabled
 */
bool nanoregex_enable_simd(NanoRegexEngine* engine, bool enable);

/**
 * @brief Optimize patterns for current CPU
 * @param engine NanoRegex engine
 * @return Number of patterns optimized
 */
uint32_t nanoregex_optimize_for_cpu(NanoRegexEngine* engine);

/**
 * @brief Benchmark pattern matching performance
 * @param engine NanoRegex engine
 * @param iterations Number of benchmark iterations
 * @param[out] avg_match_time_ns Average match time
 * @param[out] matches_per_second Matches per second
 * @return true if benchmark successful
 */
bool nanoregex_benchmark(NanoRegexEngine* engine,
                        uint32_t iterations,
                        uint64_t* avg_match_time_ns,
                        double* matches_per_second);

// =============================================================================
// ENGINE STATISTICS
// =============================================================================

/**
 * @brief Get engine performance statistics
 * @param engine NanoRegex engine
 * @param[out] total_patterns Total compiled patterns
 * @param[out] total_matches Total matches performed
 * @param[out] avg_match_time_ns Average match time
 * @param[out] memory_usage_bytes Memory usage in bytes
 */
void nanoregex_get_stats(const NanoRegexEngine* engine,
                        uint32_t* total_patterns,
                        uint64_t* total_matches,
                        uint64_t* avg_match_time_ns,
                        uint32_t* memory_usage_bytes);

/**
 * @brief Print engine status and statistics
 * @param engine NanoRegex engine
 */
void nanoregex_print_stats(const NanoRegexEngine* engine);

// =============================================================================
// COMMON PATTERN DEFINITIONS
// =============================================================================

// Financial patterns
#define NANOREGEX_STOCK_SYMBOL "\\b[A-Z]{1,5}\\b"
#define NANOREGEX_PRICE_USD "\\$[0-9,]+\\.?[0-9]*"
#define NANOREGEX_PERCENT_CHANGE "[+-]?[0-9]+\\.?[0-9]*%"
#define NANOREGEX_VOLUME "[0-9,]+\\s*(shares?|volume)"

// News sentiment patterns  
#define NANOREGEX_BULLISH "\\b(bull|rally|surge|soar|climb|gain|rise|up|positive)\\b"
#define NANOREGEX_BEARISH "\\b(bear|fall|drop|plunge|decline|down|crash|negative)\\b"
#define NANOREGEX_NEUTRAL "\\b(stable|flat|unchanged|steady|hold)\\b"

// Time patterns
#define NANOREGEX_TIME_HMS "[0-9]{1,2}:[0-9]{2}:[0-9]{2}"
#define NANOREGEX_DATE_MDY "[0-9]{1,2}/[0-9]{1,2}/[0-9]{2,4}"
#define NANOREGEX_ISO_DATETIME "[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}"

#ifdef __cplusplus
}
#endif

#endif // NANOREGEX_H