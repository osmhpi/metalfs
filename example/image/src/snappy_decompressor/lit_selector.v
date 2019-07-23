///data from read result will always be processed first
`timescale 1ns/1ps

module lit_selector
#(
	parameter NUM_PARSER=6,
	NUM_LOG=3,
	BASE_INIT=6'b1
)
(
	input clk,
	input rst_n,
	input[64*NUM_PARSER-1:0] data_lit,
	input[9*NUM_PARSER-1:0] lit_address,
	input[8*NUM_PARSER-1:0] byte_valid_in,
	input[NUM_PARSER-1:0] lit_valid,  //whether the data of parser is for this selector
	
	input[63:0] data_copy,            //data from read result
	input[8:0] address_copy,         //address from read result
	input[7:0] byte_valid_copy,
	input copy_valid,				 
	
	output[NUM_PARSER-1:0] rd_out,    ///select a parser to read
	output[63:0] data_out,
	output[8:0] address_out,
	output[7:0] byte_valid_out,
	output valid_out
	
);
reg[63:0] data_buff;
reg[8:0] address_buff;
reg[7:0] byte_valid_buff;
reg valid_buff;
reg[NUM_PARSER-1:0] base;
wire[63:0] data_w;
wire[8:0] address_w;
wire[7:0] byte_valid_w;
wire[NUM_PARSER-1:0] grant_w;

always@(posedge clk)begin
	address_buff<=copy_valid?address_copy:address_w;
	data_buff	<=copy_valid?data_copy:data_w;
	byte_valid_buff<=copy_valid?byte_valid_copy:byte_valid_w;
	valid_buff	<=(lit_valid!=0)|copy_valid;    ///if there is one req, valid
end

always@(posedge clk)begin
	if((~rst_n) | (grant_w==0))begin
		base	<= BASE_INIT;
	end
	else if(copy_valid)begin
		base	<= grant_w;
	end 
	else begin
		if(NUM_PARSER == 1)begin //if there is only once parser, no need to shift
			base	<= grant_w;
		end else begin
			base	<= {grant_w[NUM_PARSER-2:0],grant_w[NUM_PARSER-1]}; ///left shift
		end
	end
end

arbiter 
#(
	.WIDTH(NUM_PARSER)
)arbiter0
(
	.req(lit_valid),
	.grant(grant_w),
	.base(base)
);

function [NUM_LOG-1:0] onehot_int;
        input [NUM_PARSER-1:0] in;
        integer i;
        begin
            onehot_int = 0;
            for (i = NUM_PARSER-1; i >= 0; i=i-1) begin
                if (in[i])
                    onehot_int = i;
            end
        end
endfunction

select 
#(
	.NUM_SEL	(NUM_PARSER),
	.NUM_LOG	(NUM_LOG),
	.NUM_WIDTH	(64)
)select_data
(
	.data_in(data_lit),
    .sel(onehot_int(grant_w)),
    .data_out(data_w)
);

select 
#(
	.NUM_SEL	(NUM_PARSER),
	.NUM_LOG	(NUM_LOG),
	.NUM_WIDTH	(9)
)select_address
(
	.data_in(lit_address),
    .sel(onehot_int(grant_w)),
    .data_out(address_w)
);

select 
#(
	.NUM_SEL	(NUM_PARSER),
	.NUM_LOG	(NUM_LOG),
	.NUM_WIDTH	(8)
)select_bytevalid
(
	.data_in(byte_valid_in),
    .sel(onehot_int(grant_w)),
    .data_out(byte_valid_w)
);

assign data_out		=data_buff;
assign address_out	=address_buff;
assign byte_valid_out=byte_valid_buff;
assign valid_out	=valid_buff;
assign rd_out		=copy_valid?0:grant_w;

endmodule


module arbiter
#(
	parameter WIDTH = 6
)
(   
    req, grant, base   
);      
   
input [WIDTH-1:0] req;   
output [WIDTH-1:0] grant;   
input [WIDTH-1:0] base;   
   
wire [2*WIDTH-1:0] double_req = {req,req};   
wire [2*WIDTH-1:0] double_grant = double_req & ~(double_req-base);   
assign grant = double_grant[WIDTH-1:0] | double_grant[2*WIDTH-1:WIDTH];   
   
endmodule
