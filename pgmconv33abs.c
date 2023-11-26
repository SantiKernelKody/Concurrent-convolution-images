//==============================================================================
//
// pgmconv33abs.c
//
// use:
// pgmconv33abs convmatr.txt  file.pgm  result.pgm
//
//==============================================================================

#include <stdio.h>
#include "pgm.h"
extern pthread_mutex_t mutex;

//=======================================================================================
int FrameConv3x3(byte *pInp, byte *pOut, int nW, int nH, int conv[9], int denom)
{
	int i, j;
	int i1m, i1p;
	int j1m, j1p;
	int res;
	int denom2 = (denom + 1) / 2;

	// for(i=0; i<9; i++) printf("%d ", conv[i]);
	// printf("\n");
	// printf("%d  %d", nW, nH);

	for (i = 1; i < (nH - 1); i++) // rows
	{
		i1m = i - 1;
		i1p = i + 1;
		if (i1m == (-1))
			i1m = 0;
		if (i1p == nH)
			i1p = (nH - 1);
		for (j = 1; j < (nW - 1); j++)
		{
			j1m = j - 1;
			j1p = j + 1;
			if (j1m == (-1))
				j1m = 0;
			if (j1p == nW)
				j1p = (nW - 1);

			res = conv[0] * pInp[j1m + nW * i1m] + conv[1] * pInp[j + nW * i1m] + conv[2] * pInp[j1p + nW * i1m];
			res += conv[3] * pInp[j1m + nW * i] + conv[4] * pInp[j + nW * i] + conv[5] * pInp[j1p + nW * i];
			res += conv[6] * pInp[j1m + nW * i1p] + conv[7] * pInp[j + nW * i1p] + conv[8] * pInp[j1p + nW * i1p];
			res = (res + denom2) / denom;

			res = abs(res);

			if (res > 255)
				res = 255;
			// pthread_mutex_lock(&mutex);
			pOut[j + nW * i] = res;
			// pthread_mutex_unlock(&mutex);
		}
	}
	return 0;
}
void sharp_matrix(int coef[], int n)
{
	// firts row
	coef[0] = 0;
	coef[1] = -1;
	coef[2] = 0;
	// second row
	coef[3] = -1;
	coef[4] = 5;
	coef[5] = -1;
	// third row
	coef[6] = 0;
	coef[7] = -1;
	coef[8] = 0;
}
void top_sobel_matrix(int coef[], int n)
{
	// firts row
	coef[0] = 1;
	coef[1] = 2;
	coef[2] = 1;
	// second row
	coef[3] = 0;
	coef[4] = 0;
	coef[5] = 0;
	// third row
	coef[6] = -1;
	coef[7] = -2;
	coef[8] = -1;
}
void blur_matrix(int coef[], int n)
{
	// firts row
	coef[0] = 0;
	coef[1] = 0;
	coef[2] = 0;
	// second row
	coef[3] = -1;
	coef[4] = 1;
	coef[5] = -1;
	// third row
	coef[6] = 0;
	coef[7] = 0;
	coef[8] = 0;
}

//=======================================================================================
/*
int main(int nArg, char** ppArg)
{
	int i, j;
	int nW, nH; // image width and height
	int nBr;    // image brightness
	byte* pIm;  // pointer to image matrix
	byte* pIm2; // pointer to result image matrix
	int nSize;  // image size in bytes
	int nX;
	FILE* pF;
	int coef[9]; // convolution's coefficients
	int coef_n=sizeof(coef)/sizeof(int);
	int nSum = 0;

	if(nArg<3)
	{
		puts("2-Dim convolution filter with convolution vector 3x3");
		puts("pgmconv33abs matr.txt  file.pgm result.pgm");
		exit(1);
	}

	pF = fopen(ppArg[1], "r");
	printf("matrix: ");
	for(i=0; i<9; i++)
	{
		fscanf(pF, "%d", &coef[i]);
		nSum = nSum + coef[i];
		printf("%d ", coef[i]);
	}
	printf("\n");

	printf("NUEVA MATRIZ");
	blur_matrix(coef,coef_n);
	for(i=0; i<9; i++)
	{
		printf("%d ", coef[i]);
	}

	if(nSum == 0) nSum = 1;

	ReadPGM(ppArg[2], &pIm, &nW, &nH);

	nSize = nW * nH;
	pIm2 = (byte*) malloc(nSize);

	FrameConv3x3(pIm, pIm2, nW, nH, coef, nSum);

	WritePGM(ppArg[3], pIm2, nW, nH);

	free(pIm2);
	free(pIm);

	return 0;
}
*/