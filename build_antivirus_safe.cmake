# build_antivirus_safe.cmake - 避免杀毒软件误报的构建配置

cmake_minimum_required(VERSION 3.16)
project(SelfEvolveAI VERSION 1.0.0)

# 设置C标准
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# 避免误报的关键设置
if(WIN32)
    # 1. 使用动态链接运行时库（避免静态链接CRT导致的误报）
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL")
    
    # 2. 添加版本信息和元数据
    set(CMAKE_RC_COMPILER_INIT windres)
    enable_language(RC)
    
    # 3. 安全编译选项
    if(MSVC)
        # 启用安全功能
        add_compile_options(
            /GS          # 缓冲区安全检查
            /sdl         # 安全开发生命周期检查
            /guard:cf    # 控制流保护
            /Qspectre    # Spectre缓解
        )
        
        # 安全链接选项
        add_link_options(
            /DYNAMICBASE    # 地址空间布局随机化
            /NXCOMPAT       # 数据执行保护
            /SAFESEH        # 安全异常处理
            /GUARD:CF       # 控制流保护
        )
    endif()
    
    # 4. 优化设置（Release模式）
    set(CMAKE_C_FLAGS_RELEASE "/O2 /DNDEBUG")
    
elseif(UNIX)
    # Linux/macOS安全编译选项
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

# 创建版本资源文件
if(WIN32)
    configure_file(
        ${CMAKE_SOURCE_DIR}/resources/version.rc.in
        ${CMAKE_BINARY_DIR}/version.rc
        @ONLY
    )
endif()

# 主要可执行文件
add_executable(loader
    src/loader/main.c
    src/loader/platform_detection.c
    src/loader/module_loader.c
    src/loader/command_line.c
    src/loader/error_handling.c
)

# 添加版本资源（Windows）
if(WIN32)
    target_sources(loader PRIVATE ${CMAKE_BINARY_DIR}/version.rc)
endif()

# 包含目录
target_include_directories(loader PRIVATE
    src/loader/include
    src/core/include
)

# 链接库
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

# 设置输出目录
set_target_properties(loader PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    OUTPUT_NAME "loader"
)

# VM核心模块
add_library(vm_core SHARED
    src/core/vm/vm_enhanced.c
    src/core/vm/jit_compiler.c
    src/core/vm/memory_manager.c
    src/core/vm/instruction_set.c
)

target_include_directories(vm_core PRIVATE
    src/core/include
)

# 设置模块输出名称
set_target_properties(vm_core PROPERTIES
    OUTPUT_NAME "vm_${CMAKE_SYSTEM_PROCESSOR}_${CMAKE_SIZEOF_VOID_P}0"
    PREFIX ""
    SUFFIX ".native"
)

# 安装规则
install(TARGETS loader vm_core
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)

# 代码签名（如果有证书）
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

# 创建安装包
set(CPACK_PACKAGE_NAME "SelfEvolveAI")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Self-Evolve AI System")
set(CPACK_PACKAGE_VENDOR "Your Company")
set(CPACK_PACKAGE_CONTACT "your-email@example.com")

if(WIN32)
    set(CPACK_GENERATOR "NSIS")
    set(CPACK_NSIS_DISPLAY_NAME "Self-Evolve AI")
    set(CPACK_NSIS_PACKAGE_NAME "SelfEvolveAI")
    set(CPACK_NSIS_CONTACT "your-email@example.com")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/your-repo")
endif()

include(CPack)
