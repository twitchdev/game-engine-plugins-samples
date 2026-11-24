class_name StringListEditor
extends VBoxContainer
## Edits a list of strings with numbered fields plus Add and Remove buttons that keep the count
## between min_items and max_items (poll choices, prediction outcomes).

var item_label: String
var min_items: int
var max_items: int
var _rows := VBoxContainer.new()
var _add_button: Button


func _init(p_item_label: String, initial: PackedStringArray, p_min: int, p_max: int, hint := "") -> void:
	item_label = p_item_label
	min_items = p_min
	max_items = p_max
	add_child(_rows)
	var footer := SampleUI.indented(self)
	_add_button = SampleUI.button(footer, "+ Add " + item_label.to_lower(), func(): _add_row(""))
	if not hint.is_empty():
		footer.add_child(SampleUI.muted(hint))
	for item in initial:
		_add_row(item)
	_update()


## The trimmed, non-empty items.
func get_items() -> PackedStringArray:
	var items := PackedStringArray()
	for r in _rows.get_children():
		var text: String = r.get_meta("edit").text.strip_edges()
		if not text.is_empty():
			items.append(text)
	return items


func _add_row(text: String) -> void:
	if _rows.get_child_count() >= max_items:
		return
	var r := SampleUI.row(_rows, "")
	var edit := LineEdit.new()
	edit.text = text
	edit.custom_minimum_size = Vector2(SampleUI.FIELD_MIN_WIDTH, 0)
	edit.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	r.add_child(edit)
	var remove := SampleUI.button(r, "Remove", func():
		if _rows.get_child_count() > min_items:
			_rows.remove_child(r)
			r.queue_free()
			_update())
	remove.custom_minimum_size = Vector2(80, 30)
	r.set_meta("edit", edit)
	r.set_meta("remove", remove)
	_update()


func _update() -> void:
	var count := _rows.get_child_count()
	for i in count:
		var r := _rows.get_child(i)
		(r.get_child(0) as Label).text = "%s %d" % [item_label, i + 1]
		(r.get_meta("remove") as Button).disabled = count <= min_items
	_add_button.disabled = count >= max_items
