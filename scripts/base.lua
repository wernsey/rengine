--[[ 
Poofs! some functions I don't want accessible to user created
scripts; http://stackoverflow.com/a/1054957/115589
]]
require = nil;
collectgarbage = nil;
load = nil;
loadfile = nil;
loadstring = nil;
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
end
