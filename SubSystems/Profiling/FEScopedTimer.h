#include "FEProfilingManager.h"

namespace FocalEngine
{
    class FEScopedTimer
    {
        const char* EventName = nullptr;
        FE_CHRONO_TIME_POINT StartTime;

        bool bIsStopped = false;
        bool bIsForSection = false;
    public:
        FEScopedTimer(const char* EventName, bool bIsSection = false);
        ~FEScopedTimer();

        void Stop();
    };
}