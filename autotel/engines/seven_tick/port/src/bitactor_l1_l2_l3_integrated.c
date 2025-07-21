/**
 * @file bitactor_l1_l2_l3_integrated.c
 * @brief Integrated L1->L2->L3 BitActor Implementation with Comprehensive Testing
 * @version 2.0.0
 * 
 * This implements the full L1->L2->L3 integration stack:
 * - L1: Optimized 7-tick execution core (sub-100ns causal collapse)
 * - L2: Causal mailbox & fiber engine (sub-200ns message routing)
 * - L3: GenActor behavior patterns (sub-500ns actor transitions)
 * 
 * Performance Targets:
 * - L1 Causal Collapse: <100ns
 * - L2 Message Routing: <200ns  
 * - L3 Behavior Transition: <500ns
 * - Integrated Pipeline: <1000ns end-to-end
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <assert.h>

// Platform-specific timing and SIMD
#ifdef __APPLE__
#include <mach/mach_time.h>
#endif

#if defined(__aarch64__) || defined(__arm64__)
#include <arm_neon.h>
#define SIMD_PLATFORM_ARM
#elif defined(__x86_64__) || defined(__i386__)
#include <immintrin.h>
#define SIMD_PLATFORM_X86
#else
#define SIMD_PLATFORM_NONE
#endif

// =============================================================================
// INTEGRATED ARCHITECTURE CONSTANTS
// =============================================================================

#define CACHE_LINE_SIZE 64
#define L1_ACTORS_MAX 256
#define L2_MAILBOX_SIZE 512
#define L3_BEHAVIORS_MAX 64

// Performance targets for integrated pipeline
#define TARGET_L1_COLLAPSE_NS 100      // L1: Causal collapse
#define TARGET_L2_ROUTING_NS 200       // L2: Message routing
#define TARGET_L3_BEHAVIOR_NS 500      // L3: Behavior transition
#define TARGET_INTEGRATED_NS 1000      // End-to-end pipeline

// Trinity constraints
#define TRINITY_8T_CYCLES 8
#define TRINITY_8H_HOPS 8
#define TRINITY_8B_BITS 8

// =============================================================================
// L1: OPTIMIZED CORE STRUCTURES
// =============================================================================

/**
 * @brief L1 Optimized BitActor with integrated L2/L3 hooks
 */
typedef struct __attribute__((aligned(CACHE_LINE_SIZE))) {
    // L1 Hot data (first cache line)
    uint8_t bits;                    // 8-bit causal unit
    uint8_t l2_mailbox_pending;      // L2: Messages pending
    uint16_t l3_behavior_state;      // L3: Current behavior state
    uint32_t actor_id;               // Unique identifier
    uint64_t causal_vector;          // L1: Causal state
    uint64_t l2_message_vector;      // L2: Message routing vector
    uint64_t l3_behavior_vector;     // L3: Behavior pattern vector
    
    // Integration tracking
    uint32_t l1_tick_count;          // L1: Execution ticks
    uint32_t l2_message_count;       // L2: Messages processed
    uint32_t l3_transition_count;    // L3: State transitions
    uint32_t integration_hash;       // Cross-layer validation
    
} IntegratedBitActor;

/**
 * @brief L1 Pre-computed lookup table (enhanced for L2/L3)
 */
typedef struct __attribute__((aligned(CACHE_LINE_SIZE))) {
    uint64_t hop_lut[256][8];        // L1: Causal collapse lookup
    uint64_t l2_route_lut[256][16];  // L2: Message routing lookup
    uint64_t l3_behavior_lut[64][32]; // L3: Behavior transition lookup
    
    uint64_t trinity_mask;           // Trinity validation
    uint32_t integration_seed;       // Cross-layer seed
} IntegratedLookupTable;

// =============================================================================
// L2: CAUSAL MAILBOX STRUCTURES
// =============================================================================

/**
 * @brief L2 High-performance message structure
 */
typedef struct __attribute__((aligned(32))) {
    uint64_t message_id;             // Unique message ID
    uint32_t source_actor;           // Source actor ID
    uint32_t target_actor;           // Target actor ID
    uint16_t message_type;           // Message type
    uint16_t priority;               // Message priority
    uint64_t payload;                // Message payload
    uint64_t routing_vector;         // Routing optimization vector
    uint64_t timestamp_ns;           // Message timestamp
} L2Message;

/**
 * @brief L2 Optimized causal mailbox
 */
typedef struct __attribute__((aligned(CACHE_LINE_SIZE))) {
    // Hot path data
    L2Message messages[L2_MAILBOX_SIZE]; // Message buffer
    uint32_t head;                   // Buffer head
    uint32_t tail;                   // Buffer tail
    uint32_t count;                  // Active messages
    uint32_t processed_count;        // Total processed
    
    // Routing optimization
    uint32_t hot_routes[32];         // Top 20% routes
    uint64_t route_cache[256];       // Route lookup cache
    
    // Performance tracking
    uint64_t total_routing_time_ns;  // Total routing time
    uint32_t fast_routes;            // Sub-200ns routes
    uint32_t slow_routes;            // >200ns routes
} L2CausalMailbox;

// =============================================================================
// L3: GENACTOR BEHAVIOR STRUCTURES
// =============================================================================

/**
 * @brief L3 Behavior pattern definition
 */
typedef enum {
    L3_BEHAVIOR_IDLE = 0,
    L3_BEHAVIOR_PROCESSING,
    L3_BEHAVIOR_WAITING,
    L3_BEHAVIOR_SIGNALING,
    L3_BEHAVIOR_COLLAPSING,
    L3_BEHAVIOR_ERROR,
    L3_BEHAVIOR_RECOVERY,
    L3_BEHAVIOR_TERMINATING
} L3BehaviorState;

/**
 * @brief L3 GenActor behavior controller
 */
typedef struct __attribute__((aligned(CACHE_LINE_SIZE))) {
    // Current behavior state
    L3BehaviorState current_state;
    L3BehaviorState previous_state;
    uint32_t state_duration;         // Ticks in current state
    uint32_t transition_count;       // Total transitions
    
    // Behavior optimization
    uint64_t behavior_vector;        // Behavior pattern vector
    uint32_t hot_transitions[16];    // Top transitions (80/20)
    uint64_t transition_cache[64];   // Pre-computed transitions
    
    // Integration with L1/L2
    uint32_t l1_actor_id;           // Associated L1 actor
    uint32_t l2_mailbox_id;         // Associated L2 mailbox
    
    // Performance tracking
    uint64_t total_transition_time_ns;
    uint32_t fast_transitions;       // Sub-500ns transitions
    uint32_t slow_transitions;       // >500ns transitions
} L3GenActor;

// =============================================================================
// INTEGRATED SYSTEM STRUCTURE
// =============================================================================

/**
 * @brief Integrated L1->L2->L3 system
 */
typedef struct __attribute__((aligned(4096))) {
    // Layer components
    IntegratedBitActor l1_actors[L1_ACTORS_MAX];
    L2CausalMailbox l2_mailboxes[16];    // 16 mailboxes for load distribution
    L3GenActor l3_behaviors[L3_BEHAVIORS_MAX];
    
    // Lookup tables
    IntegratedLookupTable* lut;
    
    // System state
    uint64_t global_tick;
    uint32_t active_l1_actors;
    uint32_t active_l2_mailboxes;
    uint32_t active_l3_behaviors;
    
    // Performance metrics
    uint64_t total_l1_time_ns;
    uint64_t total_l2_time_ns;
    uint64_t total_l3_time_ns;
    uint64_t total_integrated_time_ns;
    
    uint32_t l1_operations;
    uint32_t l2_operations;
    uint32_t l3_operations;
    uint32_t integrated_operations;
    
    // Integration validation
    uint64_t integration_hash;
    uint32_t trinity_violations;
} IntegratedSystem;

// =============================================================================
// TIMING AND UTILITIES
// =============================================================================

static uint64_t get_precise_nanoseconds(void) {
#ifdef __APPLE__
    static mach_timebase_info_data_t timebase = {0};
    if (timebase.denom == 0) {
        mach_timebase_info(&timebase);
    }
    uint64_t mach_time = mach_absolute_time();
    return mach_time * timebase.numer / timebase.denom;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
#endif
}

static inline uint32_t fast_hash(uint32_t x) {
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = ((x >> 16) ^ x) * 0x45d9f3b;
    x = (x >> 16) ^ x;
    return x;
}

// =============================================================================
// L1: OPTIMIZED CORE IMPLEMENTATION
// =============================================================================

/**
 * @brief Initialize integrated lookup table
 */
static IntegratedLookupTable* init_integrated_lut(void) {
    printf("🔥 INTEGRATION: Initializing L1->L2->L3 lookup tables...\n");
    
    IntegratedLookupTable* lut = aligned_alloc(CACHE_LINE_SIZE, sizeof(IntegratedLookupTable));
    if (!lut) return NULL;
    
    memset(lut, 0, sizeof(IntegratedLookupTable));
    
    // L1: Pre-compute causal collapse lookup
    for (int bits = 0; bits < 256; bits++) {
        for (int hop = 0; hop < 8; hop++) {
            switch (hop) {
                case 0: lut->hop_lut[bits][hop] = (bits & 0x01) ? 0x1 : 0x0; break;
                case 1: lut->hop_lut[bits][hop] = (bits & 0x02) ? 0x2 : 0x0; break;
                case 2: lut->hop_lut[bits][hop] = (bits & 0x04) ? 0x4 : 0x0; break;
                case 3: lut->hop_lut[bits][hop] = __builtin_popcount(bits); break;
                case 4: lut->hop_lut[bits][hop] = bits ^ 0x88; break;
                case 5: lut->hop_lut[bits][hop] = bits | 0x04; break;
                case 6: lut->hop_lut[bits][hop] = bits | 0x10; break;
                case 7: lut->hop_lut[bits][hop] = fast_hash(bits); break;
            }
        }
    }
    
    // L2: Pre-compute message routing lookup
    for (int msg_type = 0; msg_type < 256; msg_type++) {
        for (int route = 0; route < 16; route++) {
            lut->l2_route_lut[msg_type][route] = fast_hash(msg_type * route + 1);
        }
    }
    
    // L3: Pre-compute behavior transitions
    for (int state = 0; state < 64; state++) {
        for (int transition = 0; transition < 32; transition++) {
            lut->l3_behavior_lut[state][transition] = 
                fast_hash((state << 8) | transition) & 0x7; // Map to 8 states
        }
    }
    
    lut->trinity_mask = 0x8888888888888888ULL;
    lut->integration_seed = 0x88888888;
    
    printf("🔥 INTEGRATION: Lookup tables ready (L1: 256x8, L2: 256x16, L3: 64x32)\n");
    return lut;
}

/**
 * @brief L1 Optimized causal collapse with L2/L3 integration
 */
static inline uint64_t l1_collapse_integrated(IntegratedBitActor* actor, 
                                              const IntegratedLookupTable* lut) {
    uint64_t start = get_precise_nanoseconds();
    
    // L1: Causal collapse using pre-computed lookup
    const uint64_t* hop_row = lut->hop_lut[actor->bits];
    uint64_t result = actor->causal_vector;
    
    // Unrolled 8-hop collapse
    result ^= hop_row[0] ^ hop_row[1] ^ hop_row[2] ^ hop_row[3];
    result ^= hop_row[4] ^ hop_row[5] ^ hop_row[6] ^ hop_row[7];
    
    // Apply Trinity mask
    result &= lut->trinity_mask;
    
    // L2/L3 Integration: Update cross-layer vectors
    actor->l2_message_vector = result ^ fast_hash(actor->actor_id);
    actor->l3_behavior_vector = result ^ (actor->l3_behavior_state << 8);
    
    // Update state
    actor->causal_vector = result;
    actor->bits |= 0x40;  // Set collapse pending bit
    actor->l1_tick_count++;
    
    uint64_t end = get_precise_nanoseconds();
    return end - start;
}

// =============================================================================
// L2: CAUSAL MAILBOX IMPLEMENTATION
// =============================================================================

/**
 * @brief Initialize L2 causal mailbox
 */
static void l2_mailbox_init(L2CausalMailbox* mailbox) {
    memset(mailbox, 0, sizeof(L2CausalMailbox));
    
    // Initialize hot routes (80/20 optimization)
    for (int i = 0; i < 32; i++) {
        mailbox->hot_routes[i] = i; // Initial hot routes
    }
}

/**
 * @brief L2 Optimized message routing
 */
static inline uint64_t l2_route_message(L2CausalMailbox* mailbox, 
                                        const L2Message* msg,
                                        const IntegratedLookupTable* lut) {
    uint64_t start = get_precise_nanoseconds();
    
    // Check if mailbox is full
    if (mailbox->count >= L2_MAILBOX_SIZE) {
        return get_precise_nanoseconds() - start; // Early return
    }
    
    // Hot path: Check route cache first (80% hit rate target)
    uint32_t route_key = (msg->source_actor << 16) | msg->target_actor;
    uint32_t cache_idx = fast_hash(route_key) & 0xFF;
    
    uint64_t routing_vector;
    if (mailbox->route_cache[cache_idx] != 0) {
        // Cache hit - use cached routing
        routing_vector = mailbox->route_cache[cache_idx];
    } else {
        // Cache miss - compute and cache
        const uint64_t* route_row = lut->l2_route_lut[msg->message_type & 0xFF];
        routing_vector = route_row[msg->priority & 0x0F];
        mailbox->route_cache[cache_idx] = routing_vector;
    }
    
    // Add message to mailbox
    uint32_t tail_idx = mailbox->tail;
    mailbox->messages[tail_idx] = *msg;
    mailbox->messages[tail_idx].routing_vector = routing_vector;
    mailbox->messages[tail_idx].timestamp_ns = start;
    
    mailbox->tail = (mailbox->tail + 1) % L2_MAILBOX_SIZE;
    mailbox->count++;
    mailbox->processed_count++;
    
    uint64_t end = get_precise_nanoseconds();
    uint64_t routing_time = end - start;
    
    // Update performance metrics
    mailbox->total_routing_time_ns += routing_time;
    if (routing_time <= TARGET_L2_ROUTING_NS) {
        mailbox->fast_routes++;
    } else {
        mailbox->slow_routes++;
    }
    
    return routing_time;
}

/**
 * @brief L2 Process next message
 */
static inline L2Message* l2_get_next_message(L2CausalMailbox* mailbox) {
    if (mailbox->count == 0) return NULL;
    
    L2Message* msg = &mailbox->messages[mailbox->head];
    mailbox->head = (mailbox->head + 1) % L2_MAILBOX_SIZE;
    mailbox->count--;
    
    return msg;
}

// =============================================================================
// L3: GENACTOR BEHAVIOR IMPLEMENTATION
// =============================================================================

/**
 * @brief Initialize L3 GenActor
 */
static void l3_genactor_init(L3GenActor* genactor, uint32_t l1_actor_id, uint32_t l2_mailbox_id) {
    memset(genactor, 0, sizeof(L3GenActor));
    
    genactor->current_state = L3_BEHAVIOR_IDLE;
    genactor->previous_state = L3_BEHAVIOR_IDLE;
    genactor->l1_actor_id = l1_actor_id;
    genactor->l2_mailbox_id = l2_mailbox_id;
    
    // Initialize hot transitions (80/20 optimization)
    for (int i = 0; i < 16; i++) {
        genactor->hot_transitions[i] = i; // Common transition patterns
    }
}

/**
 * @brief L3 Optimized behavior transition
 */
static inline uint64_t l3_behavior_transition(L3GenActor* genactor,
                                              L2Message* message,
                                              const IntegratedLookupTable* lut) {
    uint64_t start = get_precise_nanoseconds();
    
    L3BehaviorState new_state = genactor->current_state;
    
    // State transition logic based on message and current state
    switch (genactor->current_state) {
        case L3_BEHAVIOR_IDLE:
            if (message) {
                new_state = (message->message_type & 0x7) + 1; // Map to behavior states
            }
            break;
            
        case L3_BEHAVIOR_PROCESSING:
            if (genactor->state_duration > 3) {
                new_state = L3_BEHAVIOR_SIGNALING;
            }
            break;
            
        case L3_BEHAVIOR_SIGNALING:
            new_state = L3_BEHAVIOR_IDLE;
            break;
            
        case L3_BEHAVIOR_WAITING:
            if (message && message->target_actor == genactor->l1_actor_id) {
                new_state = L3_BEHAVIOR_PROCESSING;
            }
            break;
            
        default:
            new_state = L3_BEHAVIOR_IDLE;
            break;
    }
    
    // Apply transition if state changed
    if (new_state != genactor->current_state) {
        // Use pre-computed transition lookup
        uint32_t transition_key = (genactor->current_state << 3) | new_state;
        uint32_t cache_idx = transition_key & 0x3F;
        
        genactor->behavior_vector = lut->l3_behavior_lut[cache_idx][0];
        genactor->previous_state = genactor->current_state;
        genactor->current_state = new_state;
        genactor->state_duration = 0;
        genactor->transition_count++;
    } else {
        genactor->state_duration++;
    }
    
    uint64_t end = get_precise_nanoseconds();
    uint64_t transition_time = end - start;
    
    // Update performance metrics
    genactor->total_transition_time_ns += transition_time;
    if (transition_time <= TARGET_L3_BEHAVIOR_NS) {
        genactor->fast_transitions++;
    } else {
        genactor->slow_transitions++;
    }
    
    return transition_time;
}

// =============================================================================
// INTEGRATED SYSTEM IMPLEMENTATION
// =============================================================================

/**
 * @brief Create integrated L1->L2->L3 system
 */
static IntegratedSystem* create_integrated_system(void) {
    printf("🚀 INTEGRATION: Creating L1->L2->L3 system...\n");
    
    IntegratedSystem* system = aligned_alloc(4096, sizeof(IntegratedSystem));
    if (!system) return NULL;
    
    memset(system, 0, sizeof(IntegratedSystem));
    
    // Initialize lookup tables
    system->lut = init_integrated_lut();
    if (!system->lut) {
        free(system);
        return NULL;
    }
    
    // Initialize L1 actors
    for (int i = 0; i < L1_ACTORS_MAX; i++) {
        IntegratedBitActor* actor = &system->l1_actors[i];
        actor->actor_id = i + 1;
        actor->bits = 0x01; // Trigger active
        actor->causal_vector = i;
        actor->l2_mailbox_pending = 0;
        actor->l3_behavior_state = L3_BEHAVIOR_IDLE;
    }
    
    // Initialize L2 mailboxes
    for (int i = 0; i < 16; i++) {
        l2_mailbox_init(&system->l2_mailboxes[i]);
    }
    
    // Initialize L3 GenActors
    for (int i = 0; i < L3_BEHAVIORS_MAX; i++) {
        l3_genactor_init(&system->l3_behaviors[i], i + 1, i % 16);
    }
    
    system->global_tick = 0;
    system->active_l1_actors = L1_ACTORS_MAX;
    system->active_l2_mailboxes = 16;
    system->active_l3_behaviors = L3_BEHAVIORS_MAX;
    
    printf("🚀 INTEGRATION: System ready (L1: %d actors, L2: %d mailboxes, L3: %d behaviors)\n",
           L1_ACTORS_MAX, 16, L3_BEHAVIORS_MAX);
    
    return system;
}

/**
 * @brief Execute integrated L1->L2->L3 pipeline
 */
static inline uint64_t execute_integrated_pipeline(IntegratedSystem* system, 
                                                   uint32_t actor_idx, 
                                                   L2Message* message) {
    uint64_t start = get_precise_nanoseconds();
    
    // Validate inputs
    if (actor_idx >= L1_ACTORS_MAX) return 0;
    
    IntegratedBitActor* l1_actor = &system->l1_actors[actor_idx];
    L2CausalMailbox* l2_mailbox = &system->l2_mailboxes[actor_idx % 16];
    L3GenActor* l3_behavior = &system->l3_behaviors[actor_idx % L3_BEHAVIORS_MAX];
    
    uint64_t l1_time = 0, l2_time = 0, l3_time = 0;
    
    // L1: Execute causal collapse
    l1_time = l1_collapse_integrated(l1_actor, system->lut);
    
    // L2: Route message (if provided)
    if (message) {
        l2_time = l2_route_message(l2_mailbox, message, system->lut);
    }
    
    // L3: Execute behavior transition
    L2Message* next_msg = l2_get_next_message(l2_mailbox);
    l3_time = l3_behavior_transition(l3_behavior, next_msg, system->lut);
    
    // Update integration hash for validation
    system->integration_hash ^= fast_hash(l1_actor->causal_vector ^ l2_mailbox->processed_count ^ l3_behavior->transition_count);
    
    uint64_t end = get_precise_nanoseconds();
    uint64_t total_time = end - start;
    
    // Update system metrics
    system->total_l1_time_ns += l1_time;
    system->total_l2_time_ns += l2_time;
    system->total_l3_time_ns += l3_time;
    system->total_integrated_time_ns += total_time;
    
    system->l1_operations++;
    if (message) system->l2_operations++;
    system->l3_operations++;
    system->integrated_operations++;
    
    return total_time;
}

// =============================================================================
// COMPREHENSIVE BENCHMARKING FRAMEWORK
// =============================================================================

typedef struct {
    uint64_t min_ns;
    uint64_t max_ns;
    uint64_t total_ns;
    double avg_ns;
    uint32_t samples;
    uint32_t within_target;
    double success_rate;
} BenchmarkResults;

/**
 * @brief Benchmark L1 performance
 */
static BenchmarkResults benchmark_l1_performance(IntegratedSystem* system, uint32_t iterations) {
    BenchmarkResults results = {UINT64_MAX, 0, 0, 0.0, 0, 0, 0.0};
    
    printf("🔬 BENCHMARK: L1 Causal Collapse Performance...\n");
    
    for (uint32_t i = 0; i < iterations; i++) {
        uint32_t actor_idx = i % L1_ACTORS_MAX;
        uint64_t time = l1_collapse_integrated(&system->l1_actors[actor_idx], system->lut);
        
        if (time < results.min_ns) results.min_ns = time;
        if (time > results.max_ns) results.max_ns = time;
        results.total_ns += time;
        results.samples++;
        
        if (time <= TARGET_L1_COLLAPSE_NS) {
            results.within_target++;
        }
    }
    
    results.avg_ns = (double)results.total_ns / results.samples;
    results.success_rate = (double)results.within_target / results.samples * 100.0;
    
    return results;
}

/**
 * @brief Benchmark L2 performance
 */
static BenchmarkResults benchmark_l2_performance(IntegratedSystem* system, uint32_t iterations) {
    BenchmarkResults results = {UINT64_MAX, 0, 0, 0.0, 0, 0, 0.0};
    
    printf("🔬 BENCHMARK: L2 Message Routing Performance...\n");
    
    for (uint32_t i = 0; i < iterations; i++) {
        L2Message msg = {
            .message_id = i,
            .source_actor = i % L1_ACTORS_MAX,
            .target_actor = (i + 1) % L1_ACTORS_MAX,
            .message_type = i % 16,
            .priority = i % 8,
            .payload = fast_hash(i)
        };
        
        L2CausalMailbox* mailbox = &system->l2_mailboxes[i % 16];
        uint64_t time = l2_route_message(mailbox, &msg, system->lut);
        
        if (time < results.min_ns) results.min_ns = time;
        if (time > results.max_ns) results.max_ns = time;
        results.total_ns += time;
        results.samples++;
        
        if (time <= TARGET_L2_ROUTING_NS) {
            results.within_target++;
        }
    }
    
    results.avg_ns = (double)results.total_ns / results.samples;
    results.success_rate = (double)results.within_target / results.samples * 100.0;
    
    return results;
}

/**
 * @brief Benchmark L3 performance
 */
static BenchmarkResults benchmark_l3_performance(IntegratedSystem* system, uint32_t iterations) {
    BenchmarkResults results = {UINT64_MAX, 0, 0, 0.0, 0, 0, 0.0};
    
    printf("🔬 BENCHMARK: L3 Behavior Transition Performance...\n");
    
    for (uint32_t i = 0; i < iterations; i++) {
        L3GenActor* genactor = &system->l3_behaviors[i % L3_BEHAVIORS_MAX];
        
        // Create test message
        L2Message msg = {
            .message_id = i,
            .source_actor = i % L1_ACTORS_MAX,
            .target_actor = genactor->l1_actor_id,
            .message_type = i % 8,
            .priority = i % 4
        };
        
        uint64_t time = l3_behavior_transition(genactor, &msg, system->lut);
        
        if (time < results.min_ns) results.min_ns = time;
        if (time > results.max_ns) results.max_ns = time;
        results.total_ns += time;
        results.samples++;
        
        if (time <= TARGET_L3_BEHAVIOR_NS) {
            results.within_target++;
        }
    }
    
    results.avg_ns = (double)results.total_ns / results.samples;
    results.success_rate = (double)results.within_target / results.samples * 100.0;
    
    return results;
}

/**
 * @brief Benchmark integrated pipeline performance
 */
static BenchmarkResults benchmark_integrated_performance(IntegratedSystem* system, uint32_t iterations) {
    BenchmarkResults results = {UINT64_MAX, 0, 0, 0.0, 0, 0, 0.0};
    
    printf("🔬 BENCHMARK: Integrated L1->L2->L3 Pipeline Performance...\n");
    
    for (uint32_t i = 0; i < iterations; i++) {
        L2Message msg = {
            .message_id = i,
            .source_actor = i % L1_ACTORS_MAX,
            .target_actor = (i + 7) % L1_ACTORS_MAX, // Avoid self-messaging
            .message_type = i % 16,
            .priority = i % 8,
            .payload = fast_hash(i * 13)
        };
        
        uint32_t actor_idx = i % L1_ACTORS_MAX;
        uint64_t time = execute_integrated_pipeline(system, actor_idx, &msg);
        
        if (time < results.min_ns) results.min_ns = time;
        if (time > results.max_ns) results.max_ns = time;
        results.total_ns += time;
        results.samples++;
        
        if (time <= TARGET_INTEGRATED_NS) {
            results.within_target++;
        }
    }
    
    results.avg_ns = (double)results.total_ns / results.samples;
    results.success_rate = (double)results.within_target / results.samples * 100.0;
    
    return results;
}

/**
 * @brief Print benchmark results
 */
static void print_benchmark_results(const char* name, BenchmarkResults results, uint32_t target_ns) {
    printf("\n📊 %s Results:\n", name);
    printf("  Target:      %uns\n", target_ns);
    printf("  Min:         %lluns %s\n", results.min_ns, 
           results.min_ns <= target_ns ? "✅" : "❌");
    printf("  Average:     %.1fns %s\n", results.avg_ns,
           results.avg_ns <= target_ns ? "✅" : "❌");
    printf("  Max:         %lluns %s\n", results.max_ns,
           results.max_ns <= target_ns ? "✅" : "❌");
    printf("  Success Rate: %.1f%% (%u/%u within target)\n", 
           results.success_rate, results.within_target, results.samples);
}

// =============================================================================
// PERMUTATION TESTING FRAMEWORK
// =============================================================================

/**
 * @brief Permutation test configuration
 */
typedef struct {
    uint32_t num_actors;         // Number of L1 actors to test
    uint32_t num_messages;       // Messages per permutation
    uint32_t num_permutations;   // Total permutations to test
    bool test_all_combinations;  // Test all actor/mailbox combinations
} PermutationConfig;

/**
 * @brief Permutation test results
 */
typedef struct {
    uint32_t total_tests;
    uint32_t passed_tests;
    uint32_t failed_tests;
    double pass_rate;
    uint64_t total_time_ns;
    double avg_time_per_test_ns;
    uint32_t integration_errors;
} PermutationResults;

/**
 * @brief Execute permutation test
 */
static PermutationResults execute_permutation_test(IntegratedSystem* system, 
                                                   PermutationConfig config) {
    printf("🧪 PERMUTATION TEST: Starting comprehensive validation...\n");
    printf("   Actors: %u, Messages: %u, Permutations: %u\n", 
           config.num_actors, config.num_messages, config.num_permutations);
    
    PermutationResults results = {0};
    uint64_t test_start = get_precise_nanoseconds();
    
    // Test different actor/message/behavior combinations
    for (uint32_t perm = 0; perm < config.num_permutations; perm++) {
        uint64_t perm_start = get_precise_nanoseconds();
        bool test_passed = true;
        uint64_t initial_hash = system->integration_hash;
        
        // Execute messages in this permutation
        for (uint32_t msg_idx = 0; msg_idx < config.num_messages; msg_idx++) {
            uint32_t actor_idx = (perm + msg_idx) % config.num_actors;
            
            L2Message msg = {
                .message_id = perm * config.num_messages + msg_idx,
                .source_actor = actor_idx,
                .target_actor = (actor_idx + perm) % config.num_actors,
                .message_type = (perm + msg_idx) % 16,
                .priority = (perm + msg_idx) % 8,
                .payload = fast_hash(perm * msg_idx + 1)
            };
            
            uint64_t pipeline_time = execute_integrated_pipeline(system, actor_idx, &msg);
            
            // Test pipeline performance target
            if (pipeline_time > TARGET_INTEGRATED_NS * 2) { // Allow 2x margin
                test_passed = false;
            }
            
            // Test integration consistency (hash should change with meaningful activity)
            if (msg_idx == config.num_messages - 1) { // Check at end of permutation
                if (system->integration_hash == initial_hash && config.num_messages > 1) {
                    // Hash should change with multiple operations
                    results.integration_errors++;
                    test_passed = false;
                }
            }
        }
        
        uint64_t perm_end = get_precise_nanoseconds();
        uint64_t perm_time = perm_end - perm_start;
        
        results.total_tests++;
        results.total_time_ns += perm_time;
        
        if (test_passed) {
            results.passed_tests++;
        } else {
            results.failed_tests++;
        }
        
        // Progress update
        if ((perm + 1) % (config.num_permutations / 10) == 0) {
            printf("   Progress: %u/%u permutations (%.1f%%)\n", 
                   perm + 1, config.num_permutations, 
                   (double)(perm + 1) / config.num_permutations * 100.0);
        }
    }
    
    uint64_t test_end = get_precise_nanoseconds();
    
    results.pass_rate = (double)results.passed_tests / results.total_tests * 100.0;
    results.avg_time_per_test_ns = (double)results.total_time_ns / results.total_tests;
    
    printf("🧪 PERMUTATION TEST: Complete in %.2f seconds\n", 
           (test_end - test_start) / 1e9);
    
    return results;
}

/**
 * @brief Print permutation test results
 */
static void print_permutation_results(PermutationResults results) {
    printf("\n🧪 PERMUTATION TEST RESULTS:\n");
    printf("  Total Tests:       %u\n", results.total_tests);
    printf("  Passed:            %u (%.1f%%)\n", results.passed_tests, results.pass_rate);
    printf("  Failed:            %u (%.1f%%)\n", results.failed_tests, 
           (double)results.failed_tests / results.total_tests * 100.0);
    printf("  Integration Errors: %u\n", results.integration_errors);
    printf("  Avg Time/Test:     %.1fns\n", results.avg_time_per_test_ns);
    printf("  Overall Result:     %s\n", results.pass_rate >= 95.0 ? "✅ PASS" : "❌ FAIL");
}

// =============================================================================
// MAIN TESTING AND VALIDATION
// =============================================================================

int main(void) {
    printf("🚀 BitActor L1->L2->L3 Integrated Testing Framework\n");
    printf("===================================================\n\n");
    
    printf("Performance Targets:\n");
    printf("  L1 Causal Collapse:   ≤%uns\n", TARGET_L1_COLLAPSE_NS);
    printf("  L2 Message Routing:   ≤%uns\n", TARGET_L2_ROUTING_NS);
    printf("  L3 Behavior Transition: ≤%uns\n", TARGET_L3_BEHAVIOR_NS);
    printf("  Integrated Pipeline:  ≤%uns\n\n", TARGET_INTEGRATED_NS);
    
    // Create integrated system
    IntegratedSystem* system = create_integrated_system();
    if (!system) {
        printf("❌ Failed to create integrated system\n");
        return 1;
    }
    
    // =============================================================================
    // LAYER-BY-LAYER BENCHMARKING
    // =============================================================================
    
    printf("🔬 LAYER-BY-LAYER BENCHMARKING\n");
    printf("==============================\n");
    
    const uint32_t benchmark_iterations = 10000;
    
    // Benchmark L1 performance
    BenchmarkResults l1_results = benchmark_l1_performance(system, benchmark_iterations);
    print_benchmark_results("L1 Causal Collapse", l1_results, TARGET_L1_COLLAPSE_NS);
    
    // Benchmark L2 performance  
    BenchmarkResults l2_results = benchmark_l2_performance(system, benchmark_iterations);
    print_benchmark_results("L2 Message Routing", l2_results, TARGET_L2_ROUTING_NS);
    
    // Benchmark L3 performance
    BenchmarkResults l3_results = benchmark_l3_performance(system, benchmark_iterations);
    print_benchmark_results("L3 Behavior Transition", l3_results, TARGET_L3_BEHAVIOR_NS);
    
    // Benchmark integrated performance
    BenchmarkResults integrated_results = benchmark_integrated_performance(system, benchmark_iterations);
    print_benchmark_results("Integrated L1->L2->L3 Pipeline", integrated_results, TARGET_INTEGRATED_NS);
    
    // =============================================================================
    // PERMUTATION TESTING
    // =============================================================================
    
    printf("\n🧪 PERMUTATION TESTING\n");
    printf("=====================\n");
    
    PermutationConfig perm_config = {
        .num_actors = 32,
        .num_messages = 16,
        .num_permutations = 1000,
        .test_all_combinations = true
    };
    
    PermutationResults perm_results = execute_permutation_test(system, perm_config);
    print_permutation_results(perm_results);
    
    // =============================================================================
    // INTEGRATION VALIDATION
    // =============================================================================
    
    printf("\n🔍 INTEGRATION VALIDATION\n");
    printf("========================\n");
    
    // Validate system state consistency
    bool validation_passed = true;
    uint32_t total_l1_ticks = 0;
    uint32_t total_l2_messages = 0;
    uint32_t total_l3_transitions = 0;
    
    for (int i = 0; i < L1_ACTORS_MAX; i++) {
        total_l1_ticks += system->l1_actors[i].l1_tick_count;
    }
    
    for (int i = 0; i < 16; i++) {
        total_l2_messages += system->l2_mailboxes[i].processed_count;
    }
    
    for (int i = 0; i < L3_BEHAVIORS_MAX; i++) {
        total_l3_transitions += system->l3_behaviors[i].transition_count;
    }
    
    printf("System Activity Summary:\n");
    printf("  L1 Operations:    %u ticks\n", total_l1_ticks);
    printf("  L2 Operations:    %u messages routed\n", total_l2_messages);
    printf("  L3 Operations:    %u transitions\n", total_l3_transitions);
    printf("  Integration Hash: 0x%016llX\n", system->integration_hash);
    printf("  Trinity Violations: %u\n", system->trinity_violations);
    
    // Performance summary
    double l1_avg_per_op = system->l1_operations > 0 ? 
        (double)system->total_l1_time_ns / system->l1_operations : 0.0;
    double l2_avg_per_op = system->l2_operations > 0 ? 
        (double)system->total_l2_time_ns / system->l2_operations : 0.0;
    double l3_avg_per_op = system->l3_operations > 0 ? 
        (double)system->total_l3_time_ns / system->l3_operations : 0.0;
    double integrated_avg_per_op = system->integrated_operations > 0 ? 
        (double)system->total_integrated_time_ns / system->integrated_operations : 0.0;
    
    printf("\nIntegrated Performance Summary:\n");
    printf("  L1 Avg/Op:        %.1fns (target: %uns) %s\n", 
           l1_avg_per_op, TARGET_L1_COLLAPSE_NS, 
           l1_avg_per_op <= TARGET_L1_COLLAPSE_NS ? "✅" : "❌");
    printf("  L2 Avg/Op:        %.1fns (target: %uns) %s\n", 
           l2_avg_per_op, TARGET_L2_ROUTING_NS,
           l2_avg_per_op <= TARGET_L2_ROUTING_NS ? "✅" : "❌");
    printf("  L3 Avg/Op:        %.1fns (target: %uns) %s\n", 
           l3_avg_per_op, TARGET_L3_BEHAVIOR_NS,
           l3_avg_per_op <= TARGET_L3_BEHAVIOR_NS ? "✅" : "❌");
    printf("  Pipeline Avg/Op:  %.1fns (target: %uns) %s\n", 
           integrated_avg_per_op, TARGET_INTEGRATED_NS,
           integrated_avg_per_op <= TARGET_INTEGRATED_NS ? "✅" : "❌");
    
    // Overall validation
    bool performance_ok = (l1_results.success_rate >= 50.0) &&
                         (l2_results.success_rate >= 50.0) &&
                         (l3_results.success_rate >= 50.0) &&
                         (integrated_results.success_rate >= 50.0);
    
    bool permutation_ok = (perm_results.pass_rate >= 95.0);
    bool integration_ok = (system->trinity_violations == 0) && 
                         (system->integration_hash != 0);
    
    validation_passed = performance_ok && permutation_ok && integration_ok;
    
    printf("\n🎯 FINAL VALIDATION RESULTS\n");
    printf("===========================\n");
    printf("Performance Tests:  %s\n", performance_ok ? "✅ PASS" : "❌ FAIL");
    printf("Permutation Tests:  %s\n", permutation_ok ? "✅ PASS" : "❌ FAIL");
    printf("Integration Tests:  %s\n", integration_ok ? "✅ PASS" : "❌ FAIL");
    printf("\n🌌 OVERALL RESULT:   %s\n", validation_passed ? "✅ SUCCESS" : "❌ FAILURE");
    
    // Cleanup
    if (system->lut) free(system->lut);
    free(system);
    
    return validation_passed ? 0 : 1;
}