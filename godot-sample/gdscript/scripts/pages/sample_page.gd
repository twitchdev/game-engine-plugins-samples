class_name SamplePage
extends VBoxContainer
## One tab of the sample. Each page builds its controls once in build() and rebuilds its result
## panels when data arrives.

var app: SampleApp
var _hint_panel: PanelContainer
var _hint_label: Label


func get_title() -> String:
	return ""


func setup(sample_app: SampleApp) -> void:
	app = sample_app
	add_theme_constant_override("separation", 6)
	build()


## Builds the page's controls.
func build() -> void:
	pass


## Shows a hint at the top of the page while the user still needs to set a Client ID or log in.
func add_connection_hint() -> void:
	_hint_panel = PanelContainer.new()
	_hint_label = SampleUI.level_label("", SampleUI.Level.WARNING)
	_hint_panel.add_child(_hint_label)
	add_child(_hint_panel)


func _process(_delta: float) -> void:
	if _hint_panel == null or app == null:
		return
	var hint := ""
	if not Twitch.is_initialized():
		hint = "No Client ID yet. Enter it on the Authorization page and press Use this Client ID."
	elif not app.is_logged_in():
		hint = "Not logged in. Log in on the Authorization page before using this page."
	_hint_label.text = hint
	_hint_panel.visible = not hint.is_empty()
