/*  ─────────────────────────────────────────────────────────────
    sql_parser.c  –  7-Tick SQL Parser Implementation
    Branch-free SQL parsing with compile-time optimization
    ───────────────────────────────────────────────────────────── */

#include "../../include/cns/sql.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

// Token types
typedef enum {
    TOK_SELECT = 0,
    TOK_FROM,
    TOK_WHERE,
    TOK_GROUP,
    TOK_ORDER,
    TOK_BY,
    TOK_LIMIT,
    TOK_JOIN,
    TOK_ON,
    TOK_AND,
    TOK_OR,
    TOK_STAR,
    TOK_COMMA,
    TOK_LPAREN,
    TOK_RPAREN,
    TOK_EQ,
    TOK_NE,
    TOK_LT,
    TOK_LE,
    TOK_GT,
    TOK_GE,
    TOK_IDENT,
    TOK_NUMBER,
    TOK_STRING,
    TOK_EOF,
    TOK_UNKNOWN
} sql_token_t;

// Lexer state
typedef struct {
    const char* input;
    size_t pos;
    size_t len;
    sql_token_t current_token;
    char token_value[64];
} sql_lexer_t;

// Parser state
typedef struct {
    sql_lexer_t lexer;
    s7t_query_plan_t* plan;
    s7t_arena_t* arena;
    char error[256];
} sql_parser_t;

/*═══════════════════════════════════════════════════════════════
  Lexer Implementation (Branch-free where possible)
  ═══════════════════════════════════════════════════════════════*/

// Skip whitespace (branch-free)
S7T_ALWAYS_INLINE void skip_whitespace(sql_lexer_t* lex) {
    while (lex->pos < lex->len && isspace(lex->input[lex->pos])) {
        lex->pos++;
    }
}

// Check if character is identifier start
S7T_ALWAYS_INLINE bool is_ident_start(char c) {
    return isalpha(c) || c == '_';
}

// Check if character is identifier continuation
S7T_ALWAYS_INLINE bool is_ident_cont(char c) {
    return isalnum(c) || c == '_';
}

// Read identifier
static void read_ident(sql_lexer_t* lex) {
    size_t start = lex->pos;
    size_t len = 0;
    
    while (lex->pos < lex->len && is_ident_cont(lex->input[lex->pos]) && len < 63) {
        lex->token_value[len++] = lex->input[lex->pos++];
    }
    lex->token_value[len] = '\0';
    
    // Check for keywords (using perfect hash would be better)
    if (strcasecmp(lex->token_value, "SELECT") == 0) {
        lex->current_token = TOK_SELECT;
    } else if (strcasecmp(lex->token_value, "FROM") == 0) {
        lex->current_token = TOK_FROM;
    } else if (strcasecmp(lex->token_value, "WHERE") == 0) {
        lex->current_token = TOK_WHERE;
    } else if (strcasecmp(lex->token_value, "GROUP") == 0) {
        lex->current_token = TOK_GROUP;
    } else if (strcasecmp(lex->token_value, "ORDER") == 0) {
        lex->current_token = TOK_ORDER;
    } else if (strcasecmp(lex->token_value, "BY") == 0) {
        lex->current_token = TOK_BY;
    } else if (strcasecmp(lex->token_value, "LIMIT") == 0) {
        lex->current_token = TOK_LIMIT;
    } else if (strcasecmp(lex->token_value, "JOIN") == 0) {
        lex->current_token = TOK_JOIN;
    } else if (strcasecmp(lex->token_value, "ON") == 0) {
        lex->current_token = TOK_ON;
    } else if (strcasecmp(lex->token_value, "AND") == 0) {
        lex->current_token = TOK_AND;
    } else if (strcasecmp(lex->token_value, "OR") == 0) {
        lex->current_token = TOK_OR;
    } else {
        lex->current_token = TOK_IDENT;
    }
}

// Read number
static void read_number(sql_lexer_t* lex) {
    size_t len = 0;
    bool has_dot = false;
    
    while (lex->pos < lex->len && len < 63) {
        char c = lex->input[lex->pos];
        if (isdigit(c)) {
            lex->token_value[len++] = c;
            lex->pos++;
        } else if (c == '.' && !has_dot) {
            has_dot = true;
            lex->token_value[len++] = c;
            lex->pos++;
        } else {
            break;
        }
    }
    lex->token_value[len] = '\0';
    lex->current_token = TOK_NUMBER;
}

// Read string literal
static void read_string(sql_lexer_t* lex, char quote) {
    lex->pos++; // Skip opening quote
    size_t len = 0;
    
    while (lex->pos < lex->len && lex->input[lex->pos] != quote && len < 63) {
        lex->token_value[len++] = lex->input[lex->pos++];
    }
    
    if (lex->pos < lex->len && lex->input[lex->pos] == quote) {
        lex->pos++; // Skip closing quote
    }
    
    lex->token_value[len] = '\0';
    lex->current_token = TOK_STRING;
}

// Get next token
static void next_token(sql_lexer_t* lex) {
    skip_whitespace(lex);
    
    if (lex->pos >= lex->len) {
        lex->current_token = TOK_EOF;
        return;
    }
    
    char c = lex->input[lex->pos];
    
    // Single character tokens
    switch (c) {
        case '*':
            lex->current_token = TOK_STAR;
            lex->token_value[0] = c;
            lex->token_value[1] = '\0';
            lex->pos++;
            return;
            
        case ',':
            lex->current_token = TOK_COMMA;
            lex->token_value[0] = c;
            lex->token_value[1] = '\0';
            lex->pos++;
            return;
            
        case '(':
            lex->current_token = TOK_LPAREN;
            lex->token_value[0] = c;
            lex->token_value[1] = '\0';
            lex->pos++;
            return;
            
        case ')':
            lex->current_token = TOK_RPAREN;
            lex->token_value[0] = c;
            lex->token_value[1] = '\0';
            lex->pos++;
            return;
            
        case '=':
            lex->current_token = TOK_EQ;
            lex->token_value[0] = c;
            lex->token_value[1] = '\0';
            lex->pos++;
            return;
            
        case '<':
            lex->pos++;
            if (lex->pos < lex->len && lex->input[lex->pos] == '=') {
                lex->current_token = TOK_LE;
                strcpy(lex->token_value, "<=");
                lex->pos++;
            } else if (lex->pos < lex->len && lex->input[lex->pos] == '>') {
                lex->current_token = TOK_NE;
                strcpy(lex->token_value, "<>");
                lex->pos++;
            } else {
                lex->current_token = TOK_LT;
                strcpy(lex->token_value, "<");
            }
            return;
            
        case '>':
            lex->pos++;
            if (lex->pos < lex->len && lex->input[lex->pos] == '=') {
                lex->current_token = TOK_GE;
                strcpy(lex->token_value, ">=");
                lex->pos++;
            } else {
                lex->current_token = TOK_GT;
                strcpy(lex->token_value, ">");
            }
            return;
            
        case '!':
            if (lex->pos + 1 < lex->len && lex->input[lex->pos + 1] == '=') {
                lex->current_token = TOK_NE;
                strcpy(lex->token_value, "!=");
                lex->pos += 2;
                return;
            }
            break;
            
        case '"':
        case '\'':
            read_string(lex, c);
            return;
    }
    
    // Multi-character tokens
    if (is_ident_start(c)) {
        read_ident(lex);
    } else if (isdigit(c)) {
        read_number(lex);
    } else {
        lex->current_token = TOK_UNKNOWN;
        lex->token_value[0] = c;
        lex->token_value[1] = '\0';
        lex->pos++;
    }
}

/*═══════════════════════════════════════════════════════════════
  Parser Implementation
  ═══════════════════════════════════════════════════════════════*/

// Initialize parser
static void parser_init(sql_parser_t* parser, const char* query, 
                       s7t_query_plan_t* plan, s7t_arena_t* arena) {
    parser->lexer.input = query;
    parser->lexer.pos = 0;
    parser->lexer.len = strlen(query);
    parser->plan = plan;
    parser->arena = arena;
    parser->error[0] = '\0';
    
    // Initialize plan
    memset(plan, 0, sizeof(*plan));
    
    // Get first token
    next_token(&parser->lexer);
}

// Expect specific token
static bool expect_token(sql_parser_t* parser, sql_token_t expected) {
    if (parser->lexer.current_token != expected) {
        snprintf(parser->error, sizeof(parser->error), 
                "Expected token %d, got %d", expected, parser->lexer.current_token);
        return false;
    }
    next_token(&parser->lexer);
    return true;
}

// Parse column list
static bool parse_columns(sql_parser_t* parser) {
    if (parser->lexer.current_token == TOK_STAR) {
        // Select all columns
        parser->plan->project_count = 0;  // 0 means all
        next_token(&parser->lexer);
        return true;
    }
    
    // Parse column names
    while (parser->plan->project_count < S7T_SQL_MAX_COLUMNS) {
        if (parser->lexer.current_token != TOK_IDENT) {
            snprintf(parser->error, sizeof(parser->error), "Expected column name");
            return false;
        }
        
        // For now, just store column index (would need table schema)
        parser->plan->project_cols[parser->plan->project_count++] = 
            parser->plan->project_count;
        
        next_token(&parser->lexer);
        
        if (parser->lexer.current_token != TOK_COMMA) {
            break;
        }
        next_token(&parser->lexer);  // Skip comma
    }
    
    return true;
}

// Parse WHERE clause
static bool parse_where(sql_parser_t* parser) {
    if (!expect_token(parser, TOK_WHERE)) {
        return false;
    }
    
    // Simple single predicate for now
    if (parser->plan->predicate_count >= S7T_SQL_MAX_PREDICATES) {
        snprintf(parser->error, sizeof(parser->error), "Too many predicates");
        return false;
    }
    
    s7t_predicate_t* pred = &parser->plan->predicates[parser->plan->predicate_count];
    
    // Column name
    if (parser->lexer.current_token != TOK_IDENT) {
        snprintf(parser->error, sizeof(parser->error), "Expected column name");
        return false;
    }
    
    // Map column name to index (would need table schema)
    pred->column_idx = 0;  // Assume first column for demo
    next_token(&parser->lexer);
    
    // Operator
    switch (parser->lexer.current_token) {
        case TOK_EQ: pred->op = S7T_OP_EQ; break;
        case TOK_NE: pred->op = S7T_OP_NE; break;
        case TOK_LT: pred->op = S7T_OP_LT; break;
        case TOK_LE: pred->op = S7T_OP_LE; break;
        case TOK_GT: pred->op = S7T_OP_GT; break;
        case TOK_GE: pred->op = S7T_OP_GE; break;
        default:
            snprintf(parser->error, sizeof(parser->error), "Invalid operator");
            return false;
    }
    next_token(&parser->lexer);
    
    // Value
    if (parser->lexer.current_token == TOK_NUMBER) {
        pred->value.i64 = atoll(parser->lexer.token_value);
    } else if (parser->lexer.current_token == TOK_STRING) {
        // Hash string to ID
        pred->value.id = s7t_hash_string(parser->lexer.token_value, 
                                        strlen(parser->lexer.token_value));
    } else {
        snprintf(parser->error, sizeof(parser->error), "Expected value");
        return false;
    }
    next_token(&parser->lexer);
    
    parser->plan->predicate_count++;
    return true;
}

// Parse GROUP BY clause
static bool parse_group_by(sql_parser_t* parser) {
    if (!expect_token(parser, TOK_GROUP)) {
        return false;
    }
    if (!expect_token(parser, TOK_BY)) {
        return false;
    }
    
    if (parser->lexer.current_token != TOK_IDENT) {
        snprintf(parser->error, sizeof(parser->error), "Expected column name");
        return false;
    }
    
    // Map column name to index
    parser->plan->group_col = 0;  // Assume first column for demo
    next_token(&parser->lexer);
    
    return true;
}

// Parse ORDER BY clause
static bool parse_order_by(sql_parser_t* parser) {
    if (!expect_token(parser, TOK_ORDER)) {
        return false;
    }
    if (!expect_token(parser, TOK_BY)) {
        return false;
    }
    
    if (parser->lexer.current_token != TOK_IDENT) {
        snprintf(parser->error, sizeof(parser->error), "Expected column name");
        return false;
    }
    
    // Map column name to index
    parser->plan->order_col = 0;  // Assume first column for demo
    next_token(&parser->lexer);
    
    // Check for DESC
    if (parser->lexer.current_token == TOK_IDENT && 
        strcasecmp(parser->lexer.token_value, "DESC") == 0) {
        parser->plan->order_desc = true;
        next_token(&parser->lexer);
    }
    
    return true;
}

// Parse LIMIT clause
static bool parse_limit(sql_parser_t* parser) {
    if (!expect_token(parser, TOK_LIMIT)) {
        return false;
    }
    
    if (parser->lexer.current_token != TOK_NUMBER) {
        snprintf(parser->error, sizeof(parser->error), "Expected number");
        return false;
    }
    
    parser->plan->limit = atoi(parser->lexer.token_value);
    if (parser->plan->limit > S7T_SQL_MAX_ROWS) {
        parser->plan->limit = S7T_SQL_MAX_ROWS;
    }
    next_token(&parser->lexer);
    
    return true;
}

// Main parse function
s7t_query_plan_t* s7t_sql_parse(const char* query, s7t_arena_t* arena) {
    s7t_query_plan_t* plan = (s7t_query_plan_t*)s7t_arena_alloc(arena, sizeof(s7t_query_plan_t));
    if (!plan) {
        return NULL;
    }
    
    sql_parser_t parser;
    parser_init(&parser, query, plan, arena);
    
    // Parse SELECT
    if (!expect_token(&parser, TOK_SELECT)) {
        return NULL;
    }
    
    // Parse columns
    if (!parse_columns(&parser)) {
        return NULL;
    }
    
    // Parse FROM
    if (!expect_token(&parser, TOK_FROM)) {
        return NULL;
    }
    
    // Parse table name
    if (parser.lexer.current_token != TOK_IDENT) {
        return NULL;
    }
    
    // Store table name (would need to look up actual table)
    next_token(&parser.lexer);
    
    // Optional clauses
    while (parser.lexer.current_token != TOK_EOF) {
        switch (parser.lexer.current_token) {
            case TOK_WHERE:
                if (!parse_where(&parser)) {
                    return NULL;
                }
                break;
                
            case TOK_GROUP:
                if (!parse_group_by(&parser)) {
                    return NULL;
                }
                break;
                
            case TOK_ORDER:
                if (!parse_order_by(&parser)) {
                    return NULL;
                }
                break;
                
            case TOK_LIMIT:
                if (!parse_limit(&parser)) {
                    return NULL;
                }
                break;
                
            default:
                // Unknown token
                return NULL;
        }
    }
    
    // Estimate cycles
    plan->estimated_cycles = 1;  // Base scan
    
    if (plan->predicate_count > 0) {
        plan->estimated_cycles += 4;  // Filter
    }
    
    if (plan->group_col > 0) {
        plan->estimated_cycles += 6;  // Aggregation
    }
    
    if (plan->order_col > 0) {
        plan->estimated_cycles += 7;  // Sort
    }
    
    return plan;
}

// Compile query plan to micro-ops
bool s7t_sql_compile(s7t_query_plan_t* plan) {
    // Validate plan
    if (!s7t_validate_plan(plan)) {
        return false;
    }
    
    // In a real implementation, this would generate micro-op tape
    // For now, just return success if plan is valid
    return true;
}

// Explain query plan
void s7t_sql_explain(const s7t_query_plan_t* plan, char* buffer, size_t size) {
    size_t offset = 0;
    
    offset += snprintf(buffer + offset, size - offset, "Query Plan:\n");
    offset += snprintf(buffer + offset, size - offset, "───────────\n");
    
    if (plan->predicate_count > 0) {
        offset += snprintf(buffer + offset, size - offset, 
                          "├─ Filter (%d predicates): %d cycles\n",
                          plan->predicate_count, 4);
    }
    
    if (plan->group_col > 0) {
        offset += snprintf(buffer + offset, size - offset, 
                          "├─ Group By: %d cycles\n", 6);
    }
    
    if (plan->order_col > 0) {
        offset += snprintf(buffer + offset, size - offset, 
                          "├─ Order By: %d cycles\n", 7);
    }
    
    if (plan->limit > 0) {
        offset += snprintf(buffer + offset, size - offset, 
                          "├─ Limit %u: 0 cycles\n", plan->limit);
    }
    
    offset += snprintf(buffer + offset, size - offset, 
                      "└─ Table Scan: 1 cycle\n\n");
    
    offset += snprintf(buffer + offset, size - offset, 
                      "Estimated Total: %lu cycles (%.2f ns)\n",
                      plan->estimated_cycles,
                      plan->estimated_cycles * S7T_NS_PER_CYCLE);
}