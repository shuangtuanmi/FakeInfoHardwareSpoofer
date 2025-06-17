cmake_minimum_required(VERSION 3.16)
project(FakeInfoHardwareSpoofer
    VERSION 1.0.0
    DESCRIPTION "Hardware Information Spoofing Tool"
    HOMEPAGE_URL "https://github.com/shuangtuanmi/FakeInfoHardwareSpoofer"
    LANGUAGES CXX
)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 平台检测
if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_LINUX TRUE)
elseif(APPLE)
    set(PLATFORM_MACOS TRUE)
endif()

# 编译选项
if(MSVC)
    add_compile_options(/W4 /utf-8)
    add_compile_definitions(
        UNICODE
        _UNICODE
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        _WIN32_WINNT=0x0A00  # Windows 10
    )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        add_compile_options(-g -O0)
    else()
        add_compile_options(-O3)
    endif()
endif()

# 构建选项
option(BUILD_TESTS "Build test programs" ON)
option(ENABLE_AUTO_DEPLOY "Enable automatic deployment of Qt libraries" OFF)

# 平台特定设置
if(PLATFORM_WINDOWS)
    # 禁用vcpkg自动依赖复制，我们手动处理
    set(VCPKG_APPLOCAL_DEPS OFF)
endif()

# 设置输出目录 - 分离Debug和Release
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin/Debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin/Release)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib/Debug)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib/Release)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lib/Debug)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lib/Release)

# 为了兼容性，也设置通用的输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# 多配置生成器的输出目录
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib)
endforeach()

# 查找依赖库
find_package(Qt5 REQUIRED COMPONENTS Core Widgets Network)

# 平台特定依赖
if(PLATFORM_WINDOWS)
    # 查找Detours (Windows only)
    find_package(unofficial-detours CONFIG QUIET)
    if(unofficial-detours_FOUND)
        message(STATUS "Found Detours via vcpkg")
        set(DETOURS_LIBRARIES unofficial::detours::detours)
        add_compile_definitions(DETOURS_AVAILABLE)
    else()
        # 备用查找方式
        find_path(DETOURS_INCLUDE_DIR detours.h PATHS
            "${VCPKG_ROOT}/installed/x64-windows/include/detours"
            "${VCPKG_ROOT}/installed/x64-windows/include"
            "C:/Detours/include"
            "C:/Program Files/Microsoft Detours/include"
            "${CMAKE_SOURCE_DIR}/third_party/detours/include"
        )
        find_library(DETOURS_LIBRARY detours PATHS
            "${VCPKG_ROOT}/installed/x64-windows/lib"
            "C:/Detours/lib.X64"
            "C:/Program Files/Microsoft Detours/lib.X64"
            "${CMAKE_SOURCE_DIR}/third_party/detours/lib.X64"
        )

        if(DETOURS_INCLUDE_DIR AND DETOURS_LIBRARY)
            set(DETOURS_LIBRARIES ${DETOURS_LIBRARY})
            add_compile_definitions(DETOURS_AVAILABLE)
            message(STATUS "Found Detours manually: ${DETOURS_LIBRARY}")
        else()
            message(WARNING "Detours not found. Hook functionality will be limited.")
        endif()
    endif()
endif()

# 主程序
add_executable(FakeInfoHardwareSpoofer
    main.cpp
    MainWindow.cpp
    MainWindow.h
    SystemInfo.cpp
    SystemInfo.h
    HardwareRandomizer.cpp
    HardwareRandomizer.h
)

# 启用Qt的MOC
set_target_properties(FakeInfoHardwareSpoofer PROPERTIES
    AUTOMOC ON
    WIN32_EXECUTABLE TRUE
    OUTPUT_NAME "FakeInfoHardwareSpoofer"
)

# 链接库
target_link_libraries(FakeInfoHardwareSpoofer
    Qt5::Core
    Qt5::Widgets
    Qt5::Network
)

# 平台特定链接库
if(PLATFORM_WINDOWS)
    target_link_libraries(FakeInfoHardwareSpoofer
        wbemuuid
        ole32
        oleaut32
    )
elseif(PLATFORM_LINUX)
    target_link_libraries(FakeInfoHardwareSpoofer
        pthread
        dl
    )
elseif(PLATFORM_MACOS)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(IOKIT_LIBRARY IOKit)
    target_link_libraries(FakeInfoHardwareSpoofer
        ${COREFOUNDATION_LIBRARY}
        ${IOKIT_LIBRARY}
    )
endif()

# Hook DLL (Windows only)
if(PLATFORM_WINDOWS)
    add_library(HookDLL SHARED
        HookDLL.cpp
    )

    set_target_properties(HookDLL PROPERTIES
        OUTPUT_NAME "HookDLL"
        PREFIX ""
    )

    # 链接Detours库
    if(DETOURS_LIBRARIES)
        if(unofficial-detours_FOUND)
            target_link_libraries(HookDLL
                ${DETOURS_LIBRARIES}
                wbemuuid
                ole32
                oleaut32
                advapi32
            )
            message(STATUS "HookDLL linked with vcpkg Detours")
        else()
            target_include_directories(HookDLL PRIVATE ${DETOURS_INCLUDE_DIR})
            target_link_libraries(HookDLL
                ${DETOURS_LIBRARIES}
                wbemuuid
                ole32
                oleaut32
                advapi32
            )
            message(STATUS "HookDLL linked with manual Detours")
        endif()
        message(STATUS "HookDLL will be built with Detours support")
    else()
        target_link_libraries(HookDLL
            wbemuuid
            ole32
            oleaut32
            advapi32
        )
        message(WARNING "HookDLL will be built without Detours support")
    endif()
endif()

# 安装规则
if(PLATFORM_WINDOWS)
    install(TARGETS FakeInfoHardwareSpoofer HookDLL
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION bin
        ARCHIVE DESTINATION lib
    )
else()
    install(TARGETS FakeInfoHardwareSpoofer
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
    )
endif()

# 安装文档和配置文件
install(FILES
    ${CMAKE_SOURCE_DIR}/README.md
    DESTINATION .
)

if(EXISTS ${CMAKE_SOURCE_DIR}/sample_config.json)
    install(FILES
        ${CMAKE_SOURCE_DIR}/sample_config.json
        DESTINATION bin
    )
endif()

# Qt部署（Windows）
if(WIN32)
    # 查找Qt5的windeployqt工具
    find_program(WINDEPLOYQT_EXECUTABLE windeployqt
        HINTS
            ${Qt5_DIR}/../../../bin
            ${Qt5_DIR}/../../bin
            ${VCPKG_ROOT}/installed/x64-windows/tools/qt5/bin
            ${VCPKG_ROOT}/installed/x64-windows/bin
            "C:/Qt/5.15.2/msvc2019_64/bin"
            $ENV{Qt5_DIR}/../../../bin
            $ENV{QTDIR}/bin
    )

    if(WINDEPLOYQT_EXECUTABLE)
        message(STATUS "Found windeployqt: ${WINDEPLOYQT_EXECUTABLE}")
        
        # 部署主程序的Qt库
        add_custom_command(TARGET FakeInfoHardwareSpoofer POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Deploying Qt5 libraries for main application..."
            COMMAND ${WINDEPLOYQT_EXECUTABLE} --no-angle --no-translations --no-system-d3d-compiler --no-opengl-sw $<TARGET_FILE:FakeInfoHardwareSpoofer>
            COMMENT "Deploying Qt5 libraries for main application"
        )
        
        # 为安装包准备Qt库
        install(CODE "
            execute_process(
                COMMAND ${WINDEPLOYQT_EXECUTABLE} --no-angle --no-translations --no-system-d3d-compiler --no-opengl-sw \${CMAKE_INSTALL_PREFIX}/bin/FakeInfoHardwareSpoofer.exe
            )
        ")
    else()
        message(WARNING "windeployqt not found. Qt libraries will not be automatically deployed.")
        message(STATUS "You may need to manually copy Qt libraries or ensure windeployqt is in PATH")
    endif()
endif()

# 测试程序 (可选)
if(BUILD_TESTS)
    if(PLATFORM_WINDOWS)
        # WMI测试程序
        add_executable(TestWMI
            test_wmi.cpp
        )
        target_link_libraries(TestWMI
            wbemuuid
            ole32
            oleaut32
        )
        set_target_properties(TestWMI PROPERTIES
            OUTPUT_NAME "TestWMI"
        )

        # 注册表测试程序
        add_executable(TestRegistry
            test_registry.cpp
        )
        target_link_libraries(TestRegistry
            advapi32
        )
        set_target_properties(TestRegistry PROPERTIES
            OUTPUT_NAME "TestRegistry"
        )
    endif()

    # SystemInfo测试程序 (跨平台)
    add_executable(TestSystemInfo
        test_systeminfo.cpp
        SystemInfo.cpp
        SystemInfo.h
    )
    target_link_libraries(TestSystemInfo
        Qt5::Core
        Qt5::Network
    )

    if(PLATFORM_WINDOWS)
        target_link_libraries(TestSystemInfo
            wbemuuid
            ole32
            oleaut32
        )
    endif()

    set_target_properties(TestSystemInfo PROPERTIES
        OUTPUT_NAME "TestSystemInfo"
    )
endif()

# CPack配置 (打包)
include(InstallRequiredSystemLibraries)
set(CPACK_PACKAGE_NAME "FakeInfoHardwareSpoofer")
set(CPACK_PACKAGE_VENDOR "FakeInfo Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Hardware Information Spoofing Tool")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "FakeInfoHardwareSpoofer")
set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

if(PLATFORM_WINDOWS)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_DISPLAY_NAME "FakeInfo Hardware Spoofer")
    set(CPACK_NSIS_PACKAGE_NAME "FakeInfo Hardware Spoofer")
    set(CPACK_NSIS_CONTACT "your-email@example.com")
    set(CPACK_NSIS_HELP_LINK "https://github.com/shuangtuanmi/FakeInfoHardwareSpoofer")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/shuangtuanmi/FakeInfoHardwareSpoofer")
    set(CPACK_NSIS_MODIFY_PATH ON)
elseif(PLATFORM_LINUX)
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "FakeInfo Team <your-email@example.com>")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libqt5core5a, libqt5widgets5, libqt5network5")
    set(CPACK_RPM_PACKAGE_LICENSE "MIT")
    set(CPACK_RPM_PACKAGE_GROUP "Applications/System")
elseif(PLATFORM_MACOS)
    set(CPACK_GENERATOR "DragNDrop;TGZ")
    set(CPACK_DMG_VOLUME_NAME "FakeInfo Hardware Spoofer")
    set(CPACK_DMG_FORMAT "UDZO")
endif()

include(CPack)

# 开发工具
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
