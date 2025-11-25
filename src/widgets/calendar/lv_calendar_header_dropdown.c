/**
 * @file lv_calendar_obj_dropdown.c
 *
 */

/*********************
 *      INCLUDES
 *********************/
#include "../../core/lv_obj_class_private.h"
#include "lv_calendar_header_dropdown.h"
#if LV_USE_CALENDAR && LV_USE_CALENDAR_HEADER_DROPDOWN

#include "lv_calendar.h"
#include "../dropdown/lv_dropdown.h"
#include "../../layouts/flex/lv_flex.h"
#include <time.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void my_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj);
static void year_event_cb(lv_event_t * e);
static void month_event_cb(lv_event_t * e);
static void value_changed_event_cb(lv_event_t * e);
static void generate_year_list(char * buffer, size_t buffer_size);

/**********************
 *  STATIC VARIABLES
 **********************/

const lv_obj_class_t lv_calendar_header_dropdown_class = {
    .base_class = &lv_obj_class,
    .width_def = LV_PCT(100),
    .height_def = LV_SIZE_CONTENT,
    .constructor_cb = my_constructor,
    .name = "lv_calendar_header_dropdown",
};

static const char * month_list ="January\nFebruary\nMarch\nApril\nMay\nJune\nJuly\nAugust\nSeptember\nOctober\nNovember\nDecember";
static char year_list_buffer[128];

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_obj_t * lv_calendar_add_header_dropdown(lv_obj_t * parent)
{
    lv_obj_t * obj = lv_obj_class_create_obj(&lv_calendar_header_dropdown_class, parent);
    lv_obj_class_init_obj(obj);

    return obj;
}

void lv_calendar_header_dropdown_set_year_list(lv_obj_t * parent, const char * years_list)
{
    /* Search for the header dropdown */
    lv_obj_t * header = lv_obj_get_child_by_type(parent, 0, &lv_calendar_header_dropdown_class);
    if(NULL == header) {
        /* Header not found */
        return;
    }

    /* Search for the year dropdown
     * Index is 0 because in the header dropdown constructor the year dropdown (year_dd)
     * is the first created child of the header */
    const int32_t year_dropdown_index = 0;
    lv_obj_t * year_dropdown = lv_obj_get_child_by_type(header, year_dropdown_index, &lv_dropdown_class);
    if(NULL == year_dropdown) {
        /* year dropdown not found */
        return;
    }

    lv_dropdown_clear_options(year_dropdown);
    lv_dropdown_set_options(year_dropdown, years_list);

    lv_obj_invalidate(parent);
}

/**********************
 *  STATIC FUNCTIONS
 **********************/

static void my_constructor(const lv_obj_class_t * class_p, lv_obj_t * obj)
{
    LV_TRACE_OBJ_CREATE("begin");

    LV_UNUSED(class_p);

    lv_obj_t * calendar = lv_obj_get_parent(obj);
    lv_obj_move_to_index(obj, 0);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
    //create year list
    generate_year_list(year_list_buffer, sizeof(year_list_buffer));

    lv_obj_t * year_dd = lv_dropdown_create(obj);
    lv_dropdown_set_options(year_dd, year_list_buffer);
    lv_obj_add_event_cb(year_dd, year_event_cb, LV_EVENT_VALUE_CHANGED, calendar);
    lv_obj_set_flex_grow(year_dd, 1);

    lv_obj_t * month_dd = lv_dropdown_create(obj);
    lv_dropdown_set_options(month_dd, month_list);
    lv_obj_add_event_cb(month_dd, month_event_cb, LV_EVENT_VALUE_CHANGED, calendar);
    lv_obj_set_flex_grow(month_dd, 1);
    lv_obj_t * month_list_obj = lv_dropdown_get_list(month_dd);
    if(month_list_obj) {
        //set max height
        lv_obj_set_style_max_height(month_list_obj, 400, 0);
        LV_LOG_USER("Month list actual height: %d", lv_obj_get_height(month_list_obj));
    }

    
    lv_obj_add_event_cb(obj, value_changed_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    /*Refresh the drop downs*/
    lv_obj_send_event(obj, LV_EVENT_VALUE_CHANGED, NULL);
}

static void month_event_cb(lv_event_t * e)
{
    lv_obj_t * dropdown = lv_event_get_current_target(e);
    lv_obj_t * calendar = lv_event_get_user_data(e);

    uint32_t sel = lv_dropdown_get_selected(dropdown);

    const lv_calendar_date_t * d;
    d = lv_calendar_get_showed_date(calendar);
    lv_calendar_date_t newd = *d;
    newd.month = sel + 1;

    lv_calendar_set_month_shown(calendar, newd.year, newd.month);
}

static void year_event_cb(lv_event_t * e)
{
    lv_obj_t * dropdown = lv_event_get_current_target(e);
    lv_obj_t * calendar = lv_event_get_user_data(e);

    uint32_t sel = lv_dropdown_get_selected(dropdown);

    const lv_calendar_date_t * d;
    d = lv_calendar_get_showed_date(calendar);

    /* Get the first year on the options list
     * NOTE: Assumes the first 4 digits in the option list are numbers */
    const char * year_p = lv_dropdown_get_options(dropdown);
    const uint32_t year = (year_p[0] - '0') * 1000 + (year_p[1] - '0') * 100 + (year_p[2] - '0') * 10 +
                          (year_p[3] - '0');

    lv_calendar_date_t newd = *d;
    newd.year = year - sel;

    lv_calendar_set_month_shown(calendar, newd.year, newd.month);
}

static void value_changed_event_cb(lv_event_t * e)
{
    lv_obj_t * header = lv_event_get_current_target(e);
    lv_obj_t * calendar = lv_obj_get_parent(header);
    const lv_calendar_date_t * cur_date = lv_calendar_get_showed_date(calendar);

    lv_obj_t * year_dd = lv_obj_get_child(header, 0);

    /* Get the first year on the options list
     * NOTE: Assumes the first 4 digits in the option list are numbers */
    const char * year_p = lv_dropdown_get_options(year_dd);
    const uint32_t year = (year_p[0] - '0') * 1000 + (year_p[1] - '0') * 100 + (year_p[2] - '0') * 10 +
                          (year_p[3] - '0');

    lv_dropdown_set_selected(year_dd, year - cur_date->year, LV_ANIM_OFF);

    lv_obj_t * month_dd = lv_obj_get_child(header, 1);
    lv_dropdown_set_selected(month_dd, cur_date->month - 1, LV_ANIM_OFF);
}
static void generate_year_list(char * buffer, size_t buffer_size) 
{
    // current year
    time_t t = time(NULL);
    struct tm * tm_info = localtime(&t);
    int current_year = tm_info->tm_year + 1900;
    
    // current+5 ro current -1
    int start_year = current_year + 5;
    int end_year = current_year - 1;
    
    char * p = buffer;
    size_t remaining = buffer_size;
    
    for (int year = start_year; year >= end_year; year--) 
    {
        int written = snprintf(p, remaining, "%d", year);
        if (written < 0 || written >= remaining) {
            break; 
        }
        
        p += written;
        remaining -= written;
        
        // add \n
        if (year > end_year) 
        {
            if (remaining > 1) 
            {
                *p = '\n';
                p++;
                remaining--;
            } else {
                break;
            }
        }
    }
    // make sure last one with null
    if (buffer_size > 0) {
        buffer[buffer_size - 1] = '\0';
    }
}

#endif /*LV_USE_CALENDAR_HEADER_ARROW*/