#if !defined(FILE_OP_H__INCLUDED_)
#define FILE_OP_H__INCLUDED_



//--------------------------------------------------------------
// file/directory operations

bool dir_create(const char *dir);
bool dir_delete(const char *dir);
char *get_current_dir();
bool file_rename(const char *source,const char *target);
bool file_copy(const char *source,const char *target);
bool file_delete(const char *filename);
bool file_test_existence(const char *filename);


//--------------------------------------------------------------
// searching directories

extern CMichiArray DirSearchName, DirSearchNameP;
#define dir_search_name			((char**)DirSearchNameP.data)
extern CMichiArray DirSearchIsDir;
#define dir_search_is_dir		((bool*)DirSearchIsDir.data)

int _cdecl dir_search(const char *dir,const char *filter,bool show_directories);

#endif
