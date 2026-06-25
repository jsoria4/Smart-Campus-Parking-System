# Interface Contract Template

# FSM #_ — Safety_and_Emergency_Override_Module · Interface Contract
**Owner:** Connor Wickens · **Hardware:** FPGA (ESP32 / FPGA / RPi / HPS) · **Talks to:** fsm-0-entrance.md, fsm-1-counter.md*, fsm-2-exit.md, fsm-4-lighting.md*, fsm-5-congestion.md*

**One-line purpose:** This module actively polls for an emergency, and if found triggers an ermergency override that opens the entrance and exit gates.

## Inputs
*(every signal this module reads)*

|  Signal  |    Source    | Width | Level | Active |      Timing / notes     |
|----------|--------------|-------|-------|--------|-------------------------|
|flame_sens| flame sensor |   1   |  3.3V |  High  |polls for flame emergency|
|smoke_sens|smoke detector|   1   |  3.3V |  High  |polls for smoke emergency|

## Outputs
*(every signal this module drives, and what the receiver should do with it)*

|     Signal     |   Destination   | Width | Level | Active |      Meaning       |
|----------------|-----------------|-------|-------|--------|--------------------|
|    Override    |  Entrance Gate  |   1   | 3.3V  |  High  |   Force Gate Open  |
|    Override    |    Exit Gate    |   1   | 3.3V  |  High  |   Force Gate Open  |
|    Override    | Counter module  |   1   | 3.3V  |  High  |    Clear count(?)  |
|    Override    |Congestion Module|   1   | 3.3V  |  High  |         (?)        |
|     buzzer     |     buzzer      |   1   | 3.3V  |  High  |  sounds the buzzer |
| flash_leds_sig |       LEDs      |   1   | 3.3V  |  High  |    flashes LEDs    |
|lcd_evacuate_sig|    Exit Gate    |   1   | 3.3V  |  High  |LCD evacuate message|

## States exposed
EMERGENCY_ACTIVE

## Override behavior
When the emergency override signal is high, sound buzzer, flash LEDs, and send an evacuation message to the LED display

On `emergency` high:

## Timing & assumptions
*(pulse widths, debounce windows, what "cleared" means for your sensors, any sequencing the other side must respect)*

Override signal is held at high until manually deactivated. When cleared, the system goes back to polling and the buzzers, LEDs, and LCD display turn off

## Open questions

Discuss the use of the override signal for the counter module (Jasmine/Group)
Discuss the use of the override signal for the lighting module(Group)
Discuss the use of the override signal for the congestion module(Landon)

## fsm-3-safety.md — **Connor (FPGA)** ⚠️ HIGHEST PRIORITY
**Talks to:** ALL modules — broadcasts `emergency` to every gate FSM and overrides their states.
**States:** `MONITOR_ENVIRONMENT → EMERGENCY_DETECTED → ALARM_ACTIVE → EVACUATION_OVERRIDE → DISPLAY_EVAC`
**Peripherals to account for:** flame sensor, smoke detector, buzzer, flashing LEDs, LCD ("EVACUATE THE PARKING!")
**Key output:** `emergency` (broadcast) — forces all gate motors CW/open. This is the signal every other contract's "Override behavior" section refers to. Define its width, level, active state, and latching/reset behavior carefully.
