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

set(ALL_SOURCE_FILES "")

option(BUILD_SHARED_LIBS "Build FEBasicApplication as a shared library" OFF)
option(USE_STATIC_RUNTIME "Use static runtime (/MT) instead of dynamic (/MD)" ON)
set(FE_PLATFORM_BACKEND "GLFW"  CACHE STRING "Platform backend: GLFW | Emscripten")
set(FE_GRAPHIC_API_BACKEND "OpenGL" CACHE STRING "Renderer backend: OpenGL | WebGPU")
set_property(CACHE FE_PLATFORM_BACKEND PROPERTY STRINGS GLFW Emscripten)
set_property(CACHE FE_GRAPHIC_API_BACKEND PROPERTY STRINGS OpenGL WebGPU)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# When targeting Emscripten, force Emscripten platform + WebGPU graphics.
if(EMSCRIPTEN)
    set(FE_PLATFORM_BACKEND    "Emscripten" CACHE STRING "Platform backend: GLFW | Emscripten" FORCE)
    set(FE_GRAPHIC_API_BACKEND "WebGPU"     CACHE STRING "Renderer backend: OpenGL | WebGPU"   FORCE)
    message(STATUS "Emscripten detected: forcing FE_PLATFORM_BACKEND=Emscripten, FE_GRAPHIC_API_BACKEND=WebGPU")
endif()

# Validate combos
if(FE_PLATFORM_BACKEND STREQUAL "Emscripten" AND FE_GRAPHIC_API_BACKEND STREQUAL "OpenGL")
    message(FATAL_ERROR "Emscripten + OpenGL (WebGL2) is not supported")
endif()

file(GLOB imgui_SRC
	"ThirdParty/imgui/misc/cpp/imgui_stdlib.cpp"
	"ThirdParty/imgui/imgui.cpp"
	"ThirdParty/imgui/imgui_demo.cpp"
	"ThirdParty/imgui/imgui_draw.cpp"
	"ThirdParty/imgui/imgui_tables.cpp"
	"ThirdParty/imgui/imgui_widgets.cpp"
)

# Platform code
if(FE_PLATFORM_BACKEND STREQUAL "GLFW")
    set(FE_PLATFORM_SRC
        Backends/Platforms/GLFW/FEPlatformGLFW.cpp
        Backends/Platforms/GLFW/FEPlatformGLFW.h
        Backends/Platforms/GLFW/FEPlatformWindowGLFW.cpp
        Backends/Platforms/GLFW/FEPlatformWindowGLFW.h)
    list(APPEND imgui_SRC ThirdParty/imgui/imgui_impl_glfw.cpp)
elseif(FE_PLATFORM_BACKEND STREQUAL "Emscripten")
    set(FE_PLATFORM_SRC
        Backends/Platforms/Emscripten/FEPlatformEmscripten.cpp
        Backends/Platforms/Emscripten/FEPlatformEmscripten.h
        Backends/Platforms/Emscripten/FEPlatformWindowEmscripten.cpp
        Backends/Platforms/Emscripten/FEPlatformWindowEmscripten.h)
    # Emscripten code currently uses GLFW emulation (via -sUSE_GLFW=3) and ImGui's GLFW glue.
    list(APPEND imgui_SRC ThirdParty/imgui/imgui_impl_glfw.cpp)
endif()

# Graphic API code
if(FE_GRAPHIC_API_BACKEND STREQUAL "OpenGL")
    set(FE_GRAPHIC_API_SRC
        Backends/GraphicsAPIs/OpenGL/FEDeviceOpenGL.cpp
        Backends/GraphicsAPIs/OpenGL/FEDeviceOpenGL.h
        Backends/GraphicsAPIs/OpenGL/FEDeviceSurfaceOpenGL.cpp
        Backends/GraphicsAPIs/OpenGL/FEDeviceSurfaceOpenGL.h)
    list(APPEND imgui_SRC ThirdParty/imgui/imgui_impl_opengl3.cpp)
elseif(FE_GRAPHIC_API_BACKEND STREQUAL "WebGPU")
    set(FE_GRAPHIC_API_SRC
        Backends/GraphicsAPIs/WebGPU/FEDeviceWebGPU.cpp
        Backends/GraphicsAPIs/WebGPU/FEDeviceWebGPU.h
        Backends/GraphicsAPIs/WebGPU/FEDeviceSurfaceWebGPU.cpp
        Backends/GraphicsAPIs/WebGPU/FEDeviceSurfaceWebGPU.h)
    list(APPEND imgui_SRC ThirdParty/imgui/imgui_impl_wgpu.cpp)
endif()

list(APPEND ALL_SOURCE_FILES ${FE_PLATFORM_SRC} ${FE_GRAPHIC_API_SRC})

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

if(NOT EMSCRIPTEN)
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
endif()

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

# FEConsoleWindow is Win32-only (HWND, WORD, console color API). Web has no OS console.
if(EMSCRIPTEN)
	list(REMOVE_ITEM BasicApplicationSubSystem_SRC
		"${CMAKE_CURRENT_SOURCE_DIR}/FEConsoleWindow.cpp"
		"${CMAKE_CURRENT_SOURCE_DIR}/FEConsoleWindow.h"
	)
endif()

file(GLOB Backends_SRC
	"Backends/FEGraphicsAPI.h"
	"Backends/FEMonitorInfo.h"
	"Backends/Platforms/FEPlatformInterface.h"
	"Backends/Platforms/FEPlatformWindowInterface.h"
	"Backends/GraphicsAPIs/FEDeviceInterface.h"
	"Backends/GraphicsAPIs/FEDeviceSurfaceInterface.h"
)

list(APPEND ALL_SOURCE_FILES
            ${BasicApplicationSubSystem_SRC}
            ${BasicApplicationNetworkSubSystem_SRC}
            ${BasicApplicationProfilingSubSystem_SRC}
            ${Backends_SRC}
            # *************** THIRD_PARTY ***************
            ${imgui_SRC}
)

if(NOT EMSCRIPTEN)
	set(GLEW_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glew2/lib/$(PlatformTarget))
	set(GLFW_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/GLFW/lib/$(PlatformTarget))

	set(GLEW_LIB_DIR ${GLEW_LIB_DIR} PARENT_SCOPE)
	set(GLFW_LIB_DIR ${GLFW_LIB_DIR} PARENT_SCOPE)

	link_directories(${GLEW_LIB_DIR})
	link_directories(${GLFW_LIB_DIR})
endif()

# *************** DAWN WEBGPU ***************
# Dawn is the native WebGPU implementation; Emscripten provides its own webgpu.h via -sUSE_WEBGPU=1.
if(FE_GRAPHIC_API_BACKEND STREQUAL "WebGPU" AND NOT EMSCRIPTEN)
    set(DAWN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/Dawn)
    set(DAWN_LIB_DIR ${DAWN_DIR}/Lib/x64/)
    set(DAWN_INCLUDE_DIR ${DAWN_DIR}/include)

    set(DAWN_DIR ${DAWN_DIR} PARENT_SCOPE)
    set(DAWN_LIB_DIR ${DAWN_LIB_DIR} PARENT_SCOPE)
    set(DAWN_INCLUDE_DIR ${DAWN_INCLUDE_DIR} PARENT_SCOPE)

    link_directories(${DAWN_LIB_DIR})

	# No static runtime variant of Dawn.
	set(DAWN_RUNTIME_VARIANT "Dynamic_runtime")

	set(DAWN_LIBRARY_DEBUG "${DAWN_LIB_DIR}Debug/${DAWN_RUNTIME_VARIANT}/webgpu_dawn.lib")
	set(DAWN_LIBRARY_DEBUG ${DAWN_LIBRARY_DEBUG} PARENT_SCOPE)
	set(DAWN_DLL_DEBUG "${DAWN_LIB_DIR}Debug/${DAWN_RUNTIME_VARIANT}/webgpu_dawn.dll")
	set(DAWN_DLL_DEBUG ${DAWN_DLL_DEBUG} PARENT_SCOPE)
	set(ABSEIL_DLL_DEBUG "${DAWN_LIB_DIR}Debug/${DAWN_RUNTIME_VARIANT}/abseil_dll.dll")
	set(ABSEIL_DLL_DEBUG ${ABSEIL_DLL_DEBUG} PARENT_SCOPE)
	
    set(DAWN_LIBRARY_RELEASE "${DAWN_LIB_DIR}Release/${DAWN_RUNTIME_VARIANT}/webgpu_dawn.lib")
	set(DAWN_LIBRARY_RELEASE ${DAWN_LIBRARY_RELEASE} PARENT_SCOPE)
	set(DAWN_DLL_RELEASE "${DAWN_LIB_DIR}Release/${DAWN_RUNTIME_VARIANT}/webgpu_dawn.dll")
	set(DAWN_DLL_RELEASE ${DAWN_DLL_RELEASE} PARENT_SCOPE)
	set(ABSEIL_DLL_RELEASE "${DAWN_LIB_DIR}Release/${DAWN_RUNTIME_VARIANT}/abseil_dll.dll")
	set(ABSEIL_DLL_RELEASE ${ABSEIL_DLL_RELEASE} PARENT_SCOPE)
endif()
# *************** DAWN WEBGPU END ***************

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

if(FE_GRAPHIC_API_BACKEND STREQUAL "WebGPU" AND NOT EMSCRIPTEN)
    target_compile_definitions(FEBasicApplication PRIVATE IMGUI_IMPL_WEBGPU_BACKEND_DAWN)
endif()

# Platform dependency linking
if(FE_PLATFORM_BACKEND STREQUAL "GLFW")
    if(BUILD_SHARED_LIBS)
        if(USE_STATIC_RUNTIME)
            set(GLFW_LIBRARY "glfw3_mt.lib")
        else()
            set(GLFW_LIBRARY "glfw3dll.lib")
        endif()
    else()
        if(USE_STATIC_RUNTIME)
            set(GLFW_LIBRARY "glfw3_mt.lib")
        else()
            set(GLFW_LIBRARY "glfw3.lib")
        endif()
    endif()

    target_link_libraries(FEBasicApplication
        PRIVATE
            ${GLFW_LIBRARY}
    )
elseif(FE_PLATFORM_BACKEND STREQUAL "Emscripten")
    # PUBLIC so the parent executable also gets the flag at its link step.
    # GLFW headers are in Emscripten's default sysroot; -sUSE_GLFW=3 only affects linking.
    target_link_options(FEBasicApplication PUBLIC "-sUSE_GLFW=3")
    # Asyncify lets emscripten_sleep yield to the browser event loop — needed for
    # WebGPU's async adapter/device acquisition (which depends on JS promises resolving).
    target_link_options(FEBasicApplication PUBLIC "-sASYNCIFY=1")
endif()

# Graphic API dependency linking
if(FE_GRAPHIC_API_BACKEND STREQUAL "OpenGL")
    if(USE_STATIC_RUNTIME)
        set(GLEW_LIBRARY "glew32s.lib")
        target_compile_definitions(FEBasicApplication PRIVATE GLEW_STATIC)
    else()
        set(GLEW_LIBRARY "glew32.lib")
    endif()

    target_compile_definitions(FEBasicApplication PUBLIC FE_GRAPHICS_API_OPENGL)

    target_link_libraries(FEBasicApplication
        PRIVATE
            ${GLEW_LIBRARY}
            opengl32.lib
    )
elseif(FE_GRAPHIC_API_BACKEND STREQUAL "WebGPU")
    if(EMSCRIPTEN)
        # emdawnwebgpu is Dawn integrated as an Emscripten port; provides webgpu/webgpu.h.
        # Must be active at both compile and link time.
        target_compile_options(FEBasicApplication PUBLIC "--use-port=emdawnwebgpu")
        target_link_options(FEBasicApplication PUBLIC "--use-port=emdawnwebgpu")
    else()
        target_link_libraries(FEBasicApplication
            PRIVATE
                debug ${DAWN_LIBRARY_DEBUG}
                optimized ${DAWN_LIBRARY_RELEASE}
        )
    endif()
endif()

source_group("Source Files" FILES ${BasicApplicationSubSystem_SRC})
source_group("Source Files/SubSystems/Networking" FILES ${BasicApplicationNetworkSubSystem_SRC})
source_group("Source Files/SubSystems/Profiling" FILES ${BasicApplicationProfilingSubSystem_SRC})
source_group("Source Files/Backends" FILES ${Backends_SRC})
source_group("Source Files/Backends/Platforms/${FE_PLATFORM_BACKEND}" FILES ${FE_PLATFORM_SRC})
source_group("Source Files/Backends/GraphicsAPIs/${FE_GRAPHIC_API_BACKEND}" FILES ${FE_GRAPHIC_API_SRC})
# *************** THIRD_PARTY ***************
source_group("Source Files/ThirdParty/imgui" FILES ${imgui_SRC})

# set the startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT FEBasicApplication)

if(FE_GRAPHIC_API_BACKEND STREQUAL "OpenGL")
    set(GLEW_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glew2/include)
    set(GLEW_INCLUDE_DIR ${GLEW_INCLUDE_DIR} PARENT_SCOPE)
endif()

set(BASICAPP_THIRDPARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty)
set(BASICAPP_THIRDPARTY_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty PARENT_SCOPE)
set(BASICAPP_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)

include_directories(
	${GLEW_INCLUDE_DIR}
	${BASICAPP_THIRDPARTY_DIR}
	${BASICAPP_THIRDPARTY_DIR}/imgui
	$<$<STREQUAL:${FE_GRAPHIC_API_BACKEND},WebGPU>:${DAWN_INCLUDE_DIR}>
)

# Export the BUILD_SHARED_LIBS and USE_STATIC_RUNTIME variables
set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS} PARENT_SCOPE)
set(USE_STATIC_RUNTIME ${USE_STATIC_RUNTIME} PARENT_SCOPE)

# Export the GLEW_LIBRARY and GLFW_LIBRARY variables
set(GLEW_LIBRARY ${GLEW_LIBRARY} PARENT_SCOPE)
set(GLFW_LIBRARY ${GLFW_LIBRARY} PARENT_SCOPE)

set(FE_PLATFORM_BACKEND    ${FE_PLATFORM_BACKEND}    PARENT_SCOPE)
set(FE_GRAPHIC_API_BACKEND ${FE_GRAPHIC_API_BACKEND} PARENT_SCOPE)

if(FE_PLATFORM_BACKEND STREQUAL "GLFW" AND BUILD_SHARED_LIBS AND NOT USE_STATIC_RUNTIME)
    add_custom_command(TARGET FEBasicApplication POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${GLFW_LIB_DIR}/glfw3.dll"
            "$<TARGET_FILE_DIR:FEBasicApplication>"
    )
endif()

if(FE_GRAPHIC_API_BACKEND STREQUAL "OpenGL" AND NOT USE_STATIC_RUNTIME)
    add_custom_command(TARGET FEBasicApplication POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${GLEW_LIB_DIR}/glew32.dll"
            "$<TARGET_FILE_DIR:FEBasicApplication>"
    )
endif()

add_custom_command(TARGET FEBasicApplication PRE_BUILD
    COMMAND ${CMAKE_COMMAND}
        -D PROJECT_VERSION_PREFIX=FE_BASIC_APPLICATION_
        -D PROJECT_VERSION_MAJOR=0
        -D PROJECT_VERSION_MINOR=4
        -D PROJECT_VERSION_PATCH=4
        -D PROJECT_VERSION_DIR=${CMAKE_CURRENT_SOURCE_DIR}/VersionInfo
        -P ${CMAKE_CURRENT_SOURCE_DIR}/VersionInfo/UpdateProjectVersion.cmake
)