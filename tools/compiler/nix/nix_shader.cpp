/*----------------------------------------------------------------------------*\
| Nix shader                                                                   |
| -> shader files                                                              |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#include "nix.h"
#include "nix_common.h"

std::vector<sShaderFile> NixShaderFile;

#ifdef NIX_API_OPENGL
	int NixglShaderCurrent;
#endif


static char ErrStr[4096];
static char shader_buf[2048];

int NixLoadShaderFile(const char *filename)
{
	if (strlen(filename)<1)
		return -1;
	msg_write(string("loading shader file ",SysFileName(filename)));
	msg_right();
	sShaderFile sf;
/*#ifndef NIX_IDE_VCS
	msg_error("ignoring shader file....(no visual studio!)");
	msg_left();
	return -1;
#endif*/
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		int index=-1;
		for (int i=0;i<NumShaderFiles;i++)
			if (!DXEffect[i])
				index=i;
		if (index<0){
			index=NumShaderFiles;
			NumShaderFiles++;
		}
		LPD3DXBUFFER pBufferErrors=NULL;
		HRESULT hr=D3DXCreateEffectFromFile(	lpDevice,
												sys_str_f(filename),
												NULL,
												NULL,
												0,//D3DXSHADER_DEBUG,
												NULL,
												&DXEffect[index],
												&pBufferErrors);
		if (hr!=D3D_OK){
			msg_error(DXErrorMsg(hr));
			if (pBufferErrors){
				msg_write((char*)pBufferErrors->GetBufferPointer(),pBufferErrors->GetBufferSize());
				for (unsigned int i=0;i<pBufferErrors->GetBufferSize();i++)
					ErrStr[i]=((char*)pBufferErrors->GetBufferPointer())[i];
				ErrStr[pBufferErrors->GetBufferSize()]=0;
				HuiErrorBox(NixWindow,"error in shader file",ErrStr);
			}
			msg_left();
			return -1;
		}
		DXEffect[index]->FindNextValidTechnique(NULL,&DXEffectTech[index]);
		if (!DXEffectTech[index]){
			msg_error("none of the techniques found is supported by the hardware!");
			msg_left();
			return -1;
		}
		//D3DXEFFECT_DESC m_EffectDesc[NumShaderFiles];

		msg_ok();
		msg_left();
		return index;
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
#ifdef NIX_OS_LINUX
	//	msg_todo("NixLoadShaderFile for OpenGL");
		//msg_write("-shader");
		int s = glCreateShader(GL_FRAGMENT_SHADER);
		if (s <= 0){
			msg_error("could not create gl shader object");
			msg_left();
			return -1;
		}

		CFile *f = FileOpen(string(filename, ".glsl"));
		if (!f){
			msg_left();
			return -1;
		}
		const char *pbuf[2];
		int size;
		f->ReadComplete(shader_buf,size);
		FileClose(f);

		pbuf[0]=shader_buf;
		glShaderSource( s, 1, pbuf,NULL );


		//msg_write("-compile");
		glCompileShader( s );

		int status;
		glGetShaderiv(s,GL_COMPILE_STATUS,&status);
		//msg_write(status);
		if (status!=GL_TRUE){
			msg_error("while compiling shader...");
			glGetShaderInfoLog(s,sizeof(ErrStr),&size,ErrStr);
			msg_write(ErrStr);
			HuiErrorBox(NixWindow,"error in shader file",ErrStr);
			msg_left();
			return -1;
		}

		//msg_write("-program");
		int p=glCreateProgram();
		if (p<=0){
			msg_error("could not create gl shader program");
			msg_left();
			return -1;
		}

		//msg_write("-attach");
		glAttachShader(p,s);

		//msg_write("-link");
		glLinkProgram(p);
		glGetProgramiv(p,GL_LINK_STATUS,&status);
		//msg_write(status);
		if (status!=GL_TRUE){
			msg_error("while linking the shader program...");
			glGetProgramInfoLog(s,sizeof(ErrStr),&size,ErrStr);
			msg_write(ErrStr);
			HuiErrorBox(NixWindow,"error linking shader file",ErrStr);
			msg_left();
			return -1;
		}
		sf.glShader = p;
		msg_write("ok?");

		NixShaderFile.push_back(sf);
		msg_left();
		return NixShaderFile.size() - 1;
#endif
	}
#endif
	return -1;
}

void NixDeleteShaderFile(int index)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if ((index<0)||(index>=NumShaderFiles))
			return;
		if (!DXEffect[index])
			return;

		DXEffect[index]->Release();
		DXEffect[index]=NULL;
		msg_write(string("deleted shader file ",i2s(index)));
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		msg_todo("NixDeleteShaderFile for OpenGL");
	}
#endif
}

void NixSetShaderFile(int index)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if (DXEffectCurrent){
			DXEffectCurrent->End();
			DXEffectCurrent=NULL;
		}
		if ((index<0)||(index>=NumShaderFiles))
			return;

		DXEffectCurrent=NULL;
		DXEffectCurrentNumPasses;
		if (SUCCEEDED(DXEffect[index]->SetTechnique(DXEffectTech[index]))){
			DXEffectCurrent=DXEffect[index];
			DXEffect[index]->Begin(&DXEffectCurrentNumPasses,0);
		}else{
			msg_error("could not set the shader technique");
		}
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
#ifdef NIX_OS_LINUX
		if (index >= 0)
			NixglShaderCurrent = NixShaderFile[index].glShader;
		else
			NixglShaderCurrent = 0;
		glUseProgram(NixglShaderCurrent);
#endif
	}
#endif
}

void NixSetShaderData(int index,const char *var_name,const void *data,int size)
{
	if (index<0)
		return;
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		D3DXHANDLE hdl;
		if (hdl=DXEffect[index]->GetParameterByName(NULL,"WorldViewProjectionMatrix"))
			DXEffect[index]->SetValue(hdl,data,size);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		msg_todo("NixSetShaderData for OpenGL");
		/*int loc = glGetUniformLocationARB(my_program, “my_color_texture”);

glActiveTexture(GL_TEXTURE0 + i);
glBindTexture(GL_TEXTURE_2D, my_texture_object);

glUniform1iARB(my_sampler_uniform_location, i);*/

	}
#endif
}

void NixGetShaderData(int index,const char *var_name,void *data,int size)
{
	if (index<0)
		return;
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		D3DXHANDLE hdl;
		if (hdl=DXEffect[index]->GetParameterByName(NULL,"WorldViewProjectionMatrix"))
			DXEffect[index]->GetValue(hdl,data,size);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		msg_todo("NixGetShaderData for OpenGL");
	}
#endif
}
