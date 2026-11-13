#include <Arduino.h>
#include <AFMotor.h> // Librería para el Motor Shield L293D v1

// Definir el motor en el puerto M1 del Shield
AF_DCMotor motor(1);

// --- PARÁMETROS CONFIGURABLES DE LA PRÁCTICA ---
const int pinSensorAnalogo = A0;    // Pin analógico conectado a la salida análoga del sensor
const float Tm = 1.0;               // Tiempo de muestreo de cada ventana en segundos
const int N = 20;                   // Pulsos por vuelta del disco ranurado
const float D_rueda = 2.5;          // Diámetro de la rueda en cm
const unsigned long t_prueba = 30000; // 30 segundos de prueba por cada sentido

// Variables para la detección del flanco analógico
int umbral = 512;                   
bool estadoAnterior = false;

void setup() {
  Serial.begin(115200); 
  delay(1000);

  Serial.println("=== INICIO DE PRUEBA FINAL Y VALIDACIÓN (CUADRO 4) ===");
  
  // --- CASO 1: ADELANTE ---
  // Ajusta aquí el porcentaje de Duty Cycle (%D) que logre la velocidad objetivo de 3.0 cm/s
  int dutyAdelante = 40; // Puedes cambiar este valor según tu caracterización previa
  ejecutarPrueba("Adelante", FORWARD, dutyAdelante);

  // Pausa de seguridad entre cambios de giro
  motor.setSpeed(0);
  motor.run(RELEASE);
  delay(3000);

  // --- CASO 2: ATRÁS ---
  // Ajusta aquí el porcentaje de Duty Cycle (%D) que logre la velocidad objetivo de 1.5 cm/s
  int dutyAtras = 25; // Puedes cambiar este valor según tu caracterización previa
  ejecutarPrueba("Atras", BACKWARD, dutyAtras);

  // --- APAGADO SEGURO FINAL ---
  motor.setSpeed(0);
  motor.run(RELEASE);
  Serial.println("=== PRUEBA FINALIZADA CON ÉXITO ===");
}

void loop() {
  // Vacío, la prueba se ejecuta de forma secuencial en el setup
}

void ejecutarPrueba(String nombreCaso, uint8_t direccion, int D_porcentaje) {
  Serial.println("\n----------------------------------------");
  Serial.print("Ejecutando caso: ");
  Serial.print(nombreCaso);
  Serial.print(" | Duty Cycle: ");
  Serial.print(D_porcentaje);
  Serial.println("%");

  int valorDigital = map(D_porcentaje, 0, 100, 0, 255);
  motor.setSpeed(valorDigital);
  motor.run(direccion);

  unsigned long inicioPrueba = millis();
  int totalVentanas = 0;
  float sumaRPM = 0;
  float arrayRPM[30]; // Almacena hasta 30 muestras de 1 segundo para calcular desviación estándar

  // Bucle de adquisición durante los 30 segundos exactos
  while (millis() - inicioPrueba < t_prueba && totalVentanas < 30) {
    unsigned long inicioMuestreo = millis();
    unsigned int contPulsos = 0;

    // Ventana de muestreo de 1 segundo (Tm = 1.0)
    while (millis() - inicioMuestreo < (Tm * 1000)) {
      int valorAnalogo = analogRead(pinSensorAnalogo);
      bool estadoActual = (valorAnalogo > umbral);
      
      if (estadoActual && !estadoAnterior) {
        contPulsos++;
      }
      estadoAnterior = estadoActual;
    }

    // Cálculos instantáneos de esta ventana de 1s
    float f_sensor = (float)contPulsos / Tm;
    float RPM_actual = (f_sensor / (float)N) * 60.0;

    arrayRPM[totalVentanas] = RPM_actual;
    sumaRPM += RPM_actual;
    totalVentanas++;
  }

  // --- CÁLCULOS ESTADÍSTICOS FINALES (30 SEGUNDOS) ---
  float RPM_med = sumaRPM / (float)totalVentanas;

  // Cálculo de la Desviación Estándar (sigma)
  float sumaDiferenciasCuadradas = 0;
  for (int i = 0; i < totalVentanas; i++) {
    float diff = arrayRPM[i] - RPM_med;
    sumaDiferenciasCuadradas += (diff * diff);
  }
  float sigma_RPM = sqrt(sumaDiferenciasCuadradas / (float)totalVentanas);

  // Conversión a velocidad angular y lineal media
  float omega_med = RPM_med * (2.0 * PI / 60.0);
  float v_med = (PI * D_rueda * RPM_med) / 60.0;

  // --- REPORTE EN MONITOR SERIAL (Para transcribir a la Tabla 4) ---
  Serial.println(">>> RESULTADOS FINALES PARA LA TABLA 4 <<<");
  Serial.print("Caso: "); Serial.println(nombreCaso);
  Serial.print("D (%): "); Serial.println(D_porcentaje);
  Serial.print("RPM_med: "); Serial.println(RPM_med, 2);
  Serial.print("sigma_RPM (Desviación): "); Serial.println(sigma_RPM, 2);
  Serial.print("omega_med (rad/s): "); Serial.println(omega_med, 2);
  Serial.print("v_med (cm/s): "); Serial.println(v_med, 2);
  Serial.println("----------------------------------------");
}