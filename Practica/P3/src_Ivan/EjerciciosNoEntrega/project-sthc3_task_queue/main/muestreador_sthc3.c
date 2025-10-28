
#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "shtc3.h"
#include "freertos/queue.h"

#define STACK_SIZE 3072
#define STEP_MUESTREO 3000

static const char *TAG = "STHC3_Muestreador";

//-- Structs
typedef struct
{
    float temp_value;
    float hum_value;
}SesnsorMsg;

typedef struct
{
    uint32_t sampling_time;
    QueueHandle_t *writeQueue;
}MuestreadorCfg;



shtc3_t sensor;
i2c_master_bus_handle_t bus_handle;

void init_i2c(void) {
    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = 8,
        .sda_io_num = 10,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &bus_handle));

   shtc3_init(&sensor, bus_handle, 0x70);
}


void muestreador_task( void * pvParameters ){

    while (true){

        // Take params -- TODO
        MuestreadorCfg *cfg = (MuestreadorCfg *)pvParameters;
        
        // Init struct
        SesnsorMsg msg;
        float temp;
        float hum;

        // Get sensor data and fill struct
        shtc3_get_temp_and_hum(&sensor, &temp, &hum);
        msg.temp_value = temp;
        msg.hum_value = hum;

        // Send to queue
        if (xQueueSendToBack(*(cfg->writeQueue), &msg, ( TickType_t ) 100 ) != pdPASS ){
            ESP_LOGE(TAG, "Can't write in queue");
        }
        vTaskDelay(cfg->sampling_time / portTICK_PERIOD_MS);
    }
}


void app_main(void)
{
    // I2C init
    init_i2c();

    // Queue init
    QueueHandle_t sensorDataQueue = xQueueCreate( 10, sizeof( SesnsorMsg ) );

    // Define "muestreador" task
    TaskHandle_t muestreadorTaskHandle = NULL;
    MuestreadorCfg task_cfg = {
        .sampling_time = STEP_MUESTREO,
        .writeQueue = &sensorDataQueue
    };
    xTaskCreate(muestreador_task, "Muestreador", STACK_SIZE, (void *) &task_cfg, 5, &muestreadorTaskHandle);
    configASSERT( muestreadorTaskHandle );

    // Show sensor value saved
    while (true){
        SesnsorMsg recv_msg;
        if( xQueueReceive( sensorDataQueue, &( recv_msg ), ( TickType_t ) 100 ) != pdPASS ){
            ESP_LOGE(TAG, "Can't read from queue (Empty)"); // 100 ticks with empty queue
        }
        else{
            ESP_LOGI(TAG, "Sensor Values -- Temp: %f  HUM: %f", recv_msg.temp_value, recv_msg.hum_value);
        }
    }
}
