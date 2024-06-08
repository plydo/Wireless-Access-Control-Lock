// rdm6300.cpp - support for the RDM6300 that works with 125kHz EM4100 RFIDs
/*
Copyright 2024 Mark Pickhard
Copyright rights associated with this file are nonexclusively transferred to The Bodgery Inc,
  a 501c(3) nonprofit entity.
This file is part of WACL. WACL is free software: you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.
WACL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
  Public License for more details.
You should have received a copy of the GNU General Public License along with WACL. If not, see
  <https://www.gnu.org/licenses/>.
*/

/*
NOTE:
This is currently set up for ESP32 using RX2 = GPIO16 = Serial2 (Hardware Serial)

The ESP32 is able to reassign any of the three UARTs to different pins using
a feature called either GPIO Matrix, Pin Mux, or IO MUX.
*/
#include "main.h"

#include <rdm6300.h>

Rdm6300 rdm6300obj;

void rdm6300Class::begin(int8_t rxPin, int8_t txPin)
{
  Serial2.begin(RDM6300_BAUDRATE, SERIAL_8N1, rxPin, txPin);
  rdm6300obj.set_tag_timeout(RDM6300_TIMEOUT);
  rdm6300obj.begin(&Serial2);
}
uint32_t rdm6300Class::tagID() { return rdm6300obj.get_tag_id(); }
uint32_t rdm6300Class::newTagID() { return rdm6300obj.get_new_tag_id(); }
void rdm6300Class::setTimeout(uint32_t x) { rdm6300obj.set_tag_timeout(x); }