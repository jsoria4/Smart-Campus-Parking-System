from alpr_fsm import ALPRFSM, State
from alpr_pipeline import Pipeline
from gpiozero import DigitalInputDevice, DigitalOutputDevice
import queue
import time

BEAM_PIN = 17
GATE_CLOSED_PIN = 27
GATE_OPENED_PIN = 22
BUZZER_PIN = 10

BUZZER_COOLDOWN_SECONDS = 3

# Output signals:
# Open Gate Signal
# Buzzer signal

# Input signals:
# Gate closed signal
# Breaker sensor

# GPIO and the event queue have to exist before the FSM is built: the
# constructor reports INIT -> IDLE and may run a whole cycle if a car is
# already in the beam at boot.
events = queue.Queue()

beam_pin = DigitalInputDevice(BEAM_PIN, pull_up=True, bounce_time=0.05)
gate_pin = DigitalInputDevice(GATE_CLOSED_PIN, pull_up=True, bounce_time=0.05)
open_gate_pin = DigitalOutputDevice(GATE_OPENED_PIN)
buzzer_pin = DigitalOutputDevice(BUZZER_PIN)

# Queue event names, not FSM.<method>: an edge during construction would hit
# FSM before it is assigned. Queued names wait until the loop below starts.
beam_pin.when_activated = lambda: events.put("breaker_sensor_broken")
gate_pin.when_activated = lambda: events.put("close_gate")

pipeline = None

def init_pipeline():
    global pipeline
    pipeline = Pipeline(model="../best50_ncnn_model")

def pipeline_take_picture():
    return pipeline.take_picture()

# Stub
def is_verified_plate(plate):
    return True

def is_beam_broken():
    return beam_pin.is_active

def on_state_changed(state):
    # Outputs follow state only: each pin is high in exactly one state
    open_gate_pin.value = state == State.OPEN_GATE
    buzzer_pin.value = state == State.BUZZER

def on_buzzer_reached():
    # The FSM stays in BUZZER (buzzer pin high) until this returns
    time.sleep(BUZZER_COOLDOWN_SECONDS)

FSM = ALPRFSM(
    on_state_changed = on_state_changed,
    on_buzzer_reached = on_buzzer_reached,
    initialize_pipeline = init_pipeline,
    pipeline_take_picture = pipeline_take_picture,
    is_verified_plate = is_verified_plate,
    is_beam_broken = is_beam_broken,
)

# Blocking event queue: sleeps until a GPIO edge queues an event
while True:
    getattr(FSM, events.get())()
