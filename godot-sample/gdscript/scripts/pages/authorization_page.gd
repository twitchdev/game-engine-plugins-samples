class_name AuthorizationPage
extends SamplePage
## Enter a Client ID, choose scopes, and log in with Twitch's device code flow.

# Every scope the other pages need. Drops and raid events need no scope.
const SCOPES := [
	["channel:manage:polls", "Polls page"],
	["channel:manage:predictions", "Predictions page"],
	["channel:manage:broadcast", "Edit channel info, create stream markers"],
	["clips:edit", "Create clips"],
	["channel:manage:redemptions", "Channel point rewards and redemption events"],
	["user:read:subscriptions", "Check whether you subscribe to a channel"],
	["channel:read:subscriptions", "Subscription events"],
	["moderator:read:followers", "Follow events"],
	["bits:read", "Cheer events and the Bits leaderboard"],
	["channel:read:hype_train", "Hype Train events"],
	["user:read:email", "Email address in your user info"],
]

var _client_id_edit: LineEdit
var _client_id_changed: Label
var _scope_checks: Array[CheckBox] = []
var _status_value: Label
var _logged_out_box: VBoxContainer
var _code_box: Control
var _code_label: Label
var _activation_label: Label
var _logged_in_box: VBoxContainer
var _granted_value: Label
var _user_box: VBoxContainer
var _user_code := ""
var _activation_uri := ""


func get_title() -> String:
	return "Authorization"


func build() -> void:
	_build_client_id()
	_build_scopes()
	_build_login()
	app.auth_state_changed.connect(_update_login)
	_update_login()


func _process(delta: float) -> void:
	super(delta)
	var typed := _client_id_edit.text.strip_edges()
	_client_id_changed.visible = not app.applied_client_id.is_empty() and not typed.is_empty() and typed != app.applied_client_id
	_status_value.text = app.status_text()


# ------------------------------------------------------------------ 1. Client ID

func _build_client_id() -> void:
	SampleUI.section(self, "1. Client ID",
			"Register an application in the Twitch developer console. Set the OAuth Redirect URL to "
			+ "http://localhost and the Client Type to Public (the device code login used here needs a "
			+ "public client). Then paste its Client ID below.")

	_client_id_edit = SampleUI.text_row(self, "Client ID", app.get_saved_client_id(), "", "Paste your Client ID here")
	var buttons := SampleUI.indented(self)
	app.watch_button(SampleUI.button(buttons, "Use this Client ID", func(): app.apply_client_id(_client_id_edit.text)),
			func(): return not _client_id_edit.text.strip_edges().is_empty())
	SampleUI.button(buttons, "Open the Twitch developer console", func(): OS.shell_open("https://dev.twitch.tv/console/apps"))

	_client_id_changed = SampleUI.level_label("The Client ID changed. Press Use this Client ID to switch to it.", SampleUI.Level.WARNING)
	SampleUI.indented(self).add_child(_client_id_changed)

	var proxy_text := ("On: events come from a local EventSubProxy (development only). Change it in Project Settings > Twitch > Debug, then restart."
			if Twitch.get_use_eventsub_proxy() else
			"Off: events come from Twitch. To test with a local EventSubProxy, enable it in Project Settings > Twitch > Debug, then restart.")
	SampleUI.row(self, "EventSubProxy").add_child(SampleUI.muted(proxy_text))


# ------------------------------------------------------------------ 2. Scopes

func _build_scopes() -> void:
	SampleUI.section(self, "2. Scopes",
			"Permissions requested when you log in. To change them after logging in, log out and log in again.")
	for scope in SCOPES:
		var r := HBoxContainer.new()
		var check := CheckBox.new()
		check.text = scope[0]
		check.button_pressed = true
		check.custom_minimum_size = Vector2(SampleUI.LABEL_WIDTH + 80, 0)
		r.add_child(check)
		r.add_child(SampleUI.muted(scope[1]))
		add_child(r)
		_scope_checks.append(check)
	var buttons := HBoxContainer.new()
	add_child(buttons)
	SampleUI.button(buttons, "Select all", func():
		for c in _scope_checks:
			c.button_pressed = true)
	SampleUI.button(buttons, "Select none", func():
		for c in _scope_checks:
			c.button_pressed = false)


# ------------------------------------------------------------------ 3. Log in

func _build_login() -> void:
	SampleUI.section(self, "3. Log in")
	_status_value = SampleUI.key_value(self, "Status", "")

	# Logged out: the Log in button and, once requested, the device code.
	_logged_out_box = VBoxContainer.new()
	add_child(_logged_out_box)
	app.action_button(SampleUI.indented(_logged_out_box), "Log in with Twitch", _log_in, Callable(), false)

	var code := SampleUI.panel(_logged_out_box)
	_code_box = code.get_parent() as Control
	code.add_child(SampleUI.label("Open the activation page and enter this code:"))
	_code_label = SampleUI.label("", Color.WHITE, 30, true)
	code.add_child(_code_label)
	_activation_label = SampleUI.label("", SampleUI.TEXT, 0, false, true)
	code.add_child(_activation_label)
	var code_buttons := HBoxContainer.new()
	code.add_child(code_buttons)
	SampleUI.button(code_buttons, "Open activation page", func(): OS.shell_open(_activation_uri))
	SampleUI.button(code_buttons, "Copy code", func():
		DisplayServer.clipboard_set(_user_code)
		app.info("Code copied to the clipboard."))
	code.add_child(SampleUI.muted("This page updates by itself once you approve the login on Twitch."))

	# Logged in: granted scopes, user info, log out.
	_logged_in_box = VBoxContainer.new()
	add_child(_logged_in_box)
	_granted_value = SampleUI.key_value(_logged_in_box, "Granted scopes", "")
	var buttons := SampleUI.indented(_logged_in_box)
	app.action_button(buttons, "Get my user info", _get_my_user_info)
	app.action_button(buttons, "Log out", _log_out)
	_user_box = VBoxContainer.new()
	_logged_in_box.add_child(_user_box)


func _update_login() -> void:
	var logged_in := app.is_logged_in()
	_logged_out_box.visible = not logged_in
	_logged_in_box.visible = logged_in
	if logged_in:
		_user_code = ""
	_code_box.visible = not _user_code.is_empty()
	_code_label.text = _user_code
	_activation_label.text = "Activation page: " + _activation_uri
	_granted_value.text = "(none)" if app.granted_scopes.is_empty() else ", ".join(app.granted_scopes)


func _log_in() -> void:
	var requested := []
	for check in _scope_checks:
		if check.button_pressed:
			requested.append(check.text)

	app.run("Log in with Twitch", func():
		var task := Twitch.get_authentication_info(requested)
		var info: TwitchAuthenticationInfo = await task.completed
		if app.failed(task, "Log in with Twitch"):
			return
		if info == null:
			app.info("Already logged in.")
			return
		_user_code = info.user_code
		_activation_uri = info.uri
		_update_login()
		app.info("Enter code %s at %s to finish logging in." % [info.user_code, info.uri])
		app.refresh_auth_state()
		OS.shell_open(info.uri))


func _get_my_user_info() -> void:
	app.run("Get my user info", func():
		var task := Twitch.get_my_user_info()
		var me: TwitchUserInfo = await task.completed
		if app.failed(task, "Get my user info"):
			return
		SampleUI.clear(_user_box)
		var box := SampleUI.panel(_user_box)
		SampleUI.key_value(box, "Display name", me.display_name)
		SampleUI.key_value(box, "Login", me.login_name)
		SampleUI.key_value(box, "User ID", me.channel_id)
		SampleUI.key_value(box, "Broadcaster type", me.broadcaster_type if not me.broadcaster_type.is_empty() else "(none)")
		SampleUI.key_value(box, "Email", me.email)
		app.success("You are logged in as %s." % me.display_name))


func _log_out() -> void:
	app.run("Log out", func():
		var task := Twitch.log_out()
		await task.completed
		if app.failed(task, "Log out"):
			return
		SampleUI.clear(_user_box)
		app.success("Logged out. The saved token was removed.")
		app.refresh_auth_state())
