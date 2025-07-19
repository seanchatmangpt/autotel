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
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# ============================================================================
# WEAVER SYSTEM
# ============================================================================

# Create build directory
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/build)

# Extract spans from TTL
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

# Depend on weave target
add_dependencies(cns_static weave)

# ============================================================================
# GATEKEEPER
# ============================================================================

# Build gatekeeper executable
add_executable(gatekeeper 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gatekeeper.c
)

target_link_libraries(gatekeeper PRIVATE 
    cns_static
    m
)

# Depend on weave target
add_dependencies(gatekeeper weave)

# ============================================================================
# COMMAND LINE INTERFACE
# ============================================================================

# Build CNS CLI
add_executable(cns 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.c
)

target_link_libraries(cns PRIVATE 
    cns_static
    m
)

# Depend on weave target
add_dependencies(cns weave)

# ============================================================================
# BENCHMARKS
# ============================================================================

# Build benchmark executable
add_executable(cns_bench 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/cmd_benchmark.c
)

target_link_libraries(cns_bench PRIVATE 
    cns_static
    m
)

# Depend on weave target
add_dependencies(cns_bench weave)

# ============================================================================
# TESTS
# ============================================================================

# Enable testing
enable_testing()

# Add test executable
add_executable(cns_test 
    ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_cns.c
)

target_link_libraries(cns_test PRIVATE 
    cns_static
    m
)

# Depend on weave target
add_dependencies(cns_test weave)

# Add test
add_test(NAME CNSUnitTest COMMAND cns_test)

# ============================================================================
# INSTALLATION
# ============================================================================

# Install headers
install(DIRECTORY include/ DESTINATION include/cns)

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