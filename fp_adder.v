`timescale 1ns / 1ps
//////////////////////////////////////////////////////////////////////////////////
// Company: 
// Engineer: 
// 
// Create Date: 2017/05/17 22:53:16
// Design Name: 
// Module Name: fft
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


module Fp_adder #
(
        parameter integer SIZE = 10,
        parameter integer DATA_WIDTH = 32
)

(
       // Clock and Reset shared with the AXI-Lite Slave Port
     input wire  s00_axi_aclk,
     input wire  s00_axi_aresetn,
     
     // AXI-Stream Slave
     output wire  s00_axis_tready,
     input wire [DATA_WIDTH-1 : 0] s00_axis_tdata,
     input wire  s00_axis_tlast,
     input wire  s00_axis_tvalid,
     
     // AXI-Stream Master
     output wire  m00_axis_tvalid,
     output wire [DATA_WIDTH-1 : 0] m00_axis_tdata,
     output wire [(DATA_WIDTH/8)-1 : 0] m00_axis_tstrb,
     output wire  m00_axis_tlast,
     input wire  m00_axis_tready,
     
     // Start signals coming from the AXI-Lite Slave Port
     input wire start
    );
    
    reg [SIZE-1 : 0] addr_stream_in;    // address used to fill in array(mem_A)
    
    reg [SIZE-1 : 0] addr_A;   // address used to read  from Array(mem_A)
    
        
    reg busy;              // Accelerator is busy computing   
    reg [DATA_WIDTH-1 : 0] mem_A [0 : SIZE-1]; // BRAM for Array
    reg item_done;
    reg result_done;         
    reg start_transfer;
    reg transfer;          // accelerator is transferring the results back

    reg ready;
    reg compare_exp; //compare the exponents and shift the mantissa
    reg addition;   // add two mantissa
    reg normalizing; 
    reg rounding;
    reg round_normalizing;
    
    // multiply-and-addition
    
    reg [DATA_WIDTH-1 : 0] mac;     // the previous mad value (i.e. accumulator)
        
    // BRAM Output Registers 
    reg [DATA_WIDTH-1 : 0] operand_A;   
    reg [DATA_WIDTH-1 : 0] operand_B;      
    
    wire [DATA_WIDTH-1 : 0] Exp_AB_sub;
    wire [5:0] shift;
        
    reg [26:0] A_Mant; // (1 + Mantissa(23)) + Guard,Round, Sticky bit(3)
    reg [26:0] B_Mant; 
    reg [26:0] R_Mant;
    reg [7:0] R_exp;  
    
    reg [7:0] result_exp;
    reg [22:0] result_Mant;    
    reg result_sign;
    
    reg add_overflow;
    reg round_overflow;
    
    assign s00_axis_tready = !busy  && !transfer;
    
    always @ (posedge s00_axi_aclk) begin
        if(!s00_axi_aresetn || busy)
            addr_stream_in <=0;
        else if(!busy && !transfer && s00_axis_tvalid) begin
            addr_stream_in <= addr_stream_in + 1;
        end
    end
    
    always @ (posedge s00_axi_aclk) begin
        if (!busy && !transfer && s00_axis_tvalid) // receive data 
            mem_A [addr_stream_in] <= s00_axis_tdata;
        
        if(start) begin
            operand_A <= mem_A [addr_A-1];
            operand_B <= mem_A[addr_A];
        end
        else if(ready) begin
            operand_A <= mem_A[addr_A];
            operand_B <= mac;
        end
        
    end    
    
    // control signals
    always @ (posedge s00_axi_aclk) begin
        if (!s00_axi_aresetn || result_done) 
            busy <= 0;
        else if (start) 
            busy <= 1'b1;
    end
    always @ (posedge s00_axi_aclk) begin
        if (!s00_axi_aresetn || round_normalizing) 
            compare_exp <= 0;
        else if (busy && !result_done && !item_done) 
            compare_exp <= 1'b1;
    end
    always @ (posedge s00_axi_aclk) begin
        if (!s00_axi_aresetn || round_normalizing) 
            addition <= 0;
        else if (compare_exp) 
            addition <= 1'b1;
    end
    always @ (posedge s00_axi_aclk) begin
        if (!s00_axi_aresetn || round_normalizing)
            normalizing <= 0;
        else if (addition) 
            normalizing <= 1'b1;
    end
    always @ (posedge s00_axi_aclk) begin
        if (!s00_axi_aresetn || round_normalizing)
            rounding <= 0;
        else if (normalizing) 
            rounding <= 1'b1;
    end
    always @ (posedge s00_axi_aclk) begin
        if (!s00_axi_aresetn || round_normalizing)
            round_normalizing <= 0;
        else if (rounding) 
            round_normalizing <= 1'b1;
    end
    always @ (posedge s00_axi_aclk) begin
        if (!s00_axi_aresetn || item_done)
            item_done <= 0;
        else if (round_normalizing) 
            item_done <= 1'b1;
    end
    always @ (posedge s00_axi_aclk) begin
        if(!s00_axi_aresetn || ready)  
            ready <= 0;
        else if (item_done)
            ready <= 1;
    end
    
    // get Exp_A - Exp_B
    assign Exp_AB_sub = operand_A[30:23] - operand_B[30:23];
    
    always @ (posedge s00_axi_aclk) begin   
        //-------------------------------------------------------------------------------- compare exponent and shifting
         if(compare_exp && !addition) begin
            if (!Exp_AB_sub[DATA_WIDTH-1] && (Exp_AB_sub != 0)) begin // if Exp_A - Exp_B > 0
                if(operand_A[31:0] == 32'b00000000000000000000000000000000)
                    A_Mant[26:0] <= {1'b0, operand_A [22:0], 3'b000}; //(1 + Mantissa(23)) + Guard,Round, Sticky bit(3)
                else
                    A_Mant[26:0] <= {1'b1, operand_A [22:0], 3'b000}; //(1 + Mantissa(23)) + Guard,Round, Sticky bit(3)
                if(operand_B[31:0] == 32'b00000000000000000000000000000000)
                    B_Mant[26:0] <= {1'b0, operand_B [22:0], 3'b000} ; // shift B
                else
                    B_Mant[26:0] <= ({1'b1, operand_B [22:0], 3'b000} >> Exp_AB_sub); // shift B 
                R_exp <= operand_A [30:23];
             end
            else if (Exp_AB_sub[DATA_WIDTH-1] && (Exp_AB_sub != 0)) begin // else if Exp_A - Exp_B < 0
                if(operand_A[31:0] == 32'b00000000000000000000000000000000)
                    A_Mant[26:0] <= {1'b0, operand_A[22:0], 3'b000} ; // shift A
                else
                    A_Mant[26:0] <= ({1'b1, operand_A[22:0], 3'b000} >> (~Exp_AB_sub+1)); // shift A
                    
                if(operand_B[31:0] == 32'b00000000000000000000000000000000)
                    B_Mant[26:0] <= {1'b0, operand_B [22:0], 3'b000};
                else
                    B_Mant[26:0] <= {1'b1, operand_B [22:0], 3'b000};
                R_exp <= operand_B [30:23];
                
            end
            else if (Exp_AB_sub == 0) begin
                if(operand_A[31:0] == 32'b00000000000000000000000000000000)
                    A_Mant[26:0] <= {1'b0, operand_A [22:0], 3'b000};
                else
                    A_Mant[26:0] <= {1'b1, operand_A [22:0], 3'b000};
                if(operand_B[31:0] == 32'b00000000000000000000000000000000)
                    B_Mant[26:0] <= {1'b0, operand_B [22:0], 3'b000};
                else
                    B_Mant[26:0] <= {1'b1, operand_B [22:0], 3'b000};
                R_exp <= operand_A [30:23];
            end    
        end

        //---------------------------------------------------------------------------------------------- addition 
        if (addition && !normalizing) begin // addition of mantissa
            if(!operand_A[31] && !operand_B[31]) begin
                {add_overflow , R_Mant} <= A_Mant + B_Mant;
                result_sign <= 0;    
            end
            else if(!operand_A[31] && operand_B[31]) begin
                if(A_Mant >= B_Mant) begin  // comparator consider values as unsigned values
                    R_Mant <= A_Mant - B_Mant;  // substraction consifer values as signed values
                    result_sign <= 0;
                end
                else begin
                    R_Mant <= B_Mant - A_Mant;
                    result_sign <= 1;
                end
                add_overflow <= 0; 
            end               
            else if(operand_A[31] && !operand_B[31]) begin
                if(A_Mant >= B_Mant) begin
                    R_Mant <= A_Mant - B_Mant;
                    result_sign <= 1;
                end  
                else begin
                    R_Mant <= B_Mant - A_Mant;
                    result_sign <= 0;
                end
                  
                add_overflow <= 0;
            end
            else if (operand_A[31] && operand_B[31]) begin
                result_sign <= 1;
                {add_overflow,R_Mant} <= A_Mant + B_Mant;
            end
            
        end
        //------------------------------------------------------------------------------------------- normalizing
        if(normalizing && !rounding) begin  
            
            if(add_overflow) begin  
                R_Mant <= R_Mant >> 1;
                R_exp <= R_exp + 1;
                
            end
            else if (!add_overflow && !R_Mant[26]) begin
                
                if(R_Mant[25]) begin
                    R_exp <= R_exp - 1;
                    R_Mant <= R_Mant << 1;
                end
                else if(R_Mant[24]) begin
                    R_exp <= R_exp - 2;
                    R_Mant <= R_Mant << 2;
                end                
                else if(R_Mant[23]) begin
                    R_exp <= R_exp - 3;
                    R_Mant <= R_Mant << 3;
                end
                else if(R_Mant[22]) begin
                    R_exp <= R_exp - 4;
                    R_Mant <= R_Mant << 4;
                end
                else if(R_Mant[21]) begin
                    R_exp <= R_exp - 5;
                    R_Mant <= R_Mant << 5;
                end
                else if(R_Mant[20]) begin
                    R_exp <= R_exp - 6;
                    R_Mant <= R_Mant << 6;
                end
                else if(R_Mant[19]) begin
                    R_exp <= R_exp - 7;
                    R_Mant <= R_Mant << 7;
                end
                else if(R_Mant[18]) begin
                    R_exp <= R_exp - 8;
                    R_Mant <= R_Mant << 8;
                end                
                else if(R_Mant[17]) begin
                    R_exp <= R_exp - 9;
                    R_Mant <= R_Mant << 9;
                end
                else if(R_Mant[16]) begin
                    R_exp <= R_exp - 10;
                    R_Mant <= R_Mant << 10;
                end
                else if(R_Mant[15]) begin
                    R_exp <= R_exp - 11;
                    R_Mant <= R_Mant << 11;
                end
                else if(R_Mant[14]) begin
                    R_exp <= R_exp - 12;
                    R_Mant <= R_Mant << 12;
                end
                else if(R_Mant[13]) begin
                    R_exp <= R_exp - 13;
                    R_Mant <= R_Mant << 13;
                end
                else if(R_Mant[12]) begin
                    R_exp <= R_exp - 14;
                    R_Mant <= R_Mant << 14;
                end                
                else if(R_Mant[11]) begin
                    R_exp <= R_exp - 15;
                    R_Mant <= R_Mant << 15;
                end
                else if(R_Mant[10]) begin
                    R_exp <= R_exp - 16;
                    R_Mant <= R_Mant << 16;
                end
                else if(R_Mant[9]) begin
                    R_exp <= R_exp - 17;
                    R_Mant <= R_Mant << 17;
                end
                else if(R_Mant[8]) begin
                    R_exp <= R_exp - 18;
                    R_Mant <= R_Mant << 18;
                end
                else if(R_Mant[7]) begin
                    R_exp <= R_exp - 19;
                    R_Mant <= R_Mant << 19;
                end
                else if(R_Mant[6]) begin
                    R_exp <= R_exp - 20;
                    R_Mant <= R_Mant << 20;
                end                
                else if(R_Mant[5]) begin
                    R_exp <= R_exp - 21;
                    R_Mant <= R_Mant << 21;
                end
                else if(R_Mant[4]) begin
                    R_exp <= R_exp - 22;
                    R_Mant <= R_Mant << 22;
                end
                else if(R_Mant[3]) begin
                    R_exp <= R_exp - 23;
                    R_Mant <= R_Mant << 23;
                end
                else if(R_Mant[2]) begin
                    R_exp <= R_exp - 24;
                    R_Mant <= R_Mant << 24;
                end
                else if(R_Mant[1]) begin
                    R_exp <= R_exp - 25;
                    R_Mant <= R_Mant << 25;
                end
                else if(R_Mant[0]) begin
                    R_exp <= R_exp - 26;
                    R_Mant <= R_Mant << 26;
                end
                else begin
                    R_exp <= 8'b0;
                    R_Mant <= 27'b0;
                end
                                                                                                                    
            end
        end
        //----------------------------------------------------------------------------- round to nearest even
        if(rounding && !round_normalizing) begin    
            if(R_Mant[1]&& R_Mant[0]) begin // > 0.5 -> round up
                {round_overflow,result_Mant} <= R_Mant[25:3] + 1;
            end
            else if(R_Mant[2] && R_Mant[1] && !R_Mant[0]) begin // =0.5 -> round to even
                if(R_Mant[3]) 
                    {round_overflow,result_Mant} <= R_Mant[25:3] + 1;
                
                else begin
                    result_Mant <= R_Mant[25:3];
                    round_overflow <= 0;
                end
            end
            else begin
                result_Mant <= R_Mant[25:3];
                round_overflow <=0;
            end
        end
        //------------------------------------------------------------------------------- considering overflow during rounding
        if(round_normalizing && !item_done) begin
            if(round_overflow) begin
                result_Mant <= result_Mant >>1;
                result_exp <= R_exp + 1;
            end
            else
                result_exp <= R_exp;
            
        end
    end

    always @ (posedge s00_axi_aclk) begin
        if(!s00_axi_aresetn || result_done)
            addr_A=1;
        else if(round_normalizing && (addr_A != SIZE-1)) 
            addr_A <= addr_A + 1;
    end
    
    always @ (posedge s00_axi_aclk) begin
        if (!s00_axi_aresetn || transfer) 
            result_done <= 0;
        else if (addr_A ==SIZE-1 && item_done) 
            result_done <= 1'b1;
    end
            
    always @ (posedge s00_axi_aclk) begin
        if(!s00_axi_aresetn || transfer)
            mac <= 0;
        else if(item_done) 
            mac <= {result_sign,result_exp,result_Mant};
    end
    
    always @ (posedge s00_axi_aclk) begin
        if (!s00_axi_aresetn || transfer) 
            start_transfer <= 0; 
        else if (result_done)
            start_transfer <= 1;
    end
    
    
    always @ (posedge s00_axi_aclk) begin
        if (!s00_axi_aresetn || transfer) 
            transfer <= 0; 
        else if (start_transfer && m00_axis_tready)
            transfer <= 1;
    end
    
        
    assign m00_axis_tvalid = transfer;
    assign m00_axis_tdata = mac;
    assign m00_axis_tlast = transfer && m00_axis_tready;
    
endmodule
