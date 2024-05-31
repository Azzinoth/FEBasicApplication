#include "FEProfilingManager.h"

namespace FocalEngine
{
    class FEScopedTimer
    {
        std::string EventName;
        FE_CHRONO_TIME_POINT StartTime;

        bool bIsStopped = false;
        bool bIsForSection = false;
    public:
        FEScopedTimer(const std::string& EventName, bool bIsSection = false);
        ~FEScopedTimer();

        void Stop();
    };
}