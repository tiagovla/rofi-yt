cmake_minimum_required(VERSION 3.14)
project(rofi-yt)

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

find_package(PkgConfig)
pkg_search_module(CAIRO REQUIRED cairo)
pkg_search_module(CURL REQUIRED libcurl)
pkg_search_module(GLIB2 REQUIRED glib-2.0)
pkg_get_variable(ROFI_PLUGINS_DIR rofi pluginsdir)

find_package(JPEG REQUIRED)
file(GLOB SOURCES "src/*.c")

add_library(yt SHARED ${SOURCES})
set_target_properties(yt PROPERTIES PREFIX "")
target_link_libraries(yt ${GLIB2_LIBRARIES} ${CAIRO_LIBRARIES}
                      ${CURL_LIBRARIES} jpeg)
target_include_directories(
  yt PRIVATE src ${GLIB2_INCLUDE_DIRS} ${CAIRO_INCLUDE_DIRS}
             ${CURL_INCLUDE_DIRS})

target_include_directories(yt PRIVATE vendor/cairo_jpeg)
target_sources(yt PRIVATE vendor/cairo_jpeg/cairo_jpg.c)

install(TARGETS yt DESTINATION ${ROFI_PLUGINS_DIR})
