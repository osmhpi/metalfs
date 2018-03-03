#ifndef __MF_STREAM_H__
#define __MF_STREAM_H__

#include <stdint.h>
#include <ap_int.h>
#include <hls_stream.h>
#include <hls_snap.H>

struct byte_stream_element {
    snapu8_t data;
    snap_bool_t last;
};
typedef hls::stream<byte_stream_element> mf_byte_stream;

template<uint8_t NB>
struct stream_element {
    ap_uint<8 * NB> data;
    ap_uint<NB> strb;
    snap_bool_t last;
};

typedef stream_element<8> mf_stream_element;
typedef hls::stream<mf_stream_element> mf_stream;

// Convert a stream of bytes (uint8_t) to a stream of arbitrary width
// (on byte boundaries) words
template<typename T, bool BS>
void stream_bytes2words(hls::stream<T> &words_out, mf_byte_stream &bytes_in)
{
    T tmpword;
    byte_stream_element tmpbyte;
    snap_bool_t last_byte = false;
    BYTES2WORDS_LOOP:
    for (int i = 0; last_byte == false; i++) {
    //#pragma HLS loop_tripcount max=1488
        bytes_in.read(tmpbyte);
        last_byte = tmpbyte.last;
        if (!BS) { // Shift bytes in little endian order
            tmpword.data = (tmpword.data >> 8) |
                (ap_uint<sizeof(tmpword.data) * 8>(tmpbyte.data) << ((sizeof(tmpword.data) - 1) * 8));
            tmpword.strb = (tmpword.strb >> 1) |
                (ap_uint<sizeof(tmpword.data)>(1) << (sizeof(tmpword.data) - 1) * 8);
        } else { // Shift bytes in big endian order
            tmpword.data = (tmpword.data << 8) | ap_uint<sizeof(tmpword.data) * 8>(tmpbyte.data);
            tmpword.strb = (tmpword.strb << 1) | ap_uint<sizeof(tmpword.data)>(1);
        }
        if (i % sizeof(tmpword.data) == sizeof(tmpword.data) - 1 || last_byte) {
            tmpword.last = last_byte;
            words_out.write(tmpword);
        }
    }
}

// Convert an arbitrary width (on byte boundaries) words into a stream of
// bytes (uint8_t)
template<typename T, bool BS>
void stream_words2bytes(mf_byte_stream &bytes_out, hls::stream<T> &words_in)
{
    T inval;
    ap_uint<sizeof(inval.data) * 8> tmpword;
    ap_uint<sizeof(inval.data)> tmpstrb;
    byte_stream_element outval;
    WORDS2BYTES_LOOP:
    for (;;) {
        words_in.read(inval);
        tmpword = inval.data;
        tmpstrb = inval.strb;
        for (int i = 0; i < sizeof(tmpword.data); i++) {
            if (!BS) { // shift bytes out in little endian order
                outval.data = uint8_t(tmpword);
                tmpword >>= 8;
                tmpstrb >>= 1;
                outval.last = inval.last && tmpstrb == ap_uint<sizeof(tmpword.data)>(0);
            } else { // shift bytes out in big endian order
                outval.data = uint8_t(tmpword >> ((sizeof(tmpword.data) - 1) * 8));
                tmpword <<= 8;
                tmpstrb <<= 1;
                outval.last = inval.last && tmpstrb == ap_uint<sizeof(tmpword.data)>(0);
            }
            bytes_out.write(outval);

            if (outval.last)
                break;
        }

        if (inval.last)
            break;
    }
}

// GCD of TIn and TOut data sizes must be either TIn or TOut
template<typename TOut, typename TIn>
void stream_convert_width(hls::stream<TOut> &words_out, hls::stream<TIn> &words_in)
{
//	if (sizeof(TIn::data) == sizeof(TOut::data)) {
//		// Handle this trivial case (no one should actually use this)
//		for (;;) {
//			TIn element = words_in.read();
//			words_out.write(element);
//			if (element.last)
//				break;
//		}
//	}
//	else
	if (sizeof(TIn::data) < sizeof(TOut::data))
    {
        TIn inval;
        TOut tmpword;
        snap_bool_t last_word = false;
        for (uint64_t i = 0; last_word == false; i++) {
        //#pragma HLS loop_tripcount max=1488
            words_in.read(inval);
            last_word = inval.last;

            // Shift words in "big endian" order
            tmpword.data = (tmpword.data << (sizeof(inval.data) * 8)) | ap_uint<sizeof(tmpword.data) * 8>(inval.data);
            tmpword.strb = (tmpword.strb << sizeof(inval.data)) | ap_uint<sizeof(tmpword.data)>(inval.strb);

            if (i % sizeof(tmpword.data) == sizeof(tmpword.data) - 1 || last_word) {
                // If last_word == true, did we shift everything to the correct position already?
                tmpword.last = last_word;
                words_out.write(tmpword);
            }
        }
    }
    else if (sizeof(TIn::data) > sizeof(TOut::data))
    {
        TIn inval;
        ap_uint<sizeof(inval.data) * 8> tmpword;
        ap_uint<sizeof(inval.data)> tmpstrb;
        TOut outval;
        for (;;) {
            words_in.read(inval);
            tmpword = inval.data;
            tmpstrb = inval.strb;
            for (int i = 0; i < sizeof(tmpword) / sizeof(outval.data); i++) {

                // shift words out in "big endian" order
                outval.data = ap_uint<sizeof(outval.data) * 8>(tmpword >> ((sizeof(tmpword) / sizeof(outval.data) - 1) * sizeof(outval.data) * 8));
                outval.strb = ap_uint<sizeof(outval.data)>(tmpstrb >> ((sizeof(tmpword) / sizeof(outval.data) - 1) * sizeof(outval.data)));
                tmpword <<= sizeof(outval.data) * 8;
                tmpstrb <<= sizeof(outval.data);
                outval.last = inval.last && tmpstrb == ap_uint<sizeof(tmpword)>(0);

                words_out.write(outval);

                if (outval.last)
                    break;
            }

            if (inval.last)
                break;
        }
    }
}

#endif // __MF_STREAM_H__
