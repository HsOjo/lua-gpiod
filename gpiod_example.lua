#!/usr/bin/env lua
--[[
Complete gpiod Lua wrapper library usage example
Demonstrates how to use the generic gpiod library for GPIO operations
]]

-- Add current directory to package path
local gpiod = require("gpiod")

print("=== gpiod Lua Wrapper Library Example ===")
print("gpiod version:", gpiod.version())

-- Open GPIO chip
local chip = gpiod.chip_open("gpiochip4")
print("Chip name:", chip:name())
print("Chip label:", chip:label())
print("Number of GPIO lines:", chip:num_lines())

-- Get GPIO line (e.g., GPIO 18)
local line = chip:get_line(18)
print("GPIO line offset:", line:offset())
print("GPIO line name:", line:name() or "No name")
print("Is line used:", line:is_used())

-- Request output mode
line:request_output("example", 0)
print("GPIO direction:", line:direction())
print("Consumer:", line:consumer())

-- LED blinking example
print("Starting GPIO 18 blinking (5 times)...")
for i = 1, 5 do
    print("Setting high level")
    line:set_value(1)
    gpiod.sleep(0.5)
    
    print("Setting low level")
    line:set_value(0)
    gpiod.sleep(0.5)
end

-- Clean up resources
line:release()
chip:close()
print("Example completed")