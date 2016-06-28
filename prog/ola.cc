#include	<cstdio>
#include	<cstring>
#include	<cstdlib>
#include	<cmath>

#include	<sndfile.hh>

#define		STRETCH			1.006511 //Stretching Factor alpha
//#define		FRAME_SIZE		1024 //N
//#define		A_HOP_SIZE		512 //Analysis Hop Size Ha
#define		FRAME_SIZE		1024 //N
#define		A_HOP_SIZE		512 //Analysis Hop Size Ha
#define		WINDOW			w_hamming
//#define DEBUG

//worst results, best performance with the one-function!
static float w_one(int n)
{
	return 1.0;
}

static float w_hamming(int n)
{
	return 0.54 - 0.46 * cos((2*M_PI*n) / (FRAME_SIZE-1));
}

static float w_hann(int n) //artifacts in output due to w_hann(0) == 0
{
	return 0.5 * (1 - cos((2*M_PI*n) / (FRAME_SIZE-1)));
}

static void act_file(const char *fnameIn, const char* fnameOut)
{	
	SndfileHandle fileIn = SndfileHandle(fnameIn);
	int channels = 1;
	int srate = 48000;
	SndfileHandle fileOut = SndfileHandle(fnameOut, SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_PCM_16, channels, srate);

	printf("Opened file '%s'\n", fnameIn);
	printf("    Sample rate : %d\n", fileIn.samplerate());
	printf("    Channels    : %d\n", fileIn.channels());


	float (*window)(int) = WINDOW;
	double sf = STRETCH; //stretching factor alpha
	int framesize = FRAME_SIZE;
	int a_hs = A_HOP_SIZE;
	int s_hs = a_hs * sf;
	int ringsize = framesize / s_hs + 1;

	printf("frame size: %d\nAnalysis Hop Size Ha: %d\nstretch: %f\nSynthesis Hop Size Hs: %d\nring size: %d\n", framesize, a_hs, s_hs, ringsize, sf);

	float** frame = (float**) calloc(sizeof(float*), ringsize);
	float** frame_w = (float**) calloc(sizeof(float*), ringsize);

	for (int r = 0; r<ringsize; r++) {
		frame[r] = (float*) calloc(sizeof(float), framesize);
		frame_w[r] = (float*) calloc(sizeof(float), framesize);
	}

	int outsize = framesize * ringsize * 2;
	float* out = (float*) calloc(sizeof(float*), outsize);
	int c_out = 0; //out cursor
	int cw_out = 0; //out written cursor

	printf("Init: Reading frame 0\n");
	fileIn.read(frame[0], framesize);
	//init: copy first frame onto out
	for (int n = 0; n < framesize; n++) { 
		out[n] = window(n) * frame[0][n];
	}
	cw_out = 0;
	c_out = framesize;
	printf("c_out set to %d\n", c_out);
	//fileOut.write(out, framesize - s_hs);
	//
	printf("Working on the rest of the input..\n");

	for (int x = 1; ; x++) {
		int i = x % ringsize;
		int i_before = (x-1) % ringsize;
		//read each following frame from file
		//printf("Reading frame %d to slot %d\n", x, i);
		memcpy(frame[i], &frame[i_before][framesize - a_hs], a_hs * sizeof(float));
#ifdef DEBUG
		printf("\nCopied %d from frame[%d][%d] to frame[%d][0], read %d to frame[%d][%d]\n", a_hs, i_before, framesize - a_hs, i, framesize - a_hs, i, a_hs);
#endif

		int rsize = fileIn.read(&frame[i][a_hs], framesize - a_hs);
		if (!rsize) break;
		for (int n = 0; n < framesize; n++) {
			frame_w[i][n] = window(n) * frame[i][n];
		}

		for (int n = 0; n < framesize; n++) {
			float wn = window(n);
			float cur_value = (wn * frame_w[i][n]) / (wn*wn);
			if (n < 1 || n > (framesize - 2)) {
				printf("[%d]=%f (w=%f)\n", n, cur_value, wn);
			}
			int oi = (c_out - s_hs + n) % outsize;
			if (n < s_hs) {
				out[oi] += cur_value;
			} else {
				out[oi] = cur_value;
			}
		}


		int new_cw_out = c_out - s_hs;
#ifdef DEBUG
		printf("cw_out = %d\n", cw_out);
		printf("new_cw_out = %d = %d - %d\n", new_cw_out, c_out, s_hs);
#endif
		c_out += framesize - s_hs;
#ifdef DEBUG
		printf("c_out advanced to %d\n", c_out);
#endif

		if ((new_cw_out / outsize) != (cw_out / outsize)) { //ring buffer reading is not trivial..
			printf("differ: %d != %d\n", new_cw_out / outsize, cw_out / outsize);
			//out[cw_out % outsize] = -1.0f;
			fileOut.write((const float*) (&out[cw_out % outsize]), outsize - (cw_out % outsize));
			cw_out += outsize - (cw_out % outsize); //shift to next full outsize
		}
		int bytesToWrite = new_cw_out - cw_out;
		if (bytesToWrite > 0) {
#ifdef DEBUG
			printf("bytesToWrite = %d = %d - %d (pos: %x)\n", bytesToWrite, new_cw_out, cw_out, &out[cw_out % outsize]);
#endif
			//out[cw_out % outsize] = 1.0f;
			int numWritten = fileOut.write((const float*) (&out[cw_out % outsize]), bytesToWrite);
			if (numWritten != bytesToWrite) {
				printf("Written entities: %d, Error: %s\n", numWritten, fileOut.strError());
				break;
			}
		}
		cw_out = new_cw_out;


#ifdef DEBUG
		printf("%d: First values of input: %f %f %f %f\n", x, frame[i][0], frame[i][1], frame[i][2], frame[i][3]);
		printf("%d: First values of windowed: %f %f %f %f\n", x, frame_w[i][0], frame_w[i][1], frame_w[i][2], frame_w[i][3]);
#endif

	}
	//TODO: last write

	printf("Done.\n");

	printf("writeSync()\n");
	fileOut.writeSync();

	puts("");

} 

int main(void)
{	
	const char * fname = "in/fc.wav";

	puts("\nReading file\n");

	act_file(fname, "out.wav");

	puts("Done.\n");
	return 0;
} /* main */


