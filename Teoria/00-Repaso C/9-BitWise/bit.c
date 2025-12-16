#include <stdio.h>

/********* TAREAS ************
 * Compila y ejecuta el código.
 * Estudia la sintaxis de los operadores a nivel de bit 
 * y trata de comprender el resultado de cada operación
 ************** */

int a,b,c;

int main() {

	a = 7;
	b = 9;
	c = a & b;
	printf("%x AND %x = %x\n",a,b,c);

	c= a | b;
	printf("%x OR %x = %x\n",a,b,c);

	c = a ^ b;
	printf("%x XOR %x = %x\n",a,b,c);

	c = ~a;
	printf("NOT %x = %x\n",a,c);

	c = a << 2;
	printf(" %x << 2 = %x\n",a,c);

	c = a >> 1;
	printf(" %x >> 1 = %x\n",a,c);

	// Poner a 0 el bit 2 de a ->  pasa de 7 a 3
	c = a & 0xFB;
	printf(" %x bit 2 a 0 -> %x\n",a,c);

	// Poner a 1 el bit 6 de a -> pasa de 7 a 2^6 + 7 = 71
	c = a | 0x40;
	printf(" %x bit 6 a 1 -> %x\n",a,c);

	// Extrar los bits 2,3 y4 de a (a = 7 = 0000111 -> bits 432 =  001)
	c = (a & 0x1C) >> 2;
	printf("bits 4-3-2 de %x: %x\n",a,c);	
}

