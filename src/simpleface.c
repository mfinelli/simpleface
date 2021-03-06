#include <pebble.h>

#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1
#define FORECAST_API_KEY 2
#define FORECAST_UNITS 3

static Window *s_main_window;
static TextLayer *s_time_layer;
static TextLayer *s_weather_layer;
static TextLayer *s_battery_layer;
static TextLayer *s_date_layer;
static int s_battery_level;

static void update_battery() {
    static char s_buffer[8];
    snprintf(s_buffer, sizeof(s_buffer), "%d%%", s_battery_level);
    text_layer_set_text(s_battery_layer, s_buffer);
}

static void update_time() {
    // Get a tm structure
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);

    // Write the current hours and minutes into a buffer.
    static char s_buffer[8];
    strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ?
            "%H:%M" : "%I:%M", tick_time);

    // Write the current date into a buffer.
    static char s_date_buffer[16];
    strftime(s_date_buffer, sizeof(s_date_buffer), "%a %d %b", tick_time);

    // Display this time on the text layer.
    text_layer_set_text(s_time_layer, s_buffer);
    text_layer_set_text(s_date_layer, s_date_buffer);
}

static void battery_callback(BatteryChargeState state) {
    // Record the new battery level.
    s_battery_level = state.charge_percent;
    update_battery();
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_time();

    // Get weather update every thirty minutes.
    if(tick_time->tm_min % 30 == 0) {
        // Begin dictionary.
        DictionaryIterator *iter;
        app_message_outbox_begin(&iter);

        // Add a key-value pair.
        dict_write_uint8(iter, 0, 0);

        // Send the message!
        app_message_outbox_send();
    }
}

static void main_window_load(Window *window) {
    // Get information about the window.
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    // Create the TextLayer with specific bounds.
    s_time_layer = text_layer_create(
            GRect(0, PBL_IF_ROUND_ELSE(58, 52), bounds.size.w, 50));
    s_weather_layer = text_layer_create(
            GRect(0, PBL_IF_ROUND_ELSE(125, 120), bounds.size.w, 25));
    s_battery_layer = text_layer_create(
            GRect(0, PBL_IF_ROUND_ELSE(15, 15), bounds.size.w, 20));
    s_date_layer = text_layer_create(
            GRect(0, PBL_IF_ROUND_ELSE(35, 35), bounds.size.w, 20));

    // Improve the layout to be more like a watchface.
    text_layer_set_background_color(s_time_layer, GColorClear);
    text_layer_set_text_color(s_time_layer, GColorBlack);
    text_layer_set_font(s_time_layer,
            fonts_get_system_font(FONT_KEY_BITHAM_42_BOLD));
    text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);

    // Style the weather text.
    text_layer_set_background_color(s_weather_layer, GColorClear);
    text_layer_set_text_color(s_weather_layer, GColorBlack);
    text_layer_set_font(s_weather_layer,
            fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
    text_layer_set_text(s_weather_layer, "Loading...");

    // Style battery level.
    text_layer_set_background_color(s_battery_layer, GColorClear);
    text_layer_set_text_color(s_battery_layer, GColorBlack);
    text_layer_set_font(s_battery_layer,
            fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text_alignment(s_battery_layer, GTextAlignmentCenter);
    text_layer_set_text(s_battery_layer, "...");

    // Style the date text.
    text_layer_set_background_color(s_date_layer, GColorClear);
    text_layer_set_text_color(s_date_layer, GColorBlack);
    text_layer_set_font(s_date_layer,
            fonts_get_system_font(FONT_KEY_GOTHIC_18));
    text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);

    // Add it as a child layer to the Window's root layer
    layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_battery_layer));
    layer_add_child(window_layer, text_layer_get_layer(s_date_layer));

    // Initialize the batter level.
    battery_callback(battery_state_service_peek());
}

static void main_window_unload(Window *window) {
    text_layer_destroy(s_time_layer);
}

static void inbox_received_callback(DictionaryIterator *iterator,
        void *context) {
    // Store incoming information.
    static char temperature_buffer[8];
    static char conditions_buffer[32];
    static char weather_layer_buffer[32];
    static int forecast_api_key;

    // Read tuples for data.
    Tuple *temp_tuple = dict_find(iterator, KEY_TEMPERATURE);
    Tuple *conditions_tuple = dict_find(iterator, KEY_CONDITIONS);
    Tuple *forecast_api_key_tuple = dict_find(iterator, FORECAST_API_KEY);
    Tuple *units_tuple = dict_find(iterator, FORECAST_UNITS);

    if(forecast_api_key_tuple && !(int)forecast_api_key_tuple->value->int8) {
        snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s",
                "No API Key!");
        text_layer_set_text(s_weather_layer, weather_layer_buffer);
    }


    // Iff all data is available then use it.
    else if(temp_tuple && conditions_tuple && units_tuple) {
        if(!strcmp(units_tuple->value->cstring, "si")) {
            snprintf(temperature_buffer, sizeof(temperature_buffer), "%d°C",
                    (int)temp_tuple->value->int32);
        } else {
            snprintf(temperature_buffer, sizeof(temperature_buffer), "%d°F",
                    (int)temp_tuple->value->int32);
        }

        snprintf(conditions_buffer, sizeof(conditions_buffer), "%s",
                conditions_tuple->value->cstring);

        // Assemble full string and display.
        snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s %s",
                temperature_buffer, conditions_buffer);
        text_layer_set_text(s_weather_layer, weather_layer_buffer);
    }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message Dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator,
        AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void init() {
    // Register for battery level updates.
    battery_state_service_subscribe(battery_callback);

    // Create main window element and assign to pointer.
    s_main_window = window_create();

    // Set handlers to manage the elements inside the Window.
    window_set_window_handlers(s_main_window, (WindowHandlers) {
            .load = main_window_load,
            .unload = main_window_unload
    });

    // Show the Window on the watch, with animated=true.
    window_stack_push(s_main_window, true);

    // Make sure time is displayed from the start.
    update_time();

    // Register with TickTimerService.
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);

    // Register callbacks
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);

    // Open AppMessage
    app_message_open(app_message_inbox_size_maximum(),
            app_message_outbox_size_maximum());
}

static void deinit() {
    window_destroy(s_main_window);
}

int main(void) {
    setlocale(LC_ALL, "");

    init();
    app_event_loop();
    deinit();
}
