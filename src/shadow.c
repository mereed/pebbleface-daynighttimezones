#include <pebble.h>
#include "config.h"

//go here to confirm image http://www.timeanddate.com/worldclock/sunearth.html
//http://codecorner.galanter.net/2015/04/03/simplify-access-to-framebuffer-on-pebble-time/
  
#define STR_SIZE 20
#define TIME_OFFSET_PERSIST 1
#define REDRAW_INTERVAL 15

static Window *window;
static TextLayer *time_text_layer;
static TextLayer *time_NY_text_layer;
static TextLayer *time_JPN_text_layer;
static TextLayer *time_GMT_text_layer;
static TextLayer *date_text_layer;
static TextLayer *s_battery_layer;

static GBitmap *background_image;
static BitmapLayer *background_layer;
static GFont custom_font;
static GFont custom_font2;

GBitmap *img_battery_100;
GBitmap *img_battery_90;
GBitmap *img_battery_80;
GBitmap *img_battery_70;
GBitmap *img_battery_60;
GBitmap *img_battery_50;
GBitmap *img_battery_40;
GBitmap *img_battery_30;
GBitmap *img_battery_20;
GBitmap *img_battery_10;
GBitmap *img_battery_charge;
BitmapLayer *layer_batt_img;

static GBitmap *world_bitmap;
//static GBitmap *world_NIGHT_bitmap;
static Layer *canvas;
// this is a manually created bitmap, of the same size as world_bitmap
#ifdef PBL_PLATFORM_APLITE 
  //APPLITE
  static GBitmap image;
#endif
//static GBitmap *imageB;
static int redraw_counter;
// s is set to memory of size STR_SIZE, and temporarily stores strings
char *s;
// Local time is wall time, not UTC, so an offset is used to get UTC
int time_offset;

static void handle_battery(BatteryChargeState charge_state) {
  static char battery_text[] = "10+";

  if (charge_state.is_charging) {
    //snprintf(battery_text, sizeof(battery_text), "++");
    snprintf(battery_text, sizeof(battery_text), "%d+", charge_state.charge_percent);
  } else {
    snprintf(battery_text, sizeof(battery_text), "%d", charge_state.charge_percent);
	    
		
        if (charge_state.charge_percent <= 10) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_10);
        } else if (charge_state.charge_percent <= 20) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_20);
		} else if (charge_state.charge_percent <= 30) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_30);
		} else if (charge_state.charge_percent <= 40) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_40);
		} else if (charge_state.charge_percent <= 50) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_50);
        } else if (charge_state.charge_percent <= 60) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_60);
		} else	if (charge_state.charge_percent <= 70) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_70);
		} else if (charge_state.charge_percent <= 80) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_80);
		} else if (charge_state.charge_percent <= 90) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_90);
		} else if (charge_state.charge_percent <= 99) {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_100);
		} else {
            bitmap_layer_set_bitmap(layer_batt_img, img_battery_100);
        } 
  }
  memmove(battery_text, &battery_text[0], sizeof(battery_text) - 1);
  text_layer_set_text(s_battery_layer, battery_text);
}

static void draw_earth() {

  #ifdef PBL_PLATFORM_APLITE 
  //APPLITE
    
  // ##### calculate the time
#ifdef UTC_OFFSET
  int now = (int)time(NULL) + -3600 * UTC_OFFSET;
#else
  int now = (int)time(NULL) + time_offset;
#endif
  

  float day_of_year; // value from 0 to 1 of progress through a year
  float time_of_day; // value from 0 to 1 of progress through a day
  // approx number of leap years since epoch
  // = now / SECONDS_IN_YEAR * .24; (.24 = average rate of leap years)
  int leap_years = (int)((float)now / 131487192.0);
  // day_of_year is an estimate, but should be correct to within one day
  day_of_year = now - (((int)((float)now / 31556926.0) * 365 + leap_years) * 86400);
  day_of_year = day_of_year / 86400.0;
  time_of_day = day_of_year - (int)day_of_year;
  day_of_year = day_of_year / 365.0;
  // ##### calculate the position of the sun
  // left to right of world goes from 0 to 65536
  int sun_x = (int)((float)TRIG_MAX_ANGLE * (1.0 - time_of_day));
  // bottom to top of world goes from -32768 to 32768
  // 0.2164 is march 20, the 79th day of the year, the march equinox
  // Earth's inclination is 23.4 degrees, so sun should vary 23.4/90=.26 up and down
  int sun_y = -sin_lookup((day_of_year - 0.2164) * TRIG_MAX_ANGLE) * .26 * .25;
  // ##### draw the bitmap
  int x, y;
  for(x = 0; x < image.bounds.size.w; x++) {
    int x_angle = (int)((float)TRIG_MAX_ANGLE * (float)x / (float)(image.bounds.size.w));
    for(y = 0; y < image.bounds.size.h; y++) {
      int byte = y * image.row_size_bytes + (int)(x / 8);
      int y_angle = (int)((float)TRIG_MAX_ANGLE * (float)y / (float)(image.bounds.size.h * 2)) - TRIG_MAX_ANGLE/4;
      // spherical law of cosines
      float angle = ((float)sin_lookup(sun_y)/(float)TRIG_MAX_RATIO) * ((float)sin_lookup(y_angle)/(float)TRIG_MAX_RATIO);
      angle = angle + ((float)cos_lookup(sun_y)/(float)TRIG_MAX_RATIO) * ((float)cos_lookup(y_angle)/(float)TRIG_MAX_RATIO) * ((float)cos_lookup(sun_x - x_angle)/(float)TRIG_MAX_RATIO);
      if ((angle < 0) ^ (0x1 & (((char *)world_bitmap->addr)[byte] >> (x % 8)))) {
        // white pixel
        ((char *)image.addr)[byte] = ((char *)image.addr)[byte] | (0x1 << (x % 8));
      } else {
        // black pixel
        ((char *)image.addr)[byte] = ((char *)image.addr)[byte] & ~(0x1 << (x % 8));
      }
    }
  }
  layer_mark_dirty(canvas);
  #else
  //BASALT
  #endif
  //
}

static void draw_watch(struct Layer *layer, GContext *ctx) {
  #ifdef PBL_PLATFORM_APLITE
    graphics_draw_bitmap_in_rect(ctx, &image, GRect(0,96,144,72));//image.bounds);
  #else
    //if (redraw_counter < REDRAW_INTERVAL) {
    // GBitmap *fb = graphics_capture_frame_buffer_format(ctx, GBitmapFormat8Bit);// 
    // graphics_draw_bitmap_in_rect(ctx, fb,GRect(0, 0, 144, 72));
    //}else {
    //redraw_counter = 0;

        graphics_draw_bitmap_in_rect(ctx, world_bitmap, GRect(0, 96, 144, 72));//gbitmap_get_bounds(world_bitmap));
        int now2 = (int)time(NULL) + time_offset;
  
        float day_of_year; // value from 0 to 1 of progress through a year
        float time_of_day; // value from 0 to 1 of progress through a day
        // approx number of leap years since epoch
        // = now / SECONDS_IN_YEAR * .24; (.24 = average rate of leap years)
        int leap_years = (int)((float)now2 / 131487192.0);
        // day_of_year is an estimate, but should be correct to within one day
        day_of_year = now2 - (((int)((float)now2 / 31556926.0) * 365 + leap_years) * 86400);
        day_of_year = day_of_year / 86400.0;
        time_of_day = day_of_year - (int)day_of_year;
        day_of_year = day_of_year / 365.0;
        // ##### calculate the position of the sun
        // left to right of world goes from 0 to 65536
        int sun_x = (int)((float)TRIG_MAX_ANGLE * (1.0 - time_of_day));
        // bottom to top of world goes from -32768 to 32768
        // 0.2164 is march 20, the 79th day of the year, the march equinox
        // Earth's inclination is 23.4 degrees, so sun should vary 23.4/90=.26 up and down
        int sun_y = -sin_lookup((day_of_year - 0.2164) * TRIG_MAX_ANGLE) * .26 * .25;
        // ##### draw the bitmap
        int x, y;

        static GBitmap *bb;
        bb = gbitmap_create_with_resource(RESOURCE_ID_NIGHT_PBLv2);//CLR64);//NIGHT_PBL);
  
        GBitmap *fb = graphics_capture_frame_buffer_format(ctx, GBitmapFormat8Bit);//
        uint8_t *fb_data = gbitmap_get_data(fb);  
    
        uint8_t *background_data = gbitmap_get_data(bb); 
        #define WINDOW_WIDTH 144 
        uint8_t (*fb_matrix)[WINDOW_WIDTH] = (uint8_t (*)[WINDOW_WIDTH]) fb_data;
        uint8_t (*background_matrix)[WINDOW_WIDTH] = (uint8_t (*)[WINDOW_WIDTH]) background_data;
  
        for(x = 0; x < 144; x++) {
          int x_angle = (int)((float)TRIG_MAX_ANGLE * (float)x / (float)(144));
           for(y = 0; y < 72; y++) {
              int y_angle = (int)((float)TRIG_MAX_ANGLE * (float)y / (float)(72 * 2)) - TRIG_MAX_ANGLE/4;
              // spherical law of cosines
              float angle = ((float)sin_lookup(sun_y)/(float)TRIG_MAX_RATIO) * ((float)sin_lookup(y_angle)/(float)TRIG_MAX_RATIO);
              angle = angle + ((float)cos_lookup(sun_y)/(float)TRIG_MAX_RATIO) * ((float)cos_lookup(y_angle)/(float)TRIG_MAX_RATIO) * ((float)cos_lookup(sun_x - x_angle)/(float)TRIG_MAX_RATIO);
              if (angle > 0) { //^ (0x1 & (((char *)world_bitmap->addr)[byte] >> (x % 8)))) {
                // white pixel
              } else {
                // black pixel
                fb_matrix[y+96][x] = background_matrix[y][x];
              }
          }
        }
        graphics_release_frame_buffer(ctx,fb);
        gbitmap_destroy(bb);
      //}
  #endif
}

static void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed) {
  handle_battery(battery_state_service_peek());
  //static char time_text[8] = "00:00 am";
  static char time_text[] = "00:00xx";
  static char time_JPN_text[] = "00:00xx"; //BEN ADDED
  static char time_NY_text[] = "00:00xx"; //BEN ADDED
  static char time_GMT_text[] = "00:00xx"; //BEN ADDED
  static char date_text[] = "Xxx, Xxx 00";
  static struct tm *JPN_time; //BEN ADDED
  static struct tm *NY_time; //BEN ADDED
  static struct tm *GMT_time; //BEN ADDED
 
  char *time_format;
	
	
  if (clock_is_24h_style()) {
        time_format = "%R";		
    } else {
        time_format = "%I:%M";
	
	}
    strftime(time_text, sizeof(time_text), time_format, tick_time);

	
    if (!clock_is_24h_style() && (time_text[0] == '0')) {
        memmove(time_text, &time_text[1], sizeof(time_text) - 1);
    }
    text_layer_set_text(time_text_layer, time_text);	
	
  strftime(date_text, sizeof(date_text), "%a, %b %e", tick_time);
  text_layer_set_text(date_text_layer, date_text);

  //new york START
  NY_time = tick_time;
  NY_time->tm_hour += 10;  // might need to adjust values
  if (NY_time->tm_hour > 23) {
   NY_time->tm_hour -= 24;
  }  
  
  //if (clock_is_24h_style()) {
    //strftime(time_SWE_text, sizeof(time_SWE_text), "%R",  SWE_time);
    strftime(time_NY_text, sizeof(time_NY_text), "%R", NY_time);
 // if (time_SWE_text[0] == '0') {
 //   memmove(time_SWE_text, &time_SWE_text[1], sizeof(time_SWE_text) - 1);
//  }
  text_layer_set_text(time_NY_text_layer, time_NY_text);  //Just added  
  //new york END//
  
  //JAPAN START
  JPN_time = tick_time;
  JPN_time->tm_hour += 13;  // might need to adjust values
  if (JPN_time->tm_hour > 23) {
   JPN_time->tm_hour -= 24;
  }  
    strftime(time_JPN_text, sizeof(time_JPN_text), "%R", JPN_time);
 // if (time_JPN_text[0] == '0') {
 //   memmove(time_JPN_text, &time_JPN_text[1], sizeof(time_JPN_text) - 1);
 // }
  text_layer_set_text(time_JPN_text_layer, time_JPN_text);  //Just added
  //JAPAN END//  
 
  //GMT START
  GMT_time = tick_time;
  GMT_time->tm_hour += 15;  // might need to adjust values
  if (GMT_time->tm_hour > 23) {
   GMT_time->tm_hour -= 24;
  }  
    strftime(time_GMT_text, sizeof(time_GMT_text), "%R", GMT_time);
 // if (time_JPN_text[0] == '0') {
 //   memmove(time_JPN_text, &time_JPN_text[1], sizeof(time_JPN_text) - 1);
 // }
  text_layer_set_text(time_GMT_text_layer, time_GMT_text);  //Just added
  //GMT END//  
  
  redraw_counter++;
  if (redraw_counter >= REDRAW_INTERVAL) {
    draw_earth();
    redraw_counter = 0;
  } 

}

// Get the time from the phone, which is probably UTC
// Calculate and store the offset when compared to the local clock
static void app_message_inbox_received(DictionaryIterator *iterator, void *context) {
  Tuple *t = dict_find(iterator, 0);
  int unixtime = t->value->int32;
  int now = (int)time(NULL) + time_offset;
  time_offset = unixtime - now;

  status_t s = persist_write_int(TIME_OFFSET_PERSIST, time_offset); 
  if (s) {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Saved time offset %d with status %d", time_offset, (int) s);
  } else {
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Failed to save time offset with status %d", (int) s);
  }
  #ifdef PBL_PLATFORM_APLITE //20150607 Tried to fix applite offset
    draw_earth();
  #endif
  
}

static void window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);

  custom_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LECO_14));
  custom_font2 = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_LECO_32));
	
#ifdef BLACK_ON_WHITE
  window_set_background_color(window, GColorWhite);
#else
      //#ifdef PBL_COLOR
      //window_set_background_color(window, GColorDukeBlue);
      //#else
      window_set_background_color(window, GColorWhite );
      //#endif
#endif
  GRect bounds = layer_get_bounds(window_layer);

  background_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BG2);
#ifdef PBL_PLATFORM_BASALT
  GRect bitmap_bounds = gbitmap_get_bounds(background_image);
#else
  GRect bitmap_bounds = background_image->bounds;
#endif	
  GRect frame = GRect(0, 0, bitmap_bounds.size.w, bitmap_bounds.size.h);
  background_layer = bitmap_layer_create(frame);
  bitmap_layer_set_bitmap(background_layer, background_image);
  layer_add_child(window_layer, bitmap_layer_get_layer(background_layer));
  
 //Local
  time_text_layer = text_layer_create(GRect(0, 33, 144-0, 168-0));//20150614 (GRect(0, 72, 144-0, 168-72));
#ifdef BLACK_ON_WHITE
  text_layer_set_background_color(time_text_layer, GColorWhite);
  text_layer_set_text_color(time_text_layer, GColorBlack);
#else
      //#ifdef PBL_COLOR
      //text_layer_set_background_color(time_text_layer, GColorDukeBlue);
      //#else
      text_layer_set_background_color(time_text_layer, GColorClear );
      //#endif
  text_layer_set_text_color(time_text_layer, GColorBlack);
#endif
  #ifdef PBL_PLATFORM_APLITE
    text_layer_set_font(time_text_layer, custom_font2);//FONT_KEY_BITHAM_34_MEDIUM_NUMBERS
  #else
    text_layer_set_font(time_text_layer, fonts_get_system_font(FONT_KEY_LECO_32_BOLD_NUMBERS  ));//FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));//FONT_KEY_BITHAM_34_MEDIUM_NUMBERS
  #endif  
  text_layer_set_text(time_text_layer, "");
  text_layer_set_text_alignment(time_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(time_text_layer));

  //Date
  date_text_layer = text_layer_create(GRect(0,24, 144-0, 168-120));//20150614 (GRect(0, 110, 144-0, 168-72));
	/*
  #ifdef PBL_PLATFORM_APLITE
    text_layer_set_font(date_text_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21));//FONT_KEY_BITHAM_34_MEDIUM_NUMBERS
  #else
    text_layer_set_font(date_text_layer, fonts_get_system_font(FONT_KEY_LECO_20));//FONT_KEY_BITHAM_34_MEDIUM_NUMBERS));//FONT_KEY_BITHAM_34_MEDIUM_NUMBERS
  #endif  
	*/
#ifdef BLACK_ON_WHITE
  text_layer_set_background_color(date_text_layer, GColorWhite);
  text_layer_set_text_color(date_text_layer, GColorBlack);
#else
      //#ifdef PBL_COLOR
      //text_layer_set_background_color(date_text_layer, GColorDukeBlue);
      //#else
      text_layer_set_background_color(date_text_layer, GColorClear );
      //#endif     
  text_layer_set_text_color(date_text_layer, GColorBlack);
#endif
  text_layer_set_font(date_text_layer, custom_font);
//  text_layer_set_font(date_text_layer, fonts_get_system_font(FONT_KEY_ROBOTO_CONDENSED_21 ));
  text_layer_set_text(date_text_layer, "");
  text_layer_set_text_alignment(date_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(date_text_layer));

	
	//new york
  time_NY_text_layer = text_layer_create(GRect(2, 72, 50, 16));
#ifdef BLACK_ON_WHITE
  text_layer_set_background_color(time_NY_text_layer, GColorClear);
  text_layer_set_text_color(time_NY_text_layer, GColorBlack);
#else
      //#ifdef PBL_COLOR
      //text_layer_set_background_color(time_SWE_text_layer, GColorDukeBlue);
      //#else
      text_layer_set_background_color(time_NY_text_layer, GColorClear );
      //#endif    
  text_layer_set_text_color(time_NY_text_layer, GColorBlack);
#endif
  text_layer_set_font(time_NY_text_layer, custom_font);
  text_layer_set_text(time_NY_text_layer, "");
  text_layer_set_text_alignment(time_NY_text_layer, GTextAlignmentLeft);
  layer_add_child(window_layer, text_layer_get_layer(time_NY_text_layer));  
  
  //JAPAN
  time_JPN_text_layer = text_layer_create(GRect(92, 72, 50, 16));
#ifdef BLACK_ON_WHITE
  text_layer_set_background_color(time_JPN_text_layer, GColorClear);
  text_layer_set_text_color(time_JPN_text_layer, GColorBlack);
#else
      //#ifdef PBL_COLOR
      //text_layer_set_background_color(time_JPN_text_layer, GColorDukeBlue);
      //#else
      text_layer_set_background_color(time_JPN_text_layer, GColorClear );
      //#endif  
  text_layer_set_text_color(time_JPN_text_layer, GColorBlack);
#endif
  text_layer_set_font(time_JPN_text_layer, custom_font);
  text_layer_set_text(time_JPN_text_layer, "");
  text_layer_set_text_alignment(time_JPN_text_layer, GTextAlignmentRight);
  layer_add_child(window_layer, text_layer_get_layer(time_JPN_text_layer));

 //GMT
  time_GMT_text_layer = text_layer_create(GRect(48, 72, 50, 16));
#ifdef BLACK_ON_WHITE
  text_layer_set_background_color(time_GMT_text_layer, GColorClear);
  text_layer_set_text_color(time_GMT_text_layer, GColorBlack);
#else
      //#ifdef PBL_COLOR
      //text_layer_set_background_color(time_GMT_text_layer, GColorDukeBlue);
      //#else
      text_layer_set_background_color(time_GMT_text_layer, GColorClear );
      //#endif  
  text_layer_set_text_color(time_GMT_text_layer, GColorBlack);
#endif
  text_layer_set_font(time_GMT_text_layer, custom_font);
  text_layer_set_text(time_GMT_text_layer, "");
  text_layer_set_text_alignment(time_GMT_text_layer, GTextAlignmentCenter);
  layer_add_child(window_layer, text_layer_get_layer(time_GMT_text_layer));
	
   //BATTERY TEXT
  s_battery_layer = text_layer_create(GRect(40, 6, 30, 20));
  text_layer_set_text_color(s_battery_layer, GColorBlack);
  text_layer_set_background_color(s_battery_layer, GColorClear);//GColorClear);
  text_layer_set_font(s_battery_layer, fonts_get_system_font(FONT_KEY_GOTHIC_14));
  text_layer_set_text_alignment(s_battery_layer, GTextAlignmentRight);
  text_layer_set_text(s_battery_layer, "  -");
  layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));
  
  img_battery_100   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_100);
  img_battery_90   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_90);
  img_battery_80   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_80);
  img_battery_70   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_70);
  img_battery_60   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_60);
  img_battery_50   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_50);
  img_battery_40   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_40);
  img_battery_30   = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_30);
  img_battery_20    = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_20);
  img_battery_10    = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_10);
  img_battery_charge = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BATT_CHARGING);

  layer_batt_img  = bitmap_layer_create(GRect(72, 11, 19, 9));
  bitmap_layer_set_bitmap(layer_batt_img, img_battery_100);
  layer_add_child(window_layer, bitmap_layer_get_layer(layer_batt_img));
	
  //BOTH APPLITE AND BASALT
    canvas = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
    layer_set_update_proc(canvas, draw_watch);
    layer_add_child(window_layer, canvas);
  //APPLITE ONLY 
  #ifdef PBL_PLATFORM_APLITE 
    image = *world_bitmap;
    image.addr = malloc(image.row_size_bytes * image.bounds.size.h);
  //draw_earth();// 20150607 does this fix appligth?
  #else
    //imageB = gbitmap_create_with_resource(RESOURCE_ID_WORLD);
  #endif     
  draw_earth(); ///20150607 does this fix appligth?
  
}

static void window_unload(Window *window) {
	
  layer_remove_from_parent(bitmap_layer_get_layer(background_layer));
  bitmap_layer_destroy(background_layer);
  gbitmap_destroy(background_image);
  background_image = NULL;
	
  layer_remove_from_parent(bitmap_layer_get_layer(layer_batt_img));
  bitmap_layer_destroy(layer_batt_img);
	
  gbitmap_destroy(img_battery_100);
  gbitmap_destroy(img_battery_90);
  gbitmap_destroy(img_battery_80);
  gbitmap_destroy(img_battery_70);
  gbitmap_destroy(img_battery_60);
  gbitmap_destroy(img_battery_50);
  gbitmap_destroy(img_battery_40);
  gbitmap_destroy(img_battery_30);
  gbitmap_destroy(img_battery_20);
  gbitmap_destroy(img_battery_10);
  gbitmap_destroy(img_battery_charge);
	
  text_layer_destroy(time_text_layer);
  text_layer_destroy(time_NY_text_layer);
  text_layer_destroy(time_JPN_text_layer);
  text_layer_destroy(time_GMT_text_layer);
  text_layer_destroy(date_text_layer);
  text_layer_destroy(s_battery_layer);
  fonts_unload_custom_font(custom_font);
  fonts_unload_custom_font(custom_font2);
  battery_state_service_unsubscribe();
  tick_timer_service_unsubscribe();
  layer_destroy(canvas);
  #ifdef PBL_PLATFORM_APLITE
    free(image.addr);
  #endif
}

static void init(void) {
  redraw_counter = 20; //PRESET READY TO UPDATE ON FIRST RUN -- MAY NEED TO FIX FOR APPLITE TO NOT LOAD TWICE

  // Load the UTC offset, if it exists
  time_offset = 0;
  if (persist_exists(TIME_OFFSET_PERSIST)) {
    time_offset = persist_read_int(TIME_OFFSET_PERSIST);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "loaded offset %d", time_offset);
  }

  #ifdef PBL_PLATFORM_APLITE 
  world_bitmap = gbitmap_create_with_resource(RESOURCE_ID_WORLD);
  #else 
  world_bitmap = gbitmap_create_with_resource(RESOURCE_ID_DAY_PBL_V10);//v6 darkerDAY_PBL);  //SIMPLE_DAY);//
  //world_NIGHT_bitmap = gbitmap_create_with_resource(RESOURCE_ID_NIGHT_PBL); // DO THIS ABOVE IN OTHER ROUTINE
  #endif

  window = window_create();
  window_set_window_handlers(window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload,
  });
  
  const bool animated = true;
  window_stack_push(window, animated);

  s = malloc(STR_SIZE);
  tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
  battery_state_service_subscribe(handle_battery);

  app_message_register_inbox_received(app_message_inbox_received);
  app_message_open(30, 0);
}

static void deinit(void) {
  //tick_timer_service_unsubscribe();
  free(s);
  window_destroy(window);
  gbitmap_destroy(world_bitmap);

}

int main(void) {
  init();

  APP_LOG(APP_LOG_LEVEL_DEBUG, "Done initializing, pushed window: %p", window);

  app_event_loop();
  deinit();
}

