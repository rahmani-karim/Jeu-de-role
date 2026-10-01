#include "jeu.h"
#include <stdio.h>
// Fonction de test pour voir le contenu du fichier sauvegarde.txt
int main() {
    srand(time(NULL));
    printf("=== JEU DE ROLES - CHATEAU ===\n\n");
    
    Jeu* jeu = NULL;
    int choix;
    
    /* BOUCLE DU MENU DE DÉMARRAGE */
    /* On reste dans cette boucle tant que le jeu n'est pas créé */
    while (1) {
        printf("\n1. Nouvelle partie\n");
        printf("2. Charger une partie\n");
        printf("3. Consulter les regles du jeu\n"); /* NOUVELLE OPTION */
        printf("4. Quitter\n");
        printf("Votre choix: ");
        
        if (scanf("%d", &choix) != 1) {
            choix = 0; // Entrée invalide
        }
        while(getchar() != '\n'); // Vider le buffer
        
        if (choix == 1) {
            jeu = initialiser_jeu();
            if (jeu) break; // Le jeu est prêt, on sort du menu pour jouer
            printf("Erreur d'initialisation du jeu\n");
        }
        else if (choix == 2) {
            
            
            jeu = charger_jeu("sauvegarde.txt");
            if (jeu) {
                printf(">>> SUCCES : Partie chargee !\n");
                break; // On sort de la boucle pour aller jouer directement
            }
            printf(">>> ECHEC : Impossible de charger la sauvegarde.\n");
            printf(">>> Verifiez que le fichier existe ou essayez 'Nouvelle partie'.\n");
        }
        else if (choix == 3) {
            /* Affiche les règles puis revient au menu */
            afficher_regles_combat();
            pause_console();
        }
        else if (choix == 4) {
            printf("Au revoir !\n");
            return 0;
        }
        else {
            printf("Choix invalide\n");
        }
    }
    
    if (!jeu) {
        printf("Erreur: jeu non initialisé\n");
        return 1;
    }
    
    /* DÉBUT DU JEU */
    printf("\n=== BIENVENUE DANS LE CHATEAU ===\n");
    printf("Vous controlez : %s. Votre but: survivre et eliminer les autres personnages.\n", jeu->joueur->nom);
    
    /* On affiche la carte pour situer le joueur */
    afficher_matrice_chateau(jeu);
    
    
    pause_console();
    
    /* Boucle principale du jeu */
    while (!jeu->jeu_termine) {
        clear_screen();
        executer_tour(jeu);
        
        if (!jeu->jeu_termine) {
            pause_console();
        }
    }
    
    /* Fin du jeu */
    printf("\n=== PARTIE TERMINEE ===\n");
    printf("Nombre de tours joué: %d\n", jeu->tour);
    printf("Personnages encore vivants: %d\n", jeu->nb_personnages_vivants);
    
    /* Nettoyage */
    liberer_jeu(jeu);
    
    printf("\nMerci d'avoir jouer !\n");
    
    return 0;
}