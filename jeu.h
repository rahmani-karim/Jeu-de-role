#ifndef JEU_H
#define JEU_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

/* Constantes du jeu */
#define NB_SALLES 16
#define NB_PERSONNAGES 6
#define MAX_NOM 50
#define MAX_OBJETS_SAC 4
#define NB_TYPES_OBJETS 5

/* Énumérations */
typedef enum {
    NORD, SUD, EST, OUEST, AUCUNE
} Direction;

typedef enum {
    MEDICAMENT, POISON, ARME, BOUCLIER, CLE_TELEPORTATION, AUCUN_OBJET
} TypeObjet;

/* Structures de données */
typedef struct Objet {
    char nom[MAX_NOM];
    TypeObjet type;
    struct Objet *suivant;
} Objet;

typedef struct Personnage {
    char nom[MAX_NOM];
    int indice_vie;
    Objet *sac;
    int nb_objets_sac;
    int salle;
    struct Personnage *suivant;
    bool est_joueur;
    bool est_vivant;
} Personnage;

typedef struct Salle {
    int id;
    Personnage *personnages_presents;
    Objet *objets_au_sol;
    int nb_objets_sol;
} Salle;

typedef struct Jeu {
    Personnage *personnages;
    Personnage *joueur;
    Salle salles[NB_SALLES];
    int tour;
    int nb_personnages_vivants;
    bool jeu_termine;
} Jeu;

/* Prototypes de fonctions */


/* Initialisation et libération */
Jeu* initialiser_jeu(void);
void liberer_jeu(Jeu *jeu);
Personnage* creer_personnage(const char *nom, bool est_joueur);
Objet* creer_objet(TypeObjet type);

/* Gestion des listes chaînées */
void ajouter_personnage_liste(Personnage **liste, Personnage *p);
void retirer_personnage_liste(Personnage **liste, Personnage *p);
void ajouter_objet_liste(Objet **liste, Objet *obj, int *compteur);
void retirer_objet_liste(Objet **liste, TypeObjet type, int *compteur);
void liberer_liste_objets(Objet **liste, int *compteur);

/* Gestion des objets */
bool sac_est_plein(const Personnage *p);
void ajouter_objet_sac(Personnage *p, Objet *obj);
void retirer_objet_sac(Personnage *p, TypeObjet type);
void generer_objets_salle(Salle *salle);
void gerer_inventaire_slots(Jeu* jeu, Personnage* p);
void ia_ramasser_tout(Jeu *jeu, Personnage *p);
Objet* get_objet_at_index(Objet* liste, int index) ;
/* Déplacement et salles */

void deplacer_personnage(Jeu *jeu, Personnage *p, Direction dir);
void afficher_portes_disponibles(int salle);

/* Combat */
void afficher_regles_combat(void);
void resoudre_combat(Jeu *jeu, Personnage *attaquant, Objet *obj_att, 
                     Personnage *defenseur, Objet *obj_def);
void combat_automatique(Jeu*jeu, Personnage *p1, Personnage *p2);
TypeObjet choisir_objet_combat_joueur(Personnage *p);
Objet* choisir_objet_combat_ia(Personnage *p);
void utiliser_cle_teleportation(Jeu *jeu, Personnage *p);
Objet* choisir_objet_reaction_ia(Personnage *p, TypeObjet menace);
/* Affichage */



/* Logique du jeu */
void tour_joueur(Jeu *jeu);
void tour_ia_ameliore(Jeu *jeu);
void executer_tour(Jeu *jeu);
void verifier_fin_jeu(Jeu *jeu);


/* Sauvegarde et chargement */
void sauvegarder_jeu(const Jeu *jeu, const char *fichier);
Jeu* charger_jeu(const char *fichier);

/* Utilitaires */
Direction choisir_direction_joueur(void);
void clear_screen(void);
void pause_console(void);
void afficher_matrice_chateau(const Jeu *jeu);
void afficher_ligne_separation(int largeur_colonne) ;

#endif /* JEU_H */