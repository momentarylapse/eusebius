/*----------------------------------------------------------------------------*\
| Nix net                                                                      |
| -> network functions                                                         |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2008.12.13 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#ifndef _NIX_NET_EXISTS_
#define _NIX_NET_EXISTS_


//--------------------------------------------------------------------//
//                  all about networking                              //
//--------------------------------------------------------------------//


// general
void _cdecl NixNetInit();
void _cdecl NixNetClose(int &s);
void _cdecl NixNetSetBlocking(int s,bool blocking);
int _cdecl NixNetCreate(int port,bool blocking);
int _cdecl NixNetAccept(int sh);
int _cdecl NixNetConnect(char *addr,int port);
bool _cdecl NixNetConnectionLost(int s);

// send / recieve directly
int _cdecl NixNetRead(int s,char *buf,int max_size);
int _cdecl NixNetWrite(int s,char *buf,int size);
bool _cdecl NixNetReadyToWrite(int s);
bool _cdecl NixNetReadyToRead(int s);

// buffered data
void _cdecl NixNetResetBuffer();
bool _cdecl NixNetReadBuffer(int s);
bool _cdecl NixNetWriteBuffer(int s);
// read from buffer
int _cdecl NixNetReadInt();
bool _cdecl NixNetReadBool();
float _cdecl NixNetReadFloat();
vector _cdecl NixNetReadVector();
char *_cdecl NixNetReadStr();
void _cdecl NixNetReadStrL(char *str,int &length);
char _cdecl NixNetReadChar();
int _cdecl NixNetReadBlockStart();
void _cdecl NixNetReadBlockEnd();
// write to buffer
void _cdecl NixNetWriteInt(int i);
void _cdecl NixNetWriteBool(bool b);
void _cdecl NixNetWriteFloat(float f);
void _cdecl NixNetWriteVector(vector v);
void _cdecl NixNetWriteChar(char c);
void _cdecl NixNetWriteStr(char *str);
void _cdecl NixNetWriteStrL(char *str,int length);
void _cdecl NixNetWriteBlockStart(int id);
void _cdecl NixNetWriteBlockEnd();

// ...
void _cdecl NixNetSendBugReport(char *sender,char *program,char *version,char *comment);

extern float NixNetConnectTimeout;

#endif
