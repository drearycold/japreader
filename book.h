#ifndef __BOOK_H__
#define __BOOK_H__

#include <sqlite3.h>

#include "type.h"

struct __BookInfo {
	sqlite3* p_bookdb;
};

void open_book(pBookInfo p_book_info);
void close_book(pBookInfo p_book_info);

#endif
