# =============================================================================
# check_symbols.cmake — assert the marker symbols survive compilation and link.
#
# Run by CTest (see the top-level CMakeLists.txt):
#   cmake -DMODE=<msvc|nm> -DEXE=<exe> -DLIB=<lib> -DMAPFILE=<map>
#         "-DSYMBOLS=a;b;c" -P cmake/check_symbols.cmake
#
# Symbols are matched as substrings, which transparently handles the leading
# underscore MSVC adds to cdecl C symbols on x86 (`_DCApiEnd`) and the
# underscore-free names on x64 and ELF (`DCApiEnd`).
# =============================================================================

if(NOT DEFINED SYMBOLS)
    message(FATAL_ERROR "check_symbols: SYMBOLS not provided")
endif()

set(_haystack "")
set(_sources "")

if(MODE STREQUAL "msvc")
    # 1) dumpbin /SYMBOLS on the static library: proves the compiler emitted a
    #    real, defined symbol for each marker (defeats "compiler inlined it").
    find_program(DUMPBIN NAMES dumpbin)
    if(DUMPBIN AND EXISTS "${LIB}")
        execute_process(
            COMMAND "${DUMPBIN}" /SYMBOLS "${LIB}"
            OUTPUT_VARIABLE _dump
            RESULT_VARIABLE _rc)
        if(NOT _rc EQUAL 0)
            message(FATAL_ERROR "check_symbols: dumpbin failed (${_rc}) on ${LIB}")
        endif()
        string(APPEND _haystack "${_dump}\n")
        list(APPEND _sources "dumpbin(${LIB})")
    endif()

    # 2) linker .map of the final image: proves the symbols survived linking
    #    (defeats "/OPT:REF dead-stripped it") and made it into the image whose
    #    PDB the engine consumes.
    if(EXISTS "${MAPFILE}")
        file(READ "${MAPFILE}" _map)
        string(APPEND _haystack "${_map}\n")
        list(APPEND _sources "map(${MAPFILE})")
    endif()

    if(_sources STREQUAL "")
        message(FATAL_ERROR
            "check_symbols: neither dumpbin nor a .map file was available; "
            "cannot verify symbols (LIB=${LIB} MAPFILE=${MAPFILE})")
    endif()

elseif(MODE STREQUAL "nm")
    # nm on the linked, unstripped ELF executable lists the marker symbols in
    # the .text section.
    find_program(NM NAMES nm)
    if(NOT NM)
        message(FATAL_ERROR "check_symbols: nm not found on PATH")
    endif()
    execute_process(
        COMMAND "${NM}" "${EXE}"
        OUTPUT_VARIABLE _dump
        RESULT_VARIABLE _rc)
    if(NOT _rc EQUAL 0)
        message(FATAL_ERROR "check_symbols: nm failed (${_rc}) on ${EXE}")
    endif()
    set(_haystack "${_dump}")
    list(APPEND _sources "nm(${EXE})")

else()
    message(FATAL_ERROR "check_symbols: unknown MODE='${MODE}' (expected msvc|nm)")
endif()

# -----------------------------------------------------------------------------
# Assert every requested symbol is present.
# -----------------------------------------------------------------------------
set(_missing "")
foreach(sym IN LISTS SYMBOLS)
    string(FIND "${_haystack}" "${sym}" _pos)
    if(_pos EQUAL -1)
        list(APPEND _missing "${sym}")
    endif()
endforeach()

if(_missing)
    message(FATAL_ERROR
        "check_symbols: MISSING marker symbol(s): ${_missing}\n"
        "  scanned: ${_sources}\n"
        "  This means the markers were inlined, folded, or dead-stripped — "
        "the SDK would be unusable in this configuration.")
endif()

list(LENGTH SYMBOLS _n)
message(STATUS "check_symbols: all ${_n} marker symbols present (scanned: ${_sources})")
