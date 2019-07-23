/****************************
Module name: 	parser
Author:			Jianyu Chen
Email:			chenjy0046@gmail.com
School:			Delft University of Technology
Date:			28th March, 2019
Function:		This is the 2nd level parser. It will get slice(from the 1st level parser) 
				from distributor and cut it into tokens. The tokens will be sorted into 2
				kinds: literal and copy. These two kinds of token will be sent to the two kinds
				of sub parsers (parser_lit and parser_copy).
****************************/
`timescale 1ns/1ps

module parser#(
	parameter PARSER_NUM=0 //the number lable for parser, for debug only
)
(
	input clk,
	input rst_n,
	
	input[143:0] data,
	input[15:0] tokenpos_in,
	input[16:0] address_in,
	input[2:0] garbage_in,
	input start_lit_in,
	
	input valid_in,
	input block_out_finish,  
	input page_finish,
	
	output block_finish,
	///for literal content 
	input[3:0] lit_rd,
	output[255:0] lit_data_out,
	output[35:0] lit_address_out,
	output[31:0] lit_wr_out,
	output[15:0] lit_ram_select_out,
	
	///output for read token
	input[15:0] copy_rd,
	output[143:0] copy_address_out,  //ram15,ram14 ...ram0
	output[15:0] copy_ram_select_out,
	output[127:0] copy_rd_out,
	output[255:0] copy_offset_out,
	/////
	output all_empty,  //set to high if all the fifo are empty
	output ready  //whether ready to receive a new page
);

reg stop_flag,stop_flag_delay;		///whether the pipeline should stop in the next cycle
wire[15:0] copy_almost_full;
wire[3:0] lit_almost_full;
always@(posedge clk)begin
	if(~rst_n)begin stop_flag	<= 1'b0; end
	else if(copy_almost_full!=16'b0 | lit_almost_full!=4'b0)begin
		stop_flag	<=1'b1;
	end
	else begin  stop_flag	<= 1'b0; end
	
	stop_flag_delay	<= stop_flag;
end

reg[143:0] data_buff;
reg[15:0] tokenpos_buff;
reg[16:0] address_buff;
reg start_lit_buff;
reg[2:0] garbage_buff;
reg[2:0] state;
reg valid_fsm;
reg[4:0] length_left;  		//the length left
reg slice_req;             //set to high if current slice is finished
reg overflow_record;	//if address_in[16] is different from this bit, overflow happens
reg block_finish_r;		
/////to parser_lit
reg[127:0] lit_data;
reg[3:0] lit_length;
reg[15:0] lit_address;
reg lit_valid;
//reg lit_start_lit;


/////to parser_copy
//reg[23:0] copy_data;
reg[5:0] copy_length;
reg[15:0] copy_address;
reg[15:0] copy_offset,copy_offset_record;
reg copy_valid;
reg[15:0] copy_length_record;  //to record the total length of a copy token
//////
wire[3:0] lza_z;
wire lza_a;

reg[16:0] address_buff_w;
always@(*)begin
	if(start_lit_buff==1'b1)begin
		address_buff_w	<=address_buff+lza_z;
	end
	else begin
		casex(data_buff[143:136])
			8'b1111_0000:begin address_buff_w	<=address_buff+17'b1+data_buff[135:128]; end
			8'b1111_0100:begin address_buff_w	<=address_buff+17'b1+{data_buff[127:120],data_buff[135:128]}; end
			8'bxxxx_xx01:begin address_buff_w	<=address_buff+17'd4+data_buff[140:138]; end
			8'bxxxx_xx10:begin address_buff_w	<=address_buff+17'd1+data_buff[143:138]; end
			default:     begin address_buff_w	<=address_buff+17'b1+{10'b0,data_buff[143:138]}; end
		endcase
	end
end

always@(posedge clk)begin
	if(~rst_n)begin overflow_record<=1'b0; end
	else if(block_out_finish)begin overflow_record<=~overflow_record; end
end


reg empty_flag;
wire[15:0] 	offset_2b,offset_3b;  ///offset for token of 2 or 3 bytes
wire[5:0]	length_2b,length_3b;
wire copy_sep_2b,copy_sep_3b;
assign offset_2b={5'b0,data_buff[143:141],data_buff[135:128]};
assign offset_3b={data_buff[127:120],data_buff[135:128]};
assign length_2b={3'b0,data_buff[140:138]}+6'd3;
assign length_3b=data_buff[143:138];
assign copy_sep_2b=(offset_2b<=length_2b) & (offset_2b>2);
assign copy_sep_3b=(offset_3b<=length_3b) & (offset_3b>2);

//the signal below is for debug only, do not synthesize it
reg debug_signal1;
reg overflow_flag;

always@(posedge clk)begin
	
	if(~rst_n)begin
		state <= 3'd0;
		copy_valid		<= 1'b0;
		lit_valid		<= 1'b0;
		debug_signal1	<= 1'b0;
	end else
	case(state)
	3'd0:begin   ///idle state
		state			<= 3'b1;
		slice_req		<= 1'b1;
		block_finish_r	<= 1'b0;
		///set the output to sub parsers to invalid
		lit_valid		<= 1'b0;
		copy_valid		<= 1'b0;
	end
	
	3'd1:begin
		if(valid_in)begin
			data_buff		<= data;
			tokenpos_buff	<= tokenpos_in;
			start_lit_buff	<= start_lit_in;
			address_buff	<= address_in;
			
			length_left		<= 5'd16 - {2'b0,garbage_in};
			slice_req		<= 1'b0;
			garbage_buff	<= garbage_in;
			
			if(address_in[16]^overflow_record)begin
				state		<=	3'd3;
			end else begin
				state		<=	3'd2;
			end
			/////for debug
			//if(data[143:112]==32'h3eb4_120e)begin
			/*if(address_in>17'h3910)begin
				$display("data detected   %d",PARSER_NUM);
			end*/

		end
		
		///set the output to sub parsers to invalid
		lit_valid		<=1'b0;
		copy_valid		<=1'b0;
	end
	
	3'd2:begin
		if(!stop_flag)begin    ///only work if not stop
		data_buff			<=	(data_buff<<{lza_z,3'b0});
		tokenpos_buff		<=	(tokenpos_buff<<lza_z);
		valid_fsm			<=	tokenpos_buff[15] |	start_lit_buff;
		
		if(lza_a==1'b0)begin empty_flag<=1'b1; end
		else begin empty_flag<=1'b0; end
		
		if(page_finish)begin
			state	<=3'd0;
		end else if((~start_lit_buff)&((data_buff[137:136]==2'b01 & copy_sep_2b) | (data_buff[137:136]==2'b10 & copy_sep_3b)))begin
			state			<=3'd4;
		end else if(address_buff_w[16]^overflow_record)begin
			state			<=3'd3;
			block_finish_r	<=1'b1;
		end else if(lza_a==1'b0)begin
			state			<=3'd1;
			slice_req		<=1'b1;
		end
		
		address_buff	<= address_buff_w;
		//record whether a overflow happens in this cycle
		overflow_flag	<= address_buff_w[16]^overflow_record;
		
		if(start_lit_buff==1'b1)begin
		//lza_z is the length of leading zero, 
			if(lza_a)begin
			//lit_length should be the length minus one
				lit_length 	<= lza_z-4'b1;
			end else begin
			//if no start of token, all of data are literal
				lit_length 	<= 4'b1111-garbage_buff;
			end
			
			lit_data		<=data_buff[143:16];
			lit_valid		<=1'b1;
			lit_address     <=address_buff[15:0];
			
			copy_valid		<=1'b0;
			start_lit_buff	<=1'b0;

			if(lza_a==1'b0)begin 
				length_left		<=5'b0;
			end else begin
				length_left		<=length_left-lza_z;
			end
				
		end
		else begin
		
		
		casex(data_buff[143:136])
			8'b1111_0000:begin  //when the literal token has 2 bytes
				if(length_left==5'd1||length_left==5'd2)begin	lit_valid<=1'b0; end  ///if this is the last byte or last two bytes, there is no literal content
				else begin lit_valid<=1'b1; end
				
				//in this case, the length will be at least 60, the token is 2 bytes long, and the lit_length = length -1, so -3
				lit_length		<=length_left-5'd3;  	
				
				lit_data		<={data_buff[127:16],16'b0};
				lit_address     <=address_buff[15:0];
				copy_valid		<=1'b0;
				
				//this token (including the corresponding literal content) will certainly exceed this slice
				length_left		<=5'b0;
			end
			
			8'b1111_0100:begin  //when the literal token has 3 bytes
				if(length_left==5'd1||length_left==5'd2||length_left==5'd3)begin	lit_valid<=1'b0; end  ///if this is the last byte or last two bytes or last three bytes, there is no literal content
				else begin lit_valid<=1'b1; end
				
				//in this case, the length will be at least 61, the token is 3 bytes long, and the lit_length = length -1, so -4
				lit_length		<=length_left-5'd4;	
				
				lit_data		<={data_buff[119:16],24'b0};
				lit_address     <=address_buff[15:0];
				copy_valid		<=1'b0;
				
				//this token (including the corresponding literal content) will certainly exceed this slice
				length_left		<=5'b0;
			end
			
			8'b1111_1000:begin
				$display("wrong case 8'b1111_1000   %d",PARSER_NUM);
				debug_signal1	<= 1'b1;
			end
			
			8'b1111_1100:begin
				$display("wrong case 8'b1111_1100	%d",PARSER_NUM);
				debug_signal1	<= 1'b1;
			end
			
			8'bxxxx_xx01:begin
				if(copy_sep_2b)begin  ///if offset <=(length-1) -> offset<length	
					copy_length		<=offset_2b-16'b1;
				end
				else begin
					copy_length		<=length_2b;
				end
				copy_offset			<=offset_2b;
				copy_address		<=address_buff[15:0];
				copy_length_record	<=length_2b-offset_2b;
				copy_offset_record	<=offset_2b;

				copy_valid		<=1'b1;
				lit_valid		<=1'b0;
				
				//this token is 2-byte long
				if(lza_a==1'b0)begin
					length_left		<=5'b0;
				end else begin
					length_left		<=length_left-5'd2;
				end
				
			end
			
			8'bxxxx_xx10:begin
				if(copy_sep_3b)begin  ///if offset <=(length-1) -> offset<length	
					copy_length		<=offset_3b-16'b1;
				end
				else begin
					copy_length		<=length_3b;
				end
				copy_offset			<=offset_3b;
				copy_address		<=address_buff[15:0];
				copy_length_record	<=length_3b-offset_3b;
				copy_offset_record	<=offset_3b;
				copy_valid		<=1'b1;
				lit_valid		<=1'b0;
				
				//this token is 3-byte long
				if(lza_a==1'b0)begin
					length_left		<=5'b0;
				end else begin
					length_left		<=length_left-5'd3;
				end
				
			end
			
			8'bxxxx_xx11:begin
				$display("wrong case 8'bxxxx_xx11  %d",PARSER_NUM);
				debug_signal1	<= 1'b1;
			end
			
			default:begin  ///when the literal token has 1 byte
				
				if(length_left==5'd1)begin	lit_valid<=1'b0; end  ///if this is the last byte, there is no literal content
				else begin lit_valid<=1'b1; end
				
				if({1'b0,length_left-5'b1}>(data_buff[143:138]+6'b1))begin  //if  lengthleft-1>length of token
					lit_length	<=data_buff[141:138];  end
				else begin  lit_length	<=length_left-5'd2; end
		
				lit_data		<={data_buff[135:16],8'b0};
				lit_address     <=address_buff[15:0];
				
				copy_valid		<=1'b0;
				
				 
				if(lza_a==1'b0)begin
					length_left		<=5'b0;
				end else begin
					length_left		<=length_left-5'd2-data_buff[143:138];
				end
				
			end	
		endcase
		end
	end
	else begin  ///if stop
		lit_valid		<=1'b0;
		copy_valid		<=1'b0;
	end
	end
	
	3'd3:begin  ///if the overflow of address happens, it will first go to this state
		
		if(length_left	==5'b0)begin //if the current slice is totally processed, go back to state 1
			state		<=3'd1;
			slice_req	<=1'b1;
		end else
		if(page_finish)begin
		//if this file is finished and the BRAMs are cleaned, go back to the initial state
			state	<=3'd0;
		end else 
		if(block_out_finish)begin
			//if this slice is not totally processed, go back to state2 to continue
				state	<=3'd2;
		end
		block_finish_r	<=1'b0;
		lit_valid		<=1'b0;
		copy_valid		<=1'b0;
	end
	
	/*
	In Snappy, it is allowed to have offset < length, for example, in "abc abc abc abc" every "abc" 
	is regarded as repitation of the last "abc" (except the first "abc", which is literal content), 
	it is extremly slow to be solved directly, so I add this special state, in this state, each "abc" 
	will be regarded as the repitition of the first "abc"
	*/
	3'd4:begin  ///solve the offset<length
		if(!stop_flag)begin    ///only work if not stop
			if(copy_offset_record<=copy_length_record)begin
				copy_length<=copy_length;
			end else begin
				copy_length<=copy_length_record;
				if(empty_flag)begin ///if this is the last token, set request and jump to state 1
					state			<=3'd1;
					slice_req		<=1'b1;
				end else if(overflow_flag)begin //check whether this is the last token of a 64KB block
					state			<=3'd3;
				end else begin
					state			<=3'd2;
				end
			end
			copy_address	<=copy_address+copy_offset_record;
			copy_offset		<=copy_offset+copy_offset_record;
			copy_length_record<=copy_length_record-copy_offset_record;
			copy_valid		<=1'b1;			
		end else begin
			copy_valid		<=1'b0;
		end
	end
	
	default:begin	state	<=	3'b0;	end
	endcase
end


LZA16 lza16(
	.x({1'b0,tokenpos_buff[14:0]}), // input data
	.a(lza_a),						//
	.z(lza_z)
);
////////////////generate parser for literal and its FIFOs
wire[255:0] lit_data_w;
wire[35:0] lit_address_w;
wire[35:0] lit_wr_w;
wire[15:0] lit_ram_select_w;
wire[15:0] lit_ram_select;
wire[3:0] lit_fifo_valid;
wire[3:0] lit_fifo_empty;
parser_lit#(
	.PARSER_NUM(PARSER_NUM)
)parser_lit0(
	.clk(clk),
	.rst_n(rst_n),
	.data(lit_data),
//	.start_lit(lit_start_lit),
	.length(lit_length), //length of the token  minus 1
	.address_in(lit_address),   /////////[2:0]shift within bram, [4:3]:bram selection [15:5]: bram address 
	.valid_in(lit_valid),   //if stop, input should be invalid
	
	.data0(lit_data_w[63:0]),
	.data1(lit_data_w[127:64]),
	.data2(lit_data_w[191:128]),
	.data3(lit_data_w[255:192]),
	.address0(lit_address_w[8:0]),
	.address1(lit_address_w[17:9]),
	.address2(lit_address_w[26:18]),
	.address3(lit_address_w[35:27]),
	.wr_out0(lit_wr_w[8:0]),  //if [8] is 1, at least one of [7:0] is 1 
	.wr_out1(lit_wr_w[17:9]),
	.wr_out2(lit_wr_w[26:18]),
	.wr_out3(lit_wr_w[35:27]),
	.ram_select_out0(lit_ram_select_w[3:0]),
	.ram_select_out1(lit_ram_select_w[7:4]),
	.ram_select_out2(lit_ram_select_w[11:8]),
	.ram_select_out3(lit_ram_select_w[15:12]),
	.valid_out()
);

reg[3:0] valid_reg;
genvar lit_i;
generate
	for(lit_i=0;lit_i<4;lit_i=lit_i+1)begin:generate_lit
		always@(posedge clk)begin
			if(~rst_n)begin
				valid_reg[lit_i] <=1'b0;
			end else if((lit_fifo_empty[lit_i]==1'b0) & (valid_reg[lit_i]==1'b0))begin
				valid_reg[lit_i] <=1'b1;
			end else if(lit_rd[lit_i])begin
				valid_reg[lit_i] <= ~lit_fifo_empty[lit_i];
			end
		end
		
		reg rd_en_w;
		always@(*)begin
			if(lit_fifo_empty[lit_i])begin
				rd_en_w <=1'b0;
			end
			else if((valid_reg[lit_i]==1'b0)|lit_rd[lit_i])begin
				rd_en_w <=1'b1;
			end
			else begin
				rd_en_w <=1'b0;
			end
		end
		
		/**********wire for debug*/
	wire debug_literal_full;
	always@(posedge clk)begin
		if(debug_literal_full)begin
			$display("literal fifo full    %d %d",PARSER_NUM,lit_i);
		end
	end

/***********************/
		
		fifo_parser_lit fifo_lit0(
		.clk(clk),
		.srst(~rst_n),
	
		.full(debug_literal_full),
		/////////9 bit address + 4 bit ram select + 8 bit writevalid + 64bit data= 85 bits
		.din({lit_address_w[lit_i*9+8:lit_i*9],lit_ram_select_w[lit_i*4+3:lit_i*4],lit_wr_w[lit_i*9+7:lit_i*9],lit_data_w[lit_i*64+63:lit_i*64]}),
		.wr_en(lit_wr_w[lit_i*9+8]),
	
		.empty(lit_fifo_empty[lit_i]),
		.dout({lit_address_out[lit_i*9+8:lit_i*9],lit_ram_select[lit_i*4+3:lit_i*4],lit_wr_out[lit_i*8+7:lit_i*8],lit_data_out[lit_i*64+63:lit_i*64]}),
		.rd_en(rd_en_w),
	
		.valid(),
		.prog_full(lit_almost_full[lit_i]),
	
		.wr_rst_busy(),
		.rd_rst_busy()
		);
		
	end
endgenerate
assign lit_fifo_valid=valid_reg;


/////////////////////////parser for copy
wire[143:0] copy_address_w;
wire[15:0] copy_ram_select_w;
wire[127:0] copy_rd_w;
wire[15:0] copy_offset_w;
wire[15:0] copy_fifo_empty;
parser_copy #(
	.PARSER_NUM(PARSER_NUM)
)parser_copy0(
	.clk(clk),
	.rst_n(rst_n),
	.length_in(copy_length),
	.address_in(copy_address),	/////////[2:0]:shift [6:3]index for bram [15:7]: bram address 
	.offset_in(copy_offset),
	.valid_in(copy_valid),
	
	.address_out(copy_address_w), 		//address for 16 rams, each is 9-bits
	.ram_select(copy_ram_select_w),     ///chose it wants to read from a ram
	.rd_out(copy_rd_w),
	.offset_out(copy_offset_w)
);

genvar copy_i;
generate
	for(copy_i=0;copy_i<16;copy_i=copy_i+1)begin:generate_copy
		reg valid_reg;
		always@(posedge clk)begin
			if(~rst_n)begin
				valid_reg <=1'b0;
			end else if(copy_fifo_empty[copy_i]==1'b0 & valid_reg==1'b0)begin
				valid_reg <=1'b1;
			end else if(copy_rd[copy_i])begin
				valid_reg <= ~copy_fifo_empty[copy_i];
			end
		end
		
		reg rd_en_w;
		always@(*)begin
			if(copy_fifo_empty[copy_i])begin
				rd_en_w<=1'b0;
			end
			else if((valid_reg==1'b0)|copy_rd[copy_i])begin
				rd_en_w<=1'b1;
			end
			else begin
		          rd_en_w <=1'b0;
			end
		end
		
/**********wire for debug*/
wire debug_copy_full;
	always@(posedge clk)begin
	if(debug_copy_full)begin
		$display("copy fifo full    %d %d",PARSER_NUM,copy_i);
	end
end
/***********************/
		
		fifo_parser_copy fifo_parser_copy0(
		.clk(clk),
		.srst(~rst_n),
	
//		.almost_full(),
		.full(debug_copy_full),
		
		///9 bites address + 8 bits read select + 16 offset = 33 bits
		.din({copy_address_w[copy_i*9+8:copy_i*9],copy_rd_w[127-copy_i*8:127-7-copy_i*8],copy_offset_w}),
		.wr_en(copy_ram_select_w[copy_i]),
	
		.empty(copy_fifo_empty[copy_i]),
		.dout({copy_address_out[copy_i*9+8:copy_i*9],copy_rd_out[copy_i*8+7:copy_i*8],copy_offset_out[copy_i*16+15:copy_i*16]}),
		.rd_en(rd_en_w),
	
		.valid(),
		.prog_full(copy_almost_full[copy_i]),
	
		.wr_rst_busy(),
		.rd_rst_busy()
		);
		assign copy_ram_select_out[copy_i]=valid_reg;
	end
endgenerate

/////////////////////////////////////////
reg all_empty_reg;
always@(posedge clk)begin
	/*if all FIFOs are empty and no slice inside, it is empty*/
	all_empty_reg	<=(copy_fifo_empty==16'hffff) & (lit_fifo_empty==4'hf) & slice_req;
end

assign ready	=slice_req;
assign all_empty=all_empty_reg;
assign block_finish=block_finish_r;

wire[15:0] lit_ram_select_temp;
assign lit_ram_select_temp[3:0]		=lit_ram_select[3:0] 	& {4{lit_fifo_valid[0]}};
assign lit_ram_select_temp[7:4]		=lit_ram_select[7:4] 	& {4{lit_fifo_valid[1]}};
assign lit_ram_select_temp[11:8]	=lit_ram_select[11:8] 	& {4{lit_fifo_valid[2]}};
assign lit_ram_select_temp[15:12]	=lit_ram_select[15:12] 	& {4{lit_fifo_valid[3]}};

assign lit_ram_select_out[3:0]	={lit_ram_select_temp[12],lit_ram_select_temp[8],	lit_ram_select_temp[4],lit_ram_select_temp[0]};
assign lit_ram_select_out[7:4]	={lit_ram_select_temp[13],lit_ram_select_temp[9],	lit_ram_select_temp[5],lit_ram_select_temp[1]};
assign lit_ram_select_out[11:8]	={lit_ram_select_temp[14],lit_ram_select_temp[10],	lit_ram_select_temp[6],lit_ram_select_temp[2]};
assign lit_ram_select_out[15:12]={lit_ram_select_temp[15],lit_ram_select_temp[11],	lit_ram_select_temp[7],lit_ram_select_temp[3]};

endmodule 

/*
Leading zero counter
*/
module LZA16(
			input[15:0] x,	//the input
			output a,		//0 if every bit in x is zero
			output[3:0] z	//the length of leading zero in x
			);
wire[3:0] a1;
wire[7:0] z1;
reg[1:0] z_out;
assign z[1:0]=z_out;
always@(*)begin
	case(z[3:2])
		2'b11: begin z_out<=z1[7:6]; end
		2'b10: begin z_out<=z1[5:4]; end
		2'b01: begin z_out<=z1[3:2]; end
		2'b00: begin z_out<=z1[1:0]; end
	endcase
end
PENC PENC4(.x(a1[3:0]),.a(a),.z(z[3:2]));		
PENC PENC3(.x(x[3:0]),.a(a1[0]),.z(z1[7:6]));
PENC PENC2(.x(x[7:4]),.a(a1[1]),.z(z1[5:4]));
PENC PENC1(.x(x[11:8]),.a(a1[2]),.z(z1[3:2]));
PENC PENC0(.x(x[15:12]),.a(a1[3]),.z(z1[1:0]));
				
endmodule

module PENC(
			input[3:0] x,
			output a,
			output[1:0] z
			);

assign z[1]=~(x[3]|x[2]);
assign z[0]=~(((~x[2])&x[1])|x[3]);
assign a=(x[0]|x[1]|x[2]|x[3]);
endmodule 
