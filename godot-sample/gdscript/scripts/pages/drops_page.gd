class_name DropsPage
extends SamplePage
## Check the logged-in user's Drops, grant the rewards this "game" knows about, and mark them fulfilled.

const STATUS_OPTIONS := ["Claimed (not granted yet)", "Fulfilled", "All"]
# Unknown means "no filter" in the core library.
var STATUS_VALUES := [Twitch.DROP_FULFILLMENT_STATUS_CLAIMED, Twitch.DROP_FULFILLMENT_STATUS_FULFILLED, Twitch.DROP_FULFILLMENT_STATUS_UNKNOWN]

var _reward_ids_edit: LineEdit
var _rewards_entered: Label
var _game_id_edit: LineEdit
var _status_select: OptionButton
var _entitlement_ids_edit: LineEdit
var _results: VBoxContainer
var _entitlements: Array = []
var _loaded := false
var _last_query := ""


func get_title() -> String:
	return "Drops"


func build() -> void:
	add_connection_hint()

	SampleUI.section(self, "Your reward IDs",
			"The benefit IDs of the rewards in your Drops campaign (Twitch developer console > Drops > Rewards). "
			+ "The sample treats these as the items your game knows how to grant. Separate several IDs with commas.")
	_reward_ids_edit = SampleUI.text_row(self, "Reward (benefit) IDs")
	_rewards_entered = SampleUI.key_value(self, "Rewards entered", "0")
	_reward_ids_edit.text_changed.connect(func(text: String):
		_rewards_entered.text = str(SampleUI.split_list(text).size())
		_show_results())

	SampleUI.section(self, "Check for Drops",
			"Lists Drops the logged-in user earned for games owned by your Client ID's organization. "
			+ "No extra scope is needed. There is no live notification for Drops, so check when it makes sense in your game.")
	_game_id_edit = SampleUI.text_row(self, "Game ID (optional)", "", "Leave blank to include all of your games.")
	_status_select = SampleUI.selector(self, "Show", PackedStringArray(STATUS_OPTIONS))
	var buttons := SampleUI.indented(self)
	app.action_button(buttons, "Check for Drops", func(): app.run("Check for Drops", _query))
	app.action_button(buttons, "Grant and fulfill matching rewards", _grant_matching,
			func(): return _loaded and not SampleUI.split_list(_reward_ids_edit.text).is_empty())

	SampleUI.section(self, "Results")
	_results = VBoxContainer.new()
	add_child(_results)
	_show_results()

	SampleUI.section(self, "Fulfill by entitlement ID",
			"Marks specific entitlements fulfilled without checking first. Separate several IDs with commas.")
	_entitlement_ids_edit = SampleUI.text_row(self, "Entitlement IDs")
	app.action_button(SampleUI.indented(self), "Fulfill these entitlements", _fulfill_manual,
			func(): return not SampleUI.split_list(_entitlement_ids_edit.text).is_empty())


## Loads the entitlements (a coroutine; also used after fulfilling).
func _query() -> void:
	var status: int = STATUS_VALUES[_status_select.selected]
	var game_id := _game_id_edit.text.strip_edges()
	var task := Twitch.get_drop_entitlements(game_id, status)
	var result: TwitchDropEntitlementList = await task.completed
	if app.failed(task, "Check for Drops"):
		return
	_entitlements = result.entitlements
	_loaded = true
	_last_query = "%s, %s, checked at %s" % [STATUS_OPTIONS[_status_select.selected],
			"all games" if game_id.is_empty() else "game " + game_id, Time.get_time_string_from_system()]
	_show_results()
	app.success("Found %d Drop entitlement(s)." % _entitlements.size())


func _grant_matching() -> void:
	var known := SampleUI.split_list(_reward_ids_edit.text)
	var matches: Array[TwitchDropEntitlement] = []
	for e: TwitchDropEntitlement in _entitlements:
		if e.fulfillment_status == Twitch.DROP_FULFILLMENT_STATUS_CLAIMED and known.has(e.benefit_id):
			matches.append(e)
	if matches.is_empty():
		app.info("None of the claimed Drops match your reward IDs, so there is nothing to grant.")
		return

	app.run("Grant and fulfill matching rewards", func():
		# A real game adds each item to the player's inventory here, and only fulfills the
		# entitlements it actually granted so the rest are retried next time.
		for e in matches:
			app.info("Granting the in-game item for reward %s (entitlement %s)." % [e.benefit_id, e.id])
		var task := Twitch.fulfill_drop_entitlement_list(matches)
		await task.completed
		if app.failed(task, "Grant and fulfill matching rewards"):
			return
		app.success("Marked %d entitlement(s) fulfilled." % matches.size())
		await _query())


func _show_results() -> void:
	SampleUI.clear(_results)
	if not _loaded:
		_results.add_child(SampleUI.muted("Press Check for Drops to load the logged-in user's entitlements."))
		return
	_results.add_child(SampleUI.muted(_last_query))
	if _entitlements.is_empty():
		_results.add_child(SampleUI.label("No entitlements found. If you expected some, check the filters and that the Drops "
				+ "campaign belongs to the organization that owns your Client ID.", SampleUI.TEXT, 0, false, true))
		return

	var known := SampleUI.split_list(_reward_ids_edit.text)
	for e: TwitchDropEntitlement in _entitlements:
		var box := SampleUI.panel(_results)
		SampleUI.key_value(box, "Entitlement ID", e.id)
		SampleUI.key_value(box, "Reward (benefit) ID", e.benefit_id + ("   (matches your reward IDs)" if known.has(e.benefit_id) else ""))
		SampleUI.key_value(box, "Status", SampleUI.enum_text("DropFulfillmentStatus", e.fulfillment_status))
		SampleUI.key_value(box, "Game ID", e.game_id)
		SampleUI.key_value(box, "Granted", e.timestamp)
		SampleUI.key_value(box, "Last updated", e.last_updated)
		var action := "Fulfill " + e.id
		var id := e.id
		app.action_button(SampleUI.indented(box), action, func():
			app.run(action, func():
				var task := Twitch.fulfill_drop_entitlements(PackedStringArray([id]))
				await task.completed
				if app.failed(task, action):
					return
				app.success("Marked entitlement %s fulfilled." % id)
				await _query()),
				func(): return e.fulfillment_status == Twitch.DROP_FULFILLMENT_STATUS_CLAIMED)


func _fulfill_manual() -> void:
	var ids := SampleUI.split_list(_entitlement_ids_edit.text)
	app.run("Fulfill these entitlements", func():
		var task := Twitch.fulfill_drop_entitlements(ids)
		await task.completed
		if not app.failed(task, "Fulfill these entitlements"):
			app.success("Sent %d entitlement(s) to Twitch to mark fulfilled. Check for Drops to confirm." % ids.size()))
