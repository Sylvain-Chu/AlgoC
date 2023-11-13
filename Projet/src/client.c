/*
 * SPDX-FileCopyrightText: 2021 John Samuel
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "client.h"
#include "bmp.h"


int envoie_operateur_numeros(int socketfd, char* json) {
    char data[1024];
    memset(data, 0, sizeof(data));

    strcpy(data, json);
    write(socketfd, data, strlen(data));


    printf("Message envoyé: %s\n", data);


    return 0;
}

int envoie_json(int socketfd, char *json) {
    char data[1024];
    memset(data, 0, sizeof(data));
    strcpy(data, json);

    int write_status = write(socketfd, data, strlen(data));
    if (write_status < 0) {
        perror("erreur ecriture");
        exit(EXIT_FAILURE);
    }
    printf("Message envoyé: %s\n", data);
    return 0;
}

int envoie_recois_message(int socketfd) {

    char data[1024];
    // la réinitialisation de l'ensemble des données
    memset(data, 0, sizeof(data));

    // Demandez à l'utilisateur d'entrer un message
    char message[1024];
    printf("Votre message (max 1000 caracteres): ");
    fgets(message, sizeof(message), stdin);
    strcpy(data, "message:");
    strcat(data, message);

    envoie_json(socketfd, data);


//    int write_status = write(socketfd, data, strlen(data));
//    if (write_status < 0) {
//        perror("erreur ecriture");
//        exit(EXIT_FAILURE);
//    }

    // la réinitialisation de l'ensemble des données
    memset(data, 0, sizeof(data));

    // lire les données de la socket
    int read_status = read(socketfd, data, sizeof(data));
    if (read_status < 0) {
        perror("erreur lecture");
        return -1;
    }

    printf("Message envoyé: %s\n", data);

    return 0;
}

void analyse(char *pathname, char *data, int nombre_couleurs) {
    // compte de couleurs
    couleur_compteur *cc = analyse_bmp_image(pathname);

    int count;
    strcpy(data, "");

    char temp_string[nombre_couleurs];

    // choisir n couleurs
    for (count = 1; count < nombre_couleurs + 1 && cc->size - count > 0; count++) {
        if (cc->compte_bit == BITS32) {
            sprintf(temp_string, "#%02x%02x%02x,", cc->cc.cc24[cc->size - count].c.rouge,
                    cc->cc.cc32[cc->size - count].c.vert, cc->cc.cc32[cc->size - count].c.bleu);
        }
        if (cc->compte_bit == BITS24) {
            sprintf(temp_string, "#%02x%02x%02x,", cc->cc.cc32[cc->size - count].c.rouge,
                    cc->cc.cc32[cc->size - count].c.vert, cc->cc.cc32[cc->size - count].c.bleu);
        }
        strcat(data, temp_string);
    }

    data[strlen(data) - 1] = '\0';

}

char *get_couleurs_path(int socketfd, char *pathname, int nombre_couleurs) {
    char *data = malloc(1024);
    if (data == NULL) {
        return NULL;
    }
    memset(data, 0, 1024);
    analyse(pathname, data, nombre_couleurs);
    return data;
}

int envoie_couleurs(int socketfd, char *json) {
    char data[1024];
    memset(data, 0, sizeof(data));
    strcpy(data, json);

    int write_status = write(socketfd, data, strlen(data));
    if (write_status < 0) {
        perror("erreur ecriture");
        exit(EXIT_FAILURE);
    }

    return 0;
}

int envoie_balises(int socketfd, char *json) {
    char data[1024];
    memset(data, 0, sizeof(data));
    strcpy(data, json);

    int write_status = write(socketfd, data, strlen(data));
    if (write_status < 0) {
        perror("erreur ecriture");
        exit(EXIT_FAILURE);
    }

    return 0;
}

int envoie_nom_de_client(int socketfd) {
    char data[1024];
    // la réinitialisation de l'ensemble des données
    memset(data, 0, sizeof(data));

    // Demandez à l'utilisateur d'entrer un message
    char nom[1024];
    gethostname(&nom[0], sizeof(data));

    strcpy(data, "nom: ");
    strcat(data, nom);

    int write_status = write(socketfd, data, strlen(data));
    if (write_status < 0) {
        perror("erreur ecriture");
        exit(EXIT_FAILURE);
    }

    // la réinitialisation de l'ensemble des données
    memset(data, 0, sizeof(data));

    // lire les données de la socket
    int read_status = read(socketfd, data, sizeof(data));
    if (read_status < 0) {
        perror("erreur lecture");
        return -1;
    }

    printf("%s\n", data);

    return 0;
}

char* jsonSerialize(char *code, char *valeurs[], int valeurs_count) {
    char json[1024];
    snprintf(json, sizeof(json), "{\"code\":\"%s\",\"valeurs\":[", code);

    for (int i = 0; i < valeurs_count; i++) {
        strcat(json, "\"");
        strcat(json, valeurs[i]);
        strcat(json, "\"");
        if (i < valeurs_count - 1) {
            strcat(json, ",");
        }
    }

    strcat(json, "]}");

    return strdup(json);
}

int main(int argc, char **argv) {
    int socketfd;
    struct sockaddr_in server_addr;
    if (argc < 2) {
        printf("usage: ./client nom/message/couleurs [additional argument]\n");
        return (EXIT_FAILURE);
    }
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Server details (address and port)
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Connect to the server
    int connect_status = connect(socketfd, (struct sockaddr *) &server_addr, sizeof(server_addr));

    if (connect_status < 0) {
        perror("connection serveur");
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "nom") == 0) {
        envoie_nom_de_client(socketfd);
    } else if (strcmp(argv[1], "message") == 0) {
        envoie_recois_message(socketfd);
    } else if (strcmp(argv[1], "calcul") == 0) {
        if (argc < 4) {
            printf("usage: ./client calcul [additional argument]\n");
            close(socketfd);
            return (EXIT_FAILURE);
        }
        //mettre toutes les valeurs dans un tableau
        char *valeurs[argc - 2];
        for (int i = 2; i < argc; i++) {
            valeurs[i - 2] = argv[i];
        }
        char *json = jsonSerialize(argv[1], valeurs, argc - 2);
        envoie_json(socketfd, json);
    } else if (strcmp(argv[1], "couleurs") == 0) {
        int nombre_de_couleurs = atoi(argv[2]);
        if (nombre_de_couleurs <= 0) {
            fprintf(stderr, "Le nombre de couleurs doit être supérieur à zéro.\n");
            return 1;
        }
        if (nombre_de_couleurs > argc - 3) {
            fprintf(stderr, "Pas assez d'arguments pour le nombre de couleurs spécifié.\n");
            return 1;
        }
        char *json = jsonSerialize(argv[1], &argv[3], argc - 3);
        envoie_json(socketfd, json);
    } else if (strcmp(argv[1], "balises") == 0) {
        int nombre_de_balises = atoi(argv[2]);
        if (nombre_de_balises <= 0) {
            fprintf(stderr, "Le nombre de balises doit être supérieur à zéro.\n");
            return 1;
        }

        if (nombre_de_balises > argc - 3) {
            fprintf(stderr, "Pas assez d'arguments pour le nombre de balises spécifié.\n");
            return 1;
        }

        char *json = jsonSerialize(argv[1], &argv[3], argc - 3);
        envoie_json(socketfd, json);
    } else if (strcmp(argv[1], "image") == 0) {
        char *json = jsonSerialize(argv[1], &argv[2], argc - 2);
        char couleurs = *get_couleurs_path(socketfd, argv[2], atoi(argv[3]));


        printf("couleur : %c\n", couleurs);

        envoie_json(socketfd, json);
    } else {
        envoie_nom_de_client(socketfd);
    }

    close(socketfd);
}
