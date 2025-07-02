# CMakeLists.txt - Safe build configuration to avoid antivirus false positives

cmake_minimum_required(VERSION 3.16)
project(SelfEvolveAI VERSION 1.0.0)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Anti-virus safe settings
if(WIN32)
    # 1. Use dynamic runtime library (avoid static linking CRT that triggers false positives)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    
    # 2. Add version info and metadata
    set(CMAKE_RC_COMPILER_INIT windres)
    enable_language(RC)
    
    # 3. Security compile options
    if(MSVC)
        # Enable security features
        add_compile_options(
            /GS          # Buffer security check
            /sdl         # Security Development Lifecycle checks
            /guard:cf    # Control Flow Guard
        )
        
        # Security link options
        add_link_options(
            /DYNAMICBASE    # Address Space Layout Randomization
            /NXCOMPAT       # Data Execution Prevention
            /SAFESEH        # Safe Exception Handling
            /GUARD:CF       # Control Flow Guard
        )
    endif()
    
    # 4. Optimization settings (Release mode)
    set(CMAKE_C_FLAGS_RELEASE "/O2 /DNDEBUG")
    
elseif(UNIX)
    # Linux/macOS security compile options
    add_compile_options(
        -O2
        -fstack-protector-strong
        -D_FORTIFY_SOURCE=2
        -fPIC
    )
    
    add_link_options(
        -Wl,-z,relro
        -Wl,-z,now
        -Wl,-z,noexecstack
    )
endif()

# Create version resource file
if(WIN32)
    configure_file(
        ${CMAKE_SOURCE_DIR}/resources/version.rc.in
        ${CMAKE_BINARY_DIR}/version.rc
        @ONLY
    )
endif()

# Check if we have existing source files
if(EXISTS "${CMAKE_SOURCE_DIR}/src/loader/main.c")
    # Main executable with existing source
    add_executable(loader
        src/loader/main.c
        src/loader/platform_detection.c
        src/loader/module_loader.c
        src/loader/command_line.c
        src/loader/error_handling.c
    )
    
    # Include directories
    target_include_directories(loader PRIVATE
        src/loader/include
        src/core/include
    )
else()
    # Create a simple test executable if source doesn't exist
    file(WRITE ${CMAKE_BINARY_DIR}/test_main.c
        "#include <stdio.h>\n"
        "#include <windows.h>\n"
        "int main() {\n"
        "    printf(\"Self-Evolve AI Test Program v1.0.0\\n\");\n"
        "    printf(\"This is a test to avoid antivirus false positives.\\n\");\n"
        "    MessageBox(NULL, L\"Test successful!\", L\"Self-Evolve AI\", MB_OK);\n"
        "    return 0;\n"
        "}\n"
    )
    
    add_executable(loader ${CMAKE_BINARY_DIR}/test_main.c)
endif()

# Add version resource (Windows)
if(WIN32)
    target_sources(loader PRIVATE ${CMAKE_BINARY_DIR}/version.rc)
endif()

# Link libraries
if(WIN32)
    target_link_libraries(loader
        kernel32
        user32
        advapi32
    )
elseif(UNIX)
    target_link_libraries(loader
        dl
        pthread
    )
endif()

# Set output properties
set_target_properties(loader PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    OUTPUT_NAME "loader"
)

# Add manifest file for Windows
if(WIN32 AND EXISTS "${CMAKE_SOURCE_DIR}/resources/manifest.xml")
    set_target_properties(loader PROPERTIES
        LINK_FLAGS "/MANIFEST:EMBED /MANIFESTINPUT:${CMAKE_SOURCE_DIR}/resources/manifest.xml"
    )
endif()

# Install rules
install(TARGETS loader
    RUNTIME DESTINATION bin
)

# Code signing (if certificate is available)
if(WIN32 AND DEFINED CODESIGN_CERTIFICATE)
    find_program(SIGNTOOL_EXECUTABLE signtool)
    if(SIGNTOOL_EXECUTABLE)
        add_custom_command(TARGET loader POST_BUILD
            COMMAND ${SIGNTOOL_EXECUTABLE} sign 
                /f "${CODESIGN_CERTIFICATE}"
                /p "${CODESIGN_PASSWORD}"
                /t "http://timestamp.digicert.com"
                /d "Self-Evolve AI System"
                /du "https://github.com/your-repo"
                $<TARGET_FILE:loader>
            COMMENT "Signing executable with code certificate"
        )
    endif()
endif()

# Create installer package
set(CPACK_PACKAGE_NAME "SelfEvolveAI")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Self-Evolve AI System")
set(CPACK_PACKAGE_VENDOR "Self-Evolve AI Project")
set(CPACK_PACKAGE_CONTACT "support@self-evolve-ai.com")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "Self-Evolve AI")
    set(CPACK_NSIS_PACKAGE_NAME "SelfEvolveAI")
    set(CPACK_NSIS_CONTACT "support@self-evolve-ai.com")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/self-evolve-ai")
endif()

include(CPack)

# Print configuration summary
message(STATUS "=== Self-Evolve AI Build Configuration ===")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Compiler: ${CMAKE_C_COMPILER_ID}")
if(WIN32)
    message(STATUS "Runtime Library: ${CMAKE_MSVC_RUNTIME_LIBRARY}")
endif()
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "==========================================")
