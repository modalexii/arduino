#pragma once
// Minimal Arduino/ESP-IDF stand-ins so face.cpp builds on a PC (tools/host_sim).
#include <cstdio>
#include <cstdlib>
#include <cstdint>
#include <cstdarg>
#include <cmath>
#include <chrono>
inline uint32_t micros(){using namespace std::chrono;static auto t0=steady_clock::now();return (uint32_t)duration_cast<microseconds>(steady_clock::now()-t0).count();}
// millis() is a simulated clock that sim.cpp advances (micros() stays real, for timings).
inline uint32_t sim_now_ms = 0;
inline uint32_t millis(){return sim_now_ms;}
#define MALLOC_CAP_SPIRAM 1
#define MALLOC_CAP_INTERNAL 2
inline void* heap_caps_malloc(size_t n,int){return malloc(n);}
inline void heap_caps_free(void*p){free(p);}
inline size_t heap_caps_get_free_size(int){return 0;}
struct SerialT{void printf(const char*f,...){va_list a;va_start(a,f);vprintf(f,a);va_end(a);} void println(const char*s){puts(s);} };
static SerialT Serial;
