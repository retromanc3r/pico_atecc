#include "pico_atecc.h"


int i2c_write_block(uint8_t *data, size_t length) {
    return i2c_write_blocking(I2C_PORT, I2C_ADDR, data, length, false);
}

int i2c_read_blocking_safe(uint8_t *response, size_t expected_length) {
    int res = i2c_read_blocking(I2C_PORT, I2C_ADDR, response, expected_length, false);
    if (res != expected_length) {
        printf("❌ ERROR: I2C read failed (expected %zu, got %d)\n", expected_length, res);
        return -1;
    }
    
    return res;
}

// ATECC608 Wake-up Sequence (I2C)
bool wake_device() {    
    uint8_t data = 0x00;
    uint8_t wake_response[4];

    // Send wakeup sequence with proper delays
    i2c_write_block(&data, sizeof(data));
    sleep_ms(1);

    int res = i2c_read_blocking_safe(wake_response, sizeof(wake_response));
    printf("🛰️ **Wake-up Response:** ");
    for (int i = 0; i < 4; i++) {
        printf("%02X ", wake_response[i]);
    }
    printf("\n");

    // Check if wake-up response matches expected value
    if (res > 0 && wake_response[0] == 0x04 && wake_response[1] == 0x11 &&
        wake_response[2] == 0x33 && wake_response[3] == 0x43) {
        printf("✅ Wake-up successful!\n");
        return true;
    } else {
        printf("❌ ERROR: Wake-up failed! Unexpected response.\n");
        return false;
    }
}

// Send a command to the ATECC608A device over I2C bus (with CRC)
bool send_command(uint8_t opcode, uint8_t param1, uint16_t param2, const uint8_t *data, size_t data_len) {
    uint8_t command[7 + data_len];
    command[0] = 0x07 + data_len;
    command[1] = opcode;
    command[2] = param1;
    command[3] = param2 & 0xFF;
    command[4] = (param2 >> 8) & 0xFF;

    if (data_len > 0) {
        memcpy(&command[5], data, data_len);
    }

    calc_crc16_ccitt(5 + data_len, command, &command[5 + data_len]);

    uint8_t full_command[8 + data_len];
    full_command[0] = 0x03;
    memcpy(&full_command[1], command, sizeof(command));

    return i2c_write_block((uint8_t*)&full_command, sizeof(full_command)) >= 0;
}

// Read the response from the ATECC608A device over I2C bus
bool get_response(uint8_t *buffer, size_t length, bool full_response) {
    uint8_t response[7];
    size_t read_length = full_response ? 7 : length + 1;
    
    if (i2c_read_blocking(I2C_PORT, I2C_ADDR, response, read_length, false) != (int)read_length) {
        printf("❌ ERROR: Failed to read response from ATECC608A\n");
        return false;
    }
    
    memcpy(buffer, &response[1], length);
    return true;
}

// Send an idle command to the ATECC608A device over I2C bus
bool send_idle_command() {
    uint8_t idle_cmd = OP_IDLE; // Idle command op-code
    int res = i2c_write_block((uint8_t*)&idle_cmd, sizeof(idle_cmd));

    if (res != sizeof(idle_cmd)) {
        printf("❌ ERROR: Failed to send idle command! (Expected %d, got %d)\n", (int)sizeof(idle_cmd), res);
        return false;
    }
    
    return true;
}

// Read the serial number from the ATECC608A device over I2C bus
bool read_serial_number() {
    uint8_t serial[9];
    uint8_t last_response[2];

    send_command(OP_READ, 0x00, 0x0000, NULL, 0);
    sleep_ms(5);
    if (!get_response(&serial[0], 4, true)) return false;

    send_command(OP_READ, 0x00, 0x0002, NULL, 0);
    sleep_ms(5);
    if (!get_response(&serial[4], 5, true)) return false;

    send_command(OP_READ, 0x00, 0x0003, NULL, 0);
    sleep_ms(5);
    if (!get_response(last_response, 2, false)) return false;
    serial[8] = last_response[0];

    printf("🆔 Serial Number: ");
    for (int i = 0; i < 9; i++) {
        printf("%02X", serial[i]);
    }
    printf("\n");

    return true;
}

// Map a random number to a specific range (min to max) using 64 bits 
uint64_t map_random_to_range(uint8_t *random_bytes, uint64_t min, uint64_t max) {
    uint64_t random_value = 0;
    for (int i = 0; i < 8; i++) {
        random_value = (random_value << 8) | random_bytes[i];
    }
    return min + (random_value % (max - min + 1));
}

// Generate a random number in a specific range using the ATECC608A
void generate_random_number_in_range(uint64_t min, uint64_t max) {
    uint8_t response[35];

    send_command(OP_RANDOM, 0x00, 0x0000, NULL, 0);
    sleep_ms(23);

    if (i2c_read_blocking(I2C_PORT, I2C_ADDR, response, sizeof(response), false) != (int)sizeof(response)) {
        printf("❌ ERROR: Failed to read random number response\n");
        return;
    }

    if (response[0] != 0x23) {
        printf("❌ ERROR: Invalid response length byte!\n");
        return;
    }

    uint64_t mapped_value = map_random_to_range(&response[1], min, max);

    printf("🎲 Random Number (Mapped to Range %llu-%llu): %llu\n", min, max, mapped_value);
}

// Generate a random value of specific length using the ATECC608A device over I2C bus
bool generate_random_value(uint8_t length) {
    uint8_t response[35];

    send_command(OP_RANDOM, 0x00, 0x0000, NULL, 0);
    sleep_ms(23);

    if (i2c_read_blocking(I2C_PORT, I2C_ADDR, response, sizeof(response), false) != (int)sizeof(response)) {
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
}

// Compute SHA-256 hash of a message using the ATECC608A device over I2C bus
bool compute_sha256_hash(const char *message) {
    size_t message_len = strlen(message);
    size_t offset = 0;
    uint8_t response[35];

    // Step 1: Start SHA computation
    if (!send_command(OP_SHA, 0x00, 0x0000, NULL, 0)) {
        printf("❌ ERROR: SHA Start command failed!\n");
        return false;
    }
    sleep_ms(5);

    // Step 2: Process full 64-byte blocks (SHA Update)
    while (message_len - offset >= 64) {
        if (!send_command(OP_SHA, 0x01, 0x0000, (const uint8_t *)&message[offset], 64)) {
            printf("❌ ERROR: SHA Update command failed!\n");
            return false;
        }
        offset += 64;
        sleep_ms(5);
    }

    // Step 3: Process the final block (SHA End)
    if (!send_command(OP_SHA, 0x02, message_len - offset, (const uint8_t *)&message[offset], message_len - offset)) {
        printf("❌ ERROR: SHA End command failed!\n");
        return false;
    }
    sleep_ms(5);

    // Step 4: Read the response
    int res = i2c_read_blocking_safe(response, sizeof(response));
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

// Read the configuration of a specific slot from the ATECC608A device over I2C bus
bool read_slot_config(uint8_t slot) {
    uint8_t response[4];
    printf("🔎 Checking Slot %d Configuration...\n", slot);

    if (!send_command(OP_READ, 0x00, slot, NULL, 0)) {
        printf("❌ ERROR: Failed to send slot config read command!\n");
        return false;
    }
    sleep_ms(20);

    if (i2c_read_blocking(I2C_PORT, I2C_ADDR, response, 4, false) != 4) {
        printf("❌ ERROR: Failed to read slot configuration!\n");
        return false;
    }

    printf("🔎 Slot %d Config Data: %02X %02X %02X %02X\n", slot, response[0], response[1], response[2], response[3]);
    return true;
}

// Read the configuration data of all slots from the ATECC608A device over I2C bus
bool read_config_zone() {
    uint8_t config_data[128] = {0};  // Full 128-byte config zone
    uint8_t response[5];  // Read buffer (5 bytes to include index byte)
    int config_index = 0;  // Tracks where to store valid bytes

    printf("🔎 Reading Configuration Data...\n");

    for (uint8_t i = 0; i < TOTAL_READS; i++) {
        if (!send_command(OP_READ, 0x00, i, NULL, 0)) {
            printf("❌ ERROR: Failed to send read command for index %d!\n", i);
            return false;
        }
        sleep_ms(20);

        if (i2c_read_blocking(I2C_PORT, I2C_ADDR, response, 5, false) != 5) {  // Read 5 bytes
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

// Check the lock status of the ATECC608A device over I2C bus
bool check_lock_status() {
    printf("🔍 Checking ATECC608A Lock Status...\n");

    uint8_t response[5];  // Read 5 bytes to ensure full data capture
    uint8_t expected_address = 0x15;  // Correct address for lock bytes

    // 🔹 Send read command for lock status at word address 0x15
    if (!send_command(OP_READ, 0x00, expected_address, NULL, 0)) {
        printf("❌ ERROR: Failed to send lock status read command!\n");
        return false;
    }

    sleep_ms(23);
    if (i2c_read_blocking(I2C_PORT, I2C_ADDR, response, 5, false) != 5) { 
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

// Send a Nonce command to the ATECC608A device over I2C bus
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

    int res = i2c_write_block((uint8_t*)&command, sizeof(command));
    if (res != sizeof(command)) {
        printf("❌ ERROR: I2C write failed for Nonce Command.\n");
        return false;
    }

    sleep_ms(5);

    // Read 32-byte response
    res = i2c_read_blocking(I2C_PORT, I2C_ADDR, response, sizeof(response), false);
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

// Receive AES response from the ATECC608A device over I2C bus
bool receive_aes_response(uint8_t *output_data) {
    uint8_t response[19];

    int res = i2c_read_blocking(I2C_PORT, I2C_ADDR, response, sizeof(response), false);
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

// Send an AES command to the ATECC608A device over I2C bus
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
    int res = i2c_write_block((uint8_t*)&command, sizeof(command));
    if (res != sizeof(command)) {
        printf("❌ ERROR: I2C write failed (expected %zu bytes, got %d)\n", sizeof(command), res);
        return false;
    }
    return true;
}

// AES 128-bit encryption using the ATECC608A device over I2C bus
bool aes_encrypt(const uint8_t *plaintext, uint8_t *ciphertext, uint8_t key_slot) {
    send_idle_command();
    if (!wake_device()) {
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

// AES 128-bit decryption using the ATECC608A device over I2C bus
bool aes_decrypt(const uint8_t *ciphertext, uint8_t *plaintext, uint8_t key_slot) {
    send_idle_command();
    if (!wake_device()) {
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

// Main function to test the ATECC608A device
int main() {
    stdio_init_all();
    i2c_init(I2C_PORT, 100 * 1000);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    gpio_pull_up(I2C_SCL_PIN);

    printf("📡 Initializing ATECC608A...\n");

    // Send a wake-up sequence
    if (!wake_device()) {
        printf("❌ ERROR: Failed to wake up ATECC608A\n");
        return 1;
    }
    
    // Read the serial number
    if (!read_serial_number()) {
        printf("❌ ERROR: Failed to read Serial Number\n");
        return 1;
    }

    // Generate a random number in a specific range
    generate_random_number_in_range(100, 65535);

    // Compute a SHA-256 hash
    if (!compute_sha256_hash("COLD WAR")) {
        printf(" ERROR: Failed to compute a SHA-256 hash\n");
        return 1;
    }
    
    // Read the configuration of a specific slot
    if (!read_slot_config(0x03)) {
        printf("❌ ERROR: Failed to read slot configuration\n");
        return 1;
    }

    // Generate a random value of specific length
    if (!generate_random_value(16)) {
        printf("❌ ERROR: Failed to generate random value\n");
        return 1;
    }
    
    // Read the configuration data of all slots
    if (!read_config_zone()) {
        printf("❌ ERROR: Failed to read configuration data\n");
        return 1;
    }

    // Read the configuration data of all slots and check the lock status
    if (!check_lock_status()) {
        printf("❌ ERROR: Failed to check lock status\n");
        return 1;
    }
    
    // This will fail if you have not setup the ATECC608A for AES
    uint8_t plaintext[16] = "Hello, AES!\0\0\0\0"; // Ensure 16 bytes
    uint8_t ciphertext[16];
    uint8_t decrypted_text[16];
    uint8_t key_slot = 0x03; // Slot where AES 128-bit key is stored

    printf("🔹 Plaintext: ");
    for (int i = 0; i < 16; i++) {
        printf("%02X ", plaintext[i]);
    }
    printf("\n");

    // Encrypt plaintext message using AES 128-bit 
    if (aes_encrypt(plaintext, ciphertext, key_slot)) {
        printf("🔹 Ciphertext: ");
        for (int i = 0; i < 16; i++) {
            printf("%02X ", ciphertext[i]);
        }
        printf("\n");
    } else {
        printf("❌ AES 128-bit encryption failed!\n");
        printf("❓ Is the slot configured for AES?\n");
        return 1;
    }

    // Decrypt plaintext message using AES 128-bit 
    if (aes_decrypt(ciphertext, decrypted_text, key_slot)) {
        printf("🔹 Decrypted Text: ");
        for (int i = 0; i < 16; i++) {
            printf("%02X ", decrypted_text[i]);
        }
        printf("\n");

        // ✅ Check if decryption matches original plaintext
        if (memcmp(plaintext, decrypted_text, 16) == 0) {
            printf("✅ AES Decryption Successful! Plaintext Matches!\n");
        } else {
            printf("❌ AES Decryption Failed! Plaintext Mismatch!\n");
        }
    } else {
        printf("❌ AES Decryption Failed!\n");
        return 1;
    }

    printf("🎉 ATECC608A Test Complete!\n");

    return 0;
}
