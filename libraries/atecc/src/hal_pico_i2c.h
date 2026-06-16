#ifndef ATECC_HAL_H
#define ATECC_HAL_H

#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "atecc_crc.h"

// ATECC608 I2C Configuration
#define I2C_ADDR  0x60  // ATECC608 I2C address
#define I2C_PORT  i2c0  // I2C Port
#define I2C_SDA_PIN 4   // SDA Pin
#define I2C_SCL_PIN 5   // SCL Pin

// I2C Communication
int hal_i2c_send(uint8_t *txdata, size_t txlength);
int hal_i2c_receive(uint8_t *rxdata, size_t rxlength);

bool send_atecc_command(uint8_t opcode, uint8_t param1, uint16_t param2, const uint8_t *data, size_t data_len);
bool receive_atecc_response(uint8_t *buffer, size_t length, bool full_response);

bool send_idle_command();
bool wake_atecc_device();

#endif // ATECC_HAL_H