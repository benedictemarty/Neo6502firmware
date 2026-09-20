// ***************************************************************************************
// ***************************************************************************************
//
//      Name :      cxx_no_extern_string.h
//      Author :    bmarty <bmarty@mailo.com>
//      Purpose :   Forced include (-include) for the RP2040 build (T-13, memory budget).
//                  libstdc++ declares std::string members as extern templates, so the
//                  linker takes them from the prebuilt string-inst.o — compiled WITH
//                  exceptions (cleanup landing pads → __cxa_end_cleanup, __cxa_rethrow,
//                  the emergency exception pool and the libgcc unwinder in RAM).
//                  _GLIBCXX_EXTERN_TEMPLATE = -1 makes the compiler instantiate
//                  std::string in our own objects, built with -fno-exceptions.
//                  c++config.h is include-guarded, so the redefinition sticks.
//
// ***************************************************************************************
// ***************************************************************************************

#ifndef _CXX_NO_EXTERN_STRING_H
#define _CXX_NO_EXTERN_STRING_H
#ifdef __cplusplus
#include <bits/c++config.h>
#undef _GLIBCXX_EXTERN_TEMPLATE
#define _GLIBCXX_EXTERN_TEMPLATE -1
#endif
#endif
