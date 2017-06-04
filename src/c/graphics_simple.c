#include <pebble.h>
#include "graphics_simple.h"

void draw_battery_charged(GContext *ctx, GColor color_main, int shift, char *direction) {
  graphics_context_set_stroke_color(ctx, color_main);
  if (strcmp(direction, "horizontal") == 0) {
    graphics_draw_line(ctx, GPoint(0+shift, 0), GPoint(3+shift, 0));
    graphics_draw_line(ctx, GPoint(0+shift, 0), GPoint(0+shift, 7));
    graphics_draw_line(ctx, GPoint(0+shift, 7), GPoint(3+shift, 7));

    graphics_draw_line(ctx, GPoint(6+shift, 1), GPoint(8+shift, 1));
    graphics_draw_line(ctx, GPoint(5+shift, 2), GPoint(10+shift, 2));
    graphics_draw_line(ctx, GPoint(2+shift, 3), GPoint(8+shift, 3));
    graphics_draw_line(ctx, GPoint(2+shift, 4), GPoint(8+shift, 4));
    graphics_draw_line(ctx, GPoint(5+shift, 5), GPoint(10+shift, 5));
    graphics_draw_line(ctx, GPoint(6+shift, 6), GPoint(8+shift, 6));

    graphics_draw_line(ctx, GPoint(10+shift, 0), GPoint(13+shift, 0));
    graphics_draw_line(ctx, GPoint(13+shift, 0), GPoint(13+shift, 7));
    graphics_draw_line(ctx, GPoint(10+shift, 7), GPoint(13+shift, 7));
    graphics_draw_line(ctx, GPoint(14+shift, 2), GPoint(14+shift, 5));
  }
  else if (strcmp(direction, "vertical") == 0) {
    graphics_draw_line(ctx, GPoint(2, 0), GPoint(5, 0));
    graphics_draw_line(ctx, GPoint(7, 4), GPoint(7, 1));
    graphics_draw_line(ctx, GPoint(0, 1), GPoint(7, 1));
    graphics_draw_line(ctx, GPoint(0, 4), GPoint(0, 1));
    
    graphics_draw_line(ctx, GPoint(6, 8), GPoint(6, 6));
    graphics_draw_line(ctx, GPoint(5, 9), GPoint(5, 4));
    graphics_draw_line(ctx, GPoint(4, 12), GPoint(4, 6));
    graphics_draw_line(ctx, GPoint(3, 12), GPoint(3, 6));
    graphics_draw_line(ctx, GPoint(2, 9), GPoint(2, 4));
    graphics_draw_line(ctx, GPoint(1, 8), GPoint(1, 6));
    
    graphics_draw_line(ctx, GPoint(7, 14), GPoint(7, 11));
    graphics_draw_line(ctx, GPoint(0, 14), GPoint(7, 14));
    graphics_draw_line(ctx, GPoint(0, 14), GPoint(0, 11));
  }
}

void draw_battery_filled(GContext *ctx, GColor color_main, GColor color_background, int battery_level, int shift, char *direction) {
  graphics_context_set_stroke_color(ctx, color_main);
  if (strcmp(direction, "horizontal") == 0) {
    graphics_draw_line(ctx, GPoint(0+shift, 0), GPoint(13+shift, 0));
    graphics_draw_line(ctx, GPoint(0+shift, 7), GPoint(13+shift, 7));
    graphics_draw_line(ctx, GPoint(0+shift, 0), GPoint(0+shift, 7));
    graphics_draw_line(ctx, GPoint(13+shift, 0), GPoint(13+shift, 7));
    graphics_draw_line(ctx, GPoint(14+shift, 2), GPoint(14+shift, 5));
  }
  else if (strcmp(direction, "vertical") == 0) {
    graphics_draw_line(ctx, GPoint(2, 0), GPoint(5, 0));
    graphics_draw_line(ctx, GPoint(0, 1), GPoint(7, 1));
    graphics_draw_line(ctx, GPoint(0, 14), GPoint(7, 14));
    graphics_draw_line(ctx, GPoint(7, 14), GPoint(7, 1));
    graphics_draw_line(ctx, GPoint(0, 14), GPoint(0, 1));
  }

  graphics_context_set_stroke_color(ctx, color_background);
  graphics_context_set_fill_color(ctx, color_main);
  if (strcmp(direction, "horizontal") == 0) {
    graphics_fill_rect(ctx, GRect(2+shift, 2, (uint8_t)(battery_level / 10.0), 4), 0, GCornerNone);
  }
  else if (strcmp(direction, "vertical") == 0) {
    graphics_fill_rect(ctx, GRect(2, 3+10-(uint8_t)(battery_level / 10.0), 4, (uint8_t)(battery_level / 10.0)), 0, GCornerNone);
  }
}

void draw_battery_lighting(GContext *ctx, GColor color_main, char *direction) {
  graphics_context_set_stroke_color(ctx, color_main);
  if (strcmp(direction, "horizontal") == 0) {
    graphics_draw_line(ctx, GPoint(0, 3), GPoint(2, 3));
    graphics_draw_line(ctx, GPoint(2, 4), GPoint(4, 4));
    graphics_draw_line(ctx, GPoint(2, 1), GPoint(2, 6));
    graphics_draw_line(ctx, GPoint(3, 0), GPoint(3, 1));
    graphics_draw_line(ctx, GPoint(1, 6), GPoint(1, 7));
    graphics_draw_pixel(ctx, GPoint(1, 2));
    graphics_draw_pixel(ctx, GPoint(3, 5));
  }
  else if (strcmp(direction, "vertical") == 0) {
    graphics_draw_line(ctx, GPoint(3, 16), GPoint(3, 18));
    graphics_draw_line(ctx, GPoint(4, 18), GPoint(4, 20));
    graphics_draw_line(ctx, GPoint(1, 18), GPoint(6, 18));
    graphics_draw_line(ctx, GPoint(0, 19), GPoint(1, 19));
    graphics_draw_line(ctx, GPoint(6, 17), GPoint(7, 17));
    graphics_draw_pixel(ctx, GPoint(2, 17));
    graphics_draw_pixel(ctx, GPoint(5, 19));
  }
}

void draw_hide_battery_lighting(GContext *ctx, GColor color_background, char *direction) {
  graphics_context_set_fill_color(ctx, color_background);
  if (strcmp(direction, "horizontal") == 0) {
    graphics_fill_rect(ctx, GRect(0, 0, 5, 8), 0, GCornerNone);
  }
  else if (strcmp(direction, "vertical") == 0) {
    graphics_fill_rect(ctx, GRect(0, 16, 8, 5), 0, GCornerNone);
  }
}

void draw_bluetooth(GContext *ctx, GColor color_main) {
  graphics_context_set_stroke_color(ctx, color_main);
  graphics_draw_line(ctx, GPoint(1, 6), GPoint(2, 6));
  graphics_draw_line(ctx, GPoint(2, 7), GPoint(3, 7));
  graphics_draw_line(ctx, GPoint(3, 8), GPoint(8, 8));

  graphics_draw_line(ctx, GPoint(4, 9), GPoint(7, 9));
  graphics_draw_line(ctx, GPoint(4, 10), GPoint(7, 10));

  graphics_draw_line(ctx, GPoint(1, 13), GPoint(2, 13));
  graphics_draw_line(ctx, GPoint(2, 12), GPoint(3, 12));
  graphics_draw_line(ctx, GPoint(3, 11), GPoint(8, 11));

  graphics_draw_line(ctx, GPoint(5, 1), GPoint(5, 18));
  graphics_draw_line(ctx, GPoint(6, 2), GPoint(6, 17));

  graphics_draw_pixel(ctx, GPoint(7, 3));
  graphics_draw_line(ctx, GPoint(7, 4), GPoint(8, 4));
  graphics_draw_line(ctx, GPoint(8, 5), GPoint(9, 5));
  graphics_draw_line(ctx, GPoint(9, 6), GPoint(10, 6));
  graphics_draw_line(ctx, GPoint(8, 7), GPoint(9, 7));

  graphics_draw_pixel(ctx, GPoint(7, 16));
  graphics_draw_line(ctx, GPoint(7, 15), GPoint(8, 15));
  graphics_draw_line(ctx, GPoint(8, 14), GPoint(9, 14));
  graphics_draw_line(ctx, GPoint(9, 13), GPoint(10, 13));
  graphics_draw_line(ctx, GPoint(8, 12), GPoint(9, 12));
}

void draw_bluetooth_disconnected(GContext *ctx, GColor color_main, GColor color_background, GColor color_disconnect) {
  draw_bluetooth(ctx, color_main);
  
  graphics_context_set_fill_color(ctx, color_background);
  graphics_fill_rect(ctx, GRect(8, 11, 3, 5), 0, GCornerNone);

  graphics_context_set_stroke_color(ctx, color_disconnect);
  graphics_draw_line(ctx, GPoint(12, 9), GPoint(15, 9));
  graphics_draw_line(ctx, GPoint(11, 10), GPoint(16, 10));
  graphics_draw_line(ctx, GPoint(13, 11), GPoint(14, 11));

  graphics_draw_line(ctx, GPoint(12, 18), GPoint(15, 18));
  graphics_draw_line(ctx, GPoint(11, 17), GPoint(16, 17));
  graphics_draw_line(ctx, GPoint(13, 16), GPoint(14, 16));

  graphics_draw_line(ctx, GPoint(9, 12), GPoint(9, 15));
  graphics_draw_line(ctx, GPoint(10, 11), GPoint(10, 16));
  graphics_draw_line(ctx, GPoint(11, 13), GPoint(11, 14));

  graphics_draw_line(ctx, GPoint(18, 12), GPoint(18, 15));
  graphics_draw_line(ctx, GPoint(17, 11), GPoint(17, 16));
  graphics_draw_line(ctx, GPoint(16, 13), GPoint(16, 14));

  graphics_draw_pixel(ctx, GPoint(11, 11));
  graphics_draw_pixel(ctx, GPoint(16, 11));
  graphics_draw_pixel(ctx, GPoint(11, 16));
  graphics_draw_pixel(ctx, GPoint(16, 16));

  graphics_draw_line(ctx, GPoint(9, 11), GPoint(11, 9));
  graphics_draw_line(ctx, GPoint(16, 9), GPoint(18, 11));
  graphics_draw_line(ctx, GPoint(9, 16), GPoint(11, 18));
  graphics_draw_line(ctx, GPoint(16, 18), GPoint(18, 16));
}

void draw_hide_bluetooth(GContext *ctx, GColor color_background) {
  graphics_context_set_fill_color(ctx, color_background);
  graphics_fill_rect(ctx, GRect(0, 0, 20, 20), 0, GCornerNone);
}
