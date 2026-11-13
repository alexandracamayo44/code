#include <Arduino.h>
#include <AFMotor.h> // Librería para el Motor Shield L293D v1

// Definir el motor en el puerto M1 del Shield
AF_DCMotor motor(1);

// --- PARÁMETROS CONFIGURABLES DE LA PRÁCTICA ---
const int pinSensorAnalogo = A0;    // Pin analógico conectado a la salida análoga del sensor
const float Tm = 1.0;               // Tiempo de muestreo en segundos
const int N = 20;                   // Pulsos por vuelta del disco ranurado
const float D_rueda = 2.5;          // Diámetro de la rueda en cm
const unsigned long t_estab = 5000; // Tiempo de estabilización de 5 segundos por nivel

// Variables para la detección del flanco analógico
int umbral = 512;                   // Valor medio de voltaje (0-1023) para detectar el paso de la ranura
bool estadoAnterior = false;

void setup() {
  Serial.begin(115200); 
  delay(1000);

  Serial.println("=== INICIO DE BARRIDO PWM CON MOTOR SHIELD L293D (CANAL M1) ===");
  
  // Ejecutar el barrido automático una sola vez al iniciar
  ejecutarBarridoShield();
  
  Serial.println("=== BARRIDO FINALIZADO ===");
}

void loop() {
  // Vacío, el barrido se ejecuta de forma secuencial en el setup
}

void ejecutarBarridoShield() {
  int pasos[] = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
  int numPasos = 10;

  for (int i = 0; i < numPasos; i++) {
    int D = pasos[i];

    // Mapear el porcentaje de Duty (0-100%) al valor de 8 bits del Shield (0 a 255)
    int valorDigital = map(D, 0, 100, 0, 255);
    
    // Configurar velocidad y sentido de giro en el Shield
    motor.setSpeed(valorDigital);
    motor.run(FORWARD);

    // 1. Esperar el tiempo de estabilización mecánica del motor (5 segundos)
    delay(t_estab);

    // 2. Ventana de muestreo de Tm segundos (1 segundo) midiendo por el pin analógico A0
    unsigned long inicioMuestreo = millis();
    unsigned int cont = 0;

    while (millis() - inicioMuestreo < (Tm * 1000)) {
      int valorAnalogo = analogRead(pinSensorAnalogo);
      bool estadoActual = (valorAnalogo > umbral);
      
      if (estadoActual && !estadoAnterior) {
        cont++; // Contar flanco de subida en la señal analógica
      }
      estadoAnterior = estadoActual;
    }

    // --- CÁLCULOS EXIGIDOS POR LA GUÍA ---
    float f_sensor = (float)cont / Tm;                  // Frecuencia del sensor en Hz
    float RPM = (f_sensor / (float)N) * 60.0;           // Velocidad en RPM
    float omega = RPM * (2.0 * PI / 60.0);              // Velocidad angular en rad/s
    float v = (PI * D_rueda * RPM) / 60.0;              // Velocidad lineal en cm/s

    // Imprimir en el Monitor Serial con formato claro
    Serial.print("D=");
    Serial.print(D);
    Serial.print("% | Valor Digital=");
    Serial.print(valorDigital);
    Serial.print(" | RPM=");
    Serial.print(RPM, 2);
    Serial.print(" | w=");
    Serial.print(omega, 2);
    Serial.print(" rad/s | v=");
    Serial.print(v, 2);
    Serial.println(" cm/s");
  }

  // --- APAGADO SEGURO DEL MOTOR ---
  motor.setSpeed(0);
  motor.run(RELEASE);
  Serial.println("Motor detenido de forma segura.");
}