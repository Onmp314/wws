-- some test code for W-Lua W toolkit wrapper
--
-- run this with:
--	wlua -e debug=1 test-wt.lua
-- to enable (*lots* of) Wlib debug
--
-- TODO: error handling

-- whether to switch "debug" on (cmdline args are strings)
if (type(debug) == "string") then
	-- switch on wlib debugging
	wlib.trace(1)
end


-- function handling fileselector input for test_filesel()
function done_cb(w, name)
	if(name) then
		print("file:", name)
	else
		print("canceled");
	end
	wt.quit(1);
end


-- doesn't work until Wt ACTION_CB callback handling has been implemented
function test_filesel(top)
	fsel = wt.create(wt.filesel_class, top)
	if(not fsel) then return; end

	wt.setopt(fsel, wt.LABEL, "Select file...")
	wt.setopt(fsel, wt.FILESEL_PATH, "/tmp/")
	wt.setopt(fsel, wt.FILESEL_MASK, "*.txt")
	-- TODO:
	-- wt.setopt(fsel, wt.ACTION_CB, done_cb)
end


-- tests some modal dialogs in Wt
function test_dialogs(top)
	message = "Write your message here"
	-- parent, buffer, buflen, title, caption, button name, nil
	wt.entrybox (top, message, #message, "Message:", "text:", "OK", nil)

	-- parent, message, dialog type, title, button text, nil
	wt.dialog (top, message, wt.DIAL_INFO, "Info:", "OK", nil)

	-- parent, title, directory, file mask, default filename
	path = wt.fileselect (top, "Select file...", "/tmp/", "*.txt", "")
	print("got filename:", path)
end

-- run either of above test functions
function test()
	top = wt.init()
	if(not top) then return 0; end

	test_dialogs(top)

	if (wt_realize(top) < 0) then return 0; end
	wt.run ()
end


-- run test
test_dialogs()
