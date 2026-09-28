from alpr_fsm import ALPRFSM
from alpr_pipeline import Pipeline
from gpiozero import DigitalInputDevice, DigitalOutputDevice
import queue

BEAM_PIN = 17
GATE_CLOSED_PIN = 27
GATE_OPENED_PIN = 22
BUZZER_PIN = 10

# Output signals:
# Open Gate Signal
# Buzzer signal

# Input signals:
# Gate closed signal
# Breaker sensor

pipeline = None
events = None

beam_pin = None
gate_pin = None
open_gate_pin = None
buzzer_pin = None

def init_pipeline():
    open_gate_pin.off()
    buzzer_pin.off()
    pipeline = Pipeline(model="../best50_ncnn_model");

    events = queue.Queue()

    beam_pin = DigitalInputDevice(BEAM_PIN, pull_up=True, bounce_time=0.05)
    gate_pin = DigitalInputDevice(GATE_CLOSED_PIN, pull_up=True, bounce_time=0.05)
    open_gate_pin = DigitalOutputDevice(GATE_OPENED_PIN)
    buzzer_pin = DigitalOutputDevice(BUZZER_PIN)

    beam_pin.when_activated = lambda: events.put(FSM.breaker_sensor_broken)
    gate_pin.when_activated = lambda: events.put(FSM.close_gate)

def pipeline_take_picture():
    return pipeline.take_picture()

# Stub
def is_verified_plate(plate):
    return True

def is_beam_broken():
    return False;

def on_gate_opened():
    open_gate_pin.on()

def on_buzzer_reached():
    buzzer_pin.on()

FSM = ALPRFSM(
    on_gate_opened = on_gate_opened,
    on_buzzer_reached = on_buzzer_reached,
    initialize_pipeline = init_pipeline,
    pipeline_take_picture = pipeline_take_picture,
    is_verified_plate = is_verified_plate,
    is_beam_broken = is_beam_broken,
);

# blocking event queue
while True:
    handler = events.get()
    handler()