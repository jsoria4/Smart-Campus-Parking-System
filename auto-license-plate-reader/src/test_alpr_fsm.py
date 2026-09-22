"""
Tests for alpr_fsm.py

Run from this directory with:  python -m unittest test_alpr_fsm -v
(pytest also collects these:    pytest test_alpr_fsm.py -v)

Contracts under test (from the ALPRFSM constructor):
    on_gate_opened        no args, returns None
    on_buzzer_reached     no args, returns None
    initialize_pipeline   no args, returns None
    pipeline_take_picture no args, returns str (license plate text)
    is_verified_plate     one str arg, returns bool

Public API under test:
    ALPRFSM.get_state()
    ALPRFSM.breaker_sensor_broken()
    ALPRFSM.close_gate()

The FSM is synchronous. Once breaker_sensor_broken() fires in IDLE it runs
TAKE_PICTURE -> VERIFY_PLATE -> (OPEN_GATE | BUZZER) before returning.
  * OPEN_GATE is left by close_gate().
  * BUZZER is left when on_buzzer_reached() returns (that callback is where
    the 3 second cooldown happens), then the FSM is back in IDLE.
Every event that has no arrow out of the current state (the self-loops in the
diagram) is ignored: no state change and no injected function is called.
"""

import unittest

from alpr_fsm import ALPRFSM, State


CONSTRUCTOR_ARGS = (
    "on_gate_opened",
    "on_buzzer_reached",
    "initialize_pipeline",
    "pipeline_take_picture",
    "is_verified_plate",
)


class Harness:
    """
    Builds an ALPRFSM out of recording fakes.

    calls        ordered list of the injected function names that were called
    states       fsm.get_state() as observed *inside* each injected function
    plate        what pipeline_take_picture returns
    valid        what is_verified_plate returns
    plate_seen   the argument is_verified_plate received
    hooks        optional {name: callable} run inside the named injected
                 function, used to poke the FSM re-entrantly
    """

    def __init__(self, plate="ABC1234", valid=True):
        self.plate = plate
        self.valid = valid
        self.calls = []
        self.states = {}
        self.plate_seen = None
        self.hooks = {}
        self.fsm = None
        self.fsm = ALPRFSM(**self.kwargs())

    def kwargs(self):
        return {name: getattr(self, name) for name in CONSTRUCTOR_ARGS}

    def _record(self, name):
        self.calls.append(name)
        # self.fsm is None while the constructor is still running
        self.states[name] = self.fsm.get_state() if self.fsm else None
        if name in self.hooks:
            self.hooks[name]()

    def count(self, name):
        return self.calls.count(name)

    def on_gate_opened(self):
        self._record("on_gate_opened")

    def on_buzzer_reached(self):
        self._record("on_buzzer_reached")

    def initialize_pipeline(self):
        self._record("initialize_pipeline")

    def pipeline_take_picture(self):
        self._record("pipeline_take_picture")
        return self.plate

    def is_verified_plate(self, plate):
        self._record("is_verified_plate")
        self.plate_seen = plate
        return self.valid


def good_kwargs():
    """A complete, valid set of constructor arguments."""
    return Harness().kwargs()


# ---------------------------------------------------------------------
# 1. Invalid input: constructor arguments
# ---------------------------------------------------------------------

class TestConstructorValidation(unittest.TestCase):

    NOT_CALLABLE = [None, "not a function", 42, 3.14, True, [], {}, object()]

    def test_valid_arguments_are_accepted(self):
        ALPRFSM(**good_kwargs())

    def test_non_callable_argument_raises_type_error(self):
        for name in CONSTRUCTOR_ARGS:
            for bad in self.NOT_CALLABLE:
                with self.subTest(argument=name, value=bad):
                    kwargs = good_kwargs()
                    kwargs[name] = bad
                    with self.assertRaises(TypeError):
                        ALPRFSM(**kwargs)

    def test_missing_argument_raises_type_error(self):
        for name in CONSTRUCTOR_ARGS:
            with self.subTest(missing=name):
                kwargs = good_kwargs()
                del kwargs[name]
                with self.assertRaises(TypeError):
                    ALPRFSM(**kwargs)

    def test_no_arguments_raises_type_error(self):
        with self.assertRaises(TypeError):
            ALPRFSM()

    def test_no_arg_callables_that_require_an_argument_are_rejected(self):
        for name in ("on_gate_opened", "on_buzzer_reached",
                     "initialize_pipeline", "pipeline_take_picture"):
            with self.subTest(argument=name):
                kwargs = good_kwargs()
                kwargs[name] = lambda required_arg: None
                with self.assertRaises(TypeError):
                    ALPRFSM(**kwargs)

    def test_is_verified_plate_that_takes_no_argument_is_rejected(self):
        kwargs = good_kwargs()
        kwargs["is_verified_plate"] = lambda: True
        with self.assertRaises(TypeError):
            ALPRFSM(**kwargs)

    def test_is_verified_plate_that_needs_two_arguments_is_rejected(self):
        kwargs = good_kwargs()
        kwargs["is_verified_plate"] = lambda plate, extra: True
        with self.assertRaises(TypeError):
            ALPRFSM(**kwargs)

    def test_invalid_argument_is_rejected_before_initializing_pipeline(self):
        # Validation must happen up front so a bad wiring script does not
        # leave a half-initialized pipeline behind.
        pipeline_inits = []
        kwargs = good_kwargs()
        kwargs["initialize_pipeline"] = lambda: pipeline_inits.append(1)
        kwargs["on_gate_opened"] = None
        with self.assertRaises(TypeError):
            ALPRFSM(**kwargs)
        self.assertEqual(pipeline_inits, [])


# ---------------------------------------------------------------------
# 2. Invalid input: injected functions that break their return contract
#    at runtime. The FSM must raise TypeError, must not act on the bad
#    value (no gate, no buzzer), and must not get stuck mid-cycle.
# ---------------------------------------------------------------------

class TestReturnContractViolations(unittest.TestCase):

    def test_pipeline_take_picture_returning_non_str_raises(self):
        for bad in (None, 1234, 12.5, b"ABC1234", ["A", "B", "C"], True):
            with self.subTest(returned=bad):
                h = Harness(plate=bad)
                with self.assertRaises(TypeError):
                    h.fsm.breaker_sensor_broken()
                self.assertEqual(h.count("is_verified_plate"), 0)
                self.assertEqual(h.count("on_gate_opened"), 0)
                self.assertEqual(h.count("on_buzzer_reached"), 0)
                self.assertEqual(h.fsm.get_state(), State.IDLE)

    def test_is_verified_plate_returning_non_bool_raises(self):
        for bad in (None, "True", "yes", 1, 0, [], [True]):
            with self.subTest(returned=bad):
                h = Harness(valid=bad)
                with self.assertRaises(TypeError):
                    h.fsm.breaker_sensor_broken()
                self.assertEqual(h.count("on_gate_opened"), 0)
                self.assertEqual(h.count("on_buzzer_reached"), 0)
                self.assertEqual(h.fsm.get_state(), State.IDLE)

    def test_fsm_recovers_after_a_contract_violation(self):
        h = Harness(plate=None)
        with self.assertRaises(TypeError):
            h.fsm.breaker_sensor_broken()
        # The wiring script is fixed; the next car must still be handled.
        h.plate = "ABC1234"
        h.valid = True
        h.fsm.breaker_sensor_broken()
        self.assertEqual(h.fsm.get_state(), State.OPEN_GATE)
        self.assertEqual(h.count("on_gate_opened"), 1)

    def test_empty_string_is_a_valid_plate_and_is_forwarded(self):
        # "" satisfies the "returns a string" contract; whether it is a
        # real plate is is_verified_plate's call, not the FSM's.
        h = Harness(plate="", valid=False)
        h.fsm.breaker_sensor_broken()
        self.assertEqual(h.plate_seen, "")
        self.assertEqual(h.count("on_buzzer_reached"), 1)


# ---------------------------------------------------------------------
# 3. Diagram branches
# ---------------------------------------------------------------------

class TestInit(unittest.TestCase):
    """Reset -> INIT -> IDLE"""

    def test_construction_ends_in_idle(self):
        self.assertEqual(Harness().fsm.get_state(), State.IDLE)

    def test_construction_initializes_pipeline_exactly_once(self):
        h = Harness()
        self.assertEqual(h.calls, ["initialize_pipeline"])

    def test_construction_does_not_take_a_picture_or_fire_outputs(self):
        h = Harness()
        for name in ("pipeline_take_picture", "is_verified_plate",
                     "on_gate_opened", "on_buzzer_reached"):
            self.assertEqual(h.count(name), 0, name)


class TestIdle(unittest.TestCase):
    """IDLE self-loop (sensor not broken) and IDLE -> TAKE_PICTURE"""

    def test_idle_stays_idle_and_does_nothing_when_sensor_is_not_broken(self):
        # No breaker_sensor_broken() call == sensor not broken.
        h = Harness()
        self.assertEqual(h.fsm.get_state(), State.IDLE)
        self.assertEqual(h.calls, ["initialize_pipeline"])

    def test_close_gate_in_idle_is_ignored(self):
        h = Harness()
        h.fsm.close_gate()
        self.assertEqual(h.fsm.get_state(), State.IDLE)
        self.assertEqual(h.calls, ["initialize_pipeline"])

    def test_sensor_broken_takes_exactly_one_picture(self):
        h = Harness()
        h.fsm.breaker_sensor_broken()
        self.assertEqual(h.count("pipeline_take_picture"), 1)


class TestTakePicture(unittest.TestCase):
    """IDLE -> TAKE_PICTURE, self-loop while processing, -> VERIFY_PLATE"""

    def test_state_is_take_picture_while_picture_is_processing(self):
        h = Harness()
        h.fsm.breaker_sensor_broken()
        self.assertEqual(h.states["pipeline_take_picture"], State.TAKE_PICTURE)

    def test_events_while_processing_are_ignored(self):
        # "Picture Processing" self-loop: a second sensor break (or a gate
        # close) while the picture is still processing changes nothing.
        h = Harness()
        seen = {}

        def poke():
            h.fsm.breaker_sensor_broken()
            h.fsm.close_gate()
            seen["state"] = h.fsm.get_state()

        h.hooks["pipeline_take_picture"] = poke
        h.fsm.breaker_sensor_broken()
        self.assertEqual(seen["state"], State.TAKE_PICTURE)
        self.assertEqual(h.count("pipeline_take_picture"), 1)
        self.assertEqual(h.count("is_verified_plate"), 1)

    def test_processed_picture_moves_on_to_verify_plate(self):
        h = Harness()
        h.fsm.breaker_sensor_broken()
        self.assertEqual(h.states["is_verified_plate"], State.VERIFY_PLATE)

    def test_plate_text_is_passed_unchanged_to_is_verified_plate(self):
        h = Harness(plate="XYZ 987")
        h.fsm.breaker_sensor_broken()
        self.assertEqual(h.plate_seen, "XYZ 987")
        self.assertEqual(h.count("is_verified_plate"), 1)

    def test_picture_is_taken_before_plate_is_verified(self):
        h = Harness()
        h.fsm.breaker_sensor_broken()
        self.assertLess(h.calls.index("pipeline_take_picture"),
                        h.calls.index("is_verified_plate"))


class TestValidPlate(unittest.TestCase):
    """VERIFY_PLATE -> OPEN_GATE (valid plate)"""

    def setUp(self):
        self.h = Harness(valid=True)
        self.h.fsm.breaker_sensor_broken()

    def test_state_is_open_gate(self):
        self.assertEqual(self.h.fsm.get_state(), State.OPEN_GATE)

    def test_gate_opened_callback_fires_exactly_once(self):
        self.assertEqual(self.h.count("on_gate_opened"), 1)

    def test_gate_opened_callback_sees_open_gate_state(self):
        self.assertEqual(self.h.states["on_gate_opened"], State.OPEN_GATE)

    def test_buzzer_does_not_fire(self):
        self.assertEqual(self.h.count("on_buzzer_reached"), 0)

    def test_gate_opens_after_verification(self):
        self.assertLess(self.h.calls.index("is_verified_plate"),
                        self.h.calls.index("on_gate_opened"))


class TestOpenGate(unittest.TestCase):
    """OPEN_GATE self-loop (gate open) and OPEN_GATE -> IDLE (gate closed)"""

    def setUp(self):
        self.h = Harness(valid=True)
        self.h.fsm.breaker_sensor_broken()

    def test_sensor_broken_while_gate_open_is_ignored(self):
        for _ in range(3):
            self.h.fsm.breaker_sensor_broken()
            self.assertEqual(self.h.fsm.get_state(), State.OPEN_GATE)
        self.assertEqual(self.h.count("pipeline_take_picture"), 1)
        self.assertEqual(self.h.count("is_verified_plate"), 1)
        self.assertEqual(self.h.count("on_gate_opened"), 1)

    def test_close_gate_returns_to_idle(self):
        self.h.fsm.close_gate()
        self.assertEqual(self.h.fsm.get_state(), State.IDLE)

    def test_close_gate_fires_no_callbacks(self):
        before = list(self.h.calls)
        self.h.fsm.close_gate()
        self.assertEqual(self.h.calls, before)

    def test_second_close_gate_is_ignored(self):
        self.h.fsm.close_gate()
        before = list(self.h.calls)
        self.h.fsm.close_gate()
        self.assertEqual(self.h.fsm.get_state(), State.IDLE)
        self.assertEqual(self.h.calls, before)

    def test_new_car_is_handled_after_gate_closes(self):
        self.h.fsm.close_gate()
        self.h.fsm.breaker_sensor_broken()
        self.assertEqual(self.h.count("pipeline_take_picture"), 2)
        self.assertEqual(self.h.fsm.get_state(), State.OPEN_GATE)


class TestInvalidPlate(unittest.TestCase):
    """VERIFY_PLATE -> BUZZER (invalid plate) -> IDLE (cooldown over)"""

    def setUp(self):
        self.h = Harness(plate="BAD0000", valid=False)
        self.h.fsm.breaker_sensor_broken()

    def test_buzzer_callback_fires_exactly_once(self):
        self.assertEqual(self.h.count("on_buzzer_reached"), 1)

    def test_buzzer_callback_sees_buzzer_state(self):
        self.assertEqual(self.h.states["on_buzzer_reached"], State.BUZZER)

    def test_gate_does_not_open(self):
        self.assertEqual(self.h.count("on_gate_opened"), 0)

    def test_returns_to_idle_once_buzzer_callback_returns(self):
        self.assertEqual(self.h.fsm.get_state(), State.IDLE)

    def test_buzzer_fires_after_verification(self):
        self.assertLess(self.h.calls.index("is_verified_plate"),
                        self.h.calls.index("on_buzzer_reached"))

    def test_events_during_cooldown_are_ignored(self):
        # "3 second cooldown" self-loop: the cooldown runs inside
        # on_buzzer_reached, so a sensor break / gate close during it must
        # not start a new cycle.
        h = Harness(valid=False)
        seen = {}

        def poke():
            h.fsm.breaker_sensor_broken()
            h.fsm.close_gate()
            seen["state"] = h.fsm.get_state()

        h.hooks["on_buzzer_reached"] = poke
        h.fsm.breaker_sensor_broken()
        self.assertEqual(seen["state"], State.BUZZER)
        self.assertEqual(h.count("pipeline_take_picture"), 1)
        self.assertEqual(h.count("on_buzzer_reached"), 1)
        self.assertEqual(h.fsm.get_state(), State.IDLE)

    def test_close_gate_after_buzzer_is_ignored(self):
        before = list(self.h.calls)
        self.h.fsm.close_gate()
        self.assertEqual(self.h.fsm.get_state(), State.IDLE)
        self.assertEqual(self.h.calls, before)

    def test_new_car_is_handled_after_buzzer(self):
        self.h.valid = True
        self.h.fsm.breaker_sensor_broken()
        self.assertEqual(self.h.count("pipeline_take_picture"), 2)
        self.assertEqual(self.h.fsm.get_state(), State.OPEN_GATE)


# ---------------------------------------------------------------------
# 4. Whole paths through the diagram
# ---------------------------------------------------------------------

class TestFullPaths(unittest.TestCase):

    def test_valid_plate_full_path(self):
        h = Harness(valid=True)
        h.fsm.breaker_sensor_broken()
        h.fsm.close_gate()
        self.assertEqual(h.calls, [
            "initialize_pipeline",
            "pipeline_take_picture",
            "is_verified_plate",
            "on_gate_opened",
        ])
        self.assertEqual(h.fsm.get_state(), State.IDLE)

    def test_invalid_plate_full_path(self):
        h = Harness(valid=False)
        h.fsm.breaker_sensor_broken()
        self.assertEqual(h.calls, [
            "initialize_pipeline",
            "pipeline_take_picture",
            "is_verified_plate",
            "on_buzzer_reached",
        ])
        self.assertEqual(h.fsm.get_state(), State.IDLE)

    def test_valid_invalid_valid_sequence(self):
        h = Harness(valid=True)
        h.fsm.breaker_sensor_broken()
        h.fsm.close_gate()

        h.valid = False
        h.fsm.breaker_sensor_broken()

        h.valid = True
        h.fsm.breaker_sensor_broken()
        h.fsm.close_gate()

        self.assertEqual(h.count("initialize_pipeline"), 1)
        self.assertEqual(h.count("pipeline_take_picture"), 3)
        self.assertEqual(h.count("is_verified_plate"), 3)
        self.assertEqual(h.count("on_gate_opened"), 2)
        self.assertEqual(h.count("on_buzzer_reached"), 1)
        self.assertEqual(h.fsm.get_state(), State.IDLE)

    def test_each_car_gets_its_own_plate_lookup(self):
        h = Harness(plate="AAA111", valid=True)
        h.fsm.breaker_sensor_broken()
        h.fsm.close_gate()
        h.plate = "BBB222"
        h.fsm.breaker_sensor_broken()
        self.assertEqual(h.plate_seen, "BBB222")


# ---------------------------------------------------------------------
# 5. get_state
# ---------------------------------------------------------------------

class TestGetState(unittest.TestCase):

    def test_returns_a_state_enum_member(self):
        self.assertIsInstance(Harness().fsm.get_state(), State)

    def test_state_enum_has_the_six_diagram_states(self):
        self.assertEqual(
            {s.name for s in State},
            {"INIT", "IDLE", "TAKE_PICTURE", "VERIFY_PLATE",
             "BUZZER", "OPEN_GATE"},
        )

    def test_get_state_has_no_side_effects(self):
        h = Harness()
        for _ in range(3):
            h.fsm.get_state()
        self.assertEqual(h.calls, ["initialize_pipeline"])


if __name__ == "__main__":
    unittest.main()
