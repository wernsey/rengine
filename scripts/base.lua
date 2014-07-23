--[[
*# The functions in this section are basic utility functions written in Lua
*# and available to all Lua scripts.
]]

--[[ 
Poofs! some functions I don't want accessible to user created
scripts;
See http://stackoverflow.com/a/1054957/115589 and http://lua-users.org/wiki/SandBoxes
]]
require = nil;
collectgarbage = nil;
load = nil;
loadfile = nil;
--loadstring = nil; -- Actually needed for deserializing stuff.
dofile = nil;
rawequal = nil;
rawget = nil;
rawset = nil;
setfenv = nil;
module = nil;

--[[
*@ function class(baseClass, body)
*# For Object Oriented Programming, from Chapter 12 of Game Coding Complete (4th Ed).
*# Constructs a class object.\n
*# for example, a 2 dimensional Vector class can be constructed like so
*X Vector = class(nil, {x = 0, y = 0})
*# Then a instance of Vector can be created like so:
*X local v = Vector:Create{x = 3, y = 4}
*# You can add methods to the example vector class like so:
*X function Vector:len() return math.sqrt(self.x * self.x + self.y * self.y) end
*# and call it like so:
*X log("Vector length: " .. v:len())
]]
function class(baseClass, body)
	local ret = body or {};
	
	if (baseClass ~= nil) then
		setmetatable(ret, ret);
		ret.__index = baseClass;
	end
	
	ret.Create = function(self, constructionData, originalSubClass)
		if (self.__index ~= nil) then
			if (originalSubClass ~= nil) then
				obj = self.__index:Create(constructionData, originalSubClass);
			else
				obj = self.__index:Create(constructionData, self);
			end
		else
			obj = constructionData or {};
		end
		
		setmetatable(obj, obj);
		obj.__index = self;
		
		if(self.__operators ~= nil) then
			for key, val in pairs(self.__operators) do
				obj[key] = val;
			end
		end
		
		return obj;
	end
	
	return ret;
end;

--[[
*@ function serialize(o)
*# Serializes an object {{o}} to a string.
*# Based on the one in section 12.1.1 of the PIL book http://www.lua.org/pil/12.1.1.html
*# It does not serialize tables with cycles.
*X local str = serialize({x = 5, y = 6})
]]
function serialize(o)
	if type(o) == "number" then
		return tostring(o)
	elseif type(o) == "string" then
		return string.format("%q", o)
	elseif type(o) == "table" then
		local s = "{"
		for k,v in pairs(o) do
			s = s .. "[" .. serialize(k) .. "]=" .. serialize(v) .. ","
		end
		return s .. "}";
	else	
		return "";
	end
end;

--[[
*@ function deserialize(s)
*# Deserializes an object previously serialized with {{serialize(o)}}.
*# http://www.lua.org/pil/8.html
*X local v = deserialize(str)
]]
function deserialize(s)
	local f = assert(loadstring("do return " .. s .. " end"))
	return f()
end

--[[
*@ Class Stack
*# A basic stack class for pushing and popping Lua tables
*X stack = Stack:Create{};
]]
Stack = class(nil, {elements = nil, top = 0});

--[[
*@ function Stack:push(o)
*# Pushes an element onto the stack.
*X stack:push(o);
]]
function Stack:push(o)
    if not self.elements then
        self.elements = {}
    end
    self.top = self.top + 1
    self.elements[self.top] = o;
end

--[[
*@ function Stack:pop()
*# Pops an element of the stack.
*X o = stack:pop();
]]
function Stack:pop()
    if self.elements and self.top > 0 then
        local o = self.elements[self.top]
        self.elements[self.top] = nil
        self.top = self.top - 1
        return o
    end
    error('Stack underflow', 2)
end

--[[
*@ function Stack:size()
*# Retrieves the size (number of elements) on the stack
]]
function Stack:size()
    if not self.elements then
        return 0
    end
    return self.top;
end

--[[
*@ function Stack:clear()
*# Erases all the elements in the stack.
]]
function Stack:clear()   
	self.elements = {} 
    self.top = 0;
end
