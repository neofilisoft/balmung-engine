cmake_minimum_required(VERSION 3.22)
project(VoxelBlockEngine VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(VB_BUILD_RENDERER "Build OpenGL renderer" ON)
option(VB_BUILD_CAPI "Build C API shared library" ON)
option(VB_BUILD_PYTHON "Build Python binding" ON)
option(VB_BUILD_TESTS "Build unit tests" ON)
option(VB_BUILD_GAME "Build standalone game executable" ON)

set(VB_PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(VB_INCLUDE_DIR "${VB_PROJECT_ROOT}/include")
set(VB_THIRD_PARTY_DIR "${VB_PROJECT_ROOT}/thirdparty")
set(VB_THIRD_PARTY_LEGACY_DIR "${VB_PROJECT_ROOT}/third_party")

function(vb_check_paths out_var)
  set(_missing "")
  foreach(_rel IN LISTS ARGN)
    if(NOT EXISTS "${VB_PROJECT_ROOT}/${_rel}")
      list(APPEND _missing "${_rel}")
    endif()
  endforeach()

  if(_missing)
    list(JOIN _missing ", " _missing_text)
    set(${out_var} FALSE PARENT_SCOPE)
    set(${out_var}_MISSING "${_missing_text}" PARENT_SCOPE)
  else()
    set(${out_var} TRUE PARENT_SCOPE)
    set(${out_var}_MISSING "" PARENT_SCOPE)
  endif()
endfunction()

function(vb_make_placeholder target_name reason)
  add_library(${target_name} INTERFACE)
  message(STATUS "[VB] ${target_name}: ${reason}")
endfunction()

find_package(OpenGL QUIET)
find_package(glfw3 QUIET)
find_package(Lua QUIET)
find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
  pkg_check_modules(GLFW glfw3 QUIET)
  pkg_check_modules(LUA54 lua5.4 QUIET)
endif()

set(VB_COMMON_INCLUDE_DIRS "${VB_INCLUDE_DIR}")
if(EXISTS "${VB_THIRD_PARTY_DIR}")
  list(APPEND VB_COMMON_INCLUDE_DIRS "${VB_THIRD_PARTY_DIR}")
elseif(EXISTS "${VB_THIRD_PARTY_LEGACY_DIR}")
  list(APPEND VB_COMMON_INCLUDE_DIRS "${VB_THIRD_PARTY_LEGACY_DIR}")
  message(STATUS "[VB] Using legacy third_party path: ${VB_THIRD_PARTY_LEGACY_DIR}")
endif()

set(VB_CORE_SOURCES
  src/core/event_system.cpp
  src/core/voxel.cpp
  src/core/chunk.cpp
  src/core/world.cpp
  src/core/inventory.cpp
  src/core/crafting.cpp
  src/core/save_manager.cpp
  src/core/player.cpp
)
set(VB_CORE_REQUIRED_HEADERS
  include/core/types.hpp
  include/core/event_system.hpp
  include/core/voxel.hpp
  include/core/chunk.hpp
  include/core/world.hpp
  include/core/inventory.hpp
  include/core/crafting.hpp
  include/core/save_manager.hpp
  include/core/player.hpp
)

vb_check_paths(VB_CORE_READY ${VB_CORE_SOURCES} ${VB_CORE_REQUIRED_HEADERS})
set(VB_CORE_REAL OFF)
if(VB_CORE_READY)
  add_library(voxelblock_core STATIC ${VB_CORE_SOURCES})
  target_include_directories(voxelblock_core PUBLIC ${VB_COMMON_INCLUDE_DIRS})
  set(VB_CORE_REAL ON)
  message(STATUS "[VB] Phase 1: core")
else()
  vb_make_placeholder(voxelblock_core "Phase 1 core skipped (missing files: ${VB_CORE_READY_MISSING})")
endif()

set(VB_RENDERER_SOURCES
  src/renderer/window.cpp
  src/renderer/shader.cpp
  src/renderer/camera.cpp
  src/renderer/mesh.cpp
  src/renderer/renderer.cpp
)
set(VB_RENDERER_REQUIRED_HEADERS
  include/renderer/window.hpp
  include/renderer/shader.hpp
  include/renderer/camera.hpp
  include/renderer/mesh.hpp
  include/renderer/renderer.hpp
  include/core/types.hpp
  include/core/world.hpp
  include/core/chunk.hpp
  include/core/voxel.hpp
)
vb_check_paths(VB_RENDERER_READY ${VB_RENDERER_SOURCES} ${VB_RENDERER_REQUIRED_HEADERS})

set(VB_RENDERER_REAL OFF)
if(VB_BUILD_RENDERER AND OpenGL_FOUND AND VB_RENDERER_READY AND VB_CORE_REAL)
  add_library(voxelblock_renderer STATIC ${VB_RENDERER_SOURCES})
  target_include_directories(voxelblock_renderer PUBLIC ${VB_COMMON_INCLUDE_DIRS})
  target_link_libraries(voxelblock_renderer PUBLIC voxelblock_core OpenGL::GL)
  if(glfw3_FOUND)
    target_link_libraries(voxelblock_renderer PUBLIC glfw)
  elseif(GLFW_FOUND)
    target_include_directories(voxelblock_renderer PUBLIC ${GLFW_INCLUDE_DIRS})
    target_link_libraries(voxelblock_renderer PUBLIC ${GLFW_LIBRARIES})
  endif()
  set(VB_RENDERER_REAL ON)
  message(STATUS "[VB] Phase 2: renderer (OpenGL)")
elseif(NOT VB_BUILD_RENDERER)
  vb_make_placeholder(voxelblock_renderer "Phase 2 renderer disabled by option")
elseif(NOT OpenGL_FOUND)
  vb_make_placeholder(voxelblock_renderer "Phase 2 renderer skipped (OpenGL not found)")
elseif(NOT VB_RENDERER_READY)
  vb_make_placeholder(voxelblock_renderer "Phase 2 renderer skipped (missing files: ${VB_RENDERER_READY_MISSING})")
elseif(NOT VB_CORE_REAL)
  vb_make_placeholder(voxelblock_renderer "Phase 2 renderer skipped (core target not buildable in this checkout)")
else()
  vb_make_placeholder(voxelblock_renderer "Phase 2 renderer skipped")
endif()

set(VB_SCRIPTING_SOURCES
  src/scripting/lua_runtime.cpp
)
set(VB_SCRIPTING_REQUIRED_HEADERS
  include/scripting/lua_runtime.hpp
  include/core/world.hpp
  include/core/inventory.hpp
  include/core/voxel.hpp
  include/core/crafting.hpp
  include/core/event_system.hpp
)
vb_check_paths(VB_SCRIPTING_READY ${VB_SCRIPTING_SOURCES} ${VB_SCRIPTING_REQUIRED_HEADERS})

set(VB_SCRIPTING_REAL OFF)
if(VB_SCRIPTING_READY AND VB_CORE_REAL)
  add_library(voxelblock_scripting STATIC ${VB_SCRIPTING_SOURCES})
  target_include_directories(voxelblock_scripting PUBLIC ${VB_COMMON_INCLUDE_DIRS})
  target_link_libraries(voxelblock_scripting PUBLIC voxelblock_core)
  if(LUA_FOUND)
    target_link_libraries(voxelblock_scripting PUBLIC ${LUA_LIBRARIES})
    target_include_directories(voxelblock_scripting PUBLIC ${LUA_INCLUDE_DIR})
  elseif(LUA54_FOUND)
    target_link_libraries(voxelblock_scripting PUBLIC ${LUA54_LIBRARIES})
    target_include_directories(voxelblock_scripting PUBLIC ${LUA54_INCLUDE_DIRS})
  endif()
  set(VB_SCRIPTING_REAL ON)
  message(STATUS "[VB] Scripting: lua_runtime")
elseif(NOT VB_SCRIPTING_READY)
  vb_make_placeholder(voxelblock_scripting "Scripting skipped (missing files: ${VB_SCRIPTING_READY_MISSING})")
elseif(NOT VB_CORE_REAL)
  vb_make_placeholder(voxelblock_scripting "Scripting skipped (core target not buildable in this checkout)")
else()
  vb_make_placeholder(voxelblock_scripting "Scripting skipped")
endif()

set(VB_ENGINE_SOURCES
  src/engine.cpp
)
set(VB_ENGINE_REQUIRED_HEADERS
  include/engine.hpp
  include/renderer/camera.hpp
  include/renderer/renderer.hpp
  include/scripting/lua_runtime.hpp
  include/core/world.hpp
  include/core/inventory.hpp
  include/core/crafting.hpp
  include/core/save_manager.hpp
  include/core/player.hpp
)
vb_check_paths(VB_ENGINE_READY ${VB_ENGINE_SOURCES} ${VB_ENGINE_REQUIRED_HEADERS})

set(VB_ENGINE_REAL OFF)
if(VB_ENGINE_READY AND VB_CORE_REAL AND VB_RENDERER_REAL AND VB_SCRIPTING_REAL)
  add_library(voxelblock_engine STATIC ${VB_ENGINE_SOURCES})
  target_include_directories(voxelblock_engine PUBLIC ${VB_COMMON_INCLUDE_DIRS})
  target_link_libraries(voxelblock_engine PUBLIC
    voxelblock_core
    voxelblock_renderer
    voxelblock_scripting
  )
  set(VB_ENGINE_REAL ON)
  message(STATUS "[VB] Engine: voxelblock_engine")
elseif(NOT VB_ENGINE_READY)
  vb_make_placeholder(voxelblock_engine "Engine skipped (missing files: ${VB_ENGINE_READY_MISSING})")
elseif(NOT VB_CORE_REAL)
  vb_make_placeholder(voxelblock_engine "Engine skipped (core target not buildable in this checkout)")
elseif(NOT VB_RENDERER_REAL)
  vb_make_placeholder(voxelblock_engine "Engine skipped (renderer target not buildable; current engine.cpp requires renderer)")
elseif(NOT VB_SCRIPTING_REAL)
  vb_make_placeholder(voxelblock_engine "Engine skipped (scripting target not buildable; current engine.cpp requires scripting)")
else()
  vb_make_placeholder(voxelblock_engine "Engine skipped")
endif()

set(VB_CAPI_FILES
  src/api/voxelblock_capi.cpp
  include/api/voxelblock_capi.h
)
vb_check_paths(VB_CAPI_READY ${VB_CAPI_FILES})
set(VB_CAPI_REAL OFF)

if(VB_BUILD_CAPI AND VB_CAPI_READY AND VB_ENGINE_REAL)
  add_library(voxelblock SHARED src/api/voxelblock_capi.cpp)
  target_include_directories(voxelblock PUBLIC ${VB_COMMON_INCLUDE_DIRS})
  target_link_libraries(voxelblock PRIVATE voxelblock_engine)
  set_target_properties(voxelblock PROPERTIES
    OUTPUT_NAME "voxelblock"
    VERSION 2.0.0
    SOVERSION 2
  )
  if(APPLE)
    set_target_properties(voxelblock PROPERTIES INSTALL_RPATH "@loader_path")
  elseif(UNIX)
    set_target_properties(voxelblock PROPERTIES INSTALL_RPATH "$ORIGIN")
  endif()
  set(VB_CAPI_REAL ON)
  message(STATUS "[VB] Phase 3: C API shared library")
elseif(NOT VB_BUILD_CAPI)
  message(STATUS "[VB] Phase 3: C API disabled by option")
elseif(NOT VB_CAPI_READY)
  message(STATUS "[VB] Phase 3: C API skipped (missing files: ${VB_CAPI_READY_MISSING})")
else()
  message(STATUS "[VB] Phase 3: C API skipped (engine target not buildable in this checkout)")
endif()

set(VB_GAME_SOURCES src/main.cpp)
vb_check_paths(VB_GAME_READY ${VB_GAME_SOURCES})
if(VB_BUILD_GAME AND VB_GAME_READY AND VB_ENGINE_REAL)
  add_executable(voxelblock_game src/main.cpp)
  target_include_directories(voxelblock_game PRIVATE ${VB_COMMON_INCLUDE_DIRS})
  target_link_libraries(voxelblock_game PRIVATE voxelblock_engine)
  message(STATUS "[VB] Standalone game executable")
elseif(VB_BUILD_GAME)
  if(NOT VB_GAME_READY)
    message(STATUS "[VB] Game skipped (missing files: ${VB_GAME_READY_MISSING})")
  elseif(NOT VB_ENGINE_REAL)
    message(STATUS "[VB] Game skipped (engine target not buildable in this checkout)")
  endif()
endif()

set(VB_PYTHON_FILES bindings/python/voxelblock_py.cpp)
vb_check_paths(VB_PYTHON_READY ${VB_PYTHON_FILES})
if(VB_BUILD_PYTHON AND VB_PYTHON_READY AND VB_ENGINE_REAL)
  find_package(pybind11 QUIET)
  if(pybind11_FOUND)
    pybind11_add_module(voxelblock_py bindings/python/voxelblock_py.cpp)
    target_include_directories(voxelblock_py PRIVATE ${VB_COMMON_INCLUDE_DIRS})
    target_link_libraries(voxelblock_py PRIVATE voxelblock_engine)
    set_target_properties(voxelblock_py PROPERTIES OUTPUT_NAME "voxelblock")
    message(STATUS "[VB] Phase 4: Python binding")
  else()
    message(STATUS "[VB] Phase 4: Python skipped (pybind11 not found)")
  endif()
elseif(VB_BUILD_PYTHON)
  if(NOT VB_PYTHON_READY)
    message(STATUS "[VB] Phase 4: Python skipped (missing files: ${VB_PYTHON_READY_MISSING})")
  elseif(NOT VB_ENGINE_REAL)
    message(STATUS "[VB] Phase 4: Python skipped (engine target not buildable in this checkout)")
  endif()
endif()

set(VB_TEST_SOURCES
  tests/test_runner.cpp
  tests/test_crafting.cpp
  tests/test_inventory.cpp
  tests/test_world.cpp
  tests/test_events.cpp
)
vb_check_paths(VB_TESTS_READY ${VB_TEST_SOURCES})
if(VB_BUILD_TESTS AND VB_TESTS_READY AND VB_CORE_REAL)
  enable_testing()
  add_executable(voxelblock_tests ${VB_TEST_SOURCES})
  target_include_directories(voxelblock_tests PRIVATE ${VB_COMMON_INCLUDE_DIRS} tests)
  target_link_libraries(voxelblock_tests PRIVATE voxelblock_core)
  add_test(NAME CoreTests COMMAND voxelblock_tests)
  message(STATUS "[VB] Tests: enabled")
elseif(VB_BUILD_TESTS)
  if(NOT VB_TESTS_READY)
    message(STATUS "[VB] Tests skipped (missing files: ${VB_TESTS_READY_MISSING})")
  elseif(NOT VB_CORE_REAL)
    message(STATUS "[VB] Tests skipped (core target not buildable in this checkout)")
  endif()
endif()

if(VB_CORE_REAL)
  install(TARGETS voxelblock_core ARCHIVE DESTINATION lib)
endif()
if(VB_CAPI_REAL)
  install(TARGETS voxelblock LIBRARY DESTINATION lib RUNTIME DESTINATION bin)
endif()
if(EXISTS "${VB_INCLUDE_DIR}")
  install(DIRECTORY include/ DESTINATION include)
endif()
if(EXISTS "${VB_PROJECT_ROOT}/shaders")
  install(DIRECTORY shaders/ DESTINATION share/voxelblock/shaders)
endif()
