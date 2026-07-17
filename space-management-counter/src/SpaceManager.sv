// TODO: testbench, update FSM diagram

//  FSM #1 - Space Management & Counter Module
//  Jasmine Sellers
//  Project: DE10-SoC Smart Campus Parking Lot Management System
//  Functionality:
//      * Moore FSM implemnting FSM #1 to ensure accurate counts during entry/exit events
//      * Sensor_entrance/Sensor_exit are treated as PULSE inputs: a rising-edge
//        detector converts whatever the raw sensor level looks like (a single
//        cycle, or held high for many cycles while a car physically clears the
//        sensor) into one clean, single-cycle pulse. This guarantees exactly
//        one increment/decrement per physical vehicle event, even if the
//        sensor stays asserted longer than the IDLE->...->IDLE round trip.
//  States:
//      * IDLE monitoring for vehicle passage signals
//      * INCREMENT_COUNT trigger when entrance (Sensor_entrance) is cleared
//      * DECREMENT_COUNT trigger when exit (Sensor_exit) is cleared
//      * CHECK_CAPACITY compare available space to parking lot capacity
//      * SPACE_FULL trigger when count reaches limit
//  Port List:
//      * Clk - clock signal
//      * ResetN - reset signal (set to IDLE and reset counter)
//      * Sensor_entrance - pulse signaling entrance of car (0=no car; 1=car entered)
//      * Sensor_exit - pulse signaling exit of car (0=no car; 1=car exited)
//      * Spaces_available - integer representing number of spaces currently available
//      * StateOut - for testing purposes
module SpaceManager(Clk, ResetN, Sensor_entrance, Sensor_exit, Spaces_available, StateOut);
    parameter MAX_SPACE = 10;
    localparam N = $clog2(MAX_SPACE + 1);              // TODO: Calculate n from MAX_SPACE
    input Clk, ResetN, Sensor_entrance, Sensor_exit;
    output [N-1:0] Spaces_available;
    output StateOut;

    // States
    localparam  IDLE            = 3'd0,
                INCREMENT_COUNT = 3'd1,
                DECREMENT_COUNT = 3'd2,
                CHECK_CAPACITY  = 3'd3,
                SPACE_FULL      = 3'd4;

    // Space counter
    logic [N-1:0] counter;

    // State variables
    logic [3:0] State,NextState;

    // Pulse detectors: turn raw (possibly multi-cycle) sensor levels into a
    // single-cycle pulse so the FSM only registers one event per car.
    logic entrance_prev, exit_prev;
    logic entrance_pulse, exit_pulse;

    always_ff @(posedge Clk, negedge ResetN) begin
        if (!ResetN) begin
            entrance_prev <= 1'b0;
            exit_prev     <= 1'b0;
        end
        else begin
            entrance_prev <= Sensor_entrance;
            exit_prev     <= Sensor_exit;
        end
    end

    assign entrance_pulse = Sensor_entrance & ~entrance_prev;
    assign exit_pulse     = Sensor_exit & ~exit_prev;

    assign StateOut = State;
    assign Spaces_available = MAX_SPACE[N-1:0] - counter;

    // Next State Logic
    always_comb begin
        case (State)
            IDLE: begin
                if (entrance_pulse) NextState = INCREMENT_COUNT;
                else if (exit_pulse) NextState = DECREMENT_COUNT;
                else NextState = IDLE;
            end
            INCREMENT_COUNT: begin
                NextState = CHECK_CAPACITY;
            end
            DECREMENT_COUNT: begin
                NextState = CHECK_CAPACITY;
            end
            CHECK_CAPACITY: begin
                if (counter >= MAX_SPACE) begin
                    NextState = SPACE_FULL;
                end
                else begin
                    NextState = IDLE;
                end
            end
            SPACE_FULL: begin
                if (exit_pulse) NextState = DECREMENT_COUNT;
                else NextState = IDLE;
            end
            default: begin
                NextState = IDLE;
            end
        endcase
    end

    // Stage Reg
    always_ff @(posedge Clk, negedge ResetN) begin
        if (!ResetN) begin
            State <= IDLE;
            counter <= 1'b0;
        end
        else begin
            State <= NextState;
            case (State)
                DECREMENT_COUNT: begin
                    if (counter > 1'b0) begin
                        counter <= counter - 1'b1;
                    end                end
                INCREMENT_COUNT: begin
                    if (counter < MAX_SPACE) begin
                        counter <= counter + 1'b1;
                    end
                end
                default: begin
                    counter <= counter;
                end
            endcase
        end
    end
endmodule: SpaceManager

// Testbench for the module above SpaceManager(Clk, ResetN, Sensor_entrance, Sensor_exit, Spaces_available, StateOut);
module SpaceManager_tb();
    integer MAX_SPACE = 10;
    integer N = $clog2(MAX_SPACE + 1);

    logic Clk, ResetN;
    logic Sensor_entrance, Sensor_exit;
    logic [N-1:0] Spaces_available;
    logic StateOut;

    integer i;
    integer rand_event;
    integer expected_spaces;

    // SpaceManager(Clk, ResetN, Sensor_entrance, Sensor_exit, Spaces_available, StateOut);
    SpaceManager(Clk, ResetN, Sensor_entrance, Sensor_exit, Spaces_available, StateOut);

    // Clock
    always begin
        Clk = 0; #10;
        Clk = 1; #10;
    end

    initial begin
        $display("ResetN\tEnt\tExit\tSpaces");
        $monitor("%b\t%b\t%b\t%d",
                 ResetN,
                 Sensor_entrance,
                 Sensor_exit,
                 Spaces_available);

        $display("TEST 1 - Reset");
        Sensor_entrance = 0;
        Sensor_exit     = 0;
        ResetN          = 0;
        #21;
        assert(Spaces_available == MAX_SPACE);
        ResetN = 1;

        $display("TEST 2 - 10 Cars Entering");
        for(i = 1; i <= 10; i++) begin
            Sensor_entrance = 1'b1;
            #20;
            Sensor_entrance = 1'b0;
            #60;
            assert(Spaces_available == (MAX_SPACE - i))
                else $error("Entry %d failed. Expected %d spaces, got %d", i, MAX_SPACE-i, Spaces_available);
        end

        assert(Spaces_available == 0) else $error("Lot should be full");

        $display("TEST 2 - 10 Cars Exiting");

        for(i = 1; i <= 10; i++) begin

            Sensor_exit = 1'b1;
            #20;
            Sensor_exit = 1'b0;

            #60;

            assert(Spaces_available == i)
                else $error("Exit Failed Expected %0d spaces, got %0d", i, Spaces_available);
        end

        assert(Spaces_available == MAX_SPACE)
        else $error("Lot should be empty");

        $display("\nTEST 3 - Random Entering and Exiting");
        expected_spaces = MAX_SPACE;
        for(i = 0; i < 20; i++) begin
            rand_event = $urandom_range(0,1);
            if(rand_event == 0 && expected_spaces > 0) begin
                Sensor_entrance = 1'b1;
                #20;
                Sensor_entrance = 1'b0;
                #60;
                expected_spaces--;
                assert(Spaces_available == expected_spaces)
                    else $error("Random entry failed");
                $display("Random Event %0d: ENTRY -> Spaces=%0d",
                         i, Spaces_available);
            end
            else if(rand_event == 1 && expected_spaces < MAX_SPACE) begin
                Sensor_exit = 1'b1;
                #20;
                Sensor_exit = 1'b0;
                #60;
                expected_spaces++;
                assert(Spaces_available == expected_spaces)
                    else $error("Random exit failed");
                $display("Random Event %0d: EXIT -> Spaces=%0d",
                         i, Spaces_available);
            end
        end

        $display("\nTEST 4 - Sensor held high for multiple cycles (pulse-stretch test)");
        expected_spaces = expected_spaces; // carry over from TEST 3
        Sensor_entrance = 1'b1;
        #100;                       // held high for 5 clock periods, not just 1
        Sensor_entrance = 1'b0;
        #60;
        expected_spaces--;
        assert(Spaces_available == expected_spaces)
            else $error("Stretched entrance pulse caused wrong count. Expected %0d, got %0d",
                        expected_spaces, Spaces_available);
        $display("Stretched ENTRY -> Spaces=%0d (expected %0d)", Spaces_available, expected_spaces);

        $stop;
    end
endmodule : SpaceManager_tb