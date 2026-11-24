class_name SampleUI
extends RefCounted
## Small helpers so every page lays out its fields and buttons the same way
## (SampleGUI in the Unity sample, SampleWidgets in the Unreal sample).

const LABEL_WIDTH := 200
const FIELD_MIN_WIDTH := 260

const BACKGROUND := Color(0.12, 0.11, 0.15)
const PANEL := Color(0.17, 0.16, 0.21)
const PANEL_BORDER := Color(0.27, 0.25, 0.33)
const ACCENT := Color(0.75, 0.6, 1.0)
const BAR := Color(0.6, 0.4, 1.0)
const TEXT := Color(0.88, 0.88, 0.9)
const MUTED := Color(0.68, 0.68, 0.74)

enum Level { DEBUG, INFO, SUCCESS, WARNING, ERROR }

const LEVEL_COLORS := {
	Level.DEBUG: Color(0.6, 0.6, 0.6),
	Level.INFO: Color(0.88, 0.88, 0.9),
	Level.SUCCESS: Color(0.5, 0.9, 0.55),
	Level.WARNING: Color(1.0, 0.82, 0.35),
	Level.ERROR: Color(1.0, 0.45, 0.45),
}

static var _bold: FontVariation


# ------------------------------------------------------------------ theme

static func make_theme() -> Theme:
	var theme := Theme.new()
	theme.default_font_size = 15

	var panel := StyleBoxFlat.new()
	panel.bg_color = PANEL
	panel.border_color = PANEL_BORDER
	panel.set_border_width_all(1)
	panel.set_corner_radius_all(4)
	panel.set_content_margin_all(10)
	theme.set_stylebox("panel", "PanelContainer", panel)

	# The tab bar: the selected tab in the accent color, like the Unity and Unreal samples.
	var tab_selected := StyleBoxFlat.new()
	tab_selected.bg_color = ACCENT.darkened(0.35)
	tab_selected.set_corner_radius_all(4)
	tab_selected.set_content_margin_all(8)
	var tab_unselected := tab_selected.duplicate() as StyleBoxFlat
	tab_unselected.bg_color = Color(0.3, 0.3, 0.34)
	var tab_hovered := tab_selected.duplicate() as StyleBoxFlat
	tab_hovered.bg_color = Color(0.38, 0.36, 0.45)
	theme.set_stylebox("tab_selected", "TabContainer", tab_selected)
	theme.set_stylebox("tab_unselected", "TabContainer", tab_unselected)
	theme.set_stylebox("tab_hovered", "TabContainer", tab_hovered)
	theme.set_constant("side_margin", "TabContainer", 0)
	theme.set_font("font", "TabContainer", bold_font())
	var page_panel := StyleBoxFlat.new()
	page_panel.bg_color = BACKGROUND
	page_panel.content_margin_top = 8
	theme.set_stylebox("panel", "TabContainer", page_panel)

	var bar_fill := StyleBoxFlat.new()
	bar_fill.bg_color = BAR
	theme.set_stylebox("fill", "ProgressBar", bar_fill)
	var bar_background := StyleBoxFlat.new()
	bar_background.bg_color = Color(0.25, 0.24, 0.3)
	theme.set_stylebox("background", "ProgressBar", bar_background)
	return theme


static func bold_font() -> Font:
	if _bold == null:
		_bold = FontVariation.new()
		_bold.base_font = ThemeDB.fallback_font
		_bold.variation_embolden = 0.9
	return _bold


# ------------------------------------------------------------------ text

static func label(text: String, color := TEXT, font_size := 0, bold := false, wrap := false) -> Label:
	var l := Label.new()
	l.text = text
	l.add_theme_color_override("font_color", color)
	if font_size > 0:
		l.add_theme_font_size_override("font_size", font_size)
	if bold:
		l.add_theme_font_override("font", bold_font())
	if wrap:
		l.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
		l.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	return l


static func muted(text: String) -> Label:
	return label(text, MUTED, 0, false, true)


static func level_label(text: String, level: Level) -> Label:
	return label(text, LEVEL_COLORS[level], 0, false, true)


# ------------------------------------------------------------------ layout

static func spacer(parent: Control, height: int) -> void:
	var s := Control.new()
	s.custom_minimum_size = Vector2(0, height)
	parent.add_child(s)


## A section heading with an optional explanation underneath.
static func section(parent: Control, heading: String, description := "") -> void:
	spacer(parent, 10)
	parent.add_child(label(heading, Color.WHITE, 17, true))
	if not description.is_empty():
		parent.add_child(muted(description))
	spacer(parent, 2)


## A collapsible section, expanded by default. Returns the container for its contents.
static func foldout(parent: Control, heading: String, description := "") -> VBoxContainer:
	spacer(parent, 10)
	var header := Button.new()
	header.flat = true
	header.alignment = HORIZONTAL_ALIGNMENT_LEFT
	header.text = "[-]  " + heading
	header.add_theme_font_override("font", bold_font())
	header.add_theme_font_size_override("font_size", 17)
	header.add_theme_color_override("font_color", Color.WHITE)
	parent.add_child(header)
	var content := VBoxContainer.new()
	parent.add_child(content)
	if not description.is_empty():
		content.add_child(muted(description))
	header.pressed.connect(func():
		content.visible = not content.visible
		header.text = ("[-]  " if content.visible else "[+]  ") + heading)
	return content


## A row with a fixed-width label on the left. Returns the row to add controls to.
static func row(parent: Control, label_text: String) -> HBoxContainer:
	var r := HBoxContainer.new()
	var l := label(label_text)
	# Fixed width; long labels wrap instead of pushing the field out of line.
	l.custom_minimum_size = Vector2(LABEL_WIDTH, 0)
	l.autowrap_mode = TextServer.AUTOWRAP_WORD_SMART
	r.add_child(l)
	parent.add_child(r)
	return r


## A row indented to line up with the fields. Returns the row to add buttons to.
static func indented(parent: Control) -> HBoxContainer:
	var r := HBoxContainer.new()
	var s := Control.new()
	s.custom_minimum_size = Vector2(LABEL_WIDTH, 0)
	r.add_child(s)
	parent.add_child(r)
	return r


static func hint(parent: Control, text: String) -> Label:
	var h := muted(text)
	indented(parent).add_child(h)
	return h


static func text_row(parent: Control, label_text: String, value := "", hint_text := "", placeholder := "") -> LineEdit:
	var edit := LineEdit.new()
	edit.text = value
	edit.placeholder_text = placeholder
	edit.custom_minimum_size = Vector2(FIELD_MIN_WIDTH, 0)
	edit.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	row(parent, label_text).add_child(edit)
	if not hint_text.is_empty():
		hint(parent, hint_text)
	return edit


static func check(parent: Control, text: String, value := false) -> CheckBox:
	var box := CheckBox.new()
	box.text = text
	box.button_pressed = value
	parent.add_child(box)
	return box


static func selector(parent: Control, label_text: String, options: PackedStringArray, index := 0) -> OptionButton:
	var option := OptionButton.new()
	for o in options:
		option.add_item(o)
	option.select(index)
	row(parent, label_text).add_child(option)
	return option


static func button(parent: Control, text: String, on_pressed: Callable) -> Button:
	var b := Button.new()
	b.text = text
	b.custom_minimum_size = Vector2(0, 30)
	b.pressed.connect(on_pressed)
	parent.add_child(b)
	return b


## A key/value line, like the result rows in the Unity and Unreal samples.
static func key_value(parent: Control, key: String, value: Variant) -> Label:
	var r := HBoxContainer.new()
	var k := label(key, MUTED)
	k.custom_minimum_size = Vector2(LABEL_WIDTH, 0)
	r.add_child(k)
	var text: String = number(value) if value is int else str(value)
	var v := label("-" if text.is_empty() else text, TEXT, 0, false, true)
	r.add_child(v)
	parent.add_child(r)
	return v


## A boxed panel. Returns the container for its contents.
static func panel(parent: Control) -> VBoxContainer:
	var p := PanelContainer.new()
	var box := VBoxContainer.new()
	p.add_child(box)
	parent.add_child(p)
	return box


## A horizontal bar showing `fraction` (0 to 1).
static func bar(parent: Control, fraction: float) -> ProgressBar:
	var b := ProgressBar.new()
	b.min_value = 0.0
	b.max_value = 1.0
	b.value = clampf(fraction, 0.0, 1.0)
	b.show_percentage = false
	b.custom_minimum_size = Vector2(220, 14)
	b.size_flags_vertical = Control.SIZE_SHRINK_CENTER
	parent.add_child(b)
	return b


static func clear(container: Node) -> void:
	for child in container.get_children():
		container.remove_child(child)
		child.queue_free()


# ------------------------------------------------------------------ values

## Splits a comma, semicolon, or newline separated list and drops empty and duplicate items.
static func split_list(text: String) -> PackedStringArray:
	var result := PackedStringArray()
	for part in text.replace(";", ",").replace("\n", ",").replace("\r", ",").split(",", false):
		var item := part.strip_edges()
		if not item.is_empty() and not result.has(item):
			result.append(item)
	return result


## 1234567 -> "1,234,567" (like "N0" in the C# samples).
static func number(n: int) -> String:
	var digits := str(absi(n))
	var out := ""
	while digits.length() > 3:
		out = "," + digits.substr(digits.length() - 3) + out
		digits = digits.substr(0, digits.length() - 3)
	return ("-" if n < 0 else "") + digits + out


## The readable name of a Twitch enum value, e.g. enum_text("PollStatus", Twitch.POLL_STATUS_ACTIVE) -> "Active".
static func enum_text(enum_name: String, value: int) -> String:
	var prefix := enum_name.to_snake_case().to_upper() + "_"
	for constant in ClassDB.class_get_enum_constants("Twitch", enum_name):
		if ClassDB.class_get_integer_constant("Twitch", constant) == value:
			return constant.trim_prefix(prefix).capitalize()
	return str(value)


static func tier_name(tier: String) -> String:
	match tier:
		"1000": return "1"
		"2000": return "2"
		"3000": return "3"
	return "-" if tier.is_empty() else tier
