#ifndef _CMC_WEBSOCKET_H
#define _CMC_WEBSOCKET_H

#if !defined(LITTLEENDIANNESS) && !defined(BIGENDIANNESS)
#if defined(_M_IX86) || defined(__i386__) || defined(_M_X64) || defined(__x86_64__) || defined(__MIPSEL__) || defined(__ARMEL__)
	//С��ƽ̨ �ֱ��� X86 X64 MIPS ARM
	#define LITTLEENDIANNESS
#elif defined(__MIPSEB__) || defined(__ARMEB__)
	#define BIGENDIANNESS
#endif
#endif

#pragma pack(push, 1)
typedef struct wsFrameHeader wsFrameHeader;
struct wsFrameHeader {
#if defined(BIGENDIANNESS)
	uint8_t fin : 1;
	uint8_t rsv13 : 3;
	uint8_t opcode : 4;

	uint8_t mask : 1;
	uint8_t plen : 7;
#elif defined(LITTLEENDIANNESS)
	uint8_t opcode : 4;
	uint8_t rsv13 : 3;
	uint8_t fin : 1;

	uint8_t plen : 7;
	uint8_t mask : 1;

#endif
#if defined(_MSC_VER)
};
#else
}__attribute__((packed));
#endif
#pragma pack(pop)

enum wsParserState{
	WS_STATE_OPCODE, // FIN RSV1-3 OPCODE
	WS_STATE_PLEN, //payload len
	WS_STATE_PLLEN,
	WS_STATE_MASK,
	WS_STATE_PAYLOAD,
};

enum wsFrameType {
	WS_CONTINUATION_FRAME = 0x00, //����֡
	WS_TEXT_FRAME = 0x01,	// �ı�֡
	WS_BINARY_FRAME = 0X02, // ������֡
	WS_CLOSE_FRAME = 0x08,	// �ر� ֡
	WS_PING_FRAME = 0x09,	// ping ֡.
	WS_PONG_FRAME = 0x0A,	// pong ֡
};

typedef struct wsparser wsparser_t;
typedef struct wsFrame wsFrame;
struct wsFrame {
	uint8_t opcode;
	uint64_t len;
	unsigned char * pdata;
};

struct wsparser {
	uint8_t ver : 6;		/*only 13 */
	uint8_t fin : 1;
	uint8_t mask : 1;
	uint8_t opcode;
	uint8_t msk_bytes[4];
	uint64_t len;
	enum wsParserState	st;			// parser state.
	uint32_t	offset,maxsize;		// parser position
	automem_t buf;		// parser buffer.
	automem_t payload;
};

/*
	ִ������
*/
int ws_do_handeshake(automem_t * mem, char * key,int lkey);


void wsparser_init(wsparser_t * parser, int ver,  size_t maxsize);
void wsparser_uninit(wsparser_t * parser);

typedef void(*wsparser_onhandler)(wsparser_t * par, wsFrame * frame, void * eParam);

void wsparser_pushdata(wsparser_t * parser, unsigned char * data, uint32_t size, wsparser_onhandler handler, void * eParam);
/*
	�ú������ڹ���һ������֡.
	_mask ���� ָ�������Ƿ�ʹ��mask ���򵥼���.
*/
void wsframe_make(automem_t * mem, enum wsFrameType opcode, int _mask, unsigned char * payload, size_t len);

/* �ͷ�����֡ */
void wsframe_free(wsFrame * frame);
#endif