#include	<cstdio>
#include	<cstring>

#include	<sndfile.hh>

#define		BUFFER_LEN		1024

static void act_file(const char *fnameIn, const char* fnameOut)
{	
	static float buffer[BUFFER_LEN];

	SndfileHandle fileIn = SndfileHandle(fnameIn);

	//create_file(fname, SF_FORMAT_WAV | SF_FORMAT_PCM_16);
	int channels = 2;
	int srate = 48000;
	SndfileHandle fileOut = SndfileHandle(fnameOut, SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_PCM_24, channels, srate);

	printf("Opened file '%s'\n", fnameIn);
	printf("    Sample rate : %d\n", fileIn.samplerate());
	printf("    Channels    : %d\n", fileIn.channels());

	int readlen;
	while(readlen = fileIn.read(buffer, BUFFER_LEN)) 
	{
			for (int i=0; i<readlen; i++) {
				//TODO: do something
				//printf(" %f", buffer[i]);
			}

			printf("Len: %d\n", readlen);

			fileOut.write(buffer, readlen);
	}

	printf("writeSync()");
	fileOut.writeSync();

	puts("");

} 

int main(void)
{	
	const char * fname = "sine.wav";

	puts("\nReading file\n");

	act_file(fname, "out.wav");

	puts("Done.\n");
	return 0;
} /* main */


