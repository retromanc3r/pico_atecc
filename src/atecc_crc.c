#include "pico_atecc.h"

// Calculate CRC16-CCITT (0x8005) checksum (little-endian) taken from CryptoAuthLib
void calc_crc16_ccitt(size_t length, const uint8_t *data, uint8_t *crc_le) {
    size_t counter;
    uint16_t crc_register = 0;
    uint16_t polynom = 0x8005;
    uint8_t shift_register;
    uint8_t data_bit, crc_bit;

    for (counter = 0; counter < length; counter++) {
        for (shift_register = 0x01; shift_register > 0x00u; shift_register <<= 1) {
            data_bit = ((data[counter] & shift_register) != 0u) ? 1u : 0u;
            crc_bit = (uint8_t)(crc_register >> 15);
            crc_register <<= 1;
            if (data_bit != crc_bit) {
                crc_register ^= polynom;
            }
        }
    }
    crc_le[0] = (uint8_t)(crc_register & 0x00FFu);
    crc_le[1] = (uint8_t)(crc_register >> 8u);
}

// Compute CRC of the data bytes (excluding the CRC bytes)
void compute_crc(uint8_t length, uint8_t *data, uint8_t *crc) {
    calc_crc16_ccitt(length, data, crc);
}

// Validate the CRC of the response data 
bool validate_crc(uint8_t *response, size_t length) {
    if (length < 3) return false; // Not enough bytes for CRC
    uint8_t computed_crc[2];
    compute_crc(length - 2, response, computed_crc);
    return (computed_crc[0] == response[length - 2] && computed_crc[1] == response[length - 1]);
}

// Debug function to print CRC mismatch details
void debug_crc_mismatch(uint8_t *data, size_t length, uint8_t *expected_crc) {
    uint8_t computed_crc[2];
    compute_crc(length - 2, data, computed_crc);

    printf("🔍 Expected CRC: %02X %02X\n", expected_crc[0], expected_crc[1]);
    printf("🔍 Computed CRC: %02X %02X\n", computed_crc[0], computed_crc[1]);

    if (computed_crc[0] == expected_crc[0] && computed_crc[1] == expected_crc[1]) {
        printf("✅ CRC MATCH\n");
    } else {
        printf("❌ CRC MISMATCH\n");
    }
}