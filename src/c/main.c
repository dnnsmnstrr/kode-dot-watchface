#include <pebble.h>

static Window *s_window;
static Layer *s_canvas;
static TextLayer *s_time_layer;
static GBitmap *s_logo;
static char s_time_buffer[8];

#ifdef PBL_COLOR
#define KODY_BLUE GColorVividCerulean
#define BOOK_BLACK GColorDarkGray
#define PAGE_WHITE GColorFromHEX(0xF5F5F0)
#else
#define KODY_BLUE GColorWhite
#define BOOK_BLACK GColorBlack
#define PAGE_WHITE GColorWhite
#endif

static void stroke(GContext *ctx, GColor color, uint8_t width) {
  graphics_context_set_stroke_color(ctx, color);
  graphics_context_set_stroke_width(ctx, width);
}

static void draw_eye(GContext *ctx, GPoint center, bool left) {
  graphics_context_set_fill_color(ctx, PAGE_WHITE);
  graphics_fill_circle(ctx, center, 9);
  graphics_context_set_fill_color(ctx, GColorLightGray);
  graphics_fill_circle(ctx, GPoint(center.x, center.y + 2), 6);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_circle(ctx, GPoint(center.x + (left ? 2 : -2), center.y + 3), 4);
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, GPoint(center.x + (left ? 3 : -1), center.y + 1), 1);
}

static void draw_book(GContext *ctx) {
  graphics_context_set_fill_color(ctx, BOOK_BLACK);
  graphics_fill_rect(ctx, GRect(28, 166, 91, 58), 3, GCornerBottomLeft | GCornerBottomRight);

  // The cover is held open toward Kody.
  GPoint cover[] = { GPoint(28, 166), GPoint(119, 166), GPoint(111, 178), GPoint(28, 174) };
  GPathInfo cover_info = { .num_points = 4, .points = cover };
  GPath *cover_path = gpath_create(&cover_info);
  gpath_draw_filled(ctx, cover_path);
  gpath_destroy(cover_path);

  if (s_logo) {
    graphics_context_set_compositing_mode(ctx, GCompOpSet);
    graphics_draw_bitmap_in_rect(ctx, s_logo, GRect(33, 179, 80, 24));
  }
}

static void canvas_update(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  graphics_context_set_fill_color(ctx, GColorBlack);
  graphics_fill_rect(ctx, bounds, 0, GCornerNone);

  // Kody's huge, deliberately edge-cropped silhouette.
  graphics_context_set_fill_color(ctx, KODY_BLUE);
  graphics_fill_circle(ctx, GPoint(151, 154), 74);
  graphics_fill_rect(ctx, GRect(78, 150, 122, 74), 0, GCornerNone);
  graphics_fill_circle(ctx, GPoint(170, 82), 8);
  graphics_fill_circle(ctx, GPoint(191, 91), 7);

  // Flatten the bottom and right edge into the display crop.
  graphics_fill_rect(ctx, GRect(96, 193, 104, 35), 0, GCornerNone);

  draw_eye(ctx, GPoint(128, 121), true);
  draw_eye(ctx, GPoint(145, 122), false);

  // Single straight brow, nose, mouth, and trademark buck teeth.
  stroke(ctx, GColorBlack, 3);
  graphics_draw_line(ctx, GPoint(116, 106), GPoint(154, 106));
  graphics_draw_line(ctx, GPoint(132, 135), GPoint(141, 135));
  graphics_draw_line(ctx, GPoint(136, 136), GPoint(136, 142));
  graphics_context_set_fill_color(ctx, PAGE_WHITE);
  graphics_fill_rect(ctx, GRect(132, 141, 5, 8), 0, GCornerNone);
  graphics_fill_rect(ctx, GRect(138, 141, 5, 8), 0, GCornerNone);
  stroke(ctx, GColorBlack, 1);
  graphics_draw_line(ctx, GPoint(137, 141), GPoint(137, 149));

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

  s_logo = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_KODE_LOGO);
  s_canvas = layer_create(bounds);
  layer_set_update_proc(s_canvas, canvas_update);
  layer_add_child(root, s_canvas);

  s_time_layer = text_layer_create(GRect(0, 25, bounds.size.w, 52));
  text_layer_set_background_color(s_time_layer, GColorClear);
  text_layer_set_text_color(s_time_layer, PAGE_WHITE);
  text_layer_set_font(s_time_layer, fonts_get_system_font(FONT_KEY_LECO_42_NUMBERS));
  text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
  layer_add_child(root, text_layer_get_layer(s_time_layer));
  update_time();
}

static void window_unload(Window *window) {
  text_layer_destroy(s_time_layer);
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
