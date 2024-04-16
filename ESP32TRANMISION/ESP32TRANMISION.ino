#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <HardwareSerial.h>
// Variables globales para controlar el streaming
bool streaming = true;

#include "esp_heap_caps.h"

#define LOW_MEMORY_THRESHOLD 50000 // Ajusta este valor según tus necesidades

// Definición de pines para la cámara
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define LED_GPIO_NUM       4 // Ajusta según tu configuración

HardwareSerial mySerial(1); // Utiliza el segundo puerto serial del ESP32

// Credenciales WiFi
const char* ssid = "AccesoTec";
const char* password = "12345678";
WebServer server(80);

// Prototipos de funciones para que el compilador sepa sobre ellas
void captureAndSendPhoto();
void handleCaptureAndSend();
void handleRoot();
void startCameraServer();
void returnCameraFrame();


// Inicializa la cámara
void setupCamera() {
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
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 10;
    config.fb_count = 1;

    // Inicializa la cámara
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }
    Serial.println("Inicializando cámara");
}

// Inicia el servidor de cámara y define las rutas
void startCameraServer() {
    server.on("/", handleRoot);
    server.on("/capture-send", HTTP_GET, handleCaptureAndSend);
    server.on("/stream", HTTP_GET, returnCameraFrame);
    server.begin();
    Serial.println("Servidor de cámara iniciado");
}


// Maneja la página raíz
void handleRoot() {
    Serial.println("Serving root page...");
    String html = "<!DOCTYPE html><html><head>"
                  "<link rel=\"stylesheet\" href=\"https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css\">"
                  "</head><body>"
                  "<div class=\"container\">"
                  "<h1 class=\"text-center mt-4\">ESP32-CAM Control</h1>"
                  "<div class=\"row justify-content-center mt-4\">"
                  "<img src=\"/stream\" class=\"img-fluid\" alt=\"Stream de cámara\">"
                  "</div>"
                  "<div class=\"row justify-content-center mt-4\">"
                  "<button class=\"btn btn-primary\" onclick=\"captureAndSend()\">Capturar y Enviar</button>"
                  "</div>"
                  "<script>"
                  "function captureAndSend() {"
                  "  fetch('/capture-send')"
                  "  .then(response => response.text())"
                  "  .then(data => alert(data));"
                  "}"
                  "</script>"
                  "</div></body></html>";
    server.send(200, "text/html", html);
}
// Captura y envía la foto
// Captura y envía la foto
void captureAndSendPhoto() {
    Serial.println("Inicio de captura y envío de foto...");
    streaming = false;  // Detiene el streaming temporalmente
    delay(100);  // Pequeña pausa para estabilizar el estado de la cámara

    HTTPClient http;
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Error: la captura de la cámara ha fallado.");
        streaming = true; // Reanuda el streaming si la captura falla
        return;
    }

    Serial.println("Foto capturada exitosamente. Enviando a la API...");
    String serverPath = "http://192.168.137.109:5000/api/imagenes";
    http.begin(serverPath);
    http.addHeader("Content-Type", "image/jpeg");
    int httpResponseCode = http.POST(fb->buf, fb->len);

    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.print("Respuesta de la API: ");
        Serial.println(response);

        DynamicJsonDocument doc(1024);
        deserializeJson(doc, response);

        bool activate_servo = doc["activate_servo"];
        Serial.print("activate_servo: ");
        Serial.println(activate_servo ? "true" : "false");

        if (activate_servo) {
            Serial.println("Enviando comando a Arduino para activar servomotor...");
            mySerial.println("activate_servo"); // Asume Serial2 es tu conexión serial a Arduino
        } else {
            Serial.println("No se requiere activar el servomotor según la respuesta de la API.");
        }
    } else {
        Serial.print("Error en la solicitud POST: ");
        Serial.println(httpResponseCode);
    }

    http.end();
    esp_camera_fb_return(fb);
    streaming = true; // Reanuda el streaming después de capturar y enviar la foto
    Serial.println("Proceso de captura y envío finalizado. Streaming reanudado.");
}

// Manejador para la ruta "/capture-send"
void handleCaptureAndSend() {
    Serial.println("Capture and send handler invoked.");
    captureAndSendPhoto();
    // No redirecting to keep the stream uninterrupted
    server.send(200, "text/plain", "Photo captured and sent.");
}

// Función para devolver el stream de la cámara
void returnCameraFrame() {
    if (!streaming) {
        return; // No realiza streaming si la bandera está desactivada
    }

    WiFiClient client = server.client();
    camera_fb_t* fb = esp_camera_fb_get();
    if (!fb) {
        Serial.println("Camera capture failed");
        return;
    }

    server.sendContent("HTTP/1.1 200 OK\r\nContent-Type: multipart/x-mixed-replace; boundary=frame\r\n\r\n");
    server.sendContent("--frame\r\nContent-Type: image/jpeg\r\n\r\n");
    server.sendContent(reinterpret_cast<const char*>(fb->buf), fb->len);
    server.sendContent("\r\n");
    esp_camera_fb_return(fb);

    if (!client.connected()) {
        return;
    }
}
void setup() {
  Serial.begin(115200);
  Serial.println("Inicio del Serial");

  mySerial.begin(9600, SERIAL_8N1, 3, 1); // Asegúrate de configurar correctamente los pines RX y TX

  WiFi.begin(ssid, password);
  Serial.println("Conectando a WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  mySerial.println(WiFi.localIP()); // Envía la IP a Arduino
  mySerial.println("hola desde el ESP32-CAM"); 

  setupCamera();
  startCameraServer();
}


void adjustCameraSettings() {
    size_t freeMemory = heap_caps_get_free_size(MALLOC_CAP_8BIT);

    sensor_t* s = esp_camera_sensor_get();
    if (freeMemory < LOW_MEMORY_THRESHOLD) {
        // Configuración para baja disponibilidad de memoria
        s->set_framesize(s, FRAMESIZE_SVGA); // Resolución más baja
        s->set_quality(s, 15); // Calidad más baja
    } else {
        // Configuración para alta disponibilidad de memoria
        s->set_framesize(s, FRAMESIZE_UXGA); // Resolución más alta
        s->set_quality(s, 10); // Mejor calidad
    }
}

void loop() {
    server.handleClient();
  
}
