#include <stdio.h>
#include <SDL2/SDL.h>
#include <cairo.h>
#include <string>
#include "helper.h"


int main(void)
{
    State *state;
    initialize(&state, 1000, 1000);

    //drawing(state);

    SDL_Event event;
    while(1){
        if (SDL_PollEvent(&event)){
            if (event.type == SDL_KEYDOWN)
            {
                SDL_KeyboardEvent ke = event.key;
                if(ke.keysym.scancode == 20)
                  break;
                else if(ke.keysym.scancode == 39) // 0 key in number row
                {
                    resetTransform(state);
                    drawing(state);
                }
                else if(ke.keysym.scancode == 45) // - sign in number row
                {
                    zoomOutTransform(state);
                    drawing(state);
                }
                else if(ke.keysym.scancode == 46) // + sign in number row
                {
                    zoomInTransform(state);
                    drawing(state);
                }
                else if(ke.keysym.scancode == 11) // left vim (h)
                {
                    moveTransform(state, -1, 0);
                    drawing(state);
                }
                else if(ke.keysym.scancode == 13) // down vim (j)
                {
                    moveTransform(state, 0, 1);
                    drawing(state);
                }
                else if(ke.keysym.scancode == 14) // up vim (k)
                {
                    moveTransform(state, 0, -1);
                    drawing(state);
                }
                else if(ke.keysym.scancode == 15) // right vim (l)
                {
                    moveTransform(state, 1, 0);
                    drawing(state);
                }
                else if(ke.keysym.scancode == 30) // 1 number key
                {
                    drawing(state);
                    displayBuffer(state, 0);
                }
                else if(ke.keysym.scancode == 31) // 2 number key
                {
                    drawing(state);
                    displayBuffer(state, 1);
                }
                else if(ke.keysym.scancode == 32) // 3 number key
                {
                    drawing(state);
                    displayBuffer(state, 2);
                }
                else
                {
                    printf("Key pressed: %d\n", ke.keysym.scancode);
                }
            }
        }
    }

    destroy(state);
}
