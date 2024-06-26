#include "esp_camera.h"
#include "Arduino.h"
#include "base64.h"

#include <HTTPClient.h>

// Select camera model
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// WiFi credentials
const char* ssid = "jwjung";
const char* password = "01027075860";


#define CAMERA_RESOLUTION FRAMESIZE_SVGA // 원하는 해상도로 변경하세요. 예: FRAMESIZE_SVGA /FRAMESIZE_UXGA, FRAMESIZE_SVGA, FRAMESIZE_QVGA

void setup() {
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

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
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  config.frame_size = CAMERA_RESOLUTION;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  if (psramFound()) {
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }

  // Camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t *s = esp_camera_sensor_get();
  // Initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1); // Flip it back
    s->set_brightness(s, 1); // Up the brightness just a bit
    s->set_saturation(s, -2); // Lower the saturation
  }
  // Drop down frame size for higher initial frame rate
  if (config.pixel_format == PIXFORMAT_JPEG) {
    s->set_framesize(s, FRAMESIZE_QVGA);
  }
}





void httpPost(String serverUrl, String jsonData) {
  if(WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");
    
    // JSON data to send
    
    // Send HTTP POST request
    int httpResponseCode = http.POST(jsonData);
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }
    
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}


void loop() {
  // Capture a frame
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    delay(1000);
    return;
  }

  // Encode image to base64
  String imageBase64 = base64::encode((uint8_t *)fb->buf, fb->len);
  String jsonData = "{\"image\":\"" + imageBase64 + "\"}";
  httpPost("http://125.180.95.161:25565/create/color/", jsonData);

  // Output base64 string to Serial
  Serial.println(imageBase64);

  // Return the frame buffer back to the driver for reuse
  esp_camera_fb_return(fb);

  // Wait for 0.2 second
  delay(200);
}
