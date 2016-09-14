#include "pebble.h"

#include "Red-Sox.h"

#include "handle_bluetooth.h"

#include "BTLine_update_callback.h"

#include "red_line_layer_update_callback.h"

#include "Battery.h"

#include "Callbacks.h"


Window *window;

static GBitmap     *image;
static BitmapLayer *image_layer;

TextLayer *text_dayname_layer;
TextLayer *text_mmdd_layer;
TextLayer *text_year_layer;
TextLayer *text_time_layer;

GFont        fontHelvNewLight20;
GFont		     fontRobotoCondensed25;
GFont        fontSystemGothicBold28;
GFont        fontRobotoBoldSubset40;
GFont        fontRobotoBoldSubset49;

//Common Defines

int       BTConnected = 1;
int       BTVibesDone = 0;

Layer     *RedLineLayer;
Layer     *BatteryLineLayer;
Layer     *BTLayer;

TextLayer *text_battery_layer;

int       PersistBTLoss         =  0;     // 0 = No Vib, 1 = Vib
char      PersistDateFormat[]   = "0";    // 0 = US, 1 = Intl

char      date_format[]         = "%b %d";

//End Common

static char dayname_text[] = "XXX";
static char time_text[] = "00:00";
static char mmdd_text[] = "XXXX00";
static char year_text[] = "0000";
static char seconds_text[]="00";

int FirstTime = 0;

GPoint     Linepoint;

void handle_appfocus(bool in_focus){
    if (in_focus) {
        handle_bluetooth(bluetooth_connection_service_peek());
    }
}

void handle_tick(struct tm *tick_time, TimeUnits units_changed) {

  char time_format[] = "%I:%M";

  strftime(seconds_text, sizeof(seconds_text), "%S", tick_time);
      
 if (clock_is_24h_style()) {
    strcpy(time_format,"%R");
    } else {
    strcpy(time_format,"%I:%M");
    }
  
// Set Time of Day  
  strftime(time_text, sizeof(time_text), time_format, tick_time); 
  
  // Kludge to handle lack of non-padded hour format string
  if (!clock_is_24h_style() && (time_text[0] == '0')) {
      memmove(time_text, &time_text[1], sizeof(time_text) - 1);
  } 

  if((strcmp(seconds_text,"00") == 0) || (FirstTime == 0)) {
     //Set Day and Date
     strftime(dayname_text, sizeof(dayname_text), "%a",    tick_time);
     strftime(mmdd_text,    sizeof(mmdd_text),   date_format, tick_time); 
     strftime(year_text,    sizeof(year_text),    "%Y",    tick_time); 

     text_layer_set_text(text_dayname_layer, dayname_text);
     text_layer_set_text(text_mmdd_layer, mmdd_text);
     text_layer_set_text(text_year_layer, year_text);      
  }  

if (units_changed & DAY_UNIT) {
   // Only update the day name & date when it's changed. 
    text_layer_set_text(text_dayname_layer, dayname_text);
    text_layer_set_text(text_mmdd_layer, mmdd_text);
  }

  if((strcmp(seconds_text,"00") == 0) || (FirstTime == 0)) {
     text_layer_set_text(text_time_layer, time_text); 
  }
  FirstTime = 1;
}

void handle_deinit(void) {
  tick_timer_service_unsubscribe();
  
  persist_write_string(MESSAGE_KEY_DATE_FORMAT_KEY,       PersistDateFormat);
  persist_write_int(MESSAGE_KEY_BT_VIBRATE_KEY,           PersistBTLoss);
  persist_write_int(MESSAGE_KEY_LOW_BATTERY_KEY,          PersistLow_Batt);


  battery_state_service_unsubscribe();
  bluetooth_connection_service_unsubscribe();
  app_focus_service_unsubscribe();
  
  text_layer_destroy(text_time_layer);
  text_layer_destroy(text_year_layer);
  text_layer_destroy(text_mmdd_layer);
  text_layer_destroy(text_dayname_layer);
  text_layer_destroy(text_battery_layer);

  layer_destroy(RedLineLayer);
  layer_destroy(BatteryLineLayer);
  layer_destroy(BTLayer);
   
  gbitmap_destroy(image);
  bitmap_layer_destroy(image_layer);
  
  fonts_unload_custom_font(fontHelvNewLight20);
  fonts_unload_custom_font(fontRobotoCondensed25);
  fonts_unload_custom_font(fontRobotoBoldSubset40); 
  fonts_unload_custom_font(fontRobotoBoldSubset49);
  
  window_destroy(window);
}

//Receive Input from Config page:

void handle_init(void) { 
  APP_LOG(APP_LOG_LEVEL_ERROR, "In Init * * * * *");
  
  window = window_create();
  window_stack_push(window, true /* Animated */);
  window_set_background_color(window, GColorDukeBlue);

  Layer *window_layer = window_get_root_layer(window);
  
  fontHelvNewLight20     = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_HELV_NEW_LIGHT_20));
  fontRobotoCondensed25  = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_25));
  fontSystemGothicBold28 = fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD);
  fontRobotoBoldSubset40 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_40));
  fontRobotoBoldSubset49 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49));
  
  // Register callbacks
  app_message_register_inbox_received(inbox_received_callback);
  app_message_register_inbox_dropped(inbox_dropped_callback);
  app_message_register_outbox_failed(outbox_failed_callback);
  app_message_register_outbox_sent(outbox_sent_callback);

  // Open AppMessage
  app_message_open(64, 64);

  //Red Sox Logo
  image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_RED_SOX_LOGO);
  
  // Dayname
  #ifdef PBL_PLATFORM_CHALK 
    text_dayname_layer = text_layer_create(GRect(75, 29, 88, 40));
  #else
    text_dayname_layer = text_layer_create(GRect(56, 29, 88, 40));
  #endif
    
  text_layer_set_text_color(text_dayname_layer, GColorWhite);
  text_layer_set_background_color(text_dayname_layer, GColorDukeBlue);
  text_layer_set_font(text_dayname_layer, fontSystemGothicBold28);
  text_layer_set_text_alignment(text_dayname_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_dayname_layer)); 
 
  // mmdd
  #ifdef PBL_PLATFORM_CHALK 
      text_mmdd_layer = text_layer_create(GRect(75, 58, 88, 40));
  #else
      text_mmdd_layer = text_layer_create(GRect(56, 58, 88, 40));
  #endif
    
  text_layer_set_text_color(text_mmdd_layer, GColorWhite);
  text_layer_set_background_color(text_mmdd_layer, GColorDukeBlue);
  text_layer_set_text_alignment(text_mmdd_layer, GTextAlignmentCenter);
  text_layer_set_font(text_mmdd_layer,  fontRobotoCondensed25);
  layer_add_child(window_layer, text_layer_get_layer(text_mmdd_layer));

  // year
  #ifdef PBL_PLATFORM_CHALK 
      text_year_layer = text_layer_create(GRect(75, 86, 88, 40));
  #else
    text_year_layer = text_layer_create(GRect(56, 86, 88, 40));
  #endif
    
  text_layer_set_text_color(text_year_layer, GColorWhite);
  text_layer_set_background_color(text_year_layer, GColorDukeBlue);
  text_layer_set_text_alignment(text_year_layer, GTextAlignmentCenter);
  text_layer_set_font(text_year_layer,  fontRobotoCondensed25);
  layer_add_child(window_layer, text_layer_get_layer(text_year_layer));

// The bitmap layer holds the image for logo display
  #ifdef PBL_PLATFORM_CHALK   
      image_layer = bitmap_layer_create(GRect(20, 40, 55, 73));
  #else
      image_layer = bitmap_layer_create(GRect(1, 35, 55, 73));
  #endif
    
  bitmap_layer_set_bitmap(image_layer, image);
  bitmap_layer_set_alignment(image_layer, GAlignCenter);
  layer_add_child(window_layer, bitmap_layer_get_layer(image_layer));
  
 //Persistent Value Date Format:
  if (persist_exists(MESSAGE_KEY_DATE_FORMAT_KEY)) {
     persist_read_string(MESSAGE_KEY_DATE_FORMAT_KEY  , PersistDateFormat, sizeof(PersistDateFormat));
     APP_LOG(APP_LOG_LEVEL_INFO, "    Set Date Format to Persistant: %s, 0 = US, 1 = Int'l", PersistDateFormat);
  }  else {
     APP_LOG(APP_LOG_LEVEL_INFO, "    Set Date Format to Default 0 = US");
     strcpy(PersistDateFormat, "0"); //Default
  }

  if (strcmp(PersistDateFormat, "0") == 0) {     // US
     strcpy(date_format, "%b %e %Y");
  } else {
     strcpy(date_format, "%e %b %Y");
  }

  //Persistent Value Vib On BTLoss
  if(persist_exists(MESSAGE_KEY_BT_VIBRATE_KEY)) {
     PersistBTLoss = persist_read_int(MESSAGE_KEY_BT_VIBRATE_KEY);
     APP_LOG(APP_LOG_LEVEL_INFO, "    Set BT Vibrate To Persistant %d (0 = NO Vib, 1 = Vib", PersistBTLoss);
  }  else {
     PersistBTLoss = 0; // Default
     APP_LOG(APP_LOG_LEVEL_INFO, "    Set BT Vibrate To 0 Default - No Vibrate");

  }

  //Persistent Value Vib on Low Batt
  if(persist_exists(MESSAGE_KEY_LOW_BATTERY_KEY)) {
     PersistLow_Batt = persist_read_int(MESSAGE_KEY_LOW_BATTERY_KEY);
     APP_LOG(APP_LOG_LEVEL_INFO, "    Set Low Batt Vibrate To Persistant %d (0 = NO Vib, 1 = Vib", PersistLow_Batt);
  }  else {
     PersistLow_Batt = 0; // Default
     APP_LOG(APP_LOG_LEVEL_INFO, "    Set Low Batt Vibrate To 0 Default - No Vibrate");

  }
 
  // Time of Day is here
  #ifdef PBL_PLATFORM_CHALK   
      text_time_layer = text_layer_create(GRect(1, 120, 180, 53));
      text_layer_set_font(text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_40)));
  #else
      text_time_layer = text_layer_create(GRect(1, 115, 144, 53));
      text_layer_set_font(text_time_layer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_49)));
  #endif
    
  text_layer_set_text_color(text_time_layer, GColorWhite);
  text_layer_set_background_color(text_time_layer, GColorDukeBlue);
  text_layer_set_text_alignment(text_time_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(text_time_layer));
  
  // Red Line
  #ifdef PBL_PLATFORM_CHALK   
      GRect line_frame = GRect(38, 32, 104, 2);
  #else
      GRect line_frame = GRect(10, 29, 124, 2);
  #endif
    
  RedLineLayer = layer_create(line_frame);
  layer_set_update_proc(RedLineLayer, red_line_layer_update_callback);
  layer_add_child(window_layer, RedLineLayer);
  
  // Battery Line
  #ifdef PBL_PLATFORM_CHALK   
      GRect battery_line_frame = GRect(39, 119, 104, 6);
  #else
      GRect battery_line_frame = GRect(22, 117, 104, 6);
  #endif
    
  BatteryLineLayer = layer_create(battery_line_frame);
  layer_set_update_proc(BatteryLineLayer, battery_line_layer_update_callback);
  layer_add_child(window_layer, BatteryLineLayer);
  
  tick_timer_service_subscribe(SECOND_UNIT, handle_tick);

    //Bluetooth Logo Setup area
    #ifdef PBL_PLATFORM_CHALK
        GRect BTArea = GRect(55, 5, 20, 20);
    #else
        GRect BTArea = GRect(1, 5, 20, 20);
    #endif
      
    BTLayer = layer_create(BTArea);

    layer_add_child(window_layer, BTLayer);

    layer_set_update_proc(BTLayer, BTLine_update_callback);

    bluetooth_connection_service_subscribe(&handle_bluetooth);
         
    //Battery area
    #ifdef PBL_PLATFORM_CHALK
        text_battery_layer = text_layer_create(GRect(75,2,55,27));
    #else
        text_battery_layer = text_layer_create(GRect(85,2,55,27));
    #endif
      
    text_layer_set_text_color(text_battery_layer,GColorWhite);
    text_layer_set_background_color(text_battery_layer, GColorDukeBlue);
    text_layer_set_font(text_battery_layer,   fontHelvNewLight20);
    text_layer_set_text_alignment(text_battery_layer, GTextAlignmentRight);

    layer_add_child(window_layer, text_layer_get_layer(text_battery_layer));

    battery_state_service_subscribe(&handle_battery);
    handle_battery(battery_state_service_peek());
     
    bluetooth_connection_service_subscribe(&handle_bluetooth);
    handle_bluetooth(bluetooth_connection_service_peek());
    
    app_focus_service_subscribe(&handle_appfocus);
  
  // Ensures time is displayed immediately (will break if NULL tick event accessed).
  // (This is why it's a good idea to have a separate routine to do the update itself.)
  time_t now = time(NULL);
  struct tm *current_time = localtime(&now);
  handle_tick(current_time, SECOND_UNIT);

}


int main(void) {
   handle_init();

   app_event_loop();
   
   handle_deinit();
}