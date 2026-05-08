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
#include <string.h>
#include <strings.h>
#include "list/list.h"
#include "user.h"

#define PORT_FREESCORD 4321
#define BUFSZ 256
#define MAX_CLIENT 10

/** Gérer toutes les communications avec le client renseigné dans
 * user, qui doit être l'adresse d'une struct user */
void *handle_client(void *user);

/* Le répéteur lit dans le tube, réecrit dans les sockets de tous les utilisateurs
en parcourant la liste des utilisateurs  */
void * lecture_thread(void *arg);

/** Créer et configurer une socket d'écoute sur le port donné en argument
 * retourne le descripteur de cette socket, ou -1 en cas d'erreur */
int create_listening_sock(uint16_t port)
{
	//création socket TCP
	int fd = socket(AF_INET, SOCK_STREAM,0);
	if(fd < 0)
	{
		perror("socket");
		return -1;
	}

	//configurer le socket
	struct sockaddr_in sa = { .sin_family = AF_INET,
	.sin_port = htons(port),
	.sin_addr.s_addr = htonl(INADDR_ANY) };
	int opt = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int));

	//3 Attacher le socket à la sockaddr locale
	if (bind(fd, (struct sockaddr *) &sa, sizeof(sa)) < 0) {
		perror("bind");
		close(fd);
	}

	return fd;
}


/* VARIABLES GLOBALES */
int tube[2];
struct list * users;

/* CREATION D'UN MUTEX
	Utilité : plusieurs thread ont accès à la liste (parcourir/ajouter/supprmier) donc il peut y avoir une compétition entre les thread
	Il faut donc mettre un mutex pour protéger la liste
*/
pthread_mutex_t users_mutex;

/* LIMITATION DES CLIENTS */
int nb_client = 0;

int main(void)
{
	/* CREATION D'UN SOCKET */
	//point d'entrée réseau (IPv4, SOCK_STREAM,0)
	int sock_l = socket(AF_INET, SOCK_STREAM, 0); // sock_l = socket d'écoute, sert qu'à accepter
	if (sock_l < 0) { perror("socket"); exit(1); }

	/* CONFIGURATION DE L'ADRESSE */
	//Sur quelle adresse et quelle port, je vais écouter
	struct sockaddr_in sa = { .sin_family = AF_INET, //IPv4
	.sin_port = htons(PORT_FREESCORD),
	.sin_addr.s_addr = htonl(INADDR_ANY) }; //accepte toutes les adresses IP locales (localhost, IP réseau) => le serveur accepte toutes les interfaces
	socklen_t sl = sizeof(sa);

	int opt = 1;
	/* REUTILISATION DU PORT */
	//Si je redémarre le serveur, je peux donc réutiliser le port sinon "Address already in use"
	if(setsockopt(sock_l, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)))
	{
		perror("setsockopt");
		exit(1);
	}

	/* ATTACHER LA SOCKET */
	// liaision de sock_l (socket) à toutes les adresses IP vu que j'accepte toutes les adresses IP et le port 4321 donné par la consigne, sinon socket abstraite
	//savoir si le port est déjà utilisé 
	if (bind(sock_l, (struct sockaddr *) &sa, sl) < 0) {
		perror("bind");
		exit(3);
	}

	/* LISTEN */
	// attente de connexion, prépare à recevoir des connexions
	if(listen(sock_l,20) < 0) { perror("listen");exit(2);}
	
	
	if(pipe(tube))
	{
		perror("tube");
		exit(1);
	}

	/* CREATION DE LA LISTE DES UTILISATEURS */
	users = list_create();  
	
	//répéteur
	pthread_t rep;
	if(pthread_create(&rep,NULL,lecture_thread,NULL)!=0){
		perror("pthread_create lecture_thread");
		exit(1);
	}
	pthread_detach(rep);

	printf("Serveur en attente de connexions sur le port %d\n", PORT_FREESCORD);
	
	while (1)
	{
		struct user * u = user_accept(sock_l);
		if(!u)
		{
			perror("user_accept");
			continue;
		}	


		
		pthread_mutex_lock(&users_mutex);
		//vérifier si le serveur est plein
		if(nb_client >= MAX_CLIENT)
		{
			pthread_mutex_unlock(&users_mutex);
			char * msg = "Serveur plein\r\n";
			write(u->sock, msg,strlen(msg));
			user_free(u);
			continue;
		}

		nb_client++;
		list_add(users,u);
		pthread_mutex_unlock(&users_mutex);

		pthread_t th;
		if(pthread_create(&th, NULL, handle_client,u)!=0)
		{
			perror("pthread_create");
			pthread_mutex_lock(&users_mutex);
			nb_client--;
			list_remove_element(users,u);
			pthread_mutex_unlock(&users_mutex);
			user_free(u);
			continue; 
		}
		pthread_detach(th);
	}

	close(tube[0]);
	close(tube[1]);
	
	close(sock_l);
	list_free(users,(void (*) (void * ))user_free);
	pthread_mutex_destroy(&users_mutex);
	return 0;
}

void * lecture_thread(void *arg)
{
	char buff[BUFSZ];
	ssize_t n;

	while(1)
	{
		//lire le message
		if((n = read(tube[0], buff, BUFSZ)) <= 0) break;

		pthread_mutex_lock(&users_mutex);
		struct node * cour = users->first;
		while(cour)
		{
			struct user * u = cour->elt;
			write(u->sock,buff,n);
			cour = cour->next;
		}
		pthread_mutex_unlock(&users_mutex);
	}
	return NULL;
}


int pseudo_deja_pris(char * pseudo)
{
	pthread_mutex_lock(&users_mutex);
	struct node * cour = users->first;
	while(cour)
	{
		struct user * u = cour->elt;
		if(u->nickname && strcmp(u->nickname,pseudo) == 0)
		{
			pthread_mutex_unlock(&users_mutex);
			return 1;
		}
		cour = cour->next;
	}
	pthread_mutex_unlock(&users_mutex);
	return 0;
}

const char * mots_interdits[] = {"connard", "connasse", "salope", "pute", "batard",
    "enculé", "encule", "fils de pute", "fdp", "pd", "enfoire","enfoiree",
    "merde", "con", "conne", "idiot", "idiote",
    "abruti", "abrutie", "imbecile", "cretin", "cretine",
    "bouffon", "bouffonne", "taré", "tare", "debile",
    "nique", "niquer", "ntm", "tg", "va te faire", "salaud", "salopard",
    "admin", "root", "serveur", "moderateur", "modo",
    NULL};

int pseudo_interdit(char * pseudo)
{
	for(int i=0;mots_interdits[i]!= NULL;i++)
	{
		if(strcasecmp(pseudo,mots_interdits[i]) == 0) return 1;	
	}
	return 0;
}

//vérifie que le pseudo est valide (taille et caractère interdit)
int pseudo_valide(char * pseudo)
{
	if(strlen(pseudo) == 0 || strlen(pseudo) > 16) return 0;
	if(strchr(pseudo, ':') != NULL) return 0;
	if(pseudo_interdit(pseudo)) return 0;
	return 1;
}




void *handle_client(void *user)
{
	struct user *t = (struct user *)user;
	char buff[BUFSZ];
	ssize_t n;
 
	//message de bienvenue
	char *bienvenue =
		"Bienvenue sur Freescord!\r\n"
		"\r\n";
	write(t->sock, bienvenue, strlen(bienvenue));
 
	//négociation du pseudo avec le client
	int deco = 0;
	while(1)
	{
		n = read(t->sock, buff, BUFSZ - 1);
		if(n <= 0) {
			
			deco = 1;
			break;
		}
		buff[n] = '\0';
		printf("[SERVEUR] : a reçu %s\n", buff);
		buff[strcspn(buff, "\r\n")] = '\0';
			
		char reponse[BUFSZ];
 
		//vérifier que la commande commence bien par "nickname "
		if(strncmp(buff, "nickname ", 9) != 0)
		{
			snprintf(reponse, sizeof(reponse), "3 commande invalide\r\n");
			write(t->sock, reponse, strlen(reponse));
			continue;
		}
 
		char *pseudo = buff + 9;
 
		if(!pseudo_valide(pseudo))
		{
			snprintf(reponse, sizeof(reponse), "2 pseudo invalide\r\n");
			write(t->sock, reponse, strlen(reponse));
			continue;
		}
 
		if(pseudo_deja_pris(pseudo))
		{
			snprintf(reponse, sizeof(reponse), "1 pseudo deja pris\r\n");
			write(t->sock, reponse, strlen(reponse));
			continue;
		}
 
		//pseudo accepté, on le donne au client
		t->nickname = strdup(pseudo);
		snprintf(reponse, sizeof(reponse), "0 pseudo accepte\r\n");
		write(t->sock, reponse, strlen(reponse));
		break;
	}
 
	//boucle de communication avec des clients connectés
	if(!deco){
		while((n = read(t->sock, buff, BUFSZ - 1)) > 0)
		{
			buff[n] = '\0';
			for(int i = 0; i < n; i++)
			{
				if(buff[i] == '\n') printf("\\n");
				else if(buff[i] == '\r') printf("\\r");
				else printf("%c", buff[i]);
			}
			printf("\n");
			
			char msg[BUFSZ * 2];
			int total = snprintf(msg, sizeof(msg), "%s: %s", t->nickname, buff);
			if(total <= 0) continue;
			
			write(tube[1], msg, total);
		}
		if(n < 0) perror("read");
	}

	//prévenir les autres clients qu'un client est parti
	if(t->nickname)
	{
		char message[BUFSZ];
		int total = snprintf(message, sizeof(message), "%s a quitté Freescord !\r\n", t->nickname);
		if(total > 0) write(tube[1],message,total);
		free(t->nickname);
		t->nickname=NULL;
	}

	pthread_mutex_lock(&users_mutex);
	nb_client--;
	list_remove_element(users, t);
	pthread_mutex_unlock(&users_mutex);
 
	user_free(t);
	return NULL;
}




