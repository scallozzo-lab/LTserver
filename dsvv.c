
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include "dsvv.h"


/*
static unsigned char _LocalBuffer[_BIGBUFFER_SIZE];
static unsigned int  _LocalFileLen = 0;
*/
void _EncriptDecript(unsigned char *_io, int _lg, int enc)
{
	unsigned int *_ioint = (unsigned int *) _io;
	
	if(_lg & 3)
	{
		printf("[_EncriptDecript] Error: Longitud debe estar alineada en 32bit %d\r\n", _lg);
		//while(1);
	}
	else if((unsigned char)_io & 3)
	{
		printf("[_EncriptDecript] Error: Dirección debe estar alineada a 32bit (%X)\r\n", (unsigned int)_io);
		//while(1);
	}  
	else if(_io)
		for(int idx = 0; idx < _lg / sizeof(unsigned int); idx++)
		{	
			// Encriptación
			if(idx)
				_ioint[idx] = _ioint[idx] ^ _ENCDECSEED;
			// Flag de encriptacion
			else if(enc)
				_ioint[idx] = _ENCFLAGVAL;
			// Restaura el valor original
			else
				_ioint[idx] = _DECFLAGVAL; 
		}
}	
				
/*
int RemoveVideoFile(const char *pfilename)
{
	return remove(pfilename);
}

int ReadVideoFile(const char *pfilename, unsigned char *pdest)
{
	FILE * fp;
	int retval = -1;
	unsigned int *dint = (unsigned int*)pdest;
	int sz = 0;
	fp = fopen(pfilename, "rb");
    if (fp == NULL || pdest == NULL) return -1;
    else
	{
		// Calcula longitud total del archivo de video
		fseek(fp,0 ,SEEK_END);
		sz = ftell(fp);
		fseek(fp,0 ,SEEK_SET);
		
#ifdef _DEBUG
		printf("File Size = %d\n", sz);
#endif		
		if(sz > sizeof(_LocalBuffer))
		{
			printf("ERROR: Buffer Lectura < Largo Archivo (%d <  %d)\n", (int)sizeof(_LocalBuffer), sz);	
		}
		else if(fread(pdest, sz, 1, fp) != -1)
		{
			_LocalFileLen = sz;

#ifdef _DEBUG
			printf("ReadVideoFile OK \n\r");
			for(int x = 0; x < _MAXLGVIDEO2ENC; x++)
				printf("%02X", pdest[x]);
#endif			
			if(*((unsigned int*)pdest) != _ENCFLAGVAL)
				printf("\nERROR: Archivo [%s] NO es un video encriptado SVV %X\n", pfilename,(int)*dint);
			else retval = 0;
		}
		else
			printf("Error de Lectura Archivo %s\n\r", pfilename);			
	}	
	fclose(fp);
	return retval;
}

int WriteVideoFile(const char *pfilename, unsigned char *pdest, int filelen, unsigned char enc)
{
	FILE * fp;
	int retval = -1;
	fp = fopen(pfilename, "wb");
	if (fp == NULL || pdest == NULL) 
		printf("ERROR Apertura Archivo %s\n", pfilename);
	else
    {
		// Encripta/Desencripta buffer
		if(enc) _EncriptDecript(pdest, _MAXLGVIDEO2ENC, 0);
		if(fwrite(pdest, filelen, 1, fp) != -1)
		{

#ifdef _DEBUG
			printf("WriteFile OK \n\r");
			for(int x = 0; x < _MAXLGVIDEO2ENC; x++)
				printf("%02X", pdest[x]);
#endif			
			// Restaura buffer
			if(enc) _EncriptDecript(pdest, _MAXLGVIDEO2ENC, 0);
			retval = 0;		
		}
		else
			printf("Error de Escritura %s\n\r", pfilename);
	}
	
	fclose(fp);
	return retval;
}
*/

int strcopys(char *dest, char *src, unsigned char stp)
{
	int pos = 0;
	if(dest && src)
	{
		while(*src)
		{
			if(*src == stp) return pos;
			else 
			{
				memcpy(dest, src, 1);
				dest++;
				src++;
				pos++;
			}
		}
	}
	return 0;
}
/*
int main(int argc,char *argv[])
{
	char fname[_MAXVFILENAME];

	if(argc < 2)
	{
		printf("\nDSVV Ver 1.0\nEspecificar Archivo a desencriptar: dsvv [archivo.svv]  [opcion] \n opcion: -v devuelve detalle de salida\n ejemplo: dsvv archivo.svv -v\n");
		return -1;	
	}
	else if((strlen(argv[1]) > 4) && (strlen(argv[1]) < sizeof(fname) ) )
	{
		// Crea el nuevo nombre con extensión mp4
		memset(fname, 0, sizeof(fname));
		int ptrpos = strcopys(fname, argv[1], '.');
		memcpy(&fname[ptrpos], _VIDEOFILE_OUTPUTEXT, 4);

		// Lee el archivo completo y lo vuelca en memoria estática
		if(ReadVideoFile(argv[1], _LocalBuffer) == 0)
		{
			// Crea un nuevo archivo descriptado con extensión MP4
			if(WriteVideoFile(fname,_LocalBuffer,_LocalFileLen ,1) == 0)
			{
				// Si existe opción -v imprime detalle
				if((argc > 2) && (strcmp(argv[2],"-v") == 0))
					printf("Archivo %s Desencriptado OK\n", fname);
			}
			else 
			{
				printf("ERROR: Escritura de Archivo %s\n", fname);
				return -3;
			}
		}
		else 
		{
			printf("ERROR: Lectura de Archivo %s\n", argv[1]);
			return -2;	
		}
	}
	else
	{
		printf("ERROR: Nombre de Archivo invalido\n");	
		return -1;
	}
	
	return 0;
}
*/




