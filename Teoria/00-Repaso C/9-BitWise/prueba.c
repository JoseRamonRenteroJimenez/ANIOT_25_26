#include <stdio.h>
/********* TAREAS ************
 * Compila y ejecuta el código.
 * 1. ¿Por qué la escritura usando el puntero 'p' no sobreescribe todo el valor de 'a'?
 * 2. ¿Cuánto se modifica la dirección de 'p' tras la sentencia 'p=p+1'?
 * 3. ¿Qué habría variado si 'p' fuese del tipo 'short *' ?
 *
 ************** */
int a = 3;
int b;
char * p;
int c;
int main() {

	printf("a = %x Dir de a: %p \n",a,&a);
	p = (char*) &a;
	p=p+1;
	*p= 0x1f;
	printf("a = %x. Dir apuntada por p:%p \n",a,p);
	
	a = 3;
	b = 0x00001f00;
	a= a | b;
	
	printf("a = %x. Dir apuntada por p:%p \n",a,p);
}
