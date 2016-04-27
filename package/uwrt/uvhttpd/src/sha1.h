/* sha1.h */

/* If OpenSSL is in use, then use that version of SHA-1 */
#ifndef __SHA1_INCLUDE_
#define __SHA1_INCLUDE_

#ifdef __cplusplus
extern "C"{
#endif`

typedef struct {
	uint32_t state[5];
	uint32_t count[2];
	unsigned char buffer[64];
} SHA1_CTX;

void SHA1_Transform(uint32_t state[5], const unsigned char buffer[64]);
void SHA1_Init(SHA1_CTX* context);
void SHA1_Update(SHA1_CTX* context, const unsigned char* data, uint32_t len);
void SHA1_Final(unsigned char digest[20], SHA1_CTX* context);

void sha1(char * s, unsigned int l, unsigned char out[20]);

#ifdef __cplusplus
};
#endif`

#endif /* __SHA1_INCLUDE_ */
