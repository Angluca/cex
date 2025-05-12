#include "mylib.h"
#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>

// Example C function to be called from Lua
static int
l_myfunction(lua_State* L)
{
    double arg1 = luaL_checknumber(L, 1); // Get first argument
    double arg2 = luaL_checknumber(L, 2); // Get second argument

    double result = mylib.add(arg1, arg2); // Do something

    lua_pushnumber(L, result); // Push result to Lua stack
    return 1;                  // Number of return values
}

// NOTE: this name must match with `mylualib.so|dll` file
static const struct luaL_Reg mylualib[] = {
    { "myfunction", l_myfunction }, // Lua name, C function
    { NULL, NULL }                  // Sentinel
};

// This is the function called by require "mylualib"
int
luaopen_mylualib(lua_State* L)
{
    // Create a new table
    luaL_newlib(L, mylualib);

    // You can add additional module-level values here
    lua_pushstring(L, "1.0-cex");
    lua_setfield(L, -2, "version");

    return 1; // Return the table
}
