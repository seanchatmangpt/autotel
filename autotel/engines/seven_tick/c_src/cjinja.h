#ifndef CJINJA_H
#define CJINJA_H

#include <stddef.h>
#include <sys/types.h>

// Opaque handles
typedef struct CjinjaTemplate CjinjaTemplate;
typedef struct CjinjaContext CjinjaContext;

// Core API
void cjinja_init(void);
void cjinja_shutdown(void);

CjinjaTemplate* cjinja_compile(const char* name, const char* source);
CjinjaContext* cjinja_context_create(void);
void cjinja_context_destroy(CjinjaContext* ctx);
int cjinja_context_set(CjinjaContext* ctx, const char* key, const char* value);
int cjinja_context_set_list(CjinjaContext* ctx, const char* key, const char** items, size_t count);
ssize_t cjinja_render(const CjinjaTemplate* tpl, const CjinjaContext* ctx, char* out_buf, size_t out_buf_size);
void cjinja_clear_templates(void);

#endif
