cmake_minimum_required(VERSION 3.20)
project(TaskbarHider LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(NOT WIN32)
    message(FATAL_ERROR "TaskbarHider is Windows-only.")
endif()

add_executable(TaskbarHider WIN32
    src/main.cpp
    src/resource.rc
)

target_include_directories(TaskbarHider PRIVATE src)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(RUST_TARGET "x86_64-pc-windows-msvc")
else()
    set(RUST_TARGET "i686-pc-windows-msvc")
endif()

set(RUST_CORE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/rust_core")
set(RUST_CORE_LIB "${RUST_CORE_DIR}/target/${RUST_TARGET}/release/taskbarhider_core.lib")

add_custom_command(
    OUTPUT "${RUST_CORE_LIB}"
    COMMAND cargo build --manifest-path "${RUST_CORE_DIR}/Cargo.toml" --release --target "${RUST_TARGET}"
    DEPENDS
        "${RUST_CORE_DIR}/Cargo.toml"
        "${RUST_CORE_DIR}/src/lib.rs"
    WORKING_DIRECTORY "${RUST_CORE_DIR}"
    COMMENT "Building Rust core (${RUST_TARGET})"
    VERBATIM
)

add_custom_target(TaskbarHiderRustCore DEPENDS "${RUST_CORE_LIB}")
add_dependencies(TaskbarHider TaskbarHiderRustCore)

target_compile_definitions(TaskbarHider PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    STRICT
    _CRT_SECURE_NO_WARNINGS
)

if(MSVC)
    target_compile_options(TaskbarHider PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
    target_compile_options(TaskbarHider PRIVATE
        $<$<CONFIG:Release>:/O1 /Oy /GL /GR- /DNDEBUG /guard:cf>
        $<$<CONFIG:Debug>:/Od /Zi /RTC1>
    )
    target_link_options(TaskbarHider PRIVATE
        $<$<CONFIG:Release>:/LTCG /OPT:REF /OPT:ICF /DYNAMICBASE /NXCOMPAT /GUARD:CF>
    )

    # Embed app.manifest via the MSBuild manifest pipeline. Avoids the
    # CVT1100 duplicate-resource error you get when you also declare it in .rc.
    target_link_options(TaskbarHider PRIVATE
        "/MANIFEST:EMBED"
        "/MANIFESTINPUT:${CMAKE_CURRENT_SOURCE_DIR}/src/app.manifest"
    )

    # Static CRT -> zero runtime dependencies, no redistributable needed.
    set_property(TARGET TaskbarHider PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

target_link_libraries(TaskbarHider PRIVATE
    "${RUST_CORE_LIB}"
    user32 shell32 dwmapi gdi32 advapi32 ole32 ntdll
)

set_target_properties(TaskbarHider PROPERTIES
    OUTPUT_NAME "TaskbarHider"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE        "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG          "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/bin"
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     "${CMAKE_BINARY_DIR}/bin"
)
