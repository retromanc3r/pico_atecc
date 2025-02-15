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

// ATECC608 OPCodes and Constants
#define SLOT_CONFIG_START   0x20  // SlotConfig starts at byte offset 32 (0x20)
#define SLOT_CONFIG_SIZE    32    // 16 slots * 2 bytes each
#define CONFIG_ZONE_SIZE    128   // Size of the configuration zone
#define LOCK_ZONE_CONFIG    0x00  // Lock Config Zone
#define LOCK_ZONE_DATA      0x01  // Lock Data Zone
#define LOCK_ZONE_DATA_SLOT 0x02  // Lock Data Slot
#define SERIAL_NUMBER_SIZE  9     // Serial number size
#define TOTAL_READS         32    // 128 bytes total, 4 bytes per read
#define OP_READ             0x02  // Read command op-code
#define OP_IDLE             0x02  // IDLE command op-code
#define OP_RANDOM           0x1B  // Random command op-code
#define OP_SHA              0x47  // SHA command op-code
#define OP_AES_ENCRYPT      0x51  // AES Encrypt command op-code
#define OP_AES_DECRYPT      0x55  // AES Decrypt command op-code

// I2C Communication
int hal_i2c_send(uint8_t *txdata, size_t txlength);
int hal_i2c_receive(uint8_t *rxdata, size_t rxlength);

bool send_atecc_command(uint8_t opcode, uint8_t param1, uint16_t param2, const uint8_t *data, size_t data_len);
bool receive_atecc_response(uint8_t *buffer, size_t length, bool full_response);

bool send_idle_command();
bool wake_atecc_device();

#endif // ATECC_HAL_H