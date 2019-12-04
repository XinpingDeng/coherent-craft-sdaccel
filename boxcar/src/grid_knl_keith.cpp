const int SAMP_PER_BURST = 32;
const int NPIX = 256;
const int NT = 256;
const int NBOX = 16;
const int NDM = 1024;
const int MAX_CAND = 1024;


typedef int8_t boxcar_t;
typedef struct _fftout {
  boxcar_t data[SAMP_PER_BURST];
} fft_out;

typedef struct _cand {
  uint8_t t; // sample number where max S/N occurs
  boxcar_t sn; // S/N
  uint16_t dm; // DM index
  uint8_t x;
  uint8_t y;
  uint8_t w;
} cand_t;

const boxcar_t threshold = boxcar_t(10);


void read(hls::stream<fft_out> stream_in, boxcar_t in[NPIX][NPIX]) {
  for (int x = 0; x < NPIX; x++) {
    for (int y = 0; y < NPIX/SAMP_PER_BURST; y++) {
#pragma HLS PIPELINE II=1
      fft_out d = stream_in.read();
      for (int yi = 0; yi < SAMP_PER_BURST; y++) {
#pragma HLS UNROLL
	in[x][y*SAMP_PER_BURST + yi] = d[yi];
      }
    }
  }
}

void boxcar(boxcar_t in[NPIX][NPIX], boxcar_t out[NPIX][NPIX][NBOX]) {

  static boxcar_t history[NPIX][NPIX][NBOX];
  // needs to complete to turn it into shift registers
#pragma HLS array_parition history complete

  for (int x = 0; x < NPIX; x++) {
#pragma HLS PIPELINE II=NPIX/SAMP_PER_BURST

    for (int y = 0; y < NPIX; y++) {
#pragma HLS UNROLL factor=NPIX/SAMP_PER_BURST/NBOX

      boxcar_t b = in[x][y];
      for (int w = 0; w < NBOX; w++) { // Is b a carried dependance?
#pragma HLS UNROLL
	out[x][y][w] = b;
	b += history[x][y][w];
      }

      // Shift data along in the FIFO
      for (int w = 1; w < NBOX; w++) {
	history[x][y][w] = history[x][y][w-1];
      }
      // put new data in the pipeline
      history[x][y][0] = b;
    }
  }
}

void threshold(int idm, int t, boxcar_t out[NPIX][NPIX][NBOX], cand_t cands[MAXCAND], int& ncand) {
  volatile int myncand = 0;
  for(int x = 0; x < NPIX; x++) {
    for (int y = 0; y < NPIX; y++) {
#pragma pipeline II=1
      for(int w = 0; w < NBOX; w++) {
	if (out[x][y][w] > thresh) {
	  cand_t c;
	  c.t = t;
	  c.idm = idm;
	  c.sn = out[x][y][w];
	  c.x = x;
	  c.y = y;
	  c.w = w;
	  cands[myncand] = c;
	  myncand++;
	}
      }
    }
  }

  ncand = myncand;
}



void xinping_boxcar_and_threshold(hls::stream<fft_out> stream_in,
				  cand_t* out) {
  {

    // Output of Alex's FFT will be an hls::stream 512 bits wide I think.
    // Which will send 32 numbers per clock
    // Rastering throught the output image for a single image until it gets to the next image time
    // Then the next DM.

    for (int idm = 0; idm < NDM; idm++) {
      for (int t = 0; t < NT; t++) {

	// THisis one of the canonical forms for dataflow
#pragma HLS dataflow

	// HLS should turn the in and out arrays into FIFOs (rather than PIPOs) as the access is sequential.
	// Because the ordering is TXY - we need to store a hold image before we can boxcar
	// We partition it by 32 so we can pipeline it.
	boxcar_t in[NPIX][NPIX]; 
#pragma HLS array_partition in factor=32 cycle

	// Output has NBOX
	boxcar_t out[NPIX][NPIX][NBOX];
#pragma HLS array_partition out factor=32 cycle


	// We might need partition cands - I haven't throught about this much.
	cand_t cands[MAXCAND]; 
	int ncand; // Maybe we
	read(stream_in, in);
	boxcar(in, out);
	threshold(t, idm, out, cand, &ncand);
	write(cand, ncand);
      }
    }

  }
