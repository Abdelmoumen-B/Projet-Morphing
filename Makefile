Convertir.o: Convertir.c
	gcc -Wall -c Convertir.c

Convertir: Convertir.o
	gcc -Wall -o Convertir Convertir.o

Run_convertir: Convertir
	./Convertir

Morphing.o: Morphing.c Morphing.h
	gcc -Wall -c Morphing.c

Morphing: Morphing.o uvsqgraphics_2.o
	gcc -Wall -o Morphing Morphing.o uvsqgraphics_2.o -lSDL2 -lSDL2_ttf -lm

Run_morphing: Run_convertir Morphing
	./Morphing Images_transformees/bmw.ppm Images_transformees/porsche.ppm 50

uvsqgraphics_2.o: uvsqgraphics_2.c uvsqgraphics_2.h
	gcc -Wall -c uvsqgraphics_2.c

clean:
	rm -f *.o
	rm -f Morphing Convertir
	rm -f Images_Intermediaires/*.ppm
	rm -f *.aux
	rm -f *.out
	rm -f *.toc
	rm -f *.log
