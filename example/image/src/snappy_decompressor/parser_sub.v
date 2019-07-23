/****************************
Module name: 	parser_lit
Author:			Jianyu Chen
Email:			chenjy0046@gmail.com
School:			Delft University of Technology
Date:			14th July, 2018
Function:		The module to parse the literal token. Get the tokens from 2nd level parser (the parser module), parse them
				into write BRAM-based command. And send the command back to parser
****************************/
`timescale 1ns/1ps

module parser_lit#(
	parameter PARSER_NUM=0
)
(
	input clk,
	input rst_n,
	input[127:0] data,
//	input start_lit,
	input[3:0] length, //length of the token  minus 1
	input[15:0] address_in,   /////////[2:0]shift within bram, [6:3]:bram selection [15:7]: bram address 
	input valid_in,
	
	output[63:0] data0,
	output[63:0] data1,
	output[63:0] data2,
	output[63:0] data3,
	output[8:0] address0,
	output[8:0] address1,
	output[8:0] address2,
	output[8:0] address3,
	output[8:0] wr_out0,  ///if [7:0]==0 & valid, [8]=0, whether bytes are valid  
	output[8:0] wr_out1,
	output[8:0] wr_out2,
	output[8:0] wr_out3,
	output[3:0] ram_select_out0,
	output[3:0] ram_select_out1,
	output[3:0] ram_select_out2,
	output[3:0] ram_select_out3,
	output valid_out
);


reg[183:0] data_1;  ///128bit + 56bit for shift
reg[15:0] wr_1;
reg[15:0] address_1;
reg valid_1;
reg[12:0] address_1_0,address_1_1,address_1_2,address_1_3;
wire[183:0] data_1_w;
wire[15:0] wr_1_w;
wire[15:3] address_1_0_w,address_1_1_w,address_1_2_w,address_1_3_w;
//assign data_1_w={data[7:0],data[15:8],data[23:16],data[31:24],data[39:32],data[47:40],data[55:48],data[63:56],data[71:64],data[79:72],data[87:80],data[95:88],data[103:96],data[111:104],data[119:112],data[127:120]}<<address_in[2:0];
assign data_1_w={data,56'b0}>>{address_in[2:0],3'b0};
assign wr_1_w=~(16'h7fff>>length);  
assign address_1_0_w=address_in[15:3];
assign address_1_1_w=address_in[15:3]+13'd1;
assign address_1_2_w=address_in[15:3]+13'd2;
assign address_1_3_w=address_in[15:3]+13'd3;
always@(posedge clk)begin
	data_1			<=data_1_w;
	wr_1			<=wr_1_w;
	address_1       <=address_in;
	address_1_0		<=address_1_0_w[15:3];
	address_1_1		<=address_1_1_w[15:3];
	address_1_2		<=address_1_2_w[15:3];
	address_1_3		<=address_1_3_w[15:3];
	
	if(~rst_n)begin
		valid_1		<= 1'b0;
	end else begin
		valid_1		<=valid_in;
	end
	
end

reg[31:0] wr_2;
reg[63:0] data_2_0,data_2_1,data_2_2,data_2_3;
reg[8:0] address_2_0,address_2_1,address_2_2,address_2_3;
reg[3:0] ram_select0,ram_select1,ram_select2,ram_select3;
reg valid_2;
wire[47:0] wr_2_w;
assign wr_2_w={wr_1,32'b0}>>address_1[4:0];
always@(posedge clk)begin
	wr_2[31:16]	<=	(wr_2_w[47:32]|wr_2_w[15:0]);
	wr_2[15:0]	<=	wr_2_w[31:16];		
	
	case(address_1[4:3])
	2'd0:begin		data_2_0<=data_1[183:120];					data_2_1<=data_1[119:56];                   data_2_2<={data_1[55:0],8'b0};          	data_2_3<=64'b0;                   	
					address_2_0<=address_1_0[12:4];				address_2_1<=address_1_1[12:4];				address_2_2<=address_1_2[12:4];				address_2_3<=address_1_3[12:4];	
					ram_select0<=(4'b0001<<address_1_0[3:2]);	ram_select1<=(4'b0001<<address_1_1[3:2]);	ram_select2<=(4'b0001<<address_1_2[3:2]);	ram_select3<=(4'b0001<<address_1_3[3:2]);end

	2'd1:begin		data_2_1<=data_1[183:120];					data_2_2<=data_1[119:56];                   data_2_3<={data_1[55:0],8'b0};          	data_2_0<=64'b0;	
					address_2_1<=address_1_0[12:4];				address_2_2<=address_1_1[12:4];				address_2_3<=address_1_2[12:4];				address_2_0<=address_1_3[12:4];
					ram_select1<=(4'b0001<<address_1_0[3:2]);	ram_select2<=(4'b0001<<address_1_1[3:2]);	ram_select3<=(4'b0001<<address_1_2[3:2]);	ram_select0<=(4'b0001<<address_1_3[3:2]);end

	2'd2:begin		data_2_2<=data_1[183:120];					data_2_3<=data_1[119:56];                   data_2_0<={data_1[55:0],8'b0};          	data_2_1<=64'b0;	
					address_2_2<=address_1_0[12:4];				address_2_3<=address_1_1[12:4];				address_2_0<=address_1_2[12:4];				address_2_1<=address_1_3[12:4];
					ram_select2<=(4'b0001<<address_1_0[3:2]);	ram_select3<=(4'b0001<<address_1_1[3:2]);	ram_select0<=(4'b0001<<address_1_2[3:2]);	ram_select1<=(4'b0001<<address_1_3[3:2]);end

	2'd3:begin		data_2_3<=data_1[183:120];					data_2_0<=data_1[119:56];                   data_2_1<={data_1[55:0],8'b0};          	data_2_2<=64'b0;	
					address_2_3<=address_1_0[12:4];				address_2_0<=address_1_1[12:4];				address_2_1<=address_1_2[12:4];				address_2_2<=address_1_3[12:4];
					ram_select3<=(4'b0001<<address_1_0[3:2]);	ram_select0<=(4'b0001<<address_1_1[3:2]);	ram_select1<=(4'b0001<<address_1_2[3:2]);	ram_select2<=(4'b0001<<address_1_3[3:2]);end
	default:;
	endcase
	
	if(~rst_n)begin
		valid_2	<=	1'b0;
	end else begin
		valid_2	<=	valid_1;
	end
	
	
	
	/************************************************
	for debug only**********************************/
	/*if(data_2_0[63:48]==16'h6564)begin
		$display("data_2_0	%d",PARSER_NUM);
	end
	if(data_2_1[63:48]==16'h6564)begin
		$display("data_2_1	%d",PARSER_NUM);
	end
	if(data_2_2[63:48]==16'h6564)begin
		$display("data_2_2	%d",PARSER_NUM);
	end
	if(data_2_3[63:48]==16'h6564)begin
		$display("data_2_3	%d",PARSER_NUM);
	end*/
end


assign data0=data_2_0;
assign data1=data_2_1;
assign data2=data_2_2;
assign data3=data_2_3;
assign wr_out0[7:0]=wr_2[31:24];
assign wr_out0[8]  =(wr_2[31:24]!=8'b0)	& valid_2;
assign wr_out1[7:0]=wr_2[23:16];
assign wr_out1[8]  =(wr_2[23:16]!=8'b0) & valid_2;
assign wr_out2[7:0]=wr_2[15:8];
assign wr_out2[8]  =(wr_2[15:8]!=8'b0) 	& valid_2;
assign wr_out3[7:0]=wr_2[7:0];
assign wr_out3[8]  =(wr_2[7:0]!=8'b0) 	& valid_2;
assign address0=address_2_0;
assign address1=address_2_1;
assign address2=address_2_2;
assign address3=address_2_3;
assign ram_select_out0=ram_select0;
assign ram_select_out1=ram_select1;
assign ram_select_out2=ram_select2;
assign ram_select_out3=ram_select3;
assign valid_out=valid_2;
endmodule

/****************************
Module name: 	parser_copy
Author:			Jianyu Chen
Email:			chenjy0046@gmail.com
School:			Delft University of Technology
Date:			14th July, 2018
Function:		The module to parse the copy token. Get the tokens from 2nd level parser (the parser module), parse them
				into copy BRAM-based command. And send the command back to parser
****************************/

module parser_copy#(
	parameter PARSER_NUM=0
)
(
	input clk,
	input rst_n,
	input[5:0] length_in,
	input[15:0] address_in,   /////////[2:0]:shift [6:3]index for bram [15:7]: bram address 
	input[15:0] offset_in,
	input valid_in,
	
	output[143:0] address_out, 		//address for 16 rams, each is 9-bits
//	output valid_out,
	output[15:0] ram_select,     ///whether it wants to read from a ram
	output[127:0]  rd_out,       ///choose bytes to read
	output[15:0] offset_out
);

reg[127:0]rd_2;
//reg[15:0] address_2;
reg valid_2;
reg[15:0] offset_2;
reg[15:0] address_rd_2;
always@(posedge clk)begin
		rd_2		<=~(128'h7fff_ffff_ffff_ffff_ffff_ffff_ffff_ffff >> length_in);
//		address_2 	<= address_1;
		address_rd_2<=address_in-offset_in;
		offset_2	<=offset_in;
	if(~rst_n)begin
		valid_2	<= 1'b0;
	end else begin
		valid_2	<= valid_in;
	end
	
	
	///////for debug
//	if(valid_1)begin
//		if(length<=offset)begin
//			$display("address:  %d  %d",address_1,PARSER_NUM);
//			$display("length:  %d",length);
//			$display("offset:  %d",offset);
//		end
//	end
	///////////////////

end

reg[127:0] rd_3;
reg[143:0] address_3; 		//address for 16 rams, each is 9-bits
reg[15:0] offset_3;
reg[15:0] ram_select_3;
wire[15:0] ram_select_3_w;
wire[207:0] address_3_w;
wire[143:0] address_3_w2;
wire[63:0] base;
reg valid_3;
wire[255:0] rd_3_w;
assign rd_3_w={rd_2,rd_2}>>address_rd_2[6:0];

genvar i;
generate
for(i=0;i<16;i=i+1)begin
	assign base[4*i+3:4*i]			=i[3:0]-address_rd_2[6:3];
	assign address_3_w[13*i+12:13*i]=address_rd_2[15:3]+{7'b0,base[4*i+3:4*i]};
	assign address_3_w2[9*i+8:9*i]	=address_3_w[13*i+12:13*i+4];
	assign ram_select_3_w[i]		=rd_3_w[127-8*i] |  rd_3_w[126-8*i] |  rd_3_w[125-8*i] |  rd_3_w[124-8*i] |  rd_3_w[123-8*i] |  rd_3_w[122-8*i] |  rd_3_w[121-8*i] |  rd_3_w[120-8*i]; 
end
endgenerate 

always@(posedge clk)begin
	rd_3		<=rd_3_w[127:0];
	address_3	<=address_3_w2;
	offset_3	<=offset_2;
	ram_select_3<=ram_select_3_w;
	if(~rst_n)begin
		valid_3	<= 1'b0;
	end	else begin
		valid_3	<= valid_2;
	end
	
end

//assign valid_out	=valid_3;
assign rd_out		=rd_3;
assign address_out	=address_3;
assign offset_out	=offset_3;
assign ram_select	=ram_select_3&{16{valid_3}};

endmodule
