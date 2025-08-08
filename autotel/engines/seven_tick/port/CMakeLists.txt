cmake_minimum_required(VERSION 3.16)
project(CNS VERSION 1.0.0 LANGUAGES C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler flags
set(CMAKE_C_FLAGS_RELEASE "-O3 -march=native -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g -Wall -Wextra")

# Find required packages
find_package(PkgConfig REQUIRED)

# Find jansson
pkg_check_modules(JANSSON REQUIRED jansson)

# Find OpenTelemetry (if available)
find_package(OpenTelemetry QUIET)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/chatman_nano_stack/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/chatman_nano_stack/include/cns)
include_directories(${GENERATED_DIR})

# ============================================================================
# AOT COMPILATION SYSTEM
# ============================================================================

# Create build directory and generated directory
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build)
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_DIR})

# Define directories
set(ONTOLOGY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/docs/ontology)
set(SQL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/examples/sql)

# Define the list of files our AOT compiler will create
set(GENERATED_HEADERS
    ${GENERATED_DIR}/ontology_ids.h
    ${GENERATED_DIR}/ontology_rules.h
    ${GENERATED_DIR}/shacl_validators.h
    ${GENERATED_DIR}/sql_queries.h
)

# AOT Compilation Step - runs the consolidated compiler
add_custom_command(
    OUTPUT ${GENERATED_HEADERS}
    COMMAND python3 ${CMAKE_CURRENT_SOURCE_DIR}/codegen/aot_compiler.py
            --ontologies ${ONTOLOGY_DIR}
            --sql ${SQL_DIR}
            --output ${GENERATED_DIR}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/codegen/aot_compiler.py
            ${ONTOLOGY_DIR}/cns-core.ttl
    COMMENT "Running AOT Compiler to generate C headers from ontology and SQL..."
)

# Create AOT compile target
add_custom_target(aot_compile DEPENDS ${GENERATED_HEADERS})

# Extract spans from TTL (legacy weaver support)
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/build/cns_spans.json
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}
            python3 ${CMAKE_CURRENT_SOURCE_DIR}/codegen/extract_spans.py
            ${CMAKE_CURRENT_SOURCE_DIR}/docs/ontology/cns-core.ttl
            ${CMAKE_CURRENT_BINARY_DIR}/build/cns_spans.json
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/docs/ontology/cns-core.ttl
            ${CMAKE_CURRENT_SOURCE_DIR}/codegen/extract_spans.py
    COMMENT "Extracting spans from TTL ontology"
)

# Build weaver executable
add_executable(cns_weaver 
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen/weaver_main.c
)

target_include_directories(cns_weaver PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${JANSSON_INCLUDE_DIRS}
)

target_link_libraries(cns_weaver PRIVATE 
    ${JANSSON_LIBRARIES}
    # Note: cjinja would be linked here if available
)

# Generate OTEL files using weaver
add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/src/cns_otel.h
           ${CMAKE_CURRENT_SOURCE_DIR}/src/cns_otel_inject.c
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/cns_weaver
            ${CMAKE_CURRENT_BINARY_DIR}/build/cns_spans.json
            ${CMAKE_CURRENT_SOURCE_DIR}/templates/otel_header.h.j2
            ${CMAKE_CURRENT_SOURCE_DIR}/templates/otel_inject.c.j2
            ${CMAKE_CURRENT_SOURCE_DIR}/src
            cns.h
    DEPENDS cns_weaver
            ${CMAKE_CURRENT_BINARY_DIR}/build/cns_spans.json
            ${CMAKE_CURRENT_SOURCE_DIR}/templates/otel_header.h.j2
            ${CMAKE_CURRENT_SOURCE_DIR}/templates/otel_inject.c.j2
    COMMENT "Generating OpenTelemetry instrumentation code"
)

# Create weave target
add_custom_target(weave ALL DEPENDS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cns_otel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cns_otel_inject.c
)

# ============================================================================
# CNS LIBRARY
# ============================================================================

# Source files
set(CNS_SOURCES
    src/cns_main.c
    src/cns_parser.c
    src/cns.c
    src/core/cli.c
    src/domains/bench.c
    src/domains/build.c
    src/domains/dashboard.c
    src/domains/sql/sql_domain.c
    src/domains/sql/sql_parser.c
    src/utils/memory.c
    src/utils/perf.c
    src/utils/telemetry.c
    chatman_nano_stack/topology_lifter.c
    chatman_nano_stack/conductor_manifest.c
    registry.c
    src/entanglement_oracle.c
    chatman_nano_stack/meta_probe.c
    chatman_nano_stack/bitactor_core.c
    chatman_nano_stack/main.c
    chatman_nano_stack/dark_feature_activator.c
    chatman_nano_stack/domains/deploy.c
    chatman_nano_stack/domains/docs.c
    chatman_nano_stack/examples/nano_stack_demo.c
    chatman_nano_stack/examples/pattern_composition.c
    chatman_nano_stack/examples/s7t_demo.c
    chatman_nano_stack/src/bitactor_ttl_compiler.c
    chatman_nano_stack/src/gatekeeper.c
    chatman_nano_stack/validation/cns_fifth_epoch_permutation_test.c
    chatman_nano_stack/validation/fifth_epoch_demo.c
    chatman_nano_stack/validation/fifth_epoch_enhanced_demo.c
    chatman_nano_stack/src/bitactor_80_20.c
    chatman_nano_stack/src/bitactor_80_20_optimized.c
    chatman_nano_stack/src/bitactor_bridge.c
    chatman_nano_stack/src/bitactor_l2_l3_production.c
    chatman_nano_stack/src/bitactor_l_stack_demo.c
    chatman_nano_stack/src/bitactor_ls.c
    chatman_nano_stack/src/l7_entanglement_demo.c
    chatman_nano_stack/tests/bitactor_80_20_test.c
    chatman_nano_stack/tests/bitactor_ls_test.c
    chatman_nano_stack/chatman_nano_ring.c
    chatman_nano_stack/chatman_signal_decoder.c
    chatman_nano_stack/chatman_fastbus.c
    chatman_nano_stack/chatman_event_loop.c
    chatman_nano_stack/chatman_topology_lifter.c
)

# Create CNS library
add_library(cns_static STATIC ${CNS_SOURCES})

target_include_directories(cns_static PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${JANSSON_INCLUDE_DIRS}
)

target_link_libraries(cns_static PUBLIC 
    ${JANSSON_LIBRARIES}
)

if(OpenTelemetry_FOUND)
    target_link_libraries(cns_static PUBLIC OpenTelemetry::OpenTelemetry)
endif()

# Depend on AOT compilation and weave target
add_dependencies(cns_static aot_compile weave)

# ============================================================================
# GATEKEEPER
# ============================================================================

# Build gatekeeper executable
add_executable(gatekeeper 
    ${CMAKE_CURRENT_SOURCE_DIR}/chatman_nano_stack/src/gatekeeper.c
)

target_link_libraries(gatekeeper PRIVATE 
    cns_static
    m
)

# Depend on AOT compilation and weave target
add_dependencies(gatekeeper aot_compile weave)

# ============================================================================
# COMMAND LINE INTERFACE
# ============================================================================

# Build CNS CLI
add_executable(cns 
    ${CMAKE_CURRENT_SOURCE_DIR}/chatman_nano_stack/main.c
)

target_link_libraries(cns PRIVATE 
    cns_static
    m
)

# Depend on AOT compilation and weave target
add_dependencies(cns aot_compile weave)

# ============================================================================
# BENCHMARKS
# ============================================================================

# Build benchmark executable
add_executable(cns_bench 
    ${CMAKE_CURRENT_SOURCE_DIR}/chatman_nano_stack/src/cmd_benchmark.c
)

target_link_libraries(cns_bench PRIVATE 
    cns_static
    m
)

# Depend on AOT compilation and weave target
add_dependencies(cns_bench aot_compile weave)

# ============================================================================
# TESTS
# ============================================================================

# Enable testing
enable_testing()

# Add test executable
add_executable(cns_test 
    ${CMAKE_CURRENT_SOURCE_DIR}/chatman_nano_stack/tests/test_cns.c
)

target_link_libraries(cns_test PRIVATE 
    cns_static
    m
)

# Depend on AOT compilation and weave target
add_dependencies(cns_test aot_compile weave)

# Add test
add_test(NAME CNSUnitTest COMMAND cns_test)

# ============================================================================
# INSTALLATION
# ============================================================================

# Install headers
install(DIRECTORY chatman_nano_stack/include/ DESTINATION include/cns)

# Install library
install(TARGETS cns_static
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
)

# Install executables
install(TARGETS cns gatekeeper cns_bench
    RUNTIME DESTINATION bin
)

# ============================================================================
# CUSTOM TARGETS
# ============================================================================

# Target to run gatekeeper
add_custom_target(gatekeeper-run
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/gatekeeper
    DEPENDS gatekeeper
    COMMENT "Running CNS Gatekeeper"
)

# Target to run benchmarks
add_custom_target(benchmark
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/cns_bench
    DEPENDS cns_bench
    COMMENT "Running CNS Benchmarks"
)

# Target to extract spans only
add_custom_target(extract-spans
    COMMAND ${CMAKE_COMMAND} -E env PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}
            python3 ${CMAKE_CURRENT_SOURCE_DIR}/codegen/extract_spans.py
            ${CMAKE_CURRENT_SOURCE_DIR}/docs/ontology/cns-core.ttl
            ${CMAKE_CURRENT_BINARY_DIR}/build/cns_spans.json
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/docs/ontology/cns-core.ttl
            ${CMAKE_CURRENT_SOURCE_DIR}/codegen/extract_spans.py
    COMMENT "Extracting spans from TTL ontology"
)

# ============================================================================
# DOCUMENTATION
# ============================================================================

# Print configuration summary
message(STATUS "CNS Configuration:")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  C flags: ${CMAKE_C_FLAGS}")
message(STATUS "  Jansson found: ${JANSSON_FOUND}")
message(STATUS "  OpenTelemetry found: ${OpenTelemetry_FOUND}")
message(STATUS "  Source directory: ${CMAKE_CURRENT_SOURCE_DIR}")
message(STATUS "  Binary directory: ${CMAKE_CURRENT_BINARY_DIR}") 