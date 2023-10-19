#ifndef _WebSocketObj_HPP_
#define _WebSocketObj_HPP_

#include<cstdint>

enum WebSocketOpcode {
	opcode_CONTINUATION = 0x0,
	opcode_TEXT = 0x1,
	opcode_BINARY = 0x2,
	opcode_CLOSE = 0x8,
	opcode_PING = 0x9,
	opcode_PONG = 0xA,
};

//WebSocket header protocal
struct WebSocketHeader {
	//Extended payload length 16 or 64 bit
	uint64_t len;
	//Opcode decides how to decipher the following payload
	WebSocketOpcode opcode;
	//all data sent to server should be masked
	uint8_t masking_key[4];
	//to denote whether this message is the last fragment
	bool fin;
	//denote whether we need to mask data
	//all data sent to server should be masked
	//However, data from server to client doesn't need to do that
	bool mask;
	//length of payload
	//if len0 is within 0~126, then the data length is len0
	//if len0 is 126, then the following short data denotes the length
	//if len0 is 127then the following 8 bytes data denotes the length
	//length data is big endian
	uint8_t len0;
	//WebSocket header length
	uint8_t header_size;
};

#endif
