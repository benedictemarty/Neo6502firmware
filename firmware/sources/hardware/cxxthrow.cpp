// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      cxxthrow.cpp
//      Author :    bmarty <bmarty@mailo.com>
//      Purpose :   libstdc++ error hooks without exceptions (T-13, memory budget).
//                  The SDK builds with -fno-exceptions, but the std::string templates
//                  still reference std::__throw_* ; the prebuilt libstdc++ versions throw,
//                  which drags in __cxa_throw, the emergency exception pool (a static
//                  constructor that mallocs at boot) and the libgcc unwinder — placed in
//                  RAM by the SDK linker script (3.8 KB). Defining the hooks here keeps
//                  all of that out of the link : an allocation failure is a panic.
//
// ***************************************************************************************
// ***************************************************************************************

#include "pico/stdlib.h"
#include <new>
#include <cstdlib>

// nothrow new/delete : the libstdc++ versions (new_opnt.o) wrap operator new in a
// try/catch, which drags in the personality routine and the unwinder. Same contract
// as the SDK's new_delete.cpp (malloc/free), NULL on failure instead of a panic.

void *operator new(std::size_t n, const std::nothrow_t &) noexcept { return malloc(n); }
void *operator new[](std::size_t n, const std::nothrow_t &) noexcept { return malloc(n); }
void operator delete(void *p, const std::nothrow_t &) noexcept { free(p); }
void operator delete[](void *p, const std::nothrow_t &) noexcept { free(p); }

namespace std {
    void __throw_bad_alloc(void) { panic("C++ bad_alloc"); }
    void __throw_bad_array_new_length(void) { panic("C++ bad_array_new_length"); }
    void __throw_bad_cast(void) { panic("C++ bad_cast"); }
    void __throw_bad_exception(void) { panic("C++ bad_exception"); }
    void __throw_bad_function_call(void) { panic("C++ bad_function_call"); }
    void __throw_bad_typeid(void) { panic("C++ bad_typeid"); }
    void __throw_domain_error(const char *w) { panic("C++ domain_error: %s", w); }
    void __throw_future_error(int e) { panic("C++ future_error %d", e); }
    void __throw_invalid_argument(const char *w) { panic("C++ invalid_argument: %s", w); }
    void __throw_ios_failure(const char *w) { panic("C++ ios_failure: %s", w); }
    void __throw_ios_failure(const char *w, int e) { panic("C++ ios_failure: %s (%d)", w, e); }
    void __throw_length_error(const char *w) { panic("C++ length_error: %s", w); }
    void __throw_logic_error(const char *w) { panic("C++ logic_error: %s", w); }
    void __throw_out_of_range(const char *w) { panic("C++ out_of_range: %s", w); }
    void __throw_out_of_range_fmt(const char *w, ...) { panic("C++ out_of_range: %s", w); }
    void __throw_overflow_error(const char *w) { panic("C++ overflow_error: %s", w); }
    void __throw_range_error(const char *w) { panic("C++ range_error: %s", w); }
    void __throw_runtime_error(const char *w) { panic("C++ runtime_error: %s", w); }
    void __throw_system_error(int e) { panic("C++ system_error %d", e); }
    void __throw_underflow_error(const char *w) { panic("C++ underflow_error: %s", w); }
}
