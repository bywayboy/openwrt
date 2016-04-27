cmake_minimum_required(VERSION 2.6)

PROJECT(uvhttpd)

SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_C_FLAGS "-Wall -O3 -std=gnu99 -pthread -fpic -D_GNU_SOURCE")


SET(SOURCES 
	automem.c
	base64.c
	cfg.c
	http_parser.c
	rbtree.c
	session.c
	sha1.c
#	strptime.c
	utils.c
	uvhttpd.c
	webctx.c
	webrequest.c
	webserver.c
	websocket.c
)

SET(LINK_LIBS uv uci pthread dl lua m)



ADD_EXECUTABLE(uvhttpd ${SOURCES})



TARGET_LINK_LIBRARIES(uvhttpd ${LINK_LIBS})

INSTALL(TARGETS uvhttpd
	RUNTIME DESTINATION /usr/sbin
	
)
INSTALL(PROGRAMS files/etc/init.d/uvhttpd DESTINATION /etc/init.d
)