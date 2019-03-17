#ifndef __DISP_H__
#define __DISP_H__

#include <ft2build.h>
#include FT_FREETYPE_H

#include "text.h"

struct __DispNode;
typedef struct __DispNode DispNode, *pDispNode;

struct __DispNode {
	pNode p_node;

	pDispNode p_prev;
	pDispNode p_next;

	unsigned short _X;
	unsigned short _Y;
	unsigned short _width;
	unsigned short _height;
};

struct __DispInfo;
typedef struct __DispInfo DispInfo, *pDispInfo;

struct __DispInfo
{
	FT_Library	ft_library;
	FT_Face		ft_face_text;
	FT_Face		ft_face_anno;
	FT_Face		ft_face_tran;
};

void close_fonts(pDispInfo p_disp_info);
void open_fonts(pDispInfo p_disp_info);

void init_disp_info(pDispInfo p_disp_info);
void build_disp_list(pDispNode p_disp_node, const pLine p_line);
void dispose_disp_list(pDispNode p_disp_node);
void draw_line_to_buffer(pDispInfo p_disp_info, pDispNode p_disp_node_head, unsigned int* buffer);
void draw_tran_to_buffer(pDispInfo p_disp_info, unsigned short* p_txt16, unsigned int* buffer);

#endif
