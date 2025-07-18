# 7T Engine Troubleshooting Guide

## Overview

This guide provides comprehensive troubleshooting solutions for common issues encountered when using the 7T Engine. It covers build problems, runtime errors, performance issues, and deployment challenges.

## Quick Diagnostic Commands

### System Health Check
```bash
# Check system resources
free -h                    # Memory usage
df -h                      # Disk space
nproc                      # CPU cores
lscpu                      # CPU details

# Check kernel parameters
sysctl vm.swappiness       # Memory management
sysctl vm.dirty_ratio      # Write buffer settings

# Check process limits
ulimit -a                  # Resource limits
```

### Engine Health Check
```bash
# Basic functionality test
./verification/sparql_simple_test

# Performance benchmark
./verification/seven_tick_benchmark

# Memory leak detection
valgrind --leak-check=full ./verification/sparql_simple_test
```

## Build Issues

### Compilation Errors

#### Missing Dependencies
```bash
# Error: "fatal error: 'sparql7t.h' file not found"
# Solution: Check include paths
cc -I./c_src -I./runtime/src -o test test.c c_src/sparql7t.c

# Error: "undefined reference to 's7t_create'"
# Solution: Link all required source files
cc -o test test.c c_src/sparql7t.c runtime/src/seven_t_runtime.c
```

#### Architecture-Specific Issues
```bash
# ARM64 optimization issues
cc -march=native -O3 -o test test.c

# x86_64 compatibility issues
cc -march=x86-64 -O3 -o test test.c

# Cross-compilation issues
# Use appropriate toolchain
aarch64-linux-gnu-gcc -o test test.c
```

#### Compiler Version Issues
```bash
# Check compiler version
gcc --version
clang --version

# Minimum requirements
# gcc >= 9.0, clang >= 9.0

# Update compiler if needed
sudo apt update && sudo apt install gcc-10
```

### Linker Errors

#### Undefined Symbols
```bash
# Error: "undefined reference to 'main'"
# Solution: Ensure main function exists
int main() {
    // Your code here
    return 0;
}

# Error: "undefined reference to 's7t_create'"
# Solution: Include implementation file
cc -o test test.c c_src/sparql7t.c
```

#### Library Linking Issues
```bash
# Error: "cannot find -l7t_runtime"
# Solution: Check library path
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
sudo ldconfig

# Error: "wrong ELF class: ELFCLASS64"
# Solution: Match architecture
file ./test
# Ensure 64-bit compilation for 64-bit systems
```

## Runtime Issues

### Memory Problems

#### Memory Allocation Failures
```c
// Error: "Failed to create engine"
// Solution: Check memory availability
S7TEngine* engine = s7t_create(1000000, 1000, 1000000);
if (!engine) {
    fprintf(stderr, "Memory allocation failed. Available memory: %zu MB\n", 
            get_available_memory() / (1024 * 1024));
    return 1;
}

// Calculate memory requirements
size_t required_memory = calculate_memory_requirements(max_s, max_p, max_o);
printf("Required memory: %.1f MB\n", required_memory / (1024.0 * 1024.0));
```

#### Memory Leaks
```bash
# Detect memory leaks
valgrind --leak-check=full --show-leak-kinds=all ./verification/sparql_simple_test

# Common leak patterns:
# 1. Missing s7t_destroy() calls
# 2. Unfreed ObjectNode lists
# 3. Unfreed string allocations

# Fix: Always call cleanup functions
S7TEngine* engine = s7t_create(1000, 100, 1000);
// ... use engine ...
s7t_destroy(engine);  // Always call this
```

#### Out of Memory Errors
```bash
# Error: "Cannot allocate memory"
# Solution: Reduce engine size or increase system memory

# Check available memory
free -h
cat /proc/meminfo | grep MemAvailable

# Use smaller engine size
S7TEngine* engine = s7t_create(100000, 100, 100000);  // Smaller sizes
```

### Segmentation Faults

#### Null Pointer Dereference
```c
// Error: Segmentation fault in s7t_ask_pattern
// Solution: Add null checks
int s7t_ask_pattern_safe(S7TEngine* engine, uint32_t s, uint32_t p, uint32_t o) {
    if (!engine) {
        fprintf(stderr, "Engine is NULL\n");
        return 0;
    }
    
    if (s >= engine->max_subjects || p >= engine->max_predicates) {
        fprintf(stderr, "Invalid parameters: s=%u, p=%u\n", s, p);
        return 0;
    }
    
    return s7t_ask_pattern(engine, s, p, o);
}
```

#### Buffer Overflow
```c
// Error: Buffer overflow in bit vector access
// Solution: Validate array bounds
size_t chunk = s / 64;
if (chunk >= engine->stride_len) {
    fprintf(stderr, "Chunk index out of bounds: %zu >= %zu\n", 
            chunk, engine->stride_len);
    return 0;
}
```

#### Stack Overflow
```bash
# Error: Stack overflow in recursive functions
# Solution: Increase stack size
ulimit -s 16384  # Increase stack to 16MB

# Or use iterative approach instead of recursive
```

### Performance Issues

#### Slow Pattern Matching
```bash
# Check if performance meets targets
./verification/seven_tick_benchmark

# Expected performance:
# - Pattern matching: <10 ns
# - Batch operations: <10 ns per pattern
# - Throughput: 100M+ patterns/sec

# If performance is slow:
# 1. Check compiler optimizations
cc -O3 -march=native -o test test.c

# 2. Check CPU governor
cat /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
# Should be "performance"

# 3. Check cache efficiency
perf stat -e cache-misses ./verification/seven_tick_benchmark
```

#### High Memory Usage
```bash
# Monitor memory usage
watch -n 1 'ps aux | grep seven-tick'

# Check memory per triple
# Target: <100 bytes per triple
# If higher, check for:
# 1. Sparse data structures
# 2. Inefficient bit vector usage
# 3. Memory fragmentation
```

#### CPU Bottlenecks
```bash
# Profile CPU usage
perf record -g ./verification/seven_tick_benchmark
perf report

# Check for:
# 1. Cache misses
perf stat -e cache-misses,cache-references ./verification/seven_tick_benchmark

# 2. Branch mispredictions
perf stat -e branch-misses ./verification/seven_tick_benchmark

# 3. Instruction cache misses
perf stat -e iTLB-load-misses ./verification/seven_tick_benchmark
```

## Data Issues

### Incorrect Pattern Matching Results

#### Multiple Objects Not Working
```c
// Problem: Multiple objects per (predicate, subject) not working
// Solution: Check ObjectNode implementation

// Test multiple objects
s7t_add_triple(engine, 1, 2, 3);
s7t_add_triple(engine, 1, 2, 4);  // Same subject, predicate

// Both should return 1
assert(s7t_ask_pattern(engine, 1, 2, 3) == 1);
assert(s7t_ask_pattern(engine, 1, 2, 4) == 1);
```

#### Batch Operations Failing
```c
// Problem: Batch operations return incorrect results
// Solution: Check batch implementation

TriplePattern patterns[4] = {
    {1, 2, 3}, {1, 2, 4}, {2, 2, 3}, {1, 2, 5}
};
int results[4];
s7t_ask_batch(engine, patterns, results, 4);

// Verify results
printf("Batch results: [%d, %d, %d, %d]\n", 
       results[0], results[1], results[2], results[3]);
// Expected: [1, 1, 1, 0]
```

### Data Corruption

#### Bit Vector Corruption
```c
// Problem: Bit vectors contain incorrect data
// Solution: Validate bit vector operations

// Check bit vector integrity
void validate_bit_vectors(S7TEngine* engine) {
    for (size_t p = 0; p < engine->max_predicates; p++) {
        for (size_t chunk = 0; chunk < engine->stride_len; chunk++) {
            uint64_t word = engine->predicate_vectors[p * engine->stride_len + chunk];
            // Check for unexpected patterns
            if (word != 0 && word != UINT64_MAX) {
                printf("Suspicious bit pattern at p=%zu, chunk=%zu: %lx\n", 
                       p, chunk, word);
            }
        }
    }
}
```

#### Object List Corruption
```c
// Problem: Object lists contain incorrect data
// Solution: Validate linked list integrity

void validate_object_lists(S7TEngine* engine) {
    for (size_t p = 0; p < engine->max_predicates; p++) {
        for (size_t s = 0; s < engine->max_subjects; s++) {
            ObjectNode* head = engine->ps_to_o_index[p * engine->max_subjects + s];
            ObjectNode* current = head;
            size_t count = 0;
            
            while (current && count < 1000) {  // Prevent infinite loops
                if (current->object >= engine->max_objects) {
                    printf("Invalid object ID: %u >= %zu\n", 
                           current->object, engine->max_objects);
                }
                current = current->next;
                count++;
            }
            
            if (count >= 1000) {
                printf("Possible infinite loop in object list\n");
            }
        }
    }
}
```

## Deployment Issues

### Container Issues

#### Docker Build Failures
```dockerfile
# Problem: Docker build fails
# Solution: Use multi-stage build

FROM ubuntu:20.04 AS builder
RUN apt-get update && apt-get install -y build-essential
COPY . /src
WORKDIR /src
RUN make clean && make

FROM ubuntu:20.04
COPY --from=builder /src/verification/sparql_simple_test /app/
COPY --from=builder /src/lib/lib7t_runtime.so /usr/local/lib/
RUN ldconfig
WORKDIR /app
CMD ["./sparql_simple_test"]
```

#### Container Runtime Issues
```bash
# Problem: Container crashes on startup
# Solution: Check container logs and resource limits

# Check container logs
docker logs <container_id>

# Check resource limits
docker run --memory=4g --cpus=2 seven-tick

# Check file permissions
docker run -v /host/path:/container/path seven-tick
```

### Kubernetes Issues

#### Pod Startup Failures
```yaml
# Problem: Pod fails to start
# Solution: Add proper health checks and resource limits

apiVersion: apps/v1
kind: Deployment
metadata:
  name: seven-tick
spec:
  replicas: 3
  selector:
    matchLabels:
      app: seven-tick
  template:
    metadata:
      labels:
        app: seven-tick
    spec:
      containers:
      - name: seven-tick
        image: seven-tick:latest
        ports:
        - containerPort: 8080
        resources:
          requests:
            memory: "4Gi"
            cpu: "2"
          limits:
            memory: "8Gi"
            cpu: "4"
        livenessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 30
          periodSeconds: 10
        readinessProbe:
          httpGet:
            path: /ready
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 5
```

#### Service Discovery Issues
```yaml
# Problem: Services can't communicate
# Solution: Check service configuration

apiVersion: v1
kind: Service
metadata:
  name: seven-tick-service
spec:
  selector:
    app: seven-tick
  ports:
  - port: 80
    targetPort: 8080
  type: LoadBalancer
```

### Network Issues

#### Connection Refused
```bash
# Problem: "Connection refused" errors
# Solution: Check network configuration

# Check if service is listening
netstat -tlnp | grep 8080
ss -tlnp | grep 8080

# Check firewall rules
iptables -L -n
ufw status

# Check service status
systemctl status seven-tick
```

#### High Latency
```bash
# Problem: High network latency
# Solution: Optimize network configuration

# Check network latency
ping <target_host>
traceroute <target_host>

# Check network bandwidth
iperf3 -c <target_host>

# Optimize network settings
echo 'net.core.rmem_max = 16777216' >> /etc/sysctl.conf
echo 'net.core.wmem_max = 16777216' >> /etc/sysctl.conf
sysctl -p
```

## Debugging Tools

### GDB Debugging
```bash
# Compile with debug symbols
cc -g -O0 -o test test.c c_src/sparql7t.c

# Start GDB
gdb ./test

# Common GDB commands
(gdb) break s7t_ask_pattern
(gdb) run
(gdb) print engine
(gdb) print engine->max_subjects
(gdb) continue
(gdb) bt  # Backtrace
```

### Valgrind Debugging
```bash
# Memory error detection
valgrind --tool=memcheck --track-origins=yes ./verification/sparql_simple_test

# Memory leak detection
valgrind --leak-check=full --show-leak-kinds=all ./verification/sparql_simple_test

# Callgrind profiling
valgrind --tool=callgrind ./verification/seven_tick_benchmark
kcachegrind callgrind.out.*
```

### Perf Profiling
```bash
# CPU profiling
perf record -g ./verification/seven_tick_benchmark
perf report

# Cache profiling
perf stat -e cache-misses,cache-references ./verification/seven_tick_benchmark

# Branch profiling
perf stat -e branch-misses,branch-instructions ./verification/seven_tick_benchmark
```

### AddressSanitizer
```bash
# Compile with AddressSanitizer
cc -fsanitize=address -g -O1 -o test test.c

# Run with AddressSanitizer
ASAN_OPTIONS=detect_leaks=1:abort_on_error=1 ./test
```

## Common Error Messages

### Build Errors
```
error: 's7t_create' undeclared
→ Include the header file: #include "c_src/sparql7t.h"

error: undefined reference to 's7t_create'
→ Link the implementation: cc test.c c_src/sparql7t.c

error: cannot find -l7t_runtime
→ Check library path: export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
```

### Runtime Errors
```
Segmentation fault
→ Add null pointer checks and validate parameters

Memory allocation failed
→ Reduce engine size or increase system memory

Pattern matching returns incorrect results
→ Check multiple objects implementation
```

### Performance Warnings
```
Performance below target (<10 ns)
→ Check compiler optimizations and CPU governor

Memory usage high (>100 bytes per triple)
→ Check data structure efficiency

Throughput below target (<100M patterns/sec)
→ Profile for bottlenecks
```

## Prevention Strategies

### Code Quality
```c
// Always validate parameters
int s7t_operation_safe(S7TEngine* engine, uint32_t param) {
    if (!engine) return S7T_ERROR_NULL_POINTER;
    if (param >= engine->max_subjects) return S7T_ERROR_INVALID_PARAMETER;
    return s7t_operation(engine, param);
}

// Always cleanup resources
void example_usage() {
    S7TEngine* engine = s7t_create(1000, 100, 1000);
    if (!engine) return;
    
    // Use engine
    s7t_add_triple(engine, 1, 2, 3);
    
    // Always cleanup
    s7t_destroy(engine);
}
```

### Testing
```bash
# Run comprehensive tests
make test

# Run performance tests
make performance_test

# Run memory tests
make memory_test

# Run integration tests
make integration_test
```

### Monitoring
```bash
# Monitor system resources
htop
iotop
nethogs

# Monitor application metrics
curl http://localhost:8080/metrics

# Monitor logs
tail -f /var/log/seven-tick.log
```

## Getting Help

### Self-Diagnosis
1. **Check logs** for error messages
2. **Run diagnostic tests** to isolate issues
3. **Use debugging tools** to identify root cause
4. **Check documentation** for known issues

### Community Support
- **GitHub Issues**: Report bugs and request features
- **Discussions**: Community Q&A and troubleshooting
- **Documentation**: Comprehensive guides and examples

### Professional Support
- **Performance Consulting**: Optimization and tuning
- **Deployment Support**: Production deployment assistance
- **Training**: System administration and development training

## Emergency Procedures

### System Recovery
```bash
# Stop all services
sudo systemctl stop seven-tick

# Check system resources
free -h
df -h

# Restart services
sudo systemctl start seven-tick

# Verify health
curl http://localhost:8080/health
```

### Data Recovery
```bash
# Restore from backup
./restore_from_snapshot.sh backup_file

# Validate data integrity
./validate_data_integrity.sh

# Restart with recovered data
sudo systemctl restart seven-tick
```

### Performance Emergency
```bash
# Reduce load
# 1. Scale down replicas
kubectl scale deployment seven-tick --replicas=1

# 2. Increase resources
kubectl patch deployment seven-tick -p '{"spec":{"template":{"spec":{"containers":[{"name":"seven-tick","resources":{"requests":{"memory":"8Gi","cpu":"4"}}}]}}}}'

# 3. Monitor performance
watch -n 1 'curl -s http://localhost:8080/metrics | grep throughput'
``` 