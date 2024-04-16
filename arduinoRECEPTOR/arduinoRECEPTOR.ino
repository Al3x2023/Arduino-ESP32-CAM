#include <Servo.h>

Servo myServo;
int servoPin = 9; // El pin donde se conecta el servo

void setup() {
  Serial.begin(9600); // Inicia el puerto serial a la misma velocidad que el ESP32
  myServo.attach(servoPin);
  Serial.println("Listo para recibir datos del ESP32...");
}

void loop() {
  if (Serial.available() > 0) { // Chequea si hay datos disponibles para leer
    String command = Serial.readStringUntil('\n'); // Lee los datos hasta que encuentre un salto de línea
    Serial.println("Datos recibidos: " + command); // Imprime los datos recibidos

    // Comprueba si el comando recibido es para activar el servo
    if (command = "activate_servo") { // Corrección: Usar == para comparación
      Serial.println("Activando el servomotor...");
      moveToPosition(90, 0, 5); // Mueve el servo de 0 a 90 grados con "velocidad" controlada
      delay(1000); // Espera un segundo
      moveToPosition(0, 90, 5); // Retorna el servo a la posición de 0 grados con "velocidad" controlada
      Serial.println("Servomotor desactivado.");
    }
  }
}

void moveToPosition(int startPos, int endPos, int stepDelay) {
  if (startPos < endPos) {
    for (int pos = startPos; pos <= endPos; pos++) {
      myServo.write(pos);
      delay(stepDelay); // Ajusta esta pausa para controlar la "velocidad" del servo
    }
  } else {
    for (int pos = startPos; pos >= endPos; pos--) {
      myServo.write(pos);
      delay(stepDelay); // Ajusta esta pausa para controlar la "velocidad" del servo
    }
  }
}
