/*
 * waveformGenerator.c
 *
 *  Created on: 13 May 2010
 *  Last modified: 25 April 2012
 *  Author: Matthew Page
 */
#include	<unistd.h>
#include	<stdlib.h>
#include	<stdio.h>
#include	<stdbool.h>
#include 	<stdarg.h>
#include	<syslog.h>

#include	"waveformGenerator.h"
#include	"waveImage.h"

int debugLevel = LOG_DEBUG+1;
float peakCoEfficient;

static void defaultConfig()
{
	gConfig.inputFile		= "";
	gConfig.outputPath 		= "";
	gConfig.baseFileName 	= "waveform_";
	gConfig.startTime 		= "00:00:00";
	gConfig.peaksPerSecond 	= 16;
	gConfig.secondsPerFile	= 300;
	gConfig.sampleRate		= 44100;
	gConfig.channels		= 1;
	gConfig.fileStartOffset = 0;
	gConfig.waitTime		= 1;
	gConfig.timesToCheck	= 5;
	gConfig.imageHeight		= 175;
}

static int readArguments(int argc, char** argv)
{
	int opts;
	for (opts = 1; opts < argc; opts++) {

		if (argv[opts][0] == '-') {
			switch (argv[opts][1]) {
			case 'i':
				gConfig.inputFile = argv[++opts];
				break;
			case 'o':
				gConfig.outputPath = argv[++opts];
				break;
			case 'f':
				gConfig.baseFileName = argv[++opts];
				break;
			case 'r':
				gConfig.sampleRate = atoi(argv[++opts]);
				break;
			case 'p':
				gConfig.peaksPerSecond = atoi(argv[++opts]);
				break;
			case 's':
				gConfig.fileStartOffset = atol(argv[++opts]);
				break;
			case 'c':
				gConfig.channels = atoi(argv[++opts]);
				break;
			case 'w':
				gConfig.waitTime = atoi(argv[++opts]);
				break;
			case 'n':
				gConfig.timesToCheck = atoi(argv[++opts]);
				break;
			case 't':
				gConfig.startTime = argv[++opts];
				break;
			case 'l':
				gConfig.secondsPerFile = atoi(argv[++opts]);
				break;
			case 'h':
				gConfig.imageHeight = atoi(argv[++opts]);
				break;
			}
		}
	}

	if (gConfig.inputFile == NULL)
	{
		debug(LOG_ALERT, "You must pass an input file. Use -i <input file>");
		return 1;
	}

	if (gConfig.outputPath == NULL)
	{
		debug(LOG_ALERT, "You must pass an output file. Use -o <output file>");
		return 1;
	}

	switch (gConfig.channels)
	{
		case 2:
			gConfig.channels = 2;
			break;
		default:
			gConfig.channels = 1;
			break;
	}

	switch (gConfig.sampleRate)
	{
		case 48000:
			gConfig.sampleRate = 48000;
			break;
		default:
			gConfig.sampleRate = 44100;
			break;
	}

	peakCoEfficient = (float) gConfig.peaksPerSecond / (float) gConfig.sampleRate;

	debug(LOG_INFO, "output path: %s", gConfig.outputPath);
	debug(LOG_INFO, "input file: %s", gConfig.inputFile);
	debug(LOG_INFO, "base file name: %s", gConfig.baseFileName);
	debug(LOG_INFO, "file offset: %d", gConfig.fileStartOffset);
	debug(LOG_INFO, "sample rate: %d", gConfig.sampleRate);
	debug(LOG_INFO, "peaks per second: %d", gConfig.peaksPerSecond);
	debug(LOG_INFO, "channels: %d", gConfig.channels);
	debug(LOG_INFO, "wait time: %d", gConfig.waitTime);
	debug(LOG_INFO, "start time: %s", gConfig.startTime);
	debug(LOG_INFO, "seconds per file: %d", gConfig.secondsPerFile);
	debug(LOG_INFO, "image height: %d", gConfig.imageHeight);

	return 0;
}

/*
 * Main Function
 */
int main(int argc, char** argv) {
	fprintf(stderr, "waveformGenerator v%02.02f (c)Matthew Page, 2010-12\n", VERSION);

	defaultConfig();

	int result = readArguments(argc, argv);
	if (result != 0)
	{
		// didn't get all the details we need so exiting
		debug(LOG_ALERT, "Exiting due to lack of information.");
		return 1;
	}

	initialiseTime(gConfig.startTime);

	/*
	 * Initialise sample variables
	 */
	short sampleBuffer[16384];
	int samplesRead = 0;
	int sampleCount = 0;
	short currentSample = 0;
	short currentChannel = 0;
	peaks_t peaks[2];

	/*
	 * Initialise file access
	 */
	int checkForDataCount = 0;
	bool continueReading = true;
	bool reachedEndOfFile = false;
	long filePosition = 0;
	FILE *fpi;
	
	fpi = open_pcm(gConfig.inputFile);
	seek_pcm(fpi, gConfig.fileStartOffset);

	startImageFile();

	/*
	 * Read input file and generate peaks
	 */
	while (continueReading == true)
	{
		for (checkForDataCount = 0; checkForDataCount < gConfig.timesToCheck; checkForDataCount++)
		{
			samplesRead = fread(sampleBuffer, 2, sizeof(sampleBuffer) / 2, fpi);
			if (ferror(fpi))
			{
				debug(LOG_ALERT, "Error reading from file");
			}
			
			if (samplesRead == 0) 
			{
				// wait for a while to see if the file is growing
				updateImageFile(false);
				
				debug(LOG_DEBUG, "Waiting to see if file is still growing #%d", checkForDataCount);
				filePosition = ftell(fpi);
				sleep(gConfig.waitTime);
				
				close_pcm(fpi);
				fpi = open_pcm(gConfig.inputFile);
				seek_pcm(fpi, filePosition);
			}
			else
			{
				if (checkForDataCount > 0)
				{
					debug(LOG_DEBUG, "File is still growing...");
				}
				break;
			}
		}

		if (samplesRead > 0)
		{
			int i;
			for (i = 0; i < samplesRead; i++) {
				currentSample = sampleBuffer[i];
				if (peaks[currentChannel].high > currentSample)
					peaks[currentChannel].high = currentSample;
				if (peaks[currentChannel].low < currentSample)
					peaks[currentChannel].low = currentSample;

				if ((sampleCount * peakCoEfficient) >= 1) {

					if (gConfig.channels == 2)
					{
						drawStereoPeak(peaks[0], peaks[1]);
					}
					else
					{
						drawMonoPeak(peaks[0]);
					}

					peaks[0].low = 0;
					peaks[0].high = 0;
					peaks[1].low = 0;
					peaks[1].high = 0;

					sampleCount = 0;
				}

				if (gConfig.channels == 2)
				{
					if (currentChannel == 0)
					{
						currentChannel = 1;
					}
					else
					{
						sampleCount ++;
						currentChannel = 0;
					}
				}
				else
				{
					sampleCount ++;
				}
			}
		}
		else
		{
			// we've been here before so the file has stopped growing, so give up and finish
			continueReading = false;
			debug(LOG_DEBUG, "File has stopped growing");
		}
	}


	endImageFile();
	close_pcm(fpi);

	debug(LOG_DEBUG, "Complete");

	return 0;
}

FILE* open_pcm(char *filename)
{
	FILE *fp;
	
	int i;
	for (i=0; i < 10; i++)
	{
		fp = fopen(filename, "rb");
		if (fp == NULL)
		{
			debug(LOG_DEBUG, "Trying to open file. Attempt #%d", i);
			sleep(2);
		}
		else
		{
			break;
		}
	}

	if (fp == NULL)
	{
		debug(LOG_ALERT, "Can't open input file %s", filename);
		exit(1);
	}

	return fp;
}

void seek_pcm(FILE *fp, long pos)
{
	fseek(fp, pos, 0);
	if (ferror(fp))
	{
		debug(LOG_ALERT, "Error seeking within file");
	}
}

void close_pcm(FILE *fp)
{
	fclose(fp);
}

/*
 * debug
 */
void debug(int level, const char * template, ...)
{
	va_list ap;

	va_start (ap, template);
	if (debugLevel < level) return;

	vfprintf(stderr, template, ap);
	fprintf(stderr,"\n");
	fflush(stderr);

	va_end (ap);
	return;
}
