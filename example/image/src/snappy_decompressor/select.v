/****************************
Module name: 	select
Author:			Jianyu Chen
Email:			chenjy0046@gmail.com
School:			Delft University of Technology
Date:			13th, July 2018
Function:		generate a NUM_SEL-to-1 MUX with output bandwidth with NUM_WIDTH
****************************/
`timescale 1ns/1ps

module select
#(
	parameter NUM_SEL=16, 
	NUM_LOG=4, 
	NUM_WIDTH=64
)
(
    input[NUM_WIDTH*NUM_SEL-1:0] data_in,
    input[NUM_LOG-1:0] sel,
    output[NUM_WIDTH-1:0] data_out
);


reg[NUM_WIDTH-1:0] data_array[NUM_SEL-1:0];
generate
genvar i;
for(i=0;i<NUM_SEL;i=i+1)begin:gearay
    always@(*)begin
        data_array[i]<=data_in[NUM_WIDTH*i+NUM_WIDTH-1:NUM_WIDTH*i];
    end
end
endgenerate
assign data_out= data_array[sel];
endmodule 
