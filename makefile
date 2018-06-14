wavegen: waveformGenerator.c waveImage.c
	gcc -o wavegen waveformGenerator.c waveImage.c -I. -lm -lgd

