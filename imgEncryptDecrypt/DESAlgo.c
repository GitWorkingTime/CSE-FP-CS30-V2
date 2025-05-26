#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int PC1[] = { 57,   49,    41,   33,    25,    17,    9,
               1,   58,    50,   42,    34,    26,   18,
              10,    2,    59,   51,    43,    35,   27,
              19,   11,     3,   60,    52,    44,   36,
              63,   55,    47,   39,    31,    23,   15,
               7,   62,    54,   46,    38,    30,   22,
              14,    6,    61,   53,    45,    37,   29,
              21,   13,     5,   28,    20,    12,    4};

int leftShifts[] = {1,   1,   2,   2,
					2,   2,   2,   2,
					1,   2,   2,   2,
					2,   2,   2,   2,
					1};

void hexToBin(long hex){
	/*
	Conversion Table:
		0 - 0000
		1 - 0001
		2 - 0010
		3 - 0011
		4 - 0100
		5 - 0101
		6 - 0110
		7 - 0111
		8 - 1000
		9 - 1001
		A - 1010
		B - 1011
		C - 1100
		D - 1101
		E - 1110
		F - 1111
	*/
}

//testing around
void addingStrings(){
	int one = 10;
	char str1[100];
	sprintf(str1, "%d", one);
	printf("%s\n", str1);

	int two = 23;
	char str2[100];
	sprintf(str2, "%d", two);
	printf("%s\n", str2);
	strcat(str1, str2);
	printf("str1 after strcat: %s\n", str1);	
}

void extractingBit(){
	unsigned long num = 0xa9b; // 1010 1001 1011
	unsigned int b = (12 - 2);
	unsigned int z = (num >> b) & 1;
	printf("z: %d \n", z);

	long k = 0x133457799BBCDFF1; // 00010011 00110100 01010111 01111001 10011011 10111100 11011111 11110001
	unsigned int bitPos = (64 - 49);
	unsigned long extractedBit = (k >> bitPos) & 1;
	printf("extractedBit: %ld \n", extractedBit);

}

int main(){
	//Message:
	long m = 0x0123456789ABCDEF; // 0000 0001 0010 0011 0100 0101 0110 0111 1000 1001 1010 1011 1100 1101 1110 1111
	long rightSide = m & 0x00000000FFFFFFFF;
	long leftSide = m >> 32;

	//Key:
	long k = 0x133457799BBCDFF1; // 00010011 00110100 01010111 01111001 10011011 10111100 11011111 11110001

	//Printing hexadecimal:
	printf("\n");
	printf("m: 0x%016lx \n", m);
	printf("k: 0x%016lx \n", k);
	printf("rightSide: 0x%08lx \n", rightSide);
	printf("leftSide: 0x%08lx \n", leftSide);
	printf("\n");

	//Printing values:
	printf("value of m: %ld\n", m);
	printf("value of k: %ld\n", k);
	printf("\n");

	//Determining size of PC-1:
	int size = *(&PC1 + 1) - PC1;

	//Extracting bits:
	char kP[100];
	for (int i = 0; i < size; i++){
		unsigned int bitPos = 64 - PC1[i];
		unsigned int extractedBit = (k >> bitPos) & 1;
		// printf("extracted bit: %u\n", extractedBit);
		char temp[100];
		sprintf(temp, "%u", extractedBit);

		strcat(kP, temp);
	}
	printf("Permutated key: %s\n", kP); // 1111000 0110011 0010101 0101111 0101010 1011001 1001111 0001111

	unsigned long kPHex = strtol(kP, NULL, 2);

	printf("kPHex: 0x%014lx \n", kPHex);
	unsigned int C0 = kPHex >> 28; // 1111000 0110011 0010101 0101111
	unsigned int D0 = kPHex & 0x0000000FFFFFFF; // 0101010 1011001 1001111 0001111

	printf("\n");
	printf("C0: 0x%x \n", C0);
	printf("D0: 0x%x \n", D0);
	printf("\n");

	unsigned long C[17];
	unsigned long D[17];

	C[0] = C0;
	D[0] = D0;

	for(int i = 1; i < 17; i++){
		unsigned long cA = (C[i-1] << leftShifts[i - 1]) & 0xFFFFFFF;
		unsigned long cB = C[i - 1] >> (28 - i);
		C[i] = cA^cB;
		printf("C[%d]: 0x%07lX \n", i, C[i]);

		unsigned long dA = (D[i - 1] << leftShifts[i - 1]) & 0xFFFFFFF;
		unsigned long dB = D[i - 1] >> (28 - i);
		D[i] = dA^dB;
		printf("D[%d]: 0x%07lX \n", i, C[i]);
		printf("\n");
	}
	return 0;
}