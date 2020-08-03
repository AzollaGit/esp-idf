/* Touch Pad Interrupt Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "esp_log.h"
 

#include "driver/touch_pad.h"
#include "soc/rtc_periph.h"
#include "soc/sens_periph.h"

#include "app_touch.h"
#include "app_user.h"

static const char* TAG = "APP_Touch";
#define TOUCH_THRESH_NO_USE   (0)
#define TOUCH_THRESH_PERCENT  (80)
#define TOUCHPAD_FILTER_TOUCH_PERIOD (10)

#define TOUCH_CHANNEL_NUM   4   

const uint8_t touch_channel[TOUCH_CHANNEL_NUM] = {TOUCH_PAD_NUM4, TOUCH_PAD_NUM5, TOUCH_PAD_NUM6, TOUCH_PAD_NUM7};

static bool s_pad_activated[TOUCH_PAD_MAX];

/*
  Read values sensed at all available touch pads.
  Use 2 / 3 of read value as the threshold
  to trigger interrupt when the pad is touched.
  Note: this routine demonstrates a simple way
  to configure activation threshold for the touch pads.
  Do not touch any pads when this routine
  is running (on application start).
 */
static void tp_example_set_thresholds(void)
{
    uint16_t touch_value;

    for (int i = 0; i < TOUCH_CHANNEL_NUM; i++) {
        // read filtered value
        touch_pad_read_filtered(touch_channel[i], &touch_value);
        ESP_LOGI(TAG, "test init: touch pad [%d] val is %d", touch_channel[i], touch_value);
        // set interrupt threshold.
        ESP_ERROR_CHECK(touch_pad_set_thresh(touch_channel[i], touch_value * 2 / 3));

    }
}

/*
  Check if any of touch pads has been activated
  by reading a table updated by rtc_intr()
  If so, then print it out on a serial monitor.
  Clear related entry in the table afterwards

  In interrupt mode, the table is updated in touch ISR.

  In filter mode, we will compare the current filtered value with the initial one.
  If the current filtered value is less than 80% of the initial value, we can
  regard it as a 'touched' event.
  When calling touch_pad_init, a timer will be started to run the filter.
  This mode is designed for the situation that the pad is covered
  by a 2-or-3-mm-thick medium, usually glass or plastic.
  The difference caused by a 'touch' action could be very small, but we can still use
  filter mode to detect a 'touch' event.
 */
static void tp_example_read_task(void *pvParameter)
{
    static bool state = 0;
    static uint16_t touch_value = 0;
    while (1) {
        
        for (int i = 0; i < TOUCH_PAD_MAX; i++) {
            if (s_pad_activated[i] == true) {
                if (!(touch_value & (1 << i))) {
                    ESP_LOGI(TAG, "T%d activated!", i);
                    touch_value |= (1 << i);
                    switch (i) {    // 把通道转化一下
                    case 4: i = 4; break;
                    case 5: i = 2; break;
                    case 6: i = 1; break;
                    case 7: i = 3; break;
                    }

                    ESP_LOGI(TAG, "channel = %d", i);

                    app_bell_contorl(1024/2);

                    lightChannelValue ^= (1 << i);
                    bool state = (lightChannelValue & (1 << i)) ? 1 : 0;
                    app_relay_contorl(i - 1, state);
                    vTaskDelay(200 / portTICK_PERIOD_MS);
                 
                    app_bell_contorl(1024);
                }
                // Clear information on pad activation
                s_pad_activated[i] = false;
            } else {
                touch_value &= ~(1<<i);
            }
        }
        
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

/*
  Handle an interrupt triggered when a pad is touched.
  Recognize what pad has been touched and save it in a table.
 */
static void tp_example_rtc_intr(void * arg)
{
    uint32_t pad_intr = touch_pad_get_status();
    //clear interrupt
    touch_pad_clear_status();
    for (int i = 0; i < TOUCH_PAD_MAX; i++) {
        if ((pad_intr >> i) & 0x01) {
            s_pad_activated[i] = true;
        }
    }
}

/*
 * Before reading touch pad, we need to initialize the RTC IO.
 */
static void tp_example_touch_pad_init(void)
{
    for (int i = 0; i< TOUCH_CHANNEL_NUM; i++) {
        //init RTC IO and mode for touch pad.
        touch_pad_config(touch_channel[i], TOUCH_THRESH_NO_USE);
    }
}

void app_touch_init(void)
{
    // Initialize touch pad peripheral, it will start a timer to run a filter
    ESP_LOGI(TAG, "Initializing touch pad"); 

    touch_pad_init();
    // If use interrupt trigger mode, should set touch sensor FSM mode at 'TOUCH_FSM_MODE_TIMER'.
    touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
    // Set reference voltage for charging/discharging
    // For most usage scenarios, we recommend using the following combination:
    // the high reference valtage will be 2.7V - 1V = 1.7V, The low reference voltage will be 0.5V.
    touch_pad_set_voltage(TOUCH_HVOLT_2V7, TOUCH_LVOLT_0V5, TOUCH_HVOLT_ATTEN_1V);
    // Init touch pad IO
    tp_example_touch_pad_init();
    // Initialize and start a software filter to detect slight change of capacitance.
    touch_pad_filter_start(TOUCHPAD_FILTER_TOUCH_PERIOD);
    // Set thresh hold
    tp_example_set_thresholds();
    // Register touch interrupt ISR
    touch_pad_isr_register(tp_example_rtc_intr, NULL);
    //interrupt mode, enable touch interrupt
    touch_pad_intr_enable();
    //clear interrupt
    touch_pad_clear_status();
    // Start a task to show what pads have been touched
    xTaskCreate(&tp_example_read_task, "touch_pad_read_task", 2048, NULL, 5, NULL);
}

// cd examples/peripherals/touch_pad_interrupt/