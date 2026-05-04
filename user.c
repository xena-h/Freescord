/* Luila LUNZITISA 12206571
Je déclare qu'il s'agit de mon propre travail.
Ce travail a été réalisé intégralement par un être humain. */

#include "user.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h> //ajout d'une bibliothèque pour le close()

/** accepter une connection TCP depuis la socket d'écoute sl et retourner un
 * pointeur vers un struct user, dynamiquement alloué et convenablement
 * initialisé */
struct user *user_accept(int sl)
{
	struct user * t = malloc(sizeof(struct user));
	if(!t)return NULL;

	t->addr_len = sizeof(t->address);
	t->sock = accept(sl,(struct sockaddr *)&t->address,&t->addr_len);
	if(t->sock < 0)
	{
		perror("accept");
		free(t);
		return NULL;
	}
	t->nickname=NULL;

	return t;
}


/** libérer toute la mémoire associée à user */
void user_free(struct user *user)
{
	if(!user) return;
	close(user->sock);
	free(user->nickname);
	free(user);
}
