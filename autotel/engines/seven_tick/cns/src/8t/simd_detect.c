#include "cns/8t/8t.h"

#ifdef CNS_8T_X86_64
#include <cpuid.h>
#endif

// ============================================================================
// SIMD CAPABILITY DETECTION
// ============================================================================

cns_8t_simd_caps_t cns_8t_detect_simd_capabilities(void) {
    cns_8t_simd_caps_t caps = {0};
    
#ifdef CNS_8T_X86_64
    unsigned int eax, ebx, ecx, edx;
    
    // Check if CPUID is supported
    if (__get_cpuid_max(0, NULL) >= 1) {
        // Get feature flags from CPUID
        __cpuid(1, eax, ebx, ecx, edx);
        
        // SSE2 support (bit 26 of EDX)
        caps.sse2 = (edx & (1 << 26)) != 0;
        
        // AVX support (bit 28 of ECX)
        caps.avx = (ecx & (1 << 28)) != 0;
        
        // FMA support (bit 12 of ECX)
        caps.fma = (ecx & (1 << 12)) != 0;
        
        // Check extended features (EAX=7, ECX=0)
        if (__get_cpuid_max(0, NULL) >= 7) {
            __cpuid_count(7, 0, eax, ebx, ecx, edx);
            
            // AVX2 support (bit 5 of EBX)
            caps.avx2 = (ebx & (1 << 5)) != 0;
            
            // AVX-512F support (bit 16 of EBX)
            caps.avx512f = (ebx & (1 << 16)) != 0;
            
            // AVX-512DQ support (bit 17 of EBX)
            caps.avx512dq = (ebx & (1 << 17)) != 0;
        }
    }
#elif defined(CNS_8T_ARM64)
    // ARM NEON is standard on AArch64
    caps.sse2 = true;  // Use as NEON equivalent
    caps.avx = true;   // NEON supports similar operations
    caps.avx2 = false; // No direct equivalent
    caps.fma = true;   // NEON has FMA instructions
    caps.avx512f = false;
    caps.avx512dq = false;
#else
    // No SIMD support for other architectures
    caps.sse2 = false;
    caps.avx = false;
    caps.avx2 = false;
    caps.fma = false;
    caps.avx512f = false;
    caps.avx512dq = false;
#endif
    
    return caps;
}