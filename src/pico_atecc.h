#ifndef ATECC_PICO_H
#define ATECC_PICO_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// ATECC608 I2C Configuration
#define I2C_ADDR  0x60  // ATECC608 I2C address
#define I2C_PORT  i2c0  // I2C Port
#define I2C_SDA_PIN 4   // SDA Pin
#define I2C_SCL_PIN 5   // SCL Pin

// ATECC608 Constants
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
int i2c_read_blocking_safe(uint8_t *response, size_t expected_length);
bool send_command(uint8_t opcode, uint8_t param1, uint16_t param2, const uint8_t *data, size_t data_len);
bool get_response(uint8_t *buffer, size_t length, bool full_response);
bool send_idle_command();
bool wake_device();

// Cryptographic Functions (SHA, AES, CRC)
void calc_crc16_ccitt(size_t length, const uint8_t *data, uint8_t *crc_le);
void compute_crc(uint8_t length, uint8_t *data, uint8_t *crc);
bool validate_crc(uint8_t *response, size_t length);
void debug_crc_mismatch(uint8_t *data, size_t length, uint8_t *expected_crc);
bool compute_sha256_hash(const char *message);

// AES Encryption & Decryption
bool send_aes_command(uint8_t mode, uint8_t key_slot, const uint8_t *input_data);
bool receive_aes_response(uint8_t *output_data);
bool aes_encrypt(const uint8_t *plaintext, uint8_t *ciphertext, uint8_t key_slot);
bool aes_decrypt(const uint8_t *ciphertext, uint8_t *plaintext, uint8_t key_slot);

// Random Number Generation
bool send_nonce_command(uint8_t *random_out);
bool generate_random_value(uint8_t length);
void generate_random_number_in_range(uint64_t min, uint64_t max);
uint64_t map_random_to_range(uint8_t *random_bytes, uint64_t min, uint64_t max);

// Device & Configuration Management
bool read_serial_number();
bool read_slot_config(uint8_t slot);
bool read_config_zone();
bool check_lock_status();
#endif  // ATECC_PICO_H
