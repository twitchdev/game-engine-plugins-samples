class_name EventSubscription
extends RefCounted
## Keeps one EventSub stream open and hands each event to a callback until stopped.
##
## A generation counter makes a stopped (or restarted) loop exit quietly instead of delivering
## late events.

signal state_changed

enum State { OFF, STARTING, LISTENING }

var name: String
var state := State.OFF

var _app: SampleApp
var _subscribe: Callable
var _on_event: Callable
var _stream: TwitchEventStream
var _generation := 0


## `subscribe` returns the TwitchTask of a Twitch.subscribe_to_*_events() call.
func _init(app: SampleApp, p_name: String, subscribe: Callable, on_event: Callable) -> void:
	_app = app
	name = p_name
	_subscribe = subscribe
	_on_event = on_event


func is_on() -> bool:
	return state != State.OFF


func start() -> void:
	if state != State.OFF or not Twitch.is_initialized():
		return
	_generation += 1
	var my_generation := _generation
	_set_state(State.STARTING)

	var task: TwitchTask = _subscribe.call()
	var stream: TwitchEventStream = await task.completed
	if my_generation != _generation:
		if stream:
			stream.close() # stopped while subscribing
		return
	if task.has_error():
		_app.error("%s: %s" % [name, task.error])
		_set_state(State.OFF)
		return

	_stream = stream
	_set_state(State.LISTENING)
	_app.success("Listening for %s." % name)

	while my_generation == _generation:
		var wait := stream.wait_for_event()
		var event = await wait.completed
		if my_generation != _generation:
			break
		if wait.has_error():
			_app.error("%s: %s" % [name, wait.error])
			break
		_on_event.call(event)

	if my_generation == _generation:
		_stream = null
		_set_state(State.OFF)
	stream.close() # closes the EventSub subscription; safe to call more than once


func stop(log := true) -> void:
	if state == State.OFF:
		return
	_generation += 1
	var current := _stream
	_stream = null
	_set_state(State.OFF)
	if current:
		current.close()
	if log:
		_app.info("Stopped listening for %s." % name)


func _set_state(new_state: State) -> void:
	state = new_state
	state_changed.emit()
