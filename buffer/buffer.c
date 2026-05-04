/* Luila LUNZITISA 12206571
Je déclare qu'il s'agit de mon propre travail.
Ce travail a été réalisé intégralement par un être humain. */

#include "buffer.h"
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#define BUFSZ 512

/*	char buff[512] (tampon)
	la position du curseur dans le buffer
	la taille du buffer (s'il est plein ou si j'ai tout lu)
	has_ungetc_char pour savoir si je dois remettre un caractère dans le buffer 
	char ungetc_char pour remettre un caractère dans le buffer
*/

struct buffer {
	int fd;
	char buff[BUFSZ];
	int pos_curseur;
	int taille_buffer;
	int has_ungetc; //un caractère en attente
	char ungetc_char; 
	int eof;
};

buffer *buff_create(int fd, size_t buffsz)
{
	struct buffer * b = malloc(sizeof(struct buffer));
	if(!b) { printf("malloc") ; return NULL; }
	b->pos_curseur = 0;
	b->taille_buffer = 0;
	b->has_ungetc = 0;
	b->ungetc_char = 0;
	b->fd =fd;
	b->eof=0;
	return b;
}

// int buff_getc(buffer *b)
// {
// 	ssize_t n;
	
// 	//si un caractère a été remis avec un ungetc
// 	if(b->has_ungetc){
// 		b->has_ungetc = 0;
// 		return b->ungetc_char;
// 	}

// 	//si on atteint la taille du buffer
// 	if(b->pos_curseur >= b->taille_buffer)
// 	{
// 		if((n=read(b->fd,b->buff,sizeof(b->buff)))<=0)
// 		{
// 			return EOF;
// 		}
// 		b->pos_curseur=0;
// 		b->taille_buffer=n; //nb d'octets lus
// 	}
// 	return b->buff[b->pos_curseur++]; //renvoie l'octet suivant
// }


// //version 3
// int buff_getc(buffer *b)
// {
//     if(b->has_ungetc){
//         b->has_ungetc = 0;
//         return b->ungetc_char;
//     }

//     if(b->pos_curseur >= b->taille_buffer)
//     {
//         if(b->eof) return EOF;

//         ssize_t n = read(b->fd, b->buff, sizeof(b->buff));

// 		if(n < 0){
//             if(errno == EAGAIN || errno == EWOULDBLOCK) return EOF;
//             perror("read"); return EOF;
//         }
//         if(n == 0){ b->eof = 1; return EOF; }

//         b->pos_curseur = 0;
//         b->taille_buffer = (int)n; 
//     }
//     return (unsigned char)b->buff[b->pos_curseur++];
// }


//Remplit le buffer interne depuis le fd (appelé uniquement après POLLIN)
int remplir_buff(buffer *b)
{
    ssize_t n = read(b->fd, b->buff, sizeof(b->buff));
    if(n < 0){ perror("read"); return -1; }
    if(n == 0){ b->eof = 1; return 0; }
    b->pos_curseur = 0;
    b->taille_buffer = (int)n;
    return (int)n;
}

//version 4
// buff_getc ne fait PLUS jamais de read()
int buff_getc(buffer *b)
{
    if(b->has_ungetc){
        b->has_ungetc = 0;
        return b->ungetc_char;
    }
    // buffer vide → EOF, sans appel read()
    if(b->pos_curseur >= b->taille_buffer) return EOF;

    return (unsigned char)b->buff[b->pos_curseur++];
}


int buff_ungetc(buffer *b, int c)
{
	//s'il y a déjà une caractère en attente => impossible de mettre un autre en attente
	if(b->has_ungetc) return -1;
	//sinon je marque qu'un caractère a été mis en attente 
	b->has_ungetc = 1;
	b->ungetc_char = (char) c;
	return c;
}

void buff_free(buffer *b)
{
	if(b) free(b);
}

int buff_eof(const buffer *buff)
{
	// if(buff->has_ungetc) return 0;
	// return buff->pos_curseur == buff->taille_buffer ? 1 : 0;
	return buff->eof;
}

int buff_ready(const buffer *buff)
{
	if(buff->has_ungetc) return 1;
	return buff->pos_curseur < buff->taille_buffer;
}

char *buff_fgets(buffer *b, char *dest, size_t size)
{
	if(dest == NULL ||size <= 0) return NULL;
	int i=0;
	int c;
	while(i < size-1)
	{
        c = buff_getc(b);
		//fin de fichier ou aucun caractère lu 
		if(c == EOF)
		{
           if(i==0) return NULL;
		   break;
		} 
		//dest contient le caractère du buffer
		dest[i++]= (char) c;
		if(c == '\n') break;
	}
	dest[i]='\0';
    return dest;
}

char *buff_fgets_crlf(buffer *b, char *dest, size_t size)
{
	if(dest == NULL ||size <= 0) return NULL;
	int i=0;
	int c;
    int c_next;
	while(i<size-1)
	{
		c = buff_getc(b);
		//fin de fichier
		if(c == EOF)
		{
			//aucun caractère n'a été lu
			if(i==0) return NULL;
			break;
		}
		dest[i++] = c;

		if(c == '\r')
		{
			c_next = buff_getc(b);

			if(c_next == '\n')
			{
				if(i<size) dest[i++] = (char)c_next;
				break;
			}
			else if(c_next !=  EOF){
				//comme je récupère le prochain caractère (c_next = buff_getc(b)), je dois le remettre dans le buffer si ce n'est pas EOF
				buff_ungetc(b,c_next);
			}
		}
		if(c =='\n') break;
	}
	dest[i]='\0';
    return dest;
}
