#include "file.h"


void CMichiArray::init(int _item_size_)
{
	num = 0;
	item_size = _item_size_;
	allocated = 0;
}

void CMichiArray::reserve(int size)
{
	if (allocated == 0){
		allocated = size * item_size;
		data = malloc(allocated);
	}else if (size * item_size >= allocated){
		allocated = size * item_size * 2;
		data = realloc(data, allocated);
	}
}

void CMichiArray::resize(int size)
{
	reserve(size);
	if (size > num){
		memset((char*)data + num * item_size, 0, (size - num) * item_size);
		/*for (int i=num;i<size;i++)
			init_sub_super_array(NULL, NULL, */
	}
	num = size;
}

void CMichiArray::ensure_size(int size)
{
	if (size > num)
		resize(size);
}

void CMichiArray::append_8_single(int x, int y)
{
	reserve(num + 1);
	((int*)data)[num * 2] = x;
	((int*)data)[num * 2 + 1] = y;
	num ++;
}

void CMichiArray::append_4_single(int x)
{
	reserve(num + 1);
	((int*)data)[num ++] = x;
}

void CMichiArray::append_1_single(char x)
{
	reserve(num + 1);
	((char*)data)[num ++] = x;
}

void CMichiArray::append_single(void *d)
{
	reserve(num + 1);
	memcpy(&((char*)data)[num * item_size], d, item_size);
	num ++;
}



void CMichiArray::clear()
{
	if (allocated > 0){
		free(data);
		allocated = 0;
		num = 0;
	}
}

void CMichiArray::erase_single(int index)
{
	int n = (num - 1) * item_size;
	for (int i=index*item_size;i<n;i++)
	    ((char*)data)[i] = ((char*)data)[i + item_size];
	num --;
}

void CMichiArray::erase_single_by_pointer(void *p)
{	erase_single(index(p));	}

int CMichiArray::index(void *p)
{	return ((long)p - (long)data) / item_size;	}

bool CMichiArray::iterate(void *&p)
{
	if (p == NULL)
		p = data;
	else
		*(long*)&p += item_size;

	// still within list?
	if ((long)p < (long)data + item_size * num)
		return true;

	// too far -> start at the beginning...
	p = data;
	return false;
}

bool CMichiArray::iterate_back(void *&p)
{
	if (p == NULL)
		p = (char*)data + (num - 1) * item_size;
	else
		*(long*)&p -= item_size;

	// still within list?
	if ((long)p >= (long)data)
		return true;

	// too far -> start at the ending...
	p = (char*)data + (num - 1) * item_size;
	return false;
}
