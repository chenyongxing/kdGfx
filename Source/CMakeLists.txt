set(project kdGfx)

find_package(spdlog CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
if (WIN32)
	find_package(directx-headers CONFIG REQUIRED)
endif()

file(GLOB deps_sources 
	"../ThirdParty/*.h"
	"../ThirdParty/*.cpp"
	"../ThirdParty/imgui/*.cpp"
	"../ThirdParty/imnodes/*.cpp"
)

file(GLOB sources 
	"*.h" "*.cpp"
	"RenderGraph/*.h" "RenderGraph/*.cpp"
	"RHI/*.h" "RHI/*.cpp"
	"RHI/Vulkan/*.h" "RHI/Vulkan/*.cpp"
	"Scene/*.h" "Scene/*.cpp"
)

if (WIN32)
	file(GLOB directX12_sources "RHI/DirectX12/*.h" "RHI/DirectX12/*.cpp")
	list(APPEND sources ${directX12_sources})
endif()

add_library(${project} STATIC ${deps_sources} ${sources})

if (WIN32)
	target_compile_definitions(${project} PRIVATE VK_USE_PLATFORM_WIN32_KHR)
	target_link_libraries(${project} Microsoft::DirectX-Headers dxgi.lib d3d12.lib)
else()
	if(USE_XCB)
		target_compile_definitions(${project} PRIVATE VK_USE_PLATFORM_XCB_KHR)
		target_link_libraries(${project} X11 X11-xcb)
	else()
		target_compile_definitions(${project} PRIVATE VK_USE_PLATFORM_WAYLAND_KHR)
		target_link_libraries(${project} wayland-client)
	endif()
endif()

target_include_directories(${project} PUBLIC 
	"."
	"../ThirdParty/"
)

target_link_libraries(${project}
	spdlog::spdlog_header_only
	glm::glm
	glfw
	Vulkan::Vulkan
)
