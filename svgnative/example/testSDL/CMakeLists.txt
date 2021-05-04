add_executable(testSDL testSDL.cpp helper.h helper.cpp)
target_link_libraries(testSDL PUBLIC SVGNativeViewerLib)


pkg_check_modules(SDL REQUIRED sdl2)
target_link_libraries(testSDL PUBLIC "${SDL_LINK_LIBRARIES}")
target_include_directories(testSDL SYSTEM PUBLIC "${SDL_INCLUDE_DIRS}")

pkg_check_modules(GDK_PIXBUF REQUIRED gdk-pixbuf-2.0)
target_link_libraries(testSDL PUBLIC "${GDK_PIXBUF_LINK_LIBRARIES}")
target_include_directories(testSDL SYSTEM PUBLIC "${GDK_PIXBUF_INCLUDE_DIRS}")


target_include_directories(testSDL PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/stylesheet/include")
target_include_directories(testSDL PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/skia/include/config")
target_include_directories(testSDL PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/skia/include/core")
target_include_directories(testSDL PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/skia/include/effects")
target_include_directories(testSDL PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/skia/include/encode")
target_include_directories(testSDL PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../../third_party/skia/include/gpu")
target_include_directories(testSDL PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/../../ports/skia")

target_link_libraries(testSDL PUBLIC SVGNativeViewerLib "${SKIA_LIBRARY_PATH}")
if(APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -framework Cocoa")
endif()
