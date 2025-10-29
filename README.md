# lua-gpiod

A comprehensive Lua C library wrapper for libgpiod, providing complete GPIO control functionality for Linux systems.

## Overview

This library provides a complete Lua binding for the libgpiod library, allowing you to control GPIO pins on Linux systems from Lua scripts. It supports all major gpiod features including input/output operations, event handling, bulk operations, and chip enumeration.

## Language Versions

- [English](README.md)
- [中文](README_CN.md)

## Features

- **Complete GPIO Control**: Input/output operations, value reading/writing
- **Event Handling**: Rising edge, falling edge, and both edge event detection
- **Bulk Operations**: Efficient multi-line GPIO operations
- **Chip Management**: Enumerate and manage multiple GPIO chips
- **Line Properties**: Access line names, consumers, directions, bias settings
- **Precise Timing**: Built-in precise sleep function for timing-critical applications

## Requirements

- Linux system with GPIO support
- libgpiod library installed
- Lua 5.4 (or compatible version)
- GCC compiler

## Installation

### Install Dependencies

On Ubuntu/Debian:
```bash
sudo apt-get install libgpiod-dev lua5.4-dev build-essential
```

On Fedora/RHEL:
```bash
sudo dnf install libgpiod-devel lua-devel gcc
```

### Build the Library

```bash
make
```

### Install (Optional)

```bash
sudo make install
```

## Usage

### Basic Example

```lua
local gpiod = require("gpiod")

-- Open GPIO chip
local chip = gpiod.chip_open("gpiochip0")

-- Get a GPIO line
local line = chip:get_line(18)

-- Configure as output
line:request_output("my_app", 0)

-- Set high
line:set_value(1)

-- Clean up
line:release()
chip:close()
```

### Input with Event Detection

```lua
local gpiod = require("gpiod")

local chip = gpiod.chip_open("gpiochip0")
local line = chip:get_line(24)

-- Request rising edge events
line:request_rising_edge_events("button_monitor")

print("Waiting for button press...")
if line:event_wait(5.0) then  -- 5 second timeout
    local event = line:event_read()
    print("Button pressed at:", event:timestamp())
    print("Event type:", event:event_type())
end

line:release()
chip:close()
```

### Bulk Operations

```lua
local gpiod = require("gpiod")

local chip = gpiod.chip_open("gpiochip0")
local lines = chip:get_lines({18, 19, 20, 21})

-- Configure all as outputs with initial values
lines:request_output("led_controller", {0, 1, 0, 1})

-- Set all values at once
lines:set_values({1, 1, 1, 1})

-- Read all values
local values = lines:get_values()
for i, value in ipairs(values) do
    print("Line " .. (i-1) .. ": " .. value)
end

lines:release()
chip:close()
```

## API Reference

### Module Functions

- `gpiod.chip_open(name_or_number)` - Open GPIO chip by name or number
- `gpiod.chip_iter()` - Create chip iterator
- `gpiod.sleep(seconds)` - Precise sleep function
- `gpiod.version()` - Get libgpiod version

### Chip Methods

- `chip:get_line(offset)` - Get single GPIO line
- `chip:get_lines(offsets)` - Get multiple GPIO lines
- `chip:find_line(name)` - Find line by name
- `chip:get_all_lines()` - Get all available lines
- `chip:name()` - Get chip name
- `chip:label()` - Get chip label
- `chip:num_lines()` - Get number of lines
- `chip:close()` - Close chip

### Line Methods

#### Configuration
- `line:request_input(consumer, [flags])` - Configure as input
- `line:request_output(consumer, default_val, [flags])` - Configure as output
- `line:release()` - Release line

#### Value Operations
- `line:get_value()` - Read current value
- `line:set_value(value)` - Set output value

#### Event Handling
- `line:request_rising_edge_events(consumer)` - Monitor rising edges
- `line:request_falling_edge_events(consumer)` - Monitor falling edges
- `line:request_both_edges_events(consumer)` - Monitor both edges
- `line:event_wait(timeout)` - Wait for event
- `line:event_read()` - Read event
- `line:event_get_fd()` - Get event file descriptor

#### Properties
- `line:offset()` - Get line offset
- `line:name()` - Get line name
- `line:consumer()` - Get current consumer
- `line:direction()` - Get direction ("input"/"output")
- `line:active_state()` - Get active state ("high"/"low")
- `line:bias()` - Get bias setting
- `line:is_used()` - Check if line is in use
- `line:is_open_drain()` - Check open drain configuration
- `line:is_open_source()` - Check open source configuration
- `line:update()` - Update line information

### Line Bulk Methods

- `bulk:num_lines()` - Get number of lines
- `bulk:get_line(index)` - Get line by index
- `bulk:request_input(consumer, [flags])` - Configure all as inputs
- `bulk:request_output(consumer, values, [flags])` - Configure all as outputs
- `bulk:get_values()` - Read all values
- `bulk:set_values(values)` - Set all values
- `bulk:release()` - Release all lines

### Event Methods

- `event:event_type()` - Get event type ("rising_edge"/"falling_edge")
- `event:timestamp()` - Get event timestamp

### Constants

#### Request Flags
- `gpiod.OPEN_DRAIN` - Open drain output
- `gpiod.OPEN_SOURCE` - Open source output
- `gpiod.ACTIVE_LOW` - Active low logic
- `gpiod.BIAS_DISABLE` - Disable bias
- `gpiod.BIAS_PULL_DOWN` - Pull-down bias
- `gpiod.BIAS_PULL_UP` - Pull-up bias

## Testing

### Local Testing

Run the example:
```bash
lua gpiod_example.lua
```

Test module loading:
```bash
make test
```

### Docker Testing

Build and test using Docker (recommended for CI/CD):

```bash
# Build the Docker image
docker build -t lua-gpiod-test .

# Run tests in container
docker run --rm lua-gpiod-test

# Interactive shell for manual testing
docker run --rm -it lua-gpiod-test /bin/bash
```

The Docker environment includes:
- Debian bookworm-slim base image
- All required dependencies (libgpiod, Lua 5.4, build tools)
- Automatic build and basic functionality tests
- Health check for module loading verification

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Repository

This project is hosted at: https://github.com/HsOjo/lua-gpiod

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests at the GitHub repository.

## Troubleshooting

### Permission Issues
If you get permission errors, you may need to add your user to the `gpio` group:
```bash
sudo usermod -a -G gpio $USER
```

### Missing GPIO Chips
List available GPIO chips:
```bash
gpiodetect
```

### Library Not Found
Make sure libgpiod is installed and the shared library is in the correct path.
