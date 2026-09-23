from alpr_fsm import ALPRFSM
from alpr_pipeline import Pipeline

new_pipeline = None

def init_pipeline():
    new_pipeline = Pipeline(model="../best50_ncnn_model");

def pipeline_take_picture():
    return new_pipeline.take_picture()

# Stub
def is_verified_plate(plate):
    return True

def blank():
    test = "1"
    # Do nothing

FSM = ALPRFSM(
    on_gate_opened = blank,
    on_buzzer_reached = blank,
    initialize_pipeline = init_pipeline,
    pipeline_take_picture = pipeline_take_picture,
    is_verified_plate = is_verified_plate,
);