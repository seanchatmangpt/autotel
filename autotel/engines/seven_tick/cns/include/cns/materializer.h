#ifndef CNS_MATERIALIZER_H
#define CNS_MATERIALIZER_H

#include "cns/types.h"
#include "cns/arena.h"
#include "cns/interner.h"
#include "cns/graph.h"
#include "cns/binary_materializer.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// ============================================================================
// CNS MATERIALIZER - UNIFIED BINARY SERIALIZATION FOR 7T SUBSTRATE
// ============================================================================

// Unified materializer that integrates the high-performance binary materializer
// with the 7T substrate. Provides deterministic serialization/deserialization
// with 7-tick performance guarantees through zero-copy operations and 
// optimized memory layouts.

// ============================================================================
// MATERIALIZER MODES AND FORMATS
// ============================================================================

// Serialization formats supported by the materializer
typedef enum {
    CNS_FORMAT_BINARY = 0,           // CNS binary format (default)
    CNS_FORMAT_TURTLE,               // Turtle format
    CNS_FORMAT_NTRIPLES,             // N-Triples format
    CNS_FORMAT_NQUADS,               // N-Quads format
    CNS_FORMAT_JSONLD,               // JSON-LD format
    CNS_FORMAT_RDFXML,               // RDF/XML format
    CNS_FORMAT_TRIG,                 // TriG format
    CNS_FORMAT_COUNT                 // Number of formats
} cns_serialization_format_t;

// Serialization modes for different use cases
typedef enum {
    CNS_MODE_STREAM = 0,             // Streaming serialization
    CNS_MODE_BULK,                   // Bulk serialization
    CNS_MODE_INCREMENTAL,            // Incremental serialization
    CNS_MODE_ZERO_COPY,              // Zero-copy mode (binary only)
    CNS_MODE_COMPRESSED,             // Compressed serialization
    CNS_MODE_COUNT                   // Number of modes
} cns_serialization_mode_t;

// Compression algorithms supported
typedef enum {
    CNS_COMPRESSION_NONE = 0,        // No compression
    CNS_COMPRESSION_LZ4,             // LZ4 compression
    CNS_COMPRESSION_ZSTD,            // Zstandard compression
    CNS_COMPRESSION_GZIP,            // GZIP compression
    CNS_COMPRESSION_COUNT            // Number of compression types
} cns_compression_type_t;

// ============================================================================
// SERIALIZATION CONTEXT AND STATE
// ============================================================================

// Serialization state for tracking progress
typedef struct {
    size_t nodes_processed;          // Nodes processed so far
    size_t edges_processed;          // Edges processed so far
    size_t triples_processed;        // Triples processed so far
    size_t bytes_written;            // Bytes written to output
    uint32_t current_graph_id;       // Current named graph being processed
    bool in_collection;              // Currently processing RDF collection
    bool in_blank_node;              // Currently processing blank node
    uint32_t blank_node_depth;       // Nesting depth of blank nodes
} cns_serialization_state_t;

// Deserialization state for tracking progress
typedef struct {
    size_t bytes_read;               // Bytes read from input
    size_t nodes_created;            // Nodes created so far
    size_t edges_created;            // Edges created so far
    size_t triples_created;          // Triples created so far
    uint32_t current_graph_id;       // Current named graph being read
    const char *error_position;      // Position of last error (text formats)
    uint32_t error_line;             // Line number of last error
    uint32_t error_column;           // Column number of last error
} cns_deserialization_state_t;

// Serialization options for controlling output
typedef struct {
    cns_serialization_format_t format; // Output format
    cns_serialization_mode_t mode;   // Serialization mode
    cns_compression_type_t compression; // Compression algorithm
    bool pretty_print;               // Pretty-print text formats
    bool include_prefixes;           // Include prefix declarations
    bool use_relative_iris;          // Use relative IRIs where possible
    bool validate_output;            // Validate output after serialization
    const char *base_iri;            // Base IRI for relative references
    size_t buffer_size;              // Buffer size for streaming
    uint32_t compression_level;      // Compression level (0-9)
} cns_serialization_options_t;

// Deserialization options for controlling input processing
typedef struct {
    cns_serialization_format_t format; // Input format (auto-detect if unknown)
    bool strict_mode;                // Strict parsing mode
    bool validate_iris;              // Validate IRI syntax
    bool resolve_relative_iris;      // Resolve relative IRIs
    bool deduplicate_triples;        // Remove duplicate triples
    const char *base_iri;            // Base IRI for relative resolution
    size_t buffer_size;              // Buffer size for streaming
    bool enable_recovery;            // Enable error recovery
} cns_deserialization_options_t;

// ============================================================================
// MATERIALIZER STATISTICS
// ============================================================================

// Serialization/deserialization performance statistics
typedef struct {
    uint64_t serializations_performed; // Total serializations
    uint64_t deserializations_performed; // Total deserializations
    uint64_t bytes_serialized;       // Total bytes written
    uint64_t bytes_deserialized;     // Total bytes read
    uint64_t triples_serialized;     // Total triples written
    uint64_t triples_deserialized;   // Total triples read
    cns_tick_t total_serialize_ticks; // Total serialization time
    cns_tick_t total_deserialize_ticks; // Total deserialization time
    cns_tick_t avg_serialize_ticks;   // Average serialization time
    cns_tick_t avg_deserialize_ticks; // Average deserialization time
    cns_tick_t compression_ticks;     // Time spent compressing
    cns_tick_t decompression_ticks;   // Time spent decompressing
    size_t memory_usage;             // Current memory usage
    size_t peak_memory_usage;        // Peak memory usage
    double compression_ratio;        // Average compression ratio
} cns_materializer_stats_t;

// ============================================================================
// MAIN MATERIALIZER STRUCTURE
// ============================================================================

// Main materializer structure
struct cns_materializer {
    // Memory management
    cns_arena_t *buffer_arena;       // Arena for I/O buffers
    cns_arena_t *state_arena;        // Arena for state structures
    cns_interner_t *interner;        // String interner
    
    // Binary materializer integration
    cns_graph_t *binary_cache;       // Cache for binary operations
    cns_write_buffer_t *write_buffer; // Write buffer for binary format
    cns_read_buffer_t *read_buffer;   // Read buffer for binary format
    
    // Format-specific processors
    void *turtle_processor;          // Turtle format processor
    void *jsonld_processor;          // JSON-LD format processor
    void *xml_processor;             // RDF/XML format processor
    
    // Compression support
    void *compression_context;       // Compression context
    uint8_t *compression_buffer;     // Compression buffer
    size_t compression_buffer_size;  // Compression buffer size
    
    // Serialization state
    cns_serialization_state_t serialize_state; // Current serialization state
    cns_deserialization_state_t deserialize_state; // Current deserialization state
    
    // Configuration
    cns_serialization_options_t serialize_options; // Serialization options
    cns_deserialization_options_t deserialize_options; // Deserialization options
    
    // Performance tracking
    cns_materializer_stats_t stats;  // Performance statistics
    
    // Error handling
    char error_message[256];         // Last error message
    cns_result_t last_error;         // Last error code
    
    // Configuration flags
    uint32_t flags;                  // Materializer flags
    bool enable_caching;             // Enable result caching
    bool enable_validation;          // Enable output validation
    
    // Thread safety (if enabled)
    void *mutex;                     // Mutex for thread safety
    uint32_t magic;                  // Magic number for validation
};

// Materializer flags
#define CNS_MATERIALIZER_FLAG_ENABLE_CACHING    (1 << 0)  // Enable result caching
#define CNS_MATERIALIZER_FLAG_ENABLE_VALIDATION (1 << 1)  // Enable validation
#define CNS_MATERIALIZER_FLAG_ZERO_COPY         (1 << 2)  // Enable zero-copy mode
#define CNS_MATERIALIZER_FLAG_STREAMING         (1 << 3)  // Enable streaming mode
#define CNS_MATERIALIZER_FLAG_THREAD_SAFE       (1 << 4)  // Thread-safe operations
#define CNS_MATERIALIZER_FLAG_AUTO_COMPRESS     (1 << 5)  // Automatic compression
#define CNS_MATERIALIZER_FLAG_PRETTY_PRINT      (1 << 6)  // Pretty-print output

// Magic number for materializer validation
#define CNS_MATERIALIZER_MAGIC 0x4D415445  // 'MATE'

// ============================================================================
// MATERIALIZER LIFECYCLE FUNCTIONS - O(1) OPERATIONS
// ============================================================================

// Create a new materializer with specified configuration
// PERFORMANCE: O(1) - completes within 7 CPU ticks
cns_materializer_t* cns_materializer_create(const cns_materializer_config_t *config);

// Create materializer with default configuration
// PERFORMANCE: O(1) - optimized default settings
cns_materializer_t* cns_materializer_create_default(cns_arena_t *arena,
                                                   cns_interner_t *interner);

// Destroy materializer and free all memory
// PERFORMANCE: O(1) when using arenas - just resets arena pointers
void cns_materializer_destroy(cns_materializer_t *materializer);

// Reset materializer state for new operation
// PERFORMANCE: O(1) - resets state and buffers
cns_result_t cns_materializer_reset(cns_materializer_t *materializer);

// Clone materializer configuration (not state)
// PERFORMANCE: O(1) - copies configuration only
cns_materializer_t* cns_materializer_clone_config(const cns_materializer_t *materializer);

// ============================================================================
// SERIALIZATION FUNCTIONS - 7T PERFORMANCE
// ============================================================================

// Serialize RDF graph to buffer
// PERFORMANCE: O(n) where n is graph size, but 7T per element
cns_result_t cns_materializer_serialize_graph(cns_materializer_t *materializer,
                                             const cns_graph_t *graph,
                                             uint8_t **output_buffer,
                                             size_t *output_size);

// Serialize graph to file
// PERFORMANCE: O(n) where n is graph size
cns_result_t cns_materializer_serialize_to_file(cns_materializer_t *materializer,
                                               const cns_graph_t *graph,
                                               const char *filename);

// Serialize graph to stream
// PERFORMANCE: O(n) with streaming optimizations
cns_result_t cns_materializer_serialize_to_stream(cns_materializer_t *materializer,
                                                 const cns_graph_t *graph,
                                                 FILE *stream);

// Serialize single triple
// PERFORMANCE: O(1) - 7T guaranteed
cns_result_t cns_materializer_serialize_triple(cns_materializer_t *materializer,
                                              const cns_triple_t *triple,
                                              uint8_t **output_buffer,
                                              size_t *output_size);

// Begin streaming serialization
// PERFORMANCE: O(1) - initializes streaming state
cns_result_t cns_materializer_begin_serialize(cns_materializer_t *materializer,
                                             const cns_serialization_options_t *options);

// Serialize next batch of triples
// PERFORMANCE: O(k) where k is batch size
cns_result_t cns_materializer_serialize_batch(cns_materializer_t *materializer,
                                             const cns_triple_t *triples,
                                             size_t count,
                                             uint8_t **output_buffer,
                                             size_t *output_size);

// End streaming serialization
// PERFORMANCE: O(1) - finalizes output and cleanup
cns_result_t cns_materializer_end_serialize(cns_materializer_t *materializer,
                                           uint8_t **final_buffer,
                                           size_t *final_size);

// ============================================================================
// DESERIALIZATION FUNCTIONS - 7T PERFORMANCE
// ============================================================================

// Deserialize RDF data from buffer
// PERFORMANCE: O(n) where n is data size, but 7T per element
cns_result_t cns_materializer_deserialize_buffer(cns_materializer_t *materializer,
                                                const uint8_t *input_buffer,
                                                size_t input_size,
                                                cns_graph_t *graph);

// Deserialize RDF data from file
// PERFORMANCE: O(n) where n is file size
cns_result_t cns_materializer_deserialize_file(cns_materializer_t *materializer,
                                              const char *filename,
                                              cns_graph_t *graph);

// Deserialize RDF data from stream
// PERFORMANCE: O(n) with streaming optimizations
cns_result_t cns_materializer_deserialize_stream(cns_materializer_t *materializer,
                                                FILE *stream,
                                                cns_graph_t *graph);

// Begin streaming deserialization
// PERFORMANCE: O(1) - initializes streaming state
cns_result_t cns_materializer_begin_deserialize(cns_materializer_t *materializer,
                                               const cns_deserialization_options_t *options);

// Deserialize next chunk of data
// PERFORMANCE: O(k) where k is chunk size
cns_result_t cns_materializer_deserialize_chunk(cns_materializer_t *materializer,
                                               const uint8_t *chunk_data,
                                               size_t chunk_size,
                                               cns_graph_t *graph);

// End streaming deserialization
// PERFORMANCE: O(1) - finalizes parsing and cleanup
cns_result_t cns_materializer_end_deserialize(cns_materializer_t *materializer);

// ============================================================================
// FORMAT-SPECIFIC FUNCTIONS
// ============================================================================

// Auto-detect format from buffer
// PERFORMANCE: O(1) - checks magic bytes and headers
cns_serialization_format_t cns_materializer_detect_format(const uint8_t *buffer,
                                                         size_t size);

// Auto-detect format from filename
// PERFORMANCE: O(1) - checks file extension
cns_serialization_format_t cns_materializer_detect_format_from_filename(const char *filename);

// Convert between formats
// PERFORMANCE: O(n) - deserialize + serialize
cns_result_t cns_materializer_convert_format(cns_materializer_t *materializer,
                                            const uint8_t *input_buffer,
                                            size_t input_size,
                                            cns_serialization_format_t input_format,
                                            cns_serialization_format_t output_format,
                                            uint8_t **output_buffer,
                                            size_t *output_size);

// Validate serialized data
// PERFORMANCE: O(n) - full parse and validation
cns_result_t cns_materializer_validate_data(cns_materializer_t *materializer,
                                           const uint8_t *buffer,
                                           size_t size,
                                           cns_serialization_format_t format);

// ============================================================================
// COMPRESSION FUNCTIONS
// ============================================================================

// Compress serialized data
// PERFORMANCE: O(n) where n is data size
cns_result_t cns_materializer_compress(cns_materializer_t *materializer,
                                      const uint8_t *input_buffer,
                                      size_t input_size,
                                      cns_compression_type_t compression,
                                      uint8_t **output_buffer,
                                      size_t *output_size);

// Decompress serialized data
// PERFORMANCE: O(n) where n is compressed size
cns_result_t cns_materializer_decompress(cns_materializer_t *materializer,
                                        const uint8_t *compressed_buffer,
                                        size_t compressed_size,
                                        cns_compression_type_t compression,
                                        uint8_t **output_buffer,
                                        size_t *output_size);

// Get compression ratio
// PERFORMANCE: O(1) - simple calculation
double cns_materializer_compression_ratio(size_t original_size,
                                         size_t compressed_size);

// ============================================================================
// ZERO-COPY BINARY OPERATIONS
// ============================================================================

// Create zero-copy view of binary data
// PERFORMANCE: O(1) - creates view without copying
cns_result_t cns_materializer_create_view(cns_materializer_t *materializer,
                                         const uint8_t *buffer,
                                         size_t size,
                                         cns_graph_view_t *view);

// Map file for zero-copy access
// PERFORMANCE: O(1) - memory maps file
cns_result_t cns_materializer_map_file(cns_materializer_t *materializer,
                                      const char *filename,
                                      cns_graph_view_t *view);

// Access graph elements through view
// PERFORMANCE: O(1) - direct memory access
cns_result_t cns_materializer_view_get_triple(const cns_graph_view_t *view,
                                             uint32_t index,
                                             cns_triple_t *triple);

// Close zero-copy view
// PERFORMANCE: O(1) - unmaps memory
void cns_materializer_close_view(cns_graph_view_t *view);

// ============================================================================
// CONFIGURATION FUNCTIONS
// ============================================================================

// Set serialization options
// PERFORMANCE: O(1) - updates configuration
cns_result_t cns_materializer_set_serialize_options(cns_materializer_t *materializer,
                                                   const cns_serialization_options_t *options);

// Set deserialization options
// PERFORMANCE: O(1) - updates configuration
cns_result_t cns_materializer_set_deserialize_options(cns_materializer_t *materializer,
                                                     const cns_deserialization_options_t *options);

// Get serialization options
// PERFORMANCE: O(1) - returns current configuration
cns_result_t cns_materializer_get_serialize_options(const cns_materializer_t *materializer,
                                                   cns_serialization_options_t *options);

// Get deserialization options
// PERFORMANCE: O(1) - returns current configuration
cns_result_t cns_materializer_get_deserialize_options(const cns_materializer_t *materializer,
                                                     cns_deserialization_options_t *options);

// Set materializer flags
// PERFORMANCE: O(1) - updates flags
cns_result_t cns_materializer_set_flags(cns_materializer_t *materializer, uint32_t flags);

// Get materializer flags
// PERFORMANCE: O(1) - returns current flags
uint32_t cns_materializer_get_flags(const cns_materializer_t *materializer);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

// Convert format enum to string
// PERFORMANCE: O(1) - lookup table
const char* cns_materializer_format_string(cns_serialization_format_t format);

// Convert format string to enum
// PERFORMANCE: O(1) - hash lookup
cns_serialization_format_t cns_materializer_format_from_string(const char *format_str);

// Get format file extension
// PERFORMANCE: O(1) - lookup table
const char* cns_materializer_format_extension(cns_serialization_format_t format);

// Convert compression type to string
// PERFORMANCE: O(1) - lookup table
const char* cns_materializer_compression_string(cns_compression_type_t compression);

// Check if format supports compression
// PERFORMANCE: O(1) - simple check
bool cns_materializer_format_supports_compression(cns_serialization_format_t format);

// Check if format is text-based
// PERFORMANCE: O(1) - simple check
bool cns_materializer_format_is_text(cns_serialization_format_t format);

// ============================================================================
// INFORMATION AND STATISTICS
// ============================================================================

// Get materializer statistics
// PERFORMANCE: O(1) - returns cached statistics
cns_result_t cns_materializer_get_stats(const cns_materializer_t *materializer,
                                       cns_materializer_stats_t *stats);

// Get serialization state
// PERFORMANCE: O(1) - returns current state
cns_result_t cns_materializer_get_serialize_state(const cns_materializer_t *materializer,
                                                 cns_serialization_state_t *state);

// Get deserialization state
// PERFORMANCE: O(1) - returns current state
cns_result_t cns_materializer_get_deserialize_state(const cns_materializer_t *materializer,
                                                   cns_deserialization_state_t *state);

// Get memory usage
// PERFORMANCE: O(1) - returns cached value
size_t cns_materializer_memory_usage(const cns_materializer_t *materializer);

// Get last error
// PERFORMANCE: O(1) - returns last error info
cns_result_t cns_materializer_get_last_error(const cns_materializer_t *materializer,
                                            char **error_message);

// ============================================================================
// DEBUG AND VALIDATION FUNCTIONS
// ============================================================================

// Validate materializer integrity
// PERFORMANCE: O(1) - checks internal state
cns_result_t cns_materializer_validate(const cns_materializer_t *materializer);

// Print materializer statistics
// PERFORMANCE: O(1) - prints cached statistics
cns_result_t cns_materializer_print_stats(const cns_materializer_t *materializer, FILE *output);

// Dump serialization state
// PERFORMANCE: O(1) - prints current state
cns_result_t cns_materializer_dump_state(const cns_materializer_t *materializer, FILE *output);

// Benchmark serialization performance
// PERFORMANCE: O(n * k) where n is graph size, k is iterations
cns_result_t cns_materializer_benchmark(cns_materializer_t *materializer,
                                       const cns_graph_t *graph,
                                       cns_serialization_format_t format,
                                       uint32_t iterations,
                                       cns_materializer_stats_t *results);

// ============================================================================
// PERFORMANCE MONITORING INTEGRATION
// ============================================================================

// Performance callback for materializer events
typedef void (*cns_materializer_perf_callback_t)(const cns_materializer_t *materializer,
                                                const char *operation,
                                                size_t bytes_processed,
                                                cns_tick_t ticks,
                                                void *user_data);

// Set performance monitoring callback
// PERFORMANCE: O(1) - stores callback pointer
cns_result_t cns_materializer_set_perf_callback(cns_materializer_t *materializer,
                                               cns_materializer_perf_callback_t callback,
                                               void *user_data);

// Clear performance monitoring callback
// PERFORMANCE: O(1) - clears callback pointer
cns_result_t cns_materializer_clear_perf_callback(cns_materializer_t *materializer);

// ============================================================================
// THREAD SAFETY FUNCTIONS
// ============================================================================

// Enable thread safety for materializer
// PERFORMANCE: O(1) - initializes mutex
cns_result_t cns_materializer_enable_thread_safety(cns_materializer_t *materializer);

// Disable thread safety
// PERFORMANCE: O(1) - destroys mutex
cns_result_t cns_materializer_disable_thread_safety(cns_materializer_t *materializer);

// Lock materializer for exclusive access
// PERFORMANCE: O(1) - mutex lock
cns_result_t cns_materializer_lock(cns_materializer_t *materializer);

// Unlock materializer
// PERFORMANCE: O(1) - mutex unlock
cns_result_t cns_materializer_unlock(cns_materializer_t *materializer);

// ============================================================================
// UTILITY MACROS FOR COMMON PATTERNS
// ============================================================================

// Serialize and check for errors
#define CNS_MATERIALIZER_SERIALIZE_CHECK(materializer, graph, buffer_var, size_var) \
    do { \
        cns_result_t result = cns_materializer_serialize_graph((materializer), (graph), \
                                                              &(buffer_var), &(size_var)); \
        if (CNS_7T_UNLIKELY(result != CNS_OK)) return result; \
    } while(0)

// Deserialize and check for errors
#define CNS_MATERIALIZER_DESERIALIZE_CHECK(materializer, buffer, size, graph) \
    do { \
        cns_result_t result = cns_materializer_deserialize_buffer((materializer), (buffer), \
                                                                 (size), (graph)); \
        if (CNS_7T_UNLIKELY(result != CNS_OK)) return result; \
    } while(0)

// Convert format and check for errors
#define CNS_MATERIALIZER_CONVERT_CHECK(materializer, in_buf, in_size, in_fmt, out_fmt, out_buf_var, out_size_var) \
    do { \
        cns_result_t result = cns_materializer_convert_format((materializer), (in_buf), (in_size), \
                                                            (in_fmt), (out_fmt), &(out_buf_var), &(out_size_var)); \
        if (CNS_7T_UNLIKELY(result != CNS_OK)) return result; \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif // CNS_MATERIALIZER_H