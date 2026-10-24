// esp32-banco-53ms.ino
// Banco de Pruebas ESP32 - Timer 53ms + 2 LEDs + Botón
// Seguro, solo protoboard, sin conexión a vehículo
// Board: ESP32 Dev Module - Core 3.3.12

#define LED_STATUS 2  // LED Azul - parpadea cada 53ms
#define LED_ROJO 4    // LED Rojo - controlado por botón
#define BOTON 15      // Botón a GND

unsigned long prevMillis = 0;
const unsigned long INTERVALO = 53; // 53 ms
bool estadoLed = false;

void setup() {
  Serial.begin(115200);
  pinMode(LED_STATUS, OUTPUT);
  pinMode(LED_ROJO, OUTPUT);
  pinMode(BOTON, INPUT_PULLUP); // Botón activo en LOW
  
  digitalWrite(LED_STATUS, LOW);
  digitalWrite(LED_ROJO, LOW);
  
  Serial.println("=== ESP32 Banco 53ms Iniciado ===");
  Serial.println("LED 2 parpadea cada 53ms");
}

void loop() {
  unsigned long currMillis = millis();
  
  // Timer no bloqueante de 53ms para LED_STATUS
  if (currMillis - prevMillis >= INTERVALO) {
    prevMillis = currMillis;
    estadoLed = !estadoLed;
    digitalWrite(LED_STATUS, estadoLed);
    
    // Debug cada 1 segundo aprox
    static int contador = 0;
    contador++;
    if (contador % 19 == 0) { // 19*53ms ~= 1000ms
      Serial.printf("Tick 53ms - LED_STATUS: %s\n", estadoLed ? "ON" : "OFF");
    }
  }

  // Lectura de botón - controla LED_ROJO
  if (digitalRead(BOTON) == LOW) {
    digitalWrite(LED_ROJO, HIGH);
  } else {
    digitalWrite(LED_ROJO, LOW);
  }
}
