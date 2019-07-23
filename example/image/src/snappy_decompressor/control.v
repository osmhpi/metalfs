module control#
(
	parameter NUM_PARSER=6,
	PARSER_ALLONE=16'hffff
)
(
	input clk,
	input rst_n,
	input start,
	
	input tf_empty, //token fifo
	input[NUM_PARSER-1:0] ps_finish,
	input page_input_finish,
	input[NUM_PARSER-1:0] ps_empty, //parser
	input[15:0] ram_empty,//ram 
	
//	input block_out_finish
	input cl_finish,
	output page_finish //all decompression has finished, the data is in BRAMs, but the output is not yet finished 
	
);

reg all_empty;
reg[15:0] all_empty_delay;
reg page_input_finish_flag;
always@(posedge clk)begin
	if((ps_empty==PARSER_ALLONE[NUM_PARSER-1:0]) & ram_empty==16'hffff & tf_empty)begin
		all_empty<=1'b1;
	end else begin
		all_empty<=1'b0;
	end
	
	if(start)begin
		page_input_finish_flag<=1'b0;
	end if(page_input_finish) begin
		page_input_finish_flag<=1'b1;
	end
	
	all_empty_delay[15:1] 	<=all_empty_delay[14:0];
	all_empty_delay[0] 		<=all_empty;
end

reg[2:0] state;
reg page_finish_r,block_finish_r;
always@(posedge clk)begin
	if(~rst_n)begin
		state <= 3'd0;
	end else
	case(state)
	3'd0:begin
		page_finish_r<=1'b0;
		block_finish_r<=1'b0;
		if(~tf_empty)begin
			state<=3'd1;
		end
	end
	3'd1:begin //idle case
		page_finish_r<=1'b0;
		block_finish_r<=1'b0;
		
		/*if all the data in this file has been preparsed and token fifo is empty, which means all data is in parsers or later stages, go to next state*/
		if(page_input_finish & tf_empty)begin
			state<=3'd3;
		end
	end
/*	3'd2:begin//wait for the block finish
		if(all_empty_delay==6'b1111_11 & all_empty==1'b1)begin
			state<=3'd4;
			block_finish_r<=1'b1;
		end
	end
	*/
	3'd3:begin //wait for the page clean finished 
			//when all data in a file (not 64KB block) is processed, set page_finish_r to high to inform other module
			if(all_empty_delay==16'hffff & all_empty==1'b1 & tf_empty)begin
				page_finish_r<=1'b1;
			end
			//after all the valid bits in the BRAMs are cleaned, go to next state
			if(cl_finish)begin
				state	<= 3'd4;
			end
	end
	3'd4:begin //reset the signals and go back to initial state
		block_finish_r	<=1'b0;
		state			<=3'd0;
		page_finish_r	<=1'b0;
	end
	default:state<=3'd0;
	endcase
end
assign page_finish=page_finish_r;
//assign block_finish=block_finish_r;

endmodule 
