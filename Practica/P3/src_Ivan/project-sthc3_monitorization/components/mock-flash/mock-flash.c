#include "mock-flash.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

static const char *TAG = "BUFFER";
static CircularBuffer buffer;

esp_err_t mock_flash_init(size_t capacity) {

    buffer.buffer = (uint8_t*)malloc(capacity);
    if (!buffer.buffer) {
        return ESP_ERR_NO_MEM;
    }
    buffer.capacity = capacity;
    buffer.head = 0;
    buffer.tail = 0;
    ESP_LOGI(TAG, "Buffer correctamente inicializado.");
    return ESP_OK;
}

esp_err_t writeToFlash(void* data, size_t size) {
    
    size_t availableSpace = buffer.capacity - getDataLeft(buffer);

    if (size > availableSpace) {
        ESP_LOGE(TAG, "Tamaño del dato mayor al tamaño del buffer.");
        return ESP_ERR_INVALID_SIZE;
    }

    size_t bytesToEnd = buffer.capacity - buffer.head;
    if (bytesToEnd >= size) {
        memcpy(buffer.buffer + buffer.head, data, size);
        ESP_LOGI(TAG, "Dato correctamente almacenado");
    } else {
        ESP_LOGI(TAG, "Tamaño del dato excede el espacio libre, sobreescribiendo en la cabeza.");
        memcpy(buffer.buffer + buffer.head, data, bytesToEnd);
        memcpy(buffer.buffer, (uint8_t*)data + bytesToEnd, size - bytesToEnd);
        ESP_LOGI(TAG, "Dato correctamente almacenado");
    }

    buffer.head = (buffer.head + size) % buffer.capacity;

    return ESP_OK;
}

void* readFromFlash(size_t size) {

    if (size > getDataLeft(buffer)) {
        ESP_LOGI(TAG, "No hay suficientes datos para leer.");
        return NULL;
    }

    void* data = malloc(size);
    if (!data) {
        ESP_LOGI(TAG, "No se pudo asignar memoria para el dato.");
        return NULL;
    }

    size_t bytesToEnd = buffer.capacity - buffer.tail;
    if (bytesToEnd >= size) {
        memcpy(data, buffer.buffer + buffer.tail, size);
    } else {
        memcpy(data, buffer.buffer + buffer.tail, bytesToEnd);
        memcpy((uint8_t*)data + bytesToEnd, buffer.buffer, size - bytesToEnd);
    }
    ESP_LOGI(TAG, "Dato leído correctamente.");
    buffer.tail = (buffer.tail + size) % buffer.capacity;

    return data;
}

size_t getDataLeft() {
    return (buffer.head >= buffer.tail) ? (buffer.head - buffer.tail) : (buffer.capacity - (buffer.tail - buffer.head));
}

void mock_flash_destroy() {
    
    free(buffer.buffer);
    buffer.buffer = NULL;
    buffer.capacity = 0;
    buffer.head = 0;
    buffer.tail = 0;
    ESP_LOGI(TAG, "Buffer correctamente eliminado.");
}
