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
#include "Console.hpp"


#include "hal-common/event/Loop.hpp"
#include "hal-common/SerialLineStringWriter.hpp"
#include "hal-feather-m0/SerialLine_ArduinoUSB.hpp"


namespace lr {
namespace Console {


/// The delay for the wait animation.
///
const Milliseconds cWaitAnimationDelay = 300_ms;

/// The serial line to use.
///
SerialLine_ArduinoUSB gSerialLine;

/// A string writer to write to this serial line.
///
SerialLineStringWriter gSerialWriter(&gSerialLine);

/// The display to control.
///
lcd::CharacterDisplay *gDisplay;


// Forward declarations.
void waitForUsbSerialEvent();
void waitForInput();


Status initialize(lcd::CharacterDisplay *display)
{
    gDisplay = display;

    // Initialize the USB serial line and wait for a connection.
    gSerialLine.initialize();

    // Enable the back light
    if (hasError(gDisplay->setBacklightEnabled(true))) {
        return Status::Error;
    }

    // Write some text, ignore any errors.
    gDisplay->writeText(String("Waiting for"));
    gDisplay->setCursor(0, 1);
    gDisplay->writeText(String("USB serial..."));

    // Start an event to wait for the USB connection.
    event::mainLoop().addDelayedEvent(&waitForUsbSerialEvent, cWaitAnimationDelay);

    return Status::Success;
}


void waitForUsbSerialEvent()
{
    const uint8_t waitCharacterCount = 4;
    const char waitCharacters[] = "<v>^";
    static uint8_t waitCharacterIndex = 0;

    gDisplay->setCursor(0, 2);
    if (gSerialLine.isReady()) {
        gDisplay->writeText("OK!");
        gSerialWriter.writeLine("*** Welcome to the LCD demo! ***");
        gSerialWriter.writeLine("Use the command 'help' to get help.");
        gSerialWriter.write("lcd-demo> ");
        event::mainLoop().addRepeatedEvent(&waitForInput, 100_ms);
    } else {
        gDisplay->writeChar(waitCharacters[waitCharacterIndex]);
        waitCharacterIndex = (waitCharacterIndex+1) % waitCharacterCount;
        event::mainLoop().addDelayedEvent(&waitForUsbSerialEvent, cWaitAnimationDelay);
    }
}


void waitForInput()
{
    // FIXME!
}


}
}

