#ifndef __HTTP_H__
#define __HTTP_H__

#include "http_base.h"
#include "http_handle.h"
#include "http_parse.h"

void dorequest(struct schedule *s, void *ud);

#endif