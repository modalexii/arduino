/*
 * Linear Actuator Shuttle
 * -------------------------------------------------------------
 * Board:   Hiletgo ESP32-WROOM-32 dev board
 * Driver:  L298N dual H-bridge motor driver
 * Load:    12V mini linear actuator
 *
 * Behavior: extend the actuator fully, pause, retract it fully,
 * pause, and repeat forever at a moderate speed.
 *
 * WIRING
 * -------------------------------------------------------------
 *   ESP32 GPIO 27  -> L298N IN1
 *   ESP32 GPIO 26  -> L298N IN2
 *   ESP32 GPIO 25  -> L298N ENA   (remove the ENA jumper cap!)
 *   ESP32 GND      -> L298N GND   (also common with the 12V supply GND)
 *
 *   L298N 12V, GND -> external 12V supply (NOT the ESP32's 5V/3V3 rail)
 *   L298N OUT1/OUT2 -> actuator's two motor wires
 *
 * NOTE ON TIMING
 * -------------------------------------------------------------
 * This driver has no position feedback, so "fully extended/retracted"
 * is simulated with a timed run. Set ACTUATOR_TRAVEL_TIME_MS below
 * to slightly MORE than your actuator's real travel time (stroke
 * length / rated speed, from its listing) so it fully bottoms out
 * each direction. Many mini actuators have a built-in limit switch
 * that stops the motor and protects it once it reaches the end, even
 * if you command it to keep pushing a bit longer -- check yours.
 * If your actuator does NOT have a built-in limit switch, don't set
 * this value too much higher than the real travel time, or you'll
 * stall the motor against its hard stop for that extra time on every
 * cycle.
 *
 * Your actuator: 0.83" (21mm) stroke @ 0.2"/s (5mm/s) no-load
 *   => 21mm / 5mm/s = 4.2s theoretical travel time.
 * Under actual load it'll move a bit slower than the no-load spec,
 * so ACTUATOR_TRAVEL_TIME_MS below is set to 5000ms (~20% margin).
 * If it still doesn't sound like it's bottoming out, or it's clearly
 * stalling before the timer ends, adjust this value up or down.
 *
 * NOTE ON PWM API
 * -------------------------------------------------------------
 * This uses the "legacy" ESP32 Arduino ledc API (ledcSetup +
 * ledcAttachPin + ledcWrite(channel, ...)), which is what ships with
 * ESP32 Arduino core 2.x -- the version PlatformIO's espressif32
 * platform commonly installs by default. This is why ledcAttach()
 * (the newer core 3.x, pin-based call) wasn't found: your installed
 * core doesn't have it yet. This legacy form works on core 2.x AND
 * (with a deprecation warning, not an error) on core 3.x, so it's
 * the safer choice across setups.
 */

#include <Arduino.h>

// ---- Pin assignments ----
const int IN1_PIN = 27;
const int IN2_PIN = 26;
const int ENA_PIN = 25;

// ---- Tunables ----
const unsigned long ACTUATOR_TRAVEL_TIME_MS = 5000;   // 0.83" stroke @ 0.2"/s no-load (~4.2s) + margin for load
const unsigned long PAUSE_AT_END_MS = 500;            // brief pause once fully extended/retracted
const int MOTOR_SPEED = 180;                          // 0-255 PWM duty cycle; ~70% = moderate speed

const int PWM_CHANNEL = 0;
const int PWM_FREQ_HZ = 1000;
const int PWM_RESOLUTION_BITS = 8; // gives duty range 0-255

void extendActuator();
void retractActuator();
void stopActuator();

void setup() {
  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);

  // Configure ENA as a PWM output (legacy ledc API, channel-based).
  ledcSetup(PWM_CHANNEL, PWM_FREQ_HZ, PWM_RESOLUTION_BITS);
  ledcAttachPin(ENA_PIN, PWM_CHANNEL);

  stopActuator();
}

void loop() {
  extendActuator();
  delay(ACTUATOR_TRAVEL_TIME_MS);
  stopActuator();
  delay(PAUSE_AT_END_MS);

  retractActuator();
  delay(ACTUATOR_TRAVEL_TIME_MS);
  stopActuator();
  delay(PAUSE_AT_END_MS);
}

void extendActuator() {
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
  ledcWrite(PWM_CHANNEL, MOTOR_SPEED);
}

void retractActuator() {
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, HIGH);
  ledcWrite(PWM_CHANNEL, MOTOR_SPEED);
}

void stopActuator() {
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);
  ledcWrite(PWM_CHANNEL, 0);
}