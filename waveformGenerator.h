/*
 * waveformGenerator.h
 *
 *  Created on: 22 May 2010
 *  Last modified: 25 April 2012
 *  Author: Matthew Page
 */

#ifndef WAVEFORMGENERATOR_H_
#define WAVEFORMGENERATOR_H_

#define VERSION 	1.41


struct wg_config
{
	char* inputFile;
	char* outputPath;
	char* baseFileName;
	char* startTime;

	int sampleRate;
	int channels;
	int waitTime;
	int timesToCheck;

	long fileStartOffset;

	int peaksPerSecond;
	int secondsPerFile;
	
	int imageHeight;
}
typedef wg_config;
wg_config gConfig;

/*
 * structure to hold peak data
 */
struct peaks_t
{
	short high;
	short low;
}
typedef peaks_t;

void debug(int , const char * , ...);

FILE* open_pcm(char *filename);
void seek_pcm(FILE *fp, long pos);
void close_pcm(FILE *fp);

#endif /* WAVEFORMGENERATOR_H_ */
