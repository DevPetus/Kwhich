cmake_minimum_required(VERSION 3.16)
project(kwhich LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(DBUS REQUIRED dbus-1)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(kwhich-cli
    src/main.cpp
    src/actions/file_manager.cpp
    src/capture/window_picker.cpp
    src/inspect/dual_install.cpp
    src/inspect/file_info.cpp
    src/inspect/flatpak_path.cpp
    src/inspect/proc_info.cpp
    src/inspect/process_tree.cpp
    src/inspect/source_rules.cpp
    src/inspect/wine_path.cpp
    src/ui/inspector_panel.cpp
    src/util/shell.cpp
)

target_include_directories(kwhich-cli PRIVATE src ${DBUS_INCLUDE_DIRS})
target_link_libraries(kwhich-cli PRIVATE ${DBUS_LIBRARIES} Qt6::Widgets)

include(GNUInstallDirs)
install(TARGETS kwhich-cli RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES data/kwhich.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
