/*
 * SPDX-FileCopyrightText: 2021 John Samuel
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */

#include <math.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "serveur.h"

int socketfd;
typedef struct {
    char *code;
    char **valeurs;
} JSONData;

int visualize_plot() {
    const char *browser = "firefox";

    char command[256];
    snprintf(command, sizeof(command), "%s %s", browser, svg_file_path);

    int result = system(command);

    if (result == 0) {
        printf("SVG file opened in %s.\n", browser);
    } else {
        printf("Failed to open the SVG file.\n");
    }

    return 0;
}

double degreesToRadians(double degrees) {
    return degrees * M_PI / 180.0;
}

int plot(char *data) {

    int nombre_couleurs;
    char code[10];
    sscanf(data, "%s", code);

    if (strcmp(code, "couleurs:") == 0) {
        sscanf(data, "couleurs: %d", &nombre_couleurs);

        double angles[nombre_couleurs];
        memset(angles, 0, sizeof(angles));

        if (nombre_couleurs <= 0 || nombre_couleurs > 30) {
            printf("Nombre de couleurs non valide. Limite de 30 couleurs.\n");
            return (EXIT_FAILURE);
        }

        char couleurs[30][8]; // Tableau de chaînes pour stocker les couleurs au format "#RRGGBB"

        char *start = strchr(data, ',');

        if (start != NULL) {
            start++; // Passer à la première couleur
            for (int i = 0; i < nombre_couleurs; i++) {
                char color[8];
                if (i < nombre_couleurs - 1) {
                    if (sscanf(start, "%7[^,],", color) == 1) {
                        strncpy(couleurs[i], color, sizeof(couleurs[i]));
                        start += 8; // Avancer de 8 caractères (taille de la couleur)
                    } else {
                        printf("Erreur de format dans les couleurs.\n");
                        return (EXIT_FAILURE);
                    }
                } else {
                    if (sscanf(start, "%7[^,]", color) == 1) {
                        strncpy(couleurs[i], color, sizeof(couleurs[i]));
                    } else {
                        printf("Erreur de format dans les couleurs.\n");
                        return (EXIT_FAILURE);
                    }
                }
            }

            FILE *svg_file = fopen(svg_file_path, "w");
            if (svg_file == NULL) {
                perror("Error opening file");
                return 1;
            }

            fprintf(svg_file, "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n");
            fprintf(svg_file, "<svg width=\"400\" height=\"400\" xmlns=\"http://www.w3.org/2000/svg\">\n");
            fprintf(svg_file, "  <rect width=\"100%%\" height=\"100%%\" fill=\"#ffffff\" />\n");

            double center_x = 200.0;
            double center_y = 200.0;
            double radius = 150.0;
            double start_angle = -90.0;

            for (int i = 0; i < nombre_couleurs; i++) {
                angles[i] = 360.0 / nombre_couleurs;

                double end_angle = start_angle + angles[i];

                double start_angle_rad = degreesToRadians(start_angle);
                double end_angle_rad = degreesToRadians(end_angle);

                double x1 = center_x + radius * cos(start_angle_rad);
                double y1 = center_y + radius * sin(start_angle_rad);
                double x2 = center_x + radius * cos(end_angle_rad);
                double y2 = center_y + radius * sin(end_angle_rad);

                fprintf(svg_file, "  <path d=\"M%.2f,%.2f A%.2f,%.2f 0 0,1 %.2f,%.2f L%.2f,%.2f Z\" fill=\"%s\" />\n",
                        x1, y1, radius, radius, x2, y2, center_x, center_y, couleurs[i]);

                start_angle = end_angle;
            }

            fprintf(svg_file, "</svg>\n");
            fclose(svg_file);
//            visualize_plot();
        }

    } else {
        printf("Message recu: %s\n", data);
    }

    return 0;
}

int renvoie_message(int client_socket_fd, char *data) {
    int data_size = write(client_socket_fd, (void *) data, strlen(data));

    if (data_size < 0) {
        perror("erreur ecriture");
        return (EXIT_FAILURE);
    }
    return (EXIT_SUCCESS);
}

int renvoie_nom(int client_socket_fd, char *data) {
    int data_size = write(client_socket_fd, (void *) data, strlen(data));

    if (data_size < 0) {
        perror("erreur ecriture");
        return (EXIT_FAILURE);
    }
    return (EXIT_SUCCESS);
}

int recois_envoie_message(int client_socket_fd, char data[1024]) {
    /*
     * extraire le code des données envoyées par le client.
     * Les données envoyées par le client peuvent commencer par le mot "message :" ou un autre mot.
     */
    char code[10];
    sscanf(data, "%s", code);

    // Si le message commence par le mot: 'message:'
    if (strcmp(code, "message:") == 0) {
        printf("Message recu: %s\n", data);
        renvoie_message(client_socket_fd, data);
    } else if (strcmp(code, "nom:") == 0) {
        printf("%s\n", data);
        renvoie_nom(client_socket_fd, data);
    } else {
        printf("Message recu: %s\n", data);
        plot(data);
    }

    return (EXIT_SUCCESS);
}

int recois_numeros_calcule(int client_socket_fd, char data[1024]) {
    char code[10];
    sscanf(data, "%s", code);

    if (strcmp(code, "calcul:") == 0) {
        char operator;
        double num1, num2;
        sscanf(data, "calcul: %c %lf %lf", &operator, &num1, &num2);

        double result;

        // Effectuer le calcul en fonction de l'opérateur
        switch (operator) {
            case '+':
                result = num1 + num2;
                break;
            case '-':
                result = num1 - num2;
                break;
            case '*':
                result = num1 * num2;
                break;
            case '/':
                if (num2 != 0) {
                    result = num1 / num2;
                } else {
                    renvoie_message(client_socket_fd, "Division par zéro.");
                    return (EXIT_FAILURE);
                }
                break;
            default:
                renvoie_message(client_socket_fd, "Opérateur non pris en charge.");
                return (EXIT_FAILURE);
        }

        char result_str[1024];
        snprintf(result_str, sizeof(result_str), "Résultat : %.2f", result);

        printf("Calcul : %.2f\n", result);
    } else {
        printf("Message recu: %s\n", data);
    }

    return (EXIT_SUCCESS);
}

int recois_couleurs(int client_socket_fd, char data[1024]) {
    printf("Message recu: %s\n", data);

    // Rechercher la position de "code" dans la balise JSON
    char *code_position = strstr(data, "\"code\"");
    if (code_position != NULL) {
        // Trouver la position du début de la valeur
        char *code_start = strchr(code_position, ':');
        if (code_start != NULL) {
            // Avancer pour ignorer les espaces et les guillemets
            code_start++;
            while (*code_start == ' ' || *code_start == '\"') {
                code_start++;
            }

            // Trouver la position de la virgule ou de la fin de la valeur "code"
            char *code_end = code_start;
            while (*code_end != ',' && *code_end != '}' && *code_end != '\0') {
                code_end++;
            }

            // Calculer la longueur de la valeur "code"
            int code_length = code_end - code_start;

            // Allouer de la mémoire pour stocker la valeur de "code" sans les guillemets
            char code_value[code_length + 1];

            // Copier la valeur de "code" dans la nouvelle chaîne
            strncpy(code_value, code_start, code_length);
            code_value[code_length] = '\0';

            // Afficher la valeur de "code" sans les guillemets
            printf("Code : %s\n", code_value);

            // Rechercher la position de "valeurs" dans la balise JSON
            char *valeurs_position = strstr(data, "\"valeurs\"");
            if (valeurs_position != NULL) {
                // Trouver la position du début des valeurs
                char *valeurs_start = strchr(valeurs_position, ':');
                if (valeurs_start != NULL) {
                    // Avancer pour ignorer les espaces et les crochets
                    valeurs_start++;
                    while (*valeurs_start == ' ' || *valeurs_start == '[' || *valeurs_start == '\"') {
                        valeurs_start++;
                    }

                    // Trouver la position de la virgule ou de la fin des valeurs
                    char *valeurs_end = valeurs_start;
                    while (*valeurs_end != ']' && *valeurs_end != '}' && *valeurs_end != '\0') {
                        valeurs_end++;
                    }

                    // Calculer la longueur des valeurs
                    int valeurs_length = valeurs_end - valeurs_start;

                    // Allouer de la mémoire pour stocker les valeurs
                    char valeurs[valeurs_length + 1];

                    // Copier les valeurs dans la nouvelle chaîne
                    strncpy(valeurs, valeurs_start, valeurs_length);
                    valeurs[valeurs_length] = '\0';

                    // Afficher les valeurs
                    printf("Valeurs : %s\n", valeurs);

                    // Maintenant, vous pouvez traiter les valeurs en fonction de la valeur de "code"
                    if (strcmp(code_value, "couleurs") == 0) {
                        // Traitement des couleurs et enregistrement dans le fichier "couleurs.txt"
                        char *couleur = strtok(valeurs, ",");
                        FILE *file = fopen("couleurs.txt", "w");
                        if (file != NULL) {
                            while (couleur != NULL) {
                                fprintf(file, "%s\n", couleur);
                                couleur = strtok(NULL, ",");
                            }
                            fclose(file);
                            printf("Les couleurs ont été enregistrées dans le fichier 'couleurs.txt'.\n");
                        } else {
                            perror("Erreur d'ouverture du fichier pour les couleurs.");
                        }
                    }

                    return EXIT_SUCCESS;
                }
            }
        }
    }

    // Si les balises JSON ne sont pas correctement formatées
    renvoie_message(client_socket_fd, "Balises JSON non valides.");
    return EXIT_FAILURE;
}

int recois_balises(int client_socket_fd, char data[1024]) {
    // Rechercher la position de "code" dans la balise JSON
    char *code_position = strstr(data, "\"code\"");
    if (code_position != NULL) {
        // Trouver la position du début de la valeur
        char *code_start = strchr(code_position, ':');
        if (code_start != NULL) {
            // Avancer pour ignorer les espaces et les guillemets
            code_start++;
            while (*code_start == ' ' || *code_start == '\"') {
                code_start++;
            }

            // Trouver la position de la virgule ou de la fin de la valeur "code"
            char *code_end = code_start;
            while (*code_end != ',' && *code_end != '}' && *code_end != '\0') {
                code_end++;
            }

            // Calculer la longueur de la valeur "code"
            int code_length = code_end - code_start;

            // Allouer de la mémoire pour stocker la valeur de "code" sans les guillemets
            char code_value[code_length + 1];

            // Copier la valeur de "code" dans la nouvelle chaîne
            strncpy(code_value, code_start, code_length);
            code_value[code_length] = '\0';

            // Afficher la valeur de "code" sans les guillemets
            printf("Code : %s\n", code_value);

            // Traitez le reste de la balise JSON en fonction de la valeur de "code"
            if (strcmp(code_value, "balises") == 0) {
                // Rechercher la position de "valeurs" dans la balise JSON
                char *valeurs_position = strstr(data, "\"valeurs\"");
                if (valeurs_position != NULL) {
                    // Trouver la position du début des valeurs
                    char *valeurs_start = strchr(valeurs_position, ':');
                    if (valeurs_start != NULL) {
                        // Avancer pour ignorer les espaces et les crochets
                        valeurs_start++;
                        while (*valeurs_start == ' ' || *valeurs_start == '[' || *valeurs_start == '\"') {
                            valeurs_start++;
                        }

                        // Trouver la position de la virgule ou de la fin des valeurs
                        char *valeurs_end = valeurs_start;
                        while (*valeurs_end != ']' && *valeurs_end != '}' && *valeurs_end != '\0') {
                            valeurs_end++;
                        }

                        // Calculer la longueur des valeurs
                        int valeurs_length = valeurs_end - valeurs_start;

                        // Allouer de la mémoire pour stocker les valeurs
                        char valeurs[valeurs_length + 1];

                        // Copier les valeurs dans la nouvelle chaîne
                        strncpy(valeurs, valeurs_start, valeurs_length);
                        valeurs[valeurs_length] = '\0';

                        // Afficher les valeurs

                        // Traitez les valeurs, par exemple, enregistrez-les dans un fichier
                        // Assurez-vous que le format et le traitement des valeurs correspondent à vos besoins spécifiques
                        // ...

                        return EXIT_SUCCESS;
                    }
                }
            } else {
                printf("Code non pris en charge : %s\n", code_value);
            }
        }
    }

    // Si les balises JSON ne sont pas correctement formatées
    renvoie_message(client_socket_fd, "Balises JSON non valides.");
    return EXIT_FAILURE;
}

// Fonction pour recevoir et traiter des balises JSON
int recois_balises_json(int client_socket_fd, char *data) {
    // Recherche de l'indice du début du JSON
    char *start = strstr(data, "{");
    if (start == NULL) {
        fprintf(stderr, "JSON invalide : le début du JSON est introuvable.\n");
        return EXIT_FAILURE;
    }

    // Recherche de l'indice de la fin du JSON
    char *end = strrchr(data, '}');
    if (end == NULL) {
        fprintf(stderr, "JSON invalide : la fin du JSON est introuvable.\n");
        return EXIT_FAILURE;
    }

    // Extraire le JSON de la chaîne
    int json_length = end - start + 1;
    char json[json_length];
    strncpy(json, start, json_length);
    json[json_length - 1] = '\0';

    // Trouver la position de "code"
    char *code_position = strstr(json, "\"code\"");
    if (code_position != NULL) {
        // Trouver la position du début de la valeur
        char *code_start = strchr(code_position, ':');
        if (code_start != NULL) {
            // Avancer pour ignorer les espaces et les guillemets
            code_start++;
            while (*code_start == ' ' || *code_start == '\"') {
                code_start++;
            }

            // Trouver la position de la virgule ou de la fin de l'objet JSON
            char *code_end = code_start;
            while (*code_end != ',' && *code_end != '}' && *code_end != '\0') {
                code_end++;
            }

            // Calculer la longueur de la valeur "code"
            int code_length = code_end - code_start;

            // Allouer de la mémoire pour stocker la valeur de "code" sans les guillemets
            char code_value[code_length + 1];

            // Copier la valeur de "code" dans la nouvelle chaîne
            strncpy(code_value, code_start, code_length);
            code_value[code_length] = '\0';
            code_value[code_length - 1] = '\0';

            // Afficher la valeur de "code" sans les guillemets
            printf("Code : %s\n", code_value);

            // Utiliser une structure conditionnelle pour décider quelle fonction appeler
            if (strcmp(code_value, "balises") == 0) {
                return recois_balises(client_socket_fd, data);
            } else if (strcmp(code_value, "couleurs") == 0) {
                return recois_couleurs(client_socket_fd, data);
            } else if (strcmp(code_value, "numeros_calcule") == 0) {
                return recois_numeros_calcule(client_socket_fd, data);
            } else if (strcmp(code_value, "message") == 0) {
                return recois_envoie_message(client_socket_fd, data);
            } else {
                // Code non reconnu
                fprintf(stderr, "Code non reconnu : %s\n", code_value);
                return EXIT_FAILURE;
            }
        }
    }

    return EXIT_SUCCESS;
}

void gestionnaire_ctrl_c(int signal) {
    printf("\nSignal Ctrl+C capturé. Sortie du programme.\n");
    // fermer le socket
    close(socketfd);
    exit(0); // Quitter proprement le programme.
}

JSONData jsonDeserialize(char *json) {

    char *codePart = strtok(json,",");
    char *valeurPart = strtok(NULL,"}");
    char *valeur = strtok(valeurPart,":");
    valeur = strtok(NULL,":");
    char *code = strtok(codePart,":");
    code = strtok(NULL,":");
    code = strtok(code,"\"");

    char *donnee = strtok(valeur,"[");
    donnee = strtok(donnee,"]");

    char tempDonnee = *donnee;
    int nbValeur = 0;
    char *tempData = "";
    while (tempData != NULL){
        if(nbValeur ==0 ){
            tempData = strtok(&tempDonnee,",");
        }else{
            tempData = strtok(NULL,",");
        }

        nbValeur++;

    }


    char *valeurs[nbValeur];
    int i = 0;
    char *data = "";
    while (data != NULL){
        if(i ==0 ){
            data = strtok(donnee,",");
        }else{
            data = strtok(NULL,",");
        }
        if(data != NULL){
            valeurs[i] = data;
            i++;
        }
    }

    for (int j = 0; j < nbValeur; j++)
    {
        valeurs[j] = strtok(valeurs[j],"\"");
    }

    JSONData jsonData;
    jsonData.code = code;
    jsonData.valeurs = valeurs;

    return jsonData;
}

int main() {
    int bind_status;

    struct sockaddr_in server_addr;

    /*
     * Creation d'une socket
     */
    socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketfd < 0) {
        perror("Unable to open a socket");
        return -1;
    }

    int option = 1;
    setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    // détails du serveur (adresse et port)
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Relier l'adresse à la socket
    bind_status = bind(socketfd, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if (bind_status < 0) {
        perror("bind");
        return (EXIT_FAILURE);
    }

    // Enregistrez la fonction de gestion du signal Ctrl+C
    signal(SIGINT, gestionnaire_ctrl_c);

    // Écouter les messages envoyés par le client en boucle infinie
    while (1) {
        // Écouter les messages envoyés par le client
        listen(socketfd, 10);

        // Lire et répondre au client
        struct sockaddr_in client_addr;
        char data[1024];

        unsigned int client_addr_len = sizeof(client_addr);

        // nouvelle connection de client
        int client_socket_fd = accept(socketfd, (struct sockaddr *) &client_addr, &client_addr_len);
        if (client_socket_fd < 0) {
            perror("accept");
            return (EXIT_FAILURE);
        }

        // la réinitialisation de l'ensemble des données
        memset(data, 0, sizeof(data));

        // lecture de données envoyées par un client
        int data_size = read(client_socket_fd, (void *) data, sizeof(data));

        if (data_size < 0) {
            perror("erreur lecture");
            return (EXIT_FAILURE);
        }
        printf("Message recu: %s\n", data);
        JSONData jsonData = jsonDeserialize(data);
        printf("Code : %s\n", jsonData.code);
        printf("Valeurs : ");
        //sizeof(jsonData.valeurs)
        printf("tes t: %li", sizeof(jsonData.valeurs));
//        for (int i = 0; i < sizeof(jsonData.valeurs); i++) {
//            printf("%s ", jsonData.valeurs[i]);
//        }
//        printf("\n");

        recois_balises_json(client_socket_fd, data);
    }

    return 0;
}
