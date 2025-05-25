#include <stdio.h>

long M = 0x0123456789ABCDEF;
long K = 0x133457799BBCDFF1;

void printBinaryWithPadding(long num) {
    for (int i = sizeof(long) * 8 - 1; i >= 0; i--) {
        printf("%ld", (num >> i) & 1);
        if (i % 8 == 0) printf(" "); // Group by 4 bits for readability
    }
    printf("\n");
}

int main(){
	printf("M: \n");
	printBinaryWithPadding(M);
	printf("\n");

	return 0;
}