#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define NB_PRODUITS    4
#define TAILLE         50          
#define LARGEUR        48          
#define FICHIER_VENTES "vente.dat" 
#define FICHIER_AUTH   "auth.txt"  
typedef struct {
    char nom[30];
    int  prix;        /* en centimes : 40 = 0.40 EUR */
    int  quantite;    /* nombre d'unites vendues     */
} Produit;

/* ---------- Prototypes ---------- */
void initialiserProduits(Produit produits[]);
void chargerDonnees(Produit produits[]);
void sauvegarderDonnees(Produit produits[]);

void afficherMenuClient(Produit produits[]);
void afficherMenuAdmin(void);
void acheter(Produit produits[], int index);

int  compteExiste(void);
void creerCompte(void);
int  authentifier(void);
void changerMotDePasse(void);

void espaceAdmin(Produit produits[]);
void etatCaisse(Produit produits[]);
void rapportVentes(Produit produits[]);
void reinitialiser(Produit produits[]);

/* utilitaires */
void viderBuffer(void);
void lireLigne(char *buf, int taille, FILE *src);
void attendre(void);
void ligneBordure(void);
void ligneVide(void);
void ligneCentree(const char *texte);
void ligneTexte(const char *texte);
void ligneProduit(int num, const char *nom, int prixCentimes);

/* ============================================================
   Programme principal
   ============================================================ */
int main(void)
{
    Produit produits[NB_PRODUITS];
    int choix;

    initialiserProduits(produits);  /* noms + prix de base */
    chargerDonnees(produits);        /* recupere les ventes precedentes */

    do {
        afficherMenuClient(produits);

        /* lecture du choix avec controle de saisie */
        int nbLus = scanf("%d", &choix);
        if (nbLus == EOF) {        /* entree fermee (EOF) : on quitte proprement */
            choix = 0;
            continue;
        }
        if (nbLus != 1) {
            viderBuffer();
            printf("\n[Erreur] Saisie invalide : entrez un nombre.\n");
            choix = -1;            /* valeur neutre pour relancer la boucle */
            continue;
        }
        viderBuffer();

        if (choix >= 1 && choix <= NB_PRODUITS) {
            acheter(produits, choix - 1);
        } else if (choix == 999) {
            espaceAdmin(produits);
        } else if (choix == 0) {
            printf("\nMerci de votre visite. A bientot !\n");
        } else {
            printf("\n[Erreur] Choix invalide. Reessayez.\n");
        }

    } while (choix != 0);

    return 0;
}

/* ============================================================
   Initialisation et fichiers de donnees
   ============================================================ */

/* Definit les 4 boissons et leurs prix (en centimes). */
void initialiserProduits(Produit produits[])
{
    strcpy(produits[0].nom, "Cafe Court");      produits[0].prix = 40;
    strcpy(produits[1].nom, "The a la Menthe");  produits[1].prix = 50;
    strcpy(produits[2].nom, "Chocolat Chaud");   produits[2].prix = 60;
    strcpy(produits[3].nom, "Cappuccino");       produits[3].prix = 80;

    for (int i = 0; i < NB_PRODUITS; i++)
        produits[i].quantite = 0;
}

/* Lit les ventes enregistrees (fread). On ne recopie que les
   quantites : les noms et prix restent ceux du programme. */
void chargerDonnees(Produit produits[])
{
    Produit temp[NB_PRODUITS];
    FILE *f = fopen(FICHIER_VENTES, "rb");

    if (f == NULL)
        return;                    /* premier lancement : aucun fichier */

    if (fread(temp, sizeof(Produit), NB_PRODUITS, f) == NB_PRODUITS) {
        for (int i = 0; i < NB_PRODUITS; i++)
            if (temp[i].quantite >= 0)
                produits[i].quantite = temp[i].quantite;
    }
    fclose(f);
}

/* Enregistre l'etat des ventes dans vente.dat (fwrite). */
void sauvegarderDonnees(Produit produits[])
{
    FILE *f = fopen(FICHIER_VENTES, "wb");

    if (f == NULL) {
        printf("[Erreur] Ecriture impossible dans %s.\n", FICHIER_VENTES);
        return;
    }
    fwrite(produits, sizeof(Produit), NB_PRODUITS, f);
    fclose(f);
}

/* ============================================================
   Interface client : affichage du menu et cycle de vente
   ============================================================ */

void afficherMenuClient(Produit produits[])
{
    printf("\n");
    ligneBordure();
    ligneCentree("BIENVENUE AU DISTRIBUTEUR AUTOMATIQUE");
    ligneBordure();
    ligneVide();
    for (int i = 0; i < NB_PRODUITS; i++)
        ligneProduit(i + 1, produits[i].nom, produits[i].prix);
    ligneVide();
    ligneTexte("0. Quitter le programme");
    ligneTexte("999. ESPACE ADMINISTRATION");
    ligneVide();
    ligneBordure();
    printf("\nSaisissez votre choix : ");
}

/* Cycle de vente complet pour un produit donne. */
void acheter(Produit produits[], int index)
{
    int   prix   = produits[index].prix;   /* en centimes */
    int   insere = 0;                       /* montant insere en centimes */
    float piece;
    int   centimes;

    printf("\n--- Achat : %s (%.2f EUR) ---\n",
           produits[index].nom, prix / 100.0);
    printf("Pieces acceptees : 0.10  0.20  0.50  1  2  (EUR)\n\n");

    do {
        printf("Inserez une piece (reste a payer %.2f EUR) : ",
               (prix - insere) / 100.0);

        int nbLus = scanf("%f", &piece);
        if (nbLus == EOF) {        /* entree fermee : on annule l'achat en cours */
            printf("\n[Fin d'entree] Achat annule.\n");
            return;
        }
        if (nbLus != 1) {
            viderBuffer();
            printf("[Erreur] Veuillez entrer une valeur numerique.\n");
            continue;
        }
        viderBuffer();

        /* conversion en centimes avec arrondi (0.50 -> 50) */
        centimes = (int)(piece * 100 + 0.5);

        if (centimes == 10 || centimes == 20 || centimes == 50 ||
            centimes == 100 || centimes == 200) {
            insere += centimes;
            if (insere < prix)
                printf("  -> Insere : %.2f EUR | Reste a payer : %.2f EUR\n",
                       insere / 100.0, (prix - insere) / 100.0);
        } else {
            printf("[Erreur] Piece refusee. Utilisez 0.10, 0.20, 0.50, 1 ou 2 EUR.\n");
        }

    } while (insere < prix);

    /* Finalisation : preparation et rendu de monnaie */
    printf("\nPreparation en cours...\n");
    if (insere > prix)
        printf("Rendu de monnaie : %.2f EUR\n", (insere - prix) / 100.0);
    else
        printf("Montant exact, aucun rendu de monnaie.\n");
    printf("Votre %s est prete. Bonne degustation !\n", produits[index].nom);

    /* Statistiques : on incremente le compteur et on sauvegarde */
    produits[index].quantite++;
    sauvegarderDonnees(produits);

    attendre();
}

/* ============================================================
   Gestion du compte administrateur (fichier auth.txt)
   ============================================================ */

/* Renvoie 1 si le compte admin existe deja (fichier present). */
int compteExiste(void)
{
    FILE *f = fopen(FICHIER_AUTH, "r");
    if (f != NULL) {
        fclose(f);
        return 1;
    }
    return 0;
}

/* Premier acces uniquement : creation du nom d'utilisateur + mot de passe. */
void creerCompte(void)
{
    char user[TAILLE], pass[TAILLE];
    FILE *f;

    printf("\n=== Premiere utilisation : creation du compte administrateur ===\n");
    printf("Nom d'utilisateur : ");
    lireLigne(user, TAILLE, stdin);
    printf("Mot de passe      : ");
    lireLigne(pass, TAILLE, stdin);

    f = fopen(FICHIER_AUTH, "w");
    if (f == NULL) {
        printf("[Erreur] Creation du compte impossible.\n");
        return;
    }
    fprintf(f, "%s\n%s\n", user, pass);
    fclose(f);
    printf("Compte cree avec succes !\n");
}

/* Acces suivants : on verifie uniquement le mot de passe. */
int authentifier(void)
{
    char userFichier[TAILLE], passFichier[TAILLE], saisie[TAILLE];
    FILE *f = fopen(FICHIER_AUTH, "r");

    if (f == NULL)
        return 0;
    lireLigne(userFichier, TAILLE, f);   /* ligne 1 : utilisateur */
    lireLigne(passFichier, TAILLE, f);   /* ligne 2 : mot de passe */
    fclose(f);

    printf("\n=== ESPACE ADMINISTRATION - Authentification ===\n");
    printf("Mot de passe : ");
    lireLigne(saisie, TAILLE, stdin);

    if (strcmp(saisie, passFichier) == 0) {
        printf("Acces autorise. Bonjour %s !\n", userFichier);
        return 1;
    }
    printf("[Erreur] Mot de passe incorrect. Acces refuse.\n");
    return 0;
}

/* Modification du mot de passe (verifie l'ancien avant). */
void changerMotDePasse(void)
{
    char user[TAILLE], passActuel[TAILLE], saisie[TAILLE], nouveau[TAILLE];
    FILE *f = fopen(FICHIER_AUTH, "r");

    if (f == NULL)
        return;
    lireLigne(user, TAILLE, f);
    lireLigne(passActuel, TAILLE, f);
    fclose(f);

    printf("\n=== Changement du mot de passe ===\n");
    printf("Mot de passe actuel : ");
    lireLigne(saisie, TAILLE, stdin);

    if (strcmp(saisie, passActuel) != 0) {
        printf("[Erreur] Mot de passe actuel incorrect.\n");
        return;
    }

    printf("Nouveau mot de passe : ");
    lireLigne(nouveau, TAILLE, stdin);

    f = fopen(FICHIER_AUTH, "w");
    if (f == NULL) {
        printf("[Erreur] Ecriture impossible.\n");
        return;
    }
    fprintf(f, "%s\n%s\n", user, nouveau);
    fclose(f);
    printf("Mot de passe modifie avec succes !\n");
}

/* ============================================================
   Espace administration : menu et fonctions
   ============================================================ */

void espaceAdmin(Produit produits[])
{
    int choix;

    /* Premier acces : creation du compte puis acces direct.
       Acces suivants : l'acces est toujours protege par mot de passe. */
    if (!compteExiste()) {
        creerCompte();
    } else if (!authentifier()) {
        return;                    /* mot de passe faux : retour menu client */
    }

    do {
        afficherMenuAdmin();

        int nbLus = scanf("%d", &choix);
        if (nbLus == EOF) {        /* entree fermee : retour au menu client */
            choix = 0;
            continue;
        }
        if (nbLus != 1) {
            viderBuffer();
            printf("\n[Erreur] Saisie invalide.\n");
            choix = -1;
            continue;
        }
        viderBuffer();

        switch (choix) {
            case 1: etatCaisse(produits);     break;
            case 2: rapportVentes(produits);  break;
            case 3: changerMotDePasse();      attendre(); break;
            case 4: reinitialiser(produits);  break;
            case 0: printf("\nRetour au menu client...\n"); break;
            default: printf("\n[Erreur] Choix invalide.\n");
        }

    } while (choix != 0);
}

/* 1. Etat de la caisse : chiffre d'affaires total. */
void etatCaisse(Produit produits[])
{
    int totalCentimes = 0;

    for (int i = 0; i < NB_PRODUITS; i++)
        totalCentimes += produits[i].prix * produits[i].quantite;

    printf("\n=== ETAT DE LA CAISSE ===\n");
    printf("Chiffre d'affaires total : %.2f EUR\n", totalCentimes / 100.0);
    attendre();
}

/* 2. Rapport des ventes : quantites vendues par produit. */
void rapportVentes(Produit produits[])
{
    int total = 0;

    printf("\n=== RAPPORT DES VENTES ===\n");
    for (int i = 0; i < NB_PRODUITS; i++) {
        printf("  %-18s : %d vendu(s)\n",
               produits[i].nom, produits[i].quantite);
        total += produits[i].quantite;
    }
    printf("  ----------------------------------\n");
    printf("  Total boissons vendues : %d\n", total);
    attendre();
}

/* 4. Reinitialisation : remet la caisse et les ventes a zero. */
void reinitialiser(Produit produits[])
{
    char confirmation[TAILLE];

    printf("\nReinitialiser la caisse ET les ventes ? (oui/non) : ");
    lireLigne(confirmation, TAILLE, stdin);

    if (strcmp(confirmation, "oui") == 0) {
        for (int i = 0; i < NB_PRODUITS; i++)
            produits[i].quantite = 0;
        sauvegarderDonnees(produits);
        printf("Machine reinitialisee : caisse et ventes remises a zero.\n");
    } else {
        printf("Operation annulee.\n");
    }
    attendre();
}

/* ============================================================
   Menu administration (affichage)
   ============================================================ */
void afficherMenuAdmin(void)
{
    printf("\n");
    printf("---------- MENU ADMINISTRATION ----------\n\n");
    printf("  1. Etat de la caisse        (Chiffre d'affaires)\n");
    printf("  2. Rapport des ventes       (Quantites vendues)\n");
    printf("  3. Gestion du compte        (Changer le mot de passe)\n");
    printf("  4. Reinitialiser la machine (Caisse et ventes)\n\n");
    printf("  0. Retour au menu client\n\n");
    printf("-----------------------------------------\n");
    printf("Saisissez votre choix : ");
}

/* ============================================================
   Fonctions utilitaires
   ============================================================ */

/* Vide le tampon clavier jusqu'a la fin de la ligne. */
void viderBuffer(void)
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { }
}

/* Lit une ligne (fgets) depuis un flux et retire le '\n' final. */
void lireLigne(char *buf, int taille, FILE *src)
{
    if (fgets(buf, taille, src) != NULL)
        buf[strcspn(buf, "\n")] = '\0';
    else
        buf[0] = '\0';
}

/* Pause portable : attend que l'utilisateur appuie sur Entree. */
void attendre(void)
{
    printf("\nAppuyez sur Entree pour continuer...");
    viderBuffer();
}

/* --- Dessin du cadre du menu client (largeur fixe) --- */
void ligneBordure(void)
{
    for (int i = 0; i < LARGEUR + 2; i++)
        putchar('*');
    putchar('\n');
}

void ligneVide(void)
{
    putchar('*');
    for (int i = 0; i < LARGEUR; i++)
        putchar(' ');
    printf("*\n");
}

void ligneCentree(const char *texte)
{
    int len = (int)strlen(texte);
    int gauche = (LARGEUR - len) / 2;
    int droite = LARGEUR - len - gauche;

    putchar('*');
    for (int i = 0; i < gauche; i++) putchar(' ');
    printf("%s", texte);
    for (int i = 0; i < droite; i++) putchar(' ');
    printf("*\n");
}

void ligneTexte(const char *texte)
{
    int len = (int)strlen(texte);
    printf("*  %s", texte);                 /* marge de 2 espaces a gauche */
    for (int i = len + 2; i < LARGEUR; i++)
        putchar(' ');
    printf("*\n");
}

void ligneProduit(int num, const char *nom, int prixCentimes)
{
    char gauche[40], droite[16];
    int  marge = 2, nbPoints;

    sprintf(gauche, "%d. %s", num, nom);
    sprintf(droite, "%.2f EUR", prixCentimes / 100.0);

    nbPoints = LARGEUR - 2 * marge - (int)strlen(gauche) - (int)strlen(droite) - 2;
    if (nbPoints < 1) nbPoints = 1;

    putchar('*');
    for (int i = 0; i < marge; i++) putchar(' ');
    printf("%s ", gauche);
    for (int i = 0; i < nbPoints; i++) putchar('.');
    printf(" %s", droite);
    for (int i = 0; i < marge; i++) putchar(' ');
    printf("*\n");
}
    
    
