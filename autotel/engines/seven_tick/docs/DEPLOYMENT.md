# 7T Engine Deployment Guide

## Overview

This guide covers deploying the 7T Engine in production environments, including system requirements, installation, configuration, monitoring, and scaling strategies.

## System Requirements

### Hardware Requirements

#### Minimum Requirements
- **CPU**: 64-bit processor with SIMD support
- **Memory**: 4GB RAM
- **Storage**: 10GB available space
- **Network**: 1Gbps Ethernet

#### Recommended Requirements
- **CPU**: ARM64 or x86_64 with 8+ cores
- **Memory**: 32GB+ RAM
- **Storage**: NVMe SSD with 100GB+ space
- **Network**: 10Gbps Ethernet

#### Production Requirements
- **CPU**: High-frequency processors (3GHz+)
- **Memory**: 128GB+ RAM for large datasets
- **Storage**: High-performance storage array
- **Network**: Low-latency network infrastructure

### Software Requirements

#### Operating System
- **Linux**: Ubuntu 20.04+, CentOS 8+, RHEL 8+
- **macOS**: 10.15+ (development only)
- **Windows**: WSL2 (development only)

#### Dependencies
```bash
# Essential build tools
gcc/clang 9.0+
make 4.0+
cmake 3.16+ (optional)

# Development libraries
libc6-dev
build-essential

# Optional dependencies
valgrind (for memory profiling)
perf (for performance profiling)
```

## Installation

### Building from Source

#### 1. Clone Repository
```bash
git clone https://github.com/autotel/seven_tick.git
cd seven_tick
```

#### 2. Build System
```bash
# Clean build
make clean

# Build with optimizations
make

# Verify build
./verification/sparql_simple_test
./verification/seven_tick_benchmark
```

#### 3. Install Libraries
```bash
# Install runtime library
sudo cp lib/lib7t_runtime.so /usr/local/lib/
sudo ldconfig

# Install headers (optional)
sudo cp c_src/*.h /usr/local/include/7t/
sudo cp runtime/src/*.h /usr/local/include/7t/
```

### Package Installation

#### Ubuntu/Debian
```bash
# Add repository (when available)
sudo apt update
sudo apt install seven-tick-engine

# Verify installation
seven-tick-benchmark
```

#### CentOS/RHEL
```bash
# Add repository (when available)
sudo yum install seven-tick-engine

# Verify installation
seven-tick-benchmark
```

## Configuration

### Engine Configuration

#### Basic Configuration
```c
// Engine creation with appropriate sizes
S7TEngine* engine = s7t_create(
    1000000,  // max_subjects
    10000,    // max_predicates
    1000000   // max_objects
);
```

#### Memory Configuration
```c
// Calculate memory requirements
size_t predicate_memory = max_predicates * stride_len * sizeof(uint64_t);
size_t object_memory = max_objects * stride_len * sizeof(uint64_t);
size_t index_memory = max_predicates * max_subjects * sizeof(ObjectNode*);

// Total memory estimate
size_t total_memory = predicate_memory + object_memory + index_memory;
printf("Estimated memory: %.1f MB\n", total_memory / (1024.0 * 1024.0));
```

#### Performance Configuration
```c
// Compiler flags for production
CFLAGS="-O3 -march=native -DNDEBUG -fPIC"

// Runtime optimizations
#define HASH_TABLE_SIZE 16384  // Optimize for dataset size
#define BATCH_SIZE 4           // SIMD batch size
```

### System Configuration

#### Kernel Parameters
```bash
# Optimize for high-performance computing
echo 'vm.swappiness=1' >> /etc/sysctl.conf
echo 'vm.dirty_ratio=15' >> /etc/sysctl.conf
echo 'vm.dirty_background_ratio=5' >> /etc/sysctl.conf

# Apply changes
sudo sysctl -p
```

#### CPU Governor
```bash
# Set performance governor
echo performance | sudo tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor

# Make permanent
echo 'GOVERNOR="performance"' | sudo tee -a /etc/default/cpufrequtils
```

#### Memory Configuration
```bash
# Enable huge pages
echo 'vm.nr_hugepages=1024' >> /etc/sysctl.conf

# Set memory limits
echo '* soft memlock unlimited' >> /etc/security/limits.conf
echo '* hard memlock unlimited' >> /etc/security/limits.conf
```

## Deployment Strategies

### Single-Node Deployment

#### Basic Setup
```bash
#!/bin/bash
# deploy_single_node.sh

# Build and install
make clean && make

# Create data directory
mkdir -p /var/lib/seven-tick/data

# Start engine service
./bin/seven-tick-engine \
    --data-dir /var/lib/seven-tick/data \
    --max-subjects 1000000 \
    --max-predicates 10000 \
    --max-objects 1000000 \
    --port 8080
```

#### Service Configuration
```ini
# /etc/systemd/system/seven-tick.service
[Unit]
Description=7T Engine Service
After=network.target

[Service]
Type=simple
User=seven-tick
Group=seven-tick
WorkingDirectory=/opt/seven-tick
ExecStart=/opt/seven-tick/bin/seven-tick-engine
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

### Multi-Node Deployment

#### Load Balancer Configuration
```nginx
# nginx.conf
upstream seven_tick_backend {
    server 192.168.1.10:8080;
    server 192.168.1.11:8080;
    server 192.168.1.12:8080;
}

server {
    listen 80;
    server_name seven-tick.example.com;
    
    location / {
        proxy_pass http://seven_tick_backend;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }
}
```

#### Cluster Configuration
```yaml
# cluster-config.yaml
nodes:
  - host: node1.example.com
    port: 8080
    max_subjects: 1000000
    max_predicates: 10000
    max_objects: 1000000
    
  - host: node2.example.com
    port: 8080
    max_subjects: 1000000
    max_predicates: 10000
    max_objects: 1000000
    
  - host: node3.example.com
    port: 8080
    max_subjects: 1000000
    max_predicates: 10000
    max_objects: 1000000

load_balancer:
  algorithm: round_robin
  health_check: /health
  timeout: 30s
```

### Container Deployment

#### Docker Configuration
```dockerfile
# Dockerfile
FROM ubuntu:20.04

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    libc6-dev \
    && rm -rf /var/lib/apt/lists/*

# Copy source
COPY . /opt/seven-tick
WORKDIR /opt/seven-tick

# Build
RUN make clean && make

# Create user
RUN useradd -r -s /bin/false seven-tick

# Create data directory
RUN mkdir -p /var/lib/seven-tick/data && \
    chown seven-tick:seven-tick /var/lib/seven-tick/data

# Expose port
EXPOSE 8080

# Run service
USER seven-tick
CMD ["./bin/seven-tick-engine", "--data-dir", "/var/lib/seven-tick/data"]
```

#### Docker Compose
```yaml
# docker-compose.yml
version: '3.8'

services:
  seven-tick:
    build: .
    ports:
      - "8080:8080"
    volumes:
      - seven_tick_data:/var/lib/seven-tick/data
    environment:
      - MAX_SUBJECTS=1000000
      - MAX_PREDICATES=10000
      - MAX_OBJECTS=1000000
    restart: unless-stopped

volumes:
  seven_tick_data:
```

#### Kubernetes Deployment
```yaml
# k8s-deployment.yaml
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
        env:
        - name: MAX_SUBJECTS
          value: "1000000"
        - name: MAX_PREDICATES
          value: "10000"
        - name: MAX_OBJECTS
          value: "1000000"
        resources:
          requests:
            memory: "4Gi"
            cpu: "2"
          limits:
            memory: "8Gi"
            cpu: "4"
---
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

## Monitoring and Observability

### Health Checks

#### Basic Health Check
```c
// health_check.c
#include "c_src/sparql7t.h"

int health_check() {
    // Create test engine
    S7TEngine* engine = s7t_create(1000, 100, 1000);
    if (!engine) return 1;
    
    // Add test triple
    s7t_add_triple(engine, 1, 2, 3);
    
    // Test pattern matching
    int result = s7t_ask_pattern(engine, 1, 2, 3);
    
    // Cleanup
    s7t_destroy(engine);
    
    return result ? 0 : 1;
}
```

#### HTTP Health Endpoint
```c
// health_endpoint.c
#include <microhttpd.h>

int health_handler(void *cls, struct MHD_Connection *connection,
                   const char *url, const char *method,
                   const char *version, const char *upload_data,
                   size_t *upload_data_size, void **con_cls) {
    
    int health_status = health_check();
    const char* response = health_status == 0 ? 
        "{\"status\":\"healthy\"}" : "{\"status\":\"unhealthy\"}";
    
    struct MHD_Response *resp = MHD_create_response_from_buffer(
        strlen(response), (void*)response, MHD_RESPMEM_PERSISTENT);
    
    MHD_add_response_header(resp, "Content-Type", "application/json");
    
    int ret = MHD_queue_response(connection, 
        health_status == 0 ? MHD_HTTP_OK : MHD_HTTP_SERVICE_UNAVAILABLE, resp);
    MHD_destroy_response(resp);
    
    return ret;
}
```

### Metrics Collection

#### Performance Metrics
```c
// metrics.c
typedef struct {
    uint64_t pattern_matches;
    uint64_t triple_additions;
    uint64_t batch_operations;
    double avg_latency_ns;
    double throughput_ops_per_sec;
    size_t memory_usage_bytes;
} EngineMetrics;

EngineMetrics collect_metrics(S7TEngine* engine) {
    EngineMetrics metrics = {0};
    
    // Collect basic metrics
    metrics.pattern_matches = engine->stats.pattern_count;
    metrics.triple_additions = engine->stats.triple_count;
    metrics.batch_operations = engine->stats.batch_count;
    
    // Calculate performance metrics
    if (engine->stats.total_time_ns > 0) {
        metrics.avg_latency_ns = (double)engine->stats.total_time_ns / 
                                engine->stats.pattern_count;
        metrics.throughput_ops_per_sec = engine->stats.pattern_count * 
                                        1000000000.0 / engine->stats.total_time_ns;
    }
    
    // Calculate memory usage
    metrics.memory_usage_bytes = calculate_memory_usage(engine);
    
    return metrics;
}
```

#### Prometheus Integration
```c
// prometheus_metrics.c
void export_prometheus_metrics(EngineMetrics* metrics, char* buffer, size_t size) {
    snprintf(buffer, size,
        "# HELP seven_tick_pattern_matches_total Total pattern matches\n"
        "# TYPE seven_tick_pattern_matches_total counter\n"
        "seven_tick_pattern_matches_total %lu\n"
        "# HELP seven_tick_triple_additions_total Total triple additions\n"
        "# TYPE seven_tick_triple_additions_total counter\n"
        "seven_tick_triple_additions_total %lu\n"
        "# HELP seven_tick_avg_latency_ns Average latency in nanoseconds\n"
        "# TYPE seven_tick_avg_latency_ns gauge\n"
        "seven_tick_avg_latency_ns %.2f\n"
        "# HELP seven_tick_throughput_ops_per_sec Throughput in operations per second\n"
        "# TYPE seven_tick_throughput_ops_per_sec gauge\n"
        "seven_tick_throughput_ops_per_sec %.2f\n"
        "# HELP seven_tick_memory_usage_bytes Memory usage in bytes\n"
        "# TYPE seven_tick_memory_usage_bytes gauge\n"
        "seven_tick_memory_usage_bytes %zu\n",
        metrics->pattern_matches,
        metrics->triple_additions,
        metrics->avg_latency_ns,
        metrics->throughput_ops_per_sec,
        metrics->memory_usage_bytes
    );
}
```

### Logging

#### Structured Logging
```c
// logging.c
typedef enum {
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR
} LogLevel;

void log_message(LogLevel level, const char* component, const char* message, ...) {
    time_t now = time(NULL);
    char timestamp[64];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    const char* level_str[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    
    va_list args;
    va_start(args, message);
    
    printf("[%s] %s [%s] ", timestamp, level_str[level], component);
    vprintf(message, args);
    printf("\n");
    
    va_end(args);
}

// Usage
log_message(LOG_LEVEL_INFO, "SPARQL", "Engine started with %d subjects, %d predicates, %d objects",
           max_subjects, max_predicates, max_objects);
```

## Backup and Recovery

### Data Backup

#### Snapshot Backup
```c
// backup.c
typedef struct {
    uint32_t version;
    time_t timestamp;
    size_t data_size;
    uint8_t* data;
} EngineSnapshot;

EngineSnapshot* create_snapshot(S7TEngine* engine) {
    EngineSnapshot* snapshot = malloc(sizeof(EngineSnapshot));
    snapshot->version = 1;
    snapshot->timestamp = time(NULL);
    
    // Serialize engine state
    snapshot->data_size = serialize_engine(engine, NULL, 0);
    snapshot->data = malloc(snapshot->data_size);
    serialize_engine(engine, snapshot->data, snapshot->data_size);
    
    return snapshot;
}

int save_snapshot(EngineSnapshot* snapshot, const char* filename) {
    FILE* file = fopen(filename, "wb");
    if (!file) return -1;
    
    // Write header
    fwrite(&snapshot->version, sizeof(uint32_t), 1, file);
    fwrite(&snapshot->timestamp, sizeof(time_t), 1, file);
    fwrite(&snapshot->data_size, sizeof(size_t), 1, file);
    
    // Write data
    fwrite(snapshot->data, 1, snapshot->data_size, file);
    
    fclose(file);
    return 0;
}
```

#### Incremental Backup
```c
// incremental_backup.c
typedef struct {
    uint32_t sequence_number;
    time_t timestamp;
    size_t change_count;
    TripleChange* changes;
} IncrementalBackup;

typedef struct {
    uint32_t subject;
    uint32_t predicate;
    uint32_t object;
    uint8_t operation;  // 0=add, 1=remove
} TripleChange;

IncrementalBackup* create_incremental_backup(S7TEngine* engine, 
                                           IncrementalBackup* previous) {
    IncrementalBackup* backup = malloc(sizeof(IncrementalBackup));
    backup->sequence_number = previous ? previous->sequence_number + 1 : 1;
    backup->timestamp = time(NULL);
    
    // Collect changes since last backup
    backup->changes = collect_changes(engine, previous);
    backup->change_count = count_changes(backup->changes);
    
    return backup;
}
```

### Recovery Procedures

#### Full Recovery
```c
// recovery.c
S7TEngine* recover_from_snapshot(const char* filename) {
    FILE* file = fopen(filename, "rb");
    if (!file) return NULL;
    
    // Read header
    uint32_t version;
    time_t timestamp;
    size_t data_size;
    
    fread(&version, sizeof(uint32_t), 1, file);
    fread(&timestamp, sizeof(time_t), 1, file);
    fread(&data_size, sizeof(size_t), 1, file);
    
    // Read data
    uint8_t* data = malloc(data_size);
    fread(data, 1, data_size, file);
    fclose(file);
    
    // Deserialize engine
    S7TEngine* engine = deserialize_engine(data, data_size);
    free(data);
    
    return engine;
}
```

#### Point-in-Time Recovery
```c
// point_in_time_recovery.c
S7TEngine* recover_to_point_in_time(const char* base_snapshot, 
                                   const char* incremental_dir,
                                   time_t target_time) {
    // Load base snapshot
    S7TEngine* engine = recover_from_snapshot(base_snapshot);
    if (!engine) return NULL;
    
    // Apply incremental backups up to target time
    DIR* dir = opendir(incremental_dir);
    struct dirent* entry;
    
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".inc") != NULL) {
            char filepath[256];
            snprintf(filepath, sizeof(filepath), "%s/%s", incremental_dir, entry->d_name);
            
            IncrementalBackup* backup = load_incremental_backup(filepath);
            if (backup && backup->timestamp <= target_time) {
                apply_incremental_backup(engine, backup);
            }
            
            free_incremental_backup(backup);
        }
    }
    
    closedir(dir);
    return engine;
}
```

## Security

### Access Control

#### Authentication
```c
// authentication.c
typedef struct {
    char* username;
    char* password_hash;
    uint32_t permissions;
} User;

typedef struct {
    User* users;
    size_t user_count;
} UserDatabase;

int authenticate_user(UserDatabase* db, const char* username, const char* password) {
    for (size_t i = 0; i < db->user_count; i++) {
        if (strcmp(db->users[i].username, username) == 0) {
            return verify_password(password, db->users[i].password_hash);
        }
    }
    return 0;
}
```

#### Authorization
```c
// authorization.c
#define PERMISSION_READ   0x01
#define PERMISSION_WRITE  0x02
#define PERMISSION_ADMIN  0x04

int check_permission(User* user, uint32_t required_permission) {
    return (user->permissions & required_permission) == required_permission;
}

int authorize_operation(User* user, const char* operation) {
    if (strcmp(operation, "read") == 0) {
        return check_permission(user, PERMISSION_READ);
    } else if (strcmp(operation, "write") == 0) {
        return check_permission(user, PERMISSION_WRITE);
    } else if (strcmp(operation, "admin") == 0) {
        return check_permission(user, PERMISSION_ADMIN);
    }
    return 0;
}
```

### Network Security

#### TLS Configuration
```c
// tls_config.c
#include <openssl/ssl.h>
#include <openssl/err.h>

SSL_CTX* configure_tls(const char* cert_file, const char* key_file) {
    SSL_CTX* ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx) return NULL;
    
    // Set certificate and private key
    if (SSL_CTX_use_certificate_file(ctx, cert_file, SSL_FILETYPE_PEM) <= 0) {
        SSL_CTX_free(ctx);
        return NULL;
    }
    
    if (SSL_CTX_use_PrivateKey_file(ctx, key_file, SSL_FILETYPE_PEM) <= 0) {
        SSL_CTX_free(ctx);
        return NULL;
    }
    
    // Set security options
    SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1);
    
    return ctx;
}
```

#### Firewall Configuration
```bash
# iptables rules for 7T Engine
# Allow HTTP/HTTPS traffic
iptables -A INPUT -p tcp --dport 80 -j ACCEPT
iptables -A INPUT -p tcp --dport 443 -j ACCEPT

# Allow health check endpoint
iptables -A INPUT -p tcp --dport 8080 -j ACCEPT

# Allow monitoring ports
iptables -A INPUT -p tcp --dport 9090 -j ACCEPT  # Prometheus
iptables -A INPUT -p tcp --dport 9100 -j ACCEPT  # Node Exporter

# Block other traffic
iptables -A INPUT -j DROP
```

## Scaling Strategies

### Horizontal Scaling

#### Sharding Strategy
```c
// sharding.c
typedef struct {
    uint32_t shard_id;
    S7TEngine* engine;
    uint32_t subject_range_start;
    uint32_t subject_range_end;
} Shard;

typedef struct {
    Shard* shards;
    size_t shard_count;
    uint32_t total_subjects;
} ShardedEngine;

Shard* get_shard_for_subject(ShardedEngine* se, uint32_t subject) {
    uint32_t shard_index = subject % se->shard_count;
    return &se->shards[shard_index];
}

int add_triple_sharded(ShardedEngine* se, uint32_t s, uint32_t p, uint32_t o) {
    Shard* shard = get_shard_for_subject(se, s);
    s7t_add_triple(shard->engine, s, p, o);
    return 0;
}
```

#### Load Balancing
```c
// load_balancer.c
typedef struct {
    char* host;
    int port;
    uint32_t weight;
    uint32_t current_load;
    time_t last_health_check;
    int healthy;
} BackendServer;

typedef struct {
    BackendServer* servers;
    size_t server_count;
    LoadBalancingAlgorithm algorithm;
} LoadBalancer;

BackendServer* select_backend(LoadBalancer* lb, const char* request) {
    switch (lb->algorithm) {
        case ROUND_ROBIN:
            return round_robin_select(lb);
        case LEAST_CONNECTIONS:
            return least_connections_select(lb);
        case WEIGHTED_ROUND_ROBIN:
            return weighted_round_robin_select(lb);
        default:
            return round_robin_select(lb);
    }
}
```

### Vertical Scaling

#### Memory Scaling
```c
// memory_scaling.c
typedef struct {
    size_t max_subjects;
    size_t max_predicates;
    size_t max_objects;
    size_t current_memory_usage;
    size_t memory_limit;
} MemoryConfig;

int resize_engine(S7TEngine* engine, MemoryConfig* new_config) {
    // Calculate new memory requirements
    size_t new_memory = calculate_memory_requirements(new_config);
    
    if (new_memory > new_config->memory_limit) {
        return -1;  // Exceeds memory limit
    }
    
    // Create new engine with larger capacity
    S7TEngine* new_engine = s7t_create(
        new_config->max_subjects,
        new_config->max_predicates,
        new_config->max_objects
    );
    
    if (!new_engine) return -1;
    
    // Migrate data
    migrate_engine_data(engine, new_engine);
    
    // Replace old engine
    s7t_destroy(engine);
    *engine = *new_engine;
    free(new_engine);
    
    return 0;
}
```

## Troubleshooting

### Common Issues

#### Memory Issues
```bash
# Check memory usage
free -h
cat /proc/meminfo

# Check for memory leaks
valgrind --leak-check=full ./verification/sparql_simple_test

# Monitor memory growth
watch -n 1 'ps aux | grep seven-tick'
```

#### Performance Issues
```bash
# Profile CPU usage
perf record -g ./verification/seven_tick_benchmark
perf report

# Check cache misses
perf stat -e cache-misses ./verification/seven_tick_benchmark

# Monitor system resources
htop
iostat -x 1
```

#### Network Issues
```bash
# Check network connectivity
ping seven-tick.example.com
telnet seven-tick.example.com 8080

# Check firewall rules
iptables -L -n
ufw status

# Monitor network traffic
tcpdump -i eth0 port 8080
```

### Debug Tools

#### Debug Logging
```c
// debug.c
#ifdef DEBUG
#define DEBUG_LOG(fmt, ...) \
    printf("[DEBUG] %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define DEBUG_LOG(fmt, ...)
#endif

// Usage
DEBUG_LOG("Adding triple: (%u, %u, %u)", s, p, o);
```

#### Performance Profiling
```c
// profiling.c
typedef struct {
    const char* name;
    uint64_t start_time;
    uint64_t total_time;
    uint64_t call_count;
} Profiler;

Profiler* profiler_create(const char* name) {
    Profiler* p = malloc(sizeof(Profiler));
    p->name = name;
    p->start_time = 0;
    p->total_time = 0;
    p->call_count = 0;
    return p;
}

void profiler_start(Profiler* p) {
    p->start_time = get_nanoseconds();
}

void profiler_stop(Profiler* p) {
    uint64_t end_time = get_nanoseconds();
    p->total_time += end_time - p->start_time;
    p->call_count++;
}

void profiler_report(Profiler* p) {
    printf("Profiler %s: %lu calls, %.2f ns avg\n", 
           p->name, p->call_count, 
           (double)p->total_time / p->call_count);
}
```

## Maintenance

### Regular Maintenance

#### Daily Tasks
- Monitor system resources
- Check error logs
- Verify health endpoints
- Review performance metrics

#### Weekly Tasks
- Update system packages
- Review security logs
- Analyze performance trends
- Backup verification

#### Monthly Tasks
- Security updates
- Performance optimization
- Capacity planning
- Documentation updates

### Update Procedures

#### Rolling Updates
```bash
#!/bin/bash
# rolling_update.sh

# Update one node at a time
for node in node1 node2 node3; do
    echo "Updating $node..."
    
    # Drain node
    kubectl drain $node --ignore-daemonsets --delete-emptydir-data
    
    # Update node
    ssh $node "cd /opt/seven-tick && git pull && make clean && make"
    
    # Restart service
    ssh $node "sudo systemctl restart seven-tick"
    
    # Verify health
    curl -f http://$node:8080/health || exit 1
    
    # Uncordon node
    kubectl uncordon $node
    
    echo "$node updated successfully"
done
```

#### Blue-Green Deployment
```bash
#!/bin/bash
# blue_green_deploy.sh

# Deploy new version to green environment
kubectl apply -f k8s-deployment-green.yaml

# Wait for green to be ready
kubectl rollout status deployment/seven-tick-green

# Switch traffic to green
kubectl patch service seven-tick-service -p '{"spec":{"selector":{"version":"green"}}}'

# Verify green is working
curl -f http://seven-tick-service/health || exit 1

# Decommission blue
kubectl delete deployment seven-tick-blue
``` 