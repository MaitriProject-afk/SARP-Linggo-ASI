cmake_minimum_required(VERSION 3.15)
project(SARPLinggoASI LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)

# Configure output as DLL named SARPLinggo.asi
add_library(SARPLinggoASI SHARED
    src/main.cpp
    src/config.cpp
    src/translator.cpp
    src/chat_listener.cpp
    src/clipboard_listener.cpp
    src/licensing.cpp
    src/gui.cpp
    src/d3d9_hook.cpp
    src/input_hook.cpp
    include/config.h
    include/translator.h
    include/chat_listener.h
    include/clipboard_listener.h
    include/licensing.h
    include/gui.h
    include/d3d9_hook.h
    include/input_hook.h
    
    # MinHook
    src/vendor/minhook/src/buffer.c
    src/vendor/minhook/src/hook.c
    src/vendor/minhook/src/trampoline.c
    src/vendor/minhook/src/hde/hde32.c

    # ImGui
    src/vendor/imgui/imgui.cpp
    src/vendor/imgui/imgui_draw.cpp
    src/vendor/imgui/imgui_tables.cpp
    src/vendor/imgui/imgui_widgets.cpp
    src/vendor/imgui/backends/imgui_impl_win32.cpp
    src/vendor/imgui/backends/imgui_impl_dx9.cpp
)

set_target_properties(SARPLinggoASI PROPERTIES
    OUTPUT_NAME "SARPLinggo"
    SUFFIX ".asi"
)

target_include_directories(SARPLinggoASI PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/minhook/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/vendor/imgui/backends
)

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(SARPLinggoASI PRIVATE
        -s
        -fvisibility=hidden
        -fvisibility-inlines-hidden
        -ffunction-sections
        -fdata-sections
        -O3
    )
    target_link_options(SARPLinggoASI PRIVATE
        -s
        -Wl,--gc-sections
        -static
        -static-libgcc
        -static-libstdc++
    )
else()
    target_link_options(SARPLinggoASI PRIVATE
        -static
        -static-libgcc
        -static-libstdc++
    )
endif()

target_link_libraries(SARPLinggoASI PRIVATE
    winhttp
    user32
    gdi32
    shell32
    d3d9
    dwmapi
    bcrypt
    comdlg32
)
