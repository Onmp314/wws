-- some test code for W-Lua Wlib wrapper
--
-- run this with:
--	wlua -e debug=1 test-wlib.lua
-- to enable Wlib debug


-- whether to switch "debug" on (cmdline args are strings)
if (type(debug) == "string") then
	-- switch on wlib debugging
	wlib.trace(1)
end

-- connect server, open window, wait for input, exit
function test()
	-- contact W server
	wlib.init()

	-- create and open a window
	wd = 200
	ht = 200
	win = wlib.create(wd, ht, wlib.SIMPLEWIN)
	wlib.open(win, wlib.UNDEF, wlib.UNDEF)

	-- draw some graphics
	wlib.line(win,  0, 0, wd, ht)
	wlib.line(win, wd, 0,  0, ht)

	-- wait for an event
	ev = wlib.queryevent(NULL, NULL, NULL, -1)

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
