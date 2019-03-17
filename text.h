#ifndef __TEXT_H__
#define __TEXT_H__

struct __Node;
typedef struct __Node Node,*pNode;

struct __Line;
typedef struct __Line Line,*pLine;

struct __Text;
typedef struct __Text Text,*pText;

struct __Node {
	pNode p_next;
	pNode p_prev;

	unsigned short text_len;
	unsigned short anno_len;
	unsigned short norm_len;
	unsigned short* p_text;
	unsigned short* p_anno;
	unsigned short* p_norm;
};

struct __Line {
	pLine p_next;
	pLine p_prev;

	pNode p_first_node;
	pNode p_last_node;
};

struct __Text {
	pLine p_first_line;
	pLine p_last_line;
};

void build_text(pText p_text, const char* filename);
void build_line(pLine p_line, unsigned short* data);

#endif	//__TEXT_H__
