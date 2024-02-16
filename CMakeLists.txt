cmake_minimum_required(VERSION 3.10)

set(BUILD_TYPE "Debug and Release" CACHE STRING "Choose Build type")
set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CONFIGURATION_TYPES Debug Release)

# set the project name
project(FEBasicApplication LANGUAGES CXX)

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

file(GLOB BasicApplicationSubSystem_SRC
	"FEBasicApplication.cpp"
	"FEBasicApplication.h"
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
)

set(GLEW_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glew2/lib/$(PlatformTarget))
set(GLFW_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/GLFW/lib/$(PlatformTarget))

set(GLEW_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/glew2/lib/$(PlatformTarget) PARENT_SCOPE)
set(GLFW_LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/GLFW/lib/$(PlatformTarget) PARENT_SCOPE)

link_directories(${GLEW_LIB_DIR})
link_directories(${GLFW_LIB_DIR})

add_library(FEBasicApplication
			${BasicApplicationSubSystem_SRC}
			${BasicApplicationNetworkSubSystem_SRC}
			# *************** THIRD_PARTY ***************
			${imgui_SRC}
)

target_link_libraries(FEBasicApplication glew32s.lib)
target_link_libraries(FEBasicApplication glfw3.lib)
target_link_libraries(FEBasicApplication glfw3dll.lib)
target_link_libraries(FEBasicApplication opengl32.lib)

source_group("Source Files" FILES ${BasicApplicationSubSystem_SRC})
source_group("Source Files/SubSystems/Networking" FILES ${BasicApplicationNetworkSubSystem_SRC})
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