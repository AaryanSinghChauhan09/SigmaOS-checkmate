cmake_minimum_required(VERSION 3.21)
project(checkmate LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Network Qml Quick QuickControls2 DBus Test Quick3D Concurrent)

# Everything but main.cpp, shared by the app and the tests, so each source
# compiles once and a new file in src/ needs no line here.
file(GLOB checkmate_sources CONFIGURE_DEPENDS src/*.cpp src/*.h)
list(REMOVE_ITEM checkmate_sources ${CMAKE_SOURCE_DIR}/src/main.cpp)
add_library(checkmate_core STATIC ${checkmate_sources})
target_include_directories(checkmate_core PUBLIC src)
target_link_libraries(checkmate_core PUBLIC
  Qt6::Core Qt6::Gui Qt6::Network Qt6::Qml Qt6::Quick Qt6::QuickControls2 Qt6::DBus Qt6::Quick3D Qt6::Concurrent)
target_compile_options(checkmate_core PUBLIC -Wall -Wextra)

add_executable(checkmate src/main.cpp src/resources.qrc)
target_link_libraries(checkmate PRIVATE checkmate_core)

# The tests load the real Main.qml, so they carry the resources too.
enable_testing()
add_executable(checkmate_tests tests/checkmate_tests.cpp src/resources.qrc)
target_link_libraries(checkmate_tests PRIVATE checkmate_core Qt6::Test)
add_test(NAME checkmate_tests COMMAND checkmate_tests)
set_tests_properties(checkmate_tests PROPERTIES ENVIRONMENT QT_QPA_PLATFORM=offscreen)

# `cmake --install build --prefix ~/.local` puts the app in the launcher
# before it is packaged; a package installs the same files under /usr.
include(GNUInstallDirs)
install(TARGETS checkmate RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES share/checkmate.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES share/checkmate.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/checkmate)
