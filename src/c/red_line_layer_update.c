#include <pebble.h>

#include "red_line_layer_update_callback.h"

#include "Red-Sox.h"


void red_line_layer_update_callback(Layer *RedLineLayer, GContext* ctx) { 
         
     #if PBL_COLOR
         graphics_context_set_fill_color(ctx, GColorRed);
     #else 
         //B&W
         graphics_context_set_fill_color(ctx, GColorWhite);
     #endif
       
     graphics_fill_rect(ctx, layer_get_bounds(RedLineLayer), 0, GCornerNone);
}