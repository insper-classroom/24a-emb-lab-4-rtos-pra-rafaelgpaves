/*
 * LED blink with FreeRTOS
 */
#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>

#include "ssd1306.h"
#include "gfx.h"

#include "pico/stdlib.h"
#include <stdio.h>

const uint BTN_1_OLED = 28;
const uint BTN_2_OLED = 26;
const uint BTN_3_OLED = 27;

const uint LED_1_OLED = 20;
const uint LED_2_OLED = 21;
const uint LED_3_OLED = 22;

const uint BTN_ECHO = 17;
const uint BTN_TRIG = 18;

QueueHandle_t xQueueTime;
QueueHandle_t xQueueDistance;
SemaphoreHandle_t xSemaphoreTrigger;

void btn_callback(uint gpio, uint32_t events) {
    if (events == 0x4) { // fall edge
        if (gpio == BTN_ECHO) {
            uint64_t ultima_medicao = to_us_since_boot(get_absolute_time());
            xQueueSendFromISR(xQueueTime, &ultima_medicao, 0);
        }
    }
    if (events == 0x8) {
        if (gpio == BTN_ECHO) {
            uint64_t ultima_medicao = to_us_since_boot(get_absolute_time());
            xQueueSendFromISR(xQueueTime, &ultima_medicao, 0);
        }
    }
}

void trigger_task() {
    gpio_init(BTN_TRIG);
    gpio_set_dir(BTN_TRIG, GPIO_OUT);

    // gpio_put(TRIG_BTN_PIN, 1);
    // sleep_us(10);
    // gpio_put(TRIG_BTN_PIN, 0);
    // alarm_id_t alarm = add_alarm_in_ms(1000, alarm_callback, NULL, false);

    while (true) {
        gpio_put(BTN_TRIG, 1);
        sleep_us(10);
        gpio_put(BTN_TRIG, 0);

        vTaskDelay(pdMS_TO_TICKS(900));

        xSemaphoreGive(xSemaphoreTrigger);
    }
}

void echo_task() {
    gpio_init(BTN_ECHO);
    gpio_set_dir(BTN_ECHO, GPIO_IN);
    gpio_set_irq_enabled_with_callback(
        BTN_ECHO, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &btn_callback);

    uint64_t medicao;
    uint64_t ultima_medicao;
    int vezes_medidas = 0;

    int v_som = 340;

    while (true) {

        // if (to_us_since_boot(get_absolute_time()) - ultima_medicao > 1000000) {
        //     ultima_medicao = to_us_since_boot(get_absolute_time());
        //     vezes_medidas = 0;
        //     int dist = 0;
        //     xQueueSend(xQueueDistance, &dist, 0);
        // }
        if (xQueueReceiveFromISR(xQueueTime, &medicao, 0)) {
            if (!vezes_medidas) {
                vezes_medidas++;
                ultima_medicao = medicao;
            } else {
                vezes_medidas = 0;
                int dist = v_som * (medicao - ultima_medicao) / 20000.0;
                xQueueSend(xQueueDistance, &dist, 0);
                ultima_medicao = to_us_since_boot(get_absolute_time());
            }
        }
        // xQueueSend(xQueueDistance, &v_som, 0);
        
    }
}

void oled1_btn_led_init(void) {
    gpio_init(LED_1_OLED);
    gpio_set_dir(LED_1_OLED, GPIO_OUT);

    gpio_init(LED_2_OLED);
    gpio_set_dir(LED_2_OLED, GPIO_OUT);

    gpio_init(LED_3_OLED);
    gpio_set_dir(LED_3_OLED, GPIO_OUT);

    gpio_init(BTN_1_OLED);
    gpio_set_dir(BTN_1_OLED, GPIO_IN);
    gpio_pull_up(BTN_1_OLED);

    gpio_init(BTN_2_OLED);
    gpio_set_dir(BTN_2_OLED, GPIO_IN);
    gpio_pull_up(BTN_2_OLED);

    gpio_init(BTN_3_OLED);
    gpio_set_dir(BTN_3_OLED, GPIO_IN);
    gpio_pull_up(BTN_3_OLED);
}

void oled_task() {
    printf("Inicializando Driver\n");
    ssd1306_init();

    printf("Inicializando GLX\n");
    ssd1306_t disp;
    gfx_init(&disp, 128, 32);

    printf("Inicializando btn and LEDs\n");
    oled1_btn_led_init();

    int dist;
    while (true) {
        if (xQueueReceive(xQueueDistance, &dist, 0) && xSemaphoreTake(xSemaphoreTrigger, pdMS_TO_TICKS(1)) == pdTRUE) {
            char string[50] = "ERRO";
            if (dist > 0) {
                sprintf(string, "Dist: %d cm", dist);
            }
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, string);
            gfx_draw_line(&disp, 15, 27, 15 + dist/4,
                          27);
            gfx_show(&disp);
        }
    }
}

void oled1_demo_1(void *p) {
    printf("Inicializando Driver\n");
    ssd1306_init();

    printf("Inicializando GLX\n");
    ssd1306_t disp;
    gfx_init(&disp, 128, 32);

    printf("Inicializando btn and LEDs\n");
    oled1_btn_led_init();

    char cnt = 15;
    while (1) {

        if (gpio_get(BTN_1_OLED) == 0) {
            cnt = 15;
            gpio_put(LED_1_OLED, 0);
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, "LED 1 - ON");
            gfx_show(&disp);
        } else if (gpio_get(BTN_2_OLED) == 0) {
            cnt = 15;
            gpio_put(LED_2_OLED, 0);
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, "LED 2 - ON");
            gfx_show(&disp);
        } else if (gpio_get(BTN_3_OLED) == 0) {
            cnt = 15;
            gpio_put(LED_3_OLED, 0);
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, "LED 3 - ON");
            gfx_show(&disp);
        } else {

            gpio_put(LED_1_OLED, 1);
            gpio_put(LED_2_OLED, 1);
            gpio_put(LED_3_OLED, 1);
            gfx_clear_buffer(&disp);
            gfx_draw_string(&disp, 0, 0, 1, "PRESSIONE ALGUM");
            gfx_draw_string(&disp, 0, 10, 1, "BOTAO");
            gfx_draw_line(&disp, 15, 27, cnt,
                          27);
            vTaskDelay(pdMS_TO_TICKS(50));
            if (++cnt == 112)
                cnt = 15;

            gfx_show(&disp);
        }
    }
}

void oled1_demo_2(void *p) {
    printf("Inicializando Driver\n");
    ssd1306_init();

    printf("Inicializando GLX\n");
    ssd1306_t disp;
    gfx_init(&disp, 128, 32);

    printf("Inicializando btn and LEDs\n");
    oled1_btn_led_init();

    while (1) {

        gfx_clear_buffer(&disp);
        gfx_draw_string(&disp, 0, 0, 1, "Mandioca");
        gfx_show(&disp);
        vTaskDelay(pdMS_TO_TICKS(150));

        gfx_clear_buffer(&disp);
        gfx_draw_string(&disp, 0, 0, 2, "Batata");
        gfx_show(&disp);
        vTaskDelay(pdMS_TO_TICKS(150));

        gfx_clear_buffer(&disp);
        gfx_draw_string(&disp, 0, 0, 4, "Inhame");
        gfx_show(&disp);
        vTaskDelay(pdMS_TO_TICKS(150));
    }
}

int main() {
    stdio_init_all();

    xQueueTime = xQueueCreate(32, sizeof(int));
    xQueueDistance = xQueueCreate(32, sizeof(int));
    xSemaphoreTrigger = xSemaphoreCreateBinary();

    // xTaskCreate(oled1_demo_2, "Demo 2", 4095, NULL, 1, NULL);
    xTaskCreate(trigger_task, "Trigger Task", 4095, NULL, 1, NULL);
    xTaskCreate(echo_task, "Echo Task", 4095, NULL, 1, NULL);
    xTaskCreate(oled_task, "Oled Task", 4095, NULL, 1, NULL);

    vTaskStartScheduler();

    while (true)
        ;
}
