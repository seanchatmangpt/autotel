#ifndef CNS_HANDLERS_H
#define CNS_HANDLERS_H

#include "cns.h"

// Command handler function type
typedef int (*cns_handler_fn)(void* ctx, int argc, char** argv, void (*output)(const char*, ...));

// Core cognitive command handlers
int cmd_spin(void* ctx, int argc, char** argv, void (*output)(const char*, ...));
int cmd_think(void* ctx, int argc, char** argv, void (*output)(const char*, ...));
int cmd_reflect(void* ctx, int argc, char** argv, void (*output)(const char*, ...));
int cmd_learn(void* ctx, int argc, char** argv, void (*output)(const char*, ...));
int cmd_adapt(void* ctx, int argc, char** argv, void (*output)(const char*, ...));

// System command handlers (stubs for now)
int cmd_sync(void* ctx, int argc, char** argv, void (*output)(const char*, ...));
int cmd_flow(void* ctx, int argc, char** argv, void (*output)(const char*, ...));
int cmd_measure(void* ctx, int argc, char** argv, void (*output)(const char*, ...));
int cmd_query(void* ctx, int argc, char** argv, void (*output)(const char*, ...));
int cmd_transform(void* ctx, int argc, char** argv, void (*output)(const char*, ...));
int cmd_optimize(void* ctx, int argc, char** argv, void (*output)(const char*, ...));
int cmd_validate(void* ctx, int argc, char** argv, void (*output)(const char*, ...));

// Benchmark and ML command handlers
int cmd_benchmark(void* ctx, int argc, char** argv, void (*output)(const char*, ...));
int cmd_ml(void* ctx, int argc, char** argv, void (*output)(const char*, ...));
int cmd_pm(void* ctx, int argc, char** argv, void (*output)(const char*, ...));

// Trace command handler
int cmd_trace(void* ctx, int argc, char** argv, void (*output)(const char*, ...));

// Help and system command handlers
int cmd_help(void* ctx, int argc, char** argv, void (*output)(const char*, ...));
int cmd_version(void* ctx, int argc, char** argv, void (*output)(const char*, ...));
int cmd_exit(void* ctx, int argc, char** argv, void (*output)(const char*, ...));

#endif // CNS_HANDLERS_H