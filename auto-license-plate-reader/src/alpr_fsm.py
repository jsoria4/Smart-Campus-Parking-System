from enum import Enum;

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

class ALPRFSM:
    def __init__(
        self,
        on_gate_opened, # expect no args, return void
        on_buzzer_reached, # expect no args, return void
        initialize_pipeline, # expects no args, initializes pipeline in outside script, return void
        pipeline_take_picture, # expects no args, take & process picture with YOLO and OCR, then return string text of license plate
        is_verified_plate # expects a string, returns a boolean if the plate is valid or not
    ):
        self.state = State.INIT
        self.on_gate_opened = on_gate_opened
        self.on_buzzer_reached = on_buzzer_reached
        self.initialize_pipeline = initialize_pipeline
        self.pipeline_take_picture = pipeline_take_picture
        self.is_verified_plate = is_verified_plate
        self.__init()

    def get_state():
        return self.state

    def __init():
        if self.state == State.INIT:
            self.state = State.IDLE
            # Initialize reader n stuff here

    def breaker_sensor_broken():
        if self.state == State.IDLE:
            self.state = State.TAKE_PICTURE
            self.__take_picture()

    def __take_picture():
        if self.state == State.TAKE_PICTURE:
            # Take picture here, process and send over text
            self.state = State.VERIFY_PLATE
            self.__verify_plate()

    def __verify_plate():
        if self.state == State.VERIFY_PLATE:
            # Look up plate in database
            plate_valid = True
            if (plate_valid):
                self.state = State.OPEN_GATE
                self.on_gate_opened()
                # wait for gate closed to be called
            else:
                self.state = State.BUZZER
                self.__buzzer()

    def __buzzer():
        if self.state == State.BUZZER:
            self.on_buzzer_reached()
            # Trigger buzzer, wait 3 seconds
            self.state = State.IDLE

    def close_gate():
        if self.state == State.OPEN_GATE:
            self.state = State.IDLE