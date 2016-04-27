#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "automem.h"
#include "lua.h"
#include "uv.h"

#include "webserver.h"
#include "utils.h"
#include "session.h"
#include "base64.h"

#include "sha1.h"

int luasession_init(lua_State * L, webqueuework_t * qwork)
{
	const char * socket = webheader_get(&qwork->request->headers, "Cookie");
	const char *key = NULL;
	char keybuf[46 + sizeof("UVSESS=")]="UVSESS=",i,j=sizeof("UVSESS=")-1, out[20];

	if ((NULL!= socket) && (key = strstr(socket, "UVSESS="))) {
		key += sizeof("UVSESS=") - 1;
	}
	if (NULL == key) {
		int olen;
		unsigned char h, l;
		for (i = 0; i < 16; i++) {
			keybuf[j++] = (unsigned char)(rand() % 256);
		}
		*(void**)(&keybuf[j]) = qwork;
		j += sizeof(void*);
		olen = sizeof("UVSESS=") - 1;
		sha1(keybuf, j, out);
		for (i = 0; i < sizeof(out); i++) {
			h = ((unsigned char)out[i] >> 4) & 0x0F;
			l = (unsigned char)out[i] & 0x0F;
			keybuf[olen++] = h > 9 ? h + ('A'-10) : h + '0';
			keybuf[olen++] = l > 9 ? l + ('A'-10) : l + '0';
		}
		keybuf[olen] = 0;
		key = keybuf+sizeof("UVSESS=")-1;
		webheader_set(&qwork->headers, "Set-Cookie", keybuf);
	}
	//puts(key);
}

#define _LUA_TENDDATA	0xFF
static void lua_def_serialone(automem_t * mem, lua_State * L, int idx);
static void _lua_defparser_serialtable(automem_t * mem, lua_State * L, int si)
{
	int top = lua_gettop(L);
	lua_checkstack(L, 5);
	lua_pushnil(L);
	while (lua_next(L, si)) {
		// -1 value -2 key
		int t = lua_gettop(L);
		lua_def_serialone(mem, L, t - 1);
		lua_def_serialone(mem, L, t);
		lua_pop(L, 1);
	}
	automem_append_byte(mem, _LUA_TENDDATA);
	lua_settop(L, top);
}

/*序列化栈里面的一个变量. 索引需要时正数.*/
static void lua_def_serialone(automem_t * mem, lua_State * L, int idx)
{
	int t, argc = lua_gettop(L);
	const char *str;
	size_t lstr;
	lua_Number d;
	t = lua_type(L, idx);
	switch (t) {
	case LUA_TBOOLEAN:
		automem_append_byte(mem, LUA_TBOOLEAN);
		automem_append_byte(mem, lua_toboolean(L, idx) ? 0x01 : 0x00);
		break;
	case LUA_TSTRING:
		automem_append_byte(mem, LUA_TSTRING);
		str = lua_tolstring(L, idx, &lstr);
		luadef_write_length(mem, lstr);
		if (lstr > 0) {
			automem_append_voidp(mem, str, lstr);
		}
		//lua_pop(L,1);
		break;
	case LUA_TNUMBER:
		d = lua_tonumber(L, idx);
		automem_append_byte(mem, LUA_TNUMBER);
		automem_append_voidp(mem, &d, sizeof(lua_Number));
		break;
	case LUA_TTABLE:
		automem_append_byte(mem, LUA_TTABLE);
		_lua_defparser_serialtable(mem, L, idx);
		break;
	case LUA_TNIL:
		automem_append_byte(mem, LUA_TNIL);
		break;
	default:
		luaL_error(L, "Unsupport datatype %s to serialize.", lua_typename(L, idx));
		break;
	}
}

static int lua_pushdata(lua_State * L, const char * buf);
static int lua_pushtable(lua_State * L, const char * buf)
{
	const char * buffer = buf;
	lua_newtable(L);
	while ((unsigned char)*buffer != _LUA_TENDDATA) {
		int used = lua_pushdata(L, buffer);
		if (used > 0) {
			buffer += used;
			used = lua_pushdata(L, buffer);
			if (used > 0) {
				buffer += used;
				lua_settable(L, -3);
				continue;
			}
		}
		return -1;
	}
	return (1 + buffer) - buf;
}

static int lua_pushdata(lua_State * L, const char * buf)
{
	char c;
	const char * buffer = buf;
	int used = 0, lstr;
	double d;
	c = *buffer++;
	lua_checkstack(L, 5);
	switch (c) {
	case LUA_TSTRING: // string 类型.
		lstr = luadef_read_length((unsigned char *)buffer, &used);
		lua_pushlstring(L, buffer + used, lstr);
		buffer += used + lstr;
		break;
	case LUA_TBOOLEAN:
		lua_pushboolean(L, *buffer++);
		break;
	case LUA_TNUMBER:
		d = *(double *)buffer;
		buffer += sizeof(double);
		if (!(d - (int64_t)d > 0.0))
		{
			if (INT_MAX >= d && INT_MIN <= d) {
				lua_pushinteger(L, (int)d);
			}
			else if (UINT_MAX >= d && d >= 0) {
				lua_pushunsigned(L, (unsigned int)d);
			}
			break;
		}
		lua_pushnumber(L, d);
		break;
	case LUA_TNIL:
		lua_pushnil(L);
		break;
	case LUA_TTABLE: {
		int used = lua_pushtable(L, buffer);
		if (used == -1)
			return used;
		buffer += used;
		break;
	}default:
		return -1;
	}
	return buffer - buf;
}

static int lua_parse_table(lua_State * L, char * buf);
static int lua_parse_one(lua_State * L, char * buf) {
	int pos = 1;
	switch (buf[0]) {
	case LUA_TNUMBER:
		lua_pushnumber(L, *(lua_Number *)&buf[pos]);
		pos += sizeof(lua_Number);
		break;
	case LUA_TSTRING: {
		int used;
		int sz = luadef_read_length(&buf[pos], &used);
		lua_pushlstring(L, buf, sz);
		pos += (used + sz);
	}break;
	case LUA_TBOOLEAN:
		lua_pushboolean(L, buf[pos] ? 1 : 0);
		pos++;
		break;
	case LUA_TTABLE:
		pos += lua_parse_table(L, &buf[0]);
		break;
	case LUA_TNIL:
		lua_pushnil(L);
		break;
	default:
		return 0;
	}
	return pos;
}
static int lua_parse_table(lua_State * L, char * buf) {
	int pos = 1,r1,r2;
	lua_newtable(L);
	while (buf[0] != _LUA_TENDDATA)
	{
		if (r1 = lua_parse_one(L, buf[pos])) {
			r2 = lua_parse_one(L, buf[pos]);
			if (0 == r2) {
				lua_pushnil(L);
			}
			lua_settable(L, -3);
			pos += (r1 + r2);
		}
	}
	return pos;
}


int session_parse(lua_State * L){
	size_t pos,len, retc = 0;
	const char * buf = lua_tolstring(L, 1, &len);
	pos = len;
	int r;
	while (r = lua_parse_one(L, &buf[pos])) {
		retc++;
		pos += r;
	}
	return retc;
}
