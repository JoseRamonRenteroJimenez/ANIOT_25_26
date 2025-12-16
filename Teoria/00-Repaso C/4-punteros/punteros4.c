
#include <stdio.h>
#include <stdlib.h>
/********* TAREAS ************
 * Compila y ejecuta el código. Responde a las siguientes cuestiones:
 *  1. ¿Por qué el segundo printf() imprime un valor diferente para 'd'?
 *  2. ¿Qué es 'f'? ¿Una variable? ¿Una función? ¿Cómo se "lee" esa línea?
 *  3. Utiliza la función opera para realizar primero una suma y luego
 *     una resta de dos enteros.
 *  4. Crea un tipo llamado ptrToFunc del mismo prototipo que f usando typedef  
 *  5. Crea una función "elige()" que devuelva un puntero a función que apuntará
 *     de forma alterna, a "suma" y  a "resta" en cada invocación que se haga
 *  ***********/

int suma (int x, int y);
int resta(int x, int y);

int (*f)(int a, int b);

int suma(int x, int y) {
	return x+y;
}

int resta(int x, int y) {
	return x-y;
}

int opera(int x, int y, int (*g)(int, int)) {
	return g(x,y);
}

int main(void) {

	int a = 12;
	int b =	 8;
	int c,d;

	f = suma;
	c = suma(a,b);
	d = f(a,b);
	printf("c = %d d= %d \n",c,d);	
	f = resta;
    d = f(a,b);
	printf("c = %d d= %d \n",c,d);


}

