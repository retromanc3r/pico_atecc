#include "atecc_cmd.h"
#include "hal_pico_i2c.h"

/**
 * @brief Reads the serial number from an ATECC device.
 *
 * This function reads the serial number from an ATECC device by sending read commands
 * and receiving responses in parts. It then prints the serial number in hexadecimal format.
 *
 * @return true if the serial number is successfully read, false otherwise.
 */
bool read_atecc_serial_number()
{
    uint8_t serial[9];
    uint8_t last_response[2];

    send_atecc_command(OP_READ, 0x00, 0x0000, NULL, 0);
    sleep_ms(5);
    if (!receive_atecc_response(&serial[0], 4, true))
        return false;

    send_atecc_command(OP_READ, 0x00, 0x0002, NULL, 0);
    sleep_ms(5);
    if (!receive_atecc_response(&serial[4], 5, true))
        return false;

    send_atecc_command(OP_READ, 0x00, 0x0003, NULL, 0);
    sleep_ms(5);
    if (!receive_atecc_response(last_response, 2, false))
        return false;
    serial[8] = last_response[0];

    printf("🆔 Serial Number: ");
    for (int i = 0; i < 9; i++)
    {
        printf("%02X", serial[i]);
    }
    printf("\n");

    return true;
}

/**
 * @brief Maps an array of 8 random bytes to a specified range.
 *
 * This function takes an array of 8 random bytes and maps the resulting
 * 64-bit random value to a specified range between min and max (inclusive).
 *
 * @param random_bytes An array of 8 random bytes.
 * @param min The minimum value of the desired range.
 * @param max The maximum value of the desired range.
 * @return A 64-bit random value mapped to the specified range.
 */
uint64_t map_random_to_range(uint8_t *random_bytes, uint64_t min, uint64_t max) {
    uint64_t random_value = 0;
    for (int i = 0; i < 8; i++) {
        random_value = (random_value << 8) | random_bytes[i];
    }
    return min + (random_value % (max - min + 1));
}

/**
 * @brief Generates a random number within a specified range using the ATECC608A cryptographic co-processor.
 *
 * This function utilizes the ATECC608A cryptographic co-processor to generate a random number.
 * It then maps the generated random value to the given range [min, max] and prints the result.
 *
 * @param min The minimum value of the range.
 * @param max The maximum value of the range.
 */
void generate_random_number_in_range(uint64_t min, uint64_t max) {
    uint8_t response[35];

    send_atecc_command(OP_RANDOM, 0x00, 0x0000, NULL, 0);
    sleep_ms(23);

    if (hal_i2c_receive(response, sizeof(response)) != (int)sizeof(response)) {
        printf("❌ ERROR: Failed to read random number response\n");
        return;
    }

    if (response[0] != 0x23) {
        printf("❌ ERROR: Invalid response length byte!\n");
        return;
    }

    // Map random value to range
    uint64_t mapped_value = map_random_to_range(&response[1], min, max);
    printf("🎲 Random Number (Mapped to Range %llu-%llu): %llu\n", min, max, mapped_value);
}

/**
 * @brief Generates a random value of a specified length using the ATECC608A cryptographic co-processor.
 *
 * This function utilizes the ATECC608A cryptographic co-processor to generate a random value of a specified length.
 * It then prints the generated random value in hexadecimal format.
 *
 * @param length The length of the random value to generate.
 * @return true if the random value is successfully generated, false otherwise.
 */
bool generate_random_value(uint8_t length) {
    uint8_t response[35];

    send_atecc_command(OP_RANDOM, 0x00, 0x0000, NULL, 0);
    sleep_ms(23);

    if (hal_i2c_receive(response, sizeof(response)) != (int)sizeof(response)) {
        printf("❌ ERROR: Failed to read random number response\n");
        return false;
    }

    if (response[0] != 0x23) {
        printf("❌ ERROR: Invalid response length byte!\n");
        return false;
    }

    printf("🎲 Random Value (HEX): ");
    for (int i = 4; i < length; i++) {
        printf("%02X ", response[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    return true;
}

/**
 * @brief Computes the SHA-256 hash of a given message using the ATECC608A cryptographic co-processor.
 *
 * This function computes the SHA-256 hash of a given message using the ATECC608A cryptographic co-processor.
 * It sends the message in blocks of 64 bytes to the device and reads the resulting hash value.
 *
 * @param message The message for which to compute the SHA-256 hash.
 * @return true if the SHA-256 hash is successfully computed, false otherwise.
 */
bool compute_sha256_hash(const char *message) {
    size_t message_len = strlen(message);
    size_t offset = 0;
    uint8_t response[35];

    // Step 1: Start SHA computation
    if (!send_atecc_command(OP_SHA, 0x00, 0x0000, NULL, 0)) {
        printf("❌ ERROR: SHA Start command failed!\n");
        return false;
    }
    sleep_ms(5);

    // Step 2: Process full 64-byte blocks (SHA Update)
    while (message_len - offset >= 64) {
        if (!send_atecc_command(OP_SHA, 0x01, 0x0000, (const uint8_t *)&message[offset], 64)) {
            printf("❌ ERROR: SHA Update command failed!\n");
            return false;
        }
        offset += 64;
        sleep_ms(5);
    }

    // Step 3: Process the final block (SHA End)
    if (!send_atecc_command(OP_SHA, 0x02, message_len - offset, (const uint8_t *)&message[offset], message_len - offset)) {
        printf("❌ ERROR: SHA End command failed!\n");
        return false;
    }
    sleep_ms(5);

    // Step 4: Read the response
    int res = hal_i2c_receive(response, sizeof(response));
    if (res < 0 || response[0] != 0x23) {
        printf("❌ ERROR: Failed to retrieve SHA-256 digest!\n");
        return false;
    }

    // Step 5: Validate CRC
    if (!validate_crc(response, sizeof(response))) {
        printf("❌ ERROR: CRC check failed for response!\n");
        return false;
    }

    // Step 6: Print Hash
    printf("🔢 SHA-256: ");
    for (int i = 1; i <= 32; i++) {
        printf("%02X", response[i]);
    }
    printf("\n");

    return true;
}

/**
 * @brief Reads the configuration of a specific slot from the ATECC608A device over I2C bus.
 *
 * This function reads the configuration of a specific slot from the ATECC608A device by sending read commands
 * and receiving responses. It then prints the configuration data in hexadecimal format.
 *
 * @param slot The slot number for which to read the configuration.
 * @return true if the slot configuration is successfully read, false otherwise.
 */
bool read_slot_config(uint8_t slot) {
    uint8_t response[4];
    printf("🔎 Checking Slot %d Configuration...\n", slot);

    if (!send_atecc_command(OP_READ, 0x00, slot, NULL, 0)) {
        printf("❌ ERROR: Failed to send slot config read command!\n");
        return false;
    }
    sleep_ms(20);

    if (hal_i2c_receive(response, 4) != 4) {
        printf("❌ ERROR: Failed to read slot configuration!\n");
        return false;
    }

    printf("🔎 Slot %d Config Data: %02X %02X %02X %02X\n", slot, response[0], response[1], response[2], response[3]);
    return true;
}

/**
 * @brief Reads the configuration data of all slots from the ATECC608A device over I2C bus.
 *
 * This function reads the configuration data of all slots from the ATECC608A device by sending read commands
 * and receiving responses. It then prints the configuration data in hexadecimal format.
 *
 * @return true if the configuration data is successfully read, false otherwise.
 */
bool read_config_zone() {
    uint8_t config_data[128] = {0};  // Full 128-byte config zone
    uint8_t response[5];  // Read buffer (5 bytes to include index byte)
    int config_index = 0;  // Tracks where to store valid bytes

    printf("🔎 Reading Configuration Data...\n");

    for (uint8_t i = 0; i < TOTAL_READS; i++) {
        if (!send_atecc_command(OP_READ, 0x00, i, NULL, 0)) {
            printf("❌ ERROR: Failed to send read command for index %d!\n", i);
            return false;
        }
        sleep_ms(20);

        if (hal_i2c_receive(response, 5) != 5) {  // Read 5 bytes
            printf("❌ ERROR: Failed to read configuration for index %d!\n", i);
            return false;
        }

        // Store only the last 4 bytes, skipping the index byte (0x07)
        if (config_index + 4 <= 128) {
            config_data[config_index]     = response[1];
            config_data[config_index + 1] = response[2];
            config_data[config_index + 2] = response[3];
            config_data[config_index + 3] = response[4];
            config_index += 4;  // Move forward by exactly 4 bytes
        }
    }

    // Print formatted config data (16 bytes per row)
    for (int i = 0; i < 128; i++) {
        printf("%02X ", config_data[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    return true;
}

/**
 * @brief Checks the lock status of the ATECC608A device.
 *
 * This function checks the lock status of the ATECC608A device by sending read commands
 * and receiving responses. It then prints the lock status of the device.
 *
 * @return true if the lock status is successfully checked, false otherwise.
 */
bool check_lock_status() {
    printf("🔍 Checking ATECC608A Lock Status...\n");

    uint8_t response[5];  // Read 5 bytes to ensure full data capture
    uint8_t expected_address = 0x15;  // Correct address for lock bytes

    // 🔹 Send read command for lock status at word address 0x15
    if (!send_atecc_command(OP_READ, 0x00, expected_address, NULL, 0)) {
        printf("❌ ERROR: Failed to send lock status read command!\n");
        return false;
    }

    sleep_ms(23);
    if (hal_i2c_receive(response, sizeof(response)) != 5) { 
        printf("❌ ERROR: Failed to read lock status response!\n");
        return false;
    }

   // Print raw lock status response (5 bytes) for debugging purposes
    printf("🔐 Raw Lock Status Response: %02X %02X %02X %02X %02X\n",
           response[0], response[1], response[2], response[3], response[4]);

    // Extract correct lock bytes from response[3] and response[4]
    uint8_t lock_config = response[3];  // Byte 0x15 (Config Lock)
    uint8_t lock_value = response[4];   // Byte 0x16 (Data Lock)

    printf("🔒 Config Lock Status: %02X\n", lock_config);
    printf("🔒 Data Lock Status: %02X\n", lock_value);

    // 🔐 Determine Lock Status
    if (lock_config == 0x00 && lock_value == 0x00) {
        printf("🔒 Chip is **FULLY LOCKED** (Config & Data).\n");
        return true;
    } 
    else if (lock_config == 0x55 && lock_value == 0x55) {
        printf("🔓 Chip is **UNLOCKED**.\n");
        return true;
    } 
    else if (lock_config == 0x00 && lock_value == 0x55) {
        printf("⚠️ Chip is **PARTIALLY LOCKED** (Config Locked, Data Open).\n");
        return true;
    } 
    else {
        printf("❓ **UNKNOWN LOCK STATE**: Unexpected lock values, possible read error.\n");
        return false;
    }
}

/**
 * @brief Sends a Nonce command to the ATECC608A device to generate a random nonce.
 *
 * This function sends a Nonce command to the ATECC608A device to generate a random nonce.
 * It then reads the 32-byte response and stores the random nonce in the provided buffer.
 *
 * @param random_out The buffer to store the generated random nonce.
 * @return true if the Nonce command is successfully sent and the random nonce is generated, false otherwise.
 */
bool send_nonce_command(uint8_t *random_out) {
    uint8_t command[8];
    uint8_t response[32];  // Nonce response

    printf("🔹 Sending Nonce Command...\n");

    command[0] = 0x03;  // Packet header
    command[1] = 0x07;  // Length (7 bytes)
    command[2] = 0x16;  // Nonce Opcode
    command[3] = 0x00;  // Mode (random nonce)
    command[4] = 0x00;  // Zero (reserved)
    command[5] = 0x00;  // Zero (reserved)

    uint8_t computed_crc[2];
    compute_crc(6, command, computed_crc);
    command[6] = computed_crc[0];
    command[7] = computed_crc[1];

    int res = hal_i2c_send((uint8_t*)&command, sizeof(command));
    if (res != sizeof(command)) {
        printf("❌ ERROR: I2C write failed for Nonce Command.\n");
        return false;
    }

    sleep_ms(5);

    // Read 32-byte response
    res = hal_i2c_receive(response, sizeof(response));
    if (res != sizeof(response)) {
        printf("❌ ERROR: Failed to read Nonce response.\n");
        return false;
    }

    int actual_len = sizeof(response) - 1;  // Subtract first byte
    if (actual_len > 32) actual_len = 32;   // Limit to 32 bytes
    memcpy(random_out, response + 1, actual_len);
    printf("🔹 Nonce Generated.\n");

    return true;
}

/**
 * @brief Sends an AES command to the ATECC608A device over I2C bus.
 *
 * This function constructs an AES command packet and sends it to the ATECC608A device
 * over the I2C bus. It computes the CRC for the command packet and sends it to the device.
 *
 * @param mode The AES mode (0x00 for encrypt, 0x01 for decrypt).
 * @param key_slot The key slot to use for encryption/decryption.
 * @param input_data The input data to encrypt/decrypt.
 * @return true if the AES command is successfully sent, false otherwise.
 */
bool send_aes_command(uint8_t mode, uint8_t key_slot, const uint8_t *input_data) {
    uint8_t command[24];
    command[0] = 0x03;  // Packet header
    command[1] = 0x17;  // Length (23 bytes)
    command[2] = 0x51;  // AES Opcode
    command[3] = mode;  // AES Mode
    command[4] = key_slot & 0xFF;  // Key Slot LSB
    command[5] = 0x00;  // Key Slot MSB
    memcpy(&command[6], input_data, 16); // AES Plaintext

    // **Compute CRC for first 21 bytes**
    uint8_t computed_crc[2];
    compute_crc(21, command + 1, computed_crc);
    command[22] = computed_crc[0];
    command[23] = computed_crc[1];

    // **Send the AES command over I2C**
    int res = hal_i2c_send((uint8_t*)&command, sizeof(command));
    if (res != sizeof(command)) {
        printf("❌ ERROR: I2C write failed (expected %zu bytes, got %d)\n", sizeof(command), res);
        return false;
    }
    return true;
}

/**
 * @brief Receives an AES response from the ATECC608A device over I2C bus.
 *
 * This function reads the AES response from the ATECC608A device over the I2C bus.
 * It computes the CRC for the response and validates it before returning the response data.
 *
 * @param output_data The buffer to store the AES response data.
 * @return true if the AES response is successfully received, false otherwise.
 */
bool receive_aes_response(uint8_t *output_data) {
    uint8_t response[19];

    int res = hal_i2c_receive(response, sizeof(response));
    if (res != sizeof(response)) {
        printf("❌ ERROR: I2C Read failed (Expected %d bytes, got %d)\n", (int)sizeof(response), res);
        return false;
    }

    uint8_t crc[2];
    compute_crc(17, response, crc);
    if (crc[0] != response[17] || crc[1] != response[18]) {
        printf("❌ ERROR: CRC mismatch! Expected: %02X %02X, Got: %02X %02X\n",
               crc[0], crc[1], response[17], response[18]);
        return false;
    }

    memcpy(output_data, &response[1], 16);
    return true;
}

/**
 * @brief Encrypts a plaintext message using AES 128-bit encryption.
 *
 * This function encrypts a plaintext message using AES 128-bit encryption
 * with the specified key slot on the ATECC608A device. It sends the AES command
 * to the device and receives the encrypted ciphertext in response.
 *
 * @param plaintext The plaintext message to encrypt.
 * @param ciphertext The buffer to store the encrypted ciphertext.
 * @param key_slot The key slot where the AES 128-bit key is stored.
 * @return true if the plaintext message is successfully encrypted, false otherwise.
 */
bool aes_encrypt(const uint8_t *plaintext, uint8_t *ciphertext, uint8_t key_slot) {
    send_idle_command();
    if (!wake_atecc_device()) {
        printf("❌ Failed to wake device.\n");
        return false;
    }

    sleep_ms(5);

    if (!send_aes_command(0x00, key_slot, plaintext)) {
        printf("❌ Failed to send AES encrypt command.\n");
        return false;
    }

    sleep_ms(5);

    if (!receive_aes_response(ciphertext)) {
        printf("❌ Failed to receive AES encrypt response.\n");
        return false;
    }

    return true;
}

/**
 * @brief Decrypts a ciphertext message using AES 128-bit decryption.
 *
 * This function decrypts a ciphertext message using AES 128-bit decryption
 * with the specified key slot on the ATECC608A device. It sends the AES command
 * to the device and receives the decrypted plaintext in response.
 *
 * @param ciphertext The ciphertext message to decrypt.
 * @param plaintext The buffer to store the decrypted plaintext.
 * @param key_slot The key slot where the AES 128-bit key is stored.
 * @return true if the ciphertext message is successfully decrypted, false otherwise.
 */
bool aes_decrypt(const uint8_t *ciphertext, uint8_t *plaintext, uint8_t key_slot) {
    send_idle_command();
    if (!wake_atecc_device()) {
        printf("❌ Failed to wake device.\n");
        return false;
    }

    sleep_ms(5);  // Ensure device is awake

    if (!send_aes_command(0x01, key_slot, ciphertext)) {
        printf("❌ Failed to send AES decrypt command.\n");
        return false;
    }

    sleep_ms(5);  // Ensure enough time for AES processing

    if (!receive_aes_response(plaintext)) {
        printf("❌ Failed to receive AES decrypt response.\n");
        return false;
    }

    return true;
}
