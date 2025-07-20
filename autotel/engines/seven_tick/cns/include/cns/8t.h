#ifndef CNS_8T_H
#define CNS_8T_H

// Main 8T header - includes all necessary components

#include "8t/types.h"
#include "8t/core.h"
#include "8t/interfaces.h"

// Version information
#define T8_VERSION_MAJOR 1
#define T8_VERSION_MINOR 0
#define T8_VERSION_PATCH 0
#define T8_VERSION_STRING "1.0.0"

// Feature flags
#define T8_FEATURE_SIMD 1
#define T8_FEATURE_CACHE 1
#define T8_FEATURE_PARALLEL 1

// Configuration defaults
#define T8_DEFAULT_CACHE_SIZE 1024
#define T8_DEFAULT_BATCH_SIZE 64
#define T8_DEFAULT_THREAD_COUNT 4

#endif // CNS_8T_H