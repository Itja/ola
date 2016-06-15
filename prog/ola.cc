#include	<cstdio>
#include	<cstring>
#include	<cstdlib>
#include	<cmath>

#include	<sndfile.hh>

#define		STRETCH			1.0 //Stretching Factor alpha
//#define		FRAME_SIZE		1024 //N
//#define		A_HOP_SIZE		512 //Analysis Hop Size Ha
#define		FRAME_SIZE		1024 //N
#define		A_HOP_SIZE		512 //Analysis Hop Size Ha

//worst results, best performance with the one-function!
static float w_one(int n)
{
	return 1.0;
}

static float w_hamming(int n)
{
	return 0.54 - 0.46 * cos((2*M_PI*n) / (FRAME_SIZE-1));
}

static float w_hann(int n)
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


	float (*window)(int) = w_hann;
	double sf = STRETCH; //stretching factor alpha
	int framesize = FRAME_SIZE;
	int a_hs = A_HOP_SIZE;
	int s_hs = a_hs * sf;
	int ringsize = framesize / s_hs + 1;

	printf("frame size: %d\nAnalysis Hop Size Ha: %d\nstretch: %f\nSynthesis Hop Size Hs: %d\nring size: %d\n", framesize, a_hs, s_hs, ringsize, sf);

	float** frame = (float**) calloc(sizeof(float*), ringsize);
	float** frame_w = (float**) calloc(sizeof(float*), ringsize);
	float** out = (float**) calloc(sizeof(float*), ringsize);

	for (int r = 0; r<ringsize; r++) {
		frame[r] = (float*) calloc(sizeof(float), framesize);
		frame_w[r] = (float*) calloc(sizeof(float), framesize);
		out[r] = (float*) calloc(sizeof(float), s_hs);
	}

	printf("Init: Reading frame 0\n");
	fileIn.read(frame[0], framesize);
	//init: copy first frame onto out
	for (int n = 0; n < framesize; n++) { 
		out[n / s_hs][n % s_hs] = window(n) * frame[0][n];
	}
	fileOut.write(out[0], s_hs);

	printf("Working on the rest of the input..\n");

	for (int x = 1; ; x++) {
		int i = x % ringsize;
		int i_before = (x-1) % ringsize;
		//read each following frame from file
		//printf("Reading frame %d to slot %d\n", x, i);
		memcpy(frame[i], &frame[i_before][framesize - a_hs], a_hs * sizeof(float));
		int rsize = fileIn.read(&frame[i][framesize - a_hs], framesize - a_hs);
		if (!rsize) break;
		for (int n = 0; n < framesize; n++) {
			frame_w[i][n] = window(n) * frame[i][n];
		}

		for (int n = 0; n < framesize; n++) {
			float wn = window(n);
			out[(x + n / s_hs) % ringsize][n % s_hs] += (wn * frame_w[i][n]) / (wn*wn);
		}

		printf("%d: First values of input: %f %f %f %f\n", x, frame[i][0], frame[i][1], frame[i][2], frame[i][3]);
		printf("%d: First values of windowed: %f %f %f %f\n", x, frame_w[i][0], frame_w[i][1], frame_w[i][2], frame_w[i][3]);
		printf("%d: writing %x (%f, %f, ...) size %d to file\n", x, out[i], out[i][0], out[i][1], s_hs);
		printf("%d: First values of output: %f %f %f %f\n", x, out[i][0], out[i][1], out[i][2], out[i][3]);

		int numWritten = fileOut.write((const float*) out[i], s_hs);
		if (numWritten != s_hs) {
			printf("Written entities: %d, Error: %s\n", numWritten, fileOut.strError());
			break;
		}
		for (int j = 0; j < s_hs; j++) {
			out[i][j] = 0.0f;
		}
	}

	printf("Done.\n");

	printf("writeSync()\n");
	fileOut.writeSync();

	puts("");

} 

int main(void)
{	
	const char * fname = "roh.wav";

	puts("\nReading file\n");

	act_file(fname, "out.wav");

	puts("Done.\n");
	return 0;
} /* main */


