from enum import Enum, auto;
import inspect

# Need to expose events to wiring script
# Expose one for gate opened
# Expose one for buzzer

class State(Enum):
    INIT = auto()
    IDLE = auto()
    TAKE_PICTURE = auto()
    VERIFY_PLATE = auto()
    BUZZER = auto()
    OPEN_GATE = auto()

def _check_callable(name, fn, *sample_args):
    # Raises TypeError unless fn is callable with exactly sample_args
    if not callable(fn):
        raise TypeError(f"{name} must be callable, got {type(fn).__name__}")
    try:
        signature = inspect.signature(fn)
    except (TypeError, ValueError):
        return # Some builtins have no readable signature, trust them
    try:
        signature.bind(*sample_args)
    except TypeError:
        raise TypeError(
            f"{name} must accept {len(sample_args)} argument(s), signature is {signature}"
        ) from None

class ALPRFSM:
    def __init__(
        self,
        on_gate_opened, # expect no args, return void
        on_buzzer_reached, # expect no args, return void
        initialize_pipeline, # expects no args, initializes pipeline in outside script, return void
        pipeline_take_picture, # expects no args, take & process picture with YOLO and OCR, then return string text of license plate
        is_verified_plate # expects a string, returns a boolean if the plate is valid or not
    ):
        # Validate everything before touching the pipeline
        _check_callable("on_gate_opened", on_gate_opened)
        _check_callable("on_buzzer_reached", on_buzzer_reached)
        _check_callable("initialize_pipeline", initialize_pipeline)
        _check_callable("pipeline_take_picture", pipeline_take_picture)
        _check_callable("is_verified_plate", is_verified_plate, "plate")

        self.state = State.INIT
        self.last_plate = ""
        self.on_gate_opened = on_gate_opened
        self.on_buzzer_reached = on_buzzer_reached
        self.initialize_pipeline = initialize_pipeline
        self.pipeline_take_picture = pipeline_take_picture
        self.is_verified_plate = is_verified_plate
        self.__init()

    def get_state(self):
        return self.state

    def __init(self):
        if self.state == State.INIT:
            self.state = State.IDLE
            self.initialize_pipeline()
            # Initialize reader n stuff here

    def breaker_sensor_broken(self):
        if self.state == State.IDLE:
            self.state = State.TAKE_PICTURE
            self.__take_picture()

    def __take_picture(self):
        if self.state == State.TAKE_PICTURE:
            # Take picture here, process and send over text
            plate = self.pipeline_take_picture()
            if not isinstance(plate, str):
                self.state = State.IDLE
                raise TypeError(
                    f"pipeline_take_picture must return str, got {type(plate).__name__}"
                )
            self.last_plate = plate
            self.state = State.VERIFY_PLATE
            self.__verify_plate()

    def __verify_plate(self):
        if self.state == State.VERIFY_PLATE:
            # Look up plate in database

            plate_valid = self.is_verified_plate(self.last_plate)
            # Strict check: 1/0 or "True" should not open the gate
            if type(plate_valid) is not bool:
                self.state = State.IDLE
                raise TypeError(
                    f"is_verified_plate must return bool, got {type(plate_valid).__name__}"
                )
            if (plate_valid):
                self.state = State.OPEN_GATE
                self.on_gate_opened()
                # wait for gate closed to be called
            else:
                self.state = State.BUZZER
                self.__buzzer()

    def __buzzer(self):
        if self.state == State.BUZZER:
            self.on_buzzer_reached()
            # Trigger buzzer, wait 3 seconds
            self.state = State.IDLE

    def close_gate(self):
        if self.state == State.OPEN_GATE:
            self.state = State.IDLE