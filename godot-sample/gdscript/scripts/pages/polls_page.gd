class_name PollsPage
extends SamplePage
## Create a poll on your channel from your own title and choices, watch votes come in, and end it.

# Limits enforced by the Twitch Create Poll endpoint.
const MAX_TITLE_LENGTH := 60
const MAX_CHOICE_LENGTH := 25
const MIN_CHOICES := 2
const MAX_CHOICES := 5
const MIN_DURATION := 15
const MAX_DURATION := 1800

var _title_edit: LineEdit
var _choices: StringListEditor
var _duration_edit: LineEdit
var _points_check: CheckBox
var _points_edit: LineEdit
var _current: VBoxContainer
var _poll: TwitchPoll


func get_title() -> String:
	return "Polls"


func build() -> void:
	add_connection_hint()

	SampleUI.section(self, "Create a poll",
			"Runs a poll on your channel. You need to be a Twitch Affiliate or Partner, and Twitch allows "
			+ "one active poll at a time.")
	_title_edit = SampleUI.text_row(self, "Question", "Which path should we take?", "Up to %d characters." % MAX_TITLE_LENGTH)
	_choices = StringListEditor.new("Choice", PackedStringArray(["Left", "Right"]), MIN_CHOICES, MAX_CHOICES,
			"%d to %d choices, up to %d characters each." % [MIN_CHOICES, MAX_CHOICES, MAX_CHOICE_LENGTH])
	add_child(_choices)
	_duration_edit = SampleUI.text_row(self, "Duration (seconds)", "60", "%d to %d." % [MIN_DURATION, MAX_DURATION])
	_points_check = SampleUI.check(self, "Allow extra votes with channel points")
	_points_edit = SampleUI.text_row(self, "Channel points per extra vote", "100")
	_points_edit.editable = false
	_points_check.toggled.connect(func(on: bool): _points_edit.editable = on)
	app.action_button(SampleUI.indented(self), "Start poll", _start_poll)

	SampleUI.section(self, "Current poll", "Vote counts update live while the poll runs.")
	_current = VBoxContainer.new()
	add_child(_current)
	_show_poll(null)


func _start_poll() -> void:
	var title := _title_edit.text.strip_edges()
	var choices := _choices.get_items()
	if title.is_empty():
		app.error("Question is required.")
		return
	if title.length() > MAX_TITLE_LENGTH:
		app.error("Question can be at most %d characters." % MAX_TITLE_LENGTH)
		return
	if choices.size() < MIN_CHOICES:
		app.error("Enter at least %d choices." % MIN_CHOICES)
		return
	for choice in choices:
		if choice.length() > MAX_CHOICE_LENGTH:
			app.error("Choice \"%s\" is longer than %d characters." % [choice, MAX_CHOICE_LENGTH])
			return
	var seconds := _duration_edit.text.strip_edges()
	if not seconds.is_valid_int() or seconds.to_int() < MIN_DURATION or seconds.to_int() > MAX_DURATION:
		app.error("Duration must be a whole number from %d to %d." % [MIN_DURATION, MAX_DURATION])
		return
	var points := 0
	if _points_check.button_pressed:
		var text := _points_edit.text.strip_edges()
		if not text.is_valid_int() or text.to_int() < 1 or text.to_int() > 1000000:
			app.error("Channel points per vote must be a whole number from 1 to 1000000.")
			return
		points = text.to_int()

	var definition := TwitchPollDefinition.new()
	definition.title = title
	definition.choices = choices
	definition.duration = seconds.to_int()
	definition.channel_points_voting_enabled = _points_check.button_pressed
	definition.channel_points_per_vote = points

	app.run("Start poll", func():
		var task := Twitch.new_poll(definition)
		var created: TwitchPoll = await task.completed
		if app.failed(task, "Start poll"):
			return
		# Keep the previous poll until the new one exists, then stop tracking it.
		if _poll:
			_poll.close()
		_poll = created
		created.updated.connect(func(_info):
			if created == _poll:
				_show_poll(created))
		created.ended.connect(func(info: TwitchPollInfo):
			if created == _poll:
				app.info("Poll \"%s\" ended (%s). %s" % [info.title, SampleUI.enum_text("PollStatus", info.status), _winner(info)]))
		_show_poll(created)
		app.success("Poll \"%s\" started for %s seconds." % [created.info.title, seconds]))


func _winner(info: TwitchPollInfo) -> String:
	var top: TwitchPollChoiceInfo = null
	for choice in info.choices:
		if choice.votes > 0 and (top == null or choice.votes > top.votes):
			top = choice
	if top == null:
		return "No votes were cast."
	return "Most votes: \"%s\" with %s." % [top.title, SampleUI.number(top.votes)]


func _show_poll(poll: TwitchPoll) -> void:
	SampleUI.clear(_current)
	if poll == null:
		_current.add_child(SampleUI.muted("No poll yet. Start one above."))
		return

	var info := poll.info
	var box := SampleUI.panel(_current)
	SampleUI.key_value(box, "Question", info.title)
	SampleUI.key_value(box, "Status", SampleUI.enum_text("PollStatus", info.status))
	SampleUI.key_value(box, "Started", info.started_at)
	if info.status != Twitch.POLL_STATUS_ACTIVE:
		SampleUI.key_value(box, "Ended", info.ended_at)
	SampleUI.key_value(box, "Channel point voting",
			"On, %s points per vote" % SampleUI.number(info.channel_points_per_vote) if info.channel_points_voting_enabled else "Off")

	var total := 0
	for choice in info.choices:
		total += choice.votes
	SampleUI.spacer(box, 4)
	for choice in info.choices:
		var r := SampleUI.row(box, choice.title)
		SampleUI.bar(r, float(choice.votes) / total if total > 0 else 0.0)
		r.add_child(SampleUI.label("  %s votes (%s with channel points)" % [SampleUI.number(choice.votes), SampleUI.number(choice.channel_points_votes)]))
	SampleUI.key_value(box, "Total votes", total)

	SampleUI.spacer(box, 4)
	var buttons := HBoxContainer.new()
	box.add_child(buttons)
	var is_active := func(): return poll == _poll and poll.is_active()
	app.action_button(buttons, "End poll and show results", func():
		app.run("End poll and show results", func():
			var task := poll.finish_poll()
			await task.completed
			if not app.failed(task, "End poll and show results"):
				app.success("Poll ended. Viewers can see the results.")), is_active)
	app.action_button(buttons, "Cancel poll (hide results)", func():
		app.run("Cancel poll (hide results)", func():
			var task := poll.delete_poll()
			await task.completed
			if not app.failed(task, "Cancel poll (hide results)"):
				app.success("Poll canceled. No results are shown to viewers.")), is_active)
	SampleUI.button(buttons, "Clear", func():
		if _poll:
			_poll.close()
		_poll = null
		_show_poll(null))
