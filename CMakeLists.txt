cmake_minimum_required(VERSION 3.20)

project(kdGfx)

option(USE_XCB OFF)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
	add_definitions(
	    -DNOMINMAX
	    -D_CRT_SECURE_NO_WARNINGS
    )
endif()
if (USE_XCB)
	add_definitions(-DUSE_XCB)
endif()

if (WIN32)
    find_package(directx-dxc CONFIG REQUIRED)
else()
    set(DIRECTX_DXC_TOOL dxc)
endif()

macro(compile_shader BUILD_TARGET SRC_DIR DST_DIR SM_VERSION)
    file(GLOB Shaders ${SRC_DIR}/*.hlsl)
    foreach(FILEPATH IN LISTS Shaders)
        get_filename_component(FILENAME ${FILEPATH} NAME_WE)
        if (WIN32)
            add_custom_command(
                TARGET ${BUILD_TARGET}
                PRE_BUILD
                WORKING_DIRECTORY ${SRC_DIR}
                COMMAND echo ${DIRECTX_DXC_TOOL} -T vs_${SM_VERSION} -E vertexMain -no-warnings -Fo ${DST_DIR}/${FILENAME}.vs.dxil ${FILENAME}.hlsl
                COMMAND ${DIRECTX_DXC_TOOL} -T vs_${SM_VERSION} -E vertexMain -no-warnings -Fo ${DST_DIR}/${FILENAME}.vs.dxil ${FILENAME}.hlsl
                COMMAND echo ${DIRECTX_DXC_TOOL} -T ps_${SM_VERSION} -E pixelMain -no-warnings -Fo ${DST_DIR}/${FILENAME}.ps.dxil ${FILENAME}.hlsl
                COMMAND ${DIRECTX_DXC_TOOL} -T ps_${SM_VERSION} -E pixelMain -no-warnings -Fo ${DST_DIR}/${FILENAME}.ps.dxil ${FILENAME}.hlsl
                COMMAND echo ${DIRECTX_DXC_TOOL} -T vs_${SM_VERSION} -E vertexMain -spirv -fspv-entrypoint-name=main -fvk-invert-y -Fo ${DST_DIR}/${FILENAME}.vs.spv ${FILENAME}.hlsl
                COMMAND ${DIRECTX_DXC_TOOL} -T vs_${SM_VERSION} -E vertexMain -spirv -fspv-entrypoint-name=main -fvk-invert-y -Fo ${DST_DIR}/${FILENAME}.vs.spv ${FILENAME}.hlsl
                COMMAND echo ${DIRECTX_DXC_TOOL} -T ps_${SM_VERSION} -E pixelMain -spirv -fspv-entrypoint-name=main -Fo ${DST_DIR}/${FILENAME}.ps.spv ${FILENAME}.hlsl
                COMMAND ${DIRECTX_DXC_TOOL} -T ps_${SM_VERSION} -E pixelMain -spirv -fspv-entrypoint-name=main -Fo ${DST_DIR}/${FILENAME}.ps.spv ${FILENAME}.hlsl
            )
        else()
            add_custom_command(
                TARGET ${BUILD_TARGET}
                PRE_BUILD
                WORKING_DIRECTORY ${SRC_DIR}
                COMMAND echo ${DIRECTX_DXC_TOOL} -T vs_${SM_VERSION} -E vertexMain -spirv -fspv-entrypoint-name=main -fvk-invert-y -Fo ${DST_DIR}/${FILENAME}.vs.spv ${FILENAME}.hlsl
                COMMAND ${DIRECTX_DXC_TOOL} -T vs_${SM_VERSION} -E vertexMain -spirv -fspv-entrypoint-name=main -fvk-invert-y -Fo ${DST_DIR}/${FILENAME}.vs.spv ${FILENAME}.hlsl
                COMMAND echo ${DIRECTX_DXC_TOOL} -T ps_${SM_VERSION} -E pixelMain -spirv -fspv-entrypoint-name=main -Fo ${DST_DIR}/${FILENAME}.ps.spv ${FILENAME}.hlsl
                COMMAND ${DIRECTX_DXC_TOOL} -T ps_${SM_VERSION} -E pixelMain -spirv -fspv-entrypoint-name=main -Fo ${DST_DIR}/${FILENAME}.ps.spv ${FILENAME}.hlsl
            )
        endif()
    endforeach()

    file(GLOB ComputeShaders ${SRC_DIR}/*.compute)
    foreach(FILEPATH IN LISTS ComputeShaders)
        get_filename_component(FILENAME ${FILEPATH} NAME_WE)
        if (WIN32)
            add_custom_command(
                TARGET ${BUILD_TARGET}
                PRE_BUILD
                WORKING_DIRECTORY ${SRC_DIR}
                COMMAND echo ${DIRECTX_DXC_TOOL} -T cs_${SM_VERSION} -E main -no-warnings -Fo ${DST_DIR}/${FILENAME}.cs.dxil ${FILENAME}.compute
                COMMAND ${DIRECTX_DXC_TOOL} -T cs_${SM_VERSION} -E main -no-warnings -Fo ${DST_DIR}/${FILENAME}.cs.dxil ${FILENAME}.compute
                COMMAND echo ${DIRECTX_DXC_TOOL} -T cs_${SM_VERSION} -E main -spirv -fspv-entrypoint-name=main -Fo ${DST_DIR}/${FILENAME}.cs.spv ${FILENAME}.compute
                COMMAND ${DIRECTX_DXC_TOOL} -T cs_${SM_VERSION} -E main -spirv -fspv-entrypoint-name=main -Fo ${DST_DIR}/${FILENAME}.cs.spv ${FILENAME}.compute
            )
        else()
            add_custom_command(
                TARGET ${BUILD_TARGET}
                PRE_BUILD
                WORKING_DIRECTORY ${SRC_DIR}
                COMMAND echo ${DIRECTX_DXC_TOOL} -T cs_${SM_VERSION} -E main -spirv -fspv-entrypoint-name=main -Fo ${DST_DIR}/${FILENAME}.cs.spv ${FILENAME}.compute
                COMMAND ${DIRECTX_DXC_TOOL} -T cs_${SM_VERSION} -E main -spirv -fspv-entrypoint-name=main -Fo ${DST_DIR}/${FILENAME}.cs.spv ${FILENAME}.compute
            )
        endif()
    endforeach()
endmacro()

add_subdirectory(Source)
add_subdirectory(Examples)
