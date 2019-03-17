#ifndef __DICT_H__
#define __DICT_H__

#include <sqlite3.h>

#include "type.h"

struct __DictInfo {
	sqlite3* p_dictdb;
};

void open_dict(pDictInfo p_dict_info);
void close_dict(pDictInfo p_dict_info);

#endif
