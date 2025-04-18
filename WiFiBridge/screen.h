#pragma once

#include <TFT_eSPI.h>
// A library for interfacing with LCD displays
//
// Can be installed from the library manager (Search for "TFT_eSPI")
//https://github.com/Bodmer/TFT_eSPI

TFT_eSPI tft = TFT_eSPI();

// Create a sprite instance
TFT_eSprite sprite = TFT_eSprite(&tft);

void drawSpeed(float speed) {
    // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
    // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
    // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
    // Note the following larger fonts are primarily numeric only!
    // Font 6. Large 48 pixel font, needs ~2666 bytes in FLASH, only characters 1234567890:-.apm
    // Font 7. 7 segment 48 pixel font, needs ~2438 bytes in FLASH, only characters 1234567890:-.
    // Font 8. Large 75 pixel font needs ~3256 bytes in FLASH, only characters 1234567890:-.

    char str[10];
    sprintf(str, "%.1f", speed);
  
    // Set up the sprite (size depends on font height)
    sprite.setColorDepth(8); // 8-bit color depth to save memory
    sprite.createSprite(320, 100); // Adjust height based on font size
    sprite.fillSprite(TFT_BLACK); // Clear sprite background
  
    int fontNum = 8;
    sprite.setTextColor(TFT_WHITE, TFT_BLACK);
    sprite.setTextDatum(MC_DATUM); // Middle center alignment
    sprite.drawString(str, 160, 50, fontNum); // Draw at center of sprite
  
    sprite.pushSprite(0, 70); // Push sprite to TFT at position (x=0, y=70)
  
    sprite.deleteSprite(); // Free memory
}

void setupScreen() {
    // Start the tft display and set it to black
    tft.init();
    tft.setRotation(1); //This is the display in landscape
    // Clear the screen before writing to it
    tft.fillScreen(TFT_BLACK);
}