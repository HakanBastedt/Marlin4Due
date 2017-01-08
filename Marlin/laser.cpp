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
#include "HAL.h"
#include "pins.h"
#include <avr/interrupt.h>
#include <Arduino.h>
#include "Marlin.h"

laser_t laser;

void laser_init()
{
  laser_init_pwm(LASER_INTENSITY_PIN);
//  laserext_timer_start();
  laser_intensity(0);

#ifdef LASER_PERIPHERALS
  digitalWrite(LASER_PERIPHERALS_PIN, HIGH);  // Laser peripherals are active LOW, so preset the pin
  pinMode(LASER_PERIPHERALS_PIN, OUTPUT);

  digitalWrite(LASER_PERIPHERALS_STATUS_PIN, HIGH);  // Set the peripherals status pin to pull-up.
  pinMode(LASER_PERIPHERALS_STATUS_PIN, INPUT);
#endif // LASER_PERIPHERALS

#if LASER_CONTROL == 2
  digitalWrite(LASER_FIRING_PIN, LASER_UNARM);  // Laser FIRING is active LOW, so preset the pin
  pinMode(LASER_FIRING_PIN, OUTPUT);
#endif

  // initialize state to some sane defaults
  laser.intensity = 100.0;
  laser.ppm = 0.0;
  laser.duration = 0;
  laser.status = LASER_OFF;
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

void laser_fire(float intensity = 100.0) // Fire with range 0-100
{ 
  if (intensity > 100.0) intensity = 100.0; // restrict intensity between 0 and 100
  if (intensity < 0) intensity = 0;
  
  static const float OldRange = 100.0 - 0.0;
  static const float NewRange = (100 - LASER_SEVEN); //7% power on my unit outputs hardly any noticable burn at F3000 on paper, so adjust the raster contrast based off 7 being the lower. 7 still produces burns at slower feed rates, but getting less power than this isn't typically needed at slow feed rates.
  float NewValue = intensity * NewRange / OldRange + LASER_SEVEN;
  
  laser_intensity_bits((0.01*LASER_PWM_MAX_DUTY_CYCLE)*NewValue); // Range 0 - LASER_PWM_MAX_DUTY_CYCLE

#if LASER_CONTROL == 2
  digitalWrite(LASER_FIRING_PIN, LASER_ARM);
#endif
  
  if (laser.diagnostics) {
    SERIAL_ECHOLN("Laser fired");
  }
}

void laser_extinguish(){
  if (laser.diagnostics) {
    SERIAL_ECHOLN("Laser being extinguished");
  }
  
  laser_intensity_bits(0);
  
#if LASER_CONTROL == 2
  digitalWrite(LASER_FIRING_PIN, LASER_UNARM);
#endif
  
  laser.time += millis() - (laser.last_firing / 1000);
  
  if (laser.diagnostics) {
    SERIAL_ECHOLN("Laser extinguished");
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

void laser_diagnose()
{
  if (!laser.diagnostics)
    return;
  SERIAL_ECHO_START;
  SERIAL_ECHO("Intensity ");SERIAL_ECHOLN(laser.intensity);
  SERIAL_ECHO("Duration  ");SERIAL_ECHOLN(laser.duration);
  SERIAL_ECHO("Ppm       ");SERIAL_ECHOLN(laser.ppm);
  SERIAL_ECHO("Mode      ");SERIAL_ECHOLN(laser.mode);
  SERIAL_ECHO("Status    ");SERIAL_ECHOLN(laser.status);
  SERIAL_ECHO("Fired     ");SERIAL_ECHOLN(laser.fired);
  MYSERIAL.flush();
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
