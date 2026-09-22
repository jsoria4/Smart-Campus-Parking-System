from alpr_fsm import ALPRFSM

FSM = ALPRFSM(
    on_gate_opened = lambda x: x*x,
    on_buzzer_reached = lambda x: x*x,
    initialize_pipeline = lambda x: x*x,
    pipeline_take_picture = lambda x: x*x,
    is_verified_plate = lambda x: x*x,
);