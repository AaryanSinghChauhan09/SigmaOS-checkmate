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

# The version comes from the release, never from the code: a tag's tarball
# unpacks as checkmate-<version>, and a checkout asks git.
get_filename_component(source_dir_name "${CMAKE_CURRENT_SOURCE_DIR}" NAME)
if(source_dir_name MATCHES "^checkmate-([0-9][0-9.]*)$")
  set(CHECKMATE_VERSION "${CMAKE_MATCH_1}")
else()
  execute_process(COMMAND git describe --tags --always --dirty
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    OUTPUT_VARIABLE CHECKMATE_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
  string(REGEX REPLACE "^v" "" CHECKMATE_VERSION "${CHECKMATE_VERSION}")
  if(NOT CHECKMATE_VERSION)
    set(CHECKMATE_VERSION dev)
  endif()
endif()

add_executable(checkmate src/main.cpp src/resources.qrc)
target_link_libraries(checkmate PRIVATE checkmate_core)
target_compile_definitions(checkmate PRIVATE CHECKMATE_VERSION="${CHECKMATE_VERSION}")

# The tests load the real Main.qml, so they carry the resources too.
enable_testing()
add_executable(checkmate_tests tests/checkmate_tests.cpp src/resources.qrc)
target_link_libraries(checkmate_tests PRIVATE checkmate_core Qt6::Test)
add_dependencies(checkmate_tests checkmate)
find_program(XVFB_RUN xvfb-run REQUIRED)
add_test(NAME checkmate_tests COMMAND ${XVFB_RUN} -a $<TARGET_FILE:checkmate_tests>)
set_tests_properties(checkmate_tests PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen;QT_QUICK_BACKEND=rhi;QSG_RHI_BACKEND=opengl;LIBGL_ALWAYS_SOFTWARE=1")

# --version answers without a display. A checkout without tags reports git's commit or "dev" rather than a number.
add_test(NAME checkmate_version_without_display
  COMMAND ${CMAKE_COMMAND} -E env --unset=QT_QPA_PLATFORM --unset=WAYLAND_DISPLAY --unset=DISPLAY
          $<TARGET_FILE:checkmate> --version)
set_tests_properties(checkmate_version_without_display PROPERTIES PASS_REGULAR_EXPRESSION "^Checkmate [0-9a-z]")

# `cmake --install build --prefix ~/.local` puts the app in the launcher
# before it is packaged; a package installs the same files under /usr.
include(GNUInstallDirs)
install(TARGETS checkmate RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES share/checkmate.desktop DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES share/checkmate.svg DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps)
install(FILES LICENSE DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/checkmate)
