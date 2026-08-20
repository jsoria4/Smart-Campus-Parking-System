module Emergency_Detect(

input  wire clk,
input  wire resetN,              // Active-low reset
 
// Sensor inputs
input  wire flame_sens,      	 // Triggers when fire threshold is reached
input  wire smoke_sens,      	 // Triggers when smoke threshold is reached
input  wire m_reset,       	 // Manual reset button

// Outputs
output reg  buzzer,             // Alarm buzzer
output reg  flash_leds,         // Flashing LEDs
output reg  force_gates_cw,     // Force all gates open (cw)
//output reg  override_all,  	// Override signal to all other FSMs
output reg  lcd_evacuate,       // Send EVACUATE to LCD
output reg  polling_active,	//clock signal for time increment of checking
output wire [2:0] current_state
);

localparam [2:0]
        MONITOR_ENV         = 3'd0,
        EMERGENCY_DETECTED  = 3'd1,
        ALARM_ACTIVE        = 3'd2,
        EVACUATION_OVERRIDE = 3'd3,
        DISPLAY_EVAC        = 3'd4;

reg [2:0] state, next_state;

assign current_state = state;

wire detect = flame_sens | smoke_sens;

wire emergency_cleared  = (~flame_sens) & (~smoke_sens) & m_reset;



//State register (constant environmental check)

always @(posedge clk or negedge resetN) begin
        if (!resetN)
            state <= MONITOR_ENV;
        else
            state <= next_state;
end

//Next-State Logic (Found thru FSM)

always @(*) begin
        next_state = state; 	// this is the default case
 
        case (state) 		//all next cases shown in FSM and their next states
 
            MONITOR_ENV:
                if (detect)
                    next_state = EMERGENCY_DETECTED;
                // if theres no detection, it continues to monitor
 
            EMERGENCY_DETECTED:

                // There are two parallel paths branching from this node that act simultaneously
                // one goes to ALARM_ACTIVE and then DISPLAY_EVAC
                // the other goes to EVACUATION_OVERRIDE then to DISPLAY_EVAC

                next_state = ALARM_ACTIVE;
 
            ALARM_ACTIVE:
                next_state = EVACUATION_OVERRIDE;
 
            EVACUATION_OVERRIDE:
                next_state = DISPLAY_EVAC;
 
            DISPLAY_EVAC:
                if (emergency_cleared)
                    next_state = MONITOR_ENV;
                // else stay (self-loop ? keep sending EVACUATE to LCD)	
 
            default: next_state = MONITOR_ENV;
 
        endcase
end

always @(*) begin
        // Default all outputs off
        buzzer           = 1'b0;
        flash_leds       = 1'b0;
        force_gates_cw   = 1'b0;
//        override_all 	 = 1'b0;
        lcd_evacuate     = 1'b0;
        polling_active   = 1'b0;
 
        case (current_state)
 
            MONITOR_ENV:
                polling_active = 1'b1;  // Self-loop: polling flame & smoke
 
            EMERGENCY_DETECTED: begin
                // Transition state ? outputs activate in subsequent states
            end
 
            ALARM_ACTIVE: begin
                buzzer     = 1'b1;
                flash_leds = 1'b1;
            end
 
            EVACUATION_OVERRIDE: begin
                buzzer            = 1'b1;   // Keep alarm on
                flash_leds        = 1'b1;
                force_gates_cw    = 1'b1;   // Force all gates open
//                override_all 		 = 1'b1;   // Override all other FSMs
            end
 
            DISPLAY_EVAC: begin
                buzzer            = 1'b1;   // Keep alarm on
                flash_leds        = 1'b1;
                force_gates_cw    = 1'b1;   // Keep gates open
//                override_all 		 = 1'b1;
                lcd_evacuate      = 1'b1;   // Self-loop: send EVACUATE to LCD
            end
 
        endcase
    end
endmodule