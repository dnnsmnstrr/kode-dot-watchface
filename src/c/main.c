#include <pebble.h>

static Window *s_window;
static Layer *s_canvas;
static TextLayer *s_time_layer;
static GBitmap *s_logo;
static GFont s_outfit_font;
static char s_time_buffer[8];
static int16_t s_scale;
static int16_t s_offset_x;
static int16_t s_offset_y;

#ifdef PBL_COLOR
#define KODY_BLUE GColorVividCerulean
#define BOOK_BLACK GColorDarkGray
#define PAGE_WHITE GColorFromHEX(0xF5F5F0)
#else
#define KODY_BLUE GColorWhite
#define BOOK_BLACK GColorBlack
#define PAGE_WHITE GColorWhite
#endif

static int16_t scaled(int16_t value) {
  return (value * s_scale) / 1000;
}

static GPoint scaled_point(int16_t x, int16_t y) {
  return GPoint(s_offset_x + scaled(x), s_offset_y + scaled(y));
}

static GRect scaled_rect(int16_t x, int16_t y, int16_t w, int16_t h) {
  return GRect(s_offset_x + scaled(x), s_offset_y + scaled(y), scaled(w), scaled(h));
}

static int16_t scaled_size(int16_t value) {
  const int16_t result = scaled(value);
  return result > 1 ? result : 1;
}

static void stroke(GContext *ctx, GColor color, uint8_t width) {
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, scaled_size(width));
}

static void draw_eye(GContext *ctx, GPoint center, bool left) {
  graphics_context_set_fill_color(ctx, PAGE_WHITE);
  graphics_fill_circle(ctx, scaled_point(center.x, center.y), scaled_size(9));
  graphics_context_set_fill_color(ctx, GColorLightGray);
  graphics_fill_circle(ctx, scaled_point(center.x, center.y + 2), scaled_size(6));
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, scaled_point(center.x + (left ? 2 : -2), center.y + 3), scaled_size(4));
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, scaled_point(center.x + (left ? 3 : -1), center.y + 1), scaled_size(1));
}

static void draw_book(GContext *ctx) {
  graphics_context_set_fill_color(ctx, BOOK_BLACK);
  graphics_fill_rect(ctx, scaled_rect(28, 166, 91, 58), scaled_size(3), GCornerBottomLeft | GCornerBottomRight);

  // The cover is held open toward Kody.
  GPoint cover[] = {
    scaled_point(28, 166), scaled_point(119, 166),
    scaled_point(111, 178), scaled_point(28, 174)
  };
  GPathInfo cover_info = { .num_points = 4, .points = cover };
  GPath *cover_path = gpath_create(&cover_info);
  gpath_draw_filled(ctx, cover_path);
  gpath_destroy(cover_path);

  if (s_logo) {
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_bitmap_in_rect(ctx, s_logo, scaled_rect(33, 179, 80, 24));
  }
}

static void canvas_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Kody's huge, deliberately edge-cropped silhouette.
  graphics_context_set_fill_color(ctx, KODY_BLUE);
  graphics_fill_circle(ctx, scaled_point(151, 154), scaled_size(74));
  graphics_fill_rect(ctx, scaled_rect(78, 150, 122, 74), 0, GCornerNone);
  graphics_fill_circle(ctx, scaled_point(170, 82), scaled_size(8));
  graphics_fill_circle(ctx, scaled_point(191, 91), scaled_size(7));

  // Flatten the bottom and right edge into the display crop.
  graphics_fill_rect(ctx, scaled_rect(96, 193, 104, 35), 0, GCornerNone);

  draw_eye(ctx, GPoint(128, 121), true);
  draw_eye(ctx, GPoint(145, 122), false);

  // Single straight brow, nose, mouth, and trademark buck teeth.
  stroke(ctx, GColorBlack, 3);
  graphics_draw_line(ctx, scaled_point(116, 106), scaled_point(154, 106));
  graphics_draw_line(ctx, scaled_point(132, 135), scaled_point(141, 135));
  graphics_draw_line(ctx, scaled_point(136, 136), scaled_point(136, 142));
  graphics_context_set_fill_color(ctx, PAGE_WHITE);
  graphics_fill_rect(ctx, scaled_rect(132, 141, 5, 8), 0, GCornerNone);
  graphics_fill_rect(ctx, scaled_rect(138, 141, 5, 8), 0, GCornerNone);
  stroke(ctx, GColorBlack, 1);
  graphics_draw_line(ctx, scaled_point(137, 141), scaled_point(137, 149));

  draw_book(ctx);
}

static void update_time(void) {
  time_t now = time(NULL);
  struct tm *tick_time = localtime(&now);
  if (clock_is_24h_style()) {
    strftime(s_time_buffer, sizeof(s_time_buffer), "%H:%M", tick_time);
  } else {
    strftime(s_time_buffer, sizeof(s_time_buffer), "%l:%M", tick_time);
  }
  text_layer_set_text(s_time_layer, s_time_buffer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  update_time();
}

static void window_load(Window *window) {
  Layer *root = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(root);

#if defined(PBL_PLATFORM_EMERY)
  s_logo = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_KODE_LOGO);
#elif defined(PBL_PLATFORM_CHALK)
  s_logo = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_KODE_LOGO_ROUND);
#else
  s_logo = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_KODE_LOGO_SMALL);
#endif
  const int16_t width_scale = (bounds.size.w * 1000) / 200;
  const int16_t height_scale = (bounds.size.h * 1000) / 228;
  s_scale = width_scale < height_scale ? width_scale : height_scale;
  s_offset_x = (bounds.size.w - scaled(200)) / 2;
  s_offset_y = bounds.size.h - scaled(228);

  s_canvas = layer_create(bounds);
  layer_set_update_proc(s_canvas, canvas_update);
  layer_add_child(root, s_canvas);

  const int16_t time_y = bounds.size.h >= 200 ? 25 : 8;
  s_time_layer = text_layer_create(GRect(0, time_y, bounds.size.w, 52));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, PAGE_WHITE);
  s_outfit_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_OUTFIT_BOLD_42));
  text_layer_set_font(s_time_layer, s_outfit_font);
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(s_time_layer));
  update_time();
}

static void window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
  fonts_unload_custom_font(s_outfit_font);
  layer_destroy(s_canvas);
  gbitmap_destroy(s_logo);
}

static void init(void) {
  s_window = window_create();
  window_set_background_color(s_window, GColorBlack);
  window_set_window_handlers(s_window, (WindowHandlers) {
    .load = window_load,
    .unload = window_unload
  });
  window_stack_push(s_window, true);
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void deinit(void) {
  tick_timer_service_unsubscribe();
  window_destroy(s_window);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}
