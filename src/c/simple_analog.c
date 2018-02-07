#include "simple_analog.h"
#include <ctype.h>
#include "pebble.h"
#include <time.h>

static Window *s_window;
static Layer *s_hands_layer;
static GFont s_font;
static GFont s_font_date;
static BitmapLayer *s_bitmap_layer;
static GBitmap *s_bitmap;


static GPath *s_minute_arrow, *s_hour_arrow, *s_batt_arrow;
static char s_num_buffer[4], s_day_buffer[10];
static int battery_level = 0;
bool colour_state = 1;
bool connected = 1;
bool seconds_hand = 1;
uint32_t key1 = 0;
uint32_t key2 = 1;

GColor hands_fill_color, hands_stroke_color;
GColor bg_color, fg_color;



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


void colour_update_proc(){
  
// look for existing settings  

  if (persist_exists(key1)) {
  // Read persisted value
  colour_state = persist_read_bool(key1);
} else {
  // Choose a default value
  colour_state = 1;

  // Remember the default value until the user chooses their own value
  persist_write_bool(key1, colour_state);
}


if (persist_exists(key2)) {
  // Read persisted value
  seconds_hand = persist_read_bool(key2);
} else {
  // Choose a default value
  seconds_hand = 1;

  // Remember the default value until the user chooses their own value
  persist_write_bool(key2, seconds_hand);
}
  
// Dark theme
  if(colour_state==0) {
    bg_color = GColorBlack;
    fg_color = GColorWhite;
    hands_fill_color = PBL_IF_COLOR_ELSE(GColorYellow, GColorWhite);
    hands_stroke_color = GColorBlack;
    
    if(connected==1){
        gbitmap_destroy(s_bitmap);
        s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DIAL_NEG);
          }
    else{
        gbitmap_destroy(s_bitmap);
        s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DIAL_NEG_NO_BT);
          }
    bitmap_layer_set_bitmap(s_bitmap_layer,s_bitmap);
  }

// Light theme
 if(colour_state==1) {
    bg_color = GColorWhite;
    fg_color = GColorBlack;
    hands_fill_color = PBL_IF_COLOR_ELSE(GColorBlue, GColorBlack);
    hands_stroke_color = GColorWhite;
    
    if(connected==1){
        gbitmap_destroy(s_bitmap);
        s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DIAL_BLANK);
          }
    else{
        gbitmap_destroy(s_bitmap);
        s_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_DIAL_NO_BT);
          }
    
    bitmap_layer_set_bitmap(s_bitmap_layer,s_bitmap);
  }
}


// interpret setting app changes

void prv_inbox_received_handler(DictionaryIterator *iter, void *context) {
  // Read color preferences
  Tuple *colour_state_t = dict_find(iter, MESSAGE_KEY_colour_state);
  if(colour_state_t) {
    colour_state = colour_state_t->value->int32 == 1;
    persist_write_bool(key1, colour_state);
  }

  // Read second hand preferences
  Tuple *seconds_hand_t = dict_find(iter, MESSAGE_KEY_seconds_hand);
  if(seconds_hand_t) {
    seconds_hand = seconds_hand_t->value->int32 == 1;
    persist_write_bool(key2, seconds_hand);
  }

  colour_update_proc();
}

static void battery_handler(BatteryChargeState charge){
battery_level = charge.charge_percent;

}


static void handle_bluetooth(bool connection){
  connected = connection;
  colour_update_proc();
}



// What to do if the hands need updating
static void hands_update_proc(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  GPoint center = GPoint (71,80);
  
// Assign second hand length 
  const int16_t second_hand_length = bounds.size.w / 2;
  
// Get time
  time_t now = time(NULL);
  struct tm *t = localtime(&now);
  
// Work out second hand angle and coordinates of its end
  int32_t second_angle = TRIG_MAX_ANGLE * t->tm_sec / 60;
  GPoint second_hand = {
    .x = (int16_t)(sin_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.x,
    .y = (int16_t)(-cos_lookup(second_angle) * (int32_t)second_hand_length / TRIG_MAX_RATIO) + center.y,
  };

// Day complication
// Interpret day as all caps full day
  strftime(s_day_buffer, sizeof(s_day_buffer), "%A", t);
   for(char* pc=s_day_buffer;*pc!=0;++pc) *pc = lower_to_upper(*pc);
 
  graphics_context_set_text_color(ctx, fg_color);
  s_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CANDARA_14));
  graphics_draw_text(ctx, s_day_buffer, s_font, GRect(34, 40, 76, 18), GTextOverflowModeWordWrap, GTextAlignmentCenter, NULL );
  
// Date complication
// Interpet date and assign
  strftime(s_num_buffer, sizeof(s_num_buffer), "%d", t);
  
  graphics_context_set_text_color(ctx, fg_color);
  s_font_date = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BASKVILL_22));
  graphics_draw_text(ctx, s_num_buffer, s_font_date, GRect(62, 135, 22, 37), GTextOverflowModeWordWrap, GTextAlignmentLeft, NULL );
   

//  batt hand
    graphics_context_set_fill_color(ctx, bg_color);
    graphics_context_set_stroke_color(ctx, hands_fill_color);
    gpath_rotate_to(s_batt_arrow,  (TRIG_MAX_ANGLE/-6) +(TRIG_MAX_ANGLE * battery_level / 300));
    gpath_draw_outline(ctx, s_batt_arrow);
  
// dots in the middle of batt hand
  graphics_context_set_fill_color(ctx, hands_fill_color);
  uint16_t radius_batt = 3;
  graphics_fill_circle(ctx, GPoint(71,122), radius_batt);
  
  graphics_context_set_fill_color(ctx, bg_color);
  graphics_fill_circle(ctx, GPoint(71,122), 1);

  // minute/hour hand
  graphics_context_set_fill_color(ctx, hands_fill_color);
  graphics_context_set_stroke_color(ctx, hands_stroke_color);
 
  gpath_rotate_to(s_hour_arrow, (TRIG_MAX_ANGLE * (((t->tm_hour % 12) * 6) + (t->tm_min / 10))) / (12 * 6));
  gpath_draw_filled(ctx, s_hour_arrow);
  gpath_draw_outline(ctx, s_hour_arrow);

  gpath_rotate_to(s_minute_arrow, TRIG_MAX_ANGLE * t->tm_min / 60);
  gpath_draw_filled(ctx, s_minute_arrow);
  gpath_draw_outline(ctx, s_minute_arrow);

 // large dot in the middle of time hands
  graphics_context_set_fill_color(ctx, hands_fill_color);
  uint16_t radius = 8;
  graphics_fill_circle(ctx, GPoint(71,80), radius); 
  
// only display seconds hand if told to
  if (seconds_hand==1){
  graphics_context_set_stroke_color(ctx, fg_color);
  graphics_draw_line(ctx, second_hand, center);
  }
  
// small dot in the middle of time hands
  graphics_context_set_fill_color(ctx, bg_color);
  graphics_fill_circle(ctx, GPoint(71,80), 2);
 
}


static void handle_second_tick(struct tm *tick_time, TimeUnits units_changed) {
  layer_mark_dirty(window_get_root_layer(s_window));
}


static void window_load(Window *window) {
  
// Create window
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  
// Create background image layer  
  s_bitmap_layer = bitmap_layer_create(bounds);
  
//Create layer for hands
  s_hands_layer = layer_create(bounds);
  layer_set_update_proc(s_hands_layer, hands_update_proc);
  
//Add all of the layers to the window  
  layer_add_child(window_layer, bitmap_layer_get_layer(s_bitmap_layer));
  layer_add_child(window_layer, s_hands_layer);
  
  handle_bluetooth(connection_service_peek_pebble_app_connection());
  colour_update_proc();

}

static void window_unload(Window *window) {
  
  gbitmap_destroy(s_bitmap);
  bitmap_layer_destroy(s_bitmap_layer);
  layer_destroy(s_hands_layer);

}

static void init() {
  s_day_buffer[0] = '\0';
  s_num_buffer[0] = '\0';

 // Subscribe to battery events
  battery_state_service_subscribe(battery_handler);
  
// Subscribe to seconds tick event  
  tick_timer_service_subscribe(SECOND_UNIT, handle_second_tick);

// Subscribe to bluetooth connectivity events
  connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = handle_bluetooth
  });

// Check states
  battery_handler(battery_state_service_peek());
  handle_bluetooth(connection_service_peek_pebble_app_connection());
// Open AppMessage connection
  app_message_register_inbox_received(prv_inbox_received_handler);
  app_message_open(128, 128);
  
  colour_update_proc();
  
// init hand paths
  s_minute_arrow = gpath_create(&MINUTE_HAND_POINTS);
  s_hour_arrow = gpath_create(&HOUR_HAND_POINTS);
  s_batt_arrow = gpath_create(&BATT_HAND_POINTS);

  GPoint center = GPoint(71,80);
  gpath_move_to(s_minute_arrow, center);
  gpath_move_to(s_hour_arrow, center);
 
  GPoint batt_center = GPoint(71,122);
  gpath_move_to(s_batt_arrow, batt_center);
  

  
// Create window  
  s_window = window_create();
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  window_stack_push(s_window, true);


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

