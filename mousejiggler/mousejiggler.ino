// Pro Micro Mouse Jiggler - by Widge, 2025
// Start/stop button + external status LEDs added 2026
//
// TX LED blinks each jiggle, RX LED heartbeats every 2s (while jiggling)
// All LEDs off if no host is enumerated
//
// --- WIRING -------------------------------------------------------------
//  Button : pin 7  ->  button  ->  GND        (internal pullup, no resistor)
//  RUN LED: pin 9  ->  resistor -> LED -> GND (on while jiggling)
//  IDLE LED:pin 10 ->  resistor -> LED -> GND (on while idle)
//  Use ~220-1k ohm series resistors. Pins 9 and 10 are hardware PWM, so
//  brightness is adjustable below.
// ------------------------------------------------------------------------


// == SET YOUR SPECIFIC VALUES HERE ===========================
// ||                                                        ||
// || How long should one press keep the jiggler running?     ||
      constexpr unsigned long windowMinutes = 60;
// ||                                                        ||
// || How long before the window ends should the RUN LED     ||
// || start flashing as a warning? (in seconds)              ||
      constexpr unsigned long warningSeconds = 120;
// ||                                                        ||
// || LED brightness, 0 (off) to 255 (full). Change this one ||
// || number to dim/brighten both LEDs.                      ||
      constexpr uint8_t ledBrightnessRun  = 30;
      constexpr uint8_t ledBrightnessIdle = 150;
// ||                                                        ||
// || Warning flash timing: one flash every flashPeriod ms,  ||
// || lit for flashOn ms of it.                              ||
      constexpr unsigned long flashPeriod = 500;
      constexpr unsigned long flashOn     = 250;
// ||                                                        ||
// || When active, how often should we wait between jiggles? ||
      constexpr int periodicityMinutes = 1;  // in minutes   ||
      constexpr int periodicitySeconds = 5; // and seconds  ||
// ||                                                        ||
// || How many pixels should we jiggle?                      ||
      constexpr int jiggle = 2;
// ||                                                        ||
// || How fast should we jiggle (in milliseconds)            ||
// || This is the delay between when the mouse is moved to   ||
// || the right and back to the left. A shorter delay makes  ||
// || it less perceptible to humans, but too short can cause ||
// || it to also fail to register to the computer. Increase  ||
// || this value if the jiggle fails to prevent inactivity.  ||
      constexpr int jiggleFreeze = 5;
// ||                                                        ||
// || When we jiggle, how many times should we blink the TX  ||
// || light?                                                 ||
      constexpr int blinks = 3;
// ||                                                        ||
// || When active, How often should we pulse the RX light to ||
// || indicate life? (in seconds                             ||
      constexpr int heartbeat = 2;
// ||                                                        ||
// || Which pins? (9 and 10 must stay PWM-capable)           ||
      constexpr uint8_t pinButton  = 7;
      constexpr uint8_t pinLedRun  = 10;
      constexpr uint8_t pinLedIdle = 9;
// ||                                                        ||
// || Button debounce time (ms). Raise if a single press     ||
// || ever registers twice.                                  ||
      constexpr unsigned long debounceMs = 40;
// ||                                                        ||
// ============================================================



#include <Mouse.h>

bool stateActive = false;                 // is a USB host enumerated?
bool jiggling    = false;                 // are we inside the run window?

unsigned long windowStart = 0;
constexpr unsigned long windowMs  = windowMinutes * 60000UL;
constexpr unsigned long warningMs = warningSeconds * 1000UL;

unsigned long previousMillis = 0;
constexpr unsigned long interval = (periodicityMinutes * 60000UL) + (periodicitySeconds * 1000UL);

unsigned long ledMillis = 0;
constexpr unsigned long ledPeriod = heartbeat * 1000UL;
bool ledOn = false;

bool txBlinkActive = false;
int txBlinkCount = 0;
unsigned long txLastToggle = 0;
constexpr unsigned long txBlinkInterval = 200; // ms

// Button debounce state
bool buttonRaw    = false;   // last raw reading (true = pressed)
bool buttonStable = false;   // last debounced reading
unsigned long buttonChanged = 0;

// Host polling
unsigned long hostPollMillis = 0;
constexpr unsigned long hostPollInterval = 250; // ms


static inline bool usbConfigured() {
  // On 32U4 Arduino cores, this reports true once the host has enumerated the CDC device.
  USBDevice.poll();
  return USBDevice.configured() && !USBDevice.isSuspended();
}


// --- External status LEDs ------------------------------------------------

void ledsOff() {
  analogWrite(pinLedRun, 0);
  analogWrite(pinLedIdle, 0);
}


void updateIndicators() {
  if (!jiggling) {
    analogWrite(pinLedRun, 0);
    analogWrite(pinLedIdle, ledBrightnessIdle);
    return;
  }

  analogWrite(pinLedIdle, 0);

  const unsigned long elapsed = millis() - windowStart;
  const unsigned long remaining = (elapsed >= windowMs) ? 0 : (windowMs - elapsed);

  if (remaining <= warningMs) {
    // About to expire: one flash every flashPeriod ms.
    const bool lit = ((millis() - windowStart) % flashPeriod) < flashOn;
    analogWrite(pinLedRun, lit ? ledBrightnessRun : 0);
  } else {
    analogWrite(pinLedRun, ledBrightnessRun);
  }
}


// --- Start / stop --------------------------------------------------------

void doJiggle() {
  Mouse.move(jiggle, 0, 0);
  delay(jiggleFreeze);     // tiny pause so host sees two distinct reports
  Mouse.move(-jiggle, 0, 0);
}


void triggerTxBlink() {
  txBlinkActive = true;
  txLastToggle = 0;
  txBlinkCount = 0;
  TXLED0; // ensure OFF before starting
}


void startJiggling() {
  jiggling = true;
  windowStart = millis();
  ledMillis = millis();
  ledOn = false;
  RXLED0;

  doJiggle();               // confirm it works the moment the button is pressed
  previousMillis = millis();
  triggerTxBlink();
}


void stopJiggling() {
  jiggling = false;
  txBlinkActive = false;
  TXLED0;
  RXLED0;
}


void updateButton() {
  const unsigned long now = millis();
  const bool raw = (digitalRead(pinButton) == LOW);   // INPUT_PULLUP: LOW = pressed

  if (raw != buttonRaw) {
    buttonRaw = raw;
    buttonChanged = now;
    return;
  }

  if ((now - buttonChanged) >= debounceMs && raw != buttonStable) {
    buttonStable = raw;
    if (buttonStable) {                 // act on the press, not the release
      if (jiggling) stopJiggling();
      else          startJiggling();
    }
  }
}


// --- On-board TX/RX LEDs -------------------------------------------------

void updateTxBlink() {
  if (!txBlinkActive) return;
  unsigned long now = millis();

  if (txLastToggle == 0 || (now - txLastToggle) >= txBlinkInterval) {
    txLastToggle = now;

    // Toggle ON/OFF each call
    if (txBlinkCount % 2 == 0) {
      TXLED1; // ON
    } else {
      TXLED0; // OFF
    }
    txBlinkCount++;

    // Blink away
    if (txBlinkCount >= (blinks *2)) {
      txBlinkActive = false;
      TXLED0; // ensure OFF at the end
    }
  }
}


void jigglypuff() {
  const unsigned long now = millis();

  // --- Heartbeat on RX LED (quick blink to show sketch is alive) ---
  if (now - ledMillis >= ledPeriod) {
    ledMillis = now;
    ledOn = !ledOn;
    if (ledOn) RXLED1; else RXLED0;
  }

  // --- Periodic mouse jiggle ---
  if (now - previousMillis >= interval) {
    previousMillis = now;
    doJiggle();
    triggerTxBlink();
  }
  updateTxBlink();
}


void setup() {
  TX_RX_LED_INIT;
  TXLED0; RXLED0;

  pinMode(pinButton, INPUT_PULLUP);
  pinMode(pinLedRun, OUTPUT);
  pinMode(pinLedIdle, OUTPUT);
  ledsOff();

  Mouse.begin();

  // Seed the debouncer with the resting state so a held pin at boot
  // isn't read as a fresh press.
  buttonRaw = buttonStable = (digitalRead(pinButton) == LOW);
  buttonChanged = millis();

  stateActive = usbConfigured();
  hostPollMillis = millis();
}


void loop() {
  const unsigned long now = millis();

  // Keep an eye on the host without blocking the button.
  if (now - hostPollMillis >= hostPollInterval) {
    hostPollMillis = now;
    stateActive = usbConfigured();
  }

  // The run window expires on real time, host or no host.
  if (jiggling && (now - windowStart) >= windowMs) {
    stopJiggling();
  }

  if (stateActive) {
    updateButton();
    if (jiggling) jigglypuff();
    updateIndicators();
  } else {
    // No computer connected: everything dark, button ignored.
    TXLED0;
    RXLED0;
    ledsOff();
  }
}