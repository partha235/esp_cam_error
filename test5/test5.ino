#include "esp_camera.h"
#include <WiFi.h>
#include "SD.h"
#include <SPI.h>

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

// WiFi credentials
const char* ssid = "your_ssid";
const char* password = "your_password";

// Create an HTTP server
WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  delay(1000); // Wait for serial monitor to initialize

  // Initialize the camera
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
  config.frame_size = FRAMESIZE_QVGA;   // Frame size
  config.pixel_format = PIXFORMAT_GRAYSCALE;  // Black and white image
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;  // Not needed for grayscale
  config.fb_count = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed");
    return;
  }

  // Initialize the SD card
  if (!SD.begin()) {
    Serial.println("SD Card initialization failed!");
    return;
  }
  Serial.println("SD Card initialized.");

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Start the HTTP server
  server.begin();
}

void loop() {
  WiFiClient client = server.available();

  if (client) {
    String request = client.readStringUntil('\r');
    Serial.println("Request: " + request);
    client.flush();

    if (request.indexOf("GET /") >= 0) {
      // Capture image from the camera
      camera_fb_t* fb = esp_camera_fb_get();
      if (!fb) {
        client.println("HTTP/1.1 500 Internal Server Error");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.println("Failed to capture image");
        return;
      }

      // Save the captured image to the SD card
      File file = SD.open("/image.raw", FILE_WRITE);
      if (file) {
        file.write(fb->buf, fb->len);  // Save raw grayscale image
        file.close();
        Serial.println("Image saved to SD card.");
      } else {
        Serial.println("Failed to save image to SD card.");
      }

      // Serve the image to the browser
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: image/raw");
      client.println("Connection: close");
      client.println();

      File imageFile = SD.open("/image.raw");
      if (imageFile) {
        // Read image from SD card and send it to the client
        while (imageFile.available()) {
          client.write(imageFile.read());
        }
        imageFile.close();
      } else {
        Serial.println("Error reading the image file from SD.");
      }

      esp_camera_fb_return(fb); // Return the camera frame buffer
    }

    delay(1000);
    client.stop();
  }
}
