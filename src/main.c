#include "main.h"

GBitmap *bitmap_matrix;
//GBitmap *bitmap_pause;
//GBitmap *bitmap_play;
GBitmap *bitmap_add;
//GBitmap *bitmap_settings;
GBitmap *bitmap_delete;
GBitmap *bitmap_edit;
GBitmap *bitmap_adjust;
GBitmap *bitmap_reset;
GBitmap *bitmap_minus;
GBitmap *bitmap_tick;

Settings settings={MODE_COUNT_UP, false /*alarm*/, true /*sort*/};
static bool JS_ready = false;
static bool data_loaded_from_watch = false;

// *****************************************************************************************************
// MESSAGES
// *****************************************************************************************************

#define KEY_MEDICATIONS   100
#define KEY_MODE          1
#define KEY_ALARM         2
#define KEY_SORT          3

static void inbox_received_handler(DictionaryIterator *iter, void *context) {
  LOG("Inbox received...");
  JS_ready = true;
  Tuple *tuple_t;

  if (!data_loaded_from_watch) {
    LOG("Loading settings from phone...");
    tuple_t=dict_find(iter, KEY_MODE);   if (tuple_t) settings.Mode = tuple_t->value->int32;
    tuple_t=dict_find(iter, KEY_ALARM); if (tuple_t) settings.Alarm = tuple_t->value->int8 > 0; // convert int to boolean
    tuple_t=dict_find(iter, KEY_SORT); if (tuple_t) settings.Sort = tuple_t->value->int8 > 0; // convert int to boolean
    jobs_list_read_dict(iter, KEY_MEDICATIONS);
  }
  
  LOG("Inbox processed.");
  main_menu_highlight_top();
}

static void send_settings_to_phone() {
  if (!JS_ready) return;
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  int dummy_int;
  
  dummy_int=settings.Mode;   dict_write_int(iter, KEY_MODE, &dummy_int, sizeof(int), true);
  dummy_int=settings.Alarm;  dict_write_int(iter, KEY_ALARM, &dummy_int, sizeof(int), true);
  dummy_int=settings.Sort;   dict_write_int(iter, KEY_SORT, &dummy_int, sizeof(int), true);
  jobs_list_write_dict(iter, KEY_MEDICATIONS);

  dict_write_end(iter);
  app_message_outbox_send();
}

// *****************************************************************************************************
// WAKEUP
// *****************************************************************************************************

#define WAKEUP_REASON 0
#define WAKEUP_ID_KEY 0
static WakeupId wakeup_id;

void main_wakeup_set() {
  wakeup_cancel_all();
  if (!settings.Alarm || jobs_count==0) return;
  time_t next_alarm = jobs_get_next_wakeup_time();
  if (next_alarm) wakeup_id = wakeup_schedule(next_alarm, WAKEUP_REASON, true);
  //LOG("now=%ld, wakeup=%ld, diff=%ld", time(NULL), jobs_get_next_wakeup_time(), jobs_get_next_wakeup_time()-time(NULL));
}

// *****************************************************************************************************
// DATA STORAGE
// *****************************************************************************************************

void main_save_data(void) {
  persist_write_int(STORAGE_KEY_VERSION, CURRENT_STORAGE_VERSION);
  persist_write_data(STORAGE_KEY_SETTINGS, &settings, sizeof(Settings));
  if (settings.Sort) jobs_list_sort();
  jobs_list_save(STORAGE_KEY_FIRST_MED);
  send_settings_to_phone();
  main_wakeup_set();
}

static void main_load_data(void) {
  uint8_t stored_version = persist_read_int(STORAGE_KEY_VERSION); // defaults to 0 if key is missing
  
  if (stored_version) {
    data_loaded_from_watch = true;
    persist_read_data(STORAGE_KEY_SETTINGS, &settings, sizeof(Settings));
    jobs_list_load(STORAGE_KEY_FIRST_MED, stored_version);
    if (stored_version < CURRENT_STORAGE_VERSION)  {
      LOG("Saving data in new version");
      main_save_data();
    }
  }
}

// *****************************************************************************************************
// MAIN
// *****************************************************************************************************

void init(void) {
  main_load_data();
  bitmap_matrix=gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ICON_MATRIX);
  //bitmap_pause=gbitmap_create_as_sub_bitmap(bitmap_matrix, ICON_RECT_PAUSE);
  //bitmap_play=gbitmap_create_as_sub_bitmap(bitmap_matrix, ICON_RECT_PLAY);
  bitmap_add=gbitmap_create_as_sub_bitmap(bitmap_matrix, ICON_RECT_ADD);
  //bitmap_settings=gbitmap_create_as_sub_bitmap(bitmap_matrix, ICON_RECT_SETTINGS);
  bitmap_delete=gbitmap_create_as_sub_bitmap(bitmap_matrix, ICON_RECT_DELETE);
  bitmap_edit=gbitmap_create_as_sub_bitmap(bitmap_matrix, ICON_RECT_EDIT);
  bitmap_adjust=gbitmap_create_as_sub_bitmap(bitmap_matrix, ICON_RECT_ADJUST);
  bitmap_reset=gbitmap_create_as_sub_bitmap(bitmap_matrix, ICON_RECT_RESET);
  bitmap_minus=gbitmap_create_as_sub_bitmap(bitmap_matrix, ICON_RECT_MINUS);
  bitmap_tick=gbitmap_create_as_sub_bitmap(bitmap_matrix, ICON_RECT_TICK);
  main_menu_show();
  
  app_message_register_inbox_received(inbox_received_handler);
  app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

void deinit(void) {
  main_menu_hide();
  main_wakeup_set();
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}