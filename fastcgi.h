#ifndef __FASTCGI_H__
#define __FASTCGI_H__

#include "util.h"

/*
 * FCGI_Header中的字节数。协议的未来版本不会减少该数。
 */
#define FCGI_HEADER_LEN         8

/*
 * 可用于FCGI_Header的version组件的值
 */
#define FCGI_VERSION_1           1

/*
 * 可用于FCGI_Header的type组件的值
 */
#define FCGI_BEGIN_REQUEST       1
#define FCGI_ABORT_REQUEST       2
#define FCGI_END_REQUEST         3
#define FCGI_PARAMS              4
#define FCGI_STDIN               5
#define FCGI_STDOUT              6
#define FCGI_STDERR              7
#define FCGI_DATA                8
#define FCGI_GET_VALUES          9
#define FCGI_GET_VALUES_RESULT  10
#define FCGI_UNKNOWN_TYPE       11
#define FCGI_MAXTYPE            (FCGI_UNKNOWN_TYPE)

/*
 * 可用于FCGI_Header的requestId组件的值
 */
#define FCGI_REQUEST_ID          1

/*
 * 可用于FCGI_BeginRequestBody的flags组件的掩码
 */
#define FCGI_KEEP_CONN           1
/*
 * 可用于FCGI_BeginRequestBody的role组件的值
 */
#define FCGI_RESPONDER           1
#define FCGI_AUTHORIZER          2
#define FCGI_FILTER              3

/*
 * 可用于FCGI_EndRequestBody的protocolStatus组件的值
 */
#define FCGI_REQUEST_COMPLETE    0
#define FCGI_CANT_MPX_CONN       1
#define FCGI_OVERLOADED          2
#define FCGI_UNKNOWN_ROLE        3
/*
 * 可用于FCGI_GET_VALUES/FCGI_GET_VALUES_RESULT记录的变量名
 */
#define FCGI_MAX_CONNS  "FCGI_MAX_CONNS"
#define FCGI_MAX_REQS   "FCGI_MAX_REQS"
#define FCGI_MPXS_CONNS "FCGI_MPXS_CONNS"

typedef struct {
    unsigned char version;
    unsigned char type;
    unsigned char requestIdB1;
    unsigned char requestIdB0;
    unsigned char contentLengthB1;
    unsigned char contentLengthB0;
    unsigned char paddingLength;
    unsigned char reserved;
} FCGI_Header;

typedef struct {
    unsigned char roleB1;
    unsigned char roleB0;
    unsigned char flags;
    unsigned char reserved[5];
} FCGI_BeginRequestBody;

typedef struct {
    FCGI_Header header;
    FCGI_BeginRequestBody body;
} FCGI_BeginRequestRecord;

typedef struct{
	FCGI_Header header;
	unsigned char nameLength;
	unsigned char valueLength;
	unsigned char data[0];
}FCGI_ParamsRecord;

/*
 * 构造请求头部，返回FCGI_Header结构体
 */
FCGI_Header make_header(
		int type,
		int requestId,
		int contentLength,
		int paddingLength);

/*
 * 构造请求体，返回FCGI_BeginRequestBody结构体
 */
FCGI_BeginRequestBody make_beginRequestBody(
		int role);

#endif