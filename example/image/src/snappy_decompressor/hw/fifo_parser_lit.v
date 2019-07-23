`timescale 1ns/1ps

module fifo_parser_lit
#(
    parameter WIDTH=85,
    parameter DEPTH=8
)
(
    input clk,
	input srst,
	
//	.almost_full(),
	output full,
	input[WIDTH-1:0] din,
	input wr_en,
	
	output empty,
	output[WIDTH-1:0] dout,
	input rd_en,
	
	output valid,
	output prog_full,
	output wr_rst_busy,
	output rd_rst_busy
);
reg [WIDTH-1:0]fifo_out;
reg [WIDTH-1:0]ram[7:0];
reg [3:0]read_ptr,write_ptr,counter;
wire fifo_half,fifo_full;

always@(posedge clk)
  if(srst)
    begin
    read_ptr<=0;
    write_ptr<=0;
    counter<=0;
    end
  else
    case({rd_en,wr_en})
      2'b00:
            counter=counter;     
      2'b01:                        
            begin
				ram[write_ptr]=din;
				counter=counter+1;
				write_ptr=(write_ptr==7)?0:write_ptr+1;
            end
      2'b10:                       
            begin
				fifo_out=ram[read_ptr];
				counter=counter-1;
				read_ptr=(read_ptr==7)?0:read_ptr+1;
            end
      2'b11:                  
            begin
                ram[write_ptr]=din;
				fifo_out=ram[read_ptr];
                write_ptr=(write_ptr==7)?0:write_ptr+1;
                read_ptr=(read_ptr==7)?0:read_ptr+1;
            end
        endcase

assign empty=(counter==0); 
assign fifo_half=(counter>=3);
assign fifo_full=(counter==8);
assign dout=fifo_out;
assign prog_full=fifo_half;
assign full=fifo_full;

endmodule
