cmake_minimum_required(VERSION 3.10)

set(BUILD_TYPE "Debug and Release" CACHE STRING "Choose Build type")
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CONFIGURATION_TYPES Debug Release)

# set the project name
project(FEBasicApplication)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
  # Disable C4251 warning
  add_compile_options(/wd4251)
endif()

option(BUILD_SHARED_LIBS "Build FEBasicApplication as a shared library" OFF)
option(USE_STATIC_RUNTIME "Use static runtime (/MT) instead of dynamic (/MD)" ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(MSVC)
	# Disable C4251 warning
	add_compile_options(/wd4251)

	if(USE_STATIC_RUNTIME)
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MTd")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MT")
	else()
		set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /MDd")
		set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD")
	endif()

	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

# Turn on the ability to create folders to organize projects (.vcproj)
# It creates "CMakePredefinedTargets" folder by default and adds CMake
# defined projects like INSTALL.vcproj and ZERO_CHECK.vcproj
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# *************** THIRD_PARTY ***************

file(GLOB imgui_SRC
	"ThirdParty/imgui/imgui.cpp"
	"ThirdParty/imgui/imgui_demo.cpp"
	"ThirdParty/imgui/imgui_draw.cpp"
	"ThirdParty/imgui/imgui_impl_glfw.cpp"
	"ThirdParty/imgui/imgui_impl_opengl3.cpp"
	"ThirdParty/imgui/imgui_tables.cpp"
	"ThirdParty/imgui/imgui_widgets.cpp"
)

# *************** THIRD_PARTY END ***************

file(GLOB BasicApplicationNetworkSubSystem_SRC
	"SubSystems/Networking/FENetworkingBaseClasses.cpp"
	"SubSystems/Networking/FENetworkingBaseClasses.h"
	"SubSystems/Networking/FEClientSideNetworkConnection.cpp"
	"SubSystems/Networking/FEClientSideNetworkConnection.h"
	"SubSystems/Networking/FEServerSideNetworkConnection.cpp"
	"SubSystems/Networking/FEServerSideNetworkConnection.h"
	"SubSystems/Networking/FENetworking.cpp"
	"SubSystems/Networking/FENetworking.h"
)

file(GLOB BasicApplicationProfilingSubSystem_SRC
	"SubSystems/Profiling/FEProfilingRegistry.cpp"
	"SubSystems/Profiling/FEProfilingRegistry.h"
	"SubSystems/Profiling/FEScopedTimer.cpp"
	"SubSystems/Profiling/FEScopedTimer.h"
	"SubSystems/Profiling/FEProfilingManager.cpp"
	"SubSystems/Profiling/FEProfilingManager.h"
)

file(GLOB BasicApplicationSubSystem_SRC
	"FEBasicApplication.cpp"
	"FEBasicApplication.h"
	"FEVirtualUI.cpp"
	"FEVirtualUI.h"
	"FEWindow.cpp"
	"FEWindow.h"
	"FEConsoleWindow.cpp"
	"FEConsoleWindow.h"
	"FEThreadPool.cpp"
	"FEThreadPool.h"
	"FELog.cpp"
	"FELog.h"
	"FETime.cpp"
	"FETime.h"
	"FEUniqueID.cpp"
	"FEUniqueID.h"
	"FEBasicApplicationAPI.h"
)

set(ALL_SOURCE_FILES "")
list(APPEND ALL_SOURCE_FILES
            ${BasicApplicationSubSystem_SRC}
            ${BasicApplicationNetworkSubSystem_SRC}
            ${BasicApplicationProfilingSubSystem_SRC}
            # *************** THIRD_PARTY ***************
            ${imgui_SRC}
)

set(GLEW_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glew2/lib/$(PlatformTarget))
set(GLFW_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/GLFW/lib/$(PlatformTarget))

set(GLEW_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glew2/lib/$(PlatformTarget) PARENT_SCOPE)
set(GLFW_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/GLFW/lib/$(PlatformTarget) PARENT_SCOPE)

link_directories(${GLEW_LIB_DIR})
link_directories(${GLFW_LIB_DIR})

if(BUILD_SHARED_LIBS)
    add_library(FEBasicApplication SHARED ${ALL_SOURCE_FILES})
    target_compile_definitions(FEBasicApplication PRIVATE FEBASICAPPLICATION_EXPORTS)
    target_compile_definitions(FEBasicApplication PUBLIC FEBASICAPPLICATION_SHARED)

	# Sometimes /GL conflicts with WINDOWS_EXPORT_ALL_SYMBOLS
	set_target_properties(FEBasicApplication PROPERTIES COMPILE_OPTIONS "/GL-")
	set_target_properties(FEBasicApplication PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)

	# Disable C4251 warning
	target_compile_options(FEBasicApplication PRIVATE /wd4251)
else()
    add_library(FEBasicApplication STATIC ${ALL_SOURCE_FILES})
endif()

# Adjust GLFW and GLEW linking based on BUILD_SHARED_LIBS and USE_STATIC_RUNTIME
if(BUILD_SHARED_LIBS)
    if(USE_STATIC_RUNTIME)
        message(WARNING "Building a shared library with static runtime is uncommon and may lead to issues.")
        set(GLFW_LIBRARY "glfw3_mt.lib")
        set(GLEW_LIBRARY "glew32s.lib")
    else()
        set(GLFW_LIBRARY "glfw3dll.lib")
        set(GLEW_LIBRARY "glew32.lib")
    endif()
else()
    if(USE_STATIC_RUNTIME)
        set(GLFW_LIBRARY "glfw3_mt.lib")
        set(GLEW_LIBRARY "glew32s.lib")
    else()
        set(GLFW_LIBRARY "glfw3.lib")
        set(GLEW_LIBRARY "glew32.lib")
    endif()
endif()

# Define GLEW_STATIC for static builds
if(USE_STATIC_RUNTIME)
    add_definitions(-DGLEW_STATIC)
endif()

target_link_libraries(FEBasicApplication 
    PRIVATE
		${GLEW_LIBRARY}
		${GLFW_LIBRARY}
		opengl32.lib
)

source_group("Source Files" FILES ${BasicApplicationSubSystem_SRC})
source_group("Source Files/SubSystems/Networking" FILES ${BasicApplicationNetworkSubSystem_SRC})
source_group("Source Files/SubSystems/Profiling" FILES ${BasicApplicationProfilingSubSystem_SRC})
# *************** THIRD_PARTY ***************
source_group("Source Files/ThirdParty/imgui" FILES ${imgui_SRC})

# set the startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT FEBasicApplication)

set(GLEW_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glew2/include)
set(BASICAPP_THIRDPARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty)

set(GLEW_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glew2/include PARENT_SCOPE)
set(BASICAPP_THIRDPARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty PARENT_SCOPE)
set(BASICAPP_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

include_directories(
	${GLEW_INCLUDE_DIR}
	${BASICAPP_THIRDPARTY_DIR}
)

# Export the BUILD_SHARED_LIBS and USE_STATIC_RUNTIME variables
set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS} PARENT_SCOPE)
set(USE_STATIC_RUNTIME ${USE_STATIC_RUNTIME} PARENT_SCOPE)

# Export the GLEW_LIBRARY and GLFW_LIBRARY variables
set(GLEW_LIBRARY ${GLEW_LIBRARY} PARENT_SCOPE)
set(GLFW_LIBRARY ${GLFW_LIBRARY} PARENT_SCOPE)

# If building as DLL, copy necessary DLLs to output directory
if(BUILD_SHARED_LIBS)
    add_custom_command(TARGET FEBasicApplication POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${GLFW_LIB_DIR}/glfw3.dll"
            "$<TARGET_FILE_DIR:FEBasicApplication>"
    )
endif()

# If using dynamic runtime, copy necessary DLLs to output directory
if(NOT USE_STATIC_RUNTIME)
    add_custom_command(TARGET FEBasicApplication POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${GLEW_LIB_DIR}/glew32.dll"
            "$<TARGET_FILE_DIR:FEBasicApplication>"
    )
endif()

add_custom_command(TARGET FEBasicApplication PRE_BUILD
	COMMAND ${CMAKE_COMMAND} ${CMAKE_COMMAND} -D FEBASICAPPLICATION_FOLDER=${BASICAPP_DIR}
		-P ${CMAKE_CURRENT_SOURCE_DIR}/UpdateFEBasicApplicationVersion.cmake
)