#include "esp_camera.h"
#include <WiFi.h>
#include "base64.hpp"

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
const char* ssid = "bps_wifi";
const char* password = "sagabps@235";

// Create an HTTP server
WiFiServer server(80);

void setup() {
  Serial.begin(115200);
  delay(1000); // Wait for serial monitor to initialize

  Serial.print("Free heap size: ");
  Serial.println(ESP.getFreeHeap());


  // Configure the camera
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
  config.frame_size = FRAMESIZE_VGA;
  // Change the pixel format to GRAYSCALE (black and white image)
  config.pixel_format = PIXFORMAT_GRAYSCALE;

  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  // Initialize the camera
  if (esp_camera_init(&config) != ESP_OK) {
    Serial.println("Camera init failed");
    return;
  }

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

  // Start the server
  server.begin();
}


void loop() {
  WiFiClient client = server.available();

  if (client) {
    String request = client.readStringUntil('\r');
    Serial.println("Request: " + request);
    client.flush();

    if (request.indexOf("GET /") >= 0) {
      // Capture the image from the camera
      camera_fb_t* fb = esp_camera_fb_get();
      if (!fb) {
        client.println("HTTP/1.1 500 Internal Server Error");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.println("Failed to capture image");
      } else {
        // Start sending the response
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: text/html");
        client.println("Connection: close");
        client.println();

        // Manually calculate the output size for base64 encoding
        unsigned int encoded_size = 4 * ((fb->len + 2) / 3);
        
        // Dynamically allocate memory for base64 output
        unsigned char* output = (unsigned char*)ps_malloc(encoded_size);
        if (output == NULL) {
          Serial.println("Memory allocation for base64 failed.");
          client.println("HTTP/1.1 500 Internal Server Error");
          client.println("Content-Type: text/plain");
          client.println("Connection: close");
          client.println();
          client.println("Failed to allocate memory for base64 encoding.");
        } else {
          // Encode the captured image buffer to base64
          encode_base64(fb->buf, fb->len, output);

          // Convert the base64 encoded data to a string and send it in the HTML
          String imageBase64 = String((char*)output);
          client.println("<img src=\"data:image/jpeg;base64," + imageBase64 + "\"/>");

          // Free the allocated memory
          free(output);
        }

        // Return the frame buffer to free memory
        esp_camera_fb_return(fb);
      }
    }

    delay(1000);  // Add delay to prevent overwhelming the server
    client.stop();  // Ensure the client connection is properly closed
    Serial.println("Server closed");
  }
}



