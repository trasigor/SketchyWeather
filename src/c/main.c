#include <pebble.h>
  
#include "pebble_process_info.h"
#include "bitmap_weather_ids.h"
#include "graphics_simple.h"

#define DB_SW_TEMPERATURE 0
#define DB_SW_CONDITIONS 1
#define DB_SW_BLUETOOTH 2
#define DB_SW_BATTERY 3
#define DB_SW_HOURLY_BEEP 4
#define DB_SW_HOUR_LEAD_ZERO 5
#define DB_SW_SCREEN_COLOR 6
#define DB_SW_DND 7
#define DB_SW_DND_START 8
#define DB_SW_DND_END 9
#define DB_SW_FORECAST_DURATION 10
#define DB_SW_TOMORROW 11
#define DB_SW_AFTER_TOMORROW 12
#define DB_SW_HUMIDITY 13
#define DB_SW_TOMORROW_CONDITIONS 14
#define DB_SW_AFTER_TOMORROW_CONDITIONS 15
#define DB_SW_MAIN_SCREEN_INFO 16
#define DB_SW_FORECAST_ON_ONE_SHAKE 17

extern const PebbleProcessInfo __pbl_app_info;
  
static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_bottom_main_info_layer;

static Layer *s_bottom_additional_layer;
static char s_day_of_week_buffer[32];
static char s_date_buffer[32];
static void draw_bottom_additional_info_layer(Layer *layer, GContext *ctx);
static bool day_of_week_shown;
static void show_hide_forecast();
static void hide_forecast_callback();
static AppTimer *s_forecast_timer;
static int s_forecast_show_duration; // in sec; 0 - don't show

static GFont s_time_font;
static GFont s_weather_date_font;

static BitmapLayer *s_weather_image_layer;
static BitmapLayer *s_weather_small_image_left_layer;
static BitmapLayer *s_weather_small_image_right_layer;
static GBitmap *s_weather_image_bitmap = NULL;
static GBitmap *s_weather_small_image_left_bitmap = NULL;
static GBitmap *s_weather_small_image_right_bitmap = NULL;

static Layer *s_forecast_layer;
static void draw_forecast_layer(Layer *layer, GContext *ctx);
static char s_tomorrow_buffer[32];
static char s_after_tomorrow_buffer[32];
static int s_tomorrow_condition_id;
static int s_after_tomorrow_condition_id;

static void update_bottom_main_info();
  
static Layer *s_bluetooth_layer;
static bool last_bluetooth_connection;

static bool weather_out_of_date;

static char s_config_bluetooth[32];
static char s_config_battery[32];
static char s_config_screen_color[32];
static char s_config_hourly_beep[32];
static char s_config_dnd[32];
static uint8_t s_config_dnd_start;
static uint8_t s_config_dnd_end;
static char s_config_main_screen_info[32];
static char s_temperature_buffer[8];
static int s_humidity_buffer;
static bool s_config_forecast_on_one_shake;

static uint8_t battery_level;
static bool battery_plugged;
static bool battery_charging;
static Layer *s_battery_layer;
static void battery_handler();
static void battery_layer_update_callback(Layer *layer, GContext *ctx);

static int16_t get_current_version();

static void update_weather();
static void draw_weather_image(int weather_id);
static void bluetooth_handler(bool connected);
static void bluetooth_layer_update(Layer *layer, GContext *ctx);

static void set_container_image(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id);

static bool is_inverted_color();
static GColor get_main_color();
static GColor get_background_color();
static void update_layers_color();

static Layer *s_background_layer;
static void draw_background(Layer *layer, GContext *ctx);

int last_beeped = 0;
static void make_beep();

static bool is_silence(int32_t hour);

static bool hour_lead_zero;
static void tap_handler(AccelAxisType axis, int32_t direction);

static int SCREEN_HEIGHT = 168;
static int SCREEN_WIDTH = 144;
static int LAYER_VERTICAL_INDENT = 0;

static void persist_read_and_set();
static char* merge_strings(char *str_first, char *str_second);

time_t last_shake = 0;

PropertyAnimation *prop_anim_move_day_of_week = NULL;
PropertyAnimation *prop_anim_move_forecast = NULL;

#if defined(PBL_ROUND)
int forecast_layer_indent = 4;
#else
int forecast_layer_indent = 0;
#endif

static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char bufferTime[] = "00:00";
  
  // Write the current hours and minutes into the buffer
  if(clock_is_24h_style() == true) {
    //Use 24 hour format
    if (hour_lead_zero) {
      strftime(bufferTime, sizeof(bufferTime), "%H:%M", tick_time);
    }
    else {
      strftime(bufferTime, sizeof(bufferTime), "%k:%M", tick_time);
    }
  } else {
    //Use 12 hour format
    if (hour_lead_zero) {
      strftime(bufferTime, sizeof(bufferTime), "%I:%M", tick_time);
    }
    else {
      strftime(bufferTime, sizeof(bufferTime), "%l:%M", tick_time);
    }
  }
  
  if (strncmp(bufferTime, " ", 1) == 0) {
    clock_copy_time_string(bufferTime, sizeof("0:00"));
  }
  // Display this time on the TextLayer
  text_layer_set_text(s_time_layer, bufferTime);
  
  if (0 == strlen(s_date_buffer) || (tick_time->tm_min==0 && tick_time->tm_sec==0)) {
    static char bufferDayNum[] = "01";
    strftime(bufferDayNum, sizeof(bufferDayNum), "%e", tick_time);
    
    static char bufferMonth[32];
    strftime(bufferMonth, sizeof(bufferMonth), "%b", tick_time);
    
    if (strncmp(bufferDayNum, " ", 1) == 0) {
      snprintf(s_date_buffer, sizeof(s_date_buffer), "%s%s", bufferMonth, bufferDayNum);
    }
    else {
      snprintf(s_date_buffer, sizeof(s_date_buffer), "%s %s", bufferMonth, bufferDayNum);
    }
  }
  
  if (0 == strlen(s_day_of_week_buffer) || (tick_time->tm_min==0 && tick_time->tm_sec==0)) {
    strftime(s_day_of_week_buffer, sizeof(s_day_of_week_buffer), "%a", tick_time);
    layer_mark_dirty(s_bottom_additional_layer);
  }
  
  if (last_beeped!=tick_time->tm_hour
      && !is_silence(tick_time->tm_hour)
      && strcmp(s_config_hourly_beep, "beep")==0
//       && tick_time->tm_min==0
      && tick_time->tm_sec==0) {
    last_beeped = tick_time->tm_hour;
    make_beep();
  }
}

static void main_window_load(Window *window) {
  // Read saved data in persistent store
  persist_read_and_set();
  APP_LOG(APP_LOG_LEVEL_INFO, "Sketchy Weather version %d.%d",
          (int)(get_current_version()/10), get_current_version()%10);
  
  int weather_image_vertical_correction = 0;
  int editional_info_vertical_correction = 0;
  GRect window_bounds = layer_get_bounds(window_get_root_layer(window));
  
  SCREEN_HEIGHT = window_bounds.size.h;
  SCREEN_WIDTH = window_bounds.size.w;
  if (SCREEN_HEIGHT >= 228) {
    LAYER_VERTICAL_INDENT = 25;
  }
  
  // Create GFont
  s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SKETCHY_59));

  // Create second custom font, apply it and add to Window
  s_weather_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_SKETCHY_25));
  
  // Create bottom background Layer
  s_background_layer = layer_create(window_bounds);
  layer_set_update_proc(s_background_layer, draw_background);
  layer_add_child(window_get_root_layer(window), s_background_layer);
  
  //Create GBitmap, then set to created BitmapLayer
  int weather_image_height = 90;
  #ifdef PBL_ROUND
    weather_image_vertical_correction = 5;
  #endif
  if (SCREEN_HEIGHT >= 228) {
    weather_image_vertical_correction = 5;
    weather_image_height = 125;
  }
  s_weather_image_layer = bitmap_layer_create(GRect(0, 0+weather_image_vertical_correction, window_bounds.size.w, weather_image_height));
  bitmap_layer_set_bitmap(s_weather_image_layer, s_weather_image_bitmap);
  layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_weather_image_layer));
  if (persist_exists(DB_SW_CONDITIONS)) {
      draw_weather_image(persist_read_int(DB_SW_CONDITIONS));
  }
  
  // Create time TextLayer
  s_time_layer = text_layer_create(GRect(0, weather_image_height-25+weather_image_vertical_correction+((int)(LAYER_VERTICAL_INDENT/5)), window_bounds.size.w, 85));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, get_main_color());
  text_layer_set_font(s_time_layer, s_time_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_time_layer));
  
  // Create bottom main layer TextLayer
  s_bottom_main_info_layer = text_layer_create(GRect(0, 132+LAYER_VERTICAL_INDENT*2, window_bounds.size.w, 36));
  text_layer_set_background_color(s_bottom_main_info_layer, GColorClear);
  text_layer_set_text_color(s_bottom_main_info_layer, get_main_color());
  text_layer_set_text_alignment(s_bottom_main_info_layer, GTextAlignmentCenter);
  text_layer_set_font(s_bottom_main_info_layer, s_weather_date_font);
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_bottom_main_info_layer));
  
  // Handle current bluetooth connection state
  #ifdef PBL_ROUND
  s_bluetooth_layer = layer_create(GRect(12, 105, 20, 20));
  #else
  s_bluetooth_layer = layer_create(GRect(3, 2, 20, 20));
  #endif
  layer_set_update_proc(s_bluetooth_layer, bluetooth_layer_update);
  layer_add_child(window_get_root_layer(window), s_bluetooth_layer);
  
  bluetooth_handler(bluetooth_connection_service_peek());
  
  // Pebble Battery
  BatteryChargeState initial = battery_state_service_peek();
  battery_level = initial.charge_percent;
  battery_plugged = initial.is_plugged;
  battery_charging = initial.is_charging;
  #ifdef PBL_ROUND
  s_battery_layer = layer_create(GRect(window_bounds.size.w-23, 108, 8, 22));
  #else
  s_battery_layer = layer_create(GRect(-3+window_bounds.size.w-22, 4, 22, 8));
  #endif
  layer_set_update_proc(s_battery_layer, battery_layer_update_callback);
  layer_add_child(window_get_root_layer(window), s_battery_layer);
  
  battery_handler();
  
  // Bottom additional info layer
  if (SCREEN_HEIGHT >= 180) {
    editional_info_vertical_correction = 10;
  }
  s_bottom_additional_layer = layer_create(GRect(0, window_bounds.size.h+3, window_bounds.size.w, 35+editional_info_vertical_correction));
  layer_set_update_proc(s_bottom_additional_layer, draw_bottom_additional_info_layer);
  layer_add_child(window_get_root_layer(window), s_bottom_additional_layer);
  
  // Forecast
  s_forecast_layer = layer_create(GRect(0, -135-LAYER_VERTICAL_INDENT*2, window_bounds.size.w, 135+LAYER_VERTICAL_INDENT*2));
  layer_set_update_proc(s_forecast_layer, draw_forecast_layer);
  layer_add_child(window_get_root_layer(window), s_forecast_layer);
  
  // Forecast images
  int weather_small_image_height = 54;
  if (SCREEN_HEIGHT >= 228) {
    weather_small_image_height = 75;
  }
  s_weather_small_image_left_layer = bitmap_layer_create(GRect(0, 30, window_bounds.size.w/2, weather_small_image_height));
  bitmap_layer_set_bitmap(s_weather_small_image_left_layer, s_weather_small_image_left_bitmap);
  layer_add_child(s_forecast_layer, bitmap_layer_get_layer(s_weather_small_image_left_layer));
  s_weather_small_image_right_layer = bitmap_layer_create(GRect(window_bounds.size.w/2+1, 30, window_bounds.size.w/2, weather_small_image_height));
  bitmap_layer_set_bitmap(s_weather_small_image_right_layer, s_weather_small_image_right_bitmap);
  layer_add_child(s_forecast_layer, bitmap_layer_get_layer(s_weather_small_image_right_layer));
  
  if (s_forecast_show_duration) {
    accel_tap_service_subscribe(tap_handler);
  }
  
  // Make sure the time is displayed from the start
  update_time();
  
  update_bottom_main_info();
}

static void main_window_unload(Window *window) {
  //Unload GFont
  fonts_unload_custom_font(s_time_font);
  fonts_unload_custom_font(s_weather_date_font);
  
  //Destroy BitmapLayers
  gbitmap_destroy(s_weather_image_bitmap);
  gbitmap_destroy(s_weather_small_image_left_bitmap);
  gbitmap_destroy(s_weather_small_image_right_bitmap);
  
  bitmap_layer_destroy(s_weather_image_layer);
  bitmap_layer_destroy(s_weather_small_image_left_layer);
  bitmap_layer_destroy(s_weather_small_image_right_layer);
  
  // Destroy TextLayer
  text_layer_destroy(s_time_layer);
  text_layer_destroy(s_bottom_main_info_layer);
  
  // Destroy layers
  layer_destroy(s_background_layer);
  layer_destroy(s_bottom_additional_layer);
  layer_destroy(s_bluetooth_layer);
  layer_destroy(s_battery_layer);
  layer_destroy(s_forecast_layer);
    
  tick_timer_service_unsubscribe();
  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  accel_tap_service_unsubscribe();
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
  
  // Get weather update every 30 minutes
  if(tick_time->tm_min % 30 == 0 || (weather_out_of_date && tick_time->tm_min % 2 == 0)) {
    update_weather();
  }
}

static void update_weather() {
  // If bluetooth connection exists
  if(bluetooth_connection_service_peek()) {
    // Begin dictionary
    DictionaryIterator *iter;
    app_message_outbox_begin(&iter);

    // Add a key-value pair
    dict_write_uint8(iter, 0, 0);

    // Send the message!
    app_message_outbox_send();
  }
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  bool should_update_forecast = false;
  bool should_update_all_bottom_info = false;
  
  Tuple *temperature_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
  if (temperature_tuple) {
    snprintf(s_temperature_buffer, sizeof(s_temperature_buffer), "%s", temperature_tuple->value->cstring);
    persist_write_string(DB_SW_TEMPERATURE, s_temperature_buffer);
    should_update_all_bottom_info = true;
  }
  
  Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);
  if (conditions_tuple) {
    if (get_bitmap_id((int)conditions_tuple->value->int32, false)) {
      draw_weather_image((int)conditions_tuple->value->int32);
      persist_write_int(DB_SW_CONDITIONS, (int)conditions_tuple->value->int32);
    }
  }
  
  Tuple *bluetooth_tuple = dict_find(iterator, MESSAGE_KEY_BLUETOOTH);
  if (bluetooth_tuple) {
    snprintf(s_config_bluetooth, sizeof(s_config_bluetooth), "%s", bluetooth_tuple->value->cstring);
    persist_write_string(DB_SW_BLUETOOTH, s_config_bluetooth);
    // Handle current bluetooth connection state
    bluetooth_handler(bluetooth_connection_service_peek());
    update_weather();
  }
  
  Tuple *battery_tuple = dict_find(iterator, MESSAGE_KEY_BATTERY);
  if (battery_tuple) {
    if (battery_tuple->value->uint8 != 0) {
      snprintf(s_config_battery, sizeof(s_config_battery), "battery_always");
    }
    else {
      snprintf(s_config_battery, sizeof(s_config_battery), "on_charge");
    }
    persist_write_string(DB_SW_BATTERY, s_config_battery);
    battery_handler();
  }
  
  Tuple *screen_color_tuple = dict_find(iterator, MESSAGE_KEY_SCREEN_COLOR);
  if (screen_color_tuple) {
    snprintf(s_config_screen_color, sizeof(s_config_screen_color), "%s", screen_color_tuple->value->cstring);
    persist_write_string(DB_SW_SCREEN_COLOR, s_config_screen_color);
    update_layers_color();
  }
  
  Tuple *hourly_beep_tuple = dict_find(iterator, MESSAGE_KEY_HOURLY_BEEP);
  if (hourly_beep_tuple) {
    if (hourly_beep_tuple->value->uint8 != 0) {
      snprintf(s_config_hourly_beep, sizeof(s_config_hourly_beep), "beep");
    }
    else {
      snprintf(s_config_hourly_beep, sizeof(s_config_hourly_beep), "do_not_beep");
    }
    persist_write_string(DB_SW_HOURLY_BEEP, s_config_hourly_beep);
  }
  
  Tuple *hourly_lead_zero_tuple = dict_find(iterator, MESSAGE_KEY_HOUR_LEAD_ZERO);
  if (hourly_lead_zero_tuple) {
    hour_lead_zero = hourly_lead_zero_tuple->value->uint8 != 0;
    persist_write_bool(DB_SW_HOUR_LEAD_ZERO, hour_lead_zero);
    update_time();
  }
  
  Tuple *dnd_tuple = dict_find(iterator, MESSAGE_KEY_DND);
  if (dnd_tuple) {
    if (dnd_tuple->value->uint8 != 0) {
      snprintf(s_config_dnd, sizeof(s_config_dnd), "dnd_on");
    }
    else {
      snprintf(s_config_dnd, sizeof(s_config_dnd), "dnd_off");
    }
    persist_write_string(DB_SW_DND, s_config_dnd);
  }
  
  Tuple *dnd_start_tuple = dict_find(iterator, MESSAGE_KEY_DND_START);
  if (dnd_start_tuple) {
    s_config_dnd_start = (int)dnd_start_tuple->value->int32;
    persist_write_int(DB_SW_DND_START, s_config_dnd_start);
  }
  
  Tuple *dnd_end_tuple = dict_find(iterator, MESSAGE_KEY_DND_END);
  if (dnd_end_tuple) {
    s_config_dnd_end = (int)dnd_end_tuple->value->int32;
    persist_write_int(DB_SW_DND_END, s_config_dnd_end);
  }
  
  Tuple *forecast_duration_tuple = dict_find(iterator, MESSAGE_KEY_FORECAST_DURATION);
  if (forecast_duration_tuple) {
    s_forecast_show_duration = (int)forecast_duration_tuple->value->int32;
    persist_write_int(DB_SW_FORECAST_DURATION, s_forecast_show_duration);
    if (s_forecast_show_duration) {
      accel_tap_service_subscribe(tap_handler);
    }
    else {
      accel_tap_service_unsubscribe();
    }
  }
  
  Tuple *humidity_tuple = dict_find(iterator, MESSAGE_KEY_HUMIDITY);
  if (humidity_tuple) {
    s_humidity_buffer = (int)humidity_tuple->value->int32;
    persist_write_int(DB_SW_HUMIDITY, s_humidity_buffer);
    should_update_all_bottom_info = true;
  }
  
  Tuple *tomorrow_tuple = dict_find(iterator, MESSAGE_KEY_TOMORROW);
  if (tomorrow_tuple) {
    snprintf(s_tomorrow_buffer, sizeof(s_tomorrow_buffer), "%s", tomorrow_tuple->value->cstring);
    should_update_forecast = true;
  }
  
  Tuple *after_tomorrow_tuple = dict_find(iterator, MESSAGE_KEY_AFTER_TOMORROW);
  if (after_tomorrow_tuple) {
    snprintf(s_after_tomorrow_buffer, sizeof(s_after_tomorrow_buffer), "%s", after_tomorrow_tuple->value->cstring);
    should_update_forecast = true;
  }
  
  Tuple *tomorrow_conditions_tuple = dict_find(iterator, MESSAGE_KEY_TOMORROW_CONDITIONS);
  if (tomorrow_conditions_tuple) {
    s_tomorrow_condition_id = (int)tomorrow_conditions_tuple->value->int32;
    should_update_forecast = true;
  }
  
  Tuple *after_tomorrow_conditions_tuple = dict_find(iterator, MESSAGE_KEY_AFTER_TOMORROW_CONDITIONS);
  if (after_tomorrow_conditions_tuple) {
    s_after_tomorrow_condition_id = (int)after_tomorrow_conditions_tuple->value->int32;
    should_update_forecast = true;
  }
  
  Tuple *main_screen_info_tuple = dict_find(iterator, MESSAGE_KEY_MAIN_SCREEN_INFO);
  if (main_screen_info_tuple) {
    snprintf(s_config_main_screen_info, sizeof(s_config_main_screen_info), "%s", main_screen_info_tuple->value->cstring);
    persist_write_string(DB_SW_MAIN_SCREEN_INFO, s_config_main_screen_info);
    should_update_all_bottom_info = true;
  }
  
  Tuple *forecast_on_one_shake_tuple = dict_find(iterator, MESSAGE_KEY_FORECAST_ON_ONE_SHAKE);
  if (forecast_on_one_shake_tuple) {
    s_config_forecast_on_one_shake = forecast_on_one_shake_tuple->value->uint8 != 0;
    persist_write_bool(DB_SW_FORECAST_ON_ONE_SHAKE, s_config_forecast_on_one_shake);
  }
  
  if (should_update_all_bottom_info) {
    update_bottom_main_info();
    layer_mark_dirty(s_bottom_additional_layer);
  }
  if (should_update_forecast) {
    layer_mark_dirty(s_forecast_layer);
  }
}

static void bluetooth_handler(bool connected) {
  layer_mark_dirty(s_bluetooth_layer);
  if (!connected) {
    // Get a tm structure
    time_t temp = time(NULL); 
    struct tm *tick_time = localtime(&temp);
    
    if (last_bluetooth_connection != connected && battery_plugged == false && !is_silence(tick_time->tm_hour)) {
      // Vibe pattern: ON for 300ms:
      static const uint32_t segments[] = {300};
      VibePattern pat = {
        .durations = segments,
        .num_segments = ARRAY_LENGTH(segments),
      };
      vibes_enqueue_custom_pattern(pat);
    }
  }
  last_bluetooth_connection = connected;
}

static void bluetooth_layer_update(Layer *layer, GContext *ctx) {
  if (bluetooth_connection_service_peek()) {
    if (strcmp(s_config_bluetooth, "always") == 0) {
      // BLUETOOTH_CONNECTED
      #if defined(PBL_COLOR)
        draw_bluetooth(ctx, GColorBlueMoon);
      #else
        draw_bluetooth(ctx, get_main_color());
      #endif
    }
    else {
      // EMPTY
      draw_hide_bluetooth(ctx, get_background_color());
    }
  } else if (strcmp(s_config_bluetooth, "never") != 0) {
    // BLUETOOTH_DISCONNECTED
    #if defined(PBL_COLOR)
      draw_bluetooth_disconnected(ctx, GColorBlueMoon, get_background_color(), GColorSunsetOrange);
    #else
      draw_bluetooth_disconnected(ctx, get_main_color(), get_background_color(), get_main_color());
    #endif
  }
}

static void battery_handler() {
  BatteryChargeState charge_state = battery_state_service_peek();
  battery_level = charge_state.charge_percent;
  battery_plugged = charge_state.is_plugged;
  battery_charging = charge_state.is_charging;
  layer_mark_dirty(s_battery_layer);
}

static void battery_layer_update_callback(Layer *layer, GContext *ctx) {
  #if defined(PBL_ROUND)
  char *direction = "vertical";
  #else
  char *direction = "horizontal";
  #endif
  if (battery_plugged && !battery_charging) {
    draw_hide_battery_lighting(ctx, get_background_color(), direction);
    draw_battery_charged(ctx, get_main_color(), 7, direction);
  }
  else {
    if (battery_plugged || strcmp(s_config_battery, "battery_always")==0 || battery_level<=20) {
      draw_battery_filled(ctx, get_main_color(), get_background_color(), battery_level, 7, direction);
    }
    if (battery_plugged && battery_charging) {
      draw_battery_lighting(ctx, get_main_color(), direction);
    }
    else {
      draw_hide_battery_lighting(ctx, get_background_color(), direction);
    }
  }
}

static void set_container_image(GBitmap **bmp_image, BitmapLayer *bmp_layer, const int resource_id) {
  if (*bmp_image != NULL) {
    gbitmap_destroy(*bmp_image);
  }
  
  *bmp_image = gbitmap_create_with_resource(resource_id);
  
  bitmap_layer_set_bitmap(bmp_layer, *bmp_image);
  
  if (is_inverted_color()) {
    bitmap_layer_set_compositing_mode(bmp_layer, GCompOpSet);
  }
  else {
    #if defined(PBL_COLOR)
    bitmap_layer_set_compositing_mode(bmp_layer, GCompOpAssign);
    #else
    bitmap_layer_set_compositing_mode(bmp_layer, GCompOpSet);
    #endif
  }
}

static void update_layers_color() {
  text_layer_set_text_color(s_bottom_main_info_layer, get_main_color());
  text_layer_set_text_color(s_time_layer, get_main_color());
  bluetooth_handler(bluetooth_connection_service_peek());
  draw_weather_image(persist_read_int(DB_SW_CONDITIONS));
  layer_mark_dirty(s_battery_layer);
}

static bool is_inverted_color() {
  bool is_inverted = false;
  if (strcmp(s_config_screen_color, "white_screen")==0) {
    is_inverted = true;
  }
  
  return is_inverted;
}

static GColor get_main_color() {
  if (is_inverted_color()) {
    return GColorBlack;
  }
  return GColorWhite;
}

static GColor get_background_color() {
  if (is_inverted_color()) {
    return GColorWhite;
  }
  return GColorBlack;
}

static int16_t get_current_version() {
  return __pbl_app_info.process_version.major*10 + __pbl_app_info.process_version.minor;
}

static void draw_weather_image(int weather_id) {
  int image_id = get_bitmap_id(weather_id, false);
  
  if (image_id!=0) {
    set_container_image(&s_weather_image_bitmap, s_weather_image_layer, image_id);
  }
}

static void make_beep() {
  // Vibe pattern
  static const uint32_t segments[] = {100,100,200};
  VibePattern pat = {
    .durations = segments,
    .num_segments = ARRAY_LENGTH(segments),
  };
  vibes_enqueue_custom_pattern(pat);
}

static bool is_silence(int32_t hour) {
  if (strcmp(s_config_dnd, "dnd_on") == 0) {
    if (s_config_dnd_start>s_config_dnd_end) {
      if (hour>=s_config_dnd_start || hour<=s_config_dnd_end) {
        return true;
      }
    }
    else if (s_config_dnd_start<s_config_dnd_end) {
      if (hour>=s_config_dnd_start && hour<=s_config_dnd_end) {
        return true;
      }
    }
  }

  return false;
}

static void draw_background(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_stroke_color(ctx, get_main_color());
  graphics_context_set_fill_color(ctx, get_background_color());
  
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);
  
  int line_indent = 4;
  int line_pos_y = 135+LAYER_VERTICAL_INDENT*2;
  #if defined(PBL_ROUND)
    GBitmap *fb = graphics_capture_frame_buffer(ctx);
  
    for (int y = line_pos_y; y <= line_pos_y+1; y++) {
      // Get the min and max x values for this row
      GBitmapDataRowInfo info = gbitmap_get_data_row_info(fb, y);
      // Iterate over visible pixels in that row
      for (int x = info.min_x+line_indent; x < info.max_x-line_indent; x++) {
        memset(&info.data[x], get_main_color().argb, 1);
      }
    }
  
    // Release framebuffer
    graphics_release_frame_buffer(ctx, fb);
  #else
    graphics_context_set_stroke_color(ctx, get_main_color());
    graphics_draw_line(ctx, GPoint(line_indent, line_pos_y), GPoint(bounds.size.w-line_indent, line_pos_y));
    graphics_draw_line(ctx, GPoint(line_indent, line_pos_y+1), GPoint(bounds.size.w-line_indent, line_pos_y+1));
  #endif
}

static char* merge_strings(char *str_first, char *str_second) {
  static char merged_buffer[32];
  if (strlen(str_first) && strlen(str_second)) {
    #if defined(PBL_ROUND)
    snprintf(merged_buffer, sizeof(merged_buffer), "%s|%s", str_first, str_second);
    #else
    snprintf(merged_buffer, sizeof(merged_buffer), "%s | %s", str_first, str_second);
    #endif
  }
  else if (strlen(str_first)) {
    snprintf(merged_buffer, sizeof(merged_buffer), "%s", str_first);
  }
  else if (strlen(str_second)) {
    snprintf(merged_buffer, sizeof(merged_buffer), "%s", str_second);
  }
  
  return merged_buffer;
}

static void update_bottom_main_info() {
  static char first_info_buffer[32];
  static char second_info_buffer[32];
  if (strcmp(s_config_main_screen_info, "date_day_of_week")==0) {
    snprintf(first_info_buffer, sizeof(first_info_buffer), "%s", s_date_buffer);
    snprintf(second_info_buffer, sizeof(second_info_buffer), "%s", s_day_of_week_buffer);
  }
  else if (strcmp(s_config_main_screen_info, "temperature_humidity")==0) {
    snprintf(first_info_buffer, sizeof(first_info_buffer), "%s", s_temperature_buffer);
    snprintf(second_info_buffer, sizeof(second_info_buffer), "Δ%d%%", s_humidity_buffer);
  }
  else {
    // default
    snprintf(first_info_buffer, sizeof(first_info_buffer), "%s", s_date_buffer);
    snprintf(second_info_buffer, sizeof(second_info_buffer), "%s", s_temperature_buffer);
  }
  
  static char bottom_main_info_buffer[32];
  snprintf(bottom_main_info_buffer, sizeof(bottom_main_info_buffer), "%s", merge_strings(first_info_buffer, second_info_buffer));
  
  text_layer_set_text(s_bottom_main_info_layer, bottom_main_info_buffer);
}

static void hide_forecast_callback() {
  if (day_of_week_shown) {
    show_hide_forecast();
  }
}

static void tap_handler(AccelAxisType axis, int32_t direction) {
  time_t timestamp = time(NULL);
  
  if (s_config_forecast_on_one_shake || 3 >= timestamp - last_shake) {
    show_hide_forecast();
  }
  else {
    last_shake = timestamp;
  }
}

static void show_hide_forecast() {
  GRect start_dow;
  GRect finish_dow;
  GRect start_f;
  GRect finish_f;
  
  GRect bound_dow = layer_get_bounds(s_bottom_additional_layer);
  GRect bound_f = layer_get_bounds(s_forecast_layer);
  
  if (day_of_week_shown) {
    day_of_week_shown = false;
    
    start_dow = GRect(0, SCREEN_HEIGHT-bound_dow.size.h, bound_dow.size.w, bound_dow.size.h);
    finish_dow = GRect(0, SCREEN_HEIGHT+3, bound_dow.size.w, bound_dow.size.h);
    
    start_f = GRect(0, 0, bound_f.size.w, bound_f.size.h);
    finish_f = GRect(0, -bound_f.size.h, bound_f.size.w, bound_f.size.h);
  }
  else {
    day_of_week_shown = true;
    
    start_dow = GRect(0, SCREEN_HEIGHT+3, bound_dow.size.w, bound_dow.size.h);
    finish_dow = GRect(0, SCREEN_HEIGHT-bound_dow.size.h, bound_dow.size.w, bound_dow.size.h);
    
    start_f = GRect(0, -bound_f.size.h, bound_f.size.w, bound_f.size.h);
    finish_f = GRect(0, 0, bound_f.size.w, bound_f.size.h);
    
    s_forecast_timer = app_timer_register(s_forecast_show_duration*1000, hide_forecast_callback, NULL);
  }
  
  #ifdef PBL_SDK_2
    if (NULL != prop_anim_move_day_of_week) {
      property_animation_destroy(prop_anim_move_day_of_week);
    }
    if (NULL != prop_anim_move_forecast) {
      property_animation_destroy(prop_anim_move_forecast);
    }
  #endif
  
  prop_anim_move_day_of_week = property_animation_create_layer_frame(s_bottom_additional_layer, &start_dow, &finish_dow);
  Animation *anim_move_1_a = property_animation_get_animation(prop_anim_move_day_of_week);
  animation_set_duration(anim_move_1_a, 750);
  
  prop_anim_move_forecast = property_animation_create_layer_frame(s_forecast_layer, &start_f, &finish_f);
  Animation *anim_move_1_b = property_animation_get_animation(prop_anim_move_forecast);
  animation_set_duration(anim_move_1_b, 750);
  
  animation_schedule(anim_move_1_a);
  animation_schedule(anim_move_1_b);
}

static void draw_bottom_additional_info_layer(Layer *layer, GContext *ctx) {
  static char first_info_buffer[32];
  static char second_info_buffer[32];
  if (strcmp(s_config_main_screen_info, "date_day_of_week")==0) {
    snprintf(first_info_buffer, sizeof(first_info_buffer), "%s", s_temperature_buffer);
    snprintf(second_info_buffer, sizeof(second_info_buffer), "Δ%d%%", s_humidity_buffer);
  }
  else if (strcmp(s_config_main_screen_info, "temperature_humidity")==0) {
    snprintf(first_info_buffer, sizeof(first_info_buffer), "%s", s_date_buffer);
    snprintf(second_info_buffer, sizeof(second_info_buffer), "%s", s_day_of_week_buffer);
  }
  else {
    // default
    snprintf(first_info_buffer, sizeof(first_info_buffer), "%s", s_day_of_week_buffer);
    snprintf(second_info_buffer, sizeof(second_info_buffer), "Δ%d%%", s_humidity_buffer);
  }
  
  static char additional_info_buffer[32];
  snprintf(additional_info_buffer, sizeof(additional_info_buffer), "%s", merge_strings(first_info_buffer, second_info_buffer));
  
  GRect box = layer_get_bounds(layer);
  
  graphics_context_set_text_color(ctx, get_main_color());
  graphics_context_set_stroke_color(ctx, get_main_color());
  graphics_context_set_fill_color(ctx, get_background_color());
  graphics_fill_rect(ctx, GRect(box.origin.x, box.origin.y+4, box.size.w, box.size.h), 0, GCornerNone);
  graphics_draw_text(ctx, additional_info_buffer, s_weather_date_font, box, GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL);
}

static void draw_forecast_layer(Layer *layer, GContext *ctx) {
  int image_id;
  #if defined(PBL_ROUND)
  GTextAlignment tomorrow_image_aligment = GTextAlignmentRight;
  GTextAlignment after_tomorrow_image_aligment = GTextAlignmentLeft;
  #else
  GTextAlignment tomorrow_image_aligment = GTextAlignmentCenter;
  GTextAlignment after_tomorrow_image_aligment = GTextAlignmentCenter;
  #endif
  GRect box = layer_get_bounds(layer);
  
  graphics_context_set_text_color(ctx, get_main_color());
  graphics_context_set_stroke_color(ctx, get_main_color());
  graphics_context_set_fill_color(ctx, get_background_color());
  graphics_fill_rect(ctx, box, 0, GCornerNone);
  graphics_draw_line(ctx, GPoint(box.size.w/2, box.origin.y+4), GPoint(box.size.w/2, box.size.h));
  
  graphics_draw_text(ctx, s_tomorrow_buffer, s_weather_date_font,
                     GRect(box.origin.x, box.origin.y+forecast_layer_indent, box.size.w/2-forecast_layer_indent, box.size.h),
                     GTextOverflowModeWordWrap, tomorrow_image_aligment, NULL);
  
  graphics_draw_text(ctx, s_after_tomorrow_buffer, s_weather_date_font,
                     GRect(box.size.w/2+forecast_layer_indent, box.origin.y+forecast_layer_indent, box.size.w/2, box.size.h),
                     GTextOverflowModeWordWrap, after_tomorrow_image_aligment, NULL);

  if (s_tomorrow_condition_id) {
    image_id = get_bitmap_id(s_tomorrow_condition_id, true);
    if (image_id!=0) {
      set_container_image(&s_weather_small_image_left_bitmap, s_weather_small_image_left_layer, image_id);
    }
  }
  
  if (s_after_tomorrow_condition_id) {
    image_id = get_bitmap_id(s_after_tomorrow_condition_id, true);
    if (image_id!=0) {
      set_container_image(&s_weather_small_image_right_bitmap, s_weather_small_image_right_layer, image_id);
    }
  }
}

static void persist_read_and_set() {
  if (persist_exists(DB_SW_TEMPERATURE)) {
    persist_read_string(DB_SW_TEMPERATURE, s_temperature_buffer, sizeof(s_temperature_buffer));
  }
  if (persist_exists(DB_SW_BLUETOOTH)) {
    persist_read_string(DB_SW_BLUETOOTH, s_config_bluetooth, sizeof(s_config_bluetooth));
  }
  if (persist_exists(DB_SW_BATTERY)) {
    persist_read_string(DB_SW_BATTERY, s_config_battery, sizeof(s_config_battery));
  }
  if (persist_exists(DB_SW_SCREEN_COLOR)) {
    persist_read_string(DB_SW_SCREEN_COLOR, s_config_screen_color, sizeof(s_config_screen_color));
  }
  if (persist_exists(DB_SW_HOURLY_BEEP)) {
    persist_read_string(DB_SW_HOURLY_BEEP, s_config_hourly_beep, sizeof(s_config_hourly_beep));
  }
  if (persist_exists(DB_SW_DND)) {
    persist_read_string(DB_SW_DND, s_config_dnd, sizeof(s_config_dnd));
  }
  if (persist_exists(DB_SW_DND_START)) {
    s_config_dnd_start = persist_read_int(DB_SW_DND_START);
  }
  if (persist_exists(DB_SW_DND_END)) {
    s_config_dnd_end = persist_read_int(DB_SW_DND_END);
  }
  if (persist_exists(DB_SW_HUMIDITY)) {
    s_humidity_buffer = persist_read_int(DB_SW_HUMIDITY);
  }
  if (persist_exists(DB_SW_MAIN_SCREEN_INFO)) {
    persist_read_string(DB_SW_MAIN_SCREEN_INFO, s_config_main_screen_info, sizeof(s_config_main_screen_info));
  }
  hour_lead_zero = persist_exists(DB_SW_HOUR_LEAD_ZERO) ? persist_read_bool(DB_SW_HOUR_LEAD_ZERO) : true;
  s_config_forecast_on_one_shake = persist_exists(DB_SW_FORECAST_ON_ONE_SHAKE) ? persist_read_bool(DB_SW_FORECAST_ON_ONE_SHAKE) : false;

  s_forecast_show_duration = 3;
  if (persist_exists(DB_SW_FORECAST_DURATION)) {
    s_forecast_show_duration = persist_read_int(DB_SW_FORECAST_DURATION);
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
  weather_out_of_date = true;
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
  weather_out_of_date = true;
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
  weather_out_of_date = false;
}
  
static void init() {
  // Create main Window element and assign to pointer
  s_main_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(s_main_window, true);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  
  // Subscribe to Bluetooth updates
  bluetooth_connection_service_subscribe(bluetooth_handler);
  
  // Register battery handler
  battery_state_service_subscribe(battery_handler);
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);
  
  // Open AppMessage
  #ifdef PBL_COLOR
  app_message_open(512, 8);
  #else
  app_message_open(256, 8); //leave a bit of extra headroom on watches with more RAM
  #endif
//   app_message_open(APP_MESSAGE_INBOX_SIZE_MINIMUM, APP_MESSAGE_OUTBOX_SIZE_MINIMUM);
}

static void deinit() {
  // Stop any animation in progress
  animation_unschedule_all();

  // Destroy Window
  window_destroy(s_main_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}