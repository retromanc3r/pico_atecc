#include "hal_pico_i2c.h"
#include "atecc_cmd.h"

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
    if (!wake_atecc_device()) {
        printf("❌ ERROR: Failed to wake up ATECC608A\n");
        return 1;
    }
    
    // Read the serial number
    if (!read_atecc_serial_number()) {
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
