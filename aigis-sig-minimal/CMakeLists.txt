cmake_minimum_required(VERSION 3.10)

project(PQMagicAigisSigMinimal C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

get_filename_component(PQMAGIC_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(AIGIS_SIG_MODES "1;2;3" CACHE STRING "Aigis-sig modes to compile")
set(AIGIS_RANDOMBYTES_SOURCE "${PQMAGIC_ROOT}/utils/randombytes.c" CACHE STRING
    "randombytes source file, or 'external' to leave randombytes unresolved for the final application")
option(USE_SM3 "Use SM3 as hash component" ON)
option(USE_SHAKE "Use SHAKE as hash component" OFF)
option(ENABLE_ASAN "Build with AddressSanitizer instrumentation" OFF)

if(ENABLE_ASAN)
    if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
        message(STATUS "AddressSanitizer: ENABLED")
        add_compile_options(-fsanitize=address -fno-omit-frame-pointer -g)
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
    else()
        message(FATAL_ERROR "ENABLE_ASAN is only wired for GCC/Clang-compatible compilers")
    endif()
endif()

if(USE_SHAKE)
    set(USE_SM3 OFF)
endif()

if((USE_SHAKE AND USE_SM3) OR ((NOT USE_SHAKE) AND (NOT USE_SM3)))
    message(FATAL_ERROR "Choose exactly one hash mode with -DUSE_SM3=ON or -DUSE_SHAKE=ON")
endif()

set(AIGIS_SIG_DIR "${PQMAGIC_ROOT}/sig/aigis-sig/std")

set(AIGIS_SIG_SOURCES
    "${AIGIS_SIG_DIR}/sign.c"
    "${AIGIS_SIG_DIR}/packing.c"
    "${AIGIS_SIG_DIR}/poly.c"
    "${AIGIS_SIG_DIR}/polyvec.c"
    "${AIGIS_SIG_DIR}/ntt.c"
    "${AIGIS_SIG_DIR}/reduce.c"
    "${AIGIS_SIG_DIR}/rounding.c"
)

set(COMMON_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/provider_compat"
    "${PQMAGIC_ROOT}"
    "${PQMAGIC_ROOT}/include"
    "${PQMAGIC_ROOT}/utils"
    "${AIGIS_SIG_DIR}"
)

set(AIGIS_SIG_OBJECTS "")
set(PROVIDER_MODE_DEFINES "")

foreach(MODE ${AIGIS_SIG_MODES})
    add_library(aigis_sig_minimal_objects_${MODE} OBJECT ${AIGIS_SIG_SOURCES})
    target_include_directories(aigis_sig_minimal_objects_${MODE} PRIVATE ${COMMON_INCLUDE_DIRS})
    target_compile_definitions(aigis_sig_minimal_objects_${MODE} PRIVATE AIGIS_SIG_MODE=${MODE})

    if(USE_SHAKE)
        target_compile_definitions(aigis_sig_minimal_objects_${MODE} PRIVATE USE_SHAKE)
    endif()

    list(APPEND AIGIS_SIG_OBJECTS $<TARGET_OBJECTS:aigis_sig_minimal_objects_${MODE}>)

    if(MODE STREQUAL "1")
        list(APPEND PROVIDER_MODE_DEFINES PQMAGIC_AIGIS_ENABLE_SIG1)
    elseif(MODE STREQUAL "2")
        list(APPEND PROVIDER_MODE_DEFINES PQMAGIC_AIGIS_ENABLE_SIG2)
    elseif(MODE STREQUAL "3")
        list(APPEND PROVIDER_MODE_DEFINES PQMAGIC_AIGIS_ENABLE_SIG3)
    else()
        message(FATAL_ERROR "Unsupported AIGIS_SIG_MODE: ${MODE}")
    endif()
endforeach()

add_library(aigis_sig_minimal_provider OBJECT
    "${CMAKE_CURRENT_SOURCE_DIR}/src/aigis_provider.c"
)
target_include_directories(aigis_sig_minimal_provider PRIVATE ${COMMON_INCLUDE_DIRS})
target_compile_definitions(aigis_sig_minimal_provider PRIVATE ${PROVIDER_MODE_DEFINES})

set(AIGIS_RANDOMBYTES_OBJECTS "")
if(AIGIS_RANDOMBYTES_SOURCE STREQUAL "external")
    message(STATUS "randombytes: external symbol supplied by final application")
else()
    if(NOT IS_ABSOLUTE "${AIGIS_RANDOMBYTES_SOURCE}")
        get_filename_component(AIGIS_RANDOMBYTES_SOURCE "${AIGIS_RANDOMBYTES_SOURCE}" ABSOLUTE)
    endif()
    message(STATUS "randombytes source: ${AIGIS_RANDOMBYTES_SOURCE}")
    add_library(aigis_sig_minimal_randombytes OBJECT
        "${AIGIS_RANDOMBYTES_SOURCE}"
    )
    target_include_directories(aigis_sig_minimal_randombytes PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/provider_compat"
        "${PQMAGIC_ROOT}/include"
        "${PQMAGIC_ROOT}/utils"
    )
    set(AIGIS_RANDOMBYTES_OBJECTS $<TARGET_OBJECTS:aigis_sig_minimal_randombytes>)
endif()

if(USE_SM3)
    add_library(aigis_sig_minimal_hash OBJECT
        "${PQMAGIC_ROOT}/hash/sm3/x86-64/sm3.c"
        "${PQMAGIC_ROOT}/hash/sm3/sm3_extended.c"
    )
    target_include_directories(aigis_sig_minimal_hash PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/provider_compat"
        "${PQMAGIC_ROOT}"
        "${PQMAGIC_ROOT}/include"
        "${PQMAGIC_ROOT}/hash/sm3"
    )
else()
    add_library(aigis_sig_minimal_hash OBJECT
        "${PQMAGIC_ROOT}/hash/keccak/fips202.c"
    )
    target_include_directories(aigis_sig_minimal_hash PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/provider_compat"
        "${PQMAGIC_ROOT}"
        "${PQMAGIC_ROOT}/include"
        "${PQMAGIC_ROOT}/hash/keccak"
    )
endif()

add_library(pqmagic_aigis_sig_minimal_static STATIC
    ${AIGIS_SIG_OBJECTS}
    $<TARGET_OBJECTS:aigis_sig_minimal_provider>
    ${AIGIS_RANDOMBYTES_OBJECTS}
    $<TARGET_OBJECTS:aigis_sig_minimal_hash>
)

set_target_properties(pqmagic_aigis_sig_minimal_static PROPERTIES
    OUTPUT_NAME "pqmagic_aigis_sig_minimal_std"
)

add_library(pqmagic_aigis_sig_minimal SHARED
    ${AIGIS_SIG_OBJECTS}
    $<TARGET_OBJECTS:aigis_sig_minimal_provider>
    ${AIGIS_RANDOMBYTES_OBJECTS}
    $<TARGET_OBJECTS:aigis_sig_minimal_hash>
)

set_target_properties(pqmagic_aigis_sig_minimal PROPERTIES
    OUTPUT_NAME "pqmagic_aigis_sig_minimal_std"
)
