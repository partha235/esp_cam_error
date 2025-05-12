#include "esp_camera.h"
#include "Arduino.h"
#include "FS.h"                // SD Card ESP32
#include "SD_MMC.h"            // SD Card ESP32
#include "soc/soc.h"           // Disable brownour problems
#include "soc/rtc_cntl_reg.h"  // Disable brownour problems
#include "driver/rtc_io.h"
#include <EEPROM.h>            // read and write from flash memory

// define the number of bytes you want to access
#define EEPROM_SIZE 1

// Camera pin definitions
#define PWDN_GPIO_NUM  32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM  0
#define SIOD_GPIO_NUM  26
#define SIOC_GPIO_NUM  27

#define Y9_GPIO_NUM    35
#define Y8_GPIO_NUM    34
#define Y7_GPIO_NUM    39
#define Y6_GPIO_NUM    36
#define Y5_GPIO_NUM    21
#define Y4_GPIO_NUM    19
#define Y3_GPIO_NUM    18
#define Y2_GPIO_NUM    5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM  23
#define PCLK_GPIO_NUM  22

// LED pin (flash or normal)
#define LED_GPIO_NUM   4

int pictureNumber = 0;

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  Serial.begin(115200);
  delay(1000);
  Serial.println("\nInitializing camera...");
  
  // Initialize camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_RGB565; // Use RGB565
  config.frame_size = FRAMESIZE_QQVGA;     // Adjust resolution
  config.fb_count = 1;

  if (psramFound()) {
    Serial.println("PSRAM Found");
  } else {
    Serial.println("No PSRAM Found");
    config.frame_size = FRAMESIZE_QQVGA; // Lower resolution
  }

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  Serial.println("Camera initialized successfully.");

  if (!SD_MMC.begin()) {
    Serial.println("SD Card Mount Failed");
    return;
  }
  Serial.println("SD Card initialized.");

  // Capture image
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return;
  }
  Serial.printf("Captured image size: %d bytes\n", fb->len);

  // Save image (raw RGB565)
  String path = "/picture5000.bmp";
  File file = SD_MMC.open(path.c_str(), FILE_WRITE);
  if (file) {
    // Write BMP header for RGB565
    writeBMPHeader(file, fb->width, fb->height);
    file.write(fb->buf, fb->len); // Write raw image data
    Serial.printf("Saved image to: %s\n", path.c_str());
    file.close();
  } else {
    Serial.println("Failed to save image");
  }

  esp_camera_fb_return(fb);

  delay(2000);
  Serial.println("Going to sleep now.");
  esp_deep_sleep_start();
}

// Function to write BMP header for RGB565
void writeBMPHeader(File &file, int width, int height) {
  uint32_t fileSize = 54 + (width * height * 2);
  uint16_t bfType = 0x4D42; // "BM"
  uint32_t bfReserved = 0;
  uint32_t bfOffBits = 54;
  uint32_t biSize = 40;
  uint16_t biPlanes = 1;
  uint16_t biBitCount = 16;
  uint32_t biCompression = 3; // BI_BITFIELDS
  uint32_t biSizeImage = width * height * 2;
  uint32_t biXPelsPerMeter = 0;
  uint32_t biYPelsPerMeter = 0;
  uint32_t biClrUsed = 0;
  uint32_t biClrImportant = 0;

  // Write BMP file header
  file.write((uint8_t *)&bfType, 2);
  file.write((uint8_t *)&fileSize, 4);
  file.write((uint8_t *)&bfReserved, 4);
  file.write((uint8_t *)&bfOffBits, 4);

  // Write BMP info header
  file.write((uint8_t *)&biSize, 4);
  file.write((uint8_t *)&width, 4);
  file.write((uint8_t *)&height, 4);
  file.write((uint8_t *)&biPlanes, 2);
  file.write((uint8_t *)&biBitCount, 2);
  file.write((uint8_t *)&biCompression, 4);
  file.write((uint8_t *)&biSizeImage, 4);
  file.write((uint8_t *)&biXPelsPerMeter, 4);
  file.write((uint8_t *)&biYPelsPerMeter, 4);
  file.write((uint8_t *)&biClrUsed, 4);
  file.write((uint8_t *)&biClrImportant, 4);

  // Write RGB565 color masks
  uint32_t redMask = 0xF800;
  uint32_t greenMask = 0x07E0;
  uint32_t blueMask = 0x001F;
  file.write((uint8_t *)&redMask, 4);
  file.write((uint8_t *)&greenMask, 4);
  file.write((uint8_t *)&blueMask, 4);
}



void loop() {
  
}
