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
	long num = 0x133457799BBCDFF1; //00010011 00110100 01010111 01111001 10011011 10111100 11011111 11110001
	unsigned int bitPos = 57;
	unsigned long mask = (1 << bitPos) - 1;
	unsigned long extractedBit = (num >> bitPos) & mask;
	printf("extractedBit: %lu \n", extractedBit);
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

	//Printing IP-1:
	int size = *(&PC1 + 1) - PC1;
	// printf("size of arr: %d\n", size);

	extractingBit();

	// char kChar[100] = "0001001100110100010101110111100110011011101111001101111111110001";
	// printf("57th bit: %c\n", kChar[56]);


	//Extracting bits:
	// char kP[100];
	// for (int i = 0; i < size; i++){
	// 	unsigned int bitPos = PC1[i];
	// 	unsigned int mask = 1 << bitPos;
	// 	unsigned int extractedBit = (k & mask) >> bitPos;
	// 	printf("extracted bit: %u\n", extractedBit);
	// }



	return 0;
}