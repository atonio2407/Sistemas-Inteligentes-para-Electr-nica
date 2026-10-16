//Grupo 3 (perceptron): Adrián Martínez, Andrés Almeida, Jesús Hernández, Cesar de Moya

#include <WiFi.h>
#include <HTTPClient.h>

// ==========================================
// 1. CONFIGURACIÓN WI-FI Y SERVIDOR
// ==========================================
const char* ssid     = "electrolitico";
const char* password = "contrase";
const char* serverUrl = "http://192.168.43.100:5000/api/colores";

// ==========================================
// 2. PINES ESP32-S3 Y SENSOR TCS3200
// ==========================================
const int S0 = 4;        
const int S1 = 5;
const int S2 = 6;
const int S3 = 7;
const int sensorOut = 8;

const int BOTON_EXT = 9;  // Pulsador en GPIO 9
const int BOTON_BOOT = 0; // Botón BOOT integrado

#ifndef RGB_BUILTIN
  #define RGB_PIN 48 
#else
  #define RGB_PIN RGB_BUILTIN
#endif

// ==========================================
// 3. VARIABLES
// ==========================================
String macAddress = "";
int colorHue = 0;
bool estadoBotonAnterior = HIGH;
unsigned long tiempoUltimoRebote = 0;

int readColor(int s2_val, int s3_val);
void ruedaColor(byte pos, byte &r, byte &g, byte &b);
void enviarDatosServidor(String jsonPayload);

void setup() {
  Serial.begin(115200);
  delay(1500);

  pinMode(S0, OUTPUT);
  pinMode(S1, OUTPUT);
  pinMode(S2, OUTPUT);
  pinMode(S3, OUTPUT);
  pinMode(sensorOut, INPUT);

  digitalWrite(S0, HIGH);
  digitalWrite(S1, LOW); // Escala al 20%

  pinMode(BOTON_EXT, INPUT_PULLUP);
  pinMode(BOTON_BOOT, INPUT_PULLUP);

  neopixelWrite(RGB_PIN, 0, 0, 0);

  Serial.println("\n==========================================");
  Serial.println(" ESP32-S3: Detector de Color + Feedback LED");
  Serial.println("==========================================");

  // Conexión Wi-Fi (máximo 8 segundos)
  Serial.print("Conectando al Wi-Fi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 16) {
    delay(500);
    Serial.print(".");
    intentos++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ Wi-Fi conectado con éxito!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[!] Sin Wi-Fi (funcionando en modo local)");
  }

  macAddress = WiFi.macAddress();
  Serial.print("MAC: ");
  Serial.println(macAddress);
  Serial.println(">> Presiona el boton para capturar <<\n");
}

void loop() {
  // 1. LED MULTICOLOR EN ESPERA
  byte red, green, blue;
  ruedaColor(colorHue & 255, red, green, blue);
  neopixelWrite(RGB_PIN, red / 8, green / 8, blue / 8); // Brillo sutil
  colorHue++;

  // 2. DETECCIÓN DEL PULSADOR
  bool presionado = (digitalRead(BOTON_EXT) == LOW) || (digitalRead(BOTON_BOOT) == LOW);

  if (presionado && estadoBotonAnterior == HIGH) {
    if (millis() - tiempoUltimoRebote > 250) {
      tiempoUltimoRebote = millis();

      // Apagar LED antes de medir para evitar luz parásita
      neopixelWrite(RGB_PIN, 0, 0, 0);
      delay(30);

      // --- PASO 1: Leer tiempos del sensor (µs) ---
      int pulsoR = readColor(LOW, LOW);   // Rojo
      int pulsoG = readColor(HIGH, HIGH); // Verde
      int pulsoB = readColor(LOW, HIGH);  // Azul

      // --- PASO 2: CONVERSIÓN A INTENSIDAD REAL ---
      float intR = (pulsoR > 0) ? (10000.0 / pulsoR) : 0;
      float intG = (pulsoG > 0) ? (10000.0 / pulsoG) : 0;
      float intB = (pulsoB > 0) ? (10000.0 / pulsoB) : 0;

      int r = 0, g = 0, b = 0;

      if (pulsoR < 350 || pulsoG < 350 || pulsoB < 350) {
        float maxIntensidad = max(intR, max(intG, intB));
        if (maxIntensidad > 0) {
          r = constrain((int)((intR / maxIntensidad) * 255.0), 0, 255);
          g = constrain((int)((intG / maxIntensidad) * 255.0), 0, 255);
          b = constrain((int)((intB / maxIntensidad) * 255.0), 0, 255);
        }
      }

      // --- PASO 3: MOSTRAR RESULTADOS EN CONSOLA ---
      Serial.println("\n------------------------------------------");
      Serial.printf("Pulsos brutos (µs) -> R:%d | G:%d | B:%d\n", pulsoR, pulsoG, pulsoB);

      // --- PASO 4: ILUMINAR EL LED SEGÚN EL COLOR DOMINANTE ---
      if (r > g && r > b) {
        Serial.printf(">> COLOR DETECTADO: [ ROJO ] (R: %d)\n", r);
        Serial.println(">> Iluminando LED en ROJO por 3 segundos...");
        neopixelWrite(RGB_PIN, 120, 0, 0); // Rojo brillante
      } else if (g > r && g > b) {
        Serial.printf(">> COLOR DETECTADO: [ VERDE ] (G: %d)\n", g);
        Serial.println(">> Iluminando LED en VERDE por 3 segundos...");
        neopixelWrite(RGB_PIN, 0, 120, 0); // Verde brillante
      } else if (b > r && b > g) {
        Serial.printf(">> COLOR DETECTADO: [ AZUL ] (B: %d)\n", b);
        Serial.println(">> Iluminando LED en AZUL por 3 segundos...");
        neopixelWrite(RGB_PIN, 0, 0, 120); // Azul brillante
      } else {
        Serial.println(">> COLOR INDEFINIDO / MIXTO");
        neopixelWrite(RGB_PIN, 60, 60, 60); // Blanco tenue
      }

      // JSON listo
      String jsonPayload = "{\"mac\":\"" + macAddress + "\",\"R\":" + String(r) + ",\"G\":" + String(g) + ",\"B\":" + String(b) + "}";
      Serial.print(">> JSON: ");
      Serial.println(jsonPayload);

      // Enviar datos por HTTP al servidor
      enviarDatosServidor(jsonPayload);

      // MANTENER EL COLOR ENCENDIDO POR 3 SEGUNDOS
      delay(3000);

      Serial.println(">> Reanudando modo espera (arcoiris)...");
      Serial.println("------------------------------------------");
    }
  }

  estadoBotonAnterior = presionado ? LOW : HIGH;
  delay(15);
}

// Lectura de 20 ms idéntica a la del profesor
int readColor(int s2_val, int s3_val) {
  digitalWrite(S2, s2_val);
  digitalWrite(S3, s3_val);
  delay(20); 
  return pulseIn(sensorOut, LOW, 20000); 
}

void enviarDatosServidor(String jsonPayload) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverUrl);
    http.setTimeout(2000);
    http.addHeader("Content-Type", "application/json");

    Serial.print(">> Enviando HTTP... ");
    int httpResponseCode = http.POST(jsonPayload);

    if (httpResponseCode > 0) {
      Serial.printf("Código: %d | Respuesta: %s\n", httpResponseCode, http.getString().c_str());
    } else {
      Serial.printf("Error HTTP: %d\n", httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("[Aviso] Sin Wi-Fi: Solo registrado localmente.");
  }
}

void ruedaColor(byte pos, byte &r, byte &g, byte &b) {
  pos = 255 - pos;
  if (pos < 85) {
    r = 255 - pos * 3; g = 0; b = pos * 3;
  } else if (pos < 170) {
    pos -= 85;
    r = 0; g = pos * 3; b = 255 - pos * 3;
  } else {
    pos -= 170;
    r = pos * 3; g = 255 - pos * 3; b = 0;
  }
}