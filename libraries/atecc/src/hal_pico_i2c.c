#include "hal_pico_i2c.h"
#include "hardware/i2c.h"

/** @brief Send a command to an ATECC device.
 *
 * The send_atecc_command function sends a command to an ATECC (Atmel CryptoAuthentication)
 * device, using the specified opcode, parameters, and data.
 *
 * @param[in]  opcode    The opcode of the command to send.
 * @param[in]  param1    The first parameter of the command.
 * @param[in]  param2    The second parameter of the command.
 * @param[in]  data      The data to send with the command.
 *
 * @return true on success, false on failure.
 */
int hal_i2c_send(uint8_t *txdata, size_t txlength) {
    return i2c_write_blocking(I2C_PORT, I2C_ADDR, txdata, txlength, false);
}

/**
 * @brief Receives data over the I2C bus.
 *
 * This function reads data from the I2C bus and stores it in the provided
 * rxdata buffer of length rxlength. If the read operation fails, the function
 * returns -1.
 *
 * @param[in]  bus      The I2C bus to read from.
 * @param[out] rxdata   Pointer to the buffer where received data will be stored.
 * @param[in]  rxlength The length of the data to be received.
 *
 * @return the number of bytes read, or -1 on failure.
 */
int hal_i2c_receive(uint8_t *rxdata, size_t rxlength) {
   int res = i2c_read_blocking(I2C_PORT, I2C_ADDR, rxdata, rxlength, false);
   if (res != rxlength) {
       printf("❌ ERROR: I2C read failed (expected %zu, got %d)\n", rxlength, res);
       return -1;
   }

   return res;
}

/**
 * @brief Sends a command to an ATECC device over the I2C bus on a Pico microcontroller.
 *
 * This function constructs the command with the given parameters, calculates the CRC16-CCITT checksum,
 * and sends the full command using the hal_i2c_send function.
 *
 * @param[in] device_address The I2C address of the ATECC device.
 * @param[in] command The command to send to the ATECC device.
 * @param[in] param1 The first parameter for the command.
 * @param[in] param2 The second parameter for the command.
 * @param[in] data The data to send with the command.
 * @param[in] data_length The length of the data to send.
 *
 * @return int Returns 0 on success, or a negative error code on failure.
 */
// Send an ATECC command over I2C bus (Pico) 
bool send_atecc_command(uint8_t opcode, uint8_t param1, uint16_t param2, const uint8_t *data, size_t data_len) {
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

    return hal_i2c_send((uint8_t*)&full_command, sizeof(full_command)) >= 0;
}

/**
 * @brief Reads a response from the ATECC608A device via I2C into a provided buffer.
 *
 * This function reads a response from the ATECC608A device using the I2C protocol. 
 * It handles either a full response or a partial response based on the full_response flag.
 *
 * @param[out] buffer The buffer to store the response read from the device.
 * @param[in] buffer_size The size of the buffer.
 * @param[in] full_response A flag indicating whether to read a full response (true) or a partial response (false).
 * @return bool Returns true if the response was successfully read, otherwise false.
 */
bool receive_atecc_response(uint8_t *buffer, size_t length, bool full_response) {
    uint8_t response[7];
    size_t read_length = full_response ? 7 : length + 1;
    
    if (hal_i2c_receive(response, read_length) != (int)read_length) {
        printf("❌ ERROR: Failed to read response from ATECC608A\n");
        return false;
    }
    
    memcpy(buffer, &response[1], length);
    return true;
}

/**
 * @brief Sends an idle command via I2C.
 *
 * This function sends an idle command using the hal_i2c_send function. 
 * It returns true if the command is successfully sent, otherwise it 
 * prints an error message and returns false.
 *
 * @return true if the command is successfully sent, false otherwise.
 */
bool send_idle_command() {
    uint8_t idle_cmd = OP_IDLE; // Idle command op-code
    int res = hal_i2c_send((uint8_t*)&idle_cmd, sizeof(idle_cmd));
    if (res != sizeof(idle_cmd)) {
        printf("❌ ERROR: Failed to send idle command! (Expected %d, got %d)\n", (int)sizeof(idle_cmd), res);
        return false;
    }
    
    return true;
}

/**
 * @brief Wakes up the ATECC device.
 *
 * This function sends a wake-up sequence to the ATECC device using the hal_i2c_send function. 
 * It then receives a response from the device and checks if the wake-up was successful.
 *
 * @return true if the wake-up was successful, false otherwise.
 */
bool wake_atecc_device() {    
    uint8_t data = 0x00;
    uint8_t wake_response[4];

    // Send wakeup sequence with proper delays
    hal_i2c_send(&data, sizeof(data));
    sleep_ms(1);

    int res = hal_i2c_receive(wake_response, sizeof(wake_response));
    printf("Wake-up Response: ");
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

