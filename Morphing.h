#ifndef MORPHING_H
#define MORPHING_H

#include <stdlib.h>
#include <stdio.h>
#include "uvsqgraphics_2.h"
#include "uvsqcouleur_2.h"

struct image {
    int hauteur, largeur;
    int range;
    COULEUR **pixels;
    int decalv, decalh;
};

struct image lire_fichier(char *nom);
void ecrire_fichier(struct image I, char *nom);

#endif
