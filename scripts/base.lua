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
For Object Oriented Programming
From Chapter 12 of Game Coding Complete (4th Ed)
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
Serializes an object {{o}} to a string.
Based on the one in section 12.1.1 of the PIL book http://www.lua.org/pil/12.1.1.html
It does not serialize tables with cycles.
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
Deserializes an object pr
http://www.lua.org/pil/8.html
]]
function deserialize(s)
	local f = assert(loadstring("do return " .. s .. " end"))
	return f()
end
