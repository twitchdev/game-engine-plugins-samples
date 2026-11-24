extends Control
## Entry point. Checks that the TwitchSDK addon is installed before loading the sample, because
## the sample's scripts use the Twitch singleton and wouldn't even compile without it.


func _ready() -> void:
	if not Engine.has_singleton("Twitch"):
		var message := Label.new()
		message.text = ("The TwitchSDK addon isn't installed.\n\n"
				+ "Run import-plugin.bat in the godot-sample folder (after building the addon with "
				+ "TwitchSDK-Godot\\build.bat), then reopen the project.")
		message.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
		message.vertical_alignment = VERTICAL_ALIGNMENT_CENTER
		message.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		message.set_anchors_preset(Control.PRESET_FULL_RECT)
		add_child(message)
		push_error("TwitchSDK addon not found (addons/twitchsdk). Run import-plugin.bat.")
		return

	var self_test := OS.get_cmdline_user_args().has("--self-test")
	var app_script: GDScript = load("res://scripts/sample_app.gd")
	if app_script == null or not app_script.can_instantiate():
		push_error("The sample's scripts failed to load; see the errors above.")
		if self_test:
			get_tree().quit(1)
		return
	var app: Control = app_script.new()
	app.set_anchors_preset(Control.PRESET_FULL_RECT)
	add_child(app)

	# Headless smoke test: godot --headless --path . -- --self-test
	if self_test:
		await app.self_test()
		get_tree().quit()
