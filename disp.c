#include "disp.h"

#include <stdlib.h>

#define FONT_SIZE_TEXT	16
#define FONT_SIZE_ANNO	9
#define FONT_SIZE_TRAN	12

#define LINE_HEIGHT_TEXT	16
#define LINE_HEIGHT_ANNO	10
#define LINE_HEIGHT_TRAN	13

void build_disp_list(pDispNode p_disp_node, const pLine p_line)
{
	pNode curTextNode = p_line->p_first_node;
	pDispNode curDispNode = p_disp_node;

	while( curTextNode )
	{
		curDispNode->p_node = curTextNode;

		curDispNode->p_next = calloc(1, sizeof(DispNode));
		curDispNode->p_next->p_prev = curDispNode;

		curTextNode = curTextNode->p_next;
		curDispNode = curDispNode->p_next;
	}
}

void dispose_disp_list(pDispNode p_disp_node)
{
	pDispNode curDispNode = p_disp_node;
	pDispNode nextDispNode = NULL;
	while( curDispNode )
	{
		nextDispNode = curDispNode->p_next;
		free(curDispNode);
		curDispNode = nextDispNode;
	}
}

void close_fonts(pDispInfo p_disp_info)
{
	FT_Done_Face(p_disp_info->ft_face_text);
	FT_Done_Face(p_disp_info->ft_face_anno);
	FT_Done_Face(p_disp_info->ft_face_tran);
}

void open_fonts(pDispInfo p_disp_info)
{
	FT_New_Face(p_disp_info->ft_library, 
				"./msgothic.ttc", 
				1, &p_disp_info->ft_face_text);

	FT_New_Face(p_disp_info->ft_library, 
				"./msgothic.ttc", 
				1, &p_disp_info->ft_face_anno);

	FT_New_Face(p_disp_info->ft_library, 
				"./arial.ttf", 
				0, &p_disp_info->ft_face_tran);

	FT_Set_Pixel_Sizes(p_disp_info->ft_face_text, 0, FONT_SIZE_TEXT);
	FT_Set_Pixel_Sizes(p_disp_info->ft_face_anno, 0, FONT_SIZE_ANNO);
	FT_Set_Pixel_Sizes(p_disp_info->ft_face_tran, 0, FONT_SIZE_TRAN);
}

void init_disp_info(pDispInfo p_disp_info)
{
	FT_Init_FreeType(&p_disp_info->ft_library);
	open_fonts(p_disp_info);
}

void draw_line_to_buffer(pDispInfo p_disp_info, pDispNode p_disp_node, unsigned int* buffer)
{
	size_t pen_anno_X = 4;
	size_t pen_text_X = 4;
	size_t pen_anno_Y = 4;
	size_t pen_text_Y = pen_anno_Y + FONT_SIZE_ANNO + 1;

	FT_Face ft_face_text = p_disp_info->ft_face_text;	//helper
	FT_Face ft_face_anno = p_disp_info->ft_face_anno;	//helper
	FT_GlyphSlot slot_text = ft_face_text->glyph;
	FT_GlyphSlot slot_anno = ft_face_anno->glyph;

	for( pDispNode curNode = p_disp_node; curNode && curNode->p_node; curNode = curNode->p_next )
	{
		if( pen_text_X < pen_anno_X )
			pen_text_X = pen_anno_X;
		if( pen_text_X + curNode->p_node->text_len*FONT_SIZE_TEXT > 480 )
		{
			pen_text_X = 4;
			pen_text_Y += LINE_HEIGHT_TEXT + LINE_HEIGHT_ANNO;
			pen_anno_Y += LINE_HEIGHT_TEXT + LINE_HEIGHT_ANNO;
		}
		curNode->_X = pen_text_X;
		curNode->_Y = pen_text_Y;

		pen_anno_X = pen_text_X;
		const unsigned short* p_text = curNode->p_node->p_text;
		for( size_t code_idx=0; p_text && code_idx < curNode->p_node->text_len; code_idx ++)
		{
			FT_UInt glyph_index = FT_Get_Char_Index(ft_face_text, p_text[code_idx]);
			FT_Load_Glyph(ft_face_text, glyph_index, FT_LOAD_NO_BITMAP);
			FT_Render_Glyph(ft_face_text->glyph, FT_RENDER_MODE_LCD);

			size_t width = slot_text->bitmap.width / 3;
			int pad_y_top = LINE_HEIGHT_TEXT - slot_text->bitmap_top;
			int pad_y_bottom = LINE_HEIGHT_TEXT - pad_y_top - slot_text->bitmap.rows;
			for(int y=0; y<pad_y_top; y++)
			{
				unsigned int* row = &buffer[(pen_text_Y+y) * 512];
				for(size_t x=0; x<width; x++)
				{
					row[x+pen_text_X] = 0xFFFFFFFF;
				}
			}
			for(size_t y=0; y<slot_text->bitmap.rows; y++)
			{
				unsigned int* row = &buffer[(pen_text_Y+y+pad_y_top) * 512];
				for(size_t x=0; x<width; x++)
				{
					unsigned char r = 255 - *(slot_text->bitmap.buffer + x*3 + y*slot_text->bitmap.pitch);
					unsigned char g = 255 - *(slot_text->bitmap.buffer + x*3+1 + y*slot_text->bitmap.pitch);
					unsigned char b = 255 - *(slot_text->bitmap.buffer + x*3+2 + y*slot_text->bitmap.pitch);
					row[x+pen_text_X] = (r) | (g << 8) | (b << 16) | 0xFF000000;
				}
			}
			for(int y=0; y<pad_y_bottom; y++)
			{
				unsigned int* row = &buffer[(pen_text_Y+y+LINE_HEIGHT_TEXT-pad_y_bottom) * 512];
				for(size_t x=0; x<width; x++)
				{
					row[x+pen_text_X] = 0xFFFFFFFF;
				}
			}
			pen_text_X += slot_text->advance.x >> 6;
		}

		const unsigned short* p_anno = curNode->p_node->p_anno;
		for( size_t code_idx=0; p_anno && code_idx < curNode->p_node->anno_len; code_idx ++)
		{
			FT_UInt glyph_index = FT_Get_Char_Index(ft_face_anno, curNode->p_node->p_anno[code_idx]);
			FT_Load_Glyph(ft_face_anno, glyph_index, FT_LOAD_NO_BITMAP);
			FT_Render_Glyph(ft_face_anno->glyph, FT_RENDER_MODE_LCD);

			size_t width = slot_anno->bitmap.width / 3;
			int pad_y_top = LINE_HEIGHT_ANNO - slot_anno->bitmap_top;
			int pad_y_bottom = LINE_HEIGHT_ANNO - pad_y_top - slot_anno->bitmap.rows;
			for(int y=0; y<pad_y_top; y++)
			{
				unsigned int* row = &buffer[(pen_anno_Y+y) * 512];
				for(size_t x=0; x<width; x++)
				{
					row[x+pen_anno_X] = 0xFFFFFFFF;
				}
			}
			for(size_t y=0; y<slot_anno->bitmap.rows; y++)
			{
				unsigned int* row = &buffer[(pen_anno_Y+y+pad_y_top) * 512];
				for(size_t x=0; x<width; x++)
				{
					unsigned char r = 255 - *(slot_anno->bitmap.buffer + x*3 + y*slot_anno->bitmap.pitch);
					unsigned char g = 255 - *(slot_anno->bitmap.buffer + x*3+1 + y*slot_anno->bitmap.pitch);
					unsigned char b = 255 - *(slot_anno->bitmap.buffer + x*3+2 + y*slot_anno->bitmap.pitch);
					row[x+pen_anno_X] = (r) | (g << 8) | (b << 16) | 0xFF000000;
				}
			}
			for(int y=0; y<pad_y_bottom; y++)
			{
				unsigned int* row = &buffer[(pen_anno_Y+y+LINE_HEIGHT_ANNO-pad_y_bottom) * 512];
				for(size_t x=0; x<width; x++)
				{
					row[x+pen_anno_X] = 0xFFFFFFFF;
				}
			}
			pen_anno_X += slot_anno->advance.x >> 6;
		}
		curNode->_width = pen_text_X - curNode->_X;
		curNode->_height = FONT_SIZE_TEXT;
	}

}

void draw_tran_to_buffer(pDispInfo p_disp_info, unsigned short* p_txt16, unsigned int* buffer)
{
	FT_Face ft_face_tran = p_disp_info->ft_face_tran;
	FT_GlyphSlot slot_tran = ft_face_tran->glyph;
	size_t pen_tran_X = 20;
	size_t pen_tran_Y = 230;
	for( size_t code_idx=0; p_txt16[code_idx]; code_idx ++)
	{
		if( pen_tran_X + 32 > 480 )
		{
			pen_tran_X = 16;
			pen_tran_Y += LINE_HEIGHT_TRAN;
		}

		FT_UInt glyph_index = FT_Get_Char_Index(ft_face_tran, p_txt16[code_idx]);
		FT_Load_Glyph(ft_face_tran, glyph_index, FT_LOAD_NO_BITMAP);
		FT_Render_Glyph(ft_face_tran->glyph, FT_RENDER_MODE_LCD);

		size_t width = slot_tran->bitmap.width / 3;
		int pad_y_top = LINE_HEIGHT_TRAN - slot_tran->bitmap_top;
		int pad_y_bottom = LINE_HEIGHT_TRAN - pad_y_top - slot_tran->bitmap.rows;
		for(int y=0; y<pad_y_top; y++)
		{
			unsigned int* row = &buffer[(pen_tran_Y+y) * 512];
			for(size_t x=0; x<width; x++)
			{
				row[x+pen_tran_X] = 0xFFFFFFFF;
			}
		}
		for(size_t y=0; y<slot_tran->bitmap.rows; y++)
		{
			unsigned int* row = &buffer[(pen_tran_Y+y+pad_y_top) * 512];
			for(size_t x=0; x<width; x++)
			{
				unsigned char r = 255 - *(slot_tran->bitmap.buffer + x*3 + y*slot_tran->bitmap.pitch);
				unsigned char g = 255 - *(slot_tran->bitmap.buffer + x*3+1 + y*slot_tran->bitmap.pitch);
				unsigned char b = 255 - *(slot_tran->bitmap.buffer + x*3+2 + y*slot_tran->bitmap.pitch);
				row[x+pen_tran_X] = (r) | (g << 8) | (b << 16) | 0xFF000000;
			}
		}
		for(int y=0; y<pad_y_bottom; y++)
		{
			unsigned int* row = &buffer[(pen_tran_Y+y+LINE_HEIGHT_TRAN-pad_y_bottom) * 512];
			for(size_t x=0; x<width; x++)
			{
				row[x+pen_tran_X] = 0xFFFFFFFF;
			}
		}
		pen_tran_X += slot_tran->advance.x >> 6;
	}
}
