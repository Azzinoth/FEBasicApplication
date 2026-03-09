#pragma once

#ifdef FEBASICAPPLICATION_SHARED
    #ifdef FEBASICAPPLICATION_EXPORTS
        #define FEBASICAPPLICATION_API __declspec(dllexport)
    #else
        #define FEBASICAPPLICATION_API __declspec(dllimport)
    #endif
#else
    #define FEBASICAPPLICATION_API
#endif