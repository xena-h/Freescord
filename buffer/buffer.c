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

struct buffer {
	int fd;
	char buff[BUFSZ];
	int pos_curseur;
	int taille_buffer;
	int has_ungetc; 
	char ungetc_char; 
	int eof;
};

int remplir_buff(buffer *b)
{
	ssize_t n = read(b->fd, b->buff, sizeof(b->buff));
	if(n < 0){ perror("read"); return -1; }
	if(n == 0){ b->eof = 1; return 0; }
	b->pos_curseur = 0;
	b->taille_buffer = (int)n;
	return (int)n;
}

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
	
	remplir_buff(b);
	return b;
}


int buff_getc(buffer *b)
{
    if(b->has_ungetc){
        b->has_ungetc = 0;
        return b->ungetc_char;
    }
    
    if(b->pos_curseur >= b->taille_buffer)
	{
		if(b->eof) return EOF;

		int n = remplir_buff(b);
		if(n<=0) return EOF;

	}

    return (unsigned char)b->buff[b->pos_curseur++];
}


int buff_ungetc(buffer *b, int c)
{
	//s'il y a déjà un caractère en attente => impossible de mettre un autre caractère en attente
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
	if(buff->has_ungetc) return 0;
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
		if(c == EOF)
		{
           if(i==0) return NULL;
		   break;
		} 
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
				//comme je récupère le prochain caractère (c_next = buff_getc(b)), je dois le remettre dans le buffer si ce n'est pas la fin du fichier
				buff_ungetc(b,c_next);
			}
		}
		if(c =='\n') break;
	}
	dest[i]='\0';
    return dest;
}
