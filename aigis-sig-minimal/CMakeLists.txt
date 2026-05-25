cmake_minimum_required(VERSION 3.10)

project(PQMagicAigisSigMinimal C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

get_filename_component(PQMAGIC_ROOT "${CMAKE_CURRENT_LIST_DIR}/.." ABSOLUTE)

set(AIGIS_SIG_MODES "1;2;3" CACHE STRING "Aigis-sig modes to compile")
option(USE_SM3 "Use SM3 as hash component" ON)
option(USE_SHAKE "Use SHAKE as hash component" OFF)

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

add_library(aigis_sig_minimal_randombytes OBJECT
    "${PQMAGIC_ROOT}/utils/randombytes.c"
)
target_include_directories(aigis_sig_minimal_randombytes PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/provider_compat"
    "${PQMAGIC_ROOT}/include"
    "${PQMAGIC_ROOT}/utils"
)

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
    $<TARGET_OBJECTS:aigis_sig_minimal_randombytes>
    $<TARGET_OBJECTS:aigis_sig_minimal_hash>
)

set_target_properties(pqmagic_aigis_sig_minimal_static PROPERTIES
    OUTPUT_NAME "pqmagic_aigis_sig_minimal_std"
)

add_library(pqmagic_aigis_sig_minimal SHARED
    ${AIGIS_SIG_OBJECTS}
    $<TARGET_OBJECTS:aigis_sig_minimal_provider>
    $<TARGET_OBJECTS:aigis_sig_minimal_randombytes>
    $<TARGET_OBJECTS:aigis_sig_minimal_hash>
)

set_target_properties(pqmagic_aigis_sig_minimal PROPERTIES
    OUTPUT_NAME "pqmagic_aigis_sig_minimal_std"
)
