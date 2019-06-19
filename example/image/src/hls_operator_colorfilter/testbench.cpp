#ifndef __SYNTHESIS__

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

  hls_operator_colorfilter(in, out);

  FILE * outfile = fopen("../../../../apples.out.bmp", "w");
  do {
    element = out.read();
    fwrite(&(element.data), 1, 8, outfile);
  } while (!element.last);

  return 0;
}

#endif
