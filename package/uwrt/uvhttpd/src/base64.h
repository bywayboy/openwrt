#ifndef _QR_B64_H
#define _QR_B64_H

#ifdef __cplusplus
extern "C"{
#endif`
int base64_encode( unsigned char *dst, int *dlen,
				  unsigned char *src, int  slen );

#ifdef __cplusplus
};
#endif`
#endif