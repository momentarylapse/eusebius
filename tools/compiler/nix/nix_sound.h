/*----------------------------------------------------------------------------*\
| Nix sound                                                                    |
| -> sound emitting and music playback                                         |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2007.11.19 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#ifndef _NIX_SOUND_EXISTS_
#define _NIX_SOUND_EXISTS_


void NixSoundInit();

// sound
int NixSoundLoad(const char *filename);
void NixSoundDelete(int index);
void NixSoundPlay(int index,bool repeat);
void NixSoundStop(int index);
void NixSoundSetPause(int index,bool pause);
bool NixSoundIsPlaying(int index);
bool NixSoundEnded(int index);
void NixSoundTestRepeat();
void NixSoundSetData(int index,const vector &pos,const vector &vel,float min_dist,float max_dist,float speed,float volume,bool set_now=false);
void NixSoundSetListener(const vector &pos,const vector &ang,const vector &vel,float meters_per_unit);

// music
int _cdecl NixMusicLoad(const char *filename);
void _cdecl NixMusicPlay(int index,bool repeat);
void _cdecl NixMusicSetRate(int index,float rate);
void _cdecl NixMusicStop(int index);
void _cdecl NixMusicSetPause(int index,bool pause);
bool _cdecl NixMusicIsPlaying(int index);
bool _cdecl NixMusicEnded(int index);
void _cdecl NixMusicTestRepeat();

#endif
