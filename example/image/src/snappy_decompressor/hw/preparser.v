/****************************
Module name: 	preparser
Author:			Jianyu Chen
Email:			chenjy0046@gmail.com
School:			Delft University of Technology
Date:			23th April, 2018
Function:		The first level parser, get 16 Byte data per clock cycle. Parse the data
				into slice. Each slice contains the information of: 
				1)current 16 Byte data
				2)the beginning 2 Byte from the next 16-byte data
				3)the starting address of this slice in the decompresion result
				4)a flag bit to shows whether this slice starts with literal content
				5)a count of the length of the garbage in the start
				6)the position of starting byte of all the token in this slice
				Each slice can be processed independently by the second level parser
****************************/

`timescale 1ns/1ps

module preparser(
	input clk,
	input rst_n,
	input[127:0] data,
	input valid,
	input start,
	input[34:0] compression_length_in,
	
	
	output[143:0] data_out,
	output[15:0] token_pos,
	output[16:0] address, /////////[16] is for over flow, [15:0] store the read address
	output start_lit,  ///whether this 18 Byte starts with literal
	output[2:0] garbage_cnt_out,
	output valid_out,
	output page_input_finish
);

reg[127:0] data_buff1;
reg valid_1;
always@(posedge clk)begin
	data_buff1<=data;
	if(~rst_n)begin
		valid_1	<= 1'b0;
	end else begin
		valid_1	<=	valid;
	end
	
	/////////for debug only
//	if(data_buff1[127:96]==32'h033e_b412)begin
//		$display("preparser data found");
//	end
	//////////////////////
end

/*****************************
2nd pipeline
*****************************/
reg init_flag;
reg[127:0] data_buff2;
reg[143:0] data_buff2_1; 
reg valid_2;
reg[34:0] compression_length, data_cnt_max;  ///[34:4]: number of 16B,  [3:0] whether there is an extra unfull data

reg last_flag,last_flag_delay,finish_flag;   ////////whether all data has passed
reg[15:0] pos_final, pos_final_delay, pos_final2;  ///token pos of final
reg[30:0] data_cnt;
always@(posedge clk)begin
/*****************************
This decompressor can only process the token with the length of 1 byte, 2 bytes and 3bytes (in theory,
there are longer token, but google's implementation cannot generate longer token). So it is possible that
the first byte of a token is in this 16-byte data, and the last two bytes are in the next one. So i need to 
first transfer the 16-byte data into this format: [current 16-byte][starting 2 bytes of the next 16-byte]
in order to do so, always buffer a 16-byte and wait for the next 16-byte data 
*****************************/	
	if(valid_1)begin
		data_buff2<=data_buff1;
	end
	data_buff2_1[143:16]<=data_buff2;
	data_buff2_1[15:0]<=data_buff1[127:112];
	
	if(start)begin 
		init_flag<=1'b0;
	end else if(valid_1)begin
		init_flag<=1'b1;
	end

/***************************
solved the last slice, since one 16-byte data is buffered, we need hide the first valid signal and generate 
an extra valid signal for the last slice. After all the data is passed, make finish_flag 0 to avoid the garbage
**************************/
	if(start)begin
		compression_length	<= compression_length_in;
		data_cnt_max		<= compression_length_in - 35'd16;
	end
	
	if(start)begin
		data_cnt		<=30'b0;
		last_flag		<=1'b0;
		pos_final		<=16'hffff;
		pos_final2 <= 16'hffff;
	end else if(valid_1)begin
		if({data_cnt,4'b0}<data_cnt_max)begin
			last_flag	<=1'b0;
			data_cnt	<= data_cnt + 31'b1;
		end else begin
			data_cnt	<= 3'b0;
			last_flag	<= 1'b1;
			
			//if the last slice will come in the next cycle, assign it
			pos_final2 <= pos_final;
		end
	end else begin
		last_flag<=1'b0;
	end
	
	if(start)begin   //after the last data passed, make garbage invalid
		finish_flag	<=1'b1;
	end else if(last_flag_delay) begin
		finish_flag	<=1'b0;
	end
	
	///some signals has to be delayed for a clock cycle, since the last slice will come in the next cycle
	if(start)begin
		last_flag_delay	<= 1'b0;
		pos_final_delay	<= 16'hffff;
	end else begin
		last_flag_delay <= last_flag;
		pos_final_delay	<= pos_final2;
	end
	
	if(start)begin
		pos_final	<=16'hffff;
	end else begin
		/*since the last 16-byte block may be not full, there can be some garbage at the end of the last slice.
		These garbage data can be misunderstook as token and cause some unpredictable consequency, so i set pos_final
		as a mask to filter these garbage data*/
		if(compression_length[3:0] == 4'b0)begin
			pos_final	<=16'hffff;
		end else begin
			pos_final	<= ~(16'hffff>>compression_length[3:0]);
		end
	end

	if(~rst_n)begin
		valid_2	<= 1'b0;
	end else begin
		valid_2	<=	valid_1&init_flag;
	end
	
end


reg[143:0] data_buff3;
reg valid_3;
reg[15:0] pos_final3;  ///token pos of final
always@(posedge clk)begin
	data_buff3<=data_buff2_1;

	pos_final3	<= pos_final_delay;
	
	if(~rst_n)begin
		valid_3		<= 1'b0;
	end else begin
		valid_3		<= (valid_2|last_flag_delay)&finish_flag;	
	end
	
end


///////4th pipeline
wire[143:0] token_leng_w;
wire[255:0]	tokenpos_w;
wire[271:0] lit_length_w;
wire[3:0] ds_pos;
generate
genvar i;
for(i=0;i<16;i=i+1)begin:generate_decoder
	decoder decoder0(
		.data(data_buff3[143-i*8:120-i*8]),
	    .bytenum(15-i),
		.tokenleng(token_leng_w[143-i*9:135-i*9]),
		.tokenpos(tokenpos_w[255-16*i:240-16*i]),
		.lit_leng(lit_length_w[271-17*i:255-17*i])
	);
end
endgenerate
decoder_start decoder_start0(
	.data(data_buff3[143:104]),
	.length(),
	.tokenpos(ds_pos)
);
reg init_flag4;  ////set to 1 when processing the first page 

//store the length of token (for literal token, sum of the length of token and the length of literal data)
reg[8:0] token1_length,token2_length,token3_length,token4_length,token5_length,token6_length,token7_length,token8_length,token9_length,token10_length,token11_length,token12_length,token13_length,token14_length,token15_length,token16_length;
//store the position of the next token in this slice
reg[15:0] token1_pos,token2_pos,token3_pos,token4_pos,token5_pos,token6_pos,token7_pos,token8_pos,token9_pos,token10_pos,token11_pos,token12_pos,token13_pos,token14_pos,token15_pos,token16_pos;
reg[16:0] token1_lit,token2_lit,token3_lit,token4_lit,token5_lit,token6_lit,token7_lit,token8_lit,token9_lit,token10_lit,token11_lit,token12_lit,token13_lit,token14_lit,token15_lit,token16_lit;
reg valid_4;
//reg rst_4;
reg init_flag3;
reg[143:0] data_buff4;
reg[4:0] ds_pos0;
reg[15:0] pos_final4;  ///token pos of final
always@(posedge clk)begin
	
	if(start)begin
		init_flag3<=1'b0;
	end else if(valid_3)begin
		init_flag3<=1'b1;
	end
	
	//check whether it is the first slice
	if(valid_3&init_flag3)begin
		token1_pos<=tokenpos_w[255:240];
		token1_lit<=lit_length_w[271:255];
		ds_pos0<=5'b11111;
	end 
	else if(valid_3)begin
		token1_pos<={ds_pos,12'b1111_1111_1111};
		token1_lit<=16'b0;
		ds_pos0	<={1'b0,ds_pos};
	end

/*****************************
Process to calculate the starting byte of every token:
Each byte in the 16-byte data can be the starting byte of a token. First 
I assume that each one is the starting byte of a token, in this case, i can
calculate the position of starting byte of the next token. 
For example, if I assume that the first byte is a token, I can calculate that 
the the poition of the next token (whether it is in the rest 15 byte, if in, which
is the starting byte), and i can get a 15 byte data in this format: 00...0011...11,
each bit is corresponding to one byte. The leading one is the starting position of
the next token. If all 0, means the next token is not in this slice 
And in practice, i know the position of the first token, and i can use these assumption to
calculate the position of the 2nd token, the 3rd token ... and the first token in
the next slice.
I do this process in two step in two pipeline stages in order to keep a high
working frequency. In the real test, this module can achive 250MHz in the Kintex
UltraScale. As i know, this is the only algerithm to do this function in this frequency.
If you have a better way, please contact me, I am very interesting about that. 
*****************************/		
	token1_length<=token_leng_w[143:135];token2_length<=token_leng_w[134:126];token3_length<=token_leng_w[125:117];token4_length<=token_leng_w[116:108];
	token5_length<=token_leng_w[107:99]; token6_length<=token_leng_w[98:90];  token7_length<=token_leng_w[89:81];  token8_length<=token_leng_w[80:72];
	token9_length<=token_leng_w[71:63];  token10_length<=token_leng_w[62:54]; token11_length<=token_leng_w[53:45]; token12_length<=token_leng_w[44:36];
	token13_length<=token_leng_w[35:27]; token14_length<=token_leng_w[26:18]; token15_length<=token_leng_w[17:9];  token16_length<=token_leng_w[8:0];
	
	token2_pos<=tokenpos_w[239:224];	token3_pos<=tokenpos_w[223:208];	token4_pos<=tokenpos_w[207:192];
	token5_pos<=tokenpos_w[191:176];	token6_pos<=tokenpos_w[175:160];	token7_pos<=tokenpos_w[159:144];	token8_pos<=tokenpos_w[143:128];
	token9_pos<=tokenpos_w[127:112];	token10_pos<=tokenpos_w[111:96];	token11_pos<=tokenpos_w[95:80];		token12_pos<=tokenpos_w[79:64];
	token13_pos<=tokenpos_w[63:48];		token14_pos<=tokenpos_w[47:32];		token15_pos<=tokenpos_w[31:16];		token16_pos<=tokenpos_w[15:0];
	
	token2_lit<=lit_length_w[254:238];	token3_lit<=lit_length_w[237:221];	token4_lit<=lit_length_w[220:204];
	token5_lit<=lit_length_w[203:187];    token6_lit<=lit_length_w[186:170];    token7_lit<=lit_length_w[169:153];    token8_lit<=lit_length_w[152:136];
	token9_lit<=lit_length_w[135:119];    token10_lit<=lit_length_w[118:102];    token11_lit<=lit_length_w[101:85];     token12_lit<=lit_length_w[84:68];
	token13_lit<=lit_length_w[67:51];     token14_lit<=lit_length_w[50:34];     token15_lit<=lit_length_w[33:17];     token16_lit<=lit_length_w[16:0];
		
	data_buff4<=data_buff3;

	pos_final4<=pos_final3;
	if(~rst_n)begin
		valid_4	<= 1'b0;
	end else begin
		valid_4	<=	valid_3;
	end
	
//	rst_4	<=	rst_3;
	init_flag4<=	init_flag3;
end
///////////////////////////////

////////////5th stage
reg firstpage_flag;      	//////set to high if it is not the first page
reg init_flag5;
reg[16:0] lit_length;  ////length of literal starts from prevails 16B
reg[15:0] tokenpos;
reg[8:0] token1_length_2,token2_length_2,token3_length_2,token4_length_2,token5_length_2,token6_length_2,token7_length_2,token8_length_2,token9_length_2,token10_length_2,token11_length_2,token12_length_2,token13_length_2,token14_length_2,token15_length_2,token16_length_2;
reg valid_5;
//reg rst_5;
reg[1:0] tokenpos_next;///whether the start 2 byte of the next 16-byte is token
reg start_lit_flag; // set to 1 if the next page will start with literal content
reg[15:0] current_lit_length; ///length of literal content starts in the beginning in this 16B, from 0 to 16
reg[143:0] data_buff5;
reg[15:0] pos_final5;  ///token pos of final
wire[15:0] tokenpos_final;
wire[15:0] token0_temp,token1_temp,token2_temp,token3_temp,token4_temp,token5_temp,token6_temp,token7_temp,token8_temp,token9_temp,token10_temp,token11_temp,token12_temp,token13_temp,token14_temp,token15_temp,token16_temp;

/*****************************************
Use the assumption of token position and the token position of the first token,
to calculate the position of all the token
*****************************************/
assign token0_temp=(16'hffff>>lit_length)&{tokenpos_next[1:0],14'b1111_1111_1111_11}&{ds_pos0,11'b1111_1111_11_1};
assign token1_temp[14:0]={15{~token0_temp[15]}}|token1_pos[15:1]; 
assign token2_temp[13:0]={14{~token0_temp[14]}}|{14{~token1_temp[14]}}|token2_pos[15:2];
assign token3_temp[12:0]={13{~token0_temp[13]}}|{13{~token1_temp[13]}}|{13{~token2_temp[13]}}|token3_pos[15:3];
assign token4_temp[11:0]={12{~token0_temp[12]}}|{12{~token1_temp[12]}}|{12{~token2_temp[12]}}|{12{~token3_temp[12]}}|token4_pos[15:4];
assign token5_temp[10:0]={11{~token0_temp[11]}}|{11{~token1_temp[11]}}|{11{~token2_temp[11]}}|{11{~token3_temp[11]}}|{11{~token4_temp[11]}}|token5_pos[15:5];
assign token6_temp[9:0]= {10{~token0_temp[10]}}|{10{~token1_temp[10]}}|{10{~token2_temp[10]}}|{10{~token3_temp[10]}}|{10{~token4_temp[10]}}|{10{~token5_temp[10]}}|token6_pos[15:6];
assign token7_temp[8:0]= { 9{~token0_temp[9]}}|{9{~token1_temp[9]}}  |{9{~token2_temp[9]}}  |{9{~token3_temp[9]}}  |{9{~token4_temp[9]}}  |{9{~token5_temp[9]}}  |{9{~token6_temp[9]}}|token7_pos[15:7];
assign token8_temp[7:0]= { 8{~token0_temp[8]}}|{8{~token1_temp[8]}}  |{8{~token2_temp[8]}}  |{8{~token3_temp[8]}}  |{8{~token4_temp[8]}}  |{8{~token5_temp[8]}}  |{8{~token6_temp[8]}}|{8{~token7_temp[8]}}|token8_pos[15:8];
assign token9_temp[6:0]= { 7{~token0_temp[7]}}|{7{~token1_temp[7]}}  |{7{~token2_temp[7]}}  |{7{~token3_temp[7]}}  |{7{~token4_temp[7]}}  |{7{~token5_temp[7]}}  |{7{~token6_temp[7]}}|{7{~token7_temp[7]}}|{7{~token8_temp[7]}}|token9_pos[15:9];
assign token10_temp[5:0]={ 6{~token0_temp[6]}}|{6{~token1_temp[6]}}  |{6{~token2_temp[6]}}  |{6{~token3_temp[6]}}  |{6{~token4_temp[6]}}  |{6{~token5_temp[6]}}  |{6{~token6_temp[6]}}|{6{~token7_temp[6]}}|{6{~token8_temp[6]}}|{6{~token9_temp[6]}}|token10_pos[15:10];
assign token11_temp[4:0]={ 5{~token0_temp[5]}}|{5{~token1_temp[5]}}  |{5{~token2_temp[5]}}  |{5{~token3_temp[5]}}  |{5{~token4_temp[5]}}  |{5{~token5_temp[5]}}  |{5{~token6_temp[5]}}|{5{~token7_temp[5]}}|{5{~token8_temp[5]}}|{5{~token9_temp[5]}}|{5{~token10_temp[5]}}|token11_pos[15:11];
assign token12_temp[3:0]={ 4{~token0_temp[4]}}|{4{~token1_temp[4]}}  |{4{~token2_temp[4]}}  |{4{~token3_temp[4]}}  |{4{~token4_temp[4]}}  |{4{~token5_temp[4]}}  |{4{~token6_temp[4]}}|{4{~token7_temp[4]}}|{4{~token8_temp[4]}}|{4{~token9_temp[4]}}|{4{~token10_temp[4]}}|{4{~token11_temp[4]}}|token12_pos[15:12];
assign token13_temp[2:0]={ 3{~token0_temp[3]}}|{3{~token1_temp[3]}}  |{3{~token2_temp[3]}}  |{3{~token3_temp[3]}}  |{3{~token4_temp[3]}}  |{3{~token5_temp[3]}}  |{3{~token6_temp[3]}}|{3{~token7_temp[3]}}|{3{~token8_temp[3]}}|{3{~token9_temp[3]}}|{3{~token10_temp[3]}}|{3{~token11_temp[3]}}|{3{~token12_temp[3]}}|token13_pos[15:13];
assign token14_temp[1:0]={ 2{~token0_temp[2]}}|{2{~token1_temp[2]}}  |{2{~token2_temp[2]}}  |{2{~token3_temp[2]}}  |{2{~token4_temp[2]}}  |{2{~token5_temp[2]}}  |{2{~token6_temp[2]}}|{2{~token7_temp[2]}}|{2{~token8_temp[2]}}|{2{~token9_temp[2]}}|{2{~token10_temp[2]}}|{2{~token11_temp[2]}}|{2{~token12_temp[2]}}|{2{~token13_temp[2]}}|token14_pos[15:14];
assign token15_temp[2:0]={ 3{~token0_temp[1]}}|{3{~token1_temp[1]}}  |{3{~token2_temp[1]}}  |{3{~token3_temp[1]}}  |{3{~token4_temp[1]}}  |{3{~token5_temp[1]}}  |{3{~token6_temp[1]}}|{3{~token7_temp[1]}}|{3{~token8_temp[1]}}|{3{~token9_temp[1]}}|{3{~token10_temp[1]}}|{3{~token11_temp[1]}}|{3{~token12_temp[1]}}|{3{~token13_temp[1]}}|{3{~token14_temp[1]}}|token15_pos[15:13]; 
assign token16_temp[1:0]={ 2{~token0_temp[0]}}|{2{~token1_temp[0]}}  |{2{~token2_temp[0]}}  |{2{~token3_temp[0]}}  |{2{~token4_temp[0]}}  |{2{~token5_temp[0]}}  |{2{~token6_temp[0]}}|{2{~token7_temp[0]}}|{2{~token8_temp[0]}}|{2{~token9_temp[0]}}|{2{~token10_temp[0]}}|{2{~token11_temp[0]}}|{2{~token12_temp[0]}}|{2{~token13_temp[0]}}|{2{~token14_temp[0]}}|{2{~token15_temp[2]}}|token16_pos[15:14]; 
assign  tokenpos_final[15]=token0_temp[15];
assign	tokenpos_final[14]=token0_temp[14]&token1_temp[14];
assign	tokenpos_final[13]=token0_temp[13]&token1_temp[13]&token2_temp[13];
assign	tokenpos_final[12]=token0_temp[12]&token1_temp[12]&token2_temp[12]&token3_temp[12];
assign	tokenpos_final[11]=token0_temp[11]&token1_temp[11]&token2_temp[11]&token3_temp[11]&token4_temp[11];
assign	tokenpos_final[10]=token0_temp[10]&token1_temp[10]&token2_temp[10]&token3_temp[10]&token4_temp[10]&token5_temp[10];
assign	tokenpos_final[9] =token0_temp[9]&token1_temp[9] &token2_temp[9] &token3_temp[9] &token4_temp[9] &token5_temp[9] &token6_temp[9];
assign	tokenpos_final[8] =token0_temp[8]&token1_temp[8] &token2_temp[8] &token3_temp[8] &token4_temp[8] &token5_temp[8] &token6_temp[8] &token7_temp[8];
assign	tokenpos_final[7] =token0_temp[7]&token1_temp[7] &token2_temp[7] &token3_temp[7] &token4_temp[7] &token5_temp[7] &token6_temp[7] &token7_temp[7]&token8_temp[7];
assign	tokenpos_final[6] =token0_temp[6]&token1_temp[6]&token2_temp[6] &token3_temp[6] &token4_temp[6] &token5_temp[6] &token6_temp[6] &token7_temp[6]&token8_temp[6]&token9_temp[6];
assign	tokenpos_final[5] =token0_temp[5]&token1_temp[5]&token2_temp[5] &token3_temp[5] &token4_temp[5] &token5_temp[5] &token6_temp[5] &token7_temp[5]&token8_temp[5]&token9_temp[5]&token10_temp[5];
assign	tokenpos_final[4] =token0_temp[4]&token1_temp[4]&token2_temp[4] &token3_temp[4] &token4_temp[4] &token5_temp[4] &token6_temp[4] &token7_temp[4]&token8_temp[4]&token9_temp[4]&token10_temp[4]&token11_temp[4];
assign	tokenpos_final[3] =token0_temp[3]&token1_temp[3]&token2_temp[3] &token3_temp[3] &token4_temp[3] &token5_temp[3] &token6_temp[3] &token7_temp[3]&token8_temp[3]&token9_temp[3]&token10_temp[3]&token11_temp[3]&token12_temp[3];
assign	tokenpos_final[2] =token0_temp[2]&token1_temp[2]&token2_temp[2] &token3_temp[2] &token4_temp[2] &token5_temp[2] &token6_temp[2] &token7_temp[2]&token8_temp[2]&token9_temp[2]&token10_temp[2]&token11_temp[2]&token12_temp[2]&token13_temp[2];
assign	tokenpos_final[1] =token0_temp[1]&token1_temp[1]&token2_temp[1] &token3_temp[1] &token4_temp[1] &token5_temp[1] &token6_temp[1] &token7_temp[1]&token8_temp[1]&token9_temp[1]&token10_temp[1]&token11_temp[1]&token12_temp[1]&token13_temp[1]&token14_temp[1];
assign	tokenpos_final[0] =token0_temp[0]&token1_temp[0]&token2_temp[0] &token3_temp[0] &token4_temp[0] &token5_temp[0] &token6_temp[0] &token7_temp[0]&token8_temp[0]&token9_temp[0]&token10_temp[0]&token11_temp[0]&token12_temp[0]&token13_temp[0]&token14_temp[0]&token15_temp[2];
always@(posedge clk)begin
	if(start)begin
		lit_length<=17'b0;
	end
	else if(valid_4	&	(lit_length[16:4]==13'b0 || lit_length[16:0]==17'd16))begin  //check if lit_length>=16
		lit_length<=(token1_lit&{17{tokenpos_final[15]}})|(token2_lit&{17{tokenpos_final[14]}})|(token3_lit&{17{tokenpos_final[13]}})|(token4_lit&{17{tokenpos_final[12]}})|(token5_lit&{17{tokenpos_final[11]}})|(token6_lit&{17{tokenpos_final[10]}})|(token7_lit&{17{tokenpos_final[9]}})|(token8_lit&{17{tokenpos_final[8]}})|(token9_lit&{17{tokenpos_final[7]}})|(token10_lit&{17{tokenpos_final[6]}})|(token11_lit&{17{tokenpos_final[5]}})|(token12_lit&{17{tokenpos_final[4]}})|(token13_lit&{17{tokenpos_final[3]}})|(token14_lit&{17{tokenpos_final[2]}})|(token15_lit&{17{tokenpos_final[1]}})|(token16_lit&{17{tokenpos_final[0]}});
		current_lit_length	<=	{11'b0,lit_length[4:0]};  ///if lit_length <16, current_lit_length=lit_length
	end else if(valid_4) begin
		lit_length[16:4]	<=	lit_length[16:4]-12'b1; //calculate lit_length=lit_length-16
		lit_length[3:0]		<=	lit_length[3:0];
		current_lit_length	<=	16'd16;  				///if lit_length >=16, current_lit_length=16
	end
	
	if(start)begin
		tokenpos_next<=2'b01;
	end
	else if(valid_4)begin
		tokenpos_next[0]	<= token16_temp[0];
		tokenpos_next[1]	<= token16_temp[1]&token15_temp[1];
	end
	
//	tokenpos[15]	<=	tokenpos_final[15]&firstpage_flag;
	tokenpos[15:0]	<=	tokenpos_final[15:0];
	token1_length_2<=token1_length;		token2_length_2<=token2_length;		token3_length_2<=token3_length;		token4_length_2<=token4_length;
	token5_length_2<=token5_length;		token6_length_2<=token6_length;		token7_length_2<=token7_length;		token8_length_2<=token8_length;
	token9_length_2<=token9_length;		token10_length_2<=token10_length;	token11_length_2<=token11_length;	token12_length_2<=token12_length;
	token13_length_2<=token13_length;	token14_length_2<=token14_length;	token15_length_2<=token15_length;	token16_length_2<=token16_length;
	start_lit_flag  <=(lit_length!=17'b0);  ///if lit_length>0, this 18B starts with literal content
	
	data_buff5<=data_buff4;

	if(start)begin
		firstpage_flag	<=	1'b0;
	end 
	else if(valid_4)begin
		firstpage_flag	<=	1'b1;
	end

	pos_final5<=pos_final4;
	
	if(~rst_n)begin
		valid_5<= 1'b0;
	end else begin
		valid_5<= valid_4;
	end
	
//	rst_5	<=	rst_4;
	init_flag5<=	init_flag4;
	///////////////for debug only
	/*if(data_buff5[143:128]==16'h1aef)begin
		$display("preparser stopped  ");
	end*/
end

/////////////6th stage
reg[15:0] tokenpos_6;
reg valid_6;
reg[143:0] data_buff6;
reg[34:0] sum_length;  //[16] is the overflow bit
reg[12:0] current_length;
reg start_lit_flag_6;
reg[2:0] garbage_cnt,garbage_cnt_next;  ///if this page starts with garbage (garbage can be 0 to 2 bytes)
reg lit_garbage,lit_garbage_next;

wire[9:0] addition1_1,addition1_2,addition1_3,addition1_4,addition1_5,addition1_6,addition1_7,addition1_8;
wire[10:0] addition2_1,addition2_2,addition2_3,addition2_4;
wire[11:0] addition3_1,addition3_2;
wire[12:0] addition4_1;
wire[12:0] curent_length_w;
assign curent_length_w=addition4_1+current_lit_length[12:0];
assign addition1_1=	({9{tokenpos[15]}}&token1_length_2)+({9{tokenpos[14]}}&token2_length_2),	addition1_2=({9{tokenpos[13]}}&token3_length_2)+({9{tokenpos[12]}}&token4_length_2),		addition1_3=({9{tokenpos[11]}}&token5_length_2)+({9{tokenpos[10]}}&token6_length_2),	addition1_4=({9{tokenpos[9]}}&token7_length_2) +({9{tokenpos[8]}}&token8_length_2);
assign addition1_5=	({9{tokenpos[7]}}&token9_length_2) +({9{tokenpos[6]}}&token10_length_2),	addition1_6=({9{tokenpos[5]}}&token11_length_2)+({9{tokenpos[4]}}&token12_length_2),		addition1_7=({9{tokenpos[3]}}&token13_length_2)+({9{tokenpos[2]}}&token14_length_2),	addition1_8=({9{tokenpos[1]}}&token15_length_2)+({9{tokenpos[0]}}&token16_length_2);
assign addition2_1=	addition1_1+addition1_2,			addition2_2=addition1_3+addition1_4,				addition2_3=addition1_5+addition1_6,			addition2_4=addition1_7+addition1_8;
assign addition3_1=	addition2_1+addition2_2,			addition3_2=addition2_3+addition2_4;
assign addition4_1=	addition3_1+addition3_2;
always@(posedge clk)begin
		tokenpos_6		<=	tokenpos&pos_final5;
		start_lit_flag_6<=	start_lit_flag;		
		data_buff6		<=	data_buff5;
	
/*to calculate the length of garbage, on some slice, there are some useless bytes in the beginning, i*8
first calculate the length of these bytes, and left shift the data to eliminate it, it is also possible
to do the left shift in parsers, but that is not efficient*/
	if(~init_flag5)begin
	//the starting bytes presenting the length of data will be regarded as garbage here
		casex({data_buff5[143],data_buff5[135],data_buff5[127],data_buff5[119],data_buff5[111]})
			5'b0xxxx:begin	garbage_cnt<=3'd1;	end
			5'b10xxx:begin	garbage_cnt<=3'd2;	end
			5'b110xx:begin	garbage_cnt<=3'd3;	end
			5'b1110x:begin	garbage_cnt<=3'd4;	end
			5'b11110:begin	garbage_cnt<=3'd5;	end
			default:;
		endcase
		lit_garbage<=1'b0;
	end
	else if(valid_5)begin
	/*some bytes are already used in the last slice as the remaining bytes of a token,
	these bytes are also useless here and thus regarded as garbage*/
		garbage_cnt	<= garbage_cnt_next;
		lit_garbage	<=lit_garbage_next;
	end
	
//calculate the length of garbage for the next 18-byte
	if(valid_5)begin
	if(tokenpos[0])begin
		if(data_buff5[23:16]==8'b1111_0000 | data_buff5[17:16]==2'b01)begin
			garbage_cnt_next	<=3'd1;
			if(data_buff5[23:16]==8'b1111_0000)begin  lit_garbage_next<=1'b1;	end
			else begin lit_garbage_next<=1'b0;	end
		end 
		else if(data_buff5[23:16]==8'b1111_0100 | data_buff5[17:16]==2'b10)begin
			garbage_cnt_next	<=3'd2;
			if(data_buff5[23:16]==8'b1111_0100)begin  lit_garbage_next<=1'b1;	end
			else begin lit_garbage_next<=1'b0;	end
		end else begin garbage_cnt_next	<=3'd0;	lit_garbage_next<=1'b0;end
	end
	else if(tokenpos[1])begin
		if(data_buff5[31:24]==8'b1111_0100 | data_buff5[25:24]==2'b10)begin
			garbage_cnt_next	<=3'd1;
			if(data_buff5[31:24]==8'b1111_0100)begin  lit_garbage_next<=1'b1;	end
			else begin lit_garbage_next<=1'b0;	end
		end else begin garbage_cnt_next	<=3'd0;lit_garbage_next<=1'b0;	end
	end
	else begin
		garbage_cnt_next	<= 3'b0;
		lit_garbage_next<=1'b0;
	end
	end
	
	//calculate the length after decompression for this slice, accumulate this length to get the address for the next slice
	if(start)begin
		sum_length		<=35'b0;
		current_length	<=13'b0;
	end 
	else if(valid_5)begin
		current_length	<=	curent_length_w-((lit_garbage_next&init_flag5)?garbage_cnt_next:0);
		sum_length		<= current_length+sum_length;
	end
	
	if(~rst_n)begin
		valid_6	<= 1'b0;
	end else begin
		valid_6	<= valid_5;
	end
	
end
/////////////
/////////////7th stage
reg[15:0] tokenpos_7;
reg valid_7;
reg[16:0] sum_length_7;
reg[2:0] garbage_cnt7;
reg start_lit_flag_7;
reg[143:0] data_buff7;
///////compression_length
always@(posedge clk)begin

///left shift to eliminate the garbage bytes
	case(garbage_cnt)
	3'd0:begin tokenpos_7	<=	tokenpos_6;  				data_buff7	<=	data_buff6; 				end
	3'd1:begin tokenpos_7	<=	{tokenpos_6[14:0],1'b0}; 	data_buff7	<=	{data_buff6[135:0],8'b0};	end
	3'd2:begin tokenpos_7	<=	{tokenpos_6[13:0],2'b0}; 	data_buff7	<=	{data_buff6[127:0],16'b0};	end
	3'd3:begin tokenpos_7	<=	{tokenpos_6[12:0],3'b0};	data_buff7	<=	{data_buff6[119:0],24'b0};	end
	3'd4:begin tokenpos_7	<=	{tokenpos_6[11:0],4'b0};	data_buff7	<=	{data_buff6[111:0],32'b0};	end
	3'd5:begin tokenpos_7	<=	{tokenpos_6[10:0],5'b0};	data_buff7	<=	{data_buff6[103:0],40'b0};	end
	endcase
	
	garbage_cnt7<=garbage_cnt;
	start_lit_flag_7<=	start_lit_flag_6;
	
	if(~rst_n)begin
		valid_7<=1'b0;
	end else if(start_lit_flag_6==1'b0 & tokenpos_6==16'b0)begin
		valid_7<=1'b0;
	end else begin
		valid_7<=valid_6;
	end
	sum_length_7<=sum_length;
end

//when finish_flag is set to high, all work of preparser has finished
assign page_input_finish=~finish_flag;

assign data_out=data_buff7;
assign token_pos=tokenpos_7;
assign address=sum_length_7;
assign start_lit=start_lit_flag_7;
assign valid_out=valid_7;
assign garbage_cnt_out=garbage_cnt7;
endmodule

/****************************
Module name: 	decoder
Author:			Jianyu Chen
Email:			chenjy0046@gmail.com
School:			Delft University of Technology
Date:			13th, July 2018
Function:		Decode the token to get the position of the next token and
				the length of this token (decompressed length) in the decompresion
				result.
****************************/
module decoder(
	input[23:0] data, //[23:16]: byte0 [15:8]:byte1 [7:0]:byte2
	input[31:0] bytenum,
	output[15:0] tokenpos,
	output[8:0] tokenleng, ///length of token(including length of literal content) the length of token within this page
	output[16:0] lit_leng,  ///length of literal content which will go to the next page,including extra token bytes
	output[1:0] lit_minus   //length of extra token byte, minus it when calculate the length
);
reg[8:0] length;                    ////length of token which will be added to curent page
reg[15:0] tokenpos_reg;
reg[17:0] lit_leng_reg;

wire[16:0] length_lit3;//length of literal content when the literal token is 3
wire[8:0] space1,space2,space3;//space left in this slice
assign space1= bytenum[8:0];
assign space2= (bytenum[8:0]>9'b1)?(bytenum[8:0]-9'b1):9'b0;
assign space3= (bytenum[8:0]>9'd2)?(bytenum[8:0]-9'd2):9'b0;
assign length_lit3	=	({1'b0,data[15:8],data[23:16]}+17'b1);
always@(*)begin
	casex(data[23:16])
		8'b1111_0000:begin  //when the literal token has 2 bytes
			length		 <=	space2;// the length of it will be at least 16 
			tokenpos_reg <= 16'b0000_0000_0000_0000;
			lit_leng_reg <= {10'b0,data[15:8]}-{bytenum[17:0]}+18'd2;
		end
		8'b1111_0100:begin  //when the literal token has 3 bytes
			length		 <=	space3;// the length of it will be at least 16 
			tokenpos_reg <= 16'b0000_0000_0000_0000;
			lit_leng_reg <= {2'b0,data[7:0],data[15:8]}-{bytenum[17:0]}+18'd3;
		end
		8'bxxxx_xx01:begin
			length<={5'b0,data[20:18]}+9'd4;
			tokenpos_reg<=16'b0111_1111_1111_1111;
			lit_leng_reg<=16'b0;
		end
		8'bxxxx_xx10:begin
			length<={2'b0,data[23:18]}+9'd1;
			tokenpos_reg<=16'b0011_1111_1111_1111;
			lit_leng_reg<=16'b0;
		end
	default:begin
			length		<=	(space1 > ({3'b0,data[23:18]}+9'b1))?  ({3'b0,data[23:18]}+9'b1):space1;
			tokenpos_reg<={16'b0111_1111_1111_1111>>data[23:18]};
			lit_leng_reg<={12'b0,data[23:18]}-bytenum[17:0]+18'b1;
	    end
	endcase
end

assign tokenpos=tokenpos_reg;
assign tokenleng=length;
assign lit_leng=lit_leng_reg[17]?17'b0:lit_leng_reg[16:0]; //if the number is minus, set to zero
endmodule


/****************************
Module name: 	decoder_start
Author:			Jianyu Chen
Email:			chenjy0046@gmail.com
School:			Delft University of Technology
Date:			13th, July 2018
Function:		At the beginning of a snappy file, there are some byte (1 to 5 bytes) indicating
				the length of the decompression result. This module will use these data to calculate
				the length of result.
****************************/
module decoder_start(
	input[39:0] data,
//	input clk,
	output [34:0] length,////length of this page
	output [3:0] tokenpos
);
reg[34:0] length_reg;
reg[4:0] tokenpos_reg;
always@(*)begin
	casex({data[39],data[31],data[23],data[15],data[7]})
		5'b0xxxx:begin
			length_reg<={28'b0,data[38:32]};
			tokenpos_reg<=4'b1111;
		end
		5'b10xxx:begin
			length_reg<={21'b0,data[30:24],data[38:32]};
			tokenpos_reg<=4'b0111;
		end
		5'b110xx:begin
			length_reg<={14'b0,data[22:16],data[30:24],data[38:32]};
			tokenpos_reg<=4'b0011;
		end
		5'b1110x:begin
			length_reg<={7'b0,data[14:8],data[22:16],data[30:24],data[38:32]};
			tokenpos_reg<=4'b0001;
		end
		5'b11110:begin
			length_reg<={data[6:0],data[14:8],data[22:16],data[30:24],data[38:32]};
			tokenpos_reg<=4'b0000;
		end
		default:;
	endcase
end
assign length=length_reg;
assign tokenpos=tokenpos_reg;

endmodule
