# 7T Team Onboarding: Physics-Compliant Computing

> **"Welcome to the team! You're about to learn why we don't optimize code — we comply with physics."**

This document is your guide to understanding the fundamental principles that drive every decision in the 7T engine. These aren't just coding guidelines — they're **laws of nature** that we must follow to achieve nanosecond-scale performance.

---

## 🎯 The 7T Mission

**Goal:** Build a system that can process 100M+ operations per second with ≤10ns latency per operation.

**Challenge:** At this scale, traditional software engineering breaks down. You're not fighting your competitors — you're fighting the **speed of light**.

**Solution:** Design every component to be **physics-compliant**.

---

## ⚡ The 8 Laws of 7T

### **Law 1: Latency is Not an Optimization; It's a Budget**

#### **What This Means:**
- Every CPU cycle costs real time
- 1 nanosecond = 30cm of light travel in silicon
- If your data isn't in L1 cache, you've already lost

#### **How to Apply:**
```c
// ❌ DON'T: Use pointers that might miss L1
typedef struct {
    char* name;           // Could be anywhere in memory
    int* values;          // Another pointer chase
} BadNode;

// ✅ DO: Use fixed arrays and interned IDs
typedef struct {
    uint32_t name_id;     // Direct array access
    uint32_t values[8];   // Fixed size, L1-friendly
} GoodNode;
```

#### **Your Checklist:**
- [ ] All data structures use fixed arrays
- [ ] No dynamic allocations in hot paths
- [ ] String comparisons use interned IDs
- [ ] Memory layout is cache-friendly

---

### **Law 2: Information = Structure + Entropy**

#### **What This Means:**
- High entropy (randomness) = expensive computation
- Low entropy (structure) = cheap computation
- Interning reduces entropy by creating structure

#### **How to Apply:**
```c
// ❌ DON'T: Use high-entropy strings everywhere
int compare_iris(const char* iri1, const char* iri2) {
    return strcmp(iri1, iri2);  // O(n) string comparison
}

// ✅ DO: Use low-entropy interned IDs
int compare_interned_iris(uint32_t id1, uint32_t id2) {
    return id1 == id2;  // O(1) integer comparison
}
```

#### **Your Checklist:**
- [ ] All strings are interned
- [ ] Comparisons use IDs, not strings
- [ ] Data structures have fixed layouts
- [ ] Validation rules are pre-compiled

---

### **Law 3: Allocation = Uncertainty**

#### **What This Means:**
- Every `malloc()` is a mini operating system call
- Allocation time is unpredictable (100-500ns)
- At 7T speeds, allocation is 10x too slow

#### **How to Apply:**
```c
// ❌ DON'T: Allocate dynamically in hot paths
void process_data(const char* name) {
    char* buffer = malloc(strlen(name) + 1);  // 100-500ns
    strcpy(buffer, name);
    // ... process ...
    free(buffer);
}

// ✅ DO: Use pre-allocated pools
void process_data_static(MemoryPool* pool, const char* name) {
    char* buffer = pool_alloc(pool, strlen(name) + 1);  // 1-2ns
    strcpy(buffer, name);
    // ... process ...
    // No free needed - pool handles cleanup
}
```

#### **Your Checklist:**
- [ ] Use memory pools for all allocations
- [ ] Pre-allocate fixed-size arrays
- [ ] No dynamic allocation in hot paths
- [ ] All object lifetimes are statically known

---

### **Law 4: Branching Is Time Noise**

#### **What This Means:**
- Every `if` statement is a potential branch misprediction
- Branch misprediction costs 15-20 cycles
- At 7T speeds, branches are too expensive

#### **How to Apply:**
```c
// ❌ DON'T: Use branches for conditionals
int process_type(int type, int value) {
    if (type == TYPE_A) {
        return value * 2;
    } else if (type == TYPE_B) {
        return value + 1;
    } else {
        return value;
    }
}

// ✅ DO: Use lookup tables
int process_type_branch_free(int type, int value) {
    static const int multipliers[] = {1, 2, 1};
    static const int adders[] = {0, 0, 1};
    
    return (value * multipliers[type]) + adders[type];
}
```

#### **Your Checklist:**
- [ ] Replace conditionals with lookup tables
- [ ] Use bit operations instead of branches
- [ ] Pre-compute all decision paths
- [ ] Avoid function pointers in hot paths

---

### **Law 5: Join Is the Atom of Logic**

#### **What This Means:**
- All meaningful computation involves joining data
- Traditional joins use nested loops (O(n²))
- 7T requires constant-time joins (O(1))

#### **How to Apply:**
```c
// ❌ DON'T: Use nested loops for joins
int find_triples_slow(Triple* triples, int count, uint32_t subject) {
    int found = 0;
    for (int i = 0; i < count; i++) {
        if (triples[i].subject == subject) {
            found++;
        }
    }
    return found;
}

// ✅ DO: Use hash joins
int find_triples_fast(EngineState* engine, uint32_t subject) {
    uint32_t hash = hash_subject(subject);
    uint32_t bucket = hash % HASH_TABLE_SIZE;
    
    // Direct hash table access
    return engine->subject_index[bucket].count;
}
```

#### **Your Checklist:**
- [ ] All joins use hash tables
- [ ] Triple patterns use direct indexing
- [ ] Batch operations for multiple joins
- [ ] No iterator abstractions

---

### **Law 6: Code Is Just a Compressed Evaluation Tree**

#### **What This Means:**
- Any logic can be represented as a tree of operations
- If the tree is known ahead of time, compile it
- Don't interpret — execute static circuits

#### **How to Apply:**
```c
// ❌ DON'T: Interpret logic at runtime
typedef struct {
    char* operation;  // String operation name
    void* operands;   // Dynamic operand list
} DynamicOp;

int execute_dynamic(DynamicOp* op) {
    if (strcmp(op->operation, "SELECT") == 0) {
        return handle_select(op->operands);
    } else if (strcmp(op->operation, "FILTER") == 0) {
        return handle_filter(op->operands);
    }
    // ... more string comparisons
}

// ✅ DO: Compile to static circuits
typedef struct {
    uint32_t operation_type;  // Enum, not string
    uint32_t operands[4];     // Fixed array
} CompiledOp;

int execute_compiled(CompiledOp* op) {
    static const OpHandler handlers[] = {
        handle_select, handle_filter, handle_join
    };
    return handlers[op->operation_type](op->operands);
}
```

#### **Your Checklist:**
- [ ] All logic is compiled ahead of time
- [ ] No string-based operation dispatch
- [ ] Static evaluation graphs
- [ ] Pre-computed execution paths

---

### **Law 7: Nanoseconds are the Only True Unit**

#### **What This Means:**
- Time is the only metric that can't be faked
- You cannot optimize away the laws of physics
- Every operation must be measured in cycles

#### **How to Apply:**
```c
// ❌ DON'T: Use approximate timing
clock_t start = clock();
operation();
clock_t end = clock();
double time = ((double)(end - start)) / CLOCKS_PER_SEC;

// ✅ DO: Use cycle-accurate timing
uint64_t start_cycles = __builtin_readcyclecounter();
operation();
uint64_t end_cycles = __builtin_readcyclecounter();
uint64_t cycles = end_cycles - start_cycles;

// Validate against physics
if (cycles > MAX_CYCLES_PER_OPERATION) {
    printf("PHYSICS VIOLATION: %llu cycles\n", cycles);
}
```

#### **Your Checklist:**
- [ ] All performance tests use cycle counting
- [ ] Validate against physics-based limits
- [ ] Continuous performance monitoring
- [ ] No approximate metrics

---

### **Law 8: All Logic is Compilation**

#### **What This Means:**
- If you can write it, you can compile it
- If you can compile it, you can analyze it
- Compile-time analysis is always cheaper than runtime

#### **How to Apply:**
```c
// ❌ DON'T: Analyze at runtime
int validate_shape_runtime(const char* shape_def, void* data) {
    // Parse shape definition at runtime
    ShapeParser* parser = create_parser(shape_def);
    
    // Interpret constraints dynamically
    for (int i = 0; i < parser->constraint_count; i++) {
        if (!interpret_constraint(&parser->constraints[i], data)) {
            return 0;
        }
    }
    return 1;
}

// ✅ DO: Compile ahead of time
typedef struct {
    uint32_t constraint_count;
    CompiledConstraint constraints[MAX_CONSTRAINTS];
} CompiledShape;

int validate_shape_compiled(CompiledShape* shape, void* data) {
    // Direct constraint validation
    for (uint32_t i = 0; i < shape->constraint_count; i++) {
        if (!validate_compiled_constraint(&shape->constraints[i], data)) {
            return 0;
        }
    }
    return 1;
}
```

#### **Your Checklist:**
- [ ] All analysis happens at compile time
- [ ] No runtime parsing or interpretation
- [ ] Pre-computed validation circuits
- [ ] Static optimization decisions

---

## 🛠️ Practical Development Guidelines

### **Code Review Checklist**

When reviewing code, ask these questions:

1. **Does this respect the speed of light?**
   - Are all memory accesses L1-cache friendly?
   - Are there any pointer chases in hot paths?

2. **Does this minimize entropy?**
   - Are strings interned?
   - Are data structures fixed-size?

3. **Does this eliminate uncertainty?**
   - Are there any dynamic allocations?
   - Are object lifetimes statically known?

4. **Does this avoid branching?**
   - Are conditionals replaced with lookup tables?
   - Are there any function pointers in hot paths?

5. **Does this use constant-time joins?**
   - Are all lookups O(1)?
   - Are hash tables used for joins?

6. **Is this compiled, not interpreted?**
   - Is logic compiled ahead of time?
   - Are there any string-based dispatches?

7. **Is this measured in cycles?**
   - Are performance tests cycle-accurate?
   - Are physics-based limits enforced?

8. **Is analysis done at compile time?**
   - Is all analysis static?
   - Are there any runtime parsers?

### **Performance Testing Requirements**

Every new feature must pass these tests:

```c
// Required performance test template
void test_feature_performance(void) {
    // Test latency
    ASSERT_PERFORMANCE({
        for (int i = 0; i < 1000; i++) {
            new_feature_operation();
        }
    }, 7000); // 7 cycles per operation * 1000 operations
    
    // Test memory usage
    ASSERT_MEMORY_USAGE({
        for (int i = 0; i < 1000; i++) {
            new_feature_operation();
        }
    }, 1024 * 1024); // 1MB limit
    
    // Test throughput
    uint64_t start = get_cycles();
    for (int i = 0; i < 1000000; i++) {
        new_feature_operation();
    }
    uint64_t end = get_cycles();
    uint64_t cycles_per_op = (end - start) / 1000000;
    ASSERT_LESS_THAN(cycles_per_op, 7);
}
```

### **Common Anti-Patterns to Avoid**

1. **Dynamic Allocation in Hot Paths**
   ```c
   // ❌ DON'T
   char* buffer = malloc(size);
   // ... use buffer ...
   free(buffer);
   
   // ✅ DO
   char buffer[MAX_SIZE];
   // ... use buffer ...
   ```

2. **String Comparisons**
   ```c
   // ❌ DON'T
   if (strcmp(name1, name2) == 0) { ... }
   
   // ✅ DO
   if (name_id1 == name_id2) { ... }
   ```

3. **Branch-Heavy Logic**
   ```c
   // ❌ DON'T
   if (type == A) return x;
   else if (type == B) return y;
   else if (type == C) return z;
   
   // ✅ DO
   static const int results[] = {x, y, z};
   return results[type];
   ```

4. **Runtime Parsing**
   ```c
   // ❌ DON'T
   void process_query(const char* query_string) {
       QueryParser* parser = parse_query(query_string);
       // ... interpret parser ...
   }
   
   // ✅ DO
   void process_compiled_query(CompiledQuery* query) {
       // ... execute compiled query ...
   }
   ```

---

## 🎯 Success Metrics

### **Performance Targets**
- **Latency**: ≤10 nanoseconds per operation
- **Throughput**: ≥100M operations per second
- **Memory**: ≤32 bytes per event
- **Cycles**: ≤7 CPU cycles per operation

### **Quality Gates**
- [ ] All unit tests pass
- [ ] All performance tests pass
- [ ] No memory leaks detected
- [ ] No physics violations
- [ ] Code review checklist completed

### **Continuous Monitoring**
- [ ] Cycle-accurate performance tracking
- [ ] Memory usage monitoring
- [ ] Physics compliance validation
- [ ] Automated regression detection

---

## 🚀 Getting Started

### **Your First Week**

1. **Read the Physics Principles** (this document)
2. **Run the Unit Tests** - See how physics compliance works
3. **Study the Code Examples** - Understand the patterns
4. **Write Your First Test** - Apply the principles
5. **Review Existing Code** - Identify physics violations

### **Your First Month**

1. **Contribute to Core Components** - Apply principles in practice
2. **Write Performance Tests** - Ensure physics compliance
3. **Optimize Existing Code** - Eliminate violations
4. **Document Patterns** - Share knowledge with team

### **Your First Quarter**

1. **Design New Features** - From physics-first principles
2. **Mentor New Team Members** - Share the physics mindset
3. **Improve the Framework** - Make physics compliance easier
4. **Push Performance Limits** - Find new physics boundaries

---

## 🎉 Welcome to Physics-Compliant Computing!

You're now part of a team that doesn't just write fast code — we write **physics-compliant code**. Every line of code we write respects the laws of nature, making it impossible to violate performance targets without violating the speed of light.

**Remember:** At nanosecond scale, you're not fighting your competitors — you're fighting the laws of physics. And physics always wins.

**"The best optimization is the one that respects the speed of light."**

Welcome to the team! 🚀 