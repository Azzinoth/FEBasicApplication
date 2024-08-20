#include "FEProfilingRegistry.h"
#include <shared_mutex>
#include <sstream>
#include <fstream>

namespace FocalEngine
{
    class FEBASICAPPLICATION_API FEProfilingManager
    {
        friend class FEScopedTimer;
        friend class FEProfilingManager;
        SINGLETON_PRIVATE_PART(FEProfilingManager)

        std::unordered_map<std::thread::id, FEProfilingRegistry> ThreadData;
        std::shared_mutex ThreadDataMutex;
        FE_TIME_RESOLUTION TimeResolution = FE_TIME_RESOLUTION_MICROSECONS;

        bool bActive = false;
        int EventCount = 0;
        std::shared_mutex EventCountMutex;

        void RegisterThread(std::thread::id ThreadID);
        bool IsThreadRegistered(const std::thread::id& ThreadID);

        void RecordFunctionEntry(const std::string& FunctionName, FE_CHRONO_TIME_POINT StartTimeStamp);
        void RecordFunctionExit(const std::string& FunctionName, FE_CHRONO_TIME_POINT EndTimeStamp, double Duration);

        void RecordSectionEntry(const std::string& SectionName, FE_CHRONO_TIME_POINT StartTimeStamp);
        void RecordSectionExit(const std::string& SectionName, FE_CHRONO_TIME_POINT EndTimeStamp, double Duration);

        void ClearData();
    public:
        SINGLETON_PUBLIC_PART(FEProfilingManager)
        
        void StartProfiling();
        void StopProfiling();

        int GetEventCount();
        //std::string CreateReport();

        // Save the timeline to a JSON file, which can be opened in Chrome's tracing tool(chrome://tracing/).
        void SaveTimelineToJSON(const std::string& filename);
    };

#define PROFILING FEProfilingManager::getInstance()

#ifdef FE_ENABLE_PROFILING
    class FEScopedTimer;

    #define FE_PROFILE_FUNCTION_SCOPE() FocalEngine::FEScopedTimer Timer(__FUNCTION__)

    #define FE_CONCAT_IMPL(x, y) x##y
    #define FE_CONCAT(x, y) FE_CONCAT_IMPL(x, y)
    #define FE_UNIQUE_NAME(Base, SectionName) FE_CONCAT(Base, SectionName)
    // To profile a section of code, use FE_PROFILE_FUNCTION_SCOPE(or FE_PROFILE_FUNCTION) at the beginning of the function that includes the section.
    // Then use FE_PROFILE_SECTION_START(SectionName) at the beginning of the section and FE_PROFILE_SECTION_END(SectionName) at the end of the section.
    #define FE_PROFILE_SECTION_START(SectionName) FocalEngine::FEScopedTimer FE_UNIQUE_NAME(Timer_, SectionName)(#SectionName, true)
    // To profile a section of code, use FE_PROFILE_FUNCTION_SCOPE(or FE_PROFILE_FUNCTION) at the beginning of the function that includes the section.
    // Then use FE_PROFILE_SECTION_START(SectionName) at the beginning of the section and FE_PROFILE_SECTION_END(SectionName) at the end of the section.
    #define FE_PROFILE_SECTION_END(SectionName) FE_UNIQUE_NAME(Timer_, SectionName).Stop()

    #define FE_PROFILE_FUNCTION(FunctionDeclaration)    \
    FunctionDeclaration                                 \
    {                                                   \
	    FocalEngine::FEScopedTimer Timer(__FUNCTION__)

#else
    // That's an empty macro, not to cause any overhead when profiling is disabled.
    #define FE_PROFILE_FUNCTION_SCOPE()
    // That's an empty macro, not to cause any overhead when profiling is disabled.
    #define FE_PROFILE_SECTION_START(SectionName)
    // That's an empty macro, not to cause any overhead when profiling is disabled.
    #define FE_PROFILE_SECTION_END()
    // That's an empty macro, not to cause any overhead when profiling is disabled.
    #define FE_PROFILE_FUNCTION(FunctionDeclaration)    \
    FunctionDeclaration                                 \
    {         

#endif
}