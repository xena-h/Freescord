#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "buffer.h"

#define BUFFSZ 8
#define DEST_SZ 256

int main() {
    /* === Test buff_create et buff_free === */
    printf("=== Test buff_create ===\n");
    int fd = open("test.txt", O_RDWR | O_CREAT | O_TRUNC, 0644);
    write(fd, "hello\nworld\r\nfoo", 16);
    lseek(fd, 0, SEEK_SET);

    buffer *b = buff_create(fd, BUFFSZ);
    printf("buff_create : %s\n", b ? "OK" : "ECHEC");

    /* === Test buff_getc === */
    printf("\n=== Test buff_getc ===\n");
    int c = buff_getc(b);
    printf("1er char : '%c' (attendu 'h') : %s\n", c, c == 'h' ? "OK" : "ECHEC");

    /* === Test buff_ungetc === */
    printf("\n=== Test buff_ungetc ===\n");
    buff_ungetc(b, c);
    int c2 = buff_getc(b);
    printf("après ungetc : '%c' (attendu 'h') : %s\n", c2, c2 == 'h' ? "OK" : "ECHEC");

    /* === Test buff_ready === */
    printf("\n=== Test buff_ready ===\n");
    printf("buff_ready : %d (attendu 1) : %s\n", 
           buff_ready(b), buff_ready(b) == 1 ? "OK" : "ECHEC");

    /* === Test buff_eof === */
    printf("\n=== Test buff_eof ===\n");
    printf("buff_eof au debut : %d (attendu 0) : %s\n",
           buff_eof(b), buff_eof(b) == 0 ? "OK" : "ECHEC");

    /* === Test buff_fgets === */
    printf("\n=== Test buff_fgets ===\n");
    char dest[DEST_SZ];
    lseek(fd, 0, SEEK_SET);
    buffer *b2 = buff_create(fd, BUFFSZ);

    char *res = buff_fgets(b2, dest, DEST_SZ);
    printf("1ere ligne : '%s' (attendu 'hello\\n') : %s\n",
           res, res && strcmp(res, "hello\n") == 0 ? "OK" : "ECHEC");

    /* === Test buff_fgets_crlf === */
    printf("\n=== Test buff_fgets_crlf ===\n");
    res = buff_fgets(b2, dest, DEST_SZ);  // lire "world\r\n"
    // rembobiner pour tester crlf
    lseek(fd, 6, SEEK_SET);
    buffer *b3 = buff_create(fd, BUFFSZ);
    res = buff_fgets_crlf(b3, dest, DEST_SZ);
    printf("ligne CRLF : '%s' (attendu 'world\\r\\n') : %s\n",
           res, res && strcmp(res, "world\r\n") == 0 ? "OK" : "ECHEC");

    /* === Test buff_eof en fin de fichier === */
    printf("\n=== Test buff_eof fin de fichier ===\n");
    lseek(fd, 0, SEEK_SET);
    buffer *b4 = buff_create(fd, BUFFSZ);
    while (buff_getc(b4) != EOF);
    printf("buff_eof apres tout lu : %d (attendu 1) : %s\n",
           buff_eof(b4), buff_eof(b4) == 1 ? "OK" : "ECHEC");

    /* === Nettoyage === */
    buff_free(b);
    buff_free(b2);
    buff_free(b3);
    buff_free(b4);
    close(fd);
    remove("test.txt");

    printf("\n=== Tests terminés ===\n");
    return 0;
}