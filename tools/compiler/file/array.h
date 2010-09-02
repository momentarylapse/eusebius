#if !defined(ARRAY_H__INCLUDED_)
#define ARRAY_H__INCLUDED_


//--------------------------------------------------------------
// michi-array

// old...
#define ma_size(var)						(*(int*)((char*)var - sizeof(int)))
#define ma_new(var, type, num_elements)		var = (type*)(new char[num_elements * sizeof(type) + sizeof(int)] + sizeof(int)); ma_size(var) = num_elements
#define ma_delete(var)						delete[]((char*)var - sizeof(int))



//#define foreach(array, pointer, loop_var)	typeof(array[0]) *pointer=&array[0]; for (int loop_var=0;loop_var<array.size();pointer=&array[++loop_var])
#define foreach(array, pointer, loop_var)	if (array.size()>0)pointer=&array[0]; for (int loop_var=0;loop_var<array.size();++loop_var,pointer=(loop_var<array.size())?&array[loop_var]:NULL)


class CMichiArray
{
	public:
	void init(int _item_size_);
	void reserve(int size);
	void resize(int size);
	void ensure_size(int size);
	void append(CMichiArray *a);
	void append_8_single(int x, int y);
	void append_4_single(int x);
	void append_1_single(char x);
	void append_single(void *d);
	void erase_single(int index);
	void erase_single_by_pointer(void *p);
	bool iterate(void *&p);
	bool iterate_back(void *&p);
	int index(void *p);
	void clear();
	void *data;
	int num, allocated, item_size;
};

#define EASY_ARRAY_DEFINITION(type, name_array, name_pointer, name_num) \
	union{							\
		CMichiArray name_array;		\
		struct{						\
			type *name_pointer;		\
			int name_num;			\
		};							\
	}								\

#endif
