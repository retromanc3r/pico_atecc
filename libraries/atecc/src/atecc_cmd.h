#ifndef ATECC_CMD_H
#define ATECC_CMD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "hal_pico_i2c.h"
#include "atecc_crc.h"

bool read_atecc_serial_number();
void generate_random_number_in_range(uint64_t min, uint64_t max);
bool compute_sha256_hash(const char *message);
bool read_slot_config(uint8_t slot);
bool generate_random_value(uint8_t length);
bool read_config_zone();
bool check_lock_status();
bool send_aes_command(uint8_t mode, uint8_t key_slot, const uint8_t *input_data);
bool send_nonce_command(uint8_t *random_out);
bool receive_aes_response(uint8_t *output_data);
bool aes_encrypt(const uint8_t *plaintext, uint8_t *ciphertext, uint8_t key_slot);
bool aes_decrypt(const uint8_t *ciphertext, uint8_t *plaintext, uint8_t key_slot);

#ifdef __cplusplus
}
#endif

#endif // ATECC_CMD_H