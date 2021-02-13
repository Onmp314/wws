-- some test code for W-Lua Wlib & Wt wrappers
--
-- run this with:
--	wlua -e debug=1 test-both.lua
-- to enable Wlib debug


-- whether to switch "debug" on (cmdline args are strings)
if (type(debug) == "string") then
	-- switch on wlib debugging
	wlib.trace(1)
end

-- connect server, open window, draw gfx, wait for input, exit
function test()
	-- contact W server
	wlib.init()

	-- create and open a window
	wd = 200
	ht = 200
	win = wlib.create(wd, ht, wlib.SIMPLEWIN)
	wlib.open(win, wlib.UNDEF, wlib.UNDEF)

	-- draw some graphics with Wt with middle at (100,100)
	wt.box3d   (win,  10,  10, 180, 180)
	wt.circle3d(win, 100, 100,  10)
	wt.arrow3d (win,  90,  50,  20, 20, 0)  -- up
	wt.arrow3d (win,  90, 130,  20, 20, 1)  -- down
	wt.arrow3d (win,  50,  90,  20, 20, 2)  -- left
	wt.arrow3d (win, 130,  90,  20, 20, 3)  -- right

	-- load font with given family, size & style
	font = wlib.loadfont("fixed", 13, wlib.F_BOLD)
	if (not font) then
		-- used default font in bold
		font = wlib.loadfont(nil, 0, wlib.F_BOLD)
	end
	-- some text right on middle of window
	wlib.centerPrints(win, font, "W")

	-- wait for an event
	ev = wlib.queryevent(nil, nil, nil, -1)

	-- check what we got
	if (ev.type == wlib.EVENT_KEY) then
		print(format("Key event: '%c'", ev.key))
	elseif (ev.type == wlib.EVENT_MPRESS) then
		print(format("Mouse button (%d) event at: (%d,%d)", ev.key, ev.x, ev.y))
	end

	-- exit
	wlib.exit()
end


test()
