#include <stdio.h>
#include <SDL2/SDL.h>
#include <cairo.h>
#include <string>
#include "helper.h"


int main(int argc, char **argv)
{
    State *state;
    initialize(&state, 1000, 1000);

    doTheDrawing(state);

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
                    doTheDrawing(state);
                }
                else if(ke.keysym.scancode == 45) // - sign in number row
                {
                    zoomOutTransform(state);
                    doTheDrawing(state);
                }
                else if(ke.keysym.scancode == 46) // + sign in number row
                {
                    zoomInTransform(state);
                    doTheDrawing(state);
                }
                else if(ke.keysym.scancode == 11) // left vim (h)
                {
                    moveTransform(state, -1, 0);
                    doTheDrawing(state);
                }
                else if(ke.keysym.scancode == 13) // down vim (j)
                {
                    moveTransform(state, 0, 1);
                    doTheDrawing(state);
                }
                else if(ke.keysym.scancode == 14) // up vim (k)
                {
                    moveTransform(state, 0, -1);
                    doTheDrawing(state);
                }
                else if(ke.keysym.scancode == 15) // right vim (l)
                {
                    moveTransform(state, 1, 0);
                    doTheDrawing(state);
                }
                else if(ke.keysym.scancode == 30) // 1 number key
                {
                    setRenderer(state, 0);
                    doTheDrawing(state);
                }
                else if(ke.keysym.scancode == 31) // 2 number key
                {
                    setRenderer(state, 1);
                    doTheDrawing(state);
                }
                else if(ke.keysym.scancode == 32) // 3 number key
                {
                    setRenderer(state, 2);
                    doTheDrawing(state);
                }
                else if(ke.keysym.scancode == 33) // 4 number key
                {
                    setRenderer(state, 3);
                    doTheDrawing(state);
                }
                else if(ke.keysym.scancode == 79) // right arrow
                {
                    nextSVG(state);
                    loadCurrentSVG(state);
                    doTheDrawing(state);
                }
                else if(ke.keysym.scancode == 80) // left arrow
                {
                    prevSVG(state);
                    loadCurrentSVG(state);
                    doTheDrawing(state);
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
