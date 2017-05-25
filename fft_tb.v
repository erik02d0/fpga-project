`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 04/17/2017 03:33:29 PM
// Design Name: 
// Module Name: mat_mul_tb
// Project Name: 
// Target Devices: 
// Tool Versions: 
// Description: 
// 
// Dependencies: 
// 
// Revision:
// Revision 0.01 - File Created
// Additional Comments:
// 
//////////////////////////////////////////////////////////////////////////////////


module fft_tb # 
    (
        parameter integer ARRAY_SIZE = 2,
        parameter integer DATA_WIDTH = 32,

        parameter integer TEST_CASES = 10

    )
    (

    );
    
    reg  s00_axi_aclk;
    reg  s00_axi_aresetn;

	// Ports of Axi Slave Bus Interface S00_AXIS
    wire  s00_axis_tready;
    reg [DATA_WIDTH-1 : 0] s00_axis_tdata;
    reg [(DATA_WIDTH/8)-1 : 0] s00_axis_tstrb;
    reg  s00_axis_tlast;
    reg  s00_axis_tvalid;

    // Ports of Axi Master Bus Interface M00_AXIS
    wire  m00_axis_tvalid;
    wire [DATA_WIDTH-1 : 0] m00_axis_tdata;
    wire [(DATA_WIDTH/8)-1 : 0] m00_axis_tstrb;
    wire  m00_axis_tlast;
    reg  m00_axis_tready;
    
    
    reg start;
    
    integer row;
    
    integer count;
    integer temp_counter;
    
    integer test_case; // Stores current test case being run
    integer A [0 : ARRAY_SIZE-1];
    integer T_A [0 : TEST_CASES]; // List of test cases used as first  term in the addition
    integer T_B [0 : TEST_CASES]; // List of test cases used as second term in the addition
    integer T_R [0 : TEST_CASES]; // List of expected results for the test cases (T_A[i] + T_B[i] == T_R[i])
    integer R; // Used to store the current test case's expected result for easy comparison in the waveform view
    reg test_result_correct;
    
    integer RSW ;
    integer RHW ;
    
    Fp_adder # (
    .SIZE(ARRAY_SIZE),
    .DATA_WIDTH(DATA_WIDTH)
    ) accelerator (
    .s00_axi_aclk(s00_axi_aclk),
    .s00_axi_aresetn(s00_axi_aresetn),
    .s00_axis_tready(s00_axis_tready),
    .s00_axis_tdata(s00_axis_tdata),
    .s00_axis_tlast(s00_axis_tlast),
    .s00_axis_tvalid(s00_axis_tvalid),
    .m00_axis_tvalid(m00_axis_tvalid),
    .m00_axis_tdata(m00_axis_tdata),
    .m00_axis_tstrb(m00_axis_tstrb),
    .m00_axis_tlast(m00_axis_tlast),
    .m00_axis_tready(m00_axis_tready), 
    .start(start)
    );
    
    always
        #10 s00_axi_aclk = ~s00_axi_aclk;
        
    initial begin
        T_A[0] = 32'd0;
        T_B[0] = 32'd0;
        T_R[0] = 32'd0;
        
        T_A[1] = 32'b0_10000000_10010010000111111011011; // pi
        T_B[1] = 32'b1_10000000_10010010000111111011011; // -pi
        T_R[1] = 32'b0_00000000_00000000000000000000000; // 0.0
        
        T_A[2] = 32'b0_10000101_11101101110100101111001;  //  123.456001
        T_B[2] = 32'b0_10000101_10111100011100011010101;  //  111.111000
        T_R[2] = 32'b0_10000110_11010101001000100100111;  //  234.567001
        
        T_A[3] = 32'b1_10000101_11101101110100101111001;  // -123.456001
        T_B[3] = 32'b0_10000101_10111100011100011010101;  //  111.111000
        T_R[3] = 32'b1_10000010_10001011000010100100000;  // -12.345001
        
        T_A[4] = 32'b0_10000101_11101101110100101111001;  //  123.456001
        T_B[4] = 32'b1_10000101_10111100011100011010101;  // -111.111000
        T_R[4] = 32'b0_10000010_10001011000010100100000;  //  12.345001
        
        T_A[5] = 32'b1_10000101_11101101110100101111001;  // -123.456001
        T_B[5] = 32'b1_10000101_10111100011100011010101;  // -111.111000
        T_R[5] = 32'b1_10000110_11010101001000100100111;  // -234.567001
        
        // This test should be rounded up - truncating the result gives
        // 32'b0_10000101_10001111111111101111100 = 99.998992
        T_A[6] = 32'b0_10000101_10010000000000000000000;  //  100.000000 (42c80000)
        T_B[6] = 32'b1_01110101_00000110001001001101111;  // -0.001000   (ba83126f)
        T_R[6] = 32'b0_10000101_10001111111111101111101;  //  99.999001  (42c7ff7d)
        
        for (test_case = 0; test_case < TEST_CASES; test_case = test_case + 1) begin
            s00_axi_aclk = 1;
            s00_axi_aresetn = 0;
            s00_axis_tdata = 0;
            s00_axis_tstrb = 4'hf;
            s00_axis_tlast = 0;
            s00_axis_tvalid = 0;
            m00_axis_tready = 0;
            start = 0;
            row = 0;
            
            count = 0;
            temp_counter= 0;
            
            A[0] = T_A[test_case];
            A[1] = T_B[test_case];
            R    = T_R[test_case];
            test_result_correct = 1'bx;
                
            #20
            s00_axi_aresetn = 1;
            count = 0;
            
            // send the two matrices using the AXI Stream protocol
            
            #20
            s00_axis_tvalid = 1;
            for (row = 0; row < ARRAY_SIZE; row = row + 1) begin
                    
                s00_axis_tdata = A [row];
                if (row == ARRAY_SIZE-1)
                    s00_axis_tlast = 1;
                #20;
                    // wait until the slave is ready to read the data
                while (!s00_axis_tready) begin
                    #20;
                end  
            end
                
            s00_axis_tlast = 0;
            s00_axis_tvalid = 0;
            
            // start the accelerator
            #20
            start = 1;
            #20
            start = 0;
            
            m00_axis_tready = 1;
            
            while (!m00_axis_tlast) begin
                #20;
                if(m00_axis_tready == 1 && m00_axis_tvalid == 1) begin
                    RHW = m00_axis_tdata;
                end
            end
            m00_axis_tready = 0;
            
            test_result_correct = RHW == R;
            
            # 40;
                   
       end
       $stop;
       
    end
    
endmodule
