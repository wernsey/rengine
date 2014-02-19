--[[ TODO: This should be baked into the engine in a separate script ]]
if Map ~= nil then
(function()
	local CellWrapper = class(nil, {
		cell = nil,
		ROW = 0,
		COL = 0
	});

	function CellWrapper:set(layer, ti, si)
		self.cell:set(layer, ti, si);
		return self;
	end
	
	function CellWrapper:isBarrier()
		return self.cell:isBarrier();
	end
	
	function CellWrapper:setBarrier(b)
		return self.cell:setBarrier(b);
	end

	----------------------------------------------------------------

	local CellCollection = class(nil, {
		cells = {}
	});

	function CellCollection:set(layer, ti, si)
		for n = 1, self:length() do
			self:nth(n):set(layer, ti, si);
		end
		return self;
	end

	function CellCollection:add(cell)
		table.insert(self.cells, cell);
		return self;
	end

	function CellCollection:nth(n)
		return self.cells[n];
	end

	function CellCollection:length()
		return #self.cells;
	end
	
	function CellCollection:each(func)
		for n = 1, #self.cells do
			func(self.cells[n])
		end
		return self;
	end
	
	
	function CellCollection:isBarrier()
		for n = 1, #self.cells do
			if self.cells[n]:isBarrier() then
				return true;
			end
		end
		return false;
	end
	
	function CellCollection:setBarrier(b)
		for n = 1, #self.cells do
			self.cells[n]:setBarrier(b);
		end
	end

	local cellsById = {};
	local cellsByClass = {};
	local allCells = {};

	--[[
	Pre-process the map to get all the cells that have particular classes and IDs.
	Also store the ROW, COL values of the cell.
	]]
	for r = 1, Map.ROWS do
		for c = 1, Map.COLS do
			local cell = Map.cell(r, c)
			local id = cell:getId();
			
			local wrapper = CellWrapper:Create{cell = cell, ROW = r, COL = c};
			
			allCells[r .. "," .. c] = { wrapper };
			
			if id ~= "" then
				if cellsById[id] ~= nil then
					table.insert(cellsById[id], wrapper);
				else
					cellsById[id] = { wrapper };
				end
			end
			local class = cell:getClass();
			if class ~= "" then
				if cellsByClass[class] ~= nil then
					table.insert(cellsByClass[class], wrapper);
				else
					cellsByClass[class] = { wrapper };
				end
			end
		end
	end
	
	log("Cells preprocessed: " .. Map.ROWS .. ", " .. Map.COLS)

	function C(selector)	
		if string.sub(selector, 1, 1) == '.' then
			selector = string.sub(selector, 2)
			if cellsByClass[selector] then
				return CellCollection:Create{cells = cellsByClass[selector]}
			end
		elseif cellsById[selector] then
			return CellCollection:Create{cells = cellsById[selector]}
		elseif allCells[selector] then
			return CellCollection:Create{cells = allCells[selector]}
		end
		
		return CellCollection:Create();
	end
end)();
end;

--[[ TODO: The SpriteSheet, Drawable and Sprite classes should 
be "baked" into the engine in a separate script.
]]

--[[
*@ class SpriteSheet
*# Object that encapsulates a Sprite Sheet bitmap.
*# Load a new sprite sheet with the {{LoadSpriteSheet}} function.
]]
SpriteSheet = class(nil, {
	file = '',
	bitmap = nil,
	rows = 0,
	cols = 0,
	width = 0,
	height = 0,
	tw = 0,
	th = 0,
	mask = '#000000',
	border = 0
});

function LoadSpriteSheet(options)	
	local sheet = SpriteSheet:Create(options);
	
	sheet.bitmap = Bmp(sheet.file);
	sheet.bitmap:setMask(sheet.mask);
	
	sheet.width = sheet.bitmap:width();
	sheet.height = sheet.bitmap:height();
	
	assert(sheet.cols > 0 and sheet.rows > 0);
	
	sheet.tw = sheet.width / (sheet.cols) - sheet.border;
	sheet.th = sheet.height / (sheet.rows) - sheet.border;
	
	log("Loaded SpriteSheet " .. sheet.file .. " (" .. sheet.width .. " x " .. sheet.height .. ")");
	
	return sheet;
end;

--[[
*@ class Drawable
*# Base class for objects that can be drawn on the screen.
]]
Drawable = class(nil, {});

function Drawable:update()
end;

function Drawable:draw()
end;

--[[
*@ class Sprite
*# Specialisation of {{Drawable}} for bitmap sprites based
*# around {{SpriteSheets}} that can move around and be animated.
]]
Sprite = class(Drawable, {
	x = 0, y = 0,
	sheet = nil,
	frame = {row = 0, col = 0},
	bbox = {x=0,y=0,w=1,h=1}
});

function Sprite:draw()
	local tw = self.sheet.tw;
	local th = self.sheet.th;	
	local sx = self.frame.col * (tw + self.sheet.border);
	local sy = self.frame.row * (th + self.sheet.border);	
	G.blit(self.sheet.bitmap, self.x, self.y, sx, sy, tw, th);
end;

if Map ~= nil then
--[[ TODO: This should be combined with the code that sets up the Cell selectors above ]]

(function() 

	local drawables = {{},{},{}}
	
	local updateDrawables = function(i)
		local d = drawables[i];
		for n = 1, #d do
			d[n]:update();
		end
	end;
	
	local drawDrawables = function(i)
		local d = drawables[i];
		for n = 1, #d do
			d[n]:draw();
		end
	end;
	
	Map.collision = function(x,y,w,h)
		local c1 = math.floor(x / Map.TILE_WIDTH) + 1;
		local c2 = math.floor((x + w) / Map.TILE_WIDTH) + 1;
		local r1 = math.floor(y / Map.TILE_HEIGHT) + 1;
		local r2 = math.floor((y + h) / Map.TILE_HEIGHT) + 1;
		for r = r1, r2 do
			for c = c1, c2 do
				--log("Checking for collision: r:" .. r .. ", c:" .. c)
				local cell = Map.cell(r,c);
				if cell:isBarrier() then
					return true;
				end
			end
		end
		return false;
	end;	
	
	--[[
	*@ Map.addDrawable(drawable, i) 
	*# Adds a Drawable instance `drawable` to the map on layer `i`
	]]
	Map.addDrawable = function (drawable, i) 
		table.insert(drawables[i], drawable);
	end;
	
	onUpdate(function()		
		Map.render(Map.BACKGROUND);
		updateDrawables(Map.BACKGROUND);
		drawDrawables(Map.BACKGROUND);
		
		Map.render(Map.CENTER);
		updateDrawables(Map.CENTER);
		drawDrawables(Map.CENTER);
		
		Map.render(Map.FOREGROUND);
		updateDrawables(Map.FOREGROUND);
		drawDrawables(Map.FOREGROUND);
	end);
	
end)();
end
