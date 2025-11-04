#ifndef MOCK_FLASH_H
#define MOCK_FLASH_H

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"

typedef struct {
    uint8_t* buffer;
    size_t capacity;
    size_t head;
    size_t tail;
} CircularBuffer;

esp_err_t mock_flash_init(size_t capacity);
esp_err_t writeToFlash(void* data, size_t size);
void* readFromFlash(size_t size);
size_t getDataLeft();
void mock_flash_destroy();

#endif // MOCK_FLASH_H
