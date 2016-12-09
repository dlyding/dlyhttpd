#ifndef __HTTP_H__
#define __HTTP_H__

#include <sys/sendfile.h>
#include "http_base.h"
#include "http_handle.h"
#include "http_parse.h"
#include "fastcgi.h"

void dorequest(schedule_t *s, void *ud);

#endif