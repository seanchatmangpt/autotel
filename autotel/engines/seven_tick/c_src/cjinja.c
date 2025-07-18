#include "cjinja.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// Opcodes for bytecode VM
typedef enum {
    OP_TEXT,     // Literal text chunk
    OP_VAR,      // Variable substitution
    OP_IF,       // Conditional start
    OP_ENDIF,    // Conditional end
    OP_FOR,      // Loop start
    OP_ENDFOR,   // Loop end
    OP_INCLUDE,  // Include template
    OP_RAW,      // Raw block start
    OP_ENDRAW    // Raw block end
} Opcode;

// Bytecode instruction
typedef struct {
    Opcode op;
    union {
        struct { const char* text; size_t len; } text;
        struct { const char* name; } var;
        struct { const char* expr; size_t jump_offset; } cond;
        struct { const char* var; const char* list; size_t jump_offset; } loop;
        struct { const char* path; } include;
    } data;
} Instruction;

// Compiled template
struct CjinjaTemplate {
    char* name;
    Instruction* bytecode;
    size_t bytecode_len;
    size_t bytecode_cap;
};

// Context value
typedef struct {
    enum { VAL_STRING, VAL_LIST } type;
    union {
        char* string;
        struct { const char** items; size_t count; } list;
    } data;
} ContextValue;

// Simple hash table for context
#define CONTEXT_BUCKETS 64
typedef struct ContextEntry {
    char* key;
    ContextValue value;
    struct ContextEntry* next;
} ContextEntry;

struct CjinjaContext {
    ContextEntry* buckets[CONTEXT_BUCKETS];
};

// Global template cache
static struct {
    CjinjaTemplate** templates;
    size_t count;
    size_t capacity;
} g_template_cache = {0};

// Initialize
void cjinja_init(void) {
    g_template_cache.capacity = 16;
    g_template_cache.templates = calloc(g_template_cache.capacity, sizeof(CjinjaTemplate*));
}

void cjinja_shutdown(void) {
    cjinja_clear_templates();
    free(g_template_cache.templates);
    g_template_cache.templates = NULL;
}

// Simple hash function
static unsigned hash_string(const char* str) {
    unsigned hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c;
    return hash;
}

// Lexer states
typedef enum {
    LEX_TEXT,
    LEX_VAR_START,
    LEX_TAG_START
} LexState;

// Add instruction to bytecode
static void emit_instruction(CjinjaTemplate* tpl, Instruction inst) {
    if (tpl->bytecode_len >= tpl->bytecode_cap) {
        tpl->bytecode_cap = tpl->bytecode_cap ? tpl->bytecode_cap * 2 : 16;
        tpl->bytecode = realloc(tpl->bytecode, tpl->bytecode_cap * sizeof(Instruction));
    }
    tpl->bytecode[tpl->bytecode_len++] = inst;
}

// Parse template to bytecode
CjinjaTemplate* cjinja_compile(const char* name, const char* source) {
    CjinjaTemplate* tpl = calloc(1, sizeof(CjinjaTemplate));
    tpl->name = strdup(name);
    
    const char* p = source;
    const char* text_start = p;
    LexState state = LEX_TEXT;
    
    while (*p) {
        if (state == LEX_TEXT) {
            if (p[0] == '{' && p[1] == '{') {
                // Emit accumulated text
                if (p > text_start) {
                    Instruction inst = {
                        .op = OP_TEXT,
                        .data.text = { .text = text_start, .len = p - text_start }
                    };
                    emit_instruction(tpl, inst);
                }
                p += 2;
                while (*p == ' ') p++;
                const char* var_start = p;
                while (*p && !(p[0] == '}' && p[1] == '}')) p++;
                if (!*p) abort(); // Unclosed {{
                
                // Emit variable
                char* var_name = strndup(var_start, p - var_start);
                while (var_name[strlen(var_name)-1] == ' ') 
                    var_name[strlen(var_name)-1] = '\0';
                
                Instruction inst = {
                    .op = OP_VAR,
                    .data.var = { .name = var_name }
                };
                emit_instruction(tpl, inst);
                
                p += 2;
                text_start = p;
            }
            else if (p[0] == '{' && p[1] == '%') {
                // Emit accumulated text
                if (p > text_start) {
                    Instruction inst = {
                        .op = OP_TEXT,
                        .data.text = { .text = text_start, .len = p - text_start }
                    };
                    emit_instruction(tpl, inst);
                }
                
                p += 2;
                while (*p == ' ') p++;
                
                // Parse tag
                if (strncmp(p, "if ", 3) == 0) {
                    p += 3;
                    const char* expr_start = p;
                    while (*p && !(p[0] == '%' && p[1] == '}')) p++;
                    if (!*p) abort(); // Unclosed {%
                    
                    char* expr = strndup(expr_start, p - expr_start);
                    while (expr[strlen(expr)-1] == ' ')
                        expr[strlen(expr)-1] = '\0';
                    
                    Instruction inst = {
                        .op = OP_IF,
                        .data.cond = { .expr = expr, .jump_offset = 0 }
                    };
                    emit_instruction(tpl, inst);
                    
                    p += 2;
                    text_start = p;
                }
                else if (strncmp(p, "endif", 5) == 0) {
                    p += 5;
                    while (*p && !(p[0] == '%' && p[1] == '}')) p++;
                    if (!*p) abort();
                    
                    Instruction inst = { .op = OP_ENDIF };
                    emit_instruction(tpl, inst);
                    
                    p += 2;
                    text_start = p;
                }
                else if (strncmp(p, "for ", 4) == 0) {
                    p += 4;
                    const char* var_start = p;
                    while (*p && *p != ' ') p++;
                    char* var = strndup(var_start, p - var_start);
                    
                    while (*p == ' ') p++;
                    if (strncmp(p, "in ", 3) != 0) abort();
                    p += 3;
                    
                    const char* list_start = p;
                    while (*p && !(p[0] == '%' && p[1] == '}')) p++;
                    if (!*p) abort();
                    
                    char* list = strndup(list_start, p - list_start);
                    while (list[strlen(list)-1] == ' ')
                        list[strlen(list)-1] = '\0';
                    
                    Instruction inst = {
                        .op = OP_FOR,
                        .data.loop = { .var = var, .list = list, .jump_offset = 0 }
                    };
                    emit_instruction(tpl, inst);
                    
                    p += 2;
                    text_start = p;
                }
                else if (strncmp(p, "endfor", 6) == 0) {
                    p += 6;
                    while (*p && !(p[0] == '%' && p[1] == '}')) p++;
                    if (!*p) abort();
                    
                    Instruction inst = { .op = OP_ENDFOR };
                    emit_instruction(tpl, inst);
                    
                    p += 2;
                    text_start = p;
                }
                else {
                    abort(); // Unknown tag
                }
            }
            else {
                p++;
            }
        }
    }
    
    // Emit final text
    if (p > text_start) {
        Instruction inst = {
            .op = OP_TEXT,
            .data.text = { .text = text_start, .len = p - text_start }
        };
        emit_instruction(tpl, inst);
    }
    
    // Fix jump offsets
    for (size_t i = 0; i < tpl->bytecode_len; i++) {
        if (tpl->bytecode[i].op == OP_IF || tpl->bytecode[i].op == OP_FOR) {
            size_t depth = 1;
            size_t j = i + 1;
            Opcode end_op = (tpl->bytecode[i].op == OP_IF) ? OP_ENDIF : OP_ENDFOR;
            
            while (j < tpl->bytecode_len && depth > 0) {
                if (tpl->bytecode[j].op == tpl->bytecode[i].op) depth++;
                else if (tpl->bytecode[j].op == end_op) depth--;
                if (depth == 0) break;
                j++;
            }
            
            if (tpl->bytecode[i].op == OP_IF)
                tpl->bytecode[i].data.cond.jump_offset = j;
            else
                tpl->bytecode[i].data.loop.jump_offset = j;
        }
    }
    
    // Add to cache
    if (g_template_cache.count >= g_template_cache.capacity) {
        g_template_cache.capacity *= 2;
        g_template_cache.templates = realloc(g_template_cache.templates,
                                           g_template_cache.capacity * sizeof(CjinjaTemplate*));
    }
    g_template_cache.templates[g_template_cache.count++] = tpl;
    
    return tpl;
}

// Context management
CjinjaContext* cjinja_context_create(void) {
    return calloc(1, sizeof(CjinjaContext));
}

void cjinja_context_destroy(CjinjaContext* ctx) {
    for (int i = 0; i < CONTEXT_BUCKETS; i++) {
        ContextEntry* entry = ctx->buckets[i];
        while (entry) {
            ContextEntry* next = entry->next;
            free(entry->key);
            if (entry->value.type == VAL_STRING)
                free(entry->value.data.string);
            free(entry);
            entry = next;
        }
    }
    free(ctx);
}

int cjinja_context_set(CjinjaContext* ctx, const char* key, const char* value) {
    unsigned bucket = hash_string(key) % CONTEXT_BUCKETS;
    
    ContextEntry* entry = calloc(1, sizeof(ContextEntry));
    entry->key = strdup(key);
    entry->value.type = VAL_STRING;
    entry->value.data.string = strdup(value);
    entry->next = ctx->buckets[bucket];
    ctx->buckets[bucket] = entry;
    
    return 0;
}

int cjinja_context_set_list(CjinjaContext* ctx, const char* key, const char** items, size_t count) {
    unsigned bucket = hash_string(key) % CONTEXT_BUCKETS;
    
    ContextEntry* entry = calloc(1, sizeof(ContextEntry));
    entry->key = strdup(key);
    entry->value.type = VAL_LIST;
    entry->value.data.list.items = items;
    entry->value.data.list.count = count;
    entry->next = ctx->buckets[bucket];
    ctx->buckets[bucket] = entry;
    
    return 0;
}

// Lookup in context
static ContextValue* context_get(const CjinjaContext* ctx, const char* key) {
    unsigned bucket = hash_string(key) % CONTEXT_BUCKETS;
    ContextEntry* entry = ctx->buckets[bucket];
    
    while (entry) {
        if (strcmp(entry->key, key) == 0)
            return &entry->value;
        entry = entry->next;
    }
    return NULL;
}

// Simple expression evaluation
static int eval_expr(const CjinjaContext* ctx, const char* expr) {
    // For now, just check if variable exists and is truthy
    ContextValue* val = context_get(ctx, expr);
    if (!val) return 0;
    
    if (val->type == VAL_STRING)
        return val->data.string && val->data.string[0];
    else
        return val->data.list.count > 0;
}

// Render template
ssize_t cjinja_render(const CjinjaTemplate* tpl, const CjinjaContext* ctx,
                     char* out_buf, size_t out_buf_size) {
    size_t out_pos = 0;
    size_t pc = 0; // Program counter
    
    // Loop state
    struct {
        const char* var;
        const char** items;
        size_t count;
        size_t index;
        size_t loop_start;
    } loop_stack[16];
    int loop_depth = 0;
    
    while (pc < tpl->bytecode_len) {
        Instruction* inst = &tpl->bytecode[pc];
        
        switch (inst->op) {
            case OP_TEXT: {
                size_t len = inst->data.text.len;
                if (out_pos + len >= out_buf_size) return -1;
                memcpy(out_buf + out_pos, inst->data.text.text, len);
                out_pos += len;
                pc++;
                break;
            }
            
            case OP_VAR: {
                // Set loop variable if in loop
                if (loop_depth > 0 && strcmp(inst->data.var.name, 
                                           loop_stack[loop_depth-1].var) == 0) {
                    const char* value = loop_stack[loop_depth-1].items[loop_stack[loop_depth-1].index];
                    size_t len = strlen(value);
                    if (out_pos + len >= out_buf_size) return -1;
                    memcpy(out_buf + out_pos, value, len);
                    out_pos += len;
                } else {
                    ContextValue* val = context_get(ctx, inst->data.var.name);
                    if (val && val->type == VAL_STRING) {
                        size_t len = strlen(val->data.string);
                        if (out_pos + len >= out_buf_size) return -1;
                        memcpy(out_buf + out_pos, val->data.string, len);
                        out_pos += len;
                    }
                }
                pc++;
                break;
            }
            
            case OP_IF: {
                if (!eval_expr(ctx, inst->data.cond.expr)) {
                    pc = inst->data.cond.jump_offset;
                } else {
                    pc++;
                }
                break;
            }
            
            case OP_ENDIF: {
                pc++;
                break;
            }
            
            case OP_FOR: {
                ContextValue* val = context_get(ctx, inst->data.loop.list);
                if (val && val->type == VAL_LIST && val->data.list.count > 0) {
                    loop_stack[loop_depth].var = inst->data.loop.var;
                    loop_stack[loop_depth].items = val->data.list.items;
                    loop_stack[loop_depth].count = val->data.list.count;
                    loop_stack[loop_depth].index = 0;
                    loop_stack[loop_depth].loop_start = pc;
                    loop_depth++;
                    pc++;
                } else {
                    pc = inst->data.loop.jump_offset;
                }
                break;
            }
            
            case OP_ENDFOR: {
                if (loop_depth > 0) {
                    loop_stack[loop_depth-1].index++;
                    if (loop_stack[loop_depth-1].index < loop_stack[loop_depth-1].count) {
                        pc = loop_stack[loop_depth-1].loop_start + 1;
                    } else {
                        loop_depth--;
                        pc++;
                    }
                } else {
                    pc++;
                }
                break;
            }
            
            default:
                pc++;
        }
    }
    
    out_buf[out_pos] = '\0';
    return out_pos;
}

void cjinja_clear_templates(void) {
    for (size_t i = 0; i < g_template_cache.count; i++) {
        CjinjaTemplate* tpl = g_template_cache.templates[i];
        free(tpl->name);
        
        for (size_t j = 0; j < tpl->bytecode_len; j++) {
            Instruction* inst = &tpl->bytecode[j];
            if (inst->op == OP_VAR)
                free((char*)inst->data.var.name);
            else if (inst->op == OP_IF)
                free((char*)inst->data.cond.expr);
            else if (inst->op == OP_FOR) {
                free((char*)inst->data.loop.var);
                free((char*)inst->data.loop.list);
            }
        }
        
        free(tpl->bytecode);
        free(tpl);
    }
    g_template_cache.count = 0;
}
