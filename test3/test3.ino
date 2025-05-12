#include "esp_camera.h"
#include <WiFi.h>

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
  config.frame_size = FRAMESIZE_QVGA;
  config.pixel_format = PIXFORMAT_RGB565; // Use RGB565 instead of JPEG
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
  WiFiClient client = server.available(); // Listen for incoming clients

  if (client) {
    String request = client.readStringUntil('\r');
    Serial.println("Request: " + request);
    client.flush();

    if (request.indexOf("GET /capture") >= 0) {
      // Capture an image
      camera_fb_t* fb = esp_camera_fb_get();
      if (!fb) {
        Serial.println("Camera capture failed");
        client.println("HTTP/1.1 500 Internal Server Error");
        client.println("Content-Type: text/plain");
        client.println("Connection: close");
        client.println();
        client.println("Failed to capture image");
      } else {
        // Send the image as an HTTP response
        client.println("HTTP/1.1 200 OK");
        client.println("Content-Type: image/bmp"); // Send as BMP
        client.println("Content-Length: " + String(fb->len));
        client.println("Connection: close");
        client.println();
        client.write(fb->buf, fb->len); // Send the frame buffer
        esp_camera_fb_return(fb);
        Serial.println("Image sent to client");
      }
    } else if (request.indexOf("GET /") >= 0) {
      // Serve the HTML page for the root request
      client.println("HTTP/1.1 200 OK");
      client.println("Content-Type: text/html");
      client.println("Connection: close");
      client.println();
      client.println("<!DOCTYPE HTML>");
      client.println("<html>");
      client.println("<head><title>ESP32 Camera</title></head>");
      client.println("<body><h1>ESP32 Camera</h1>");
      client.println("<p><a href=\"/capture\">Capture Image</a></p>");
      client.println("</body></html>");
    } else {
      // Handle other requests
      client.println("HTTP/1.1 404 Not Found");
      client.println("Content-Type: text/plain");
      client.println("Connection: close");
      client.println();
      client.println("Not Found");
    }
    delay(1000);
    client.stop();
    Serial.println("Client disconnected");
  }
}
