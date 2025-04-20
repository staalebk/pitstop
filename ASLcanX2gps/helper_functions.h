#pragma once

// **Utility function: Extract unsigned integer from bytes (Big-Endian)**
uint32_t bytestouint(const uint8_t *data, uint8_t startByte, uint8_t length) {
    uint32_t value = 0;
    for (int i = 0; i < length; i++) {
        value |= ((uint32_t)data[startByte + i]) << (8 * (length - 1 - i));
    }
    return value;
}

// **Utility function: Extract unsigned integer from bytes (Little-Endian)**
uint32_t bytestouintle(const uint8_t *data, uint8_t startByte, uint8_t length) {
    uint32_t value = 0;
    for (int i = 0; i < length; i++) {
        value |= ((uint32_t)data[startByte + i]) << (8 * i);
    }
    return value;
}

// **Utility function: Extract a single bit from a byte array**
bool bitstouint(const uint8_t *data, uint8_t bitPosition) {
    uint8_t byteIndex = bitPosition / 8;  // Find the byte that contains the bit
    uint8_t bitIndex = bitPosition % 8;   // Find the bit inside the byte
    return (data[byteIndex] >> bitIndex) & 1;
}

/**
 * @brief Dump a representation of binary data to the console.
 *
 * @param [in] pData Pointer to the start of data to be logged.
 * @param [in] length Length of the data (in bytes) to be logged.
 * @return N/A.
 */
static void hexDump(const uint8_t *pData, uint32_t length) {
    char ascii[80];
    char hex[80];
    char tempBuf[80];
    uint32_t lineNumber = 0;
  
    ESP_LOGI(TAG, "     00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f");
    ESP_LOGI(TAG, "     -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --");
    strcpy(ascii, "");
    strcpy(hex, "");
    uint32_t index = 0;
    while (index < length) {
      sprintf(tempBuf, "%.2x ", pData[index]);
      strcat(hex, tempBuf);
      if (isprint(pData[index])) {
        sprintf(tempBuf, "%c", pData[index]);
      } else {
        sprintf(tempBuf, ".");
      }
      strcat(ascii, tempBuf);
      index++;
      if (index % 16 == 0) {
        ESP_LOGI(TAG, "%.4x %s %s", lineNumber * 16, hex, ascii);
        Serial.printf("%s\n", hex);
        strcpy(ascii, "");
        strcpy(hex, "");
        lineNumber++;
      }
    }
    if (index % 16 != 0) {
      while (index % 16 != 0) {
        strcat(hex, "   ");
        index++;
      }
      ESP_LOGI(TAG, "%.4x %s %s", lineNumber * 16, hex, ascii);
      Serial.printf("%s\n", hex);
    }
  }  // hexDump