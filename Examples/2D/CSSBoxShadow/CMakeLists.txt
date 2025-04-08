set(exename CSSBoxShadow)
add_executable(${exename} Main.cpp)
target_link_libraries(${exename} kdGfx)

compile_shader(${exename} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} 6_0)

if (WIN32)
	add_custom_command(TARGET ${exename} PRE_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different 
		${CMAKE_CURRENT_SOURCE_DIR}/../../../D3D12 ${CMAKE_CURRENT_BINARY_DIR}/D3D12)
endif()
