/*
 * Complete gpiod Lua wrapper library
 * Provides all major gpiod functionality
 */

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <gpiod.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>
#include <sys/epoll.h>

// Metatable names
#define GPIOD_CHIP_MT "gpiod.chip"
#define GPIOD_LINE_MT "gpiod.line"
#define GPIOD_LINE_BULK_MT "gpiod.line_bulk"
#define GPIOD_LINE_EVENT_MT "gpiod.line_event"
#define GPIOD_CHIP_ITER_MT "gpiod.chip_iter"

// Chip structure
typedef struct {
    struct gpiod_chip *chip;
} LuaChip;

// Line structure
typedef struct {
    struct gpiod_line *line;
    struct gpiod_chip *chip; // Keep reference to chip
} LuaLine;

// Line Bulk structure
typedef struct {
    struct gpiod_line_bulk bulk;
    struct gpiod_chip *chip; // Keep reference to chip
} LuaLineBulk;

// Line Event structure
typedef struct {
    struct gpiod_line_event event;
} LuaLineEvent;

// Chip Iterator structure
typedef struct {
    struct gpiod_chip_iter *iter;
} LuaChipIter;

// Helper function: precise sleep
static void sleep_precise(double seconds) {
    struct timespec ts;
    ts.tv_sec = (time_t)seconds;
    ts.tv_nsec = (long)((seconds - ts.tv_sec) * 1000000000);
    nanosleep(&ts, NULL);
}

// ============================================================================
// Chip related functions
// ============================================================================

// gpiod.chip_open(name_or_number)
static int chip_open(lua_State *L) {
    const char *name = luaL_checkstring(L, 1);
    
    LuaChip *chip = (LuaChip *)lua_newuserdata(L, sizeof(LuaChip));
    chip->chip = NULL;
    
    // Try to open by name
    chip->chip = gpiod_chip_open_by_name(name);
    if (!chip->chip) {
        // Try to open by number
        char *endptr;
        unsigned int num = strtoul(name, &endptr, 10);
        if (*endptr == '\0') {
            chip->chip = gpiod_chip_open_by_number(num);
        }
    }
    
    if (!chip->chip) {
        return luaL_error(L, "Failed to open GPIO chip: %s", name);
    }
    
    luaL_getmetatable(L, GPIOD_CHIP_MT);
    lua_setmetatable(L, -2);
    
    return 1;
}

// chip:get_line(offset)
static int chip_get_line(lua_State *L) {
    LuaChip *chip = (LuaChip *)luaL_checkudata(L, 1, GPIOD_CHIP_MT);
    unsigned int offset = luaL_checkinteger(L, 2);
    
    if (!chip->chip) {
        return luaL_error(L, "Chip is closed");
    }
    
    LuaLine *line = (LuaLine *)lua_newuserdata(L, sizeof(LuaLine));
    line->line = gpiod_chip_get_line(chip->chip, offset);
    line->chip = chip->chip;
    
    if (!line->line) {
        return luaL_error(L, "Failed to get GPIO line: %d", offset);
    }
    
    luaL_getmetatable(L, GPIOD_LINE_MT);
    lua_setmetatable(L, -2);
    
    return 1;
}

// chip:get_lines(offsets_table)
static int chip_get_lines(lua_State *L) {
    LuaChip *chip = (LuaChip *)luaL_checkudata(L, 1, GPIOD_CHIP_MT);
    luaL_checktype(L, 2, LUA_TTABLE);
    
    if (!chip->chip) {
        return luaL_error(L, "Chip is closed");
    }
    
    // Get offset array
    int num_lines = lua_rawlen(L, 2);
    unsigned int *offsets = malloc(num_lines * sizeof(unsigned int));
    
    for (int i = 0; i < num_lines; i++) {
        lua_rawgeti(L, 2, i + 1);
        offsets[i] = luaL_checkinteger(L, -1);
        lua_pop(L, 1);
    }
    
    // Create line bulk object
    LuaLineBulk *bulk = (LuaLineBulk *)lua_newuserdata(L, sizeof(LuaLineBulk));
    gpiod_line_bulk_init(&bulk->bulk);
    bulk->chip = chip->chip;
    
    int ret = gpiod_chip_get_lines(chip->chip, offsets, num_lines, &bulk->bulk);
    free(offsets);
    
    if (ret < 0) {
        return luaL_error(L, "Failed to get GPIO line bulk");
    }
    
    luaL_getmetatable(L, GPIOD_LINE_BULK_MT);
    lua_setmetatable(L, -2);
    
    return 1;
}

// chip:find_line(name)
static int chip_find_line(lua_State *L) {
    LuaChip *chip = (LuaChip *)luaL_checkudata(L, 1, GPIOD_CHIP_MT);
    const char *name = luaL_checkstring(L, 2);
    
    if (!chip->chip) {
        return luaL_error(L, "Chip is closed");
    }
    
    struct gpiod_line *line = gpiod_chip_find_line(chip->chip, name);
    if (!line) {
        lua_pushnil(L);
        return 1;
    }
    
    LuaLine *lua_line = (LuaLine *)lua_newuserdata(L, sizeof(LuaLine));
    lua_line->line = line;
    lua_line->chip = chip->chip;
    
    luaL_getmetatable(L, GPIOD_LINE_MT);
    lua_setmetatable(L, -2);
    
    return 1;
}

// chip:get_all_lines()
static int chip_get_all_lines(lua_State *L) {
    LuaChip *chip = (LuaChip *)luaL_checkudata(L, 1, GPIOD_CHIP_MT);
    
    if (!chip->chip) {
        return luaL_error(L, "Chip is closed");
    }
    
    LuaLineBulk *bulk = (LuaLineBulk *)lua_newuserdata(L, sizeof(LuaLineBulk));
    gpiod_line_bulk_init(&bulk->bulk);
    bulk->chip = chip->chip;
    
    int ret = gpiod_chip_get_all_lines(chip->chip, &bulk->bulk);
    if (ret < 0) {
        return luaL_error(L, "Failed to get all GPIO lines");
    }
    
    luaL_getmetatable(L, GPIOD_LINE_BULK_MT);
    lua_setmetatable(L, -2);
    
    return 1;
}

// chip:name()
static int chip_name(lua_State *L) {
    LuaChip *chip = (LuaChip *)luaL_checkudata(L, 1, GPIOD_CHIP_MT);
    
    if (!chip->chip) {
        return luaL_error(L, "Chip is closed");
    }
    
    lua_pushstring(L, gpiod_chip_name(chip->chip));
    return 1;
}

// chip:label()
static int chip_label(lua_State *L) {
    LuaChip *chip = (LuaChip *)luaL_checkudata(L, 1, GPIOD_CHIP_MT);
    
    if (!chip->chip) {
        return luaL_error(L, "Chip is closed");
    }
    
    lua_pushstring(L, gpiod_chip_label(chip->chip));
    return 1;
}

// chip:num_lines()
static int chip_num_lines(lua_State *L) {
    LuaChip *chip = (LuaChip *)luaL_checkudata(L, 1, GPIOD_CHIP_MT);
    
    if (!chip->chip) {
        return luaL_error(L, "Chip is closed");
    }
    
    lua_pushinteger(L, gpiod_chip_num_lines(chip->chip));
    return 1;
}

// chip:close()
static int chip_close(lua_State *L) {
    LuaChip *chip = (LuaChip *)luaL_checkudata(L, 1, GPIOD_CHIP_MT);
    
    if (chip->chip) {
        gpiod_chip_close(chip->chip);
        chip->chip = NULL;
    }
    
    return 0;
}

// ============================================================================
// Line related functions
// ============================================================================

// line:request_input(consumer, [flags])
static int line_request_input(lua_State *L) {
    LuaLine *line = (LuaLine *)luaL_checkudata(L, 1, GPIOD_LINE_MT);
    const char *consumer = luaL_checkstring(L, 2);
    int flags = luaL_optinteger(L, 3, 0);
    
    if (!line->line) {
        return luaL_error(L, "Line is released");
    }
    
    int ret = gpiod_line_request_input_flags(line->line, consumer, flags);
    if (ret < 0) {
        return luaL_error(L, "Failed to request input mode");
    }
    
    lua_pushboolean(L, 1);
    return 1;
}

// line:request_output(consumer, default_val, [flags])
static int line_request_output(lua_State *L) {
    LuaLine *line = (LuaLine *)luaL_checkudata(L, 1, GPIOD_LINE_MT);
    const char *consumer = luaL_checkstring(L, 2);
    int default_val = luaL_checkinteger(L, 3);
    int flags = luaL_optinteger(L, 4, 0);
    
    if (!line->line) {
        return luaL_error(L, "Line is released");
    }
    
    int ret = gpiod_line_request_output_flags(line->line, consumer, flags, default_val);
    if (ret < 0) {
        return luaL_error(L, "Failed to request output mode");
    }
    
    lua_pushboolean(L, 1);
    return 1;
}

// line:get_value()
static int line_get_value(lua_State *L) {
    LuaLine *line = (LuaLine *)luaL_checkudata(L, 1, GPIOD_LINE_MT);
    
    if (!line->line) {
        return luaL_error(L, "Line is released");
    }
    
    int value = gpiod_line_get_value(line->line);
    if (value < 0) {
        return luaL_error(L, "Failed to read GPIO value");
    }
    
    lua_pushinteger(L, value);
    return 1;
}

// line:set_value(value)
static int line_set_value(lua_State *L) {
    LuaLine *line = (LuaLine *)luaL_checkudata(L, 1, GPIOD_LINE_MT);
    int value = luaL_checkinteger(L, 2);
    
    if (!line->line) {
        return luaL_error(L, "Line is released");
    }
    
    int ret = gpiod_line_set_value(line->line, value);
    if (ret < 0) {
        return luaL_error(L, "Failed to set GPIO value");
    }
    
    lua_pushboolean(L, 1);
    return 1;
}

// line:offset()
static int line_offset(lua_State *L) {
    LuaLine *line = (LuaLine *)luaL_checkudata(L, 1, GPIOD_LINE_MT);
    
    if (!line->line) {
        return luaL_error(L, "Line is released");
    }
    
    lua_pushinteger(L, gpiod_line_offset(line->line));
    return 1;
}

// line:name()
static int line_name(lua_State *L) {
    LuaLine *line = (LuaLine *)luaL_checkudata(L, 1, GPIOD_LINE_MT);
    
    if (!line->line) {
        return luaL_error(L, "Line is released");
    }
    
    const char *name = gpiod_line_name(line->line);
    if (name) {
        lua_pushstring(L, name);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

// line:consumer()
static int line_consumer(lua_State *L) {
    LuaLine *line = (LuaLine *)luaL_checkudata(L, 1, GPIOD_LINE_MT);
    
    if (!line->line) {
        return luaL_error(L, "Line is released");
    }
    
    const char *consumer = gpiod_line_consumer(line->line);
    if (consumer) {
        lua_pushstring(L, consumer);
    } else {
        lua_pushnil(L);
    }
    return 1;
}

// line:direction()
static int line_direction(lua_State *L) {
    LuaLine *line = (LuaLine *)luaL_checkudata(L, 1, GPIOD_LINE_MT);
    
    if (!line->line) {
        return luaL_error(L, "Line is released");
    }
    
    int direction = gpiod_line_direction(line->line);
    switch (direction) {
        case GPIOD_LINE_DIRECTION_INPUT:
            lua_pushstring(L, "input");
            break;
        case GPIOD_LINE_DIRECTION_OUTPUT:
            lua_pushstring(L, "output");
            break;
        default:
            lua_pushstring(L, "unknown");
            break;
    }
    return 1;
}

// line:is_used()
static int line_is_used(lua_State *L) {
    LuaLine *line = (LuaLine *)luaL_checkudata(L, 1, GPIOD_LINE_MT);
    
    if (!line->line) {
        return luaL_error(L, "Line is released");
    }
    
    lua_pushboolean(L, gpiod_line_is_used(line->line));
    return 1;
}

// line:release()
static int line_release(lua_State *L) {
    LuaLine *line = (LuaLine *)luaL_checkudata(L, 1, GPIOD_LINE_MT);
    
    if (line->line) {
        gpiod_line_release(line->line);
        line->line = NULL;
    }
    
    return 0;
}

// line:request_rising_edge_events(consumer)
static int line_request_rising_edge_events(lua_State *L) {
    LuaLine *line = (LuaLine *)luaL_checkudata(L, 1, GPIOD_LINE_MT);
    const char *consumer = luaL_checkstring(L, 2);
    
    if (!line->line) {
        return luaL_error(L, "Line is released");
    }
    
    int ret = gpiod_line_request_rising_edge_events(line->line, consumer);
    if (ret < 0) {
        return luaL_error(L, "Failed to request rising edge events");
    }
    
    lua_pushboolean(L, 1);
    return 1;
}

// line:request_falling_edge_events(consumer)
static int line_request_falling_edge_events(lua_State *L) {
    LuaLine *line = (LuaLine *)luaL_checkudata(L, 1, GPIOD_LINE_MT);
    const char *consumer = luaL_checkstring(L, 2);
    
    if (!line->line) {
        return luaL_error(L, "Line is released");
    }
    
    int ret = gpiod_line_request_falling_edge_events(line->line, consumer);
    if (ret < 0) {
        return luaL_error(L, "Failed to request falling edge events");
    }
    
    lua_pushboolean(L, 1);
    return 1;
}

// line:request_both_edges_events(consumer)
static int line_request_both_edges_events(lua_State *L) {
    LuaLine *line = (LuaLine *)luaL_checkudata(L, 1, GPIOD_LINE_MT);
    const char *consumer = luaL_checkstring(L, 2);
    
    if (!line->line) {
        return luaL_error(L, "Line is released");
    }
    
    int ret = gpiod_line_request_both_edges_events(line->line, consumer);
    if (ret < 0) {
        return luaL_error(L, "Failed to request both edges events");
    }
    
    lua_pushboolean(L, 1);
    return 1;
}

// line:request_rising_edge_events_flags(consumer, flags)
static int line_request_rising_edge_events_flags(lua_State *L) {
    LuaLine *line = (LuaLine *)luaL_checkudata(L, 1, GPIOD_LINE_MT);
    const char *consumer = luaL_checkstring(L, 2);
    int flags = luaL_checkinteger(L, 3);
    
    if (!line->line) {
        return luaL_error(L, "Line is released");
    }
    
    int ret = gpiod_line_request_rising_edge_events_flags(line->line, consumer, flags);
    if (ret < 0) {
        return luaL_error(L, "Failed to request rising edge events");
    }
    
    lua_pushboolean(L, 1);
    return 1;
}

// line:request_falling_edge_events_flags(consumer, flags)
static int line_request_falling_edge_events_flags(lua_State *L) {
    LuaLine *line = (LuaLine *)luaL_checkudata(L, 1, GPIOD_LINE_MT);
    const char *consumer = luaL_checkstring(L, 2);
    int flags = luaL_checkinteger(L, 3);
    
    if (!line->line) {
        return luaL_error(L, "Line is released");
    }
    
    int ret = gpiod_line_request_falling_edge_events_flags(line->line, consumer, flags);
    if (ret < 0) {
        return luaL_error(L, "Failed to request falling edge events");
    }
    
    lua_pushboolean(L, 1);
    return 1;
}

// line:request_both_edges_events_flags(consumer, flags)
static int line_request_both_edges_events_flags(lua_State *L) {
    LuaLine *line = (LuaLine *)luaL_checkudata(L, 1, GPIOD_LINE_MT);
    const char *consumer = luaL_checkstring(L, 2);
    int flags = luaL_checkinteger(L, 3);
    
    if (!line->line) {
        return luaL_error(L, "Line is released");
    }
    
    int ret = gpiod_line_request_both_edges_events_flags(line->line, consumer, flags);
    if (ret < 0) {
        return luaL_error(L, "Failed to request both edges events");
    }
    
    lua_pushboolean(L, 1);
    return 1;
}

// line:event_wait(timeout_sec)
static int line_event_wait(lua_State *L) {
    LuaLine *line = (LuaLine *)luaL_checkudata(L, 1, GPIOD_LINE_MT);
    lua_Number timeout = luaL_optnumber(L, 2, -1);
    
    if (!line->line) {
        return luaL_error(L, "Line is released");
    }
    
    struct timespec ts;
    struct timespec *timeout_ptr = NULL;
    
    if (timeout >= 0) {
        ts.tv_sec = (time_t)timeout;
        ts.tv_nsec = (long)((timeout - ts.tv_sec) * 1000000000);
        timeout_ptr = &ts;
    }
    
    int ret = gpiod_line_event_wait(line->line, timeout_ptr);
    if (ret < 0) {
        return luaL_error(L, "Failed to wait for event");
    }
    
    lua_pushboolean(L, ret > 0);
    return 1;
}

// line:event_read()
static int line_event_read(lua_State *L) {
    LuaLine *line = (LuaLine *)luaL_checkudata(L, 1, GPIOD_LINE_MT);
    
    if (!line->line) {
        return luaL_error(L, "Line is released");
    }
    
    LuaLineEvent *event = (LuaLineEvent *)lua_newuserdata(L, sizeof(LuaLineEvent));
    
    int ret = gpiod_line_event_read(line->line, &event->event);
    if (ret < 0) {
        return luaL_error(L, "Failed to read event");
    }
    
    luaL_getmetatable(L, GPIOD_LINE_EVENT_MT);
    lua_setmetatable(L, -2);
    
    return 1;
}

// line:event_get_fd()
static int line_event_get_fd(lua_State *L) {
    LuaLine *line = (LuaLine *)luaL_checkudata(L, 1, GPIOD_LINE_MT);
    
    if (!line->line) {
        return luaL_error(L, "Line is released");
    }
    
    int fd = gpiod_line_event_get_fd(line->line);
    if (fd < 0) {
        return luaL_error(L, "Failed to get event file descriptor");
    }
    
    lua_pushinteger(L, fd);
    return 1;
}

// line:active_state()
static int line_active_state(lua_State *L) {
    LuaLine *line = (LuaLine *)luaL_checkudata(L, 1, GPIOD_LINE_MT);
    
    if (!line->line) {
        return luaL_error(L, "Line is released");
    }
    
    int state = gpiod_line_active_state(line->line);
    switch (state) {
        case GPIOD_LINE_ACTIVE_STATE_HIGH:
            lua_pushstring(L, "high");
            break;
        case GPIOD_LINE_ACTIVE_STATE_LOW:
            lua_pushstring(L, "low");
            break;
        default:
            lua_pushstring(L, "unknown");
            break;
    }
    return 1;
}

// line:bias()
static int line_bias(lua_State *L) {
    LuaLine *line = (LuaLine *)luaL_checkudata(L, 1, GPIOD_LINE_MT);
    
    if (!line->line) {
        return luaL_error(L, "Line is released");
    }
    
    int bias = gpiod_line_bias(line->line);
    switch (bias) {
        case GPIOD_LINE_BIAS_AS_IS:
            lua_pushstring(L, "as_is");
            break;
        case GPIOD_LINE_BIAS_DISABLE:
            lua_pushstring(L, "disable");
            break;
        case GPIOD_LINE_BIAS_PULL_UP:
            lua_pushstring(L, "pull_up");
            break;
        case GPIOD_LINE_BIAS_PULL_DOWN:
            lua_pushstring(L, "pull_down");
            break;
        default:
            lua_pushstring(L, "unknown");
            break;
    }
    return 1;
}

// line:is_open_drain()
static int line_is_open_drain(lua_State *L) {
    LuaLine *line = (LuaLine *)luaL_checkudata(L, 1, GPIOD_LINE_MT);
    
    if (!line->line) {
        return luaL_error(L, "Line is released");
    }
    
    lua_pushboolean(L, gpiod_line_is_open_drain(line->line));
    return 1;
}

// line:is_open_source()
static int line_is_open_source(lua_State *L) {
    LuaLine *line = (LuaLine *)luaL_checkudata(L, 1, GPIOD_LINE_MT);
    
    if (!line->line) {
        return luaL_error(L, "Line is released");
    }
    
    lua_pushboolean(L, gpiod_line_is_open_source(line->line));
    return 1;
}

// line:update()
static int line_update(lua_State *L) {
    LuaLine *line = (LuaLine *)luaL_checkudata(L, 1, GPIOD_LINE_MT);
    
    if (!line->line) {
        return luaL_error(L, "Line is released");
    }
    
    int ret = gpiod_line_update(line->line);
    if (ret < 0) {
        return luaL_error(L, "Failed to update line status");
    }
    
    lua_pushboolean(L, 1);
    return 1;
}

// ============================================================================
// Line Bulk related functions
// ============================================================================

// bulk:num_lines()
static int bulk_num_lines(lua_State *L) {
    LuaLineBulk *bulk = (LuaLineBulk *)luaL_checkudata(L, 1, GPIOD_LINE_BULK_MT);
    
    lua_pushinteger(L, gpiod_line_bulk_num_lines(&bulk->bulk));
    return 1;
}

// bulk:get_line(index)
static int bulk_get_line(lua_State *L) {
    LuaLineBulk *bulk = (LuaLineBulk *)luaL_checkudata(L, 1, GPIOD_LINE_BULK_MT);
    unsigned int index = luaL_checkinteger(L, 2);
    
    if (index >= gpiod_line_bulk_num_lines(&bulk->bulk)) {
        return luaL_error(L, "Index out of range");
    }
    
    LuaLine *line = (LuaLine *)lua_newuserdata(L, sizeof(LuaLine));
    line->line = gpiod_line_bulk_get_line(&bulk->bulk, index);
    line->chip = bulk->chip;
    
    luaL_getmetatable(L, GPIOD_LINE_MT);
    lua_setmetatable(L, -2);
    
    return 1;
}

// bulk:request_input(consumer, [flags])
static int bulk_request_input(lua_State *L) {
    LuaLineBulk *bulk = (LuaLineBulk *)luaL_checkudata(L, 1, GPIOD_LINE_BULK_MT);
    const char *consumer = luaL_checkstring(L, 2);
    int flags = luaL_optinteger(L, 3, 0);
    
    int ret = gpiod_line_request_bulk_input_flags(&bulk->bulk, consumer, flags);
    if (ret < 0) {
        return luaL_error(L, "Failed to request bulk input mode");
    }
    
    lua_pushboolean(L, 1);
    return 1;
}

// bulk:request_output(consumer, default_vals, [flags])
static int bulk_request_output(lua_State *L) {
    LuaLineBulk *bulk = (LuaLineBulk *)luaL_checkudata(L, 1, GPIOD_LINE_BULK_MT);
    const char *consumer = luaL_checkstring(L, 2);
    luaL_checktype(L, 3, LUA_TTABLE);
    int flags = luaL_optinteger(L, 4, 0);
    
    unsigned int num_lines = gpiod_line_bulk_num_lines(&bulk->bulk);
    int *values = malloc(num_lines * sizeof(int));
    
    for (unsigned int i = 0; i < num_lines; i++) {
        lua_rawgeti(L, 3, i + 1);
        values[i] = luaL_checkinteger(L, -1);
        lua_pop(L, 1);
    }
    
    int ret = gpiod_line_request_bulk_output_flags(&bulk->bulk, consumer, flags, values);
    free(values);
    
    if (ret < 0) {
        return luaL_error(L, "Failed to request bulk output mode");
    }
    
    lua_pushboolean(L, 1);
    return 1;
}

// bulk:get_values()
static int bulk_get_values(lua_State *L) {
    LuaLineBulk *bulk = (LuaLineBulk *)luaL_checkudata(L, 1, GPIOD_LINE_BULK_MT);
    
    unsigned int num_lines = gpiod_line_bulk_num_lines(&bulk->bulk);
    int *values = malloc(num_lines * sizeof(int));
    
    int ret = gpiod_line_get_value_bulk(&bulk->bulk, values);
    if (ret < 0) {
        free(values);
        return luaL_error(L, "Failed to read bulk GPIO values");
    }
    
    lua_newtable(L);
    for (unsigned int i = 0; i < num_lines; i++) {
        lua_pushinteger(L, values[i]);
        lua_rawseti(L, -2, i + 1);
    }
    
    free(values);
    return 1;
}

// bulk:set_values(values_table)
static int bulk_set_values(lua_State *L) {
    LuaLineBulk *bulk = (LuaLineBulk *)luaL_checkudata(L, 1, GPIOD_LINE_BULK_MT);
    luaL_checktype(L, 2, LUA_TTABLE);
    
    unsigned int num_lines = gpiod_line_bulk_num_lines(&bulk->bulk);
    int *values = malloc(num_lines * sizeof(int));
    
    for (unsigned int i = 0; i < num_lines; i++) {
        lua_rawgeti(L, 2, i + 1);
        values[i] = luaL_checkinteger(L, -1);
        lua_pop(L, 1);
    }
    
    int ret = gpiod_line_set_value_bulk(&bulk->bulk, values);
    free(values);
    
    if (ret < 0) {
        return luaL_error(L, "Failed to set bulk GPIO values");
    }
    
    lua_pushboolean(L, 1);
    return 1;
}

// bulk:release()
static int bulk_release(lua_State *L) {
    LuaLineBulk *bulk = (LuaLineBulk *)luaL_checkudata(L, 1, GPIOD_LINE_BULK_MT);
    
    gpiod_line_release_bulk(&bulk->bulk);
    return 0;
}

// ============================================================================
// Line Event related functions
// ============================================================================

// event:event_type()
static int event_event_type(lua_State *L) {
    LuaLineEvent *event = (LuaLineEvent *)luaL_checkudata(L, 1, GPIOD_LINE_EVENT_MT);
    
    int type = event->event.event_type;
    switch (type) {
        case GPIOD_LINE_EVENT_RISING_EDGE:
            lua_pushstring(L, "rising_edge");
            break;
        case GPIOD_LINE_EVENT_FALLING_EDGE:
            lua_pushstring(L, "falling_edge");
            break;
        default:
            lua_pushstring(L, "unknown");
            break;
    }
    return 1;
}

// event:timestamp()
static int event_timestamp(lua_State *L) {
    LuaLineEvent *event = (LuaLineEvent *)luaL_checkudata(L, 1, GPIOD_LINE_EVENT_MT);
    
    struct timespec ts = event->event.ts;
    lua_pushnumber(L, ts.tv_sec + ts.tv_nsec / 1000000000.0);
    return 1;
}

// ============================================================================
// Chip Iterator related functions
// ============================================================================

// gpiod.chip_iter()
static int gpiod_chip_iter(lua_State *L) {
    LuaChipIter *iter = (LuaChipIter *)lua_newuserdata(L, sizeof(LuaChipIter));
    iter->iter = gpiod_chip_iter_new();
    
    if (!iter->iter) {
        lua_pushnil(L);
        return 1;
    }
    
    luaL_getmetatable(L, GPIOD_CHIP_ITER_MT);
    lua_setmetatable(L, -2);
    
    return 1;
}

// iter:next()
static int chip_iter_next(lua_State *L) {
    LuaChipIter *iter = (LuaChipIter *)luaL_checkudata(L, 1, GPIOD_CHIP_ITER_MT);
    
    if (!iter->iter) {
        lua_pushnil(L);
        return 1;
    }
    
    struct gpiod_chip *chip = gpiod_chip_iter_next_noclose(iter->iter);
    if (!chip) {
        lua_pushnil(L);
        return 1;
    }
    
    LuaChip *lua_chip = (LuaChip *)lua_newuserdata(L, sizeof(LuaChip));
    lua_chip->chip = chip;
    
    luaL_getmetatable(L, GPIOD_CHIP_MT);
    lua_setmetatable(L, -2);
    
    return 1;
}

// iter:next_noclose()
static int chip_iter_next_noclose(lua_State *L) {
    LuaChipIter *iter = (LuaChipIter *)luaL_checkudata(L, 1, GPIOD_CHIP_ITER_MT);
    
    if (!iter->iter) {
        lua_pushnil(L);
        return 1;
    }
    
    struct gpiod_chip *chip = gpiod_chip_iter_next_noclose(iter->iter);
    if (!chip) {
        lua_pushnil(L);
        return 1;
    }
    
    LuaChip *lua_chip = (LuaChip *)lua_newuserdata(L, sizeof(LuaChip));
    lua_chip->chip = chip;
    
    luaL_getmetatable(L, GPIOD_CHIP_MT);
    lua_setmetatable(L, -2);
    
    return 1;
}

// iter:close()
static int chip_iter_close(lua_State *L) {
    LuaChipIter *iter = (LuaChipIter *)luaL_checkudata(L, 1, GPIOD_CHIP_ITER_MT);
    
    if (iter->iter) {
        gpiod_chip_iter_free_noclose(iter->iter);
        iter->iter = NULL;
    }
    
    return 0;
}

// ============================================================================
// Utility functions
// ============================================================================

// gpiod.sleep(seconds)
static int gpiod_sleep(lua_State *L) {
    double seconds = luaL_checknumber(L, 1);
    sleep_precise(seconds);
    return 0;
}

// gpiod.version()
static int gpiod_version(lua_State *L) {
    lua_pushstring(L, gpiod_version_string());
    return 1;
}

// ============================================================================
// Metatable and module initialization
// ============================================================================

// Chip method table
static const luaL_Reg chip_methods[] = {
    {"get_line", chip_get_line},
    {"get_lines", chip_get_lines},
    {"find_line", chip_find_line},
    {"get_all_lines", chip_get_all_lines},
    {"name", chip_name},
    {"label", chip_label},
    {"num_lines", chip_num_lines},
    {"close", chip_close},
    {"__gc", chip_close},
    {NULL, NULL}
};

// Line method table
static const luaL_Reg line_methods[] = {
    {"request_input", line_request_input},
    {"request_output", line_request_output},
    {"request_rising_edge_events", line_request_rising_edge_events},
    {"request_falling_edge_events", line_request_falling_edge_events},
    {"request_both_edges_events", line_request_both_edges_events},
    {"request_rising_edge_events_flags", line_request_rising_edge_events_flags},
    {"request_falling_edge_events_flags", line_request_falling_edge_events_flags},
    {"request_both_edges_events_flags", line_request_both_edges_events_flags},
    {"event_wait", line_event_wait},
    {"event_read", line_event_read},
    {"event_get_fd", line_event_get_fd},
    {"get_value", line_get_value},
    {"set_value", line_set_value},
    {"offset", line_offset},
    {"name", line_name},
    {"consumer", line_consumer},
    {"direction", line_direction},
    {"active_state", line_active_state},
    {"bias", line_bias},
    {"is_used", line_is_used},
    {"is_open_drain", line_is_open_drain},
    {"is_open_source", line_is_open_source},
    {"update", line_update},
    {"release", line_release},
    {"__gc", line_release},
    {NULL, NULL}
};

// Line Bulk method table
static const luaL_Reg line_bulk_methods[] = {
    {"num_lines", bulk_num_lines},
    {"get_line", bulk_get_line},
    {"request_input", bulk_request_input},
    {"request_output", bulk_request_output},
    {"get_values", bulk_get_values},
    {"set_values", bulk_set_values},
    {"release", bulk_release},
    {"__gc", bulk_release},
    {NULL, NULL}
};

// Line Event method table
static const luaL_Reg line_event_methods[] = {
    {"event_type", event_event_type},
    {"timestamp", event_timestamp},
    {NULL, NULL}
};

// Chip Iterator method table
static const luaL_Reg chip_iter_methods[] = {
    {"next", chip_iter_next},
    {"next_noclose", chip_iter_next_noclose},
    {"close", chip_iter_close},
    {"__gc", chip_iter_close},
    {NULL, NULL}
};

// Module function table
static const luaL_Reg gpiod_functions[] = {
    {"chip_open", chip_open},
    {"chip_iter", gpiod_chip_iter},
    {"sleep", gpiod_sleep},
    {"version", gpiod_version},
    {NULL, NULL}
};

// Module initialization function
int luaopen_gpiod(lua_State *L) {
    // Create Chip metatable
    luaL_newmetatable(L, GPIOD_CHIP_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, chip_methods, 0);
    
    // Create Line metatable
    luaL_newmetatable(L, GPIOD_LINE_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, line_methods, 0);
    
    // Create Line Bulk metatable
    luaL_newmetatable(L, GPIOD_LINE_BULK_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, line_bulk_methods, 0);
    
    // Create Line Event metatable
    luaL_newmetatable(L, GPIOD_LINE_EVENT_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, line_event_methods, 0);
    
    // Create Chip Iterator metatable
    luaL_newmetatable(L, GPIOD_CHIP_ITER_MT);
    lua_pushvalue(L, -1);
    lua_setfield(L, -2, "__index");
    luaL_setfuncs(L, chip_iter_methods, 0);
    
    // Create module table
    luaL_newlib(L, gpiod_functions);
    
    // Add request flag constants
    lua_pushinteger(L, GPIOD_LINE_REQUEST_FLAG_OPEN_DRAIN);
    lua_setfield(L, -2, "OPEN_DRAIN");
    
    lua_pushinteger(L, GPIOD_LINE_REQUEST_FLAG_OPEN_SOURCE);
    lua_setfield(L, -2, "OPEN_SOURCE");
    
    lua_pushinteger(L, GPIOD_LINE_REQUEST_FLAG_ACTIVE_LOW);
    lua_setfield(L, -2, "ACTIVE_LOW");
    
    lua_pushinteger(L, GPIOD_LINE_REQUEST_FLAG_BIAS_DISABLE);
    lua_setfield(L, -2, "BIAS_DISABLE");
    
    lua_pushinteger(L, GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_DOWN);
    lua_setfield(L, -2, "BIAS_PULL_DOWN");
    
    lua_pushinteger(L, GPIOD_LINE_REQUEST_FLAG_BIAS_PULL_UP);
    lua_setfield(L, -2, "BIAS_PULL_UP");
    
    // Add direction constants
    lua_pushinteger(L, GPIOD_LINE_DIRECTION_INPUT);
    lua_setfield(L, -2, "DIRECTION_INPUT");
    
    lua_pushinteger(L, GPIOD_LINE_DIRECTION_OUTPUT);
    lua_setfield(L, -2, "DIRECTION_OUTPUT");
    
    // Add active state constants
    lua_pushinteger(L, GPIOD_LINE_ACTIVE_STATE_HIGH);
    lua_setfield(L, -2, "ACTIVE_STATE_HIGH");
    
    lua_pushinteger(L, GPIOD_LINE_ACTIVE_STATE_LOW);
    lua_setfield(L, -2, "ACTIVE_STATE_LOW");
    
    // Add bias constants
    lua_pushinteger(L, GPIOD_LINE_BIAS_AS_IS);
    lua_setfield(L, -2, "BIAS_AS_IS");
    
    lua_pushinteger(L, GPIOD_LINE_BIAS_DISABLE);
    lua_setfield(L, -2, "BIAS_DISABLE_CONST");
    
    lua_pushinteger(L, GPIOD_LINE_BIAS_PULL_UP);
    lua_setfield(L, -2, "BIAS_PULL_UP_CONST");
    
    lua_pushinteger(L, GPIOD_LINE_BIAS_PULL_DOWN);
    lua_setfield(L, -2, "BIAS_PULL_DOWN_CONST");
    
    // Add event type constants
    lua_pushinteger(L, GPIOD_LINE_EVENT_RISING_EDGE);
    lua_setfield(L, -2, "EVENT_RISING_EDGE");
    
    lua_pushinteger(L, GPIOD_LINE_EVENT_FALLING_EDGE);
    lua_setfield(L, -2, "EVENT_FALLING_EDGE");
    
    return 1;
}