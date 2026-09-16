"""
Tests for alpr_fsm.py

Run with:  pytest test_alpr_fsm.py -v
"""

import pytest
from alpr_fsm import ALPRFSM, State, Event, TRANSITIONS


# ---------------------------------------------------------------------
# 1. Initial state
# ---------------------------------------------------------------------

def test_initial_state_is_init():
    fsm = ALPRFSM()
    assert fsm.state == State.INIT


# ---------------------------------------------------------------------
# 2. Every transition in the table, tested individually.
#    Driving this off TRANSITIONS itself means the test suite tracks
#    the diagram automatically if the table ever changes.
# ---------------------------------------------------------------------

@pytest.mark.parametrize("start_state, event, expected_state", [
    (start, event, end) for (start, event), end in TRANSITIONS.items()
])
def test_single_transition(start_state, event, expected_state):
    fsm = ALPRFSM()
    fsm.state = start_state  # jump straight to the state under test
    result = fsm.trigger(event)
    assert result == expected_state
    assert fsm.state == expected_state


# ---------------------------------------------------------------------
# 3. Self-loops explicitly: state must NOT change
# ---------------------------------------------------------------------

@pytest.mark.parametrize("state, event", [
    (State.IDLE, Event.BREAKER_SENSOR_OK),
    (State.TAKE_PICTURE, Event.PICTURE_PROCESSING),
    (State.BUZZER, Event.COOLDOWN_TICK),
    (State.OPEN_GATE, Event.GATE_OPEN),
])
def test_self_loops_do_not_change_state(state, event):
    fsm = ALPRFSM()
    fsm.state = state
    fsm.trigger(event)
    assert fsm.state == state


# ---------------------------------------------------------------------
# 4. Invalid transitions: every (state, event) pair NOT in the table
#    must raise, and must leave the state unchanged.
# ---------------------------------------------------------------------

def _invalid_pairs():
    """All (state, event) combinations that are not valid transitions."""
    for state in State:
        for event in Event:
            if (state, event) not in TRANSITIONS:
                yield state, event


@pytest.mark.parametrize("state, event", list(_invalid_pairs()))
def test_invalid_transitions_raise(state, event):
    fsm = ALPRFSM()
    fsm.state = state
    with pytest.raises(ValueError):
        fsm.trigger(event)
    # state must be unchanged after a rejected transition
    assert fsm.state == state


# ---------------------------------------------------------------------
# 5. End-to-end paths through the diagram
# ---------------------------------------------------------------------

def test_happy_path_valid_plate_opens_gate():
    fsm = ALPRFSM()
    fsm.trigger(Event.INIT_DONE)
    fsm.trigger(Event.BREAKER_SENSOR_BROKEN)
    fsm.trigger(Event.PICTURE_PROCESSED)
    fsm.trigger(Event.VALID_PLATE)
    assert fsm.state == State.OPEN_GATE
    fsm.trigger(Event.GATE_CLOSED)
    assert fsm.state == State.IDLE


def test_invalid_plate_sounds_buzzer_then_returns_to_idle():
    fsm = ALPRFSM()
    fsm.trigger(Event.INIT_DONE)
    fsm.trigger(Event.BREAKER_SENSOR_BROKEN)
    fsm.trigger(Event.PICTURE_PROCESSED)
    fsm.trigger(Event.INVALID_PLATE)
    assert fsm.state == State.BUZZER
    fsm.trigger(Event.COOLDOWN_OVER)
    assert fsm.state == State.IDLE


def test_idle_ignores_sensor_ok_and_stays_idle_until_broken():
    fsm = ALPRFSM()
    fsm.trigger(Event.INIT_DONE)
    for _ in range(5):
        fsm.trigger(Event.BREAKER_SENSOR_OK)
        assert fsm.state == State.IDLE
    fsm.trigger(Event.BREAKER_SENSOR_BROKEN)
    assert fsm.state == State.TAKE_PICTURE


# ---------------------------------------------------------------------
# 6. Every declared State and Event is reachable / used at least once
#    in the transition table (catches dead states or typo'd events).
# ---------------------------------------------------------------------

def test_every_state_appears_in_transition_table():
    states_in_table = set()
    for (start, _), end in TRANSITIONS.items():
        states_in_table.add(start)
        states_in_table.add(end)
    assert states_in_table == set(State)


def test_every_event_appears_in_transition_table():
    events_in_table = {event for (_, event) in TRANSITIONS.keys()}
    assert events_in_table == set(Event)
