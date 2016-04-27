#include <stdio.h>
#include <stdint.h>
#include <time.h>
#if defined(_WIN32) || defined(_WIN64)
#include <Winsock2.h>
#endif

#include "sha1.h"
#include "base64.h"
#include "automem.h"
#include "websocket.h"
static const char secret[] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

// 大小端转换宏
#if defined(LITTLEENDIANNESS)
#define SWAP_U64(x) \
	(uint64_t)(\
	(uint64_t)(((uint64_t)(x) & 0xFF00000000000000) >> 56) | \
	(uint64_t)(((uint64_t)(x) & 0x00FF000000000000) >> 40) | \
	(uint64_t)(((uint64_t)(x) & 0x0000FF0000000000) >> 24) | \
	(uint64_t)(((uint64_t)(x) & 0x000000FF00000000) >> 8) | \
	(uint64_t)(((uint64_t)(x) & 0x00000000FF000000) << 8) | \
	(uint64_t)(((uint64_t)(x) & 0x0000000000FF0000) << 24) | \
	(uint64_t)(((uint64_t)(x) & 0x000000000000FF00) << 40) | \
	(uint64_t)(((uint64_t)(x) & 0x00000000000000FF) << 56)\
	)

#define SWAP_U16(x) \
	(uint16_t)((((uint16_t)(x) & 0x00ff) << 8) | \
	(((uint16_t)(x) & 0xff00) >> 8) \
	)
#else
	#define SWAP_U64(x) x
	#define SWAP_U16(x) x
#endif


static const char SWITCH_WEBSOCKET_PROTOCOL[] =	"HTTP/1.1 101 Switching Protocols\r\n"
			"Upgrade: websocket\r\n"
			"Connection: Upgrade\r\n"
			"Sec-WebSocket-Accept: ";

int ws_do_handeshake(automem_t * mem, char * key, int lkey)
{
	char * sec;
	char sha_out[20], base64_out[40];
	size_t base64_len = sizeof(base64_out);

	if (0 == lkey) return -1;
	sec = malloc(lkey + sizeof(secret));

	strcpy(sec, key);
	strcpy(sec + lkey, secret);

	sha1(sec, lkey + sizeof(secret) - 1, sha_out);

	base64_encode(base64_out, &base64_len, sha_out, 20);
	automem_init(mem, 256);
	automem_append_voidp(mem, SWITCH_WEBSOCKET_PROTOCOL, sizeof(SWITCH_WEBSOCKET_PROTOCOL) - 1);
	automem_append_voidp(mem, base64_out, base64_len);
	automem_append_voidp(mem, "\r\n\r\n", 4);
	free(sec);
	return 0;
}


void wsparser_init(wsparser_t * parser, int ver,size_t maxsize) {
	automem_init(&parser->buf, 256);
	automem_init(&parser->payload, 2048);
	parser->offset = 0;
	parser->ver = 13;
	parser->maxsize = maxsize;
	parser->st = WS_STATE_OPCODE;
}

void wsparser_uninit(wsparser_t * parser) {
	automem_uninit(&parser->buf);
}

void wsparser_pushdata(wsparser_t * parser, unsigned char * data, uint32_t size, wsparser_onhandler handler, void * eParam)
{
	unsigned char *p = data;
	uint32_t i;
	if (parser->buf.size > 0) {
		automem_append_voidp(&parser->buf, data, size);
		data = parser->buf.pdata;
		size = parser->buf.size;
	}

	while (parser->offset < size) {
		switch (parser->st) {
		case WS_STATE_OPCODE:
			if (size - parser->offset > 2) {
				struct wsFrameHeader * fh = (struct wsFrameHeader *)&data[parser->offset];
				parser->fin = fh->fin;
				parser->mask = fh->mask;
				parser->opcode = fh->opcode==WS_CONTINUATION_FRAME? parser->opcode: fh->opcode;
				parser->offset += 2;
				parser->st = WS_STATE_PLEN; 
				printf("OpCode = %d, Fin=%d\n", parser->opcode,parser->fin);
				switch (fh->plen) {
				case 126:
					parser->st = WS_STATE_PLEN;
					break;
				case 127:
					parser->st = WS_STATE_PLLEN;
					break;
				default:
					parser->len = fh->plen;
					parser->st = parser->mask ? WS_STATE_MASK : WS_STATE_PAYLOAD;
					break;
				}
				if (WS_PING_FRAME == parser->opcode) {
					handler(parser, NULL, eParam);
				}

			}
			break;
		case WS_STATE_PLEN:
			if (size - parser->offset < 2) goto wsparser_pushdata_final;
			parser->len = SWAP_U16(*(uint16_t *)&data[parser->offset]);
			parser->offset += 2;
			parser->st = parser->mask ? WS_STATE_MASK : WS_STATE_PAYLOAD;
			break;
		case WS_STATE_PLLEN:
			if (size - parser->offset < 8) goto wsparser_pushdata_final;
			parser->len = SWAP_U64(*(uint64_t *)&data[parser->offset]);
#if defined(_DEBUG)
			printf("size = %lld", parser->len);
#endif
			parser->offset += 8;
			parser->st = parser->mask ? WS_STATE_MASK : WS_STATE_PAYLOAD;
			break;
		case WS_STATE_MASK:
			if (size - parser->offset < 4) goto wsparser_pushdata_final;
			memcpy(parser->msk_bytes, &data[parser->offset], 4);
			parser->offset += 4;
			parser->st = WS_STATE_PAYLOAD;
			break;
		case WS_STATE_PAYLOAD:
			if (size - parser->offset < parser->len) goto wsparser_pushdata_final;
			//处理帧数据
			if (parser->mask) {
				automem_t * m = &parser->payload;
				automem_ensure_newspace(m, parser->len);
				for (i = 0; i < parser->len; i++) {
					m->pdata[m->size++] = data[parser->offset+i] ^ parser->msk_bytes[i % 4];
				}
			}
			if (parser->fin == 1) {
				wsFrame * f = malloc(sizeof(wsFrame));
				f->len = parser->payload.size;
				f->opcode = parser->opcode;
				f->pdata = f->len > 0 ? parser->payload.pdata : NULL;

				handler(parser, f, eParam);
				if (f->len){
					parser->payload.pdata = NULL;
					parser->payload.size = parser->payload.buffersize = 0;
					automem_init(&parser->payload, 2048);
				}
			}

			parser->offset += (uint32_t)parser->len;
			parser->st = WS_STATE_OPCODE;
			break;
		}
	}
wsparser_pushdata_final:
	
	if (size > parser->offset) {
		if (data != parser->buf.pdata) {
			automem_append_voidp(&parser->buf, data + parser->offset, size - parser->offset);//将未用完的加入到 buf.
			parser->offset = 0;
		}
	}
	else if (size <= parser->offset) {
		if (data == parser->buf.pdata) {

		}
		if (parser->buf.buffersize > parser->maxsize)
			automem_clean(&parser->buf, parser->maxsize);
		else
			automem_reset(&parser->buf);
		parser->offset = 0;
	}

	return;
}


void wsframe_free(wsFrame * frame) {
	if (NULL != frame && frame->len > 0) {
		automem_t mem;
		mem.pdata = frame->pdata;
		mem.buffersize = mem.size = frame->len;
		automem_uninit(&mem);
	}
	free(frame);
}

/*
	构造一个或者一堆的Frame 。
	//服务器端发往客户端的数据 不允许使用 mask
*/
void wsframe_make(automem_t * mem,enum wsFrameType opcode, int _mask, unsigned char * payload, size_t len) {
	struct wsFrameHeader * fh; 
	uint8_t mask[4];
	size_t i;
	srand((unsigned int)time(NULL));
	
	automem_ensure_newspace(mem, len + 20);
	fh = (struct wsFrameHeader * )(&mem->pdata[mem->size]);

	fh->fin = 1;
	fh->opcode = opcode;
	fh->rsv13 = 0;
	fh->mask = 0;
	mem->size += 2;
	if (len < 126) {
		fh->plen = (uint8_t)len;	
	}
	else if (len <= 0xFFFF) {
		uint16_t nlen = SWAP_U16(len);
		fh->plen = 126;
		automem_append_voidp(mem, &nlen, 2);
	}
	else {
		uint64_t nlen = SWAP_U64(len);
		fh->plen = 127;
		automem_append_voidp(mem, &nlen, sizeof(uint64_t));
	}
	if (payload && len > 0){
		if (_mask) {
			fh->mask = 1;
			mask[0] = rand() % 256; mask[1] = rand() % 256; mask[2] = rand() % 256; mask[3] = rand() % 256;
			automem_append_voidp(mem, mask, 4);

			for (i = 0; i < len; i++) {
				mem->pdata[mem->size++] = payload[0] ^ mask[i % 4];
			}
		}
		else {
			automem_append_voidp(mem, payload, len);
		}
	}
}
