/*----------------------------------------------------------------------------*\
| Nix light                                                                    |
| -> handling light sources                                                    |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#include "nix.h"
#include "nix_common.h"

std::vector<sLight> NixLight;



// nur "3D" Polygone betreffend
void NixEnableLighting(bool Enabled)
{
	NixLightingEnabled=Enabled;
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9)
		lpDevice->SetRenderState(D3DRS_LIGHTING,Enabled);
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		if (Enabled)
			glEnable(GL_LIGHTING);
		else
			glDisable(GL_LIGHTING);
	}
#endif
}

// nur "2D" Polygone betreffend
void NixEnableLighting2D(bool Enabled)
{
	NixLightingEnabled2D=Enabled;
}

int NixCreateLight()
{
	/*if (NumLights>=32)
		return -1;*/
/*#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		switch (NumLights){
			case 0:	OGLLightNo[NumLights]=GL_LIGHT0;	break;
			case 1:	OGLLightNo[NumLights]=GL_LIGHT1;	break;
			case 2:	OGLLightNo[NumLights]=GL_LIGHT2;	break;
			case 3:	OGLLightNo[NumLights]=GL_LIGHT3;	break;
			case 4:	OGLLightNo[NumLights]=GL_LIGHT4;	break;
			case 5:	OGLLightNo[NumLights]=GL_LIGHT5;	break;
			case 6:	OGLLightNo[NumLights]=GL_LIGHT6;	break;
			case 7:	OGLLightNo[NumLights]=GL_LIGHT7;	break;
			default:OGLLightNo[NumLights]=-1;
		}
	}
#endif
	NumLights++;
	return NumLights-1;*/
	for (int i=0;i<NixLight.size();i++)
		if (!NixLight[i].Used){
			NixLight[i].Used = true;
			NixLight[i].Enabled = false;
			return i;
		}
	sLight l;
	l.Used = true;
	l.Enabled = false;
	NixLight.push_back(l);
	return NixLight.size() - 1;
}

void NixDeleteLight(int index)
{
	if ((index < 0) || (index > NixLight.size()))	return;
	NixEnableLight(index, false);
	NixLight[index].Used = false;
}

// Punkt-Quelle
void NixSetLightRadial(int index,const vector &pos,float radius,const color &ambient,const color &diffuse,const color &specular)
{
	if ((index < 0) || (index > NixLight.size()))	return;
	NixLight[index].Pos = pos;
	NixLight[index].Type = LightTypeRadial;
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		D3DLIGHT9 light;
		ZeroMemory(&light,sizeof(D3DLIGHT9));
		light.Type=D3DLIGHT_POINT;
		light.Position=*(D3DXVECTOR3*)&pos;
		light.Range=radius*4;
		light.Attenuation0=0.9f;
		light.Attenuation1=2/radius;
		light.Attenuation2=1/(radius*radius);
		light.Ambient	=color2D3DCOLORVALUE(ambient);
		light.Diffuse	=color2D3DCOLORVALUE(diffuse);
		light.Specular	=color2D3DCOLORVALUE(specular);
		lpDevice->SetLight(index,&light);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		//if (OGLLightNo[index]<0)	return;
		glPushMatrix();
		//glLoadIdentity();
		glLoadMatrixf((float*)&NixViewMatrix);
		float f[4];
		f[0]=pos.x;	f[1]=pos.y;	f[2]=pos.z;	f[3]=1;
		glLightfv(GL_LIGHT0+index,GL_POSITION,f); // GL_POSITION,(x,y,z,0)=directional,(x,y,z,1)=positional !!!
		glLightfv(GL_LIGHT0+index,GL_AMBIENT,(float*)&ambient);
		glLightfv(GL_LIGHT0+index,GL_DIFFUSE,(float*)&diffuse);
		glLightfv(GL_LIGHT0+index,GL_SPECULAR,(float*)&specular);
		glLightf(GL_LIGHT0+index,GL_CONSTANT_ATTENUATION,0.9f);
		glLightf(GL_LIGHT0+index,GL_LINEAR_ATTENUATION,2.0f/radius);
		glLightf(GL_LIGHT0+index,GL_QUADRATIC_ATTENUATION,1/(radius*radius));
		glPopMatrix();
	}
#endif
}

// parallele Quelle
// dir =Richtung, in die das Licht scheinen soll
void NixSetLightDirectional(int index,const vector &dir,const color &ambient,const color &diffuse,const color &specular)
{
	if ((index < 0) || (index > NixLight.size()))	return;
	NixLight[index].Dir = dir;
	NixLight[index].Type = LightTypeDirectional;
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		D3DLIGHT9 light;
		ZeroMemory(&light,sizeof(D3DLIGHT9));
		light.Type=D3DLIGHT_DIRECTIONAL;
		light.Direction=-*(D3DXVECTOR3*)&dir;
		light.Ambient	=color2D3DCOLORVALUE(ambient);
		light.Diffuse	=color2D3DCOLORVALUE(diffuse);
		light.Specular	=color2D3DCOLORVALUE(specular);
		lpDevice->SetLight(index,&light);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		//if (OGLLightNo[index]<0)	return;
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		//glLoadIdentity();
		glLoadMatrixf((float*)&NixViewMatrix);
		float f[4];
		f[0]=dir.x;	f[1]=dir.y;	f[2]=dir.z;	f[3]=0;
		glLightfv(GL_LIGHT0+index,GL_POSITION,f); // GL_POSITION,(x,y,z,0)=directional,(x,y,z,1)=positional !!!
		glLightfv(GL_LIGHT0+index,GL_AMBIENT,(float*)&ambient);
		glLightfv(GL_LIGHT0+index,GL_DIFFUSE,(float*)&diffuse);
		glLightfv(GL_LIGHT0+index,GL_SPECULAR,(float*)&specular);
		glPopMatrix();
	}
#endif
}

void NixEnableLight(int index,bool enabled)
{
	if ((index < 0) || (index > NixLight.size()))	return;
	NixLight[index].Enabled = enabled;
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9)
		lpDevice->LightEnable(index, enabled);
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
//		if (OGLLightNo[index]<0)	return;
		if (enabled)
			glEnable(GL_LIGHT0 + index);
		else
			glDisable(GL_LIGHT0 + index);
	}
#endif
}

void NixSetAmbientLight(const color &c)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		int i=color2D3DCOLOR(c);
		lpDevice->SetRenderState(D3DRS_AMBIENT,i);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL)
		glLightModelfv(GL_LIGHT_MODEL_AMBIENT,(float*)&c);
#endif
}


void NixSetShading(int mode)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if (mode==ShadingPlane)
			lpDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_FLAT );
		if (mode==ShadingRound)
			lpDevice->SetRenderState( D3DRS_SHADEMODE, D3DSHADE_GOURAUD );
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		if (mode==ShadingPlane)
			glShadeModel(GL_FLAT);
		if (mode==ShadingRound)
			glShadeModel(GL_SMOOTH);
	}
#endif
}

void NixSetMaterial(const color &ambient,const color &diffuse,const color &specular,float shininess,const color &emission)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		D3DMATERIAL9 Material;
		ZeroMemory(&Material,sizeof(D3DMATERIAL9));
		Material.Ambient	=color2D3DCOLORVALUE(ambient);
		Material.Diffuse	=color2D3DCOLORVALUE(diffuse);
		Material.Specular	=color2D3DCOLORVALUE(specular);
		Material.Power		=shininess;
		Material.Emissive	=color2D3DCOLORVALUE(emission);
		lpDevice->SetMaterial(&Material);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,(float*)&ambient);
		glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,(float*)&diffuse);
		glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,(float*)&specular);
		glMaterialfv(GL_FRONT_AND_BACK,GL_SHININESS,(float*)&shininess);
		glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,(float*)&emission);
	}
#endif
}

void NixSpecularEnable(bool enabled)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		lpDevice->SetRenderState(D3DRS_SPECULARENABLE,enabled);
	}
#endif
}
