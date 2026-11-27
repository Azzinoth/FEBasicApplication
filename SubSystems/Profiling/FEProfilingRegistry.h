#include "../FEThreadPool.h"

namespace FocalEngine
{
    class FEProfilingRegistry
    {
        friend class FEProfilingManager;

        struct FunctionInfo
        {
            int CallCount = 0;
            double TotalDuration = 0.0;
            std::unordered_map<const char*, int> ParentFunctions;
            std::vector<std::pair<FE_CHRONO_TIME_POINT, FE_CHRONO_TIME_POINT>> Timestamps;
            std::unordered_map<const char*, std::vector<std::pair<FE_CHRONO_TIME_POINT, FE_CHRONO_TIME_POINT>>> SectionTimestamps;
        };

        std::unordered_map<const char*, FunctionInfo> FunctionsData;
        std::vector<const char*> CallStack;
        std::mutex DataMutex;

        int EventCount = 0;
        FE_TIME_RESOLUTION TimeResolution = FE_TIME_RESOLUTION_MICROSECONDS;

        void RecordFunctionEntry(const char* FunctionName);
        void RecordFunctionExit(const char* FunctionName, FE_CHRONO_TIME_POINT StartTimeStamp, FE_CHRONO_TIME_POINT EndTimeStamp, double Duration);

        void RecordSectionExit(const char* SectionName, FE_CHRONO_TIME_POINT StartTimeStamp, FE_CHRONO_TIME_POINT EndTimeStamp);

        void ClearData();
    public:
        FEProfilingRegistry() = default;
        ~FEProfilingRegistry() = default;
    };
}