#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TRUE  1
#define FALSE 0

#define BAD_NUMBER_ARGS 1
#define FSEEK_ERROR 2
#define FREAD_ERROR 3
#define MALLOC_ERROR 4
#define FWRITE_ERROR 5

FILE *parseCommandLine(int argc, char **argv, int *isGrayscale) {
  if (argc > 2) {
    printf("Usage: %s [-g]\n", argv[0]);
    exit(BAD_NUMBER_ARGS);
  }
  
  if (argc == 2 && strcmp(argv[1], "-g") == 0) {
    *isGrayscale = TRUE;
  } else {
    *isGrayscale = FALSE;
  }

  return stdin;
}

unsigned getFileSizeInBytes(FILE* stream) {
  unsigned fileSizeInBytes = 0;
  
  rewind(stream);
  if (fseek(stream, 0L, SEEK_END) != 0) {
    exit(FSEEK_ERROR);
  }
  fileSizeInBytes = ftell(stream);

  return fileSizeInBytes;
}

void getBmpFileAsBytes(unsigned char* ptr, unsigned fileSizeInBytes, FILE* stream) {
  rewind(stream);
  if (fread(ptr, fileSizeInBytes, 1, stream) != 1) {
#ifdef DEBUG
    printf("feof() = %x\n", feof(stream));
    printf("ferror() = %x\n", ferror(stream));
#endif
    exit(FREAD_ERROR);
  }
}

unsigned char getAverageIntensity(unsigned char blue, unsigned char green, unsigned char red) {
  unsigned char avg = (red + green + blue) / 3;
	return avg;
}

void applyGrayscaleToPixel(unsigned char* pixel) {
  unsigned char* blue = pixel;
	unsigned char* green = pixel + 1;
	unsigned char* red = pixel + 2;
	unsigned char avg = getAverageIntensity(*blue, *green, *red);

	*blue = avg;
	*green = avg;
	*red = avg;
}

void applyThresholdToPixel(unsigned char* pixel) {
  unsigned char* blue = pixel;
	unsigned char* green = pixel + 1;
	unsigned char* red = pixel + 2;
	unsigned char avg = getAverageIntensity(*blue, *green, *red);

	if (avg >= 128){
		*blue = 0xff;
		*green = 0xff;
		*red = 0xff;
	}
	else{
		*blue = 0x00;
		*green = 0x00;
		*red = 0x00;
	}
}

void applyFilterToPixel(unsigned char* pixel, int isGrayscale) {
  if (isGrayscale){
		applyGrayscaleToPixel(pixel);
	}
	else{
		applyThresholdToPixel(pixel);
	}
}

void applyFilterToRow(unsigned char* row, int width, int isGrayscale) {
  for (int i = 0; i < width; i++){
		applyFilterToPixel(row + (i * 3), isGrayscale);
	}
}

void applyFilterToPixelArray(unsigned char* pixelArray, int width, int height, int isGrayscale) {
  int padding = 0;
	int wholeRowWidth = width * 3; //3 bytes per pixel
	padding = wholeRowWidth % 4; //how much space is left over after all pixel values
	if (padding != 0){
		padding = 4 - padding; //if 3 pixels are left over, add 1 byte of padding
	}
	wholeRowWidth += padding; //add it to space for pixels to get the space taken by one row
	/*
	for (int i = 0; i < wholeRowWidth * height; i += wholeRowWidth){
		applyFilterToRow(pixelArray + i, width, isGrayscale);
	}*/

	for (int i = 0; i < height; i++){
		applyFilterToRow(pixelArray, width, isGrayscale);
		pixelArray += wholeRowWidth;
	}

#ifdef DEBUG
	printf("wholeRowWidth = %d\n", wholeRowWidth);
  printf("padding = %d\n", padding);
#endif
}

void parseHeaderAndApplyFilter(unsigned char* bmpFileAsBytes, int isGrayscale) {
  int offsetFirstBytePixelArray = 0;
  int width = 0;
  int height = 0;
  unsigned char* pixelArray = NULL;

  offsetFirstBytePixelArray = *((int *)(bmpFileAsBytes + 10));
  width = *((int *)(bmpFileAsBytes + 18));
  height = *((int *)(bmpFileAsBytes + 22));
  pixelArray = bmpFileAsBytes + offsetFirstBytePixelArray;

#ifdef DEBUG
  printf("offsetFirstBytePixelArray = %u\n", offsetFirstBytePixelArray);
  printf("width = %u\n", width);
  printf("height = %u\n", height);
  printf("pixelArray = %p\n", pixelArray);
#endif

  applyFilterToPixelArray(pixelArray, width, height, isGrayscale);
}

int main(int argc, char **argv) {
  int grayscale = FALSE;
  unsigned fileSizeInBytes = 0;
  unsigned char* bmpFileAsBytes = NULL;
  FILE *stream = NULL;
  
  stream = parseCommandLine(argc, argv, &grayscale);
  fileSizeInBytes = getFileSizeInBytes(stream);

#ifdef DEBUG
  printf("fileSizeInBytes = %u\n", fileSizeInBytes);
#endif

  bmpFileAsBytes = (unsigned char *)malloc(fileSizeInBytes);
  if (bmpFileAsBytes == NULL) {
    exit(MALLOC_ERROR);
  }
  getBmpFileAsBytes(bmpFileAsBytes, fileSizeInBytes, stream);

  parseHeaderAndApplyFilter(bmpFileAsBytes, grayscale);

#ifndef DEBUG
  if (fwrite(bmpFileAsBytes, fileSizeInBytes, 1, stdout) != 1) {
    exit(FWRITE_ERROR);
  }
#endif

  free(bmpFileAsBytes);
  return 0;
}
