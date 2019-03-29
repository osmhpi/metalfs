#include "hls_common/mtl_stream.h"

#define HEADER_BYTES 138
#define HEADER_ELEMENTS (HEADER_BYTES / 8) + 1
#define HEADER_OFFSET (8 - (HEADER_BYTES % 8))

void process_stream(mtl_stream &in, mtl_stream &out) {
  mtl_stream_element element;
  do {
    element = in.read();
    // element.data = 0xaarrggbb
    out.write(element);
  } while (!element.last);
}

///////////////////////////////////////////////////////////////////////////////

void hls_operator_colorfilter(mtl_stream &in, mtl_stream &out) {

    #pragma HLS INTERFACE axis port=in
    #pragma HLS INTERFACE axis port=out

    #pragma HLS DATAFLOW
    mtl_stream padded_stream;
    mtl_stream processed_stream;

    insert_padding<HEADER_OFFSET>(in, padded_stream);
    process_stream(padded_stream, processed_stream);
    remove_padding<HEADER_OFFSET>(processed_stream, out);
}

///////////////////////////////////////////////////////////////////////////////
// Testbench
///////////////////////////////////////////////////////////////////////////////
#ifdef NO_SYNTH

int main() {
  mtl_stream in;
  mtl_stream out;

  FILE * infile = fopen("../../../../apples.bmp", "r");
  size_t readBytes;
  mtl_stream_element element;
  do {
    readBytes = fread(&(element.data), 1, 8, infile);
    element.keep = 0xff;
    element.last = (readBytes != 8);
    in.write(element);
  } while (!element.last);

  op_image(in, out, true);

  FILE * outfile = fopen("../../../../apples.out.bmp", "w");
  do {
    element = out.read();
    fwrite(&(element.data), 1, 8, outfile);
  } while (!element.last);

  return 0;
}
#endif // NO_SYNTH
///////////////////////////////////////////////////////////////////////////////
