#include <esp_log.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/ledc.h"

#include "app_user.h"


/******************************************************************************************************
 *                                          Bell->PWM 配置                                 
 ******************************************************************************************************/
#define LEDC_LS_TIMER          LEDC_TIMER_1
#define LEDC_LS_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_LS_CH2_GPIO       GPIO_NUM_4
#define LEDC_LS_CH2_CHANNEL    LEDC_CHANNEL_2
#define LEDC_DUTY_RESOLUTION   LEDC_TIMER_10_BIT   
#define LEDC_PWM_MAX_DUTY      (uint16_t)pow(2, LEDC_DUTY_RESOLUTION)     // 把分辨率转换为十进制

#define LEDC_TEST_CH_NUM       (1)
#define LEDC_TEST_DUTY         (LEDC_PWM_MAX_DUTY/2)
#define LEDC_TEST_FADE_TIME    (3000)
 /*
    * Prepare individual configuration
    * for each channel of LED Controller
    * by selecting:
    * - controller's channel number
    * - output duty cycle, set initially to 0
    * - GPIO number where LED is connected to
    * - speed mode, either high or low
    * - timer servicing selected channel
    *   Note: if different channels use one timer,
    *         then frequency and bit_num of these channels
    *         will be the same
    */
ledc_channel_config_t ledc_channel[LEDC_TEST_CH_NUM] = {
    {
        .channel    = LEDC_LS_CH2_CHANNEL,
        .duty       = 0,
        .gpio_num   = LEDC_LS_CH2_GPIO,
        .speed_mode = LEDC_LS_MODE,
        .hpoint     = 0,
        .timer_sel  = LEDC_LS_TIMER
    },
    
};

void app_bell_contorl(uint16_t duty) 
{
    if (duty > LEDC_PWM_MAX_DUTY)  duty = LEDC_PWM_MAX_DUTY;
    ledc_set_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel, duty);
    ledc_update_duty(ledc_channel[0].speed_mode, ledc_channel[0].channel);
}

void app_bell_init(void)
{
    int ch = 0;
    /*
     * Prepare and set configuration of timers
     * that will be used by LED Controller
     */
    ledc_timer_config_t ledc_timer = {
        .duty_resolution = LEDC_DUTY_RESOLUTION,    // resolution of PWM duty
        .freq_hz = 4000,                            // frequency of PWM signal
        .speed_mode = LEDC_LS_MODE,                 // timer mode
        .timer_num = LEDC_LS_TIMER,                 // timer index
        .clk_cfg = LEDC_AUTO_CLK,                   // Auto select the source clock
    };
    // Set configuration of timer0 for high speed channels
    ledc_timer_config(&ledc_timer);

    // Set LED Controller with previously prepared configuration
    for (ch = 0; ch < LEDC_TEST_CH_NUM; ch++) {
        ledc_channel_config(&ledc_channel[ch]);
    }

    app_bell_contorl(0);

    // ledc_set_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel, LEDC_TEST_DUTY);
    // ledc_update_duty(ledc_channel[ch].speed_mode, ledc_channel[ch].channel);
}



/******************************************************************************************************
 *                                          LED 配置                                 
 ******************************************************************************************************/

#define LED1_IO    GPIO_NUM_26
#define LED2_IO    GPIO_NUM_25
#define LED3_IO    GPIO_NUM_33
#define LED4_IO    GPIO_NUM_32

#define LED_PIN_SEL  ((1ULL<<LED1_IO) | (1ULL<<LED2_IO) | (1ULL<<LED3_IO) | (1ULL<<LED4_IO))

#define LED_ON      0
#define LED_OFF     1

// channel: 通道；（0xff:全部）； state: 开关状态
void app_led_contorl(uint8_t channel, bool state)
{
    switch (channel)
    {
    case 0:
        gpio_set_level(LED1_IO, state);
        break;
    case 1:
        gpio_set_level(LED2_IO, state);
        break;
    case 2:
        gpio_set_level(LED3_IO, state);
        break;
    case 3:
        gpio_set_level(LED4_IO, state);
        break;
    default:
        gpio_set_level(LED1_IO, state);
        gpio_set_level(LED2_IO, state);
        gpio_set_level(LED3_IO, state);
        gpio_set_level(LED4_IO, state);
        break;
    }
}

void app_led_init(void)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = LED_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    // app_led_contorl(0xFF, LED_OFF);
}


/******************************************************************************************************
 *                                          Relay 配置                                 
 ******************************************************************************************************/
#define RELAY1_IO    GPIO_NUM_16
#define RELAY2_IO    GPIO_NUM_17
#define RELAY3_IO    GPIO_NUM_5
#define RELAY4_IO    GPIO_NUM_18

#define RELAY_PIN_SEL  ((1ULL<<RELAY1_IO) | (1ULL<<RELAY2_IO) | (1ULL<<RELAY3_IO) | (1ULL<<RELAY4_IO))

#define RELAY_ON      1
#define RELAY_OFF     0

// channel: 通道；（0xff:全部）； state: 开关状态
void app_relay_contorl(uint8_t channel, bool state)
{
    switch (channel)
    {
    case 0:
        gpio_set_level(RELAY1_IO, state);
        break;
    case 1:
        gpio_set_level(RELAY2_IO, state);
        break;
    case 2:
        gpio_set_level(RELAY3_IO, state);
        break;
    case 3:
        gpio_set_level(RELAY4_IO, state);
        break;
    default:
        gpio_set_level(RELAY1_IO, state);
        gpio_set_level(RELAY2_IO, state);
        gpio_set_level(RELAY3_IO, state);
        gpio_set_level(RELAY4_IO, state);
        break;
    }

    app_led_contorl(channel, !state);   // 因为LED与Relay控制电平是反的
}

void app_relay_init(void)
{
    gpio_config_t io_conf;
    //disable interrupt
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
    //set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    //bit mask of the pins that you want to set,e.g.GPIO18/19
    io_conf.pin_bit_mask = RELAY_PIN_SEL;
    //disable pull-down mode
    io_conf.pull_down_en = 0;
    //disable pull-up mode
    io_conf.pull_up_en = 0;
    //configure GPIO with the given settings
    gpio_config(&io_conf);

    app_relay_contorl(0xFF, RELAY_OFF);
}



void app_user_init(void)
{
    app_led_init();

    app_relay_init();

    app_bell_init();
}