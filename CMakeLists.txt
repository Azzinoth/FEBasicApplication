cmake_minimum_required(VERSION 3.1)

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
	"FEThreadPool.cpp"
	"FEThreadPool.h"
	"FELog.cpp"
	"FELog.h"
	"FETime.cpp"
	"FETime.h"
	"FEUniqueID.cpp"
	"FEUniqueID.h"
)

link_directories("ThirdParty/glew2/lib/$(PlatformTarget)")
link_directories("ThirdParty/GLFW/lib/$(PlatformTarget)")

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

# set FEBasicApplication as the startup project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT FEBasicApplication)

include_directories(
	"ThirdParty/glew2/include"
	"ThirdParty/"
)