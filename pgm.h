//=======================================================================================
//
// pgm.h
//
// rear & write functions for pgm files
//
// developer: Henry Guennadi Levkin
//
//=======================================================================================

#include <stdio.h>
#include <stdlib.h>

//=======================================================================================
typedef unsigned char byte;

//=======================================================================================
int ReadPGM(char* file_name, byte** ppImg, int* pnWidth, int* pnHeight)
{
 char buf[512];

 FILE* pInpFile = fopen(file_name,"rb");
 if( pInpFile==NULL) return 1;
 
 fgets(buf,510,pInpFile); // "P5"
 fgets(buf,510,pInpFile); // "#comments"

 fgets(buf,510,pInpFile); // "width height"
 sscanf(buf,"%d %d", pnWidth, pnHeight);
 printf("width=%d height=%d\n", *pnWidth, *pnHeight);

 fgets(buf,510,pInpFile); // "MaxColor"

 *ppImg = malloc((*pnWidth) * (*pnHeight));
 
 fread(*ppImg, 1, (*pnWidth) * (*pnHeight), pInpFile);

 fclose(pInpFile);

 return 0;
}

//=======================================================================================
void WritePGM(char* file_name, byte* pImg, int nWidth, int nHeight)
{
    FILE* img_file= fopen(file_name, "wb");

    fprintf( img_file, "P5\n#\n%d %d\n255\n", nWidth, nHeight );

    fwrite( pImg, 1, nWidth * nHeight, img_file);
    fclose(img_file);
}


