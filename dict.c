#include "dict.h"

void open_dict(pDictInfo p_dict_info)
{
	sqlite3_open("ms0:/PSP/GAME/xxx/dict.sqlite", &p_dict_info->p_dictdb);
}

void close_dict(pDictInfo p_dict_info)
{
	sqlite3_close(p_dict_info->p_dictdb);
}


