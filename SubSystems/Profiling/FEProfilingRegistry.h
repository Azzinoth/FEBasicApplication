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
            std::unordered_map<std::string, int> ParentFunctions;
            std::vector<std::pair<FE_CHRONO_TIME_POINT, FE_CHRONO_TIME_POINT>> Timestamps;
            std::unordered_map<std::string, std::vector<std::pair<FE_CHRONO_TIME_POINT, FE_CHRONO_TIME_POINT>>> SectionTimestamps;
        };

        std::unordered_map<std::string, FunctionInfo> FunctionsData;
        std::vector<std::string> CallStack;

        int EventCount = 0;
        FE_TIME_RESOLUTION TimeResolution = FE_TIME_RESOLUTION_MICROSECONS;

        void RecordFunctionEntry(const std::string& FunctionName, FE_CHRONO_TIME_POINT StartTimeStamp);
        void RecordFunctionExit(const std::string& FunctionName, FE_CHRONO_TIME_POINT EndTimeStamp, double Duration);

        void RecordSectionEntry(const std::string& SectionName, FE_CHRONO_TIME_POINT StartTimeStamp);
        void RecordSectionExit(const std::string& SectionName, FE_CHRONO_TIME_POINT EndTimeStamp, double Duration);

        void ClearData();
    public:
        FEProfilingRegistry() = default;
        ~FEProfilingRegistry() = default;
    };
}