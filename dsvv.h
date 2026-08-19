#ifndef __DSVV_H__
    #define __DSVV_H__

//----------------------------------------------------------------------------------------
#define _VIDEOFILE_INPUTEXT		"svv"
#define _VIDEOFILE_OUTPUTEXT	".mp4"
/*
#define _MAXVFILENAME			64
#define _BIGBUFFER_SIZE			0x3000000LU // Tamaño máximo según duración del video (50 MBytes)
*/
#define _MAXLGVIDEO2ENC			1024
#define _ENCDECSEED				0x5A22F189UL
#define _ENCFLAGVAL			    0x49495653UL
#define _DECFLAGVAL				0x20000000UL
//#define _DEBUG	
//----------------------------------------------------------------------------------------



int strcopys(char *dest, char *src, unsigned char stp);
void _EncriptDecript(unsigned char *_io, int _lg, int enc);



#endif    