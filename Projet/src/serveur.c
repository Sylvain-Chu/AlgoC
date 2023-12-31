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
    int nbValeurs;
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

int plot(JSONData jsonData)
{
    int i;
    int num_colors = atoi(jsonData.valeurs[0]);
    double angles[num_colors];
    memset(angles, 0, sizeof(angles));

    FILE *svg_file = fopen(svg_file_path, "w");
    if (svg_file == NULL)
    {
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

    for (i = 1; i < num_colors+1; i++){
        angles[i-1] = 360.0 / num_colors;

        double end_angle = start_angle + angles[i-1];

        double start_angle_rad = degreesToRadians(start_angle);
        double end_angle_rad = degreesToRadians(end_angle);

        double x1 = center_x + radius * cos(start_angle_rad);
        double y1 = center_y + radius * sin(start_angle_rad);
        double x2 = center_x + radius * cos(end_angle_rad);
        double y2 = center_y + radius * sin(end_angle_rad);

        fprintf(svg_file, "  <path d=\"M%.2f,%.2f A%.2f,%.2f 0 0,1 %.2f,%.2f L%.2f,%.2f Z\" fill=\"%s\" />\n",
                x1, y1, radius, radius, x2, y2, center_x, center_y, jsonData.valeurs[i]);

        start_angle = end_angle;
    }

    fprintf(svg_file, "</svg>\n");

    fclose(svg_file);

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

JSONData jsonDeserialize(char *json) {
    // Assumption: json is a valid JSON string

    // Extract the code value
    char *codePart = strstr(json, "\"code\":");
    strtok(codePart, ":\"");
    char *code = strtok(NULL, ":\",");

    // Extract the array of values
    char *valeurPart = strstr(json, "\"valeurs\":");
    strtok(valeurPart, ":");
    char *valeur = strtok(NULL, ":");

    // Count the number of values in the array
    int nbValeur = 0;
    char *ptr = valeur;
    while (*ptr) {
        if (*ptr == '\"') nbValeur++;
        ptr++;
    }
    nbValeur /= 2; // Each value has two quotes

    // Allocate memory for the array of values
    char **valeurs = malloc(nbValeur * sizeof(char *));
    if (!valeurs) {
        exit(1); // Handle memory allocation failure
    }

    // Extract values into the array
    char *token = strtok(valeur, "[\",]");
    int i = 0;
    while (token != NULL && i < nbValeur) {
        valeurs[i++] = token;
        token = strtok(NULL, "[\",]");
    }

    // Populate the JSONData struct
    JSONData jsonData;
    jsonData.code = code;
    jsonData.valeurs = valeurs;
    jsonData.nbValeurs = nbValeur;

    return jsonData;
}

int recois_couleurs(int client_socket_fd, JSONData jsonData)
{
    int nombreCouleurs = jsonData.nbValeurs;
    int i;
    FILE *file;
    file = fopen("couleurs.txt","w");
    for (i = 0; i < nombreCouleurs; i++)
    {
        fprintf(file,"#%s\n",jsonData.valeurs[i]);
    }
    fclose(file);

    int data_size = write(client_socket_fd, (void*)jsonData.code, strlen(jsonData.code));

    if (data_size < 0)
    {
        perror("erreur ecriture");
        return (EXIT_FAILURE);
    }
    return (EXIT_SUCCESS);
}

int recois_balises(int client_socket_fd, JSONData jsonData)
{
    int nombreBalises = jsonData.nbValeurs;

    int i;
    FILE *file;
    file = fopen("balises.txt","w");
    for (i = 0; i < nombreBalises; i++)
    {
        fprintf(file,"%s\n",jsonData.valeurs[i]);
    }
    fclose(file);

    int data_size = write(client_socket_fd, (void*)jsonData.code, strlen(jsonData.code));

    if (data_size < 0)
    {
        perror("erreur ecriture");
        return (EXIT_FAILURE);
    }
    return (EXIT_SUCCESS);
}

int recois_numeros_calcule(int client_socket_fd, char **data){
    char* operator = data[0];

    float resultat;
    char* nbr1 = data[1];
    char* nbr2 = data[2];
    float nbr1_int = atof(nbr1);
    float nbr2_int = atof(nbr2);
    switch (*operator)
    {
        case '+':
            resultat = nbr1_int+nbr2_int;
            break;
        case '-':
            resultat = nbr1_int-nbr2_int;
            break;
        case 'x':
            resultat = nbr1_int*nbr2_int;
            break;
        case '/':
            resultat = nbr1_int/nbr2_int;
            break;
    }

    char res_char[1024];
    memset(res_char, 0, sizeof(res_char));
    res_char[0] = snprintf(res_char, sizeof res_char, " %f", resultat);

    sscanf(res_char,"%f",&resultat);

    int data_size = write(client_socket_fd, (void *)res_char, sizeof(res_char));

    if (data_size < 0)
    {
        perror("erreur ecriture");
        return (EXIT_FAILURE);
    }
    return (EXIT_SUCCESS);
}

void gestionnaire_ctrl_c(int signal) {
    printf("\nSignal Ctrl+C capturé. Sortie du programme.\n");
    // fermer le socket
    close(socketfd);
    exit(0); // Quitter proprement le programme.
}

int recois_envoie_message(int client_socket_fd, char data[1024])
{
    /*
     * extraire le code des données envoyées par le client.
     * Les données envoyées par le client peuvent commencer par le mot "message :" ou un autre mot.
     */
    printf("Message recu: %s\n", data);

    JSONData jsonData =  jsonDeserialize(data);

    if (strcmp(jsonData.code, "image") == 0){
        plot(jsonData);
    }
    else if (strcmp(jsonData.code, "nom") == 0)
    {
        renvoie_nom(client_socket_fd, data);
    }
    else if (strcmp(jsonData.code,"calcul") == 0)
    {
        recois_numeros_calcule(client_socket_fd, jsonData.valeurs);
    }
    else if (strcmp(jsonData.code, "message") == 0)
    {
        renvoie_message(client_socket_fd, data);
    }
    else if (strcmp(jsonData.code, "couleurs") == 0)
    {
        recois_couleurs(client_socket_fd, jsonData);
    }
    else if (strcmp(jsonData.code, "balises") == 0)
    {
        recois_balises(client_socket_fd, jsonData);
    }

    return (EXIT_SUCCESS);
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

        recois_envoie_message(client_socket_fd, data);
    }

    return 0;
}

