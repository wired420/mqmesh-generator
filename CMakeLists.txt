cmake_minimum_required(VERSION 3.20)
project(MeshGeneratorWeb C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Release)
endif()

set(SRC ${CMAKE_SOURCE_DIR}/src)
set(TP  ${CMAKE_SOURCE_DIR}/third_party)
set(MQ  ${CMAKE_SOURCE_DIR}/mq)

if(EMSCRIPTEN)
	set(PROTOBUF_ROOT ${TP}/protobuf-wasm)
	set(THREAD_FLAGS -pthread)
	add_compile_options(-pthread -sUSE_SDL=2 -sUSE_ZLIB=1)
else()
	set(PROTOBUF_ROOT ${TP}/protobuf-host)
	find_package(ZLIB REQUIRED)
	find_package(Threads REQUIRED)
endif()

add_compile_options(
	"SHELL:-include ${CMAKE_SOURCE_DIR}/platform/compat.h"
	-Wno-unused-value -Wno-deprecated-declarations -Wno-format-security
	-Wno-invalid-offsetof -Wno-switch -Wno-unknown-pragmas -Wno-c++11-narrowing
	-Wno-microsoft-template -Wno-writable-strings
)
add_compile_definitions(FMT_HEADER_ONLY SPDLOG_FMT_EXTERNAL SPDLOG_HEADER_ONLY GLM_ENABLE_EXPERIMENTAL
	GLM_FORCE_SILENT_WARNINGS GLM_FORCE_CTOR_INIT _USE_MATH_DEFINES NOMINMAX)

# Generated include dir that merges "imgui/..." from the imgui core and the MQ imgui utils.
set(GEN_INC ${CMAKE_BINARY_DIR}/gen-include)
file(MAKE_DIRECTORY ${GEN_INC}/imgui)
file(GLOB _imgui_core_files ${MQ}/imgui-core/*.h ${MQ}/imgui-core/*.cpp)
file(GLOB _mq_imgui_files ${MQ}/src/imgui/*.h)
foreach(f ${_imgui_core_files} ${_mq_imgui_files})
	get_filename_component(n ${f} NAME)
	file(CREATE_LINK ${f} ${GEN_INC}/imgui/${n} SYMBOLIC)
endforeach()
file(CREATE_LINK ${MQ}/src/imgui/fonts ${GEN_INC}/imgui/fonts SYMBOLIC)
file(CREATE_LINK ${MQ}/imgui-core/misc ${GEN_INC}/imgui/misc SYMBOLIC)

set(COMMON_INCLUDES
	${SRC}
	${SRC}/dependencies
	${SRC}/dependencies/zone-utilities/common
	${SRC}/dependencies/zone-utilities/log
	${SRC}/dependencies/recast/Recast/Include
	${SRC}/dependencies/recast/Detour/Include
	${SRC}/dependencies/recast/DetourCrowd/Include
	${SRC}/dependencies/recast/DetourTileCache/Include
	${SRC}/dependencies/recast/DebugUtils/Include
	${MQ}/include
	${MQ}/imgui-core
	${GEN_INC}
	${TP}
	${TP}/glm
	${TP}/fmt/include
	${TP}/spdlog/include
	${TP}/rapidjson/include
	${PROTOBUF_ROOT}/include
)
include_directories(${COMMON_INCLUDES} ${CMAKE_SOURCE_DIR})
if(NOT EMSCRIPTEN)
	add_compile_definitions(MESHGEN_HEADLESS=1)
endif()

# ---- Recast / Detour
file(GLOB RECAST_SRC
	${SRC}/dependencies/recast/Recast/Source/*.cpp
	${SRC}/dependencies/recast/Detour/Source/*.cpp
	${SRC}/dependencies/recast/DetourCrowd/Source/*.cpp
	${SRC}/dependencies/recast/DetourTileCache/Source/*.cpp
	${SRC}/dependencies/recast/DebugUtils/Source/*.cpp)
add_library(recast STATIC ${RECAST_SRC})

# ---- EQEmu zone-utilities (loaders only)
set(ZU ${SRC}/dependencies/zone-utilities)
add_library(zoneutils STATIC
	${ZU}/common/compression.cpp
	${ZU}/common/eqg_loader.cpp
	${ZU}/common/eqg_model_loader.cpp
	${ZU}/common/eqg_v4_loader.cpp
	${ZU}/common/oriented_bounding_box.cpp
	${ZU}/common/pfs.cpp
	${ZU}/common/pfs_crc.cpp
	${ZU}/common/s3d_loader.cpp
	${ZU}/common/string_util.cpp
	${ZU}/common/wld_fragment.cpp
	${ZU}/log/log_manager.cpp
	${ZU}/log/log_stdout.cpp
	${ZU}/log/log_file.cpp)
target_include_directories(zoneutils PRIVATE ${ZU}/common ${ZU}/log)

# ---- imgui (MQ fork) + MQ imgui utils
add_library(imgui STATIC
	${MQ}/imgui-core/imgui.cpp
	${MQ}/imgui-core/imgui_draw.cpp
	${MQ}/imgui-core/imgui_tables.cpp
	${MQ}/imgui-core/imgui_widgets.cpp
	${MQ}/imgui-core/imgui_demo.cpp
	${MQ}/imgui-core/imgui_stacklayout.cpp
	${MQ}/imgui-core/misc/cpp/imgui_stdlib.cpp
	${MQ}/src/imgui/ImGuiUtils.cpp
	${MQ}/src/imgui/fonts/font_fontawesome_ttf.cpp
	${MQ}/src/imgui/fonts/font_materialicons_ttf.cpp
	${MQ}/src/imgui/fonts/font_roboto_regular_ttf.cpp
	${CMAKE_SOURCE_DIR}/platform/imgui_platform.cpp)
target_include_directories(imgui PRIVATE ${MQ}/src/imgui)

# ---- MQ2Nav common
add_library(navcommon STATIC
	${SRC}/common/NavMesh.cpp
	${SRC}/common/NavMeshData.cpp
	${SRC}/common/Utilities.cpp
	${SRC}/common/ZoneData.cpp
	${SRC}/common/JsonProto.cpp
	${SRC}/common/proto/NavMeshFile.pb.cc)

# ---- Mesh generator core (no window/renderer)
set(MESHGEN_CORE
	${SRC}/meshgen/ChunkyTriMesh.cpp
	${SRC}/meshgen/InputGeom.cpp
	${SRC}/meshgen/MapGeometryLoader.cpp
	${SRC}/meshgen/NavMeshTool.cpp
	${SRC}/meshgen/ConvexVolumeTool.cpp
	${SRC}/meshgen/NavMeshInfoTool.cpp
	${SRC}/meshgen/NavMeshPruneTool.cpp
	${SRC}/meshgen/NavMeshTesterTool.cpp
	${SRC}/meshgen/NavMeshTileTool.cpp
	${SRC}/meshgen/OffMeshConnectionTool.cpp
	${SRC}/meshgen/WaypointsTool.cpp
	${SRC}/meshgen/ImGuiWidgets.cpp
	${SRC}/meshgen/EQConfig.cpp
	${SRC}/meshgen/DebugDraw.cpp
	${CMAKE_SOURCE_DIR}/platform/gl_renderer.cpp)
add_library(meshgencore STATIC ${MESHGEN_CORE})
target_compile_options(meshgencore PRIVATE "SHELL:-include ${SRC}/meshgen/pch.h")

set(LINK_LIBS meshgencore navcommon zoneutils recast imgui
	${PROTOBUF_ROOT}/lib/libprotobuf.a)

if(EMSCRIPTEN)
	set(IMGUI_BACKENDS
		${TP}/imgui-upstream/backends/imgui_impl_sdl2.cpp
		${TP}/imgui-upstream/backends/imgui_impl_opengl3.cpp)
	add_executable(meshgen
		${SRC}/meshgen/main.cpp
		${SRC}/meshgen/Application.cpp
		${SRC}/meshgen/ZonePicker.cpp
		${CMAKE_SOURCE_DIR}/platform/web_files.cpp
		${IMGUI_BACKENDS})
	target_include_directories(meshgen PRIVATE ${TP}/imgui-upstream/backends)
	target_compile_options(meshgen PRIVATE "SHELL:-include ${SRC}/meshgen/pch.h")
	target_link_libraries(meshgen ${LINK_LIBS})
	target_link_options(meshgen PRIVATE
		-pthread -sUSE_SDL=2 -sUSE_ZLIB=1
		-sUSE_WEBGL2=1 -sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2 -sFULL_ES3=1
		-sALLOW_MEMORY_GROWTH=1 -sMAXIMUM_MEMORY=4GB -sINITIAL_MEMORY=256MB
		-sPTHREAD_POOL_SIZE=navigator.hardwareConcurrency+2
		-sSTACK_SIZE=4MB -sDEFAULT_PTHREAD_STACK_SIZE=2MB
		-sEXPORTED_RUNTIME_METHODS=['FS','ccall','cwrap','HEAPU8']
		-sEXPORTED_FUNCTIONS=['_main','_meshgen_files_ready','_meshgen_open_zone','_meshgen_navmesh_ready','_meshgen_web_pick_folder','_meshgen_web_open_zone','_meshgen_web_build','_meshgen_web_status','_meshgen_web_save','_meshgen_web_publish','_meshgen_web_output_path','_malloc','_free']
		-sFORCE_FILESYSTEM=1
		-sENVIRONMENT=web,worker
		-sEXIT_RUNTIME=0
		--embed-file ${SRC}/resources/Zones.ini@/app/resources/Zones.ini
		--js-library ${CMAKE_SOURCE_DIR}/platform/web_files.js)
	set_target_properties(meshgen PROPERTIES SUFFIX ".js" LINK_DEPENDS "${CMAKE_SOURCE_DIR}/platform/web_files.js")
else()
	add_executable(meshgen-cli ${CMAKE_SOURCE_DIR}/platform/cli_main.cpp)
	target_compile_options(meshgen-cli PRIVATE "SHELL:-include ${SRC}/meshgen/pch.h")
	target_compile_definitions(meshgen-cli PRIVATE SOURCE_DIR="${CMAKE_SOURCE_DIR}")
	target_link_libraries(meshgen-cli ${LINK_LIBS} ZLIB::ZLIB Threads::Threads)
endif()

if(EMSCRIPTEN)
	# Node.js build of the headless driver, for testing the wasm build of the core.
	add_executable(meshgen-node ${CMAKE_SOURCE_DIR}/platform/cli_main.cpp)
	target_compile_options(meshgen-node PRIVATE "SHELL:-include ${SRC}/meshgen/pch.h")
	target_compile_definitions(meshgen-node PRIVATE SOURCE_DIR="${CMAKE_SOURCE_DIR}")
	target_link_libraries(meshgen-node meshgencore-headless navcommon zoneutils recast imgui ${PROTOBUF_ROOT}/lib/libprotobuf.a)
	target_link_options(meshgen-node PRIVATE -pthread -sUSE_ZLIB=1 -sNODERAWFS=1 -sENVIRONMENT=node
		-sALLOW_MEMORY_GROWTH=1 -sMAXIMUM_MEMORY=4GB -sPTHREAD_POOL_SIZE=8 -sSTACK_SIZE=4MB -sEXIT_RUNTIME=1)
	add_library(meshgencore-headless STATIC ${MESHGEN_CORE})
	target_compile_options(meshgencore-headless PRIVATE "SHELL:-include ${SRC}/meshgen/pch.h")
	target_compile_definitions(meshgencore-headless PRIVATE MESHGEN_HEADLESS=1)
	target_compile_definitions(meshgen-node PRIVATE MESHGEN_HEADLESS=1)
endif()
