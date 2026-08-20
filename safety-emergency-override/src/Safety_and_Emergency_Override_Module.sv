module Safety_and_Emergency_Override_Module(

input  wire CLOCK_50,
 
// buttons
input  wire [3:0]  KEY,
 
// switches
input  wire [9:0]  SW,
 
// HEX displays (active-low)
output reg  [6:0]  HEX0,
output wire [6:0]  HEX1,
output wire [6:0]  HEX2,
output wire [6:0]  HEX3,
output wire [6:0]  HEX4,
output wire [6:0]  HEX5, 
 
// Red LEDs
output wire [9:0]  LEDR,
 
// GPIO header
inout  wire [35:0] GPIO_0
);

// FSM Wires (Internal)
wire buzzer_sig;
wire flash_leds_sig;
wire force_gates_cw_sig;
//wire override_all_fsms_sig;
wire lcd_evacuate_sig;
wire polling_active_sig;
wire [2:0] current_state;


localparam [2:0]
    MONITOR_ENV         = 3'd0,
    EMERGENCY_DETECTED  = 3'd1,
    ALARM_ACTIVE        = 3'd2,
    EVACUATION_OVERRIDE = 3'd3,
    DISPLAY_EVAC        = 3'd4;

reg [24:0] blink_ctr;

reg [25:0] slow_ctr;
reg        slow_clk;

/* 
always @(posedge CLOCK_50 or negedge KEY[0]) begin
    if (!KEY[0]) begin
        slow_ctr <= 0;
        slow_clk <= 0;
    end else if (slow_ctr == 26'd24_999_999) begin
        slow_ctr <= 0;
        slow_clk <= ~slow_clk;
    end else begin
        slow_ctr <= slow_ctr + 1;
    end
end
*/
always @(posedge CLOCK_50 or negedge KEY[0])
    if (!KEY[0]) blink_ctr <= 0;
    else         blink_ctr <= blink_ctr + 1'b1;

// MSB toggles at 50MHz / 2^25 ≈ 1.5 Hz
assign LEDR[1] = flash_leds_sig & blink_ctr[24];


//Instantiation of FSM program
Emergency_Detect unit0 (
.clk				(CLOCK_50),
//.clk            (slow_clk),        // slow clock (testing purposes)
.resetN         (KEY[0]),
.flame_sens     (SW[0]),
.smoke_sens     (SW[1]),
.m_reset        (~KEY[1]),
.buzzer         (buzzer_sig),
.flash_leds     (flash_leds_sig),
.force_gates_cw (force_gates_cw_sig),
//.override_all   (override_all_fsms_sig),
.lcd_evacuate   (lcd_evacuate_sig),
.polling_active (polling_active_sig),
.current_state  (current_state)      // needed for HEX0 display
);


//LED Outputs
assign LEDR[0] = buzzer_sig;
//assign LEDR[1] = flash_leds_sig & blink_ctr[24];   //flashing
assign LEDR[2] = force_gates_cw_sig;
//assign LEDR[3] = override_all_fsms_sig;
assign LEDR[4] = lcd_evacuate_sig;
assign LEDR[8:5] = 4'b0;
assign LEDR[9] = polling_active_sig;

// HEX0 shows current state number (active-low segments)
// 0=MONITOR, 1=EMERGENCY_DETECTED, 2=ALARM, 3=EVAC_OVERRIDE, 4=DISPLAY_EVAC
always @(*) begin
    case (current_state)
        MONITOR_ENV:         HEX0 = 7'b1000000; // 0
        EMERGENCY_DETECTED:  HEX0 = 7'b1111001; // 1
        ALARM_ACTIVE:        HEX0 = 7'b0100100; // 2
        EVACUATION_OVERRIDE: HEX0 = 7'b0110000; // 3
        DISPLAY_EVAC:        HEX0 = 7'b0011001; // 4
        default:             HEX0 = 7'b1111111; // blank
    endcase
end
 
/*
assign HEX0 = 7'b1000000;
assign HEX1 = 7'b1000000;
assign HEX2 = 7'b1000000;
assign HEX3 = 7'b1000000;
assign HEX4 = 7'b1000000;
assign HEX5 = 7'b1000000;
// Blank unused HEX displays
*/

assign HEX1 = 7'h7F;
assign HEX2 = 7'h7F;
assign HEX3 = 7'h7F;
assign HEX4 = 7'h7F;
assign HEX5 = 7'h7F;

assign GPIO_0[0]  = buzzer_sig;            // External buzzer
assign GPIO_0[1]  = force_gates_cw_sig;    // Gate driver enable
//assign GPIO_0[2]  = override_all_fsms_sig; // Override bus
assign GPIO_0[3]  = lcd_evacuate_sig;      // LCD trigger
assign GPIO_0[4] = 1'b0;                      // Ground pin
assign GPIO_0[35:4] = 32'bz;               // Unused pins = high-Z

endmodule