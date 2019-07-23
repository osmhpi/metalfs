/****************************
Module name: 	ram_module
Author:			Jianyu Chen
Email:			chenjy0046@gmail.com
School:			Delft University of Technology
Date:			10th Sept, 2018
Function:		The module to process the BRAM-based command. In each clock cycle, it can process a write operation 
				and a copy command, and will generate 1 or 2 or 0 write command and maybe also a unsolved copy command.
				NOTICE: this module contains not only BRAM! but also circuit to process the command and FIFOs to store
				the commands.
****************************/
`timescale 1ns/1ps

module ram_module#(
	 parameter BLOCKNUM=4'b0,   ///define the number of this block, from 0 to 15
	 HALFULL_THRESH=9'd24
)(
	input clk,
	input rst_n,
	input rd_en,   ///read the ram
	output empty,   ///whether all fifo are full
	input block_out_finish,  //when the block output is finished
	input page_finish,  //in the end of a file, clean all data
	
	///signal for writing data
	input valid_wr_in,
	input[63:0] lit_in,
	input[8:0] lit_address,
	input[7:0] lit_valid,
		
	//signal for reading data
	input valid_rd_in,
	input[8:0] copy_address,
	input[7:0] copy_valid_in,  //choose bytes to read
	input[15:0] copy_offset_in,
	
	output lit_almost_full,    ///even or odd fifo almost full
	
	///signal for even fifo
	input even_rd,
	output[80:0] even_data_out, ////[80:72] address of ram [71:64] byte valid  [63:0] data 
	output[7:0] even_ram_select,
	
	///signal for odd fifo 
	input odd_rd,
	output[80:0] odd_data_out,    /////[80:72] address of ram [71:64] byte valid  [63:0] data 
	output[7:0] odd_ram_select,
	
	////////signal for unsolved fifo
	input unsolved_rd,
	output unsolved_almost_full,
	output unsolved_half_full,
	output[32:0] unsolved_data_out,
	output unsolved_valid_out,
	/////output the decompression result 
	output[63:0] data_out
);

wire ram_unsolved_valid;
wire[32:0] ram_unsolved_token;

wire[7:0] odd_ram_select_w,even_ram_select_w;

wire even_valid_out,odd_valid_out;
wire ram_even_valid,ram_odd_valid;
wire[63:0] ram_even_data,ram_odd_data;
wire[7:0] ram_even_hit,ram_odd_hit;
wire[8:0] ram_even_address,ram_odd_address;

wire[15:0] ram_select_w;
wire even_empty,odd_empty,unsolved_empty;

wire[63:0] ram_data_out;
ram_block#(
	.BLOCKNUM(BLOCKNUM)
)ram_block0
(
	.clk(clk),
	.rst_n(rst_n),
	.rd_en(rd_en),   ///read the ram
	.block_out_finish(block_out_finish),  //when the page is finished
	.page_finish(page_finish),  //in the end of a file, clean all data
	
	///signal for writing data
	.valid_wr_in(valid_wr_in),
	.lit_in(lit_in),
	.lit_address(lit_address),
	.lit_valid(lit_valid),
	
	//signal for reading data
	.valid_rd_in(valid_rd_in),
	.copy_address(copy_address),
	.copy_valid_in(copy_valid_in),  //choose bytes to read
	.copy_offset_in(copy_offset_in),
	
	
	/////
	.unsolved_valid_out(ram_unsolved_valid),
	.unsolved_token_out(ram_unsolved_token),

	.even_valid_out(ram_even_valid),
	.even_data_out(ram_even_data),
	.even_hit_out(ram_even_hit),
	.even_address_out(ram_even_address),

	.odd_valid_out(ram_odd_valid),
	.odd_data_out(ram_odd_data),
	.odd_hit_out(ram_odd_hit),
	.odd_address_out(ram_odd_address),

	.ram_select_out(ram_select_w),
	
	////////output the compression result
	.data_out(ram_data_out)
);
/**********wire for debug*/
wire debug_even_full,debug_odd_full,debug_unsolved_full;
always@(posedge clk)begin
	if(debug_even_full)begin
		$display("even fifo full   %d",BLOCKNUM);
	end
	if(debug_odd_full)begin
		$display("odd fifo full   %d",BLOCKNUM);
	end
	if(debug_unsolved_full)begin
		$display("unsolved fifo full  %d",BLOCKNUM);
	end

end

/***********************/



wire even_almost_full,odd_almost_full;

wire[7:0] ram_select_even,ram_select_odd;
assign ram_select_even={ram_select_w[14],ram_select_w[12],ram_select_w[10],ram_select_w[8],ram_select_w[6],ram_select_w[4],ram_select_w[2],ram_select_w[0]};
assign ram_select_odd ={ram_select_w[15],ram_select_w[13],ram_select_w[11],ram_select_w[9],ram_select_w[7],ram_select_w[5],ram_select_w[3],ram_select_w[1]};
////width: 89bit   [88:25]:data   [24:17]:byte valid  [16:8]:address  [7:0]:ram_select 
reg even_valid_reg;
always@(posedge clk)begin
	if(~rst_n)begin
		even_valid_reg <=1'b0;
	end else if(even_empty==1'b0 & even_valid_reg==1'b0)begin
			even_valid_reg <=1'b1;
	end else if(even_rd)begin
		even_valid_reg <= ~even_empty;
	end
end
reg even_rd_w;
always@(*)begin
	if(even_empty)begin
		even_rd_w <=1'b0;
	end
	else if((even_valid_reg==1'b0)|even_rd)begin
		even_rd_w <=1'b1;
	end
	else begin
		even_rd_w <=1'b0;
	end
end
read_result_fifo data_even(
	.clk(clk),
	.srst(~rst_n),
	
//	.almost_full(),
	.full(debug_even_full),
	.din({ram_even_address,ram_even_hit,ram_even_data,ram_select_even}),
	.wr_en(ram_even_valid),
	
	.empty(even_empty),
	.dout({even_data_out,even_ram_select_w}),
	.rd_en(even_rd_w),
	
	.valid(),
	.prog_full(even_almost_full),
	.wr_rst_busy(),
	.rd_rst_busy()
);
assign even_valid_out=even_valid_reg;

reg odd_valid_reg;
always@(posedge clk)begin
	if(~rst_n)begin
		odd_valid_reg <=1'b0;
	end else if(odd_empty==1'b0 & odd_valid_reg==1'b0)begin
			odd_valid_reg <=1'b1;
	end else if(odd_rd)begin
		odd_valid_reg <= ~odd_empty;
	end
end

reg odd_rd_w;
always@(*)begin
	if(odd_empty)begin
		odd_rd_w <=1'b0;
	end
	else if((odd_valid_reg==1'b0)|odd_rd)begin
		odd_rd_w <=1'b1;
	end
	else begin
		odd_rd_w <=1'b0;
	end
end
read_result_fifo data_odd(
	.clk(clk),
	.srst(~rst_n),
	
//	.almost_full(),
	.full(debug_odd_full),
	.din({ram_odd_address,ram_odd_hit,ram_odd_data,ram_select_odd}),
	.wr_en(ram_odd_valid),
	
	.empty(odd_empty),
	.dout({odd_data_out,odd_ram_select_w}),
	.rd_en(odd_rd_w),
	
	.valid(),
	.prog_full(odd_almost_full),
	.wr_rst_busy(),
	.rd_rst_busy()
);
assign odd_valid_out=odd_valid_reg;

wire[8:0] unsolved_data_cnt;
reg unsolved_valid_reg;
always@(posedge clk)begin
	if(~rst_n)begin
		unsolved_valid_reg <=1'b0;
	end else if(unsolved_empty==1'b0 & unsolved_valid_reg==1'b0)begin
			unsolved_valid_reg <=1'b1;
	end else if(unsolved_rd)begin
		unsolved_valid_reg <= ~unsolved_empty;
	end
end
reg unsolved_rd_w;
always@(*)begin
	if(unsolved_empty)begin
		unsolved_rd_w <=1'b0;
	end
	else if((unsolved_valid_reg==1'b0)|unsolved_rd)begin
		unsolved_rd_w <=1'b1;
	end
	else begin
		unsolved_rd_w <=1'b0;
	end
end
unsolved_fifo unsolved_fifo0(
	.clk(clk),
	.srst(~rst_n),
	
//	.almost_full(),
	.full(debug_unsolved_full),
	.din(ram_unsolved_token),
	.wr_en(ram_unsolved_valid),
	
	.empty(unsolved_empty),
	.dout(unsolved_data_out),
	.rd_en(unsolved_rd_w),
	
	.data_count(unsolved_data_cnt),
	.prog_full(unsolved_almost_full),
	.valid(),
	.wr_rst_busy(),
	.rd_rst_busy()
);
assign unsolved_valid_out=unsolved_valid_reg;
reg unsolved_half_full_r;
always@(posedge clk)begin
	unsolved_half_full_r <= (unsolved_data_cnt>HALFULL_THRESH);
end

assign unsolved_half_full=(unsolved_data_cnt>HALFULL_THRESH);
assign lit_almost_full=even_almost_full | odd_almost_full;
assign empty	=even_empty & odd_empty & unsolved_empty;
assign odd_ram_select=	odd_ram_select_w 	& {8{odd_valid_out}};
assign even_ram_select=	even_ram_select_w 	& {8{even_valid_out}};
assign data_out=ram_data_out;
endmodule 
