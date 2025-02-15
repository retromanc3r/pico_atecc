#ifndef ATECC_CRC_H
#define ATECC_CRC_H

#include <stddef.h>
#include <stdint.h>


// Function to calculate CRC16
void calc_crc16_ccitt(size_t length, const uint8_t *data, uint8_t *crc_le);
void compute_crc(uint8_t length, uint8_t *data, uint8_t *crc);
bool validate_crc(uint8_t *response, size_t length);
void debug_crc_mismatch(uint8_t *data, size_t length, uint8_t *expected_crc);

#endif // ATECC_CRC_H