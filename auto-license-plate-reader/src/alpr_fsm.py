"""
Automatic License Plate Reader FSM

Implements only the states and transitions shown in the diagram.
No internal per-state logic (sensor reads, picture taking, plate
verification, buzzer, or gate control) is implemented -- each state's
behavior is left as a stub (`pass`) for the caller to fill in.
"""

from enum import Enum, auto


class State(Enum):
    INIT = auto()
    IDLE = auto()
    TAKE_PICTURE = auto()
    VERIFY_PLATE = auto()
    BUZZER = auto()
    OPEN_GATE = auto()


class Event(Enum):
    RESET = auto()                     # Init -> Init (entry trigger)
    BREAKER_SENSOR_OK = auto()         # Idle -> Idle (self loop)
    BREAKER_SENSOR_BROKEN = auto()     # Idle -> Take Picture
    PICTURE_PROCESSING = auto()        # Take Picture -> Take Picture (self loop)
    PICTURE_PROCESSED = auto()         # Take Picture -> Verify Plate
    VALID_PLATE = auto()               # Verify Plate -> Open Gate
    INVALID_PLATE = auto()             # Verify Plate -> Buzzer
    COOLDOWN_TICK = auto()             # Buzzer -> Buzzer (self loop)
    COOLDOWN_OVER = auto()             # Buzzer -> Idle
    GATE_OPEN = auto()                 # Open Gate -> Open Gate (self loop)
    GATE_CLOSED = auto()               # Open Gate -> Idle
    INIT_DONE = auto()                 # Init -> Idle


# Transition table: (current_state, event) -> next_state
TRANSITIONS = {
    (State.INIT, Event.RESET): State.INIT,
    (State.INIT, Event.INIT_DONE): State.IDLE,

    (State.IDLE, Event.BREAKER_SENSOR_OK): State.IDLE,
    (State.IDLE, Event.BREAKER_SENSOR_BROKEN): State.TAKE_PICTURE,

    (State.TAKE_PICTURE, Event.PICTURE_PROCESSING): State.TAKE_PICTURE,
    (State.TAKE_PICTURE, Event.PICTURE_PROCESSED): State.VERIFY_PLATE,

    (State.VERIFY_PLATE, Event.VALID_PLATE): State.OPEN_GATE,
    (State.VERIFY_PLATE, Event.INVALID_PLATE): State.BUZZER,

    (State.BUZZER, Event.COOLDOWN_TICK): State.BUZZER,
    (State.BUZZER, Event.COOLDOWN_OVER): State.IDLE,

    (State.OPEN_GATE, Event.GATE_OPEN): State.OPEN_GATE,
    (State.OPEN_GATE, Event.GATE_CLOSED): State.IDLE,
}


class ALPRFSM:
    def __init__(self):
        self.state = State.INIT

    def trigger(self, event: Event) -> State:
        """Advance the FSM on `event`. Raises ValueError if the event
        is not valid from the current state."""
        key = (self.state, event)
        if key not in TRANSITIONS:
            raise ValueError(f"No transition for event {event.name} in state {self.state.name}")
        self.state = TRANSITIONS[key]
        return self.state

    # --- Per-state hooks (no logic implemented, left as stubs) ---

    def on_init(self):
        pass

    def on_idle(self):
        pass

    def on_take_picture(self):
        pass

    def on_verify_plate(self):
        pass

    def on_buzzer(self):
        pass

    def on_open_gate(self):
        pass


if __name__ == "__main__":
    fsm = ALPRFSM()
    print(fsm.state)                              # State.INIT
    fsm.trigger(Event.INIT_DONE)
    print(fsm.state)                              # State.IDLE
    fsm.trigger(Event.BREAKER_SENSOR_BROKEN)
    print(fsm.state)                              # State.TAKE_PICTURE
    fsm.trigger(Event.PICTURE_PROCESSED)
    print(fsm.state)                              # State.VERIFY_PLATE
    fsm.trigger(Event.VALID_PLATE)
    print(fsm.state)                              # State.OPEN_GATE
    fsm.trigger(Event.GATE_CLOSED)
    print(fsm.state)                              # State.IDLE
