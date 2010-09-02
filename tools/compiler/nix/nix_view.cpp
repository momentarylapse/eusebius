/*----------------------------------------------------------------------------*\
| Nix view                                                                     |
| -> camera etc...                                                             |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#include "nix.h"
#include "nix_common.h"


matrix NixViewMatrix,NixProjectionMatrix,NixInvProjectionMatrix;
matrix NixWorldMatrix,NixWorldViewProjectionMatrix;
float View3DWidth,View3DHeight,View3DCenterX,View3DCenterY,NixView3DRatio;	// 3D transformation
float View2DScaleX,View2DScaleY;				// 2D transformation
int PerspectiveModeSize,PerspectiveModeCenter,PerspectiveMode2DScale;
vector NixViewScale=vector(1,1,1);
bool NixEnabled3D;

static int OGLViewPort[4];
// sizes
static int VPx1,VPy1,VPx2,VPy2;

int RenderingToTexture = -1;



void NixResize()
{
	if (!NixUsable)
		return;

	msg_db_r("NixResize",10);

	if (NixTargetWidth<=0)
		NixTargetWidth=1;
	if (NixTargetHeight<=0)
		NixTargetHeight=1;
	NixTargetRect = rect(0, NixTargetWidth, 0, NixTargetHeight);

#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		lpDevice->GetViewport(&DXViewPort);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){

		// Projektion 2D
		matrix s,t;
		// OpenGl hat (0,0) in Fenster-Mitte und berdeckt einen Bereich von -1 bis 1 (x und y)
		MatrixScale(s,2.0f/float(NixTargetWidth),-2.0f/float(NixTargetHeight),1);
		MatrixTranslation(t,vector(-float(NixTargetWidth)/2.0f,-float(NixTargetHeight)/2.0f,0));
		MatrixMultiply(NixOGLProjectionMatrix2D,s,t);

		// Bildschirm
		glViewport(0,0,NixTargetWidth,NixTargetHeight);
		//glViewport(0,0,NixTargetWidth,NixTargetHeight);
		OGLViewPort[0]=0;
		OGLViewPort[1]=0;
		OGLViewPort[2]=NixTargetWidth;
		OGLViewPort[3]=NixTargetHeight;

		/*glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();*/
	}
#endif

	if (PerspectiveModeCenter==PerspectiveCenterAutoTarget){
		View3DCenterX=float(NixTargetWidth)/2.0f;
		View3DCenterY=float(NixTargetHeight)/2.0f;
	}
	if (PerspectiveModeSize==PerspectiveSizeAutoTarget){
		View3DWidth=float(NixTargetWidth);
		View3DHeight=float(NixTargetHeight);
	}
	if (PerspectiveModeSize==PerspectiveSizeAutoScreen){
		View3DWidth=float(NixScreenWidth);
		View3DHeight=float(NixScreenHeight);
	}

	// Kamera
	NixSetView(NixEnabled3D,NixViewMatrix);

	msg_db_l(10);
}

void NixSetWorldMatrix(const matrix &mat)
{
	NixWorldMatrix=mat;
	MatrixMultiply(NixWorldViewProjectionMatrix,NixViewMatrix,NixWorldMatrix);
	MatrixMultiply(NixWorldViewProjectionMatrix,NixProjectionMatrix,NixWorldViewProjectionMatrix);
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		lpDevice->SetTransform(D3DTS_WORLD,(D3DXMATRIX*)&mat);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		glMatrixMode(GL_MODELVIEW);
		glLoadMatrixf((float*)&NixViewMatrix);
		glMultMatrixf((float*)&mat);
	}
#endif
}

void NixSetPerspectiveMode(int mode,float param1,float param2)
{
// width and height of the 3D projection
	if (mode==PerspectiveSizeAutoTarget){
		PerspectiveModeSize=mode;
		View3DWidth=float(NixTargetWidth);
		View3DHeight=float(NixTargetHeight);
	}
	if (mode==PerspectiveSizeAutoScreen){
		PerspectiveModeSize=mode;
		View3DWidth=float(NixScreenWidth);
		View3DHeight=float(NixScreenHeight);
	}
	if (mode==PerspectiveSizeSet){
		PerspectiveModeSize=mode;
		View3DWidth=param1;
		View3DHeight=param2;
	}
// vanishing point
	if (mode==PerspectiveCenterSet){
		PerspectiveModeCenter=mode;
		View3DCenterX=param1;
		View3DCenterY=param2;
	}
	if (mode==PerspectiveCenterAutoTarget){
		PerspectiveModeCenter=mode;
		View3DCenterX=float(NixTargetWidth)/2.0f;
		View3DCenterY=float(NixTargetHeight)/2.0f;
	}
// 2D transformation
	if (mode==Perspective2DScaleSet){
		PerspectiveMode2DScale=mode;
		View2DScaleX=param1;
		View2DScaleY=param2;
	}
// aspect ratio
	if (mode==PerspectiveRatioSet){
		//PerspectiveModeRatio=mode;
		NixView3DRatio=param1;
	}
}

static vector ViewPos,ViewDir;
static vector Frustrum[8];
static plane FrustrumPl[6];

void NixSetView(bool enable3d,const vector &view_pos,const vector &view_ang,const vector &scale)
{
	ViewPos=view_pos;
	ViewDir=VecAng2Dir(view_ang);
	NixViewScale=scale;

	//if (enable3d){
		matrix t,r,s;
		vector m_pos = v0 - view_pos;
		MatrixTranslation(t,m_pos);
		MatrixRotationView(r,view_ang);
		//MatrixScale(s,scale.x,scale.y,scale.z);
		MatrixMultiply(NixViewMatrix,r,t);
		//MatrixMultiply(NixViewMatrix,s,NixViewMatrix);
	//}
	NixSetView(enable3d,NixViewMatrix);

	// die Eckpunkte des Sichtfeldes
	/*NixGetVecUnproject(Frustrum[0],vector(                   0,                    0,0.0f));
	NixGetVecUnproject(Frustrum[1],vector(float(NixScreenWidth-1),                    0,0.0f));
	NixGetVecUnproject(Frustrum[2],vector(                   0,float(NixScreenHeight-1),0.0f));
	NixGetVecUnproject(Frustrum[3],vector(float(NixScreenWidth-1),float(NixScreenHeight-1),0.0f));
	NixGetVecUnproject(Frustrum[4],vector(                   0,                    0,0.9f));
	NixGetVecUnproject(Frustrum[5],vector(float(NixScreenWidth-1),                    0,0.9f));
	NixGetVecUnproject(Frustrum[6],vector(                   0,float(NixScreenHeight-1),0.9f));
	NixGetVecUnproject(Frustrum[7],vector(float(NixScreenWidth-1),float(NixScreenHeight-1),0.9f));

	// Ebenen des Sichtfeldes (gegen UZS nach innen!?)
	PlaneFromPoints(FrustrumPl[0],Frustrum[0],Frustrum[1],Frustrum[2]); // nahe Ebene
	//PlaneFromPoints(FrustrumPl[1],Frustrum[4],Frustrum[6],Frustrum[7]); // ferne Ebene
	//PlaneFromPoints(FrustrumPl[2],Frustrum[0],Frustrum[2],Frustrum[3]); // linke Ebene
	//PlaneFromPoints(FrustrumPl[3],Frustrum[1],Frustrum[5],Frustrum[7]); // rechte Ebene
	//PlaneFromPoints(FrustrumPl[4],Frustrum[0],Frustrum[4],Frustrum[5]); // untere Ebene
	//PlaneFromPoints(FrustrumPl[5],Frustrum[2],Frustrum[3],Frustrum[7]); // obere Ebene*/
}

// 3D-Matrizen erstellen (Einstellungen ueber SetPerspectiveMode vor NixStart() zu treffen)
// enable3d: true  -> 3D-Ansicht auf (View3DWidth,View3DHeight) gemapt
//           false -> Pixel-Angaben~~~
// beide Bilder sind um View3DCenterX,View3DCenterY (3D als Fluchtpunkt) verschoben
void NixSetView(bool enable3d,const matrix &view_mat)
{
	//SetCull(CullCCW); // ???
	NixViewMatrix=view_mat;

#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		lpDevice->SetTransform(D3DTS_VIEW,(D3DXMATRIX*)&NixViewMatrix);

		matrix p,t,s,s2;
		if (enable3d){
			if (RenderingToTexture>=0)
				MatrixTranslation(t,vector(View3DCenterX/float(NixTextureWidth[RenderingToTexture])*2.0f-1,1-View3DCenterY/float(NixTextureHeight[RenderingToTexture])*2.0f,0));
			else
				MatrixTranslation(t,vector(View3DCenterX/float(NixScreenWidth)*2.0f-1,1-View3DCenterY/float(NixScreenHeight)*2.0f,0));
			// perspektivische Verzerrung
			D3DXMatrixPerspectiveFovLH((D3DXMATRIX*)&p,pi/3,View3DRatio,NixMinDepth,NixMaxDepth);
			if (RenderingToTexture>=0)
				MatrixScale(s,View3DWidth/((float)NixTextureWidth[RenderingToTexture]),View3DHeight/((float)NixTextureHeight[RenderingToTexture]),1);
			else
				MatrixScale(s,View3DWidth/((float)NixScreenWidth),View3DHeight/((float)NixScreenHeight),1);
			MatrixScale(s2,ViewScale.x,ViewScale.y,ViewScale.z);
			MatrixMultiply(NixProjectionMatrix,t,p);
			MatrixMultiply(NixProjectionMatrix,NixProjectionMatrix,s);
			MatrixMultiply(NixProjectionMatrix,NixProjectionMatrix,s2); // richtige Reihenfolge??????  ...bei Gelegenheit testen!
		}else{
			//msg_todo("NixSetView(2D) fuer DirectX9 (Sonderlichkeiten bei Target!=Screen ???)");
			//MatrixScale(s,1.0f/(float)NixTargetWidth,1.0f/(float)NixTargetHeight,1.0f/(float)NixMaxDepth);
			MatrixScale(s,2*View2DScaleX/(float)NixScreenWidth,2*View2DScaleY/(float)NixScreenHeight,1.0f/(float)NixMaxDepth);
			MatrixTranslation(t,vector(View3DCenterX/float(NixScreenWidth)*2.0f-1,1-View3DCenterY/float(NixScreenHeight)*2.0f,0.5f+ViewPos.z));
			MatrixMultiply(NixProjectionMatrix,t,s);
			MatrixScale(s,ViewScale.x,ViewScale.y,ViewScale.z);
			MatrixMultiply(NixProjectionMatrix,s,NixProjectionMatrix);
		}
		lpDevice->SetTransform(D3DTS_PROJECTION,(D3DXMATRIX*)&NixProjectionMatrix);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		//msg_write("NixSetView");
		// Projektions-Matrix editieren
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		/*msg_write(NixTargetWidth);
		msg_write(NixTargetHeight);
		msg_write(View3DCenterX);
		msg_write(View3DCenterY);
		msg_write(View3DWidth);
		msg_write(View3DHeight);
		msg_write(View2DScaleX);
		msg_write(View2DScaleY);
		msg_write(NixMaxDepth);*/
		if (enable3d){
			//msg_write("3d");
			glTranslatef(View3DCenterX/float(NixTargetWidth)*2.0f-1,1-View3DCenterY/float(NixTargetHeight)*2.0f,0);
			//glGetFloatv(GL_PROJECTION_MATRIX,(float*)&NixProjectionMatrix);
			//mout(NixProjectionMatrix);
			// perspektivische Verzerrung
			gluPerspective(60.0f,NixView3DRatio,NixMinDepth,NixMaxDepth);
			//glGetFloatv(GL_PROJECTION_MATRIX,(float*)&NixProjectionMatrix);
			//mout(NixProjectionMatrix);
			glScalef((View3DWidth/(float)NixTargetWidth),(View3DHeight/(float)NixTargetHeight),-1); // -1: Koordinatensystem: Links vs Rechts
			glScalef(NixViewScale.x,NixViewScale.y,NixViewScale.z);
			//glGetFloatv(GL_PROJECTION_MATRIX,(float*)&NixProjectionMatrix);
			//mout(NixProjectionMatrix);
		}else{
			glTranslatef(View3DCenterX/float(NixTargetWidth)*2.0f-1,1-View3DCenterY/float(NixTargetHeight)*2.0f,0);
			//glGetFloatv(GL_PROJECTION_MATRIX,(float*)&NixProjectionMatrix);
			//mout(NixProjectionMatrix);
			glScalef(2*View2DScaleX/(float)NixTargetWidth,2*View2DScaleY/(float)NixTargetHeight,1.0f/(float)NixMaxDepth);
			//glGetFloatv(GL_PROJECTION_MATRIX,(float*)&NixProjectionMatrix);
			//mout(NixProjectionMatrix);
		}
		// Matrix speichern
		glGetFloatv(GL_PROJECTION_MATRIX,(float*)&NixProjectionMatrix);

		// OpenGL muss Lichter neu ausrichten, weil sie in Kamera-Koordinaten gespeichert werden!
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		//glLoadIdentity();
		glLoadMatrixf((float*)&NixViewMatrix);
		for (int i=0;i<NixLight.size();i++){
			if (!NixLight[i].Used)	continue;
		//	if (OGLLightNo[i]<0)	continue;
			float f[4];
			/*f[0]=LightVector[i].x;	f[1]=LightVector[i].y;	f[2]=LightVector[i].z;
			if (LightDirectional[i])
				f[3]=0;
			else
				f[3]=1;
			glLightfv(OGLLightNo[i],GL_POSITION,f);*/
			if (NixLight[i].Type==LightTypeDirectional){
				f[0]=NixLight[i].Dir.x;	f[1]=NixLight[i].Dir.y;	f[2]=NixLight[i].Dir.z;	f[3]=0;
			}else if (NixLight[i].Type==LightTypeRadial){
				f[0]=NixLight[i].Pos.x;	f[1]=NixLight[i].Pos.y;	f[2]=NixLight[i].Pos.z;	f[3]=1;
			}
			glLightfv(GL_LIGHT0+i,GL_POSITION,f);
			//msg_write(i);
		}
		glPopMatrix();
	}
#endif
	MatrixInverse(NixInvProjectionMatrix,NixProjectionMatrix);
	NixEnabled3D=enable3d;
}

void NixSetViewV(bool enable3d,const vector &view_pos,const vector &view_ang)
{	NixSetView(enable3d, view_pos, view_ang, NixViewScale);	}

void NixSetViewM(bool enable3d,const matrix &view_mat)
{
	NixViewScale = vector(1, 1, 1);
	NixSetView(enable3d, view_mat);
}



#define FrustrumAngleCos	0.83f

bool NixIsInFrustrum(const vector &pos,float radius)
{
	// die absoluten Eckpunkte der BoundingBox
	vector p[8];
	p[0]=pos+vector(-radius,-radius,-radius);
	p[1]=pos+vector( radius,-radius,-radius);
	p[2]=pos+vector(-radius, radius,-radius);
	p[3]=pos+vector( radius, radius,-radius);
	p[4]=pos+vector(-radius,-radius, radius);
	p[5]=pos+vector( radius,-radius, radius);
	p[6]=pos+vector(-radius, radius, radius);
	p[7]=pos+vector( radius, radius, radius);

	bool in=false;
	for (int i=0;i<8;i++)
		//for (int j=0;j<6;j++)
			if (PlaneDistance(FrustrumPl[0],p[i])<0)
				in=true;
	/*vector d;
	VecNormalize(d,pos-ViewPos); // zu einer Berechnung zusammenfassen!!!!!!
	float fdp=VecLengthFuzzy(pos-ViewPos);
	if (fdp<radius)
		return true;
	if (VecDotProduct(d,ViewDir)>FrustrumAngleCos-radius/fdp*0.04f)
		return true;
	return false;*/
	return in;
}

bool Rendering=false;

bool NixStart(int texture)
{
	if (NixDoingEvilThingsToTheDevice)
		return false;

	NixNumTrias=0;
	RenderingToTexture=texture;
	//msg_write(string("Start ",i2s(texture)));
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){



		// Quell-Code-Chaos


		HRESULT hr;

		if (!lpDevice)
			msg_write("kein Device");
		// Test the cooperative level to see if it's okay to render
		if (FAILED(hr=lpDevice->TestCooperativeLevel())){
			msg_write("TCL evil");
        	// If the device was lost, do not render until we get it back
        	if (D3DERR_DEVICELOST==hr){
				msg_write("DeviceLost");
				return false;
			}

			// Check if the device needs to be resized.
			if (D3DERR_DEVICENOTRESET==hr){
				msg_write("DeviceNotReset");
#if 0
				// If we are windowed, read the desktop mode and use the same format for
				// the back buffer
				if (!NixFullscreen){
					//D3DAdapterInfo* pAdapterInfo = &m_Adapters[m_dwAdapter];
					//m_pD3D->GetAdapterDisplayMode( m_dwAdapter, &pAdapterInfo->d3ddmDesktop );
					d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;//pAdapterInfo->d3ddmDesktop.Format;
				}
#endif
				Usable=false;
				msg_write("......trying to repair...");
#if 0
				hr=lpDevice->TestCooperativeLevel();
				msg_write(DXErrorMsg(hr));

				// Release all vidmem objects
				// KillDeviceObjects();

				// Reset the device
				if (FAILED(hr=lpDevice->Reset(&d3dpp))){
					msg_write(DXErrorMsg(hr));
					return false;
				}else
					msg_write("Hurra!");
#endif
				NixSetVideoMode(NixApi,NixScreenWidth,NixScreenHeight,NixScreenDepth,NixFullscreen);

				// Initialize the app's device-dependent objects
				//ReincarnateDeviceObjects();

				Usable=true;
			}
			return false;
		}


		if (!Usable)
			return false;


		/*if (!lpD3D)
			msg_write("kein Direct3D!");
		if (!lpDevice)
			msg_write("kein Device!");
		if ((NumTextures>0)&&(!Texture[0]))
			msg_write("keine Texturen!");*/


		if (texture<0){
			hr=lpDevice->BeginScene();
			if (FAILED(hr)){
				msg_error(string("Device-BeginScene: ",DXErrorMsg(hr)));
				return false;
			}
			if ((WireFrame)||(!NixFullscreen))
				lpDevice->Clear(0,NULL,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,D3DCOLOR_XRGB(0,0,0),1.0f,0);
			else
				lpDevice->Clear(0,NULL,D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,D3DCOLOR_XRGB(0,0,0),1.0f,0);
		}else{
			NixRenderToTextureBeginDX(texture);
			//lpDevice->Clear(0,NULL,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,D3DCOLOR_COLORVALUE(0.0f,0.0f,0.0f,1.0f),1.0f,0);
			//lpDevice->Clear(0,NULL,D3DCLEAR_ZBUFFER,0,1.0f,0);
			lpDevice->Clear(0,NULL,D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,0,1.0f,0);
		}
	}

#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		if (texture<0){
			#ifdef NIX_OS_WINDOWS
				#ifdef NIX_ALLOW_DYNAMIC_TEXTURE
					if (OGLDynamicTextureSupport)
						glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
				#endif
				if (!wglMakeCurrent(hDC,hRC))
					return false;
			#endif
		}else{
			#ifdef NIX_ALLOW_DYNAMIC_TEXTURE
				if (OGLDynamicTextureSupport){
					
					glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, NixTexture[texture].glFrameBuffer );
					//glBindRenderbufferEXT( GL_RENDERBUFFER_EXT, NixTexture[texture].glDepthRenderBuffer );
					glFramebufferTexture2DEXT( GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, NixTexture[texture].glTexture, 0 );
					glFramebufferRenderbufferEXT( GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_RENDERBUFFER_EXT, NixTexture[texture].glDepthRenderBuffer );
					GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
					if (status == GL_FRAMEBUFFER_COMPLETE_EXT){
						//msg_write("hurra");
					}else{
						msg_write("we're screwed! (NixStart with dynamic texture target)");
						return false;
					}
				}
			#endif
		}
		glClearColor(0.0f,0.0f,0.0f,0.0f);
		glDisable(GL_SCISSOR_TEST);
		//glClearStencil(0);
		//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
		//glClear(GL_COLOR_BUFFER_BIT);
	}
#endif

	// adjust target size
	if (texture < 0){
		if (NixFullscreen){
			// fullscreen mode
			NixTargetWidth = NixScreenWidth;
			NixTargetHeight = NixScreenHeight;
		}else{
			// window mode
			irect r = NixWindow->GetInterior();
			NixTargetWidth = r.x2 - r.x1;
			NixTargetHeight = r.y2 - r.y1;
		}
	}else{
		// texture
		NixTargetWidth = NixTexture[texture].Width;
		NixTargetHeight = NixTexture[texture].Height;
	}
	VPx1 = VPy1 = 0;
	VPx2 = NixTargetWidth;
	VPy2 = NixTargetHeight;
	NixResize();
	Rendering = true;

	//msg_write("-ok?");
	return true;
}

void NixStartPart(int x1,int y1,int x2,int y2,bool set_centric)
{
	bool enable_scissors=true;
	if ((x1<0)||(y1<0)||(x2<0)||(y2<0)){
		x1=0;	y1=0;	x2=NixTargetWidth;	y2=NixTargetHeight;
		enable_scissors=false;
	}
	VPx1=x1;
	VPy1=y1;
	VPx2=x2;
	VPy2=y2;
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		/*d.x1=x1;	d.y1=y1;	d.x2=x2;	d.y2=y2;	//	Ziel
		r[0].x1=0;	r[0].y1=0;	r[0].x2=w;	r[0].y2=y1;	//	Rand (oben gesammt)
		r[1].x1=0;	r[1].y1=y1;	r[1].x2=x1;	r[1].y2=y2;	//	Rand (links mitte)
		r[2].x1=x2;	r[2].y1=y1;	r[2].x2=w;	r[2].y2=y2;	//	Rand (rechts mitte)
		r[3].x1=0;	r[3].y1=y2;	r[3].x2=w;	r[3].y2=h;	//	Rand (unten gesammt)*/
		RECT r;
		r.left=x1;		r.right=x2;
		r.top=y1;		r.bottom=y2;
		lpDevice->SetScissorRect( &r );
		lpDevice->SetRenderState(D3DRS_SCISSORTESTENABLE,(enable_scissors?TRUE:FALSE));

		//lpDevice->Clear(4,r,D3DCLEAR_ZBUFFER,D3DCOLOR_XRGB(0,0,0),-1.0f,0);
		lpDevice->Clear(1,(D3DRECT*)&r,D3DCLEAR_ZBUFFER,D3DCOLOR_XRGB(0,0,0),1.0f,0);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		if (enable_scissors)
			glEnable(GL_SCISSOR_TEST);
		else
			glDisable(GL_SCISSOR_TEST);
		glScissor(x1,NixTargetHeight-y2,x2-x1,y2-y1);
		glClearDepth(1.0f);
	}
#endif
	if (set_centric){
		View3DCenterX=float(x1+x2)/2.0f;
		View3DCenterY=float(y1+y2)/2.0f;
		NixSetView(NixEnabled3D,NixViewMatrix);
	}
}

void NixEnd()
{
	if (!Rendering)
		return;
	Rendering=false;
	//msg_write("End");
	NixSetTexture(-1);
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		if (RenderingToTexture<0){
			lpDevice->EndScene();
			// auf den Bildschirm
			if (NixFullscreen)
				lpDevice->Present(NULL,NULL,NULL,NULL);
			else{
				irect r=NixWindow->GetInterior();
				RECT R;	R.left=0;	R.right=r.x2-r.x1;	R.top=0;	R.bottom=r.y2-r.y1;
				lpDevice->Present(&R,NULL,NULL,NULL);
			}
		}else
			NixRenderToTextureEndDX(RenderingToTexture);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		glDisable(GL_SCISSOR_TEST);
		if (RenderingToTexture<0){
			// auf den Bildschirm
			#ifdef NIX_OS_WINDOWS
				if (RenderingToTexture<0)
					SwapBuffers(hDC);
			#endif
			#ifdef NIX_OS_LINUX
				#ifdef NIX_ALLOW_FULLSCREEN
					if (NixFullscreen)
						XF86VidModeSetViewPort(hui_x_display,screen,0,NixDesktopHeight-NixScreenHeight);
				#endif
				//glutSwapBuffers();
				if (NixGLDoubleBuffered)
					glXSwapBuffers(hui_x_display,GDK_WINDOW_XWINDOW(NixWindow->gl_widget->window));
			#endif
		}
		#ifdef NIX_ALLOW_DYNAMIC_TEXTURE
			if (OGLDynamicTextureSupport)
				glBindFramebufferEXT( GL_FRAMEBUFFER_EXT, 0 );
		#endif
	}
#endif

	NixProgressTextureLifes();
}

void NixSetClipPlane(int index,const plane &pl)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		lpDevice->SetClipPlane(index,(float*)&pl);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		GLdouble d[4];
		d[0]=pl.a;	d[1]=pl.b;	d[2]=pl.c;	d[3]=pl.d;
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadMatrixf((float*)&NixViewMatrix);
		glClipPlane(GL_CLIP_PLANE0+index,d);
		glPopMatrix();
		//msg_todo("SetClipPlane fuer OpenGL");
	}
#endif
}

void NixEnableClipPlane(int index,bool enabled)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		DWORD mask=(1<<index);
		if (enabled)
			ClipPlaneMask=ClipPlaneMask|mask;
		else
			ClipPlaneMask-=ClipPlaneMask&mask;
		lpDevice->SetRenderState(D3DRS_CLIPPLANEENABLE,ClipPlaneMask);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		if (enabled)
			glEnable(GL_CLIP_PLANE0+index);
		else
			glDisable(GL_CLIP_PLANE0+index);
	}
#endif
}

void NixDoScreenShot(const char *filename,const rect *source)
{
	irect rect;
	if (source){
		rect.x1=(int)source->x1;
		rect.y1=(int)source->y1;
		rect.x2=(int)source->x2;
		rect.y2=(int)source->y2;
	}else{
		rect.x1=0;
		rect.y1=0;
		rect.x2=NixTargetWidth;
		rect.y2=NixTargetHeight;
	}
	/*msg_write(rect.x1);
	msg_write(rect.y1);
	msg_write(rect.x2);
	msg_write(rect.y2);*/
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		D3DXSaveSurfaceToFile(sys_str_f(filename),D3DXIFF_BMP,FrontBuffer,NULL,(RECT*)&rect);
		//D3DXSaveSurfaceToFile(sys_str_f(filename),D3DXIFF_BMP,FrontBuffer,NULL,NULL);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		int x,y;
		int dx=rect.x2-rect.x1;
		int dy=rect.y2-rect.y1;
		int *data=new int[dx*dy];
		glReadBuffer(GL_FRONT);
		glReadPixels(	rect.x1,
						rect.y1,
						dx,
						dy,
						GL_RGBA,GL_UNSIGNED_BYTE,data);
		// flip image...
		for (x=0;x<dx;x++)
			for (y=0;y<(dy+1)/2;y++){
				int y2=dy-y-1;
				int n1=(x+dx*y );
				int n2=(x+dx*y2);
				int c=data[n1];
				data[n1]=data[n2];
				data[n2]=c;
			}
		NixSaveTGA((char*)filename,dx,dy,(NixScreenDepth==16)?16:24,0,data);
		delete[](data);
	}
#endif
	msg_write(string("screenshot saved: ",SysFileName(filename)));
}



// world -> screen (0...NixTargetWidth,0...NixTargetHeight,0...1)
void NixGetVecProject(vector &vout,const vector &vin)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		D3DXVec3Project((D3DXVECTOR3*)&vout,(D3DXVECTOR3*)&vin,&DXViewPort,(D3DXMATRIX*)&NixProjectionMatrix,(D3DXMATRIX*)&NixViewMatrix,NULL);
		/*VecTransform(vout,NixViewMatrix,vin);
		VecTransform(vout,NixProjectionMatrix,vout);*/
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		/*matrix m;
		MatrixIdentity(m);*/
		double vm[16];
		int i;
		for (i=0;i<16;i++)
			vm[i]=NixViewMatrix.e[i];
			//vm[i]=m.e[i];
		double pm[16];
		for (i=0;i<16;i++)
				pm[i]=NixProjectionMatrix.e[i];
			//pm[i]=m.e[i];
		double x,y,z;
		gluProject(vin.x,vin.y,vin.z,vm,pm,OGLViewPort,&x,&y,&z);
		vout.x=(float)x;
		vout.y=float((OGLViewPort[1]*2+OGLViewPort[3])-y); // y-Spiegelung
		vout.z=(float)z;//0.999999970197677613f;//(float)z;
		/*VecTransform(vout,NixViewMatrix,vin);
		VecTransform(vout,NixProjectionMatrix,vout);
		vout.y=((ViewPort[1]*2+ViewPort[3])-vout.y*16)/2;
		vout.x=((ViewPort[0]*2+ViewPort[2])+vout.x*16)/2;
		vout.z=0.99999997f;*/
	}
#endif
}

// world -> screen (0...1,0...1,0...1)
void NixGetVecProjectRel(vector &vout,const vector &vin)
{
	NixGetVecProject(vout,vin);
	vout.x/=(float)NixTargetWidth;
	vout.y/=(float)NixTargetHeight;
}

// screen (0...NixTargetWidth,0...NixTargetHeight,0...1) -> world
void NixGetVecUnproject(vector &vout,const vector &vin)
{
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		D3DXVec3Unproject((D3DXVECTOR3*)&vout,(D3DXVECTOR3*)&vin,&DXViewPort,(D3DXMATRIX*)&NixProjectionMatrix,(D3DXMATRIX*)&NixViewMatrix,NULL);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		double vin_y=OGLViewPort[1]*2+OGLViewPort[3]-(double)vin.y; // y-Spiegelung
		double vm[16];
		int i;
		for (i=0;i<16;i++)
			vm[i]=NixViewMatrix.e[i];
		double pm[16];
		for (i=0;i<16;i++)
			pm[i]=NixProjectionMatrix.e[i];
		double x,y,z;
		gluUnProject(vin.x,vin_y,vin.z,vm,pm,OGLViewPort,&x,&y,&z);
		vout.x=(float)x;
		vout.y=(float)y;
		vout.z=(float)z;
	}
#endif
}

// screen (0...1,0...1,0...1) -> world
void NixGetVecUnprojectRel(vector &vout,const vector &vin)
{
	vector vi_r=vin;
	vi_r.x*=(float)NixTargetWidth;
	vi_r.y*=(float)NixTargetHeight;
	NixGetVecUnproject(vout,vi_r);
}
