/*
Auteur --> aiglematth
On va implémenter un petit header pour gérer des fichiers tar en C
*/

#ifndef TARCONST
#define TARCONST

//Nos includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>

//Des macros forts utiles...
#define MAX(a,b) ((a) > (b) ? a : b)
#define MIN(a,b) ((a) < (b) ? a : b)

//On définit la structure d'un header (repris des headers officiels) et au passage le type
#define  NAMSIZ      100
#define  TUNMLEN      32
#define  TGNMLEN      32

typedef struct tarHeader {
	char    name[NAMSIZ];
	char    mode[8];
	char    uid[8];
	char    gid[8];
	char    size[12];
	char    mtime[12];
	char    chksum[8];
	char    linkflag;
	char    linkname[NAMSIZ];
	char    magic[8];
	//char    version[2];
	char    uname[TUNMLEN];
	char    gname[TGNMLEN];
	char    devmajor[8];
	char    devminor[8];
	char 	prefix[155];
    char 	pad[12];
}tarHeader;

#define  RECORDSIZE  sizeof(tarHeader)

//Maintenant j'ai besoin d'un type qui contiendra le header et le contenu du fichier
typedef struct tarEntry
{
	tarHeader header;
	char      *content;	
}tarEntry;

//Maintenant j'ai besoin d'un type qui contiendra des pointeurs vers des headers
typedef struct tarFile
{
	unsigned long size;
	tarEntry *tar;	
}tarFile;

//Quelques constantes qui peuvent être utile (encore prises du header officiel)
#define    CHKBLANKS    "        "        /* 8 blanks, no null */
#define    TMAGIC    "ustar  "        /* 7 chars and a null */

//Values possible du champ linkflag (toujours prises du header officiel ;) )
#define  LF_OLDNORMAL '\0'       /* Normal disk file, Unix compatible */
#define  LF_NORMAL    '0'        /* Normal disk file */
#define  LF_LINK      '1'        /* Link to previously dumped file */
#define  LF_SYMLINK   '2'        /* Symbolic link */
#define  LF_CHR       '3'        /* Character special file */
#define  LF_BLK       '4'        /* Block special file */
#define  LF_DIR       '5'        /* Directory */
#define  LF_FIFO      '6'        /* FIFO special file */
#define  LF_CONTIG    '7'        /* Contiguous file */

/*
Fonction qui vérifie que le header est correct (on vérifie le premier MAGIC du premier header)
:param header: Un pointeur vers la struct du header
:return:         True si oui, sinon False
*/
bool isTarHeader(tarHeader *header)
{	
	if(strcmp(header->magic, TMAGIC) == 0)
	{
		return true;
	}
	return false;
}

/*
Fonction qui vérifie que le fichier est un fichier tar (on vérifie le premier MAGIC du premier header)
:param filename: Un pointeur vers le nom du fichier
:return:         True si oui, sinon False
*/
bool isTarFile(char *filename)
{
	FILE *stream;
	tarHeader *header = malloc(sizeof(tarHeader));
	bool ret = false;
	int  magicSize = 8;
	int  offset    = 257;

	stream = fopen(filename, "r");
	if(stream == NULL)
	{
		return ret;
	}

	if(fread(header, RECORDSIZE, 1, stream) == 0)
	{
		return ret;
	}

	ret = isTarHeader(header);
	return ret;
}

/*
Fonction qui retourne une structure représentant notre fichier tar
:param stream: Un pointeur vers le flux FILE de notre fichier
:return:         NULL ou une struct tarFile
*/
tarFile *readTar(FILE *stream)
{
	tarHeader header;
	tarFile   *tar     = malloc(sizeof(tarFile));
	unsigned long size = 0;
	tar->size          = 0;
	tar->tar           = NULL;

	while(fread(&header, RECORDSIZE, 1, stream))
	{
		if(isTarHeader(&header) == false)
		{
			return tar;
		}

		tar->size++;

		tar->tar = realloc(tar->tar, tar->size * (long)sizeof(tarEntry));
		tar->tar[tar->size - 1].header  = header;

		size = strtoul(header.size, NULL, 8);
		tar->tar[tar->size - 1].content = calloc(size, (long)sizeof(char));
		fread(tar->tar[tar->size - 1].content, size, 1, stream);


		if(size % RECORDSIZE != 0)
		{
			fseek(stream, RECORDSIZE - (size%RECORDSIZE), SEEK_CUR);
		}
	}

	return tar;
}

/*
Fonction qui retourne une structure représentant notre fichier tar
:param filename: Un pointeur vers le nom de notre fichier
:return:         NULL ou une struct tarFile
*/
tarFile *readTarFile(char *filename)
{
	FILE *stream;
	tarFile *ret;

	stream = fopen(filename, "r");
	if(stream == NULL)
	{
		return NULL;
	}

	ret = readTar(stream);
	fclose(stream);
	return ret;
}


/*
Fonction qui permet d'écrire une struct tarEntry dans un flux
:param stream: Un pointeur vers le flux FILE de notre fichier
:param tar:    Un pointeur sur une structure tarEntry à écrire dans le flux
:return:         NULL ou une struct tarFile
*/
bool writeTarEntry(FILE *stream, tarEntry *tar)
{
	unsigned long int size = 0;
	tarHeader *header = NULL;
	char      *content = NULL;
	char      *none = NULL;

	header = &(tar->header);
	fwrite(header, sizeof(*header), 1, stream);

	content = tar->content;
	size = strtoul(header->size, NULL, 8);
	fwrite(content, size, 1, stream);

	if(size % RECORDSIZE != 0)
	{
		size = RECORDSIZE - (size%RECORDSIZE);
		none = realloc(none, sizeof(char) * size);
		memset(none, 0, size);
		fwrite(none, sizeof(*none) * size, 1, stream);
	}
	free(none);

	return true;
}

/*
Fonction qui permet d'écrire une struct tarFile dans un flux
:param stream: Un pointeur vers le flux FILE de notre fichier
:param tar:    Un pointeur sur une structure tarFile à écrire dans le flux
:return:         true ou false
*/
bool writeTar(FILE *stream, tarFile *tar)
{
	unsigned long int size = 0;

	for(unsigned long int i = 0; i < tar->size; i++)
	{
		writeTarEntry(stream, &(tar->tar[i]));
	}

	return true;
}

/*
Fonction qui permet d'ajouter / créer un fichier tar
:param filenae: Un pointeur vers le nom de notre fichier
:param tar:     Un pointeur sur une structure tarFile à écrire dans le flux
:return:         NULL ou une struct tarFile
*/
bool writeTarFile(char *filename, tarFile *tar, const char *mode)
{
	FILE *stream;
	bool ret;

	stream = fopen(filename, mode);
	if(stream == NULL)
	{
		return false;
	}

	ret = writeTar(stream, tar);
	fclose(stream);
	return ret;
}

/*
Permet de pointer sur la struct tarEntry correspondant au fichier donné
:param filename: Un pointeur vers le nom du fichier
:return:         Un pointeur sur la tarEntry
*/
tarEntry *generateTarEntryFrom(char *filename)
{
	tarEntry tar;
	tarHeader *header = &(tar.header);
	tarEntry *ret = &tar;
	struct stat   buffer;   

	if(stat(filename, &buffer) != 0)
	{
		return false;
	}

	memcpy(header->name, filename, NAMSIZ);

	//CONTINUERRRR

}

#endif