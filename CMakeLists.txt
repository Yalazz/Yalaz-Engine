cmake_minimum_required(VERSION 3.15)

# Projeyi C++ diliyle sabitle
project(Yalaz_Engine LANGUAGES CXX)

cmake_policy(SET CMP0079 NEW)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# =============================================================
# BUILD TYPE CONFIGURATION (Debug / Release)
# =============================================================
# Default build type if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")

# =============================================================
# COMPILER FLAGS FOR DEBUG / RELEASE
# =============================================================
if (MSVC)
  # MSVC Compiler (Visual Studio 2017+)
  add_compile_options(/MP /Zc:preprocessor)

  # Debug: Full debug info, no optimization, validation enabled
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /Od /Zi /RTC1")

  # Release: Full optimization, NDEBUG defined (disables validation)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2 /Ob2 /DNDEBUG")

  # RelWithDebInfo: Optimization with debug symbols
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} /O2 /Zi /DNDEBUG")

else()
  # GCC / MinGW / Clang
  add_compile_options(-Wall -Wextra -Wno-unknown-pragmas)

  # Debug: No optimization, debug symbols, validation enabled
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g3 -ggdb")

  # Release: Full optimization, NDEBUG defined (disables validation)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -DNDEBUG -march=native")

  # RelWithDebInfo: Optimization with debug symbols
  set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -O2 -g -DNDEBUG")

  # MinSizeRel: Optimize for size
  set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -Os -DNDEBUG")
endif()

# Print compiler flags for current build type
message(STATUS "C++ Flags: ${CMAKE_CXX_FLAGS}")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "C++ Debug Flags: ${CMAKE_CXX_FLAGS_DEBUG}")
  message(STATUS "Validation Layers: ENABLED")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
  message(STATUS "C++ Release Flags: ${CMAKE_CXX_FLAGS_RELEASE}")
  message(STATUS "Validation Layers: DISABLED (NDEBUG defined)")
endif()

# =============================================================
# Vulkan SDK & glslangValidator
# =============================================================
if (WIN32)
  set(VULKAN_SDK_PATH $ENV{VULKAN_SDK})
  find_package(Vulkan REQUIRED)
elseif(APPLE)
  find_package(Vulkan REQUIRED)
  set(VULKAN_SDK_PATH "/Library/Frameworks/Vulkan.framework")
elseif(UNIX)
  find_package(Vulkan REQUIRED)
  set(VULKAN_SDK_PATH "/usr/local/VulkanSDK")
endif()

find_program(GLSL_VALIDATOR glslangValidator
  HINTS 
    ${VULKAN_SDK_PATH}/Bin/
    ${VULKAN_SDK_PATH}/Bin32/
    /usr/bin
    /usr/local/bin
    /opt/vulkan/bin
)
if (NOT GLSL_VALIDATOR)
  message(FATAL_ERROR "glslangValidator bulunamadı! VULKAN_SDK ortam değişkenini ve SDK kurulumunu kontrol et.")
endif()

# =============================================================
# OUTPUT DIRECTORIES - Separate Debug/Release builds
# =============================================================
# For single-config generators (Makefile, Ninja) - output to build type subfolder
# This ensures shader paths (../../shaders/) work correctly
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/${CMAKE_BUILD_TYPE}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/lib/${CMAKE_BUILD_TYPE}")

# For multi-config generators (Visual Studio, Xcode)
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${PROJECT_SOURCE_DIR}/bin/${OUTPUTCONFIG}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${PROJECT_SOURCE_DIR}/lib/${OUTPUTCONFIG}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UPPER} "${PROJECT_SOURCE_DIR}/lib/${OUTPUTCONFIG}")
endforeach()

# 3rd party ve kaynak hedefleri
add_subdirectory(third_party)
add_subdirectory(src)

# =============================================================
# SHADER DERLEME — KONFLİKTSİZ, VERBOSE, ALT KLASÖRLER KORUNUR
#  *** DİKKAT: -I<dir> (boşluksuz) kullanıyoruz ***
# =============================================================
set(SHADER_SRC_DIR  ${PROJECT_SOURCE_DIR}/shaders)
set(SHADER_BIN_DIR  ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY "${SHADER_BIN_DIR}")

# Tüm shaderları topla
file(GLOB_RECURSE GLSL_SOURCE_FILES
  "${SHADER_SRC_DIR}/*.frag"
  "${SHADER_SRC_DIR}/*.vert"
  "${SHADER_SRC_DIR}/*.comp"
)
list(REMOVE_DUPLICATES GLSL_SOURCE_FILES)

# (İsteğe bağlı) problemli dosyaları hariç tutmak istersen aç:
# list(FILTER GLSL_SOURCE_FILES EXCLUDE REGEX ".*sky - Kopya\\.comp$")

# ---- Include dizinlerini -I<dir> şeklinde derle ----
# ZORUNLU: shaders kökü
set(SHADER_INCLUDE_DIRS "${SHADER_SRC_DIR}")

# Eğer başka include klasörlerin varsa buraya EKLE:
# list(APPEND SHADER_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/shaders/includes")
# list(APPEND SHADER_INCLUDE_DIRS "${PROJECT_SOURCE_DIR}/some/other/dir")

# -I<dir> argümanlarını **boşluksuz** üret
set(SHADER_INCLUDE_ARGS "")
foreach(inc ${SHADER_INCLUDE_DIRS})
  # Windows için ileri slash kullan; boşluk yoksa tırnağa gerek yok
  file(TO_CMAKE_PATH "${inc}" inc_norm)
  list(APPEND SHADER_INCLUDE_ARGS "-I${inc_norm}")
endforeach()

# (İsteğe bağlı) bindless kullanıyorsan:
# set(SHADER_DEFINES "-DUSE_BINDLESS")

set(SPIRV_BINARY_FILES "")
foreach(GLSL ${GLSL_SOURCE_FILES})
  # Kaynağın klasörü ve shader köküne göre göreli yol
  get_filename_component(GLSL_DIR_ABS "${GLSL}" DIRECTORY)
  file(RELATIVE_PATH REL_DIR "${SHADER_SRC_DIR}" "${GLSL_DIR_ABS}")

  if (REL_DIR STREQUAL "")
    set(OUT_DIR "${SHADER_BIN_DIR}")
  else()
    set(OUT_DIR "${SHADER_BIN_DIR}/${REL_DIR}")
  endif()

  # Çıkış dosyası
  get_filename_component(FILE_NAME "${GLSL}" NAME)      # ör: mesh.frag
  set(SPIRV "${OUT_DIR}/${FILE_NAME}.spv")              # ör: build/shaders/pbr/mesh.frag.spv)

  add_custom_command(
    OUTPUT  "${SPIRV}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${OUT_DIR}"
    # ÖNEMLİ: ${SHADER_INCLUDE_ARGS} burada **çoklu argüman** olarak genişler.
    # MSVC/MSBuild altında da -I<dir> boşluksuz iletilir.
    COMMAND "${GLSL_VALIDATOR}" -V ${SHADER_INCLUDE_ARGS} ${SHADER_DEFINES} "${GLSL}" -o "${SPIRV}"
    DEPENDS "${GLSL}"
    COMMENT "Compiling shader: /${REL_DIR}/${FILE_NAME}"
    VERBATIM
    USES_TERMINAL
  )

  list(APPEND SPIRV_BINARY_FILES "${SPIRV}")
endforeach()

add_custom_target(Shaders ALL DEPENDS ${SPIRV_BINARY_FILES})

# Engine hedefi shaderlara burada bağlı (SADECE kökte!)
add_dependencies(Yalaz_Engine Shaders)
