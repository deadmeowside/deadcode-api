cmake_minimum_required(VERSION 3.15)

project(deadcode-api
    VERSION 0.1.0
    DESCRIPTION "Source-level protection markers for the deadcode binary protector"
    LANGUAGES C CXX)

# -----------------------------------------------------------------------------
# Options
# -----------------------------------------------------------------------------
option(DEADCODE_API_BUILD_EXAMPLES "Build the example programs" ON)
option(DEADCODE_API_BUILD_TESTS    "Build the symbol-survival test"  ON)

if(NOT CMAKE_CXX_STANDARD)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

include(GNUInstallDirs)

# -----------------------------------------------------------------------------
# The SDK static library.
#
# This is deliberately a *static* library, not header-only: the marker
# functions must become real symbols in the consumer's binary and PDB so the
# engine can find them post-build.  See src/deadcode-api.cpp for the anti-inline
# and anti-fold reasoning.
# -----------------------------------------------------------------------------
add_library(deadcode-api STATIC src/deadcode-api.cpp)
add_library(deadcode-api::deadcode-api ALIAS deadcode-api)

target_include_directories(deadcode-api PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

set_target_properties(deadcode-api PROPERTIES
    OUTPUT_NAME deadcode-api
    POSITION_INDEPENDENT_CODE ON)

# -----------------------------------------------------------------------------
# Debug-info policy for *consumers* of the SDK (examples + test).
#
# The engine reads symbols from a PDB, so any binary you intend to protect must
# be compiled with debug info AND linked with /DEBUG, in BOTH Debug and Release.
# We apply that here to every executable that links the SDK so the symbol-
# survival test exercises the realistic shipping case.
# -----------------------------------------------------------------------------
function(deadcode_api_enable_pdb target)
    if(MSVC)
        # /Zi: emit debug info even in Release.  /DEBUG: keep a full PDB at link
        # time.  /OPT:REF,ICF are MSVC's Release defaults and are intentionally
        # left on — the SDK is built to survive them.
        target_compile_options(${target} PRIVATE /Zi)
        target_link_options(${target} PRIVATE /DEBUG)
    endif()
endfunction()

# -----------------------------------------------------------------------------
# Examples
# -----------------------------------------------------------------------------
if(DEADCODE_API_BUILD_EXAMPLES)
    set(_dc_examples
        01_protection_levels
        02_license_check
        03_self_integrity
        04_cpp_member_function)
    foreach(ex IN LISTS _dc_examples)
        add_executable(${ex} examples/${ex}.cpp)
        target_link_libraries(${ex} PRIVATE deadcode-api::deadcode-api)
        deadcode_api_enable_pdb(${ex})
        set_target_properties(${ex} PROPERTIES FOLDER examples)
    endforeach()
endif()

# -----------------------------------------------------------------------------
# Symbol-survival test
#
# Builds a program that calls every marker, then asserts (via CTest) that all
# seven marker symbols are present in the built artifacts.  Run it in BOTH
# Debug and Release — Release is where inlining/folding would bite if the SDK
# got its guards wrong.
# -----------------------------------------------------------------------------
if(DEADCODE_API_BUILD_TESTS)
    enable_testing()
    add_executable(symbol_survival test/symbol_survival.cpp)
    target_link_libraries(symbol_survival PRIVATE deadcode-api::deadcode-api)
    deadcode_api_enable_pdb(symbol_survival)

    if(WIN32)
        # On a linked PE image the COFF symbol table is stripped (symbols live
        # in the PDB), so we check two places dumpbin/the linker can see:
        #   * the static library  — proves the compiler emitted real symbols
        #   * the linker .map file — proves they survived linking into the image
        target_link_options(symbol_survival PRIVATE "/MAP:$<TARGET_FILE_DIR:symbol_survival>/symbol_survival.map")
        set(_dc_mode msvc)
    else()
        # ELF: nm on the linked, unstripped executable shows the marker symbols.
        set(_dc_mode nm)
    endif()

    add_test(NAME symbol_survival
        COMMAND ${CMAKE_COMMAND}
            -DMODE=${_dc_mode}
            -DEXE=$<TARGET_FILE:symbol_survival>
            -DLIB=$<TARGET_FILE:deadcode-api>
            -DMAPFILE=$<TARGET_FILE_DIR:symbol_survival>/symbol_survival.map
            "-DSYMBOLS=DCApiBeginVirtualization;DCApiBeginMutation;DCApiBeginUltra;DCApiEnd;DCApiBeginCheck;DCApiEndCheck;DCApiCheckIntegrity"
            -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/check_symbols.cmake)
endif()

# -----------------------------------------------------------------------------
# Install + package config
# -----------------------------------------------------------------------------
install(TARGETS deadcode-api
    EXPORT deadcode-api-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES include/deadcode-api.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT deadcode-api-targets
    FILE deadcode-api-targets.cmake
    NAMESPACE deadcode-api::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/deadcode-api)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/deadcode-api-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion)

# Minimal config file: the SDK has no transitive dependencies.
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/deadcode-api-config.cmake"
"include(\"\${CMAKE_CURRENT_LIST_DIR}/deadcode-api-targets.cmake\")\n")

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/deadcode-api-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/deadcode-api-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/deadcode-api)
