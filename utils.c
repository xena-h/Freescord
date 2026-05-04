/* Luila LUNZITISA 12206571
Je déclare qu'il s'agit de mon propre travail.
Ce travail a été réalisé intégralement par un être humain. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define BUFSZ 512

char *crlf_to_lf(char *line_with_crlf)
{
	int j=0;
	char * buff = malloc(sizeof(char) * strlen(line_with_crlf)+1);
	if(!buff){ printf("erreur d'alloc\n"); return NULL; }

	for(int i=0;line_with_crlf[i] != '\0';i++)
	{
		if((line_with_crlf[i]=='\r') && (line_with_crlf[i+1] == '\n')){
			buff[j++]= '\n';
			i++;
		}
		else {
				buff[j++] = line_with_crlf[i];
		}
	}
	buff[j]='\0';
	return buff;
}

char *lf_to_crlf(char *line_with_lf)
{
	int i,j=0;
	char * buff = malloc(sizeof(char) * strlen(line_with_lf) * 2 + 1);
	if(!buff){ printf("erreur d'alloc\n"); return NULL; }


	for(i=0;line_with_lf[i] != '\0';i++)
	{
		if(line_with_lf[i] == '\n')
		{
			buff[j] = '\r';
			j++;
			buff[j] = '\n';
			j++;
		}
		else 
		{
			buff[j] = line_with_lf[i];
			j++;
		}
	}

	buff[j]='\0';
	return buff;
}