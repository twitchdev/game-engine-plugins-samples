class_name PredictionsPage
extends SamplePage
## Create a prediction from your own title and outcomes, then lock, resolve, or cancel it.

# Limits enforced by the Twitch Create Prediction endpoint.
const MAX_TITLE_LENGTH := 45
const MAX_OUTCOME_LENGTH := 25
const MIN_OUTCOMES := 2
const MAX_OUTCOMES := 10
const MIN_DURATION := 30
const MAX_DURATION := 1800

var _title_edit: LineEdit
var _outcomes: StringListEditor
var _duration_edit: LineEdit
var _current: VBoxContainer
var _prediction: TwitchPrediction


func get_title() -> String:
	return "Predictions"


func build() -> void:
	add_connection_hint()

	SampleUI.section(self, "Create a prediction",
			"Viewers bet channel points on an outcome. You need to be a Twitch Affiliate or Partner, and "
			+ "Twitch allows one active prediction at a time.")
	_title_edit = SampleUI.text_row(self, "Title", "Will we beat the boss?", "Up to %d characters." % MAX_TITLE_LENGTH)
	_outcomes = StringListEditor.new("Outcome", PackedStringArray(["Yes", "No"]), MIN_OUTCOMES, MAX_OUTCOMES,
			"%d to %d outcomes, up to %d characters each." % [MIN_OUTCOMES, MAX_OUTCOMES, MAX_OUTCOME_LENGTH])
	add_child(_outcomes)
	_duration_edit = SampleUI.text_row(self, "Voting window (seconds)", "120",
			"%d to %d. The prediction locks when it runs out." % [MIN_DURATION, MAX_DURATION])
	app.action_button(SampleUI.indented(self), "Start prediction", _start_prediction)

	SampleUI.section(self, "Current prediction", "Totals update live while viewers place bets.")
	_current = VBoxContainer.new()
	add_child(_current)
	_show_prediction(null)


func _start_prediction() -> void:
	var title := _title_edit.text.strip_edges()
	var outcomes := _outcomes.get_items()
	if title.is_empty():
		app.error("Title is required.")
		return
	if title.length() > MAX_TITLE_LENGTH:
		app.error("Title can be at most %d characters." % MAX_TITLE_LENGTH)
		return
	if outcomes.size() < MIN_OUTCOMES:
		app.error("Enter at least %d outcomes." % MIN_OUTCOMES)
		return
	for outcome in outcomes:
		if outcome.length() > MAX_OUTCOME_LENGTH:
			app.error("Outcome \"%s\" is longer than %d characters." % [outcome, MAX_OUTCOME_LENGTH])
			return
	var seconds := _duration_edit.text.strip_edges()
	if not seconds.is_valid_int() or seconds.to_int() < MIN_DURATION or seconds.to_int() > MAX_DURATION:
		app.error("Voting window must be a whole number from %d to %d." % [MIN_DURATION, MAX_DURATION])
		return

	var definition := TwitchPredictionDefinition.new()
	definition.title = title
	definition.outcomes = outcomes
	definition.duration = seconds.to_int()

	app.run("Start prediction", func():
		var task := Twitch.new_prediction(definition)
		var created: TwitchPrediction = await task.completed
		if app.failed(task, "Start prediction"):
			return
		# Keep the previous prediction until the new one exists, then stop tracking it.
		if _prediction:
			_prediction.close()
		_prediction = created
		created.updated.connect(func(_info):
			if created == _prediction:
				_show_prediction(created))
		created.locked.connect(func(info: TwitchPredictionInfo):
			if created == _prediction:
				app.info("Prediction \"%s\" is locked. Pick the winning outcome to pay out." % info.title))
		_show_prediction(created)
		app.success("Prediction \"%s\" started. Voting closes in %s seconds." % [created.info.title, seconds]))


func _show_prediction(prediction: TwitchPrediction) -> void:
	SampleUI.clear(_current)
	if prediction == null:
		_current.add_child(SampleUI.muted("No prediction yet. Start one above."))
		return

	var info := prediction.info
	var box := SampleUI.panel(_current)
	SampleUI.key_value(box, "Title", info.title)
	SampleUI.key_value(box, "Status", SampleUI.enum_text("PredictionStatus", info.status))
	SampleUI.key_value(box, "Created", info.created_at)
	if not info.locked_at.is_empty():
		SampleUI.key_value(box, "Locked", info.locked_at)
	if not info.ended_at.is_empty():
		SampleUI.key_value(box, "Ended", info.ended_at)

	var is_open := func():
		var status := prediction.info.status
		return prediction == _prediction and (status == Twitch.PREDICTION_STATUS_ACTIVE or status == Twitch.PREDICTION_STATUS_LOCKED)
	var total_points := 0
	for outcome in info.outcomes:
		total_points += outcome.channel_points

	SampleUI.spacer(box, 4)
	for outcome in info.outcomes:
		var winner := not info.winning_outcome_id.is_empty() and outcome.id == info.winning_outcome_id
		var r := SampleUI.row(box, ("Winner: " if winner else "") + outcome.title)
		SampleUI.bar(r, float(outcome.channel_points) / total_points if total_points > 0 else 0.0)
		var details := SampleUI.label("  %s viewers, %s points (%s)" % [SampleUI.number(outcome.users), SampleUI.number(outcome.channel_points), outcome.color])
		details.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		r.add_child(details)
		var action := "Resolve: " + outcome.title
		app.action_button(r, action, func():
			app.run(action, func():
				var task := prediction.resolve(outcome)
				await task.completed
				if not app.failed(task, action):
					app.success("Prediction resolved. \"%s\" won and points were paid out." % outcome.title)), is_open)

	SampleUI.spacer(box, 4)
	var buttons := HBoxContainer.new()
	box.add_child(buttons)
	app.action_button(buttons, "Lock voting now", func():
		app.run("Lock voting now", func():
			var task := prediction.lock()
			await task.completed
			if not app.failed(task, "Lock voting now"):
				app.success("Prediction locked. Viewers can no longer place bets.")),
			func(): return prediction == _prediction and prediction.info.status == Twitch.PREDICTION_STATUS_ACTIVE)
	app.action_button(buttons, "Cancel prediction (refund points)", func():
		app.run("Cancel prediction (refund points)", func():
			var task := prediction.cancel()
			await task.completed
			if not app.failed(task, "Cancel prediction (refund points)"):
				app.success("Prediction canceled. Channel points were refunded.")), is_open)
	SampleUI.button(buttons, "Clear", func():
		if _prediction:
			_prediction.close()
		_prediction = null
		_show_prediction(null))
