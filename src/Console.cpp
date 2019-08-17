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
#include "hal-common/EnumStringMap.hpp"
#include "hal-common/FreeMemory.hpp"
#include "hal-common/SerialLineShell.hpp"
#include "hal-common/StringTokenizer.hpp"
#include "hal-feather-m0/SerialLine_ArduinoUSB.hpp"


namespace lr {
namespace Console {


/// The available commands.
///
enum class Command {
    None,
    Help,
    Write,
    Char,
    Clear,
    Reset,
    Enable,
    Disable,
    Scroll,
    Cursor,
    Backlight,
    FreeRam,
};

/// The delay for the wait animation.
///
const Milliseconds cWaitAnimationDelay = 300_ms;

/// The entries for the command map.
///
const EnumStringMap<Command>::Entry cCommandMapEntries[] = {
    {Command::Help, "help"},
    {Command::Write, "write"},
    {Command::Char, "char"},
    {Command::Clear, "clear"},
    {Command::Reset, "reset"},
    {Command::Enable, "enable"},
    {Command::Disable, "disable"},
    {Command::Scroll, "scroll"},
    {Command::Cursor, "cursor"},
    {Command::Backlight, "backlight"},
    {Command::FreeRam, "freeram"},
    {Command::None, ""}, // default
    {Command::None, nullptr}, // end mark.
};
const EnumStringMap<Command> cCommandMap(cCommandMapEntries);

/// The cursor mode map.
///
using CursorMode = lcd::CharacterDisplay::CursorMode;
const EnumStringMap<CursorMode>::Entry cCursorModeEntries[] = {
    {CursorMode::Off, "off"},
    {CursorMode::Block, "block"},
    {CursorMode::Line, "line"},
    {CursorMode::Off, ""},
    {CursorMode::Off, nullptr}
};
const EnumStringMap<CursorMode> cCursorModeMap(cCursorModeEntries);

/// The scroll action map.
///
enum class ScrollAction {
    None,
    On,
    Off,
    Left,
    Right,
    Up,
    Down
};
const EnumStringMap<ScrollAction>::Entry cScrollActionEntries[] = {
    {ScrollAction::On, "on"},
    {ScrollAction::Off, "off"},
    {ScrollAction::Left, "left"},
    {ScrollAction::Right, "right"},
    {ScrollAction::Up, "up"},
    {ScrollAction::Down, "down"},
    {ScrollAction::None, ""},
    {ScrollAction::None, nullptr},
};
const EnumStringMap<ScrollAction> cScrollActionMap(cScrollActionEntries);

/// The serial line to use.
///
SerialLine_ArduinoUSB gSerialLine;

/// The shell.
///
SerialLineShell gShell(&gSerialLine);

/// The display to control.
///
lcd::CharacterDisplay *gDisplay;


// Forward declarations.
void waitForUsbSerialEvent();
void startShell();
void handleLine(const String &line);
SerialLineShell::LineExpansion handleLineExpansion(String &line, uint8_t &cursorPosition);


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
    const char waitCharacters[] = "^>v<";
    static uint8_t waitCharacterIndex = 0;

    gDisplay->setCursor(0, 2);
    if (gSerialLine.isReady()) {
        startShell();
    } else {
        gDisplay->writeChar(waitCharacters[waitCharacterIndex]);
        waitCharacterIndex = (waitCharacterIndex+1) % waitCharacterCount;
        event::mainLoop().addDelayedEvent(&waitForUsbSerialEvent, cWaitAnimationDelay);
    }
}


void startShell()
{
    // Send the welcome message.
    gDisplay->writeText("OK!");
    gShell.writeLine("*** Welcome to the LCD demo! ***");
    gShell.writeLine("Use the command 'help' to get help.");

    // Start an event for the shell.
    event::mainLoop().addPollEvent([]{
        gShell.pollEvent();
    });

    gShell.setPrompt(String("lcd-demo> "));
    gShell.setLineFn(&handleLine);
    gShell.setLineExpansionFn(&handleLineExpansion);
}


void handleHelp()
{
    gShell.write("Available commands: ");
    const auto commandCount = sizeof(cCommandMapEntries)/sizeof(EnumStringMap<Command>::Entry) - 2;
    for (uint8_t i = 0; i < commandCount; ++i) {
        if (i > 0) {
            gShell.write(", ");
        }
        gShell.write(cCommandMapEntries[i].str);
    }
    gShell.writeLine();
}


void handleWrite(const StringTokenizer &tokenizer)
{
    gDisplay->writeText(tokenizer.getTail());
}


void handleChar(StringTokenizer &tokenizer)
{
    const char *errorText = "Use with character number. <begin> [<end>]";
    const auto beginStr = tokenizer.getNextToken();
    uint8_t begin = 0;
    uint8_t end = 0;
    if (beginStr.isEmpty()) {
        gShell.writeLine(errorText);
        return;
    }
    const auto beginResult = beginStr.toUInt8();
    if (beginResult.isSuccess()) {
        begin = beginResult.getValue();
    } else {
        gShell.writeLine(errorText);
        return;
    }
    const auto endStr = tokenizer.getNextToken();
    if (endStr.isEmpty()) {
        end = begin;
    } else {
        const auto endResult = endStr.toUInt8();
        if (endResult.isSuccess()) {
            end = endResult.getValue();
        } else {
            gShell.writeLine(errorText);
            return;
        }
    }
    for (uint8_t c = begin; c < end; ++c) {
        gDisplay->writeChar(static_cast<char>(c));
    }
}


void handleClear()
{
    gDisplay->clear();
}


void handleCursor(StringTokenizer &tokenizer)
{
    const char *errorText = "Add x and y coordinate, or mode 'off', 'block', 'line'.";
    const auto xStr = tokenizer.getNextToken();
    if (xStr.isEmpty()) {
        gShell.writeLine(errorText);
        return;
    }
    const auto xResult = xStr.toUInt8();
    if (isSuccessful(xResult)) {
        // It is a number, check for a second coordinate.
        const auto yStr = tokenizer.getNextToken();
        if (yStr.isEmpty()) {
            gShell.writeLine(errorText);
            return;
        }
        const auto yResult = yStr.toUInt8();
        if (hasError(yResult)) {
            gShell.writeLine(errorText);
            return;
        }
        const uint8_t column = xResult.getValue();
        const uint8_t row = yResult.getValue();
        gShell.write("Move cursor to column: ");
        gShell.write(String::number(column));
        gShell.write(" row: ");
        gShell.writeLine(String::number(row));
        gDisplay->setCursor(column, row);
    } else {
        // Check if a mode is selected.
        const auto cursorMode = cCursorModeMap.value(xStr.getData());
        gShell.write("Set cursor mode to: ");
        gShell.writeLine(xStr);
        gDisplay->setCursorMode(cursorMode);
    }
}


void handleBacklight(StringTokenizer &tokenizer)
{
    const auto flag = tokenizer.getNextToken();
    if (flag == "on") {
        gDisplay->setBacklightEnabled(true);
    } else if (flag == "off") {
        gDisplay->setBacklightEnabled(false);
    } else {
        gShell.writeLine("Add parameter 'on' and 'off'.");
    }
}


void handleFreeRam()
{
    gShell.write(String::number(getFreeMemory()));
    gShell.writeLine(" bytes free RAM.");
}


void handleReset()
{
    gDisplay->reset();
}


void handleEnable()
{
    gDisplay->setEnabled(true);
}


void handleDisable()
{
    gDisplay->setEnabled(false);
}


void handleScroll(StringTokenizer &tokenizer)
{
    const auto token = tokenizer.getNextToken();
    const auto action = cScrollActionMap.value(token.getData());
    switch (action) {
    case ScrollAction::On: gDisplay->setAutoScrollEnabled(true); break;
    case ScrollAction::Off: gDisplay->setAutoScrollEnabled(false); break;
    case ScrollAction::Left: gDisplay->scroll(Direction::Left); break;
    case ScrollAction::Right: gDisplay->scroll(Direction::Right); break;
    case ScrollAction::Up: gDisplay->scroll(Direction::Up); break;
    case ScrollAction::Down: gDisplay->scroll(Direction::Down); break;
    default: break;
    }
}


void handleLine(const String &line)
{
    StringTokenizer tokenizer(line, ' ');
    if (tokenizer.hasNextToken()) {
        const auto token = tokenizer.getNextToken();
        const auto command = cCommandMap.value(token.getData());
        switch (command) {
        case Command::Help: handleHelp(); break;
        case Command::Write: handleWrite(tokenizer); break;
        case Command::Char: handleChar(tokenizer); break;
        case Command::Clear: handleClear(); break;
        case Command::Reset: handleReset(); break;
        case Command::Enable: handleEnable(); break;
        case Command::Disable: handleDisable(); break;
        case Command::Scroll: handleScroll(tokenizer); break;
        case Command::Cursor: handleCursor(tokenizer); break;
        case Command::Backlight: handleBacklight(tokenizer); break;
        case Command::FreeRam: handleFreeRam(); break;
        default:
            gShell.write("Unknown command '");
            gShell.write(token);
            gShell.writeLine("'.");
            break;
        }
    }
}


SerialLineShell::LineExpansion handleLineExpansion(String &line, uint8_t &cursorPosition)
{
    return SerialLineShell::LineExpansion::Failed;
}


}
}

