class_name SampleApp
extends Control
## The sample's root: header, page tabs and the log panel. Pages use it to run API calls and log.
##
## Everything here runs on the main thread: the Twitch plugin completes its tasks from the frame loop,
## so awaiting a task and then updating the UI is always safe.

signal auth_state_changed

const MAX_LOG_ENTRIES := 300
const MAX_VISIBLE_LOG_ENTRIES := 150
const LOG_HEIGHT := 200
const SETTINGS_PATH := "user://twitch_sample.cfg"

## The latest login status (a Twitch.AuthStatus value), or -1 until it has been read.
var auth_status := -1
var granted_scopes := PackedStringArray()
## The Client ID the sample passed to Twitch.set_client_id(), or "" if none yet.
var applied_client_id := ""
var show_core_debug_messages := false

var _running := {}
var _action_buttons: Array[Dictionary] = []
var _watched_buttons: Array[Dictionary] = []
var _log: Array[Dictionary] = []
var _log_view: RichTextLabel
var _status_label: Label
var _pages: Array[SamplePage] = []
var _reading_auth_state := false


func _ready() -> void:
	theme = SampleUI.make_theme()
	_build()

	Twitch.log_message.connect(_on_core_log)
	info("TwitchSDK plugin %s loaded (core library %s)." % [Twitch.get_plugin_version(), Twitch.get_version()])

	# The plugin starts once it has a Client ID. Use the one saved by an earlier session, if any,
	# so its saved login keeps working.
	var saved := get_saved_client_id()
	if not saved.is_empty():
		apply_client_id(saved)

	var timer := Timer.new()
	timer.wait_time = 1.0
	timer.autostart = true
	timer.timeout.connect(refresh_auth_state)
	add_child(timer)
	refresh_auth_state()


func _process(_delta: float) -> void:
	_status_label.text = "Status: " + status_text()

	var started := Twitch.is_initialized()
	var logged_in := is_logged_in()
	_action_buttons = _action_buttons.filter(func(b): return is_instance_valid(b.button))
	for b in _action_buttons:
		var running := is_running(b.action)
		var allowed: bool = b.enabled.call() if b.enabled.is_valid() else true
		b.button.disabled = not (started and (logged_in or not b.requires_login) and allowed and not running)
		b.button.text = b.action + (" ..." if running else "")
	_watched_buttons = _watched_buttons.filter(func(b): return is_instance_valid(b.button))
	for b in _watched_buttons:
		b.button.disabled = not b.enabled.call()


# ------------------------------------------------------------------ layout

func _build() -> void:
	var background := Panel.new()
	var style := StyleBoxFlat.new()
	style.bg_color = SampleUI.BACKGROUND
	background.add_theme_stylebox_override("panel", style)
	background.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(background)

	var margin := MarginContainer.new()
	margin.set_anchors_preset(Control.PRESET_FULL_RECT)
	for side in ["left", "top", "right", "bottom"]:
		margin.add_theme_constant_override("margin_" + side, 12)
	add_child(margin)

	var root := VBoxContainer.new()
	root.add_theme_constant_override("separation", 8)
	margin.add_child(root)

	# Header: title on the left, versions and status on the right.
	var header := HBoxContainer.new()
	var title := SampleUI.label("Twitch SDK Godot Sample (GDScript)", SampleUI.ACCENT, 22, true)
	title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	header.add_child(title)
	var versions := VBoxContainer.new()
	var plugin := SampleUI.label("TwitchSDK plugin %s, core library %s" % [Twitch.get_plugin_version(), Twitch.get_version()], SampleUI.MUTED, 13)
	plugin.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	versions.add_child(plugin)
	_status_label = SampleUI.label("", SampleUI.MUTED, 13)
	_status_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_RIGHT
	versions.add_child(_status_label)
	header.add_child(versions)
	root.add_child(header)

	# Pages, each in its own scroll area.
	var tabs := TabContainer.new()
	tabs.size_flags_vertical = Control.SIZE_EXPAND_FILL
	root.add_child(tabs)
	for page in [AuthorizationPage.new(), PollsPage.new(), PredictionsPage.new(), DropsPage.new(), GeneralPage.new()]:
		var scroll := ScrollContainer.new()
		scroll.name = page.get_title()
		scroll.horizontal_scroll_mode = ScrollContainer.SCROLL_MODE_DISABLED
		var page_margin := MarginContainer.new()
		page_margin.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		page_margin.add_theme_constant_override("margin_right", 16)
		page_margin.add_theme_constant_override("margin_bottom", 16)
		page.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		page_margin.add_child(page)
		scroll.add_child(page_margin)
		tabs.add_child(scroll)
		page.setup(self)
		_pages.append(page)

	# Log panel.
	var log_panel := PanelContainer.new()
	log_panel.custom_minimum_size = Vector2(0, LOG_HEIGHT)
	var log_box := VBoxContainer.new()
	log_panel.add_child(log_box)
	var log_header := HBoxContainer.new()
	var log_title := SampleUI.label("Log (newest first)", Color.WHITE, 15, true)
	log_title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	log_header.add_child(log_title)
	var debug_toggle := CheckBox.new()
	debug_toggle.text = "Show core debug messages"
	debug_toggle.toggled.connect(func(on: bool): show_core_debug_messages = on)
	log_header.add_child(debug_toggle)
	SampleUI.button(log_header, "Clear", clear_log)
	log_box.add_child(log_header)
	_log_view = RichTextLabel.new()
	_log_view.bbcode_enabled = false
	_log_view.selection_enabled = true
	_log_view.size_flags_vertical = Control.SIZE_EXPAND_FILL
	log_box.add_child(_log_view)
	root.add_child(log_panel)


# ------------------------------------------------------------------ buttons and calls

## A button for an API call. It shows "..." while the call with the same `action` name is running,
## and is disabled until the library is running and, unless `requires_login` is false, the user is
## logged in. (The core doesn't reject calls made while logged out; it holds them until a login
## happens, so allowing the click would leave the button waiting with no feedback.)
func action_button(parent: Control, action: String, on_pressed: Callable, enabled := Callable(), requires_login := true) -> Button:
	var b := SampleUI.button(parent, action, on_pressed)
	_action_buttons.append({ button = b, action = action, enabled = enabled, requires_login = requires_login })
	return b


## A plain button that is enabled only while `enabled` returns true.
func watch_button(b: Button, enabled: Callable) -> Button:
	_watched_buttons.append({ button = b, enabled = enabled })
	return b


func is_running(action: String) -> bool:
	return _running.has(action)


## Runs an API call for a button: ignores repeat clicks while it's running. `work` is a coroutine
## that starts the call(s), awaits them and reports the outcome (see failed()).
func run(action: String, work: Callable) -> void:
	if _running.has(action):
		return
	if not Twitch.is_initialized():
		warning("%s: enter your Client ID on the Authorization page first." % action)
		return
	_running[action] = true
	await work.call()
	_running.erase(action)


## Logs "<action> failed: <error>" and returns true if `task` failed.
func failed(task: TwitchTask, action: String) -> bool:
	if not task.has_error():
		return false
	error("%s failed: %s" % [action, task.error])
	return true


# ------------------------------------------------------------------ Client ID and login state

func get_saved_client_id() -> String:
	var config := ConfigFile.new()
	if config.load(SETTINGS_PATH) == OK:
		var saved: String = config.get_value("sample", "client_id", "")
		if not saved.is_empty():
			return saved
	# Otherwise the project setting (Project Settings > Twitch > Auth > Client Id), if set.
	return Twitch.get_client_id()


func apply_client_id(client_id: String) -> void:
	var trimmed := client_id.strip_edges()
	if trimmed.is_empty():
		return
	var changed := not applied_client_id.is_empty() and applied_client_id != trimmed

	# Starts the plugin if this is its first Client ID, or switches Client IDs (development only).
	Twitch.set_client_id(trimmed)
	applied_client_id = trimmed

	# Remember it, so the next session starts with it and its saved login keeps working.
	var config := ConfigFile.new()
	config.load(SETTINGS_PATH)
	config.set_value("sample", "client_id", trimmed)
	config.save(SETTINGS_PATH)

	success("Using your Client ID. Now log in below.")
	if changed and is_logged_in():
		warning("You're still logged in from the previous Client ID. Log out and log in again to use the new one.")
	refresh_auth_state()


func is_logged_in() -> bool:
	return Twitch.is_initialized() and auth_status == Twitch.AUTH_STATUS_LOGGED_IN


func status_text() -> String:
	if not Twitch.is_initialized():
		return "Enter your Client ID"
	if auth_status == -1:
		return "Checking login ..."
	if auth_status == Twitch.AUTH_STATUS_LOGGED_IN:
		return "Logged in"
	if auth_status == Twitch.AUTH_STATUS_WAITING_FOR_CODE:
		return "Waiting for you to enter the code on Twitch"
	if auth_status == Twitch.AUTH_STATUS_LOADING:
		return "Loading ..."
	return "Logged out"


## Reads the login state (once a second, and right after anything that may change it).
func refresh_auth_state() -> void:
	if not Twitch.is_initialized() or _reading_auth_state:
		return
	_reading_auth_state = true
	var task := Twitch.get_auth_state()
	var state: TwitchAuthState = await task.completed
	_reading_auth_state = false
	if task.has_error() or state == null:
		return

	var had_state := auth_status != -1
	var previous := auth_status
	auth_status = state.status
	granted_scopes = state.scopes
	if not had_state or previous != auth_status:
		if auth_status == Twitch.AUTH_STATUS_LOGGED_IN:
			if had_state:
				success("Logged in to Twitch.")
			else:
				info("Already logged in (using the saved token).")
		elif had_state and previous == Twitch.AUTH_STATUS_LOGGED_IN:
			info("Logged out.")
		auth_state_changed.emit()


# ------------------------------------------------------------------ log

func debug(message: String) -> void:
	write_log(SampleUI.Level.DEBUG, message)


func info(message: String) -> void:
	write_log(SampleUI.Level.INFO, message)


func success(message: String) -> void:
	write_log(SampleUI.Level.SUCCESS, message)


func warning(message: String) -> void:
	write_log(SampleUI.Level.WARNING, message)


func error(message: String) -> void:
	write_log(SampleUI.Level.ERROR, message)


## Adds a line to the log panel and mirrors it to Godot's output (unless it came from there).
func write_log(level: SampleUI.Level, message: String, mirror := true) -> void:
	_log.push_front({ time = Time.get_time_string_from_system(), level = level, message = message })
	if _log.size() > MAX_LOG_ENTRIES:
		_log.resize(MAX_LOG_ENTRIES)
	if mirror:
		match level:
			SampleUI.Level.ERROR: push_error("[TwitchSample] " + message)
			SampleUI.Level.WARNING: push_warning("[TwitchSample] " + message)
			_: print("[TwitchSample] " + message)
	_redraw_log()


func clear_log() -> void:
	_log.clear()
	_redraw_log()


func _redraw_log() -> void:
	if _log_view == null:
		return
	_log_view.clear()
	for i in mini(_log.size(), MAX_VISIBLE_LOG_ENTRIES):
		var entry: Dictionary = _log[i]
		_log_view.push_color(SampleUI.LEVEL_COLORS[entry.level])
		_log_view.add_text("%s   %s" % [entry.time, entry.message])
		_log_view.pop()
		_log_view.newline()


## Core library messages (Twitch.log_message). The plugin prints them to the output already.
func _on_core_log(level: int, message: String) -> void:
	if level == Twitch.LOG_LEVEL_DEBUG and not show_core_debug_messages:
		return
	var sample_level := SampleUI.Level.INFO
	if level == Twitch.LOG_LEVEL_DEBUG:
		sample_level = SampleUI.Level.DEBUG
	elif level == Twitch.LOG_LEVEL_WARNING:
		sample_level = SampleUI.Level.WARNING
	elif level == Twitch.LOG_LEVEL_ERROR:
		sample_level = SampleUI.Level.ERROR
	write_log(sample_level, "[core] " + message, false)


## Headless smoke test (see main.gd): shows every page and runs the input validation, without
## needing a login. Script errors show up in the output.
func self_test() -> void:
	var tabs := find_children("*", "TabContainer", true, false)[0] as TabContainer
	for i in tabs.get_tab_count():
		tabs.current_tab = i
		for frame in 5:
			await get_tree().process_frame
		# With a window (not --headless), --screenshots=<folder> saves a picture of every page.
		for arg in OS.get_cmdline_user_args():
			if arg.begins_with("--screenshots="):
				var folder := arg.trim_prefix("--screenshots=")
				DirAccess.make_dir_recursive_absolute(folder)
				get_viewport().get_texture().get_image().save_png(folder.path_join("%d-%s.png" % [i, tabs.get_tab_title(i)]))
	var polls := _pages[1] as PollsPage
	polls._title_edit.text = ""
	polls._start_poll()
	var errors := _log.filter(func(e): return e.level == SampleUI.Level.ERROR and "Question is required" in e.message)
	print("SELF-TEST pages=%d logged_validation_error=%s status=%s" % [_pages.size(), not errors.is_empty(), status_text()])
