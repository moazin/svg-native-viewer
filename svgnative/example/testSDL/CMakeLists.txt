add_executable(testSDL testSDL.cpp helper.h helper.cpp)
target_link_libraries(testSDL PUBLIC SVGNativeViewerLib)


pkg_check_modules(SDL REQUIRED sdl2)
target_link_libraries(testSDL PUBLIC "${SDL_LINK_LIBRARIES}")
target_include_directories(testSDL SYSTEM PUBLIC "${SDL_INCLUDE_DIRS}")
