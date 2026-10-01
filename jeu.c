#include "jeu.h"
#include <ctype.h>
#include <sys/ioctl.h>
#include <unistd.h>
/* Noms des personnages */
static const char* NOMS_PERSONNAGES[NB_PERSONNAGES] = {
    "Roi", "Reine", "Moine", "Guerrier", "Sorciere", "Amazone"
};

/* Noms des objets selon type */
static const char* NOMS_OBJETS[NB_TYPES_OBJETS] = {
    "Medicament", "Poison", "Arme", "Bouclier", "Cle de teleportation"
};

/* Probabilités d'apparition des objets (en %) */
static const int PROBA_OBJETS[NB_TYPES_OBJETS] = {20,20,20,20,20};
/* ==================== INITIALISATION ET LIBÉRATION ==================== */

Jeu* initialiser_jeu() {
    Jeu* jeu = (Jeu*)malloc(sizeof(Jeu));
    if (!jeu) return NULL;
    
    srand(time(NULL));
    
    /* Initialisation des salles */
    for (int i = 0; i < NB_SALLES; i++) {
        jeu->salles[i].id = i + 1;
        jeu->salles[i].personnages_presents = NULL;
        jeu->salles[i].objets_au_sol = NULL;
        jeu->salles[i].nb_objets_sol = 0;
    }

    /* --- MENU DE SÉLECTION DU PERSONNAGE --- */
    printf("\n=== CHOISISSEZ VOTRE PERSONNAGE ===\n");
    for (int i = 0; i < NB_PERSONNAGES; i++) {
        printf("%d. %s\n", i + 1, NOMS_PERSONNAGES[i]);
    }
    
    int choix_perso;
    do {
        printf("Votre choix (1-%d): ", NB_PERSONNAGES);
        if (scanf("%d", &choix_perso) != 1) {
            choix_perso = 0;
        }
        while(getchar() != '\n'); 
    } while (choix_perso < 1 || choix_perso > NB_PERSONNAGES);
    
    /* Initialisation des personnages */
    jeu->personnages = NULL;
    jeu->nb_personnages_vivants = NB_PERSONNAGES;
    
    /* Tableau des salles disponibles */
    int salles_disponibles[NB_SALLES];
    for (int i = 0; i < NB_SALLES; i++) {
        salles_disponibles[i] = i + 1;
    }
    
    /* Mélanger les salles */
    for (int i = NB_SALLES - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = salles_disponibles[i];
        salles_disponibles[i] = salles_disponibles[j];
        salles_disponibles[j] = temp;
    }
    
    /* Création des 6 personnages */
    for (int i = 0; i < NB_PERSONNAGES; i++) {
        
        bool est_joueur = (i == (choix_perso - 1)); 

        Personnage* p = creer_personnage(NOMS_PERSONNAGES[i], est_joueur);
        
        p->salle = salles_disponibles[i];
        ajouter_personnage_liste(&jeu->personnages, p);
        ajouter_personnage_liste(&jeu->salles[p->salle - 1].personnages_presents, p);
        
        if (est_joueur) {
            jeu->joueur = p;
        }
    }
    
    jeu->tour = 1;
    jeu->jeu_termine = false;
    
    /* Générer des objets dans quelques salles aléatoires */
    for (int i = 0; i < NB_SALLES; i++) {
        jeu->salles[i].objets_au_sol = NULL;
        generer_objets_salle(&jeu->salles[i]);
        jeu->salles[i].nb_objets_sol = 0;
    }
    return jeu;
}

void liberer_jeu(Jeu *jeu) {
    if (!jeu) return;
    
    /* Libérer tous les personnages */
    Personnage* courant = jeu->personnages;
    while (courant) {
        Personnage* suivant = courant->suivant;
        liberer_liste_objets(&courant->sac, &courant->nb_objets_sac);
        free(courant);
        courant = suivant;
    }
    
    /* Libérer les objets au sol dans chaque salle */
    for (int i = 0; i < NB_SALLES; i++) {
        liberer_liste_objets(&jeu->salles[i].objets_au_sol, &jeu->salles[i].nb_objets_sol);
    }
    
    free(jeu);
}
/* ==================== GESTION DES LISTES CHAÎNÉES ==================== */
Personnage* creer_personnage(const char *nom, bool est_joueur) {
    Personnage* p = (Personnage*)malloc(sizeof(Personnage));
    if (!p) return NULL;
    
    strncpy(p->nom, nom, MAX_NOM - 1);
    p->nom[MAX_NOM - 1] = '\0';
    p->indice_vie = 3;
    p->sac = NULL;
    p->nb_objets_sac = 0;
    p->salle = 1;
    p->suivant = NULL;
    p->est_joueur = est_joueur;
    p->est_vivant = true;
    
    return p;
}

Objet* creer_objet(TypeObjet type) {
    if (type < 0 || type >= NB_TYPES_OBJETS) return NULL;
    
    Objet* obj = (Objet*)malloc(sizeof(Objet));
    if (!obj) return NULL;
    
    strncpy(obj->nom, NOMS_OBJETS[type], MAX_NOM - 1);
    obj->nom[MAX_NOM - 1] = '\0';
    obj->type = type;
    obj->suivant = NULL;
    
    return obj;
}

void ajouter_personnage_liste(Personnage **liste, Personnage *p) {
    if (!p) return;
    p->suivant = *liste;
    *liste = p;
}

void retirer_personnage_liste(Personnage **liste, Personnage *p) {
    if (!liste || !p) return;
    
    Personnage *courant = *liste;
    Personnage *precedent = NULL;
    
    while (courant) {
        if (courant == p) {
            if (precedent) {
                precedent->suivant = courant->suivant;
            } else {
                *liste = courant->suivant;
            }
            courant->suivant = NULL; 
            return;
        }
        precedent = courant;
        courant = courant->suivant;
    }
}

void ajouter_objet_liste(Objet **liste, Objet *obj, int *compteur) {
    if (!obj) return;
    
    obj->suivant = *liste;
    *liste = obj;
    if (compteur) (*compteur)++;
}

void retirer_objet_liste(Objet **liste, TypeObjet type, int *compteur) {
    if (!liste || !*liste) return;
    
    Objet* courant = *liste;
    Objet* precedent = NULL;
    
    while (courant) {
        if (courant->type == type) {
            if (precedent) {
                precedent->suivant = courant->suivant;
            } else {
                *liste = courant->suivant;
            }
            
            free(courant);
            if (compteur && *compteur > 0) (*compteur)--;
            return;
        }
        precedent = courant;
        courant = courant->suivant;
    }
}

void liberer_liste_objets(Objet **liste, int *compteur) {
    Objet* courant = *liste;
    while (courant) {
        Objet* suivant = courant->suivant;
        free(courant);
        courant = suivant;
    }
    *liste = NULL;
    if (compteur) *compteur = 0;
}

/* ==================== GESTION DES OBJETS ==================== */

bool sac_est_plein(const Personnage *p) {
    return p->nb_objets_sac >= MAX_OBJETS_SAC;
}

void ajouter_objet_sac(Personnage *p, Objet *obj) {
    if (!p || !obj) return;
    
    if (sac_est_plein(p)) {
        printf("Le sac de %s est plein !\n", p->nom);
        free(obj);
        return;
    }
    
    ajouter_objet_liste(&p->sac, obj, &p->nb_objets_sac);
}

void retirer_objet_sac(Personnage *p, TypeObjet type) {
    retirer_objet_liste(&p->sac, type, &p->nb_objets_sac);
}


void generer_objets_salle(Salle *salle) {
    if (!salle) return;
    
    /* MODIFICATION ICI : Entre 2 et 4 objets */
    /* rand() % 3 donne 0, 1 ou 2. On ajoute 2 -> Résultat : 2, 3 ou 4. */
    int nb_objets = (rand() % 3) + 2; 
    
    for (int i = 0; i < nb_objets; i++) {
        int r = rand() % 100;
        int cumul = 0;
        TypeObjet type = AUCUN_OBJET;
        
        for (int j = 0; j < NB_TYPES_OBJETS; j++) {
            cumul += PROBA_OBJETS[j];
            if (r < cumul) {
                type = (TypeObjet)j;
                break;
            }
        }
        
        if (type != AUCUN_OBJET) {
            Objet* obj = creer_objet(type);
            ajouter_objet_liste(&salle->objets_au_sol, obj, &salle->nb_objets_sol);
        }
    }
}
/* Fonction aide : Récupérer un objet à un index précis (1-4) dans une liste */
Objet* get_objet_at_index(Objet* liste, int index) {
    int i = 1;
    while (liste && i < index) {
        liste = liste->suivant;
        i++;
    }
    return (i == index) ? liste : NULL;
}

void gerer_inventaire_slots(Jeu* jeu, Personnage* p) {
    Salle* s = &jeu->salles[p->salle - 1];
    bool menu_actif = true;

    while (menu_actif) {
        printf("\n================ GESTION INVENTAIRE ================\n");
        
        printf("--- AU SOL (Salle %d) ---\n", p->salle);
        if (!s->objets_au_sol) printf("   (Rien)\n");
        else {
            Objet* o = s->objets_au_sol;
            int i = 1;
            while (o) {
                printf("   %d. %s\n", i++, o->nom);
                o = o->suivant;
            }
        }

        /* 2. Affichage Sac (Slots 1-4) */
        printf("\n--- VOTRE SAC ---\n");
        for (int i = 1; i <= MAX_OBJETS_SAC; i++) {
            Objet* obj = get_objet_at_index(p->sac, i);
            if (obj) printf("   %d. [%s]\n", i, obj->nom);
            else     printf("   %d. (Vide)\n", i);
        }
        printf("====================================================\n");
        printf("Action : Entrez 'Numero_Objet_Sol' puis 'Numero_Slot_Sac'\n");
        printf("Exemple: Tapez '1 2' pour mettre l'objet 1 du sol dans le slot 2.\n");
        printf("(Tapez '0' pour RETOURNER au menu principal)\n");
        printf("Votre choix > ");

        int choix_sol, choix_sac;
        int res = scanf("%d", &choix_sol);
        if (res != 1 || choix_sol == 0) {
            getchar(); // Vider buffer
            menu_actif = false; // Retour
            continue;
        }
        
        // On demande le slot du sac
        if (scanf("%d", &choix_sac) != 1) choix_sac = 0;
        getchar();

        /* Validation */
        if (choix_sac < 1 || choix_sac > MAX_OBJETS_SAC) {
            printf(">> Erreur : Le slot du sac doit etre entre 1 et 4.\n");
            continue;
        }

        /* 3. Logique de transfert */
        
        // A. Trouver l'objet au sol
        Objet* obj_sol = s->objets_au_sol;
        Objet* prev_sol = NULL;
        int idx = 1;
        while (obj_sol && idx < choix_sol) {
            prev_sol = obj_sol;
            obj_sol = obj_sol->suivant;
            idx++;
        }

        if (!obj_sol) {
            printf(">> Erreur : Pas d'objet numero %d au sol.\n", choix_sol);
            continue;
        }

        // B. Détacher l'objet du sol
        if (prev_sol) prev_sol->suivant = obj_sol->suivant;
        else s->objets_au_sol = obj_sol->suivant;
        s->nb_objets_sol--;
        obj_sol->suivant = NULL; // On l'isole

        // C. Gérer le Slot du Sac
        Objet* obj_sac_actuel = get_objet_at_index(p->sac, choix_sac);
        
        if (obj_sac_actuel == NULL) {
            // C1. Le slot est vide (ou n'existe pas encore dans la liste chaînée)
            // On ajoute simplement à la fin ou au début selon la logique
            // Pour simplifier avec les listes chaînées : on utilise ajouter_objet_sac
            // Mais attention, ajouter_objet_sac ajoute au début.
            // Pour respecter le "Slot", si c'est vide, on ajoute juste au sac.
            ajouter_objet_sac(p, obj_sol);
            printf(">> Vous avez ramasse : %s\n", obj_sol->nom);
        } 
        else {
            // C2. Le slot est occupé -> ECHANGE (Swap)
            // On retire l'objet du sac de la liste
            Objet* temp = p->sac;
            Objet* prev_sac = NULL;
            while(temp && temp != obj_sac_actuel) {
                prev_sac = temp;
                temp = temp->suivant;
            }
            
            // On remplace le noeud
            if (prev_sac) prev_sac->suivant = obj_sol;
            else p->sac = obj_sol;
            
            obj_sol->suivant = obj_sac_actuel->suivant;
            
            // On jette l'ancien objet au sol
            obj_sac_actuel->suivant = s->objets_au_sol;
            s->objets_au_sol = obj_sac_actuel;
            s->nb_objets_sol++;
            
            printf(">> Echange : [%s] jete, [%s] pris dans le slot %d.\n", 
                   obj_sac_actuel->nom, obj_sol->nom, choix_sac);
        }
    }
}
/* ==================== DÉPLACEMENT ET SALLES ==================== */


void deplacer_personnage(Jeu *jeu, Personnage *p, Direction dir) {
    if (!jeu || !p) return;
    
    int ancienne_salle = p->salle;
    int nouvelle_salle = ancienne_salle;
    
    switch (dir) {
        case NORD:
            if (ancienne_salle <= 4) nouvelle_salle += 12; 
            else nouvelle_salle -= 4;
            break;
        case SUD:
            if (ancienne_salle >= 13) nouvelle_salle -= 12;
            else nouvelle_salle += 4;
            break;
        case EST:
            if (ancienne_salle % 4 == 0) nouvelle_salle -= 3;
            else nouvelle_salle += 1;
            break;
        case OUEST:
            if (ancienne_salle % 4 == 1) nouvelle_salle += 3;
            else nouvelle_salle -= 1;
            break;
        case AUCUNE:
            printf("%s reste dans la salle %d\n", p->nom, ancienne_salle);
            return;
    }
    
    if (nouvelle_salle < 1 || nouvelle_salle > NB_SALLES) {
        printf("Erreur: salle invalide\n");
        return;
    }
    
    if (nouvelle_salle == ancienne_salle) {
        printf("%s reste dans la salle %d\n", p->nom, ancienne_salle);
        return;
    }
    
    retirer_personnage_liste(&jeu->salles[ancienne_salle - 1].personnages_presents, p);
    p->salle = nouvelle_salle;
    ajouter_personnage_liste(&jeu->salles[nouvelle_salle - 1].personnages_presents, p);
    
    printf("%s se deplace vers la salle %d\n", p->nom, nouvelle_salle);
}

void afficher_portes_disponibles(int salle) {
    printf("\nPortes disponibles:\n");
    
    if (salle <= 4) printf("  NORD: Sortie (revient par sud)\n");
    else printf("  NORD: Salle %d\n", salle - 4);
    
    if (salle >= 13) printf("  SUD: Sortie (revient par nord)\n");
    else printf("  SUD: Salle %d\n", salle + 4);
    
    if (salle % 4 == 0) printf("  EST: Sortie (revient par ouest)\n");
    else printf("  EST: Salle %d\n", salle + 1);
    
    if (salle % 4 == 1) printf("  OUEST: Sortie (revient par est)\n");
    else printf("  OUEST: Salle %d\n", salle - 1);
    
    printf("  RESTER: Ne pas bouger\n");
}

/* ==================== COMBAT ==================== */

void afficher_regles_combat() {
    printf("\n=== REGLES DE COMBAT ===\n");
    printf("1. Arme vs Rien -> -1 vie pour chacun\n");
    printf("2. Poison vs Rien -> -1 vie pour la victime\n");
    printf("3. Arme vs Bouclier -> Bouclier protege contre l'arme mais les deux sont utilisées\n");
    printf("4. Poison vs Medicament -> Medicament soigne (poison perdu)\n");
    printf("5. Arme vs Medicament ->  medicament contre les armes\n");
    printf("6. Poison vs Bouclier -> -1 vie \n");
    printf("7. Cle teleportation -> Fin combat, deplacement aleatoire\n");
}

/* Résout un combat entre deux personnages*/
void resoudre_combat(Jeu *jeu, Personnage *attaquant, Objet *obj_att, 
                     Personnage *defenseur, Objet *obj_def) {
    if (!attaquant || !defenseur) return;
    
    printf("\n=== COMBAT : %s vs %s ===\n", attaquant->nom, defenseur->nom);
    
    /* 1. Identification des objets utilisés */
    TypeObjet t_att = obj_att ? obj_att->type : AUCUN_OBJET;
    TypeObjet t_def = obj_def ? obj_def->type : AUCUN_OBJET;
    
    if (t_att != AUCUN_OBJET) printf("%s utilise : %s\n", attaquant->nom, obj_att->nom);
    if (t_def != AUCUN_OBJET) printf("%s utilise : %s\n", defenseur->nom, obj_def->nom);

    /* ================================================================== */
    /* CAS 1 : L'ATTAQUANT (VOUS) UTILISE LA CLÉ                          */
    /* ================================================================== */
    if (t_att == CLE_TELEPORTATION) {
        printf(">>> %s active sa Cle de Teleportation pour fuir !\n", attaquant->nom);
        utiliser_cle_teleportation(jeu, attaquant);
        printf(">>> %s a disparu instantanement !\n", attaquant->nom);
        return; /* Fin du combat immédiate */
    }

    /* ================================================================== */
    /* CAS 2 : LE DÉFENSEUR (L'ENNEMI) UTILISE LA CLÉ                     */
    /* ================================================================== */
    if (t_def == CLE_TELEPORTATION) {
        printf(">>> MAGIE ! %s active sa Cle de Teleportation !\n", defenseur->nom);

        /* L'attaquant perd son objet car il a frappé dans le vide */
        if (t_att != AUCUN_OBJET) {
            printf("... %s frappe dans le vide et perd son objet (%s) !\n", 
                   attaquant->nom, obj_att->nom);
            retirer_objet_sac(attaquant, t_att);
        }

        utiliser_cle_teleportation(jeu, defenseur);
        printf(">>> %s a disparu instantanement ! (0 Degat subi)\n", defenseur->nom);
        return; /* Fin du combat immédiate */
    }

    /* 2. Initialisation des conséquences (Par défaut) */
    int degats_att = 0;
    int degats_def = 0;
    
    bool perte_obj_att = (t_att != AUCUN_OBJET);
    bool perte_obj_def = (t_def != AUCUN_OBJET);

    /* Exception : Le bouclier ne sert à RIEN contre le poison, il n'est donc pas "usé" */
    if (t_att == POISON && t_def == BOUCLIER) {
        perte_obj_def = false; 
    }

    /* 3. LOGIQUE DES DÉGÂTS (Matrice de combat) */
 /**/
    /* 3. LOGIQUE DES DÉGÂTS (Matrice de combat) */
    if (t_att == ARME) {
        if (t_def == ARME) {
            printf(">>> CHOC FRONTAL ! Les deux se blessent et brisent leurs armes !\n");
            degats_att = 1;
            degats_def = 1;
        }
        else if (t_def == POISON) {
            printf(">>> ECHANGE SANGLANT ! %s frappe, mais s'empoisonne !\n", attaquant->nom);
            degats_att = 1; 
            degats_def = 1; 
        }
        else if (t_def == BOUCLIER) {
            printf(">>> BLOQUE ! Le bouclier se brise sous l'impact !\n");
            degats_def = 0;
        }
        /* === AJOUT ICI : Le médicament protège aussi contre l'épée === */
        else if (t_def == MEDICAMENT) {
            printf(">>> PROTECTION ! Le medicament encaisse le coup a votre place.\n");
            degats_def = 0; /* 0 Dégât subi */
            /* Le médicament sera quand même perdu (consommé) à la fin de la fonction */
        }
        /* ============================================================ */
        else { 
            printf(">>> TOUCHE ! %s subit l'attaque de plein fouet.\n", defenseur->nom);
            degats_def = 1;
        }
    }
    else if (t_att == POISON) {
        if (t_def == MEDICAMENT) {
            printf(">>> NEUTRALISE ! Le medicament contre le poison.\n");
            degats_def = 0;
        }
        /* ... le reste du bloc POISON reste inchangé ... */
        else if (t_def == ARME) {
            printf(">>> ECHANGE SANGLANT ! %s empoisonne, mais se fait frapper !\n", attaquant->nom);
            degats_att = 1; 
            degats_def = 1; 
        }
        else {
            printf(">>> EMPOISONNE ! Le gaz contourne la defense.\n");
            degats_def = 1;
        }
    }
    /* CAS C : L'Attaquant n'a RIEN (Mains nues) */
    else if (t_att == AUCUN_OBJET) {
        printf("%s attaque a mains nues !\n", attaquant->nom);
        
        if (t_def == ARME || t_def == POISON) {
             printf(">>> ERREUR FATALE ! L'adversaire riposte avec %s !\n", obj_def->nom);
             printf(">>> %s se fait blesser en approchant.\n", attaquant->nom);
             degats_att = 1; 
             perte_obj_def = true; 
        }
        else if (t_def == BOUCLIER) {
             printf("...inutile contre un bouclier.\n");
             degats_def = 0;
        }
        else {
             printf("...mais l'autre n'a rien non plus.\n");
             degats_att = 0; 
             degats_def = 0; 
        }
    }
    
    /* 4. APPLICATION DES DÉGÂTS */
    if (degats_att > 0) {
        attaquant->indice_vie -= degats_att;
        printf("-> %s perd %d vie (Reste: %d)\n", attaquant->nom, degats_att, attaquant->indice_vie);
    }
    if (degats_def > 0) {
        defenseur->indice_vie -= degats_def;
        printf("-> %s perd %d vie (Reste: %d)\n", defenseur->nom, degats_def, defenseur->indice_vie);
    }

    /* 5. DESTRUCTION DES OBJETS */
    if (perte_obj_att) retirer_objet_sac(attaquant, t_att);
    if (perte_obj_def) retirer_objet_sac(defenseur, t_def);

    /* 6. VÉRIFICATION DE LA MORT ET PILLAGE */
    
    /* Pour l'attaquant */
    if (attaquant->indice_vie <= 0) {
        attaquant->est_vivant = false;
        printf("XXX %s est MORT !\n", attaquant->nom);
        if (attaquant->sac) {
            printf("   > Ses affaires tombent au sol !\n");
            Salle* s = &jeu->salles[attaquant->salle - 1];
            while (attaquant->sac) {
                Objet* obj = attaquant->sac;
                attaquant->sac = obj->suivant;
                attaquant->nb_objets_sac--;
                obj->suivant = s->objets_au_sol;
                s->objets_au_sol = obj;
                s->nb_objets_sol++;
                printf("     + %s\n", obj->nom);
            }
        }
    }

    /* Pour le défenseur */
    if (defenseur->indice_vie <= 0) {
        defenseur->est_vivant = false;
        printf("XXX %s est MORT !\n", defenseur->nom);
        if (defenseur->sac) {
            printf("   > Ses affaires tombent au sol !\n");
            Salle* s = &jeu->salles[defenseur->salle - 1];
            while (defenseur->sac) {
                Objet* obj = defenseur->sac;
                defenseur->sac = obj->suivant;
                defenseur->nb_objets_sac--;
                obj->suivant = s->objets_au_sol;
                s->objets_au_sol = obj;
                s->nb_objets_sol++;
                printf("     + %s\n", obj->nom);
            }
        }
    }
}

void combat_automatique(Jeu* jeu, Personnage *p1, Personnage *p2) {
    if (!p1 || !p2 || !p1->est_vivant || !p2->est_vivant) return;
    
    Personnage *attaquant, *defenseur;
    
    /* 50% de chance pour savoir qui attaque en premier */
    if (rand() % 2 == 0) {
        attaquant = p1; defenseur = p2;
    } else {
        attaquant = p2; defenseur = p1;
    }
    
    /* --- 1. CHOIX DE L'ATTAQUANT --- */
    Objet* obj_att = NULL;
    Objet* obj_att_temp = NULL; /* Pointeur temporaire si c'est le joueur qui crée l'objet */

    if (attaquant->est_joueur) {
        printf("\n>>> C'est le tour de l'ennemi, mais VOUS avez l'initiative !\n");
        printf("Choisissez votre attaque contre %s :\n", defenseur->nom);
        TypeObjet choix = choisir_objet_combat_joueur(attaquant);
        if (choix != AUCUN_OBJET) {
            obj_att_temp = creer_objet(choix);
            obj_att = obj_att_temp;
        }
    } else {
        /* C'est l'IA qui attaque */
        obj_att = choisir_objet_combat_ia(attaquant);
    }

    TypeObjet type_menace = obj_att ? obj_att->type : AUCUN_OBJET;
    
    /* --- 2. CHOIX DU DÉFENSEUR (RÉACTION) --- */
    Objet* obj_def = NULL;
    Objet* obj_def_temp = NULL;

    if (defenseur->est_joueur) {
        printf("\n>>> %s vous attaque avec %s !\n", attaquant->nom, obj_att ? obj_att->nom : "Mains nues");
        printf("Choisissez votre reaction :\n");
        TypeObjet choix = choisir_objet_combat_joueur(defenseur);
        if (choix != AUCUN_OBJET) {
            obj_def_temp = creer_objet(choix);
            obj_def = obj_def_temp;
        }
    } else {
        /* C'est une IA qui se défend */
        obj_def = choisir_objet_reaction_ia(defenseur, type_menace);
    }
    
    /* --- 3. RÉSOLUTION --- */
    resoudre_combat(jeu, attaquant, obj_att, defenseur, obj_def);

    /* Nettoyage des objets temporaires créés pour le joueur */
    if (obj_att_temp) free(obj_att_temp);
    if (obj_def_temp) free(obj_def_temp);
}



TypeObjet choisir_objet_combat_joueur(Personnage *p) {
    /* 1. Si le sac est vide, pas le choix : mains nues */
    if (!p || p->nb_objets_sac == 0) {
        printf("Votre sac est vide. Vous combattez a mains nues.\n");
        return AUCUN_OBJET;
    }
    
    printf("\n=== CHOIX D'OBJET POUR LE COMBAT ===\n");   
    /* 2. On affiche la liste et on mémorise les types */
    TypeObjet types[MAX_OBJETS_SAC];
    Objet* courant = p->sac;
    int i = 1;
    
    while (courant) {
        printf("%d. %s\n", i, courant->nom);
        if (i <= MAX_OBJETS_SAC) {
            types[i-1] = courant->type;
        }
        courant = courant->suivant;
        i++;
    }
    
    /* 3. Boucle de validation stricte (Force un choix valide entre 1 et N) */
    int choix;
    do {
        printf("Votre choix (1-%d): ", p->nb_objets_sac);
        
        if (scanf("%d", &choix) != 1) {
            choix = 0; // Si l'entrée n'est pas un nombre
        }
        while(getchar() != '\n'); // Vider le buffer proprement
        
        if (choix < 1 || choix > p->nb_objets_sac) {
            printf("Choix invalide. Vous devez choisir un objet de votre sac !\n");
        }
        
    } while (choix < 1 || choix > p->nb_objets_sac);
    
    return types[choix - 1];
}

Objet* choisir_objet_combat_ia(Personnage *p) {
    if (!p || p->nb_objets_sac == 0) return NULL;
    
    if (rand() % 100 < 20) return NULL;
    
    int choix = rand() % p->nb_objets_sac;
    Objet* courant = p->sac;
    
    for (int i = 0; i < choix && courant; i++) {
        courant = courant->suivant;
    }
    
    return courant;
}

void utiliser_cle_teleportation(Jeu *jeu, Personnage *p) {
    if (!jeu || !p) return;
    
    /* 1. On retire la clé du sac (Consommable) */
    retirer_objet_sac(p, CLE_TELEPORTATION);
    
    /* 2. Choix d'une nouvelle salle aléatoire (différente de l'actuelle) */
    int ancienne_salle = p->salle;
    int nouvelle_salle;
    do {
        nouvelle_salle = rand() % NB_SALLES + 1;
    } while (nouvelle_salle == ancienne_salle);
    
    /* 3. Déplacement technique (Listes chaînées) */
    /* On le retire de la liste de la salle actuelle */
    retirer_personnage_liste(&jeu->salles[ancienne_salle - 1].personnages_presents, p);
    
    /* On met à jour sa salle */
    p->salle = nouvelle_salle;
    
    /* On l'ajoute à la liste de la nouvelle salle */
    ajouter_personnage_liste(&jeu->salles[nouvelle_salle - 1].personnages_presents, p);
    
    printf("[TELEPORTATION] %s atterrit en Salle %d !\n", p->nom, nouvelle_salle);
}
/* ==================== LOGIQUE DU JEU ==================== */
void tour_joueur(Jeu *jeu) {
    if (!jeu || !jeu->joueur) return;
    
    Salle* salle_actuelle = &jeu->salles[jeu->joueur->salle - 1];
    bool tour_termine = false; // Tant que faux, on reste dans le menu
    
    printf("\n=== TOUR %d - A VOUS DE JOUER ===\n", jeu->tour);
    
    while (!tour_termine) {
        /* 1. Analyse de la situation en temps réel */
        afficher_matrice_chateau(jeu); // Montre la carte et le status
        
        /* Recensement des ennemis */
        int nb_ennemis = 0;
        Personnage* p = salle_actuelle->personnages_presents;
        while (p) {
            if (!p->est_joueur && p->est_vivant) nb_ennemis++;
            p = p->suivant;
        }

        /* 2. Affichage du Menu Contextuel */
        printf("\n--- ACTION REQUISE ---\n");
        
        // Option 1 : Toujours dispo
        printf("1. Gerer Inventaire (Ramasser/Jeter/Voir)\n");
        
        // Option 2 & 3 : Combat (Si ennemis)
        if (nb_ennemis > 0) {
            printf("\033[1;31m2. COMBATTRE (%d ennemi(s) ici !)\033[0m\n", nb_ennemis);
            printf("3. FUIR (Quitter la salle en urgence)\n");
        } 
        // Option 2 & 3 : Mouvement (Si calme)
        else {
            printf("2. Deplacer (Changer de salle)\n");
            printf("3. (Indisponible - Pas d'ennemis)\n");
        }
        
        printf("4. Sauvegarder\n");
        printf("5. QUITTER LE JEU\n");
        printf("Votre choix > ");

        int choix;
        if (scanf("%d", &choix) != 1) choix = 0;
        getchar();

        /* 3. Traitement du Choix */
        switch (choix) {
            case 1:
                /* GESTION INVENTAIRE (Ne termine pas le tour) */
                gerer_inventaire_slots(jeu, jeu->joueur);
                break;

            case 2:
                if (nb_ennemis > 0) {
                    /* === COMBAT LOGIC === */
                    printf("\n\033[1;31m=== LE COMBAT S'ENGAGE ! ===\033[0m\n");
                    
                    // Récupérer la liste des ennemis
                    Personnage* ennemis[NB_PERSONNAGES];
                    int n_enn = 0;
                    Personnage* p_enn = salle_actuelle->personnages_presents;
                    while (p_enn) {
                        if (!p_enn->est_joueur && p_enn->est_vivant) ennemis[n_enn++] = p_enn;
                        p_enn = p_enn->suivant;
                    }

                    bool combat_actif = true;
                    while (combat_actif && jeu->joueur->est_vivant && n_enn > 0) {
                        // Choix adversaire
                        Personnage* adversaire = ennemis[0];
                        if (n_enn > 1) {
                            printf("Qui attaquer ? (1-%d): ", n_enn);
                            int c_adv; if(scanf("%d", &c_adv)!=1)c_adv=1; getchar();
                            if(c_adv > 0 && c_adv <= n_enn) adversaire = ennemis[c_adv-1];
                        }

                        TypeObjet choix_joueur = choisir_objet_combat_joueur(jeu->joueur);
                        Objet* obj_ia = choisir_objet_reaction_ia(adversaire, choix_joueur);
                        Objet* obj_joueur = (choix_joueur != AUCUN_OBJET) ? creer_objet(choix_joueur) : NULL;
                        
                        resoudre_combat(jeu, jeu->joueur, obj_joueur, adversaire, obj_ia);
                        if (obj_joueur) free(obj_joueur);

                        if (choix_joueur == CLE_TELEPORTATION) { combat_actif = false; tour_termine = true; } // TP ends turn
                        else if (!adversaire->est_vivant) {
                            jeu->nb_personnages_vivants--;
                            // Update list
                            n_enn = 0;
                            p_enn = salle_actuelle->personnages_presents;
                            while (p_enn) { if (!p_enn->est_joueur && p_enn->est_vivant) ennemis[n_enn++] = p_enn; p_enn = p_enn->suivant; }
                        }
                        if (choix_joueur == AUCUN_OBJET && !obj_ia) combat_actif = false;
                    }
                    // Fighting consumes energy/time, so usually turn ends? 
                    // Rules say "Combattre (Jusqu'a la mort ou epuisement)". 
                    // Let's say if enemies are dead, you can move. If you ran, turn ends.
                    if (n_enn > 0 && jeu->joueur->est_vivant) {
                        // Combat stopped but enemies alive (exhaustion) -> End Turn
                        tour_termine = true;
                    }
                    // If enemies died, loop continues (you can loot them or move)
                } 
                else {
                    /* DEPLACEMENT (Si pas d'ennemis) */
                    Direction d = choisir_direction_joueur();
                    if (d != AUCUNE) {
                        deplacer_personnage(jeu, jeu->joueur, d);
                        tour_termine = true; 
                    }
                }
                break;

            case 3:
                if (nb_ennemis > 0) {
                    /* FUITE */
                    printf(">>> Tentative de fuite...\n");
                    afficher_portes_disponibles(jeu->joueur->salle);
                    Direction d = choisir_direction_joueur();
                    if (d != AUCUNE) {
                        deplacer_personnage(jeu, jeu->joueur, d);
                        tour_termine = true;
                    }
                } else {
                    printf("Rien a fuir ici.\n");
                }
                break;
                
            case 4:
                sauvegarder_jeu(jeu, "sauvegarde.txt");
                break;

            case 5:
                printf("Fin de la partie demandee.\n");
                jeu->jeu_termine = true;
                tour_termine = true;
                break;

            default:
                printf("Choix invalide.\n");
        }
        
        if (jeu->jeu_termine) break;
    }
}


/* Fonction utilitaire pour que l'IA ramasse des objets */
void ia_ramasser_tout(Jeu *jeu, Personnage *p) {
    if (!jeu || !p) return;
    Salle* s = &jeu->salles[p->salle - 1];

    /* 1. Si sac pas plein : On ramasse tout ce qu'on peut */
    while (s->objets_au_sol && !sac_est_plein(p)) {
        Objet* a_prendre = s->objets_au_sol;
        s->objets_au_sol = a_prendre->suivant; // Detach from floor
        s->nb_objets_sol--;
        
        a_prendre->suivant = NULL;
        ajouter_objet_sac(p, a_prendre);
    }

    /* 2. Si sac plein : elle échange parfois */
    if (sac_est_plein(p) && s->objets_au_sol) {
        // Logique simple : Si j'ai 4 objets, j'en jette 1 au hasard pour prendre celui du sol
        // Cela évite que l'IA reste bloquée avec des objets inutiles
        if (rand() % 100 < 50) { // 50% de chance d'échanger
             Objet* a_prendre = s->objets_au_sol; // Le nouveau
             s->objets_au_sol = a_prendre->suivant;
             s->nb_objets_sol--;

             // On retire le premier objet du sac (le plus vieux)
             Objet* a_jeter = p->sac;
             p->sac = a_jeter->suivant;
             
             // On met le nouveau dans le sac
             a_prendre->suivant = p->sac;
             p->sac = a_prendre;

             // On jette l'ancien au sol
             a_jeter->suivant = s->objets_au_sol;
             s->objets_au_sol = a_jeter;
             s->nb_objets_sol++;
        }
    }
}

void tour_ia_ameliore(Jeu *jeu) {
    if (!jeu) return;
    
    /* 1. Récupération et Mélange */
    Personnage* personnages_ai[NB_PERSONNAGES];
    int nb_perso_ai = 0;
    Personnage* p_ptr = jeu->personnages;
    while (p_ptr) {
        if (!p_ptr->est_joueur && p_ptr->est_vivant) personnages_ai[nb_perso_ai++] = p_ptr;
        p_ptr = p_ptr->suivant;
    }
    for (int i = nb_perso_ai - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        Personnage* temp = personnages_ai[i];
        personnages_ai[i] = personnages_ai[j];
        personnages_ai[j] = temp;
    }
    
    /* 2. Exécution des tours IA */
    for (int i = 0; i < nb_perso_ai; i++) {
        Personnage* p = personnages_ai[i];
        if (!p->est_vivant) continue;
        
        /* A. Ramassage initial */
        ia_ramasser_tout(jeu, p);
        
        Salle* salle = &jeu->salles[p->salle - 1];
        Personnage* autre = salle->personnages_presents;

        /* B. COMBAT INEVITABLE (Si rencontre) */
        while (autre) {
            if (autre != p && autre->est_vivant) {
                
                printf("\n[AGRESSION] %s se jette sur %s (Combat inevitable) !\n", p->nom, autre->nom);
                
                bool combat_actif = true;
                while (combat_actif && p->est_vivant && autre->est_vivant) {
                    
                    /* --- GESTION EPUISEMENT IA --- */
                    if (p->nb_objets_sac == 0 && autre->nb_objets_sac == 0) {
                        printf(">>> TREVE ! %s et %s n'ont plus d'armes. Fin du combat.\n", p->nom, autre->nom);
                        
                        /* IMPORTANT : On génère des objets pour que la salle ne reste pas vide */
                        generer_objets_salle(salle);
                        printf(">>> De nouveaux objets apparaissent au sol !\n");
                        
                        combat_actif = false;
                        break;
                    }
                    
                    combat_automatique(jeu, p, autre);
                    
                    /* Arrêt si téléportation */
                    if (p->salle != salle->id || autre->salle != salle->id) combat_actif = false;
                }
                
                if (!autre->est_vivant) {
                         jeu->nb_personnages_vivants--;
                         printf(">>> %s a tue %s !\n", p->nom, autre->nom);
                         
                         /* --- AJOUT : L'IA ramasse le butin du mort AVANT de bouger --- */
                         ia_ramasser_tout(jeu, p);
                }
                break; 
            }
            autre = autre->suivant;
        }
        
        if (!p->est_vivant) continue;

        /* C. Déplacement systématique (L'IA bouge toujours pour chercher du stuff ailleurs) */
        Direction dir = rand() % 5;
        deplacer_personnage(jeu, p, dir);
        
        /* D. Ramassage à l'arrivée */
        ia_ramasser_tout(jeu, p);
    }
}
void executer_tour(Jeu *jeu) {
    if (!jeu || jeu->jeu_termine) return;
    
    printf("\n=== TOUR %d ===\n", jeu->tour);

    
    /* 1. Tour du joueur */
    tour_joueur(jeu);
    if (jeu->jeu_termine) return;
    
    /* 2. Vérifier si le joueur est mort */
    if (!jeu->joueur->est_vivant) {
        printf("\nVOUS ETES MORT !\n");
        jeu->jeu_termine = true;
        return;
    }
    
    

    
    /* 5. Générer de nouveaux objets dans la salle du joueur */
    Salle* salle_joueur = &jeu->salles[jeu->joueur->salle - 1];
    if (rand() % 100 < 30) { // 30% de chance
        generer_objets_salle(salle_joueur);
        printf("\nDe nouveaux objets sont apparus dans la salle !\n");
    }
    
    /* 6. Tour des autres personnages (IA) */
    printf("\n--- Tour des autres personnages (IA) ---\n");
    tour_ia_ameliore(jeu);
    
    /* 7. Vérifier fin du jeu */
    verifier_fin_jeu(jeu);
    
    /* 8. Affichage final de la carte pour voir les mouvements ennemis */
    if (!jeu->jeu_termine) {
        printf("\n=== FIN DU TOUR %d - SITUATION FINALE ===\n", jeu->tour);
        afficher_matrice_chateau(jeu);
    }
    
    jeu->tour++;
}

void verifier_fin_jeu(Jeu *jeu) {
    if (!jeu) return;
    
    int compte_vivants = 0;
    
    for (int i = 0; i < NB_SALLES; i++) {
        Personnage *p = jeu->salles[i].personnages_presents;
        while (p) {
            if (p->est_vivant) {
                compte_vivants++;
            }
            p = p->suivant;
        }
    }
    
    /* On met à jour la variable globale pour que l'affichage soit correct */
    jeu->nb_personnages_vivants = compte_vivants;

    /* 1. Vérification Défaite */
    if (!jeu->joueur->est_vivant) {
        printf("\n=== GAME OVER ===\n");
        printf("Votre personnage est mort.\n");
        jeu->jeu_termine = true;
        return;
    }
    
    /* 2. Vérification Victoire */
    /* On ne gagne que s'il reste 1 seul survivant (VOUS) */
    if (compte_vivants <= 1) {
        printf("\n==================================================\n");
        printf("             VICTOIRE ! FELICITATIONS !             \n");
        printf("==================================================\n");
        printf("Vous etes le dernier survivant du chateau !\n");
        printf("Tous vos ennemis ont peri.\n");
        jeu->jeu_termine = true;
    }
}

/* ==================== SAUVEGARDE ET CHARGEMENT ==================== */
void sauvegarder_jeu(const Jeu *jeu, const char *fichier) {
    if (!jeu) return;

    FILE *fp = fopen(fichier, "w");
    if (!fp) {
        printf("Erreur: Impossible d'ouvrir %s\n", fichier);
        return;
    }

    /* 1. Globales */
    fprintf(fp, "%d %d %d\n", jeu->tour, jeu->nb_personnages_vivants, jeu->jeu_termine ? 1 : 0);

    /* 2. PERSONNAGES (On scanne les salles pour les trouver tous) */
    int compteur_sauvegarde = 0;
    
    for (int i = 0; i < NB_SALLES; i++) {
        Personnage *p = jeu->salles[i].personnages_presents;
        
        while (p) {
            /* On compte les objets du sac */
            int nb_sac_reel = 0;
            Objet *o = p->sac;
            while(o) { nb_sac_reel++; o = o->suivant; }

            /* On sauvegarde ce personnage */
            fprintf(fp, "PERSO %s %d %d %d %d %d\n", 
                    p->nom, 
                    p->indice_vie, 
                    p->salle, 
                    p->est_joueur ? 1 : 0, 
                    p->est_vivant ? 1 : 0, 
                    nb_sac_reel);

            /* Contenu du sac */
            o = p->sac;
            while (o) {
                fprintf(fp, "%d ", o->type);
                o = o->suivant;
            }
            fprintf(fp, "-1\n"); /* Fin du sac */

            compteur_sauvegarde++;
            p = p->suivant;
        }
    }
    fprintf(fp, "FIN_PERSO\n");

    /* 3. SALLES (Objets au sol) */
    for (int i = 0; i < NB_SALLES; i++) {
        const Salle *s = &jeu->salles[i];
        
        int nb_sol_reel = 0;
        Objet *o = s->objets_au_sol;
        while(o) { nb_sol_reel++; o = o->suivant; }

        fprintf(fp, "SALLE %d %d\n", s->id, nb_sol_reel);
        
        o = s->objets_au_sol;
        while (o) {
            fprintf(fp, "%d ", o->type);
            o = o->suivant;
        }
        fprintf(fp, "-1\n");
    }
    fprintf(fp, "FIN_SALLES\n");

    fclose(fp);
    printf(">>> Sauvegarde terminee.\n");
}

Jeu* charger_jeu(const char *fichier) {
    FILE *fp = fopen(fichier, "r");
    if (!fp) {
        printf("Fichier de sauvegarde introuvable.\n");
        return NULL;
    }

    Jeu *jeu = (Jeu*)malloc(sizeof(Jeu));
    if (!jeu) { fclose(fp); return NULL; }

    /* Initialisation propre */
    jeu->personnages = NULL;
    jeu->joueur = NULL;
    for (int i = 0; i < NB_SALLES; i++) {
        jeu->salles[i].id = i + 1;
        jeu->salles[i].personnages_presents = NULL;
        jeu->salles[i].objets_au_sol = NULL;
        jeu->salles[i].nb_objets_sol = 0;
    }

    /* 1. GLOBALES */
    int term;
    if (fscanf(fp, "%d %d %d", &jeu->tour, &jeu->nb_personnages_vivants, &term) != 3) {
        free(jeu); fclose(fp); return NULL;
    }
    jeu->jeu_termine = (term == 1);

    /* 2. PERSONNAGES */
    char buffer[100];
    
    /* On retire la logique de 'dernier_global' qui causait le bug d'affichage */

    while (fscanf(fp, "%s", buffer) == 1) {
        if (strcmp(buffer, "FIN_PERSO") == 0) break;

        if (strcmp(buffer, "PERSO") == 0) {
            char nom[MAX_NOM];
            int vie, salle, est_j, est_v, nb_sac;
           if (fscanf(fp, "%s %d %d %d %d %d", nom, &vie, &salle, &est_j, &est_v, &nb_sac) != 6) {
    break; 
}

            Personnage *p = creer_personnage(nom, est_j == 1);
            p->indice_vie = vie;
            p->salle = salle;
            p->est_vivant = (est_v == 1);
            p->nb_objets_sac = 0;
            p->suivant = NULL;

            if (p->est_joueur) jeu->joueur = p;

            /* SAC */
            int type_obj;
            Objet *last_obj = NULL;
            while (fscanf(fp, "%d", &type_obj) == 1 && type_obj != -1) {
                Objet *o = creer_objet((TypeObjet)type_obj);
                if (!p->sac) p->sac = o;
                else last_obj->suivant = o;
                last_obj = o;
                p->nb_objets_sac++;
            }

            /* IMPORTANT : On ne l'ajoute QUE dans la salle. 
               On ne touche pas à jeu->personnages ici pour éviter de lier les salles entre elles. */
            if (salle >= 1 && salle <= NB_SALLES) {
                ajouter_personnage_liste(&jeu->salles[salle-1].personnages_presents, p);
            }
            
            /* Pour éviter que jeu->personnages soit NULL (ce qui pourrait planter ailleurs),
               on le fait pointer sur le dernier personnage chargé (souvent le Roi ou le Joueur).
               Ce n'est pas une liste complète, mais ça suffit pour éviter le crash (NULL pointer). */
            jeu->personnages = p; 
        }
    }

    /* 3. SALLES (Objets sol) */
    while (fscanf(fp, "%s", buffer) == 1) {
        if (strcmp(buffer, "FIN_SALLES") == 0) break;
        if (strcmp(buffer, "SALLE") == 0) {
            int id, nb;
            if (fscanf(fp, "%d %d", &id, &nb) != 2) {
    break;
}
            if (id >= 1 && id <= NB_SALLES) {
                int type_obj;
                Objet *last_sol = NULL;
                while (fscanf(fp, "%d", &type_obj) == 1 && type_obj != -1) {
                    Objet *o = creer_objet((TypeObjet)type_obj);
                    if (!jeu->salles[id-1].objets_au_sol) jeu->salles[id-1].objets_au_sol = o;
                    else last_sol->suivant = o;
                    last_sol = o;
                    jeu->salles[id-1].nb_objets_sol++;
                }
            }
        }
    }

    fclose(fp);
    
    /* Sécurité Joueur */
    if (!jeu->joueur && jeu->personnages) {
        jeu->joueur = jeu->personnages;
        jeu->joueur->est_joueur = true;
    }

    return jeu;
}



/* ==================== UTILITAIRES ==================== */

Direction choisir_direction_joueur() {
    printf("\nChoisissez une direction:\n");
    printf("1. Nord\n2. Sud\n3. Est\n4. Ouest\n5. Rester\n");
    printf("Votre choix: ");
    
    int choix;
    if (scanf("%d", &choix) != 1) choix = 5;
    getchar();
    
    switch (choix) {
        case 1: return NORD;
        case 2: return SUD;
        case 3: return EST;
        case 4: return OUEST;
        case 5: return AUCUNE;
        default:
            printf("Choix invalide, direction par defaut: Rester\n");
            return AUCUNE;
    }
}


void clear_screen() {
    #ifdef _WIN32
        if (system("cls")) {}
    #else
        if (system("clear")) {}
    #endif
}
  
void pause_console() {
    printf("\nAppuyez sur Entree pour continuer...");
    getchar();
}

/* Fonction auxiliaire pour dessiner une ligne de séparation adaptée */
void afficher_ligne_separation(int largeur_colonne) {
    for (int i = 0; i < 4; i++) {
        printf("+");
        for (int j = 0; j < largeur_colonne + 2; j++) { // +2 pour les espaces de marge
            printf("-");
        }
    }
    printf("+\n");
}

void afficher_matrice_chateau(const Jeu *jeu) {
    if (!jeu) return;

    /* 1. RECUPERER LA TAILLE DU TERMINAL */
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int largeur_terminal = w.ws_col;

    /* Sécurité : si on n'arrive pas à lire la taille, on prend 80 par défaut */
    if (largeur_terminal < 40) largeur_terminal = 120;

    /* 2. CALCULER LA LARGEUR D'UNE COLONNE */
    /* On a 4 colonnes et 5 barres verticales (|). Donc on retire 5 caractères */
    /* On divise par 4 pour avoir la largeur brute */
    /* On retire 2 pour les espaces de marge à l'intérieur ( " Texte " ) */
    int largeur_col = (largeur_terminal - 5) / 4 - 2;
    
    /* Si c'est trop petit, on fixe un minimum pour que ce soit lisible */
    if (largeur_col < 10) largeur_col = 10;

    printf("\n=== CARTE DU CHATEAU ===\n");
    
    for (int row = 0; row < 4; row++) {
        /* Dessiner la ligne du haut */
        afficher_ligne_separation(largeur_col);
        
        /* --- Ligne A : Numéro de salle --- */
        for (int col = 0; col < 4; col++) {
            int salle_id = row * 4 + col + 1;
            char label[50];
            
            if (jeu->joueur->salle == salle_id)
                snprintf(label, 50, "S.%d (VOUS)", salle_id); // Texte plus court
            else
                snprintf(label, 50, "Salle %d", salle_id);
                
            /* %-*.*s permet de :
               - : aligner à gauche
               * : utiliser 'largeur_col' comme largeur totale
               .* : utiliser 'largeur_col' comme limite max (coupe si trop long)
            */
            printf("| %-*.*s ", largeur_col, largeur_col, label);
        }
        printf("|\n");
        
        /* --- Ligne B : Personnages --- */
        for (int col = 0; col < 4; col++) {
            int salle_id = row * 4 + col + 1;
            char buffer[100] = ""; 
            
    
            /* ... début de la boucle ... */
            /* ... début de la boucle ... */
Personnage *p = jeu->salles[salle_id-1].personnages_presents;
while (p) {
    /* AJOUT DE CETTE CONDITION : On n'affiche QUE les vivants */
    if (p->est_vivant) { 
        if (strlen(buffer) > 0) strcat(buffer, ",");
        
        char info[20];
        snprintf(info, 20, "%.4s(%d)", p->nom, p->indice_vie);
        
        if (strlen(buffer) + strlen(info) < 95) {
            strcat(buffer, info);
        }
    }
    p = p->suivant;
}

            printf("| %-*.*s ", largeur_col, largeur_col, buffer);
        }
        printf("|\n");
    }
    /* Ligne de fermeture finale */
    afficher_ligne_separation(largeur_col);
    printf("\n=== VOTRE STATUS ===\n");
    printf("VIE : %d  |  SAC (%d/%d) : ", 
           jeu->joueur->indice_vie, 
           jeu->joueur->nb_objets_sac, 
           MAX_OBJETS_SAC);

    Objet* o = jeu->joueur->sac;
    if (!o) {
        printf("(Vide)");
    } else {
        while (o) {
            /* On affiche les objets les uns à la suite des autres */
            printf("[%s] ", o->nom);
            o = o->suivant;
        }
    }
    printf("\n====================\n");
}
Objet* choisir_objet_reaction_ia(Personnage *p, TypeObjet menace) {
    if (!p || !p->sac || p->nb_objets_sac == 0) return NULL;

    Objet* courant = p->sac;

    /* STRATEGIE DE L'IA */
    
    /* CAS 1 : TU L'ATTAQUES AVEC UNE ARME */
    if (menace == ARME) {
        /* L'IA cherche une ARME pour riposter (Dégâts mutuels) */
        while (courant) {
            if (courant->type == ARME) return courant;
            courant = courant->suivant;
        }
        /* Sinon, elle cherche un BOUCLIER pour survivre */
        courant = p->sac;
        while (courant) {
            if (courant->type == BOUCLIER) return courant;
            courant = courant->suivant;
        }
    }
    
    /* CAS 2 : TU L'ATTAQUES AVEC DU POISON */
    else if (menace == POISON) {
        /* L'IA cherche un MEDICAMENT absolument */
        courant = p->sac;
        while (courant) {
            if (courant->type == MEDICAMENT) return courant;
            courant = courant->suivant;
        }
        /* Sinon, elle essaie de te tuer avec une ARME avant de mourir */
        courant = p->sac;
        while (courant) {
            if (courant->type == ARME) return courant;
            courant = courant->suivant;
        }
    }
    
    /* CAS 3 : TU ES A MAINS NUES (Ou menace inconnue) */
    else {
        /* Elle en profite pour t'achever avec une ARME ou du POISON */
        courant = p->sac;
        while (courant) {
            if (courant->type == ARME || courant->type == POISON) return courant;
            courant = courant->suivant;
        }
    }

    /* CAS 4 : Si elle n'a pas l'objet idéal, elle prend un truc au hasard */
    int r = rand() % p->nb_objets_sac;
    courant = p->sac;
    for(int i=0; i<r; i++) if(courant->suivant) courant = courant->suivant;
    
    return courant;
}