#include "simple_analog.h"
#include <ctype.h>
#include "pebble.h"

static Window *s_window;
static Layer *s_date_layer, *s_hands_layer;
static TextLayer *s_day_label, *s_num_label, *s_day_label_neg, *s_num_label_neg;
static GFont s_font;
static GFont s_font_date;
static BitmapLayer *s_bitmap_layer, *s_bitmap_layer_no_bt;
static GBitmap *s_bitmap, *s_bitmap_no_bt;


static GPath *s_minute_arrow, *s_hour_arrow, *s_batt_arrow;
static char s_num_buffer[4], s_day_buffer[10];
static int battery_level = 0;
bool connected = 1;

char lower_to_upper(char ch1) {
char ch2;
if(ch1 >= 'a' && ch1 <= 'z'){
ch2 = ('A' + ch1 - 'a');
return ch2;
}
else{
ch2 = ch1;
return ch2;
}
}

static void battery_handler(BatteryChargeState charge){
battery_level = charge.charge_percent;
}


static void handle_bluetooth(bool connection){
  connected = !connection;

  if(connected==0){
  vibes_long_pulse();
  layer_set_hidden(bitmap_layer_get_layer(s_bitmap_layer), 0);
  layer_set_hidden(bitmap_layer_get_layer(s_bitmap_layer_no_bt), 1);
  layer_set_hidden(text_layer_get_layer(s_day_label),0);
  layer_set_hidden(text_layer_get_layer(s_day_label_neg),1);
  layer_set_hidden(text_layer_get_layer(s_num_label),0);
  layer_set_hidden(text_layer_get_layer(s_num_label_neg),1);  
}
  else{
  vibes_double_pulse();
  layer_set_hidden(bitmap_layer_get_layer(s_bitmap_layer), 1);
  layer_set_hidden(bitmap_layer_get_layer(s_bitmap_layer_no_bt), 0);
  layer_set_hidden(text_layer_get_layer(s_day_label),1);
  layer_set_hidden(text_layer_get_layer(s_day_label_neg),0);
  layer_set_hidden(text_layer_get_layer(s_num_label),1);
  layer_set_hidden(text_layer_get_layer(s_num_label_neg),0);  
  }
}
  
static void hands_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = GPoint (71,80);
  
  const int16_t second_hand_length = PBL_IF_ROUND_ELSE((bounds.size.w / 2) - 19, bounds.size.w / 2);

  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  int32_t second_angle = TRIG_MAX_ANGLE * t->tm_sec / 60;
  GPoint second_hand = {
    .x = (int16_t)(sin_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.y,
  };
  

//  batt hand
  if (connected==1){
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorYellow, GColorWhite));
    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorYellow, GColorWhite));
  }
  else {
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorBlue, GColorBlack));
    graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorBlue, GColorBlack));
   }
    gpath_rotate_to(s_batt_arrow,  (TRIG_MAX_ANGLE/-6) +(TRIG_MAX_ANGLE * battery_level / 300));
    gpath_draw_outline(ctx, s_batt_arrow);
  
// dot in the middle of batt hand
  if (connected==1){
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorYellow, GColorWhite));
  }
  else {
    graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorBlue, GColorBlack)); 
  }
  uint16_t radius_batt = 3;
  graphics_fill_circle(ctx, GPoint(71,122), radius_batt);
  
  if (connected==1){
      graphics_context_set_fill_color(ctx, GColorBlack);
  }
  else {
      graphics_context_set_fill_color(ctx, GColorWhite);   
  }

  graphics_fill_circle(ctx, GPoint(71,122), 1);

  
// second hand
 if (connected==1){
  graphics_context_set_stroke_color(ctx, GColorWhite);
 }
  else {
   graphics_context_set_stroke_color(ctx, GColorBlack);
  }
  graphics_draw_line(ctx, second_hand, center);

// minute/hour hand
  if (connected==1){
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorYellow, GColorWhite));
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorBlack, GColorBlack));
  }
  else {
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorBlue, GColorBlack));
  graphics_context_set_stroke_color(ctx, PBL_IF_COLOR_ELSE(GColorWhite, GColorWhite));  
  }
  gpath_rotate_to(s_minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, s_minute_arrow);
  gpath_draw_outline(ctx, s_minute_arrow);

  gpath_rotate_to(s_hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, s_hour_arrow);
  gpath_draw_outline(ctx, s_hour_arrow);

// dot in the middle of time hands
  
  if (connected==1){
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorYellow, GColorWhite));
  }
  else {
  graphics_context_set_fill_color(ctx, PBL_IF_COLOR_ELSE(GColorBlue, GColorBlack));
  }
  uint16_t radius = 8;
  graphics_fill_circle(ctx, GPoint(71,80), radius);
  
  if (connected==1){
  graphics_context_set_fill_color(ctx, GColorBlack);
  }
  else {
  graphics_context_set_fill_color(ctx, GColorWhite);
  }
  graphics_fill_circle(ctx, GPoint(71,80), 2);
 
}

static void date_update_proc(Layer *layer, GContext *ctx) {
  time_t now = time(NULL);
  struct tm *t = localtime(&now);

  strftime(s_day_buffer, sizeof(s_day_buffer), "%A", t);
   for(char* pc=s_day_buffer;*pc!=0;++pc) *pc = lower_to_upper(*pc);
  text_layer_set_text(s_day_label, s_day_buffer);
  text_layer_set_text(s_day_label_neg, s_day_buffer);
  

  strftime(s_num_buffer, sizeof(s_num_buffer), "%d", t);
  text_layer_set_text(s_num_label, s_num_buffer);
  text_layer_set_text(s_num_label_neg, s_num_buffer);
}

static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(s_window));
}




static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
  s_bitmap_layer = bitmap_layer_create(bounds);
  s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DIAL_BLANK);
  bitmap_layer_set_bitmap(s_bitmap_layer, s_bitmap);

  s_bitmap_layer_no_bt = bitmap_layer_create(bounds);
  s_bitmap_no_bt = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DIAL_NEG);
  bitmap_layer_set_bitmap(s_bitmap_layer_no_bt, s_bitmap_no_bt);
  

  s_date_layer = layer_create(bounds);
  layer_set_update_proc(s_date_layer, date_update_proc);

  s_day_label = text_layer_create(GRect(34, 40, 76, 18));
  text_layer_set_text(s_day_label, s_day_buffer);
  text_layer_set_text_color(s_day_label, GColorBlack);    
  text_layer_set_background_color(s_day_label, GColorClear);
  text_layer_set_text_alignment(s_day_label, GTextAlignmentCenter);
  s_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CANDARA_14));
  text_layer_set_font(s_day_label, s_font);

  s_num_label = text_layer_create(GRect(62, 136, 22, 37));
  text_layer_set_text(s_num_label, s_num_buffer);
  text_layer_set_text_color(s_num_label, GColorBlack); 
  text_layer_set_background_color(s_num_label, GColorClear);
  s_font_date = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BASKVILL_22));
  text_layer_set_font(s_num_label, s_font_date);

  s_day_label_neg = text_layer_create(GRect(34, 40, 76, 18));
  text_layer_set_text(s_day_label_neg, s_day_buffer);
  text_layer_set_text_color(s_day_label_neg, GColorWhite);    
  text_layer_set_background_color(s_day_label_neg, GColorClear);
  text_layer_set_text_alignment(s_day_label_neg, GTextAlignmentCenter);
  s_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CANDARA_14));
  text_layer_set_font(s_day_label_neg, s_font);

  s_num_label_neg = text_layer_create(GRect(62, 136, 22, 37));
  text_layer_set_text(s_num_label_neg, s_num_buffer);
  text_layer_set_text_color(s_num_label_neg, GColorWhite); 
  text_layer_set_background_color(s_num_label_neg, GColorClear);
  s_font_date = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BASKVILL_22));
  text_layer_set_font(s_num_label_neg, s_font_date);
  
  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  
  
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer_no_bt));
 
  
  layer_add_child(window_layer, s_date_layer);
  layer_add_child(s_date_layer, text_layer_get_layer(s_day_label));
  layer_add_child(s_date_layer, text_layer_get_layer(s_num_label));
  layer_add_child(s_date_layer, text_layer_get_layer(s_day_label_neg));
  layer_add_child(s_date_layer, text_layer_get_layer(s_num_label_neg));

  
  layer_add_child(window_layer, s_hands_layer);
  
  handle_bluetooth(connection_service_peek_pebble_app_connection());
}

static void window_unload(Window *window) {
  
  gbitmap_destroy(s_bitmap);
  bitmap_layer_destroy(s_bitmap_layer);
  bitmap_layer_destroy(s_bitmap_layer_no_bt);
  layer_destroy(s_date_layer);

  text_layer_destroy(s_day_label);
  text_layer_destroy(s_num_label);
  text_layer_destroy(s_day_label_neg);
  text_layer_destroy(s_num_label_neg);
  
  layer_destroy(s_hands_layer);

}

static void init() {
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);

  s_day_buffer[0] = '\0';
  s_num_buffer[0] = '\0';

  // init hand paths
  s_minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
  s_hour_arrow = gpath_create(&HOUR_HAND_POINTS);
  s_batt_arrow = gpath_create(&BATT_HAND_POINTS);

  GPoint center = GPoint(71,80);
  gpath_move_to(s_minute_arrow, center);
  gpath_move_to(s_hour_arrow, center);
 
  GPoint batt_center = GPoint(71,122);
  gpath_move_to(s_batt_arrow, batt_center);
  
  battery_state_service_subscribe(battery_handler);
  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);
 
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = handle_bluetooth
  });
  
  battery_handler(battery_state_service_peek());
  handle_bluetooth(connection_service_peek_pebble_app_connection());
}

static void deinit() {
  gpath_destroy(s_minute_arrow);
  gpath_destroy(s_hour_arrow);
 
  connection_service_unsubscribe();
  battery_state_service_unsubscribe();
  tick_timer_service_unsubscribe();
  
  window_destroy(s_window);
}

int main() {
  init();
  app_event_loop();
  deinit();
}
