/*
  laser.cpp - Laser control library for Arduino using 16 bit timers- Version 1
  Copyright (c) 2013 Timothy Schmidt.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "Configuration.h"
#ifdef LASER
#include "laser.h"
#include "pins.h"
#include <avr/interrupt.h>
#include <Arduino.h>
#include "Marlin.h"

laser_t laser;

void laser_init()
{
  uint8_t pin; 
  // Initialize pwm for laser intensity control
  pmc_enable_periph_clk(PWM_INTERFACE_ID);
  PWMC_ConfigureClocks(LASER_PWM*LASER_PWM_MAX_DUTY_CYCLE, 0, VARIANT_MCK); // Use ClockA
#if LASER_CONTROL == 1

  pin = LASER_FIRING_PIN;

#elseif LASER_CONTROL == 2

  pin = LASER_INTENSITY_PIN;

#endif
  if (pin < 6 || pin > 9)
    return;

  laser.chan = g_APinDescription[pin].ulPWMChannel; 
  PIO_Configure( g_APinDescription[pin].pPort,  g_APinDescription[pin].ulPinType,
		 g_APinDescription[pin].ulPin,  g_APinDescription[pin].ulPinConfiguration);
  PWMC_ConfigureChannel(PWM_INTERFACE, laser.chan, PWM_CMR_CPRE_CLKA, 0, 0);
  PWMC_SetPeriod(PWM_INTERFACE, laser.chan, LASER_PWM_MAX_DUTY_CYCLE);
  PWMC_SetDutyCycle(PWM_INTERFACE, laser.chan, 0);  // The 0 is the initial duty cycle
  PWMC_EnableChannel(PWM_INTERFACE, laser.chan);

#ifdef LASER_PERIPHERALS
  digitalWrite(LASER_PERIPHERALS_PIN, HIGH);  // Laser peripherals are active LOW, so preset the pin
  pinMode(LASER_PERIPHERALS_PIN, OUTPUT);

  digitalWrite(LASER_PERIPHERALS_STATUS_PIN, HIGH);  // Set the peripherals status pin to pull-up.
  pinMode(LASER_PERIPHERALS_STATUS_PIN, INPUT);
#endif // LASER_PERIPHERALS

  digitalWrite(LASER_FIRING_PIN, LASER_UNARM);  // Laser FIRING is active LOW, so preset the pin
  pinMode(LASER_FIRING_PIN, OUTPUT);

  // initialize state to some sane defaults
  laser.intensity = 100.0;
  laser.ppm = 0.0;
  laser.duration = 0;
  laser.status = LASER_OFF;
  laser.firing = LASER_OFF;
  laser.mode = CONTINUOUS;
  laser.last_firing = 0;
  laser.diagnostics = false;
  laser.time = 0;
#ifdef LASER_RASTER
  laser.raster_aspect_ratio = LASER_RASTER_ASPECT_RATIO;
  laser.raster_mm_per_pulse = LASER_RASTER_MM_PER_PULSE;
  laser.raster_direction = 1;
#endif // LASER_RASTER
#ifdef MUVE_Z_PEEL
  laser.peel_distance = 2.0;
  laser.peel_speed = 2.0;
  laser.peel_pause = 0.0;
#endif // MUVE_Z_PEEL
  
  laser_extinguish();
}

void laser_fire(int intensity = 100.0){
  laser.firing = LASER_ON;
  laser.last_firing = micros(); // microseconds of last laser firing
  if (intensity > 100.0) intensity = 100.0; // restrict intensity between 0 and 100
  if (intensity < 0) intensity = 0;

  PWMC_SetDutyCycle(PWM_INTERFACE, laser.chan, LASER_PWM_MAX_DUTY_CYCLE*intensity/100.0);
  PWMC_EnableChannel(PWM_INTERFACE, laser.chan);

#if LASER_CONTROL == 2
  digitalWrite(LASER_FIRING_PIN, LASER_ARM);
#endif
  
  if (laser.diagnostics) {
    SERIAL_ECHOLN("Laser fired");
  }
}

void laser_extinguish(){
  if (laser.firing == LASER_ON) {
    laser.firing = LASER_OFF;
    
    PWMC_SetDutyCycle(PWM_INTERFACE, laser.chan, 0);
    digitalWrite(LASER_FIRING_PIN, LASER_UNARM);
    laser.time += millis() - (laser.last_firing / 1000);
    
    if (laser.diagnostics) {
      SERIAL_ECHOLN("Laser extinguished");
    }
  }
}
void laser_set_mode(int mode){
  switch(mode){
  case 0:
    laser.mode = CONTINUOUS;
    return;
  case 1:
    laser.mode = PULSED;
    return;
  case 2:
    laser.mode = RASTER;
    return;
  }
}
#ifdef LASER_PERIPHERALS
bool laser_peripherals_ok(){
  return !digitalRead(LASER_PERIPHERALS_STATUS_PIN);
}
void laser_peripherals_on(){
  digitalWrite(LASER_PERIPHERALS_PIN, LOW);
  if (laser.diagnostics) {
    SERIAL_ECHO_START;
    SERIAL_ECHOLNPGM("Laser Peripherals Enabled");
  }
}
void laser_peripherals_off(){
  if (!digitalRead(LASER_PERIPHERALS_STATUS_PIN)) {
    digitalWrite(LASER_PERIPHERALS_PIN, HIGH);
    if (laser.diagnostics) {
      SERIAL_ECHO_START;
      SERIAL_ECHOLNPGM("Laser Peripherals Disabled");
    }
  }
}
void laser_wait_for_peripherals() {
  unsigned long timeout = millis() + LASER_PERIPHERALS_TIMEOUT;
  if (laser.diagnostics) {
    SERIAL_ECHO_START;
    SERIAL_ECHOLNPGM("Waiting for peripheral control board signal...");
  }
  while(!laser_peripherals_ok()) {
    if (millis() > timeout) {
      if (laser.diagnostics) {
	SERIAL_ERROR_START;
	SERIAL_ERRORLNPGM("Peripheral control board failed to respond");
      }
      Stop();
      break;
    }
  }
}
#endif // LASER_PERIPHERALS
#endif
