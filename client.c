/* Luila LUNZITISA 12206571
Je déclare qu'il s'agit de mon propre travail.
Ce travail a été réalisé intégralement par un être humain. */

#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>
#include <string.h>
#include "buffer/buffer.h"
#include "utils.h"
#include <errno.h>

#define PORT_FREESCORD 4321
#define BUFSZ 1024

void usage(char *nom_prog) {
	fprintf(stderr, "Usage: %s addr_ipv4\n"
			"client pour INCP (Incrementation Protocol)\n"
			"Exemple: %s 208.97.177.124\n", nom_prog, nom_prog);
}


/** se connecter au serveur TCP d'adresse donnée en argument sous forme de
 * chaîne de caractère et au port donné en argument
 * retourne le descripteur de fichier de la socket obtenue ou -1 en cas
 * d'erreur. */
int connect_serveur_tcp(char *adresse, uint16_t port)
{
	//création de socket
	int fd = socket(AF_INET,SOCK_STREAM,0);

	if(fd < 0){ perror("socket"); return -1;}
	struct sockaddr_in sa = {
		.sin_family = AF_INET,
		.sin_port = htons(port)
	};
	
	if(inet_pton(AF_INET,adresse, &sa.sin_addr) !=1 ){ fprintf(stderr, "Adresse invalide : %s\n",adresse); close(fd); return -1; }
	socklen_t sl = sizeof(sa);

	//demande de connexion au serveur
	if(connect(fd,(struct sockaddr *) &sa,sl) < 0)
	{
		perror("connect");
		close(fd);
		return -1;
	}
	return fd;
}


//Envoie n octects du buff vers sock, retourne -1 si erreur
int envoyer(int sock, char *buf, ssize_t n);

//message de bienvenue
void bienvenue(struct buffer *b, struct pollfd *fds);

//demande un pseudo à l'utilisateur et le négocie avec le serveur
void negocier_pseudo(int sock, struct buffer *b, struct pollfd * fds);


int main(int argc, char *argv[])
{
    if(argc != 2){ usage(argv[0]); return 1; }

    int sock = connect_serveur_tcp(argv[1], PORT_FREESCORD);
    if(sock < 0){ perror("sock"); exit(2); }
 
    struct buffer *b = buff_create(sock, BUFSZ);
    if(!b){ perror("buff_create"); close(sock); exit(1); }
    
    struct pollfd fds[2] = {
        { .fd = sock, .events = POLLIN },
        { .fd = 0,    .events = POLLIN }
    };
    
    char line[BUFSZ];

    //lecture du message de bienvenue puis négociation du pseeudo
    bienvenue(b,fds);
    negocier_pseudo(sock,b,fds);

    while(1)
    {
        int t = buff_ready(b) ? 0 : -1;
        int n_events = poll(fds, 2, t);
        if(n_events < 0){ perror("poll"); break; }
        
        // Serveur => Affichage 
        if (fds[0].revents & POLLIN)
        {
            if(remplir_buff(b) <= 0)
            { 
                printf("Serveur fermé\n"); 
                break; 
            }
        }

        if(buff_ready(b))
        {
            while(buff_ready(b) && buff_fgets_crlf(b, line, sizeof(line)) != NULL){
                char *converted = crlf_to_lf(line);
                if(converted)
                { 
                    write(1, converted, strlen(converted)); 
                    free(converted); 
                }
            }
        }

        // Clavier 
        if (fds[1].revents & POLLIN)
        {
            ssize_t n = read(0, line, sizeof(line));
            if(n <= 0)
            { 
                printf("Au revoir !\n"); 
                break; 
            }
            line[n] = '\0';

            char *converted = lf_to_crlf(line);
            if(!converted)
            { 
                fprintf(stderr, "Erreur conversion\n"); 
                break; 
            }

            envoyer(sock, converted, (ssize_t)strlen(converted));
            free(converted);
        }
    }
    buff_free(b);
    close(sock);

    return 0;
}


int envoyer(int sock, char *buf, ssize_t n)
{
    ssize_t j = 0;
    while(j < n)
    {
        ssize_t sent = write(sock, buf + j, n - j);
        if(sent < 0){ perror("write socket"); return -1; }
        j += sent;
    }
    return 0;
}


//lit et affiche le message de bienvenue jusqu'à la ligne vide
void bienvenue(struct buffer *b, struct pollfd *fds)
{
	char line[BUFSZ];
 
	while(1)
	{
		if(!buff_ready(b))
        {
            poll(fds, 1, -1);
            if(remplir_buff(b) <= 0) break;
        }

		while(buff_fgets_crlf(b, line, sizeof(line)) != NULL)
		{
			char *converti = crlf_to_lf(line);
			if(!converti) break;
 
			//ligne vide = fin du message de bienvenue
			if(strcmp(converti, "\n") == 0)
			{
				free(converti);
				return;
			}
			write(1, converti, strlen(converti));
			free(converti);
		}
	}
}

void negocier_pseudo(int sock, struct buffer *b, struct pollfd *fds)
{
    char line[BUFSZ];
    char message[BUFSZ];

    while(1)
    {
        printf("Entrez la commande nickname (format: nickname pseudo) : ");
        fflush(stdout);
        
        if(fgets(line, sizeof(line), stdin) == NULL)
        {
            printf("\nAu revoir !\n");
            close(sock);
            buff_free(b);
            exit(0);
        }
        
        // Supprimer le \n de fin
        line[strcspn(line, "\n")] = '\0';
        
        // Vérifier le format
        if(strncmp(line, "nickname ", 9) != 0)
        {
            printf("Format invalide ! Utilisez: nickname pseudo\n");
            continue;
        }
        
        char *pseudo = line + 9;
        
        if(strlen(pseudo) == 0 || strlen(pseudo) > 16)
        {
            printf("Pseudo invalide (1-16 caractères)\n");
            continue;
        }
        
        if(strchr(pseudo, ':') != NULL)
        {
            printf("Le caractère ':' est interdit\n");
            continue;
        }
        
        // Envoyer au serveur
        if(strlen(line) + 2 > sizeof(message))
        {
            printf("Commande trop longue\n");
            continue;
        }
        strcpy(message, line);
        strcat(message, "\n");
        
        char *converti = lf_to_crlf(message);
        if(!converti) continue;
        
        envoyer(sock, converti, (ssize_t)strlen(converti));
        free(converti);
        
        // Attendre la réponse
        while(!buff_ready(b))
        {
            if(poll(fds, 1, -1) < 0) break;
            if(remplir_buff(b) <= 0) return;
        }
        
        // Lire la réponse
        if(buff_fgets_crlf(b, line, sizeof(line)) == NULL) continue;
        
        // Supprimer le CRLF
        line[strcspn(line, "\r\n")] = '\0';
        
        // Analyser la réponse
        if(line[0] == '0')
        {            
            const char *msg = line;
            if(line[1] == ' ') msg = line + 2;
            else msg = line + 1;
            
            printf("%s\n", msg);
            break;
        }
        else if(line[0] == '1')
            printf("Pseudo déjà pris !\n");
        else if(line[0] == '2')
            printf("Pseudo invalide !\n");
        else if(line[0] == '3')
            printf("Commande invalide !\n");
        else
            printf("Réponse inattendue: %s\n", line);
    }
}