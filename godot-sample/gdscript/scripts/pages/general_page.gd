class_name GeneralPage
extends SamplePage
## Everything that doesn't belong to another page: users, streams, subscriptions, channel edits,
## stream markers, clips, the Bits leaderboard, channel point rewards, and live events.

const MAX_FEED_ENTRIES := 50
const LEADERBOARD_PERIODS := ["day", "week", "month", "year", "all"]

# Users
var _login_edit: LineEdit
var _user_id_edit: LineEdit
var _user_box: VBoxContainer
# Streams
var _my_stream_box: VBoxContainer
var _stream_logins_edit: LineEdit
var _streams_box: VBoxContainer
# Subscription check
var _broadcaster_id_edit: LineEdit
var _subscription_box: VBoxContainer
# Channel information
var _channel_title_edit: LineEdit
var _channel_game_edit: LineEdit
var _channel_language_edit: LineEdit
var _channel_tags_edit: LineEdit
var _clear_tags_check: CheckBox
# Markers and clips
var _marker_edit: LineEdit
var _marker_box: VBoxContainer
var _clip_delay_check: CheckBox
var _clip_box: VBoxContainer
# Bits leaderboard
var _leaderboard_count_edit: LineEdit
var _leaderboard_period: OptionButton
var _leaderboard_box: VBoxContainer
# Channel point rewards
var _reward_title_edit: LineEdit
var _reward_cost_edit: LineEdit
var _reward_prompt_edit: LineEdit
var _reward_input_check: CheckBox
var _redemptions_box: VBoxContainer
var _pending_redemptions: Array[TwitchCustomRewardEvent] = []
# Live events
var _subscriptions: Array[EventSubscription] = []
var _feed: PackedStringArray = []
var _feed_box: VBoxContainer


func get_title() -> String:
	return "General"


func build() -> void:
	add_connection_hint()
	_build_users()
	_build_streams()
	_build_subscription_check()
	_build_channel_info()
	_build_markers_and_clips()
	_build_bits_leaderboard()
	_build_rewards()
	_build_events()
	# Stop live events after a log out so the toggles match what's actually running.
	app.auth_state_changed.connect(func():
		if not app.is_logged_in():
			for s in _subscriptions:
				s.stop())


# ------------------------------------------------------------------ Users

func _build_users() -> void:
	var c := SampleUI.foldout(self, "Users", "Look up any Twitch user by login name or user ID.")
	app.action_button(SampleUI.indented(c), "Get my user info", func():
		_lookup("Get my user info", func(): return Twitch.get_my_user_info()))

	_login_edit = SampleUI.text_row(c, "Login name")
	app.action_button(SampleUI.indented(c), "Look up by login", func():
		var login := _login_edit.text.strip_edges()
		_lookup("Look up by login", func(): return Twitch.get_user_info_by_login_name(login)),
		func(): return not _login_edit.text.strip_edges().is_empty())

	_user_id_edit = SampleUI.text_row(c, "User ID")
	app.action_button(SampleUI.indented(c), "Look up by ID", func():
		var id := _user_id_edit.text.strip_edges()
		_lookup("Look up by ID", func(): return Twitch.get_user_info_by_id(id)),
		func(): return not _user_id_edit.text.strip_edges().is_empty())

	_user_box = VBoxContainer.new()
	c.add_child(_user_box)


func _lookup(action: String, start: Callable) -> void:
	app.run(action, func():
		var task: TwitchTask = start.call()
		var user: TwitchUserInfo = await task.completed
		if app.failed(task, action):
			return
		SampleUI.clear(_user_box)
		if user == null or user.channel_id.is_empty():
			app.warning("No user found.")
			return
		var box := SampleUI.panel(_user_box)
		SampleUI.key_value(box, "Display name", user.display_name)
		SampleUI.key_value(box, "Login", user.login_name)
		SampleUI.key_value(box, "User ID", user.channel_id)
		SampleUI.key_value(box, "Broadcaster type", user.broadcaster_type if not user.broadcaster_type.is_empty() else "(none)")
		SampleUI.key_value(box, "Description", user.description)
		SampleUI.key_value(box, "Created", user.created_at)
		SampleUI.key_value(box, "Profile image", user.profile_image_url)
		var buttons := SampleUI.indented(box)
		SampleUI.button(buttons, "Use this ID for the subscription check", func(): _broadcaster_id_edit.text = user.channel_id)
		SampleUI.button(buttons, "Open channel", func(): OS.shell_open("https://www.twitch.tv/" + user.login_name))
		app.success("Found %s (user ID %s)." % [user.display_name, user.channel_id]))


# ------------------------------------------------------------------ Streams

func _build_streams() -> void:
	var c := SampleUI.foldout(self, "Streams", "See your own stream and find out which channels are live.")
	app.action_button(SampleUI.indented(c), "Get my stream info", func():
		app.run("Get my stream info", func():
			var task := Twitch.get_my_stream_info()
			var stream: TwitchStreamInfo = await task.completed
			if app.failed(task, "Get my stream info"):
				return
			SampleUI.clear(_my_stream_box)
			var box := SampleUI.panel(_my_stream_box)
			if _is_live(stream):
				_show_stream(box, stream)
				app.success("You are live: " + stream.title)
			else:
				box.add_child(SampleUI.muted("Your channel is offline."))
				app.success("Your channel is offline.")))
	_my_stream_box = VBoxContainer.new()
	c.add_child(_my_stream_box)

	_stream_logins_edit = SampleUI.text_row(c, "Channel logins", "", "Separate several logins with commas (up to 100).")
	app.action_button(SampleUI.indented(c), "Find live streams", func():
		var logins := SampleUI.split_list(_stream_logins_edit.text)
		app.run("Find live streams", func():
			var query := TwitchStreamQuery.new()
			query.user_logins = logins
			query.first = mini(logins.size(), 100)
			var task := Twitch.query_streams(query)
			var result: TwitchStreamQueryResult = await task.completed
			if app.failed(task, "Find live streams"):
				return
			SampleUI.clear(_streams_box)
			if result.streams.is_empty():
				_streams_box.add_child(SampleUI.muted("None of those channels are live."))
			for stream in result.streams:
				_show_stream(SampleUI.panel(_streams_box), stream)
			app.success("%d of %d channel(s) are live." % [result.streams.size(), logins.size()])),
		func(): return not SampleUI.split_list(_stream_logins_edit.text).is_empty())
	_streams_box = VBoxContainer.new()
	c.add_child(_streams_box)


func _is_live(stream: TwitchStreamInfo) -> bool:
	return stream != null and not stream.id.is_empty()


func _show_stream(box: Control, stream: TwitchStreamInfo) -> void:
	SampleUI.key_value(box, "Channel", stream.user_name)
	SampleUI.key_value(box, "Title", stream.title)
	SampleUI.key_value(box, "Category", stream.game_name)
	SampleUI.key_value(box, "Viewers", stream.viewer_count)
	SampleUI.key_value(box, "Started", stream.started_at)
	SampleUI.key_value(box, "Language", stream.language)
	if not stream.tags.is_empty():
		SampleUI.key_value(box, "Tags", ", ".join(stream.tags))


# ------------------------------------------------------------------ Subscription check

func _build_subscription_check() -> void:
	var c := SampleUI.foldout(self, "Subscription check",
			"Checks whether the logged-in user subscribes to a channel. Find a channel's user ID with the Users section above.")
	_broadcaster_id_edit = SampleUI.text_row(c, "Broadcaster user ID")
	app.action_button(SampleUI.indented(c), "Check my subscription", func():
		var id := _broadcaster_id_edit.text.strip_edges()
		app.run("Check my subscription", func():
			var task := Twitch.check_user_subscription(id)
			var result: TwitchUserSubscriptionCheckResult = await task.completed
			if app.failed(task, "Check my subscription"):
				return
			SampleUI.clear(_subscription_box)
			var box := SampleUI.panel(_subscription_box)
			SampleUI.key_value(box, "Broadcaster ID", id)
			SampleUI.key_value(box, "Subscribed", "Yes" if result.is_subscribed else "No")
			if result.is_subscribed:
				SampleUI.key_value(box, "Tier", SampleUI.tier_name(result.tier))
				SampleUI.key_value(box, "Gift", "Yes, from " + result.gifter_name if result.is_gift else "No")
				app.success("You subscribe to %s at tier %s." % [id, SampleUI.tier_name(result.tier)])
			else:
				app.success("You don't subscribe to %s." % id)),
		func(): return not _broadcaster_id_edit.text.strip_edges().is_empty())
	_subscription_box = VBoxContainer.new()
	c.add_child(_subscription_box)


# ------------------------------------------------------------------ Channel information

func _build_channel_info() -> void:
	var c := SampleUI.foldout(self, "Channel information", "Updates your channel. Fill in only the fields you want to change.")
	_channel_title_edit = SampleUI.text_row(c, "Stream title")
	_channel_game_edit = SampleUI.text_row(c, "Category (game) ID", "", "A numeric category ID, e.g. 509658 for Just Chatting.")
	_channel_language_edit = SampleUI.text_row(c, "Language", "", "A two-letter code such as en or de.")
	_channel_tags_edit = SampleUI.text_row(c, "Tags", "", "Separate tags with commas. Tags can't contain spaces.")
	_clear_tags_check = SampleUI.check(c, "Remove all tags")

	var buttons := SampleUI.indented(c)
	app.action_button(buttons, "Load current values", func():
		app.run("Load current values", func():
			var task := Twitch.get_my_stream_info()
			var stream: TwitchStreamInfo = await task.completed
			if app.failed(task, "Load current values"):
				return
			if not _is_live(stream):
				app.info("Your channel is offline, so there are no current values to load. Enter them manually.")
				return
			_channel_title_edit.text = stream.title
			_channel_game_edit.text = stream.game_id
			_channel_language_edit.text = stream.language
			_channel_tags_edit.text = ", ".join(stream.tags)
			app.success("Loaded the current channel values.")))
	app.action_button(buttons, "Update channel", _update_channel)


func _update_channel() -> void:
	var request := TwitchModifyChannelInfoRequest.new()
	request.title = _channel_title_edit.text.strip_edges()
	request.game_id = _channel_game_edit.text.strip_edges()
	request.language = _channel_language_edit.text.strip_edges()
	var tags := SampleUI.split_list(_channel_tags_edit.text)
	# Tags are only changed when some are given, or when they should all be removed.
	request.tags = PackedStringArray() if _clear_tags_check.button_pressed else tags
	request.force_update_tags = _clear_tags_check.button_pressed or not tags.is_empty()
	if request.title.is_empty() and request.game_id.is_empty() and request.language.is_empty() and not request.force_update_tags:
		app.error("Fill in at least one field to change.")
		return
	app.run("Update channel", func():
		var task := Twitch.modify_channel_information(request)
		await task.completed
		if not app.failed(task, "Update channel"):
			app.success("Channel information updated."))


# ------------------------------------------------------------------ Markers and clips

func _build_markers_and_clips() -> void:
	var c := SampleUI.foldout(self, "Stream markers and clips", "Both need your channel to be live.")
	_marker_edit = SampleUI.text_row(c, "Marker description", "Sample marker", "Up to 140 characters.")
	app.action_button(SampleUI.indented(c), "Create stream marker", func():
		var description := _marker_edit.text.strip_edges()
		app.run("Create stream marker", func():
			var task := Twitch.create_stream_marker(description)
			var marker: TwitchStreamMarkerInfo = await task.completed
			if app.failed(task, "Create stream marker"):
				return
			SampleUI.clear(_marker_box)
			var box := SampleUI.panel(_marker_box)
			SampleUI.key_value(box, "Marker ID", marker.id)
			SampleUI.key_value(box, "Description", marker.description)
			SampleUI.key_value(box, "Position", "%d seconds" % marker.position_seconds)
			SampleUI.key_value(box, "Created", marker.created_at)
			app.success("Marker created %d seconds into the stream." % marker.position_seconds)))
	_marker_box = VBoxContainer.new()
	c.add_child(_marker_box)

	SampleUI.spacer(c, 6)
	_clip_delay_check = SampleUI.check(c, "Account for the stream delay when clipping")
	app.action_button(SampleUI.indented(c), "Create clip", func():
		var delay := _clip_delay_check.button_pressed
		app.run("Create clip", func():
			app.info("Creating a clip. Twitch takes a few seconds to process it ...")
			var task := Twitch.create_clip(delay)
			var clip: TwitchClipInfo = await task.completed
			if app.failed(task, "Create clip"):
				return
			SampleUI.clear(_clip_box)
			var box := SampleUI.panel(_clip_box)
			SampleUI.key_value(box, "Clip ID", clip.id)
			SampleUI.key_value(box, "URL", clip.url)
			SampleUI.key_value(box, "Duration", "%d seconds" % clip.duration)
			var open := SampleUI.button(SampleUI.indented(box), "Open clip", func(): OS.shell_open(clip.url))
			open.disabled = clip.url.is_empty()
			app.success("Clip created: " + clip.url)))
	_clip_box = VBoxContainer.new()
	c.add_child(_clip_box)


# ------------------------------------------------------------------ Bits leaderboard

func _build_bits_leaderboard() -> void:
	var c := SampleUI.foldout(self, "Bits leaderboard", "The top cheerers on your channel.")
	_leaderboard_count_edit = SampleUI.text_row(c, "Entries", "10", "1 to 100.")
	_leaderboard_period = SampleUI.selector(c, "Period", PackedStringArray(LEADERBOARD_PERIODS), 4)
	app.action_button(SampleUI.indented(c), "Get Bits leaderboard", func():
		var count_text := _leaderboard_count_edit.text.strip_edges()
		if not count_text.is_valid_int() or count_text.to_int() < 1 or count_text.to_int() > 100:
			app.error("Entries must be a whole number from 1 to 100.")
			return
		var request := TwitchBitsLeaderboardRequest.new()
		request.count = count_text.to_int()
		request.period = LEADERBOARD_PERIODS[_leaderboard_period.selected]
		app.run("Get Bits leaderboard", func():
			var task := Twitch.get_bits_leaderboard(request)
			var leaderboard: TwitchBitsLeaderboard = await task.completed
			if app.failed(task, "Get Bits leaderboard"):
				return
			SampleUI.clear(_leaderboard_box)
			var box := SampleUI.panel(_leaderboard_box)
			if leaderboard.data.is_empty():
				box.add_child(SampleUI.muted("Nobody has cheered in this period."))
			for entry in leaderboard.data:
				SampleUI.key_value(box, "#%d  %s" % [entry.rank, entry.user_name], "%s bits" % SampleUI.number(entry.score))
			app.success("Loaded %d leaderboard entries." % leaderboard.data.size())))
	_leaderboard_box = VBoxContainer.new()
	c.add_child(_leaderboard_box)


# ------------------------------------------------------------------ Channel point rewards

func _build_rewards() -> void:
	var c := SampleUI.foldout(self, "Channel point rewards",
			"Creates a custom reward on your channel. Replacing rewards affects only rewards made with this "
			+ "Client ID. Turn on channel point redemptions under Live events to see and resolve redemptions.")
	_reward_title_edit = SampleUI.text_row(c, "Reward title", "Sample reward", "Up to 45 characters, unique on your channel.")
	_reward_cost_edit = SampleUI.text_row(c, "Cost (channel points)", "100")
	_reward_prompt_edit = SampleUI.text_row(c, "Prompt", "Redeemed from the Twitch SDK Godot sample")
	_reward_input_check = SampleUI.check(c, "Viewers must enter text when redeeming")

	var buttons := SampleUI.indented(c)
	app.action_button(buttons, "Create reward", _create_reward)
	app.action_button(buttons, "Remove sample rewards", func():
		app.run("Remove sample rewards", func():
			var none: Array[TwitchCustomRewardDefinition] = []
			var task := Twitch.replace_custom_rewards(none)
			await task.completed
			if not app.failed(task, "Remove sample rewards"):
				app.success("Removed the rewards created with this Client ID.")))

	_redemptions_box = VBoxContainer.new()
	c.add_child(_redemptions_box)


func _create_reward() -> void:
	var title := _reward_title_edit.text.strip_edges()
	var cost_text := _reward_cost_edit.text.strip_edges()
	if title.is_empty() or title.length() > 45:
		app.error("Reward title is required and can be at most 45 characters.")
		return
	if not cost_text.is_valid_int() or cost_text.to_int() < 1:
		app.error("Cost must be a whole number of at least 1.")
		return
	var definition := TwitchCustomRewardDefinition.new()
	definition.title = title
	definition.cost = cost_text.to_int()
	definition.prompt = _reward_prompt_edit.text.strip_edges()
	definition.is_enabled = true
	definition.is_user_input_required = _reward_input_check.button_pressed
	app.run("Create reward", func():
		var rewards: Array[TwitchCustomRewardDefinition] = [definition]
		var task := Twitch.replace_custom_rewards(rewards)
		await task.completed
		if not app.failed(task, "Create reward"):
			app.success("Reward \"%s\" is now on your channel for %s points." % [title, SampleUI.number(definition.cost)]))


func _show_redemptions() -> void:
	SampleUI.clear(_redemptions_box)
	if _pending_redemptions.is_empty():
		return
	SampleUI.spacer(_redemptions_box, 6)
	_redemptions_box.add_child(SampleUI.label("Waiting redemptions", Color.WHITE, 15, true))
	for redemption in _pending_redemptions:
		var box := SampleUI.panel(_redemptions_box)
		SampleUI.key_value(box, "Reward", redemption.custom_reward_title)
		SampleUI.key_value(box, "Viewer", redemption.redeemer_name)
		if not redemption.user_input.is_empty():
			SampleUI.key_value(box, "Their text", redemption.user_input)
		SampleUI.key_value(box, "Redeemed", redemption.redeemed_at)
		var buttons := SampleUI.indented(box)
		var fulfill := "Fulfill " + redemption.redemption_id
		var refund := "Refund " + redemption.redemption_id
		app.action_button(buttons, fulfill, func(): _resolve(fulfill, redemption, Twitch.CUSTOM_REWARD_REDEMPTION_STATE_FULFILLED))
		app.action_button(buttons, refund, func(): _resolve(refund, redemption, Twitch.CUSTOM_REWARD_REDEMPTION_STATE_CANCELED))
		SampleUI.button(buttons, "Dismiss", func():
			_pending_redemptions.erase(redemption)
			_show_redemptions())


func _resolve(action: String, redemption: TwitchCustomRewardEvent, state: int) -> void:
	app.run(action, func():
		var task := Twitch.resolve_custom_reward_event(redemption, state)
		await task.completed
		if app.failed(task, action):
			return
		_pending_redemptions.erase(redemption)
		_show_redemptions()
		if state == Twitch.CUSTOM_REWARD_REDEMPTION_STATE_FULFILLED:
			app.success("Fulfilled %s's \"%s\"." % [redemption.redeemer_name, redemption.custom_reward_title])
		else:
			app.success("Refunded %s's \"%s\"." % [redemption.redeemer_name, redemption.custom_reward_title]))


func _on_redemption(e: TwitchCustomRewardEvent) -> void:
	_add_to_feed("%s redeemed \"%s\" for %s points.%s" % [e.redeemer_name, e.custom_reward_title,
			SampleUI.number(e.custom_reward_cost), "" if e.user_input.is_empty() else " They wrote: " + e.user_input])
	# Only redemptions still in the queue can be fulfilled or refunded.
	if e.status != Twitch.CUSTOM_REWARD_REDEMPTION_STATE_UNFULFILLED:
		return
	for pending in _pending_redemptions:
		if pending.redemption_id == e.redemption_id:
			return
	_pending_redemptions.append(e)
	_show_redemptions()


# ------------------------------------------------------------------ Live events

func _build_events() -> void:
	var c := SampleUI.foldout(self, "Live events (EventSub)",
			"Turn on the events you want to see. To test without real viewers, enable the local "
			+ "EventSubProxy (Project Settings > Twitch > Debug) and send test events with the Twitch CLI.")

	_add_subscription(c, "Follows (moderator:read:followers)", "follows", Twitch.subscribe_to_channel_follow_events,
			func(e: TwitchChannelFollowEvent): _add_to_feed("%s followed the channel." % e.user_display_name))
	_add_subscription(c, "Subscriptions (channel:read:subscriptions)", "subscriptions", Twitch.subscribe_to_channel_subscribe_events,
			func(e: TwitchChannelSubscribeEvent):
				if e.is_gift:
					_add_to_feed("%s received a gifted tier %s subscription." % [e.user_display_name, SampleUI.tier_name(e.tier)])
				else:
					_add_to_feed("%s subscribed at tier %s (%d months total)." % [e.user_display_name, SampleUI.tier_name(e.tier), e.cumulative_months]))
	_add_subscription(c, "Cheers (bits:read)", "cheers", Twitch.subscribe_to_channel_cheer_events,
			func(e: TwitchChannelCheerEvent):
				_add_to_feed("%s cheered %s bits: %s" % ["An anonymous viewer" if e.is_anonymous else e.user_display_name, SampleUI.number(e.bits), e.message]))
	_add_subscription(c, "Raids (no scope needed)", "raids", Twitch.subscribe_to_channel_raid_events,
			func(e: TwitchChannelRaidEvent):
				_add_to_feed("%s raided %s with %s viewers." % [e.from_broadcaster_name, e.to_broadcaster_name, SampleUI.number(e.viewers)]))
	_add_subscription(c, "Hype Trains (channel:read:hype_train)", "Hype Train updates", Twitch.subscribe_to_hype_train_events,
			func(e: TwitchHypeTrainEvent):
				if e.ended_at.is_empty():
					_add_to_feed("Hype Train level %d: %s / %s points." % [e.level, SampleUI.number(e.progress), SampleUI.number(e.goal)])
				else:
					_add_to_feed("Hype Train ended at level %d with %s points." % [e.level, SampleUI.number(e.total_points)]))
	_add_subscription(c, "Channel point redemptions (channel:manage:redemptions)", "channel point redemptions",
			Twitch.subscribe_to_custom_reward_events, _on_redemption)

	SampleUI.spacer(c, 6)
	var header := HBoxContainer.new()
	var title := SampleUI.label("Event feed (newest first)", Color.WHITE, 15, true)
	title.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	header.add_child(title)
	app.watch_button(SampleUI.button(header, "Clear feed", func():
		_feed.clear()
		_show_feed()), func(): return not _feed.is_empty())
	c.add_child(header)
	_feed_box = SampleUI.panel(c)
	_show_feed()


func _add_subscription(parent: Control, toggle_text: String, events_name: String, subscribe: Callable, on_event: Callable) -> void:
	var subscription := EventSubscription.new(app, events_name, subscribe, on_event)
	_subscriptions.append(subscription)

	var r := HBoxContainer.new()
	var toggle := CheckBox.new()
	toggle.text = toggle_text
	toggle.custom_minimum_size = Vector2(SampleUI.LABEL_WIDTH + 260, 0)
	r.add_child(toggle)
	var state := SampleUI.muted("")
	r.add_child(state)
	parent.add_child(r)

	toggle.toggled.connect(func(on: bool):
		if on:
			subscription.start()
		else:
			subscription.stop())
	subscription.state_changed.connect(_show_subscription_state.bind(subscription, toggle, state))
	app.watch_button(toggle, func(): return app.is_logged_in() or subscription.is_on())


func _show_subscription_state(subscription: EventSubscription, toggle: CheckBox, state: Label) -> void:
	toggle.set_pressed_no_signal(subscription.is_on())
	match subscription.state:
		EventSubscription.State.STARTING:
			state.text = "connecting ..."
			state.add_theme_color_override("font_color", SampleUI.MUTED)
		EventSubscription.State.LISTENING:
			state.text = "listening"
			state.add_theme_color_override("font_color", SampleUI.LEVEL_COLORS[SampleUI.Level.SUCCESS])
		_:
			state.text = ""


func _add_to_feed(line: String) -> void:
	_feed.insert(0, "%s   %s" % [Time.get_time_string_from_system(), line])
	if _feed.size() > MAX_FEED_ENTRIES:
		_feed.resize(MAX_FEED_ENTRIES)
	_show_feed()
	app.info("Event: " + line)


func _show_feed() -> void:
	SampleUI.clear(_feed_box)
	if _feed.is_empty():
		_feed_box.add_child(SampleUI.muted("No events yet."))
	for line in _feed:
		_feed_box.add_child(SampleUI.label(line, SampleUI.TEXT, 0, false, true))
