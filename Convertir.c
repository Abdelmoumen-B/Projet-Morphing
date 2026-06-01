#include <stdlib.h>

// Convertit les images d'arrivées jpg en images ppm transformées
int main(int argc, char *argv[]) {
	system("convert -strip -resize 600x450! Images_initiales/bmw.jpg -compress none Images_transformees/bmw.ppm");
	system("convert -strip -resize 600x450! Images_initiales/porsche.jpg -compress none Images_transformees/porsche.ppm");
	return 0;
}
