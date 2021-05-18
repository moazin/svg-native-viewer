#include <stdio.h>
#include <SDL2/SDL.h>
#include <fstream>
#include <cairo.h>
#include <cmath>
#include <string>
#include <svgnative/SNVCWrapper.h>


int main(int argc, char **argv)
{
    std::string svgInput{};
    std::ifstream input(argv[1]);
    if (!input)
    {
        fprintf(stderr, "Couldn't open the file");
        exit(EXIT_FAILURE);
    }
    for (std::string line; std::getline(input, line);)
        svgInput.append(line);
    input.close();

    snv_t *context;
    snv_create(svgInput.c_str(), &context);
    snv_rect viewBox;
    bool hasViewBox;
    snv_get_viewbox(context, &hasViewBox, &viewBox);
    if (hasViewBox)
      printf("%f %f %f %f\n", viewBox.x0, viewBox.y0, viewBox.x1, viewBox.y1);
    snv_rect bbox;
    snv_get_bbox(context, &bbox);
    int width = ceil(bbox.x1) - floor(bbox.x0) + 1;
    int height = ceil(bbox.y1) - floor(bbox.y0) + 1;
    snv_transform_scale(context, 0.8, 0.8);
    snv_transform_translate(context, floor(-bbox.x0), floor(-bbox.y0));
    snv_get_bbox(context, &bbox);
    width = ceil(bbox.x1) - floor(bbox.x0) + 1;
    height = ceil(bbox.y1) - floor(bbox.y0) + 1;

    SDL_Window *window;
    SDL_Surface *sdl_surface;
    window = SDL_CreateWindow("SNV Demo Tool",
                                     0,
                                     0,
                                     width,
                                     height,
                                     SDL_WINDOW_ALLOW_HIGHDPI);
    if (window == NULL)
    {
        fprintf(stderr, "SDL window failed to initialize: %s\n", SDL_GetError());
        return 1;
    }

    sdl_surface = SDL_GetWindowSurface(window);

    snv_render(context, (unsigned char*)sdl_surface->pixels, sdl_surface->w, sdl_surface->h, sdl_surface->pitch);
    SDL_UpdateWindowSurface(window);

    SDL_Event event;
    while(1){
        if (SDL_PollEvent(&event)){
            if (event.type == SDL_KEYDOWN)
            {
                SDL_KeyboardEvent ke = event.key;
                if(ke.keysym.scancode == 20)
                  break;
                else
                {
                    printf("Key pressed: %d\n", ke.keysym.scancode);
                }
            }
        }
    }
  snv_destroy(context);
}
