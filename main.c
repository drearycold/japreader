#include <pspsdk.h>
#include <pspkernel.h>
#include <pspgu.h>
#include <pspdisplay.h>
#include <psppower.h>
#include <pspctrl.h>

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

#include "text.h"
#include "disp.h"
#include "dict.h"

PSP_MODULE_INFO("JapReader", 0x0000, 0, 1);
PSP_MAIN_THREAD_PARAMS(45, 256, PSP_THREAD_ATTR_USER);
PSP_HEAP_SIZE_KB(16384);

pDispInfo gpDispInfo = NULL;
pDictInfo gpDictInfo = NULL;
size_t cur_line_count = 0;

int exitCallback(int arg1, int arg2, void *common)
{
	//TODO store location
	int fd_bookmark = open("./bookmark", O_WRONLY | O_CREAT | O_TRUNC, 0666);
	char buff[256];
	int sz = sprintf(buff, "%u\n", cur_line_count);
	write(fd_bookmark, buff, sz);
	close(fd_bookmark);
	sceKernelExitGame();
	return 0;
}

int powerCallback(int arg1, int powerInfo, void* arg)
{
	if( (powerInfo & (PSP_POWER_CB_POWER_SWITCH | PSP_POWER_CB_STANDBY)) > 0)
	{
		close_fonts(gpDispInfo);
		close_dict(gpDictInfo);
	}
	else if( (powerInfo & PSP_POWER_CB_RESUME_COMPLETE) > 0)
	{
		sceKernelDelayThread(1500000);
		open_fonts(gpDispInfo);
		open_dict(gpDictInfo);
	}

	return 0;
}

int callbackThread(SceSize args, void *argp)
{
	int cbid = sceKernelCreateCallback("Exit Callback", exitCallback, NULL);
	sceKernelRegisterExitCallback(cbid);

	cbid = sceKernelCreateCallback("Power Callback", powerCallback, NULL);
	scePowerRegisterCallback(0, cbid);

	sceKernelSleepThreadCB();

	return 0;
}

unsigned int __attribute__ ((aligned(16))) list[262144];
unsigned int __attribute__((aligned(16))) pixels[512*272];

//unsigned short codes[] = {0x85E4, 0x6797, 0x59C9, 0x59B9, 0x306F, 0x540C, 0x6642, 0x653B, 0x7565, 0x304C, 0x53EF, 0x80FD, 0x3067, 0x3059, 0x0};

int mainThread(SceSize args, void *argp)
{
	sceGuInit();
	sceGuStart(GU_DIRECT, list);

	sceGuDrawBuffer(GU_PSM_8888, (void*)0, 512);
	sceGuDispBuffer(480, 272, (void*)(512*272*4), 512);
	sceGuDepthBuffer((void*)(512*272*8), 512);

	sceGuOffset( 2048 - 480/2, 2048 - 272/2 );
	sceGuViewport( 2048, 2048, 480, 272 );
	sceGuDepthRange(65535, 0);
	sceGuScissor(0, 0, 480, 272);
	sceGuEnable(GU_SCISSOR_TEST);

	sceGuFrontFace(GU_CW);
	//sceGuClearColor(0xFFFFFFFF);
	//sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

	guSwapBuffersBehaviour(PSP_DISPLAY_SETBUF_IMMEDIATE);
	sceGuEnable(GU_TEXTURE_2D);
	sceGuFinish();
	sceGuSync(0, 0);

	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);

	void* fb = (void*)(512*272*4);

	gpDispInfo = calloc(1, sizeof(DispInfo));
	init_disp_info(gpDispInfo);

	gpDictInfo = calloc(1, sizeof(DictInfo));
	open_dict(gpDictInfo);

	pText p_text = malloc(sizeof(Text));
	build_text(p_text, "xxx");

	int fd_bookmark = open("./bookmark", O_RDONLY, 0666);
	if( fd_bookmark >= 0 )
	{
		char buff[256];
		int rd = read(fd_bookmark, buff, 256);
		if( rd > 1 )
			cur_line_count = strtoul(buff, NULL, 10);
		close(fd_bookmark);
	}

	pLine curLine = p_text->p_first_line;
	for(size_t i=0; i<cur_line_count; i++)
		curLine = curLine->p_next;

	pDispNode curDispNodeHead = calloc(1, sizeof(DispNode));
	build_disp_list(curDispNodeHead, curLine);
	pDispNode curBarNode = curDispNodeHead;
	int need_redraw_line = 1;
	int need_redraw_tran = 1;

	unsigned int last_buttons = 0;
	unsigned int last_timestamp = 0;
	sceCtrlSetSamplingCycle(0);
	sceCtrlSetSamplingMode(1);
	while(1)
	{
		SceCtrlData pad;
		sceCtrlReadBufferPositive(&pad, 1);

		if( last_buttons == pad.Buttons )
		{
			if( last_timestamp < pad.TimeStamp )
			{
				sceKernelDelayThread(100000);
				//continue;
				pad.Buttons = 0;
			}
		}

		last_buttons = pad.Buttons;
		last_timestamp = pad.TimeStamp;

		if( pad.Buttons & PSP_CTRL_RTRIGGER )
		{
			curLine = curLine->p_next;
			cur_line_count ++;
		}
		else if( pad.Buttons & PSP_CTRL_LTRIGGER )
		{
			curLine = curLine->p_prev;
			cur_line_count --;
		}
		else if( pad.Buttons & PSP_CTRL_LEFT )
		{
			if( curBarNode && curBarNode->p_prev )
				curBarNode = curBarNode->p_prev;
		}
		else if( pad.Buttons & PSP_CTRL_RIGHT )
		{
			if( curBarNode && curBarNode->p_next )
				curBarNode = curBarNode->p_next;
		}
		else if( pad.Buttons & PSP_CTRL_DOWN )
		{
			if( curBarNode )
			{
				pDispNode barNode = curBarNode;
				unsigned short cur_Y = curBarNode->_Y;
				while( barNode->p_next )
				{
					barNode = barNode->p_next;
					if( barNode->_Y != cur_Y )
						break;
				}
				curBarNode = barNode;
			}
		}
		else if( pad.Buttons & PSP_CTRL_UP )
		{
			if( curBarNode )
			{
				pDispNode barNode = curBarNode;
				unsigned short cur_Y = curBarNode->_Y;
				while( barNode->p_prev )
				{
					barNode = barNode->p_prev;
					if( barNode->_Y != cur_Y )
						break;
				}
				curBarNode = barNode;
			}
		}
		else if( pad.Buttons & PSP_CTRL_SQUARE )
		{
			need_redraw_tran = 1;
		}
		else
		{
			sceKernelDelayThread(100000);
			//continue;
		}
		if( curDispNodeHead->p_node != curLine->p_first_node )
		{
			dispose_disp_list(curDispNodeHead);
			curDispNodeHead = calloc(1, sizeof(DispNode));
			build_disp_list(curDispNodeHead, curLine);
			curBarNode = curDispNodeHead;
			need_redraw_line = 1;
		}

		if( need_redraw_line )
		{
			memset(pixels, 0xFF, 512*272*4);
			draw_line_to_buffer(gpDispInfo, curDispNodeHead, pixels);
			sceKernelDcacheWritebackAll();
			need_redraw_line = 0;
		}

		if( need_redraw_tran )
		{
			memset(pixels+512*230, 0xFF, 512*42*4);

			sqlite3_stmt* pStmt;
			const void* stmtTail;

			unsigned short sql_head[] = {0x53, 0x45, 0x4c, 0x45, 0x43, 0x54, 0x20, 0x65, 0x6e, 0x67, 0x20, 0x46, 0x52, 0x4f, 0x4d, 0x20, 0x64, 0x69, 0x63, 0x74, 0x20, 0x57, 0x48, 0x45, 0x52, 0x45, 0x20, 0x6a, 0x61, 0x70, 0x3d, 0x22};
			size_t sql_head_len = 32;
			unsigned short sql_tail[] = {0x22, 0x0};
			size_t sql_tail_len = 2;
			unsigned short* p_sql = calloc(1, 256);
			size_t sql_len = 0;

			memcpy(p_sql+sql_len, sql_head, sql_head_len*sizeof(unsigned short));
			sql_len += sql_head_len;
			if( curBarNode->p_node->norm_len )
			{
				memcpy(p_sql+sql_len, curBarNode->p_node->p_norm, curBarNode->p_node->norm_len*sizeof(unsigned short));
				sql_len += curBarNode->p_node->norm_len;
			}
			else
			{
				memcpy(p_sql+sql_len, curBarNode->p_node->p_text, curBarNode->p_node->text_len*sizeof(unsigned short));
				sql_len += curBarNode->p_node->text_len;
			}
			memcpy(p_sql+sql_len, sql_tail, sql_tail_len*sizeof(unsigned short));
			sql_len += sql_tail_len;

			int rc = sqlite3_prepare16_v2(gpDictInfo->p_dictdb, p_sql, sql_len*sizeof(unsigned short), &pStmt, &stmtTail);
			while( (rc = sqlite3_step(pStmt)) == SQLITE_ROW )
			{
				unsigned short* p_txt16 = (unsigned short*)sqlite3_column_text16(pStmt, 0);
				draw_tran_to_buffer(gpDispInfo, p_txt16, pixels);
				break;
			}

			sqlite3_finalize(pStmt);

			need_redraw_tran = 0;
		}

		sceGuStart(GU_DIRECT,list);
		sceGuCopyImage(GU_PSM_8888,0,0,480,272,512,pixels,0,0,512,fb+0x04000000);
		sceGuTexSync();
		sceGuFinish();
		sceGuSync(0,0);

		sceDisplayWaitVblankStart();
		if( curBarNode )
		{
			unsigned int* dest = fb + 0x04000000;
			for(int y=0; y<2; y++)
			{
				unsigned int* row = &dest[(curBarNode->_Y + 18 + y) * 512];
				for(int x=0; x<curBarNode->_width; x++)
				{
					row[x+curBarNode->_X] = 0xFF | 0xFF000000;
				}
			}
		}
		sceDisplayWaitVblankStart();
		//sceGuSwapBuffers();

		//sceKernelDelayThread(100000);
	}

	return 0;
}

int testThread(SceSize args, void *argp)
{
	sceGuInit();
	sceGuStart(GU_DIRECT, list);

	sceGuDrawBuffer(GU_PSM_8888, (void*)0, 512);
	sceGuDispBuffer(480, 272, (void*)(512*272*4), 512);
	sceGuDepthBuffer((void*)(512*272*8), 512);

	sceGuOffset( 2048 - 480/2, 2048 - 272/2 );
	sceGuViewport( 2048, 2048, 480, 272 );
	sceGuDepthRange(65535, 0);
	sceGuScissor(0, 0, 480, 272);
	sceGuEnable(GU_SCISSOR_TEST);

	sceGuFrontFace(GU_CW);
	//sceGuClearColor(0xFFFFFFFF);
	//sceGuClear(GU_COLOR_BUFFER_BIT | GU_DEPTH_BUFFER_BIT);

	guSwapBuffersBehaviour(PSP_DISPLAY_SETBUF_IMMEDIATE);
	sceGuEnable(GU_TEXTURE_2D);
	sceGuFinish();
	sceGuSync(0, 0);

	sceDisplayWaitVblankStart();
	sceGuDisplay(GU_TRUE);

	pDispInfo p_disp_info = calloc(1, sizeof(DispInfo));
	init_disp_info(p_disp_info);

	pText p_text = malloc(sizeof(Text));
	build_text(p_text, "xxx");

	pDispNode p_disp_node = calloc(1, sizeof(DispNode));
	build_disp_list(p_disp_node, p_text->p_first_line);

	for(int y=0; y<272; y++)
	{
		unsigned int* row = &pixels[y * 512];
		for(int x=0; x<512; x++)
		{
			row[x] = x*y;
		}
	}

	void* fb = (void*)(512*272*4);
	sceGuStart(GU_DIRECT,list);
	sceGuCopyImage(GU_PSM_8888,0,0,480,272,512,pixels,0,0,512,fb+0x04000000);
	sceGuTexSync();
	sceGuFinish();
	sceGuSync(0,0);

	sceDisplayWaitVblankStart();

	while(1)
		sceKernelDelayThread(100000);
	
	return 0;
}

int main(int argc, char* argv[])
{
	int thid = sceKernelCreateThread("update_thread", callbackThread, 0x11, 0xFA0, 0, 0);
	if(thid >= 0) {
		sceKernelStartThread(thid, 0, 0);
	}

	thid = sceKernelCreateThread("main_thread", mainThread, 0x11, 0xFA0, 0, 0);
	//thid = sceKernelCreateThread("main_thread", testThread, 0x11, 0xFA0, 0, 0);
	if(thid >= 0) {
		sceKernelStartThread(thid, 0, 0);
	}

	scePowerSetClockFrequency(133, 33, 47);
	
	sceKernelSleepThread();
	return 0;
}
