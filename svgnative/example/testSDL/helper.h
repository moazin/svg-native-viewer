#include <SDL2/SDL.h>
#include <cairo.h>
#include <string>

typedef struct _State State;

/**
 * Create a new state object with a canvas of width and height. In reality, the canvas
 * will have extra 300 pixels on the right for displaying information. This will also
 * initialize the cairo surface that's needed for drawing.
 */
int initialize(State **state, int width, int height);

/**
 * Destroy the state object, freeing all associated memory.
 */
void destroy(State *state);

/**
 * Clear the width x height region of the canvas, filling it with pure white.
 */
void clearCanvas(State *state);

/**
 * Draw a rectangle with the given parameters. Note that the drawing will take place
 * with any transforms taken into account. Which means, the drawing will be zoomed in
 * if you've zoomed in the canvas.
 */
void drawRectangle(State *state, float x0, float y0, float x1, float y1, double r, double g, double b, int line_width);

/**
 * TODO:
 */
void drawing(State *state);

/**
 * Change the current ViewBox so that it reduces on its spot from all sides. Has the
 * effect of zooming in.
 */
void zoomInTransform(State *state);

/**
 * Change the current ViewBox so that it resets to the default. Resetting basically.
 */
void resetTransform(State *state);

/**
 * Change the current ViewBox so that it expands on its spot from all sides. Has the
 * effect of zooming out.
 */
void zoomOutTransform(State *state);

/**
 * Set a cairo transform such that the viewBox is fit on our window. Basically the
 * zoom calls above need to be followed by this to "set" the transform.
 */
void setTransform(State *state);


/**
 * Move the current viewbox by certain amount.
 * `x` and `y` merely dictate the direction in which to change. Positive `1` means right or up
 * and Negative `1` means down or left.
 */
void moveTransform(State *state, int x, int y);

void displayBuffer(State *state, int index);
