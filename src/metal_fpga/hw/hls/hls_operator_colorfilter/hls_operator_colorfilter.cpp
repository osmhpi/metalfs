#include "hls_common/mtl_stream.h"

#define HEADER_BYTES 138
#define HEADER_ELEMENTS (HEADER_BYTES / 8) + 1
#define HEADER_OFFSET (8 - (HEADER_BYTES % 8))

#define RED_FACTOR 54
#define GREEN_FACTOR 183
#define BLUE_FACTOR 18

snapu8_t grayscale(snapu8_t red, snapu8_t green, snapu8_t blue) {
  snapu16_t red_contrib   = ((snapu16_t)red)   * RED_FACTOR;
  snapu16_t green_contrib = ((snapu16_t)green) * GREEN_FACTOR;
  snapu16_t blue_contrib  = ((snapu16_t)blue)  * BLUE_FACTOR;
  return (uint8_t)(red_contrib   >> 8) +
         (uint8_t)(green_contrib >> 8) +
         (uint8_t)(blue_contrib  >> 8);
}

snapu32_t transform_pixel(snapu32_t pixel) {
  snapu8_t alpha = (snapu8_t)(pixel >> 24);
  snapu8_t red   = (snapu8_t)(pixel >> 16);
  snapu8_t green = (snapu8_t)(pixel >> 8);
  snapu8_t blue  = (snapu8_t) pixel;

  snapu8_t gray = grayscale(red, green, blue);

  if (red < green || red < blue) {
    return (alpha, gray, gray, gray);
  } else {
    return (alpha, red, green, blue);
  }
}

void process_stream(mtl_stream &in, mtl_stream &out) {
  mtl_stream_element input, output;
  snapu32_t element_idx = 0;

  do {
    #pragma HLS PIPELINE
    input = in.read();

    if (element_idx < HEADER_ELEMENTS) {
      output = input;
      ++element_idx;
    } else {
      snapu32_t upper_pixel_in, lower_pixel_in,
                upper_pixel_out, lower_pixel_out;
      upper_pixel_in = (input.data >> 32) & 0xffffffff;
      lower_pixel_in = input.data & 0xffffffff;

      upper_pixel_out = transform_pixel(upper_pixel_in);
      lower_pixel_out = transform_pixel(lower_pixel_in);

      output.last = input.last;
      output.keep = input.keep;
      output.data = ((snapu64_t)upper_pixel_out) << 32 | ((snapu64_t)lower_pixel_out);
    }

    out.write(output);
  } while (!output.last);
}

///////////////////////////////////////////////////////////////////////////////

void hls_operator_colorfilter(mtl_stream &in, mtl_stream &out) {

    #pragma HLS INTERFACE axis port=in name=axis_input
    #pragma HLS INTERFACE axis port=out name=axis_output
    #pragma HLS INTERFACE s_axilite port=return bundle=control

    #pragma HLS DATAFLOW
    mtl_stream padded_stream;
    mtl_stream processed_stream;

    insert_padding<HEADER_OFFSET>(in, padded_stream);
    process_stream(padded_stream, processed_stream);
    remove_padding<HEADER_OFFSET>(processed_stream, out);
}

#include "testbench.cpp"
