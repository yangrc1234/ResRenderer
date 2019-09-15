#pragma once
#if defined(_WIN32) && defined(RES_RENDERER_SHARED_BUILD)
#    ifdef RES_RENDERER_BUILD
#        define RES_RENDERER_API  __declspec(dllexport)
#    else
#        define RES_RENDERER_API __declspec(dllimport)
#    endif
#else
#    define RES_RENDERER_API
#endif