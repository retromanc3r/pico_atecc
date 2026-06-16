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

// Constants for ATECC608A device
#define SLOT_CONFIG_SIZE      (32u)         // 16 slots * 2 bytes each
#define CONFIG_ZONE_SIZE      (128u)       // Size of the configuration zone
#define ATCA_SERIAL_NUM_SIZE  (9u)         // Serial number size
#define TOTAL_READS           (32u)        // 128 bytes total, 4 bytes per read

// Opcodes for ATECC608A command set
#define ATCA_CHECKMAC           ((uint8_t)0x28)  // CheckMac command op-code
#define ATCA_DERIVE_KEY         ((uint8_t)0x1C)  // DeriveKey command op-code
#define ATCA_INFO               ((uint8_t)0x30)  // Info command op-code
#define ATCA_GENDIG             ((uint8_t)0x15)  // GenDig command op-code
#define ATCA_GENKEY             ((uint8_t)0x40)  // GenKey command op-code
#define ATCA_HMAC               ((uint8_t)0x11)  // HMAC command op-code
#define ATCA_LOCK               ((uint8_t)0x17)  // Lock command op-code
#define ATCA_MAC                ((uint8_t)0x08)  // MAC command op-code
#define ATCA_NONCE              ((uint8_t)0x16)  // Nonce command op-code
#define ATCA_PAUSE              ((uint8_t)0x01)  // Pause command op-code
#define ATCA_PRIVWRITE          ((uint8_t)0x46)  // PrivWrite command op-code
#define ATCA_RANDOM             ((uint8_t)0x1B)  // Random command op-code
#define ATCA_READ               ((uint8_t)0x02)  // Read command op-code
#define ATCA_SIGN               ((uint8_t)0x41)  // Sign command op-code
#define ATCA_UPDATE_EXTRA       ((uint8_t)0x20)  // UpdateExtra command op-code
#define ATCA_VERIFY             ((uint8_t)0x45)  // Verify command op-code
#define ATCA_WRITE              ((uint8_t)0x12)  // Write command op-code
#define ATCA_ECDH               ((uint8_t)0x43)  // ECDH command op-code
#define ATCA_COUNTER            ((uint8_t)0x24)  // Counter command op-code
#define ATCA_SHA                ((uint8_t)0x47)  // SHA command op-code
#define ATCA_AES                ((uint8_t)0x51)  // AES command op-code
#define ATCA_KDF                ((uint8_t)0x56)  // KDF command op-code
#define ATCA_IDLE               ((uint8_t)0x02)  // Idle command op-code
#define RANDOM_SEED_UPDATE      ((uint8_t)0x00) // Random mode for automatic seed update
#define SLOT_CONFIG_START       ((uint8_t)0x20) // SlotConfig starts at byte offset 32 (0x20)
#define LOCK_ZONE_CONFIG        ((uint8_t)0x00) // Lock Config Zone
#define LOCK_ZONE_DATA          ((uint8_t)0x01) // Lock Data Zone
#define LOCK_ZONE_DATA_SLOT     ((uint8_t)0x02) // Lock Data Slot

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