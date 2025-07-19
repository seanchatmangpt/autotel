/*
 * LEAN SIX SIGMA COMPILER - 80/20 Design
 * Quality Target: 6σ (3.4 DPMO)
 * Performance Target: ≤7 CPU cycles per operation
 */

#ifndef LEAN_SIGMA_COMPILER_H
#define LEAN_SIGMA_COMPILER_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// SIX SIGMA QUALITY METRICS
// ============================================================================

// Defect tracking (6σ = 3.4 defects per million opportunities)
typedef struct {
    uint64_t opportunities;           // Total compilation operations
    uint64_t defects;                // Compilation failures
    double defect_rate;              // Current DPMO
    double sigma_level;              // Current sigma level
} SixSigmaMetrics;

// Performance tracking (7-tick compliance)
typedef struct {
    uint64_t cycles_lexer;           // Lexical analysis cycles
    uint64_t cycles_parser;          // Parsing cycles  
    uint64_t cycles_semantic;        // Semantic analysis cycles
    uint64_t cycles_codegen;         // Code generation cycles
    uint64_t cycles_total;           // Total compilation cycles
    bool seven_tick_compliant;       // ≤7 cycles per operation
} PerformanceMetrics;

// ============================================================================
// 80/20 LEAN LEXER - Focus on High-Frequency Tokens
// ============================================================================

// 80/20 Token types (20% of tokens used 80% of time)
typedef enum {
    // CORE TOKENS (80% usage) - Optimized with SIMD
    TOK_IDENTIFIER = 0,              // Variables, functions (35%)
    TOK_NUMBER,                      // Literals (20%)
    TOK_OPERATOR,                    // +, -, *, / (15%)
    TOK_KEYWORD,                     // int, if, while (10%)
    
    // SECONDARY TOKENS (20% usage) - Standard processing  
    TOK_DELIMITER,                   // {, }, (, ), ; (15%)
    TOK_STRING,                      // String literals (3%)
    TOK_COMMENT,                     // Comments (2%)
    TOK_EOF,                         // End of file
    TOK_ERROR                        // Lexical errors
} TokenType;

// Lean token structure (cache-aligned)
typedef struct {
    TokenType type;                  // Token type
    uint32_t hash;                   // Fast comparison hash
    uint16_t length;                 // Token length
    uint16_t line;                   // Source line (for error reporting)
    const char* text;                // Token text (zero-copy)
} LeanToken __attribute__((aligned(32)));

// 80/20 Lexer context
typedef struct {
    const char* source;              // Source code
    uint32_t position;               // Current position
    uint32_t line;                   // Current line
    LeanToken current_token;         // Current token
    SixSigmaMetrics quality;         // Quality metrics
    PerformanceMetrics perf;         // Performance metrics
} LeanLexer;

// ============================================================================
// 80/20 LEAN PARSER - Focus on Common Constructs
// ============================================================================

// 80/20 AST Node types (20% of constructs used 80% of time)
typedef enum {
    // CORE CONSTRUCTS (80% usage)
    AST_VARIABLE,                    // Variable declarations (30%)
    AST_ASSIGNMENT,                  // Assignments (25%)
    AST_FUNCTION_CALL,               // Function calls (15%)
    AST_BINARY_OP,                   // Binary operations (10%)
    
    // SECONDARY CONSTRUCTS (20% usage)
    AST_IF_STMT,                     // If statements (8%)
    AST_WHILE_LOOP,                  // While loops (5%)
    AST_FUNCTION_DEF,                // Function definitions (4%)
    AST_RETURN_STMT,                 // Return statements (3%)
    AST_BLOCK,                       // Code blocks
    AST_ERROR                        // Parse errors
} ASTNodeType;

// Lean AST node (minimized for cache efficiency)
typedef struct ASTNode {
    ASTNodeType type;                // Node type
    uint16_t child_count;            // Number of children
    struct ASTNode** children;       // Child nodes
    LeanToken token;                 // Associated token
} ASTNode __attribute__((aligned(32)));

// 80/20 Parser context
typedef struct {
    LeanLexer* lexer;                // Lexer instance
    ASTNode* root;                   // Parse tree root
    uint32_t error_count;            // Parse error count
    SixSigmaMetrics quality;         // Quality metrics
    PerformanceMetrics perf;         // Performance metrics
} LeanParser;

// ============================================================================
// 80/20 LEAN SEMANTIC ANALYZER - Focus on Common Type Checks
// ============================================================================

// 80/20 Type system (20% of types handle 80% of code)
typedef enum {
    // CORE TYPES (80% usage)
    TYPE_INT = 0,                    // Integer (40%)
    TYPE_FLOAT,                      // Floating point (20%)
    TYPE_CHAR,                       // Character (15%)
    TYPE_POINTER,                    // Pointer (5%)
    
    // SECONDARY TYPES (20% usage)
    TYPE_ARRAY,                      // Arrays (10%)
    TYPE_STRUCT,                     // Structures (5%)
    TYPE_FUNCTION,                   // Function types (3%)
    TYPE_VOID,                       // Void type (2%)
    TYPE_ERROR                       // Type errors
} LeanType;

// Lean symbol table entry
typedef struct {
    uint32_t name_hash;              // Symbol name hash
    LeanType type;                   // Symbol type
    uint32_t scope_level;            // Scope nesting level
    bool is_defined;                 // Definition status
} SymbolEntry __attribute__((aligned(16)));

// 80/20 Semantic analyzer
typedef struct {
    ASTNode* ast;                    // Parse tree
    SymbolEntry* symbols;            // Symbol table
    uint32_t symbol_count;           // Number of symbols
    uint32_t scope_level;            // Current scope
    SixSigmaMetrics quality;         // Quality metrics
    PerformanceMetrics perf;         // Performance metrics
} LeanSemanticAnalyzer;

// ============================================================================
// 80/20 LEAN IR - Minimal Instruction Set
// ============================================================================

// 80/20 IR instructions (20% of instructions handle 80% of operations)
typedef enum {
    // CORE INSTRUCTIONS (80% usage)
    IR_LOAD = 0,                     // Load from memory (25%)
    IR_STORE,                        // Store to memory (20%)
    IR_ADD,                          // Addition (15%)
    IR_SUB,                          // Subtraction (10%)
    IR_MUL,                          // Multiplication (5%)
    IR_DIV,                          // Division (5%)
    
    // SECONDARY INSTRUCTIONS (20% usage)
    IR_BRANCH,                       // Conditional branch (8%)
    IR_JUMP,                         // Unconditional jump (5%)
    IR_CALL,                         // Function call (4%)
    IR_RETURN,                       // Function return (3%)
    IR_NOP                           // No operation
} IROpcode;

// Lean IR instruction (optimized layout)
typedef struct {
    IROpcode opcode;                 // Instruction type
    uint16_t dest;                   // Destination register
    uint16_t src1;                   // Source register 1
    uint16_t src2;                   // Source register 2
    uint32_t immediate;              // Immediate value
} IRInstruction __attribute__((aligned(16)));

// 80/20 IR generator
typedef struct {
    IRInstruction* instructions;     // Instruction array
    uint32_t instruction_count;      // Number of instructions
    uint32_t register_count;         // Virtual registers used
    SixSigmaMetrics quality;         // Quality metrics
    PerformanceMetrics perf;         // Performance metrics
} LeanIRGenerator;

// ============================================================================
// 80/20 LEAN CODE GENERATOR - Focus on Common Patterns
// ============================================================================

// 80/20 Target instructions (ARM64 focus)
typedef enum {
    // CORE INSTRUCTIONS (80% usage)
    ARM_LDR = 0,                     // Load register (30%)
    ARM_STR,                         // Store register (25%)
    ARM_ADD,                         // Add (15%)
    ARM_SUB,                         // Subtract (10%)
    
    // SECONDARY INSTRUCTIONS (20% usage)
    ARM_MUL,                         // Multiply (8%)
    ARM_B,                           // Branch (5%)
    ARM_BL,                          // Branch with link (4%)
    ARM_RET                          // Return (3%)
} ARMOpcode;

// Lean code generator
typedef struct {
    uint8_t* machine_code;           // Generated machine code
    uint32_t code_size;              // Code size in bytes
    uint32_t* relocations;           // Relocation table
    uint32_t relocation_count;       // Number of relocations
    SixSigmaMetrics quality;         // Quality metrics
    PerformanceMetrics perf;         // Performance metrics
} LeanCodeGenerator;

// ============================================================================
// LEAN SIX SIGMA COMPILER - Main Interface
// ============================================================================

typedef struct {
    LeanLexer lexer;                 // Lexical analyzer
    LeanParser parser;               // Parser
    LeanSemanticAnalyzer semantic;   // Semantic analyzer  
    LeanIRGenerator ir_gen;          // IR generator
    LeanCodeGenerator code_gen;      // Code generator
    
    // Six Sigma quality tracking
    SixSigmaMetrics overall_quality; // Overall quality metrics
    PerformanceMetrics overall_perf; // Overall performance
    
    // 80/20 optimization flags
    bool enable_80_20_lexer;         // Enable 80/20 lexer optimizations
    bool enable_80_20_parser;        // Enable 80/20 parser optimizations
    bool enable_simd_optimization;   // Enable SIMD optimizations
    bool enable_quality_tracking;    // Enable Six Sigma tracking
} LeanSigmaCompiler;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

// Compiler lifecycle
LeanSigmaCompiler* lean_compiler_create(void);
void lean_compiler_destroy(LeanSigmaCompiler* compiler);
int lean_compile_source(LeanSigmaCompiler* compiler, const char* source);

// 80/20 Lexer
int lean_lexer_init(LeanLexer* lexer, const char* source);
int lean_lexer_next_token(LeanLexer* lexer);
void lean_lexer_destroy(LeanLexer* lexer);

// 80/20 Parser  
int lean_parser_init(LeanParser* parser, LeanLexer* lexer);
ASTNode* lean_parser_parse(LeanParser* parser);
void lean_parser_destroy(LeanParser* parser);

// 80/20 Semantic Analyzer
int lean_semantic_init(LeanSemanticAnalyzer* analyzer, ASTNode* ast);
int lean_semantic_analyze(LeanSemanticAnalyzer* analyzer);
void lean_semantic_destroy(LeanSemanticAnalyzer* analyzer);

// 80/20 IR Generator
int lean_ir_init(LeanIRGenerator* generator);
int lean_ir_generate(LeanIRGenerator* generator, ASTNode* ast);
void lean_ir_destroy(LeanIRGenerator* generator);

// 80/20 Code Generator
int lean_codegen_init(LeanCodeGenerator* generator);
int lean_codegen_generate(LeanCodeGenerator* generator, IRInstruction* ir, uint32_t count);
void lean_codegen_destroy(LeanCodeGenerator* generator);

// Six Sigma Quality Functions
void six_sigma_init_metrics(SixSigmaMetrics* metrics);
void six_sigma_record_opportunity(SixSigmaMetrics* metrics);
void six_sigma_record_defect(SixSigmaMetrics* metrics);
double six_sigma_calculate_dpmo(SixSigmaMetrics* metrics);
double six_sigma_calculate_sigma_level(SixSigmaMetrics* metrics);

// Performance Tracking Functions  
void perf_init_metrics(PerformanceMetrics* metrics);
void perf_start_timing(PerformanceMetrics* metrics);
void perf_end_timing(PerformanceMetrics* metrics, uint64_t* target_counter);
bool perf_check_seven_tick_compliance(PerformanceMetrics* metrics, uint32_t operations);

#endif // LEAN_SIGMA_COMPILER_H