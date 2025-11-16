
#include <stdio.h>
#include <math.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "driver/i2c_master.h"
#include "icm42670.h"
#include "led_strip.h"

static const char *TAG = "ICM42670_Led";

#define I2C_SCL_PIN CONFIG_I2C_MASTER_SCL       /*!< GPIO number used for I2C master clock */
#define I2C_SDA_PIN CONFIG_I2C_MASTER_SDA       /*!< GPIO number used for I2C master data  */
#define RGB_LED_GPIO CONFIG_RGB_LED_GPIO
#define LED_STRIP_LED_COUNT CONFIG_RGB_LEDS_COUNT
#define STACK_SIZE 3072
#define QUEUE_DEF_SIZE 20
#define MAX_RGB_VALUE 100

typedef enum{
    FACE_UP,
    FACE_DOWN
} soc_position_t;
char* soc_positio2str[2] = {
    "FACE_UP",
    "FACE_DOWN"
};

static i2c_master_bus_handle_t i2c_handle;
static icm42670_handle_t icm42670_h;
const TickType_t sample_period_ms = 100;

QueueHandle_t posDataQueue;

// ---------------- Utils ---------------- //
static void i2c_bus_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_NUM_0,
        .scl_io_num = I2C_SCL_PIN,
        .sda_io_num = I2C_SDA_PIN,
        .clk_source = I2C_CLK_SRC_DEFAULT,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_handle));
}

static void i2c_sensor_icm42670_init(void)
{
    esp_err_t ret;

    i2c_bus_init();
    ret = icm42670_create(i2c_handle, ICM42670_I2C_ADDRESS, &icm42670_h);

    /* Configuration of the accelerometer and gyroscope */
    const icm42670_cfg_t imu_cfg = {
        .acce_fs = ACCE_FS_2G,
        .acce_odr = ACCE_ODR_400HZ,
        .gyro_fs = GYRO_FS_2000DPS,
        .gyro_odr = GYRO_ODR_400HZ,
    };
    ret = icm42670_config(icm42670_h, &imu_cfg);

    ESP_ERROR_CHECK(ret);
}

static soc_position_t get_soc_z_pos(float z_val){

    soc_position_t pos;
    if (z_val > 0) {
        pos = FACE_UP;
    } else {
        pos = FACE_DOWN;
    }
    
    return pos;
}

led_strip_handle_t configure_led(void){
    // LED strip general initialization, according to your led board design
    led_strip_config_t strip_config = {
        .strip_gpio_num = RGB_LED_GPIO, // The GPIO that connected to the LED strip's data line
        .max_leds = LED_STRIP_LED_COUNT,      // The number of LEDs in the strip,
        .led_model = LED_MODEL_WS2812,        // LED strip model
        // set the color order of the strip: GRB
        .color_component_format = {
            .format = {
                .r_pos = 1, // red is the second byte in the color data
                .g_pos = 0, // green is the first byte in the color data
                .b_pos = 2, // blue is the third byte in the color data
                .num_components = 3, // total 3 color components
            },
        },
        .flags = {
            .invert_out = false, // don't invert the output signal
        }
    };

    // LED strip backend configuration: SPI
    led_strip_spi_config_t spi_config = {
        .clk_src = SPI_CLK_SRC_DEFAULT, // different clock source can lead to different power consumption
        .spi_bus = SPI2_HOST,           // SPI bus ID
        .flags = {
            .with_dma = true, // Using DMA can improve performance and help drive more LEDs
        }
    };

    // LED Strip object handle
    led_strip_handle_t led_strip;
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
    ESP_LOGI(TAG, "Created LED strip object with SPI backend");
    return led_strip;
}

esp_err_t set_gradient_led_color(float value, led_strip_handle_t led_handle)
{
    esp_err_t ret = ESP_OK;

    // Compute gradient -> clamp value between -1 and 1
    if (value > 1.0f) {
        value = 1.0f;
    } else if (value < -1.0f) {
        value = -1.0f;
    }

    // Compute gradient -> FACE_UP = GREEN  / FACE_DOWN = RED
    float norm_value = (value + 1.0f) / 2.0f; // normalized value
    int r = (int)((1.0f - norm_value) * 255.0f); 
    int g = (int)(norm_value * 255.0f);
    int b = 0;

    // Set led color
    for (int i = 0; i < LED_STRIP_LED_COUNT; i++) {
        ret = led_strip_set_pixel(led_handle, i, r, g, b);
        if (ret != ESP_OK) {
            return ret;
        }
    }
    ret = led_strip_refresh(led_handle);
    return ret;
}


// ---------------- Tasks ---------------- //
void extract_pitch_pos( void * pvParameters ){
    
    icm42670_value_t acc;
    float acc_z;
    while (1) {
        icm42670_get_acce_value(icm42670_h, &acc);
        acc_z = acc.z;
        if (xQueueSendToBack(posDataQueue, &acc_z, portMAX_DELAY ) != pdPASS ){
            ESP_LOGE(TAG, "Can't write in queue");
        }
        vTaskDelay(pdMS_TO_TICKS(sample_period_ms));
    }
}


void app_main(void)
{
    esp_err_t ret;

    // Led configuration
    led_strip_handle_t led_strip = configure_led();

    // Bus I2C and ICM42670 sensor configuration
    i2c_sensor_icm42670_init();

    // Set accelerometer and gyroscope to ON 
    ret = icm42670_acce_set_pwr(icm42670_h, ACCE_PWR_LOWNOISE);
    ret = icm42670_gyro_set_pwr(icm42670_h, GYRO_PWR_LOWNOISE);

    // Position extract task and queue init
    posDataQueue = xQueueCreate( QUEUE_DEF_SIZE, sizeof( float ) );
    TaskHandle_t positionTaskHandle = NULL;
    xTaskCreate(extract_pitch_pos, "GetPosition", STACK_SIZE, NULL, 5, &positionTaskHandle);
    
    // Main while
    float acc_z_value;
    soc_position_t z_pos = FACE_UP;
    while (1) {
        if(xQueueReceive(posDataQueue, &acc_z_value, portMAX_DELAY)){
            z_pos = get_soc_z_pos(acc_z_value);
            ESP_LOGI(TAG, "AccZ:%.2f  Status:%s", acc_z_value, soc_positio2str[z_pos]);
            ret = set_gradient_led_color(acc_z_value, led_strip);
        }
    }

    icm42670_delete(icm42670_h);
    ret = i2c_del_master_bus(i2c_handle);
    ESP_ERROR_CHECK(ret);
}