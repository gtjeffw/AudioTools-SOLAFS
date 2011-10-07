#ifndef _CIRC_FBUF_HPP_
#define _CIRC_FBUF_HPP_

#ifndef UINT32
#define UINT32 unsigned int
#endif

typedef struct tag_CIRCULAR_FLOAT_BUFFER_STRUCT
{
	float * buf;
	UINT32 size;
	UINT32 mask;
	UINT32 virtSize; //virtual size based on total data written
	//we can remove this if we specify buffer is padded with zeros upon init
	//UINT32 oldestVirtIndex; //the oldest virtual index in the buffer

} CIRCULAR_FLOAT_BUFFER_STRUCT, * CIRCULAR_FLOAT_BUFFER;


#endif //#ifndef _CIRC_FBUF_HPP_





CIRCULAR_FLOAT_BUFFER CFB_create(UINT32 minimumSize);
bool CFB_delete(CIRCULAR_FLOAT_BUFFER cfb);
UINT32 CFG_virtualPosStart(CIRCULAR_FLOAT_BUFFER b);

bool CFB_write(CIRCULAR_FLOAT_BUFFER fout, CIRCULAR_FLOAT_BUFFER fin, 
			   UINT32 size);
bool CFB_write(CIRCULAR_FLOAT_BUFFER fout, CIRCULAR_FLOAT_BUFFER fin, 
			   UINT32 virtPosStart, UINT32 size);
bool CFB_write(CIRCULAR_FLOAT_BUFFER cfb, float * fbuf, UINT32 size);

bool CFB_debugPrint(CIRCULAR_FLOAT_BUFFER cfb);