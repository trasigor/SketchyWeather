#pragma once

#include "pebble.h"

void draw_battery_charged(GContext *ctx, GColor color_main, int shift, char *direction);
void draw_battery_filled(GContext *ctx, GColor color_main, GColor color_background, int battery_level, int shift, char *direction);
void draw_battery_lighting(GContext *ctx, GColor color_main, char *direction);
void draw_hide_battery_lighting(GContext *ctx, GColor color_background, char *direction);

void draw_bluetooth_disconnected(GContext *ctx, GColor color_main, GColor color_background, GColor color_disconnect);
void draw_bluetooth(GContext *ctx, GColor color_main);
void draw_hide_bluetooth(GContext *ctx, GColor color_background);