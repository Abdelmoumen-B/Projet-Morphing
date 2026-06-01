#include "Morphing.h"
#define MAX_POINTS 100

#define R(c) (((c) >> 16) & 255)
#define G(c) (((c) >> 8) & 255)
#define B(c) ((c) & 255)


//Variables de la phase sur laquelle on est
#define POINT_G 0
#define POINT_D 1
#define QUITTER 2
#define SAUVER_G 3
#define SAUVER_D 4
#define SUPPRIMER_G 5
#define SUPPRIMER_D 6

typedef struct {
    POINT g;  // point gauche (image départ)
    POINT d;  // point droite (image arrivée)
} Couple;

typedef struct {
    int a, b, c; // indices des points
} Triangle;


// Lire les points et les stocker
int lire_points(const char *nom, Couple *tab, int max) {
    FILE *f = fopen(nom, "r");
    if (!f) {
        printf("Erreur ouverture %s\n", nom);
        exit(1);
    }

    int n = 0;

    // === Lire la première ligne : 4 coins ===
    int x1,y1,x2,y2,x3,y3,x4,y4;
    if (fscanf(f, "%d %d %d %d %d %d %d %d",
               &x1,&y1,&x2,&y2,&x3,&y3,&x4,&y4) != 8) {
        printf("Erreur lecture coins\n");
        exit(1);
    }

    // On crée 4 couples identité
    tab[0].g.x = x1; tab[0].g.y = y1; tab[0].d.x = x1; tab[0].d.y = y1;
    tab[1].g.x = x2; tab[1].g.y = y2; tab[1].d.x = x2; tab[1].d.y = y2;
    tab[2].g.x = x3; tab[2].g.y = y3; tab[2].d.x = x3; tab[2].d.y = y3;
    tab[3].g.x = x4; tab[3].g.y = y4; tab[3].d.x = x4; tab[3].d.y = y4;

    n = 4;

    // === Lire le reste : vrais couples ===
    int gx,gy,dx,dy;
    while (fscanf(f, "%d %d %d %d", &gx, &gy, &dx, &dy) == 4) {
        if (n >= max) break;
        tab[n].g.x = gx;
        tab[n].g.y = gy;
        tab[n].d.x = dx;
        tab[n].d.y = dy;
        n++;
    }

    fclose(f);
    return n;
}


// Outils géométriques trucs de maths
float det(float ax, float ay, float bx, float by) {
    return ax*by - ay*bx;
}

int point_dans_triangle(POINT P, POINT A, POINT B, POINT C) {
    float v0x = C.x - A.x;
    float v0y = C.y - A.y;
    float v1x = B.x - A.x;
    float v1y = B.y - A.y;
    float v2x = P.x - A.x;
    float v2y = P.y - A.y;

    float d = det(v0x, v0y, v1x, v1y);
    if (fabs(d) < 0.00001) return 0;

    float a = det(v2x, v2y, v1x, v1y) / d;
    float b = det(v0x, v0y, v2x, v2y) / d;

    return (a >= 0 && b >= 0 && a + b <= 1);
}

void barycentriques(POINT P, POINT A, POINT B, POINT C, float *l, float *m) {
    float v0x = B.x - A.x;
    float v0y = B.y - A.y;
    float v1x = C.x - A.x;
    float v1y = C.y - A.y;
    float v2x = P.x - A.x;
    float v2y = P.y - A.y;

    float d = det(v0x, v0y, v1x, v1y);

    *l = det(v2x, v2y, v1x, v1y) / d;
    *m = det(v0x, v0y, v2x, v2y) / d;
}

// Triangulation
Triangle *trianguler(POINT *pts, int n, int *nb_tris) {
    Triangle *T = malloc(sizeof(Triangle) * (2*n));
    *nb_tris = 0;

    // Découpe simple du rectangle en 2 triangles
	T[0] = (Triangle){0,1,3};
	T[1] = (Triangle){0,3,2};

	*nb_tris = 2;


    for (int p = 4; p < n; p++) {
        for (int t = 0; t < *nb_tris; t++) {
            Triangle tr = T[t];
            if (point_dans_triangle(pts[p], pts[tr.a], pts[tr.b], pts[tr.c])) {
                Triangle old = tr;

                T[t] = T[(*nb_tris)-1];
                (*nb_tris)--;

                T[(*nb_tris)++] = (Triangle){old.a, old.b, p};
                T[(*nb_tris)++] = (Triangle){old.b, old.c, p};
                T[(*nb_tris)++] = (Triangle){old.c, old.a, p};
                break;
            }
        }
    }
    return T;
}

// Trouver le pixel contenant un pixel
int trouver_triangle(POINT P, POINT *pts, Triangle *T, int nb_tris) {
    for (int i = 0; i < nb_tris; i++) {
        Triangle tr = T[i];
        if (point_dans_triangle(P, pts[tr.a], pts[tr.b], pts[tr.c]))
            return i;
    }
    return -1;
}

// Generation d'une image intermediaire
void generer_image_intermediaire(struct image *res,
                                 struct image *dep,
                                 struct image *arr,
                                 Couple *couples,
                                 int nb_pts,
                                 float alpha)
{
    // 1) calcul points intermédiaires
    POINT *ptsI = malloc(sizeof(POINT)*nb_pts);
    for (int i = 0; i < nb_pts; i++) {
        ptsI[i].x = (1-alpha)*couples[i].g.x + alpha*couples[i].d.x;
        ptsI[i].y = (1-alpha)*couples[i].g.y + alpha*couples[i].d.y;
    }

    // 2) triangulation
    int nb_tris;
    Triangle *T = trianguler(ptsI, nb_pts, &nb_tris);

    // 3) pour chaque pixel
    for (int y = 0; y < res->hauteur; y++) {
        for (int x = 0; x < res->largeur; x++) {
            POINT P = {x, y};

            int id = trouver_triangle(P, ptsI, T, nb_tris);
            if (id < 0) {
				res->pixels[y][x] = couleur_RGB(0,0,0);
				continue;
			}


            Triangle tr = T[id];

            POINT A = ptsI[tr.a];
            POINT B = ptsI[tr.b];
            POINT C = ptsI[tr.c];

            float l, m;
            barycentriques(P, A, B, C, &l, &m);

            // Projection dans image départ
            POINT AD = couples[tr.a].g;
            POINT BD = couples[tr.b].g;
            POINT CD = couples[tr.c].g;

            POINT PD;
            PD.x = AD.x + l*(BD.x - AD.x) + m*(CD.x - AD.x);
            PD.y = AD.y + l*(BD.y - AD.y) + m*(CD.y - AD.y);

            // Projection dans image arrivée
            POINT AA = couples[tr.a].d;
            POINT BA = couples[tr.b].d;
            POINT CA = couples[tr.c].d;

            POINT PA;
            PA.x = AA.x + l*(BA.x - AA.x) + m*(CA.x - AA.x);
            PA.y = AA.y + l*(BA.y - AA.y) + m*(CA.y - AA.y);

            // Clamp
            if (PD.x < 0) PD.x = 0;
            if (PD.y < 0) PD.y = 0;
            if (PA.x < 0) PA.x = 0;
            if (PA.y < 0) PA.y = 0;

            if (PD.x >= dep->largeur) PD.x = dep->largeur-1;
            if (PD.y >= dep->hauteur) PD.y = dep->hauteur-1;
            if (PA.x >= arr->largeur) PA.x = arr->largeur-1;
            if (PA.y >= arr->hauteur) PA.y = arr->hauteur-1;

            COULEUR cD = dep->pixels[PD.y][PD.x];
			COULEUR cA = arr->pixels[PA.y][PA.x];

			int rD = R(cD), gD = G(cD), bD = B(cD);
			int rA = R(cA), gA = G(cA), bA = B(cA);


			int r = (1-alpha)*rD + alpha*rA;
			int g = (1-alpha)*gD + alpha*gA;
			int b = (1-alpha)*bD + alpha*bA;

			res->pixels[y][x] = couleur_RGB(r, g, b);

        }
    }

    free(T);
    free(ptsI);
}

// Sauvegarde ppm
void sauver_ppm(struct image I, const char *nom) {
    FILE *f = fopen(nom, "w");
    fprintf(f, "P3\n%d %d\n%d\n", I.largeur, I.hauteur, I.range);

    for (int i = 0; i < I.hauteur; i++) {
        for (int j = 0; j < I.largeur; j++) {
            COULEUR c = I.pixels[i][j];
			fprintf(f, "%d %d %d ", R(c), G(c), B(c));
        }
        fprintf(f, "\n");
    }
    fclose(f);
}


// Calculs demandé afin d'avoir les points intermediaires
void calculer_points_intermediaires(Couple *src, int n, POINT *dest, float alpha) {
    for (int i = 0; i < n; i++) {
        dest[i].x = (1 - alpha) * src[i].g.x + alpha * src[i].d.x;
        dest[i].y = (1 - alpha) * src[i].g.y + alpha * src[i].d.y;
    }
}

// Allocation mémoire
COULEUR** allouer_pixels(int largeur, int hauteur) {
    COULEUR** pixels = malloc(hauteur * sizeof(COULEUR*));
    if (!pixels) exit(3);

    for (int i = 0; i < hauteur; i++) {
        pixels[i] = malloc(largeur * sizeof(COULEUR));
        if (!pixels[i]) exit(2);
    }
    return pixels;
}

// Désallocation mémoire
void desallouer_pixels(COULEUR** pixels, int hauteur) {
    for (int i = 0; i < hauteur; i++) {
        free(pixels[i]);
    }
    free(pixels);
}

// Fonction qui lit le fichier ppm entré et le stocke
struct image lire_fichier(char *nom) {
    struct image img;
    FILE *F = fopen(nom, "r");
    if (!F) { printf("Erreur lors de l’ouverture\n"); exit(1); }

    int a,b,c;

    // Sauter la ligne "P3"
    while ((a = fgetc(F)) != '\n') {};

    // Lire largeur hauteur range
    fscanf(F, "%d %d %d", &a, &b, &c);
    img.largeur = a;
    img.hauteur = b;
    img.range = c;

    // Allocation pixels
    img.pixels = allouer_pixels(img.largeur, img.hauteur);

    // Lecture des pixels
    for (int i = 0; i < img.hauteur; i++) {
        for (int j = 0; j < img.largeur; j++) {
            fscanf(F, "%d %d %d", &a, &b, &c);
            img.pixels[i][j] = couleur_RGB(a,b,c);
        }
    }

    fclose(F);
    img.decalh=0;
    img.decalv=0;
    return img;
}

// afficher les images cote à cote
void afficher_pixels(struct image I) {
	POINT P;
	// On affiche les pixels aux bonnes positions
	for (int i=0; i<I.hauteur ; i++) {
		P.y = i+I.decalv;
		for (int j = 0; j<I.largeur ; j++) {
			P.x = j+I.decalh;
			draw_pixel(P,I.pixels[i][j]);
		}
	}
}
	
// Main
int main(int argc, char *argv[]) {
	if (argc != 4) {
        printf("Usage: %s image_depart.ppm image_arrivee.ppm N\n", argv[0]);
        return 1;
    }
    
	// Variables
	struct image I1 = lire_fichier(argv[1]);
	struct image I2 = lire_fichier(argv[2]);

    int Phase = POINT_G;
    POINT clic;
    
    // Affichage de la fenêtre
	I2.decalh = I1.largeur + 100;
    affiche_auto_off();
    init_graphics(I1.largeur+I2.largeur+100,I1.hauteur+200);
    
    // Variables pour points
    Couple points[MAX_POINTS];
    int nb_points = 0;

	// Affichage premier
	// Affichage des images à gauche et à droite
	afficher_pixels(I1);
	afficher_pixels(I2);

    // Affichage boutons
    POINT b_quit1 = {400, 550};
    POINT b_quit2 = {500, 590};
    POINT b_sauv1 = {600, 550};
    POINT b_sauv2 = {700, 590};
    draw_fill_rectangle(b_quit1, b_quit2, rouge);
    draw_fill_rectangle(b_sauv1, b_sauv2, vert);
    POINT t_quit = {b_quit1.x + 10, b_quit1.y + 5};
    POINT t_sauv = {b_sauv1.x + 10, b_sauv1.y + 5};
    aff_pol("Quitter", 12, t_quit, bleu);
    aff_pol("Sauver", 12, t_sauv, bleu);
        
    // Tout afficher
    affiche_all();
	
	// Boucle
    while (1) {
        // Premier clique sur image gauche
        if (Phase == POINT_G) {
			printf("Veuillez cliquer sur l'image de départ !\n");
			clic = wait_clic();
		}
        // Supprimer point
		for (int i = 0; i < nb_points; i++) {
			if ((clic.x >= points[i].g.x - 2 && clic.x <= points[i].g.x + 2 && clic.y >= points[i].g.y - 2 && clic.y <= points[i].g.y + 2) || (clic.x >= points[i].d.x - 2 && clic.x <= points[i].d.x + 2 && clic.y >= points[i].d.y - 2 && clic.y <= points[i].d.y + 2)) {
				points[i].g.x = -1;
				points[i].g.y = -1;
				points[i].d.x = -1;
				points[i].d.y = -1;
				printf("Points  %d effacés !\n", i+1);
				Phase = SUPPRIMER_G;
			}
		}
        // Quitter
        if (clic.x >= 400 && clic.x <= 500 && clic.y >= 550 && clic.y <= 590) {
			Phase = QUITTER;
		}
		// Sauver
		if (clic.x >= 600 && clic.x <= 700 && clic.y >= 550 && clic.y <= 590) {
			FILE *f = fopen("points.txt", "w");
			if (f) {
				fprintf(f,"0 0  600 0  0 450  600 450\n");
				for (int i = 0; i < nb_points; i++) {
					fprintf(f, "%d %d  %d %d\n",
							points[i].g.x, points[i].g.y,
							points[i].d.x, points[i].d.y);
				}
				fclose(f);
			}
			printf("Sauvegarde réussie.\n");
			Phase = SAUVER_G;
		}
		// Phase suivante
		if ((clic.x <= 600 && clic.y <= 450) && Phase == POINT_G) {
			points[nb_points].g = clic;
			Phase = POINT_D;
		}
        

        // Premier clique sur image droite
        if (Phase == POINT_D) {
			printf("Veuillez cliquer sur l'image d'arrivée !\n");
			clic = wait_clic();
		}
		// Supprimer point
		for (int i = 0; i < nb_points; i++) {
			if ((clic.x >= points[i].g.x - 2 && clic.x <= points[i].g.x + 2 && clic.y >= points[i].g.y - 2 && clic.y <= points[i].g.y + 2) || (clic.x >= points[i].d.x - 2 && clic.x <= points[i].d.x + 2 && clic.y >= points[i].d.y - 2 && clic.y <= points[i].d.y + 2)) {
				points[i].g.x = -1;
				points[i].g.y = -1;
				points[i].d.x = -1;
				points[i].d.y = -1;
				printf("Points  %d effacés !\n", i+1);
				Phase = SUPPRIMER_D;
			}
		}
		// Quitter
		if (clic.x >= 400 && clic.x <= 500 && clic.y >= 550 && clic.y <= 590) {
			Phase = QUITTER;
		}
		// Sauver
		if ((clic.x >= 600 && clic.x <= 700 && clic.y >= 550 && clic.y <= 590) && !(Phase == SAUVER_G)) {
			FILE *f = fopen("points.txt", "w");
			if (f) {
				for (int i = 0; i < nb_points; i++) {
					fprintf(f,"0 0  600 0  0 450  600 450\n");
					fprintf(f, "%d %d  %d %d\n",
							points[i].g.x, points[i].g.y,
							points[i].d.x, points[i].d.y);
				}
				fclose(f);
			}
			printf("Sauvegarde réussie.\n");
			Phase = SAUVER_D;
		}
		// Phase suivante
		if ((clic.x >= 700 && clic.y <= 450 && clic.x <= 1300) && Phase == POINT_D) {
			points[nb_points].d = clic;
			nb_points++;
			Phase = POINT_G;
		}
        
        // Phase suivante
        if (Phase == SAUVER_G || Phase == SUPPRIMER_G) {
			Phase = POINT_G;
		}
		if (Phase == SAUVER_D || Phase == SUPPRIMER_D) {
			Phase = POINT_D;
		}
        
        // Affichages continus
        // Affichage des images à gauche et à droite
		afficher_pixels(I1);
		afficher_pixels(I2);
		
        // Affichage points et numéros
        for (int i=0; i<=nb_points; i++) {
			draw_fill_circle(points[i].g, 5, rouge);
			draw_fill_circle(points[i].d, 5, rouge);
			char buf[12];
			pol_style(GRAS);
			sprintf(buf, "%d", i+1);
			POINT txt_g = {points[i].g.x - 3, points[i].g.y - 8};
			POINT txt_d = {points[i].d.x - 3, points[i].d.y - 8};
			aff_pol(buf, 12, txt_g, blanc);
			aff_pol(buf, 12, txt_d, blanc);
		}

        // Affichage boutons
        POINT b_quit1 = {400, 550};
        POINT b_quit2 = {500, 590};
        POINT b_sauv1 = {600, 550};
        POINT b_sauv2 = {700, 590};
        draw_fill_rectangle(b_quit1, b_quit2, rouge);
        draw_fill_rectangle(b_sauv1, b_sauv2, vert);
        POINT t_quit = {b_quit1.x + 10, b_quit1.y + 5};
        POINT t_sauv = {b_sauv1.x + 10, b_sauv1.y + 5};
        aff_pol("Quitter", 12, t_quit, bleu);
        aff_pol("Sauver", 12, t_sauv, bleu);
        
        // Afficher coordonnées au milieu
		POINT centre = {LARGEUR_FENETRE/2, HAUTEUR_FENETRE/6};
		char texte[50];
		snprintf(texte, sizeof(texte), "Point %d :", nb_points);
		aff_pol_centre(texte, 22, centre, blanc);
		centre.y = HAUTEUR_FENETRE/5;
		snprintf(texte, sizeof(texte), "(%d,%d)", points[nb_points-1].g.x, points[nb_points-1].g.y);
		aff_pol_centre(texte, 22, centre, blanc);
		centre.y = HAUTEUR_FENETRE/4;
		snprintf(texte, sizeof(texte), "(%d,%d)", points[nb_points-1].d.x, points[nb_points-1].d.y);
		aff_pol_centre(texte, 22, centre, blanc);
        
        // Tout afficher
        affiche_all();
        
        // Echap pour quitter
        if (Phase == QUITTER || nb_points >= MAX_POINTS) {
			printf("Echap pour quitter.\n");
			break;
		}
    }
    int N = atoi(argv[3]);  // nombre d'images intermédiaires

	// Lire les couples depuis points.txt
	Couple couples[200];
	int nb_pts = lire_points("points.txt", couples, 200);

	// Image résultat
	struct image ImgI;
	ImgI.largeur = I1.largeur;
	ImgI.hauteur = I1.hauteur;
	ImgI.range = I1.range;
	ImgI.pixels = allouer_pixels(ImgI.largeur, ImgI.hauteur);

    
    for (int k = 0; k <= N; k++) {
    float alpha = (float)k / (float)N;

    generer_image_intermediaire(&ImgI, &I1, &I2, couples, nb_pts, alpha);

    char nom[64];
    sprintf(nom, "Images_Intermediaires/img_%03d.ppm", k);
    sauver_ppm(ImgI, nom);
	}
	system("ffmpeg -framerate 4 -i Images_Intermediaires/img_%03d.ppm -c:v libx264 -pix_fmt yuv420p morphing.mp4");
    
    wait_escape();  


    return 0;
}
