#include <pspsdk.h>
#include <pspkernel.h>

#include "text.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
//#include <stdio.h>

void build_text(pText p_text, const char* filename)
{
	size_t line_count = 0;
	unsigned short* buffer = malloc(65536);
	unsigned short* buff_end = buffer + 32768;

	unsigned short* line_buf = malloc(65536);
	size_t line_head_len = 0;

	//SceUID fd = sceIoOpen(filename, PSP_O_RDONLY, 0777);
	int fd = open(filename, O_RDONLY, 0777);
	int rd = 0;
	//while( (rd = sceIoRead(fd, buffer, 65536)) > 0 )
	while( (rd = read(fd, buffer, 65536)) > 0 )
	{
		unsigned short* buff_head = buffer;
		unsigned short* buff_newline = buffer;
		while( buff_newline < buff_end )
		{
			while( *buff_newline != 0x000A )
			{
				buff_newline ++;
				if( buff_newline == buff_end )
					break;
			}
			if( buff_newline < buff_end )
			{	//got a newline
				size_t line_len = buff_newline - buff_head;
				memcpy(line_buf+line_head_len, buff_head, line_len*2);
				line_buf[line_len+line_head_len] = 0;

				pLine p_line = calloc(1, sizeof(Line));
				build_line(p_line, line_buf);
				if( p_text->p_last_line )
				{
					p_line->p_prev = p_text->p_last_line;
					p_text->p_last_line->p_next = p_line;
					p_text->p_last_line = p_line;
				}
				else
					p_text->p_first_line = p_text->p_last_line = p_line;

				line_count ++;

				buff_head = buff_newline + 1;
				line_head_len = 0;
				buff_newline ++;
			}
		}
		{	//beyond end of buffer
			size_t frag_len = buff_newline - buff_head;
			memcpy(line_buf, buff_head, frag_len*2);
			line_head_len = frag_len;
		}
	}
	//sceIoClose(fd);
	free(buffer);
	free(line_buf);
	close(fd);
}

void build_line(pLine p_line, unsigned short* data)
{
	size_t node_count = 0;
	pNode curNode = NULL;
	p_line->p_first_node = curNode;
	p_line->p_last_node = curNode;

	size_t text_begin = 0;
	size_t text_end = 0;
	size_t anno_begin = 0;
	size_t anno_end = 0;
	size_t norm_begin = 0;
	size_t norm_end = 0;
	for(size_t idx = 0; data[idx]; idx++ )
	{
		if( data[idx] == 0xE000 )
		{
			if( anno_begin )
				anno_end = idx - 1;
			if( text_begin )
				text_end = idx - 1;
			if( norm_begin )
				norm_end = idx - 1;
		}
		else if( data[idx] == 0xE001 )
		{
			text_end = idx - 1;
			anno_begin = idx + 1;
		}
		else if( data[idx] == 0xE002 )
		{
			if( text_begin )
				text_end = idx - 1;
			if( anno_begin )
				anno_end = idx - 1;
			norm_begin = idx + 1;
		}
		if( text_begin && text_end )
		{
			size_t text_len = text_end + 1 - text_begin;
			curNode->p_text = malloc(text_len*2);
			memcpy(curNode->p_text, data + text_begin, text_len*2);
			curNode->text_len = text_len;

			text_begin = 0;
			text_end = 0;
		}
		if( anno_begin && anno_end )
		{
			size_t anno_len = anno_end + 1 - anno_begin;
			curNode->p_anno = malloc(anno_len*2);
			memcpy(curNode->p_anno, data + anno_begin, anno_len*2);
			curNode->anno_len = anno_len;
			
			anno_begin = 0;
			anno_end = 0;
		}
		if( norm_begin && norm_end )
		{
			size_t norm_len = norm_end + 1 - norm_begin;
			curNode->p_norm = malloc(norm_len*2);
			memcpy(curNode->p_norm, data+norm_begin, norm_len*2);
			curNode->norm_len = norm_len;

			norm_begin = 0;
			norm_end = 0;
		}
		if( data[idx] == 0xE000 )
		{
			text_begin = idx + 1;
			curNode = calloc(1, sizeof(Node));
			if( p_line->p_last_node )
			{
				p_line->p_last_node->p_next = curNode;
				p_line->p_last_node = curNode;
			}
			else
				p_line->p_first_node = p_line->p_last_node = curNode;
			node_count ++;
		}
	}
}

/*
int main(int argc, char* argv)
{
	pText p_text = calloc(1, sizeof(Text));
	build_text(p_text, "xxx");

	pLine curLine = p_text->p_first_line;
	unsigned short newline = 0x000A;
	while( curLine)
	{
		pNode curNode = curLine->p_first_node;
		while( curNode )
		{
			if( curNode->p_text )
			{
				for(size_t i=0; curNode->p_text[i]; i++)
					fwrite(curNode->p_text+i, 2, 1, stdout);
			}
			if( curNode->p_anno )
			{
				for(size_t i=0; curNode->p_anno[i]; i++)
					fwrite(curNode->p_anno+i, 2, 1, stdout);
			}
			curNode = curNode->p_next;
		}
		fwrite(&newline, 2, 1, stdout);
		curLine = curLine->p_next;
	}

	return 0;
}*/
