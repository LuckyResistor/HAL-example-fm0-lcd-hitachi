//
// (c)2019 by Lucky Resistor. See LICENSE for details.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#include "Application.hpp"


#include "Console.hpp"

#include "hal-common/event/Loop.hpp"
#include "hal-common/Timer.hpp"
#include "hal-feather-m0/GPIO_Pin_FeatherM0.hpp"
#include "hal-lcd-hitachi/AfBackConnection.hpp"
#include "hal-lcd-hitachi/HDisplay.hpp"
#include "hal-feather-m0/WireMaster_FeatherM0.hpp"


#pragma clang diagnostic ignored "-Wmissing-noreturn"


namespace lr {
namespace Application {


// Create the I2C channel
WireMaster_FeatherM0 gWireDisplay(WireMaster_FeatherM0::Setup::A3_A4);

// Setup the LCD display.
MCP23008 gDisplayIO(&gWireDisplay, MCP23008::Address0);
lcd::AfBackConnection gDisplayConnection(&gDisplayIO);
lcd::HDisplay gDisplay(&gDisplayConnection, 4, 20);

// The event loop.
event::BasicLoop<event::StaticStorage<16>> gEventLoop;

// The pin with the LED.
const auto gLedPin = GPIO::Pin13();


__attribute__((noreturn))
void signalError()
{
    while (true) {
        gLedPin.toggleOutput();
        Timer::delay(200_ms);
    }
}


void setup()
{
    // Set the LED as output.
    gLedPin.configureAsOutput();

    // Initialize the I2C bus
    if (hasError(gWireDisplay.initialize())) {
        signalError();
    }

    // --- Initialize the Display.
    // Initialize and check the IO expander.
    if (hasError(gDisplayIO.initialize())) {
        signalError();
    }
    if (hasError(gDisplayIO.test())) {
        signalError();
    }
    // Initialize the display
    if (hasError(gDisplay.initialize())) {
        signalError();
    }

    // --- Initialize the console.
    if (hasError(Console::initialize(&gDisplay))) {
        signalError();
    }
}


void loop()
{
    gEventLoop.loopOnce();
}


}
}

