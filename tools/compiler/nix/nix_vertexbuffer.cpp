/*----------------------------------------------------------------------------*\
| Nix vertex buffer                                                            |
| -> handling vertex buffers                                                   |
|                                                                              |
| last update: 2010.03.11 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#include "nix.h"
#include "nix_common.h"

std::vector<sVertexBuffer> NixVB;


// bei angegebenem index wird der bestehende VB neu erstellt
int NixCreateVB(int max_trias,int index)
{
	msg_db_r("creating vertex buffer", 1);
	bool create = (index < 0);
	if (create){
		index = NixVB.size();
		for (int i=0;i<NixVB.size();i++)
			if (!NixVB[i].Used){
				index = i;
				break;
			}
		if (index == NixVB.size())
			NixVB.resize(NixVB.size() + 1);
	}
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		// VertexBuffer selbst
		HRESULT hr;
		hr=lpDevice->CreateVertexBuffer(	3*max_trias*sizeof(DXVertex3D),
											D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
											D3D_VERTEX3D,
											D3DPOOL_DEFAULT,
											&DXVBVertices[index],
											NULL);
		if (!SUCCEEDED(hr))
			NixVB[index].dxVertices=NULL;
		if (!NixVB[index].dxVertices){
			msg_error("couldn't create vertex buffer");
			msg_write(DXErrorMsg(hr));
			msg_db_l(1);
			return -1;
		}
		// IndexBuffer
		#ifdef ENABLE_INDEX_BUFFERS
		hr=lpDevice->CreateIndexBuffer(	2*3*max_trias, // 2byte * 3(Punkte pro Dreieck) * AnzahlDreiecke
										D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
										D3DFMT_INDEX16, // noch nicht mehr als 65536 Punkte!
										D3DPOOL_DEFAULT,
										&DXVBIndex[index],
										NULL);
		if (!SUCCEEDED(hr))
			DXVBIndex[index]=NULL;
		if (!DXVBIndex[index]){
			msg_error("couldn't create index buffer");
			msg_write(DXErrorMsg(hr));
			msg_db_l(1);
			return -1;
		}
		#endif
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		NixVB[index].glVertices = new vector[max_trias*3];
		NixVB[index].glNormals = new vector[max_trias*3];
		NixVB[index].glTexCoords[0] = new float[max_trias*6];
		if ((!NixVB[index].glVertices) || (!NixVB[index].glNormals) || (!NixVB[index].glTexCoords[0])){
			msg_error("couldn't create vertex buffer");
			msg_db_l(1);
			return -1;
		}
		#ifdef ENABLE_INDEX_BUFFERS
			/*VBIndex[NumVBs]=new ...; // noch zu bearbeiten...
			if (!OGLVBIndex[index]){
				msg_error("IndexBuffer konnte nicht erstellt werden");
				return -1;
			}*/
		#endif
	}
#endif
	//msg_write(index);
	//msg_write(max_trias);
	NixVB[index].NumTextures = 1;
	NixVB[index].NumTrias = NixVB[index].NumPoints = 0;
	NixVB[index].MaxTrias = max_trias;
	//NixVB[index].MaxPoints = max_trias;
	NixVB[index].Used = true;
	NixVB[index].NotedFull = false;
	msg_db_l(1);
	return index;
}

int NixCreateVBM(int max_trias,int num_textures,int index)
{
	msg_db_r(string2("creating vertex buffer (%d tex coords)",num_textures), 1);
	bool create = (index < 0);
	if (num_textures > NIX_MAX_TEXTURELEVELS)
		num_textures = NIX_MAX_TEXTURELEVELS;
	if (create){
		index = NixVB.size();
		for (int i=0;i<NixVB.size();i++)
			if (!NixVB[i].Used){
				index = i;
				break;
			}
		if (index == NixVB.size())
			NixVB.resize(NixVB.size() + 1);
	}
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		// VertexBuffer selbst
		HRESULT hr;
		int s0=0,ss=0;
		if (num_textures==2){
			s0=sizeof(DXVertex3D2);
			ss=D3D_VERTEX3D2;
		}
		if (num_textures==3){
			s0=sizeof(DXVertex3D3);
			ss=D3D_VERTEX3D3;
		}
		if (num_textures==4){
			s0=sizeof(DXVertex3D4);
			ss=D3D_VERTEX3D4;
		}
		hr=lpDevice->CreateVertexBuffer(	3 * max_trias * s0,
											D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
											ss,
											D3DPOOL_DEFAULT,
											&DXVBVertices[index],
											NULL);
		if (!SUCCEEDED(hr))
			DXVBVertices[index]=NULL;
		if (!DXVBVertices[index]){
			msg_error("couldn't create vertex buffer");
			msg_write(DXErrorMsg(hr));
			msg_db_l(1);
			return -1;
		}
		// IndexBuffer
		#ifdef ENABLE_INDEX_BUFFERS
		hr=lpDevice->CreateIndexBuffer(	2*3*max_trias, // 2byte * 3(Punkte pro Dreieck) * AnzahlDreiecke
										D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
										D3DFMT_INDEX16, // noch nicht mehr als 65536 Punkte!
										D3DPOOL_DEFAULT,
										&DXVBIndex[index],
										NULL);
		if (!SUCCEEDED(hr))
			DXVBIndex[index]=NULL;
		if (!DXVBIndex[index]){
			msg_error("couldn't create index buffer");
			msg_write(DXErrorMsg(hr));
			msg_db_l(1);
			return -1;
		}
		#endif
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		NixVB[index].glVertices = new vector[max_trias*3];
		NixVB[index].glNormals = new vector[max_trias*3];
		bool failed = ((!NixVB[index].glVertices) || (!NixVB[index].glNormals));
		for (int i=0;i<num_textures;i++){
			NixVB[index].glTexCoords[i] = new float[max_trias*6];
			failed = failed || (!NixVB[index].glTexCoords[i]);
		}
		if (failed){
			msg_error("couldn't create vertex buffer");
			msg_db_l(1);
			return -1;
		}
		#ifdef ENABLE_INDEX_BUFFERS
			/*VBIndex[NumVBs]=new ...; // noch zu bearbeiten...
			if (!OGLVBIndex[index]){
				msg_error("IndexBuffer konnte nicht erstellt werden");
				return -1;
			}*/
		#endif
	}
#endif
	NixVB[index].NumTextures = num_textures;
	NixVB[index].NumTrias = NixVB[index].NumPoints = 0;
	NixVB[index].MaxTrias = max_trias;
	//NixVB[buffer].MaxPoints[index]=max_trias;
	NixVB[index].Used = true;
	NixVB[index].NotedFull = false;
	msg_db_l(1);
	return index;
}

void NixDeleteVB(int buffer)
{
	if (buffer < 0)
		return;
	if (!NixVB[buffer].Used)
		return;
	msg_db_r("deleting vertex buffer", 1);
	//msg_write(buffer);
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		//msg_write("release");
		DXVBVertices[buffer]->Release();
		//msg_write("del");
		//delete(DXVBVertices[buffer]);
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		delete[](NixVB[buffer].glVertices);
		delete[](NixVB[buffer].glNormals);
		for (int i=0;i<NixVB[buffer].NumTextures;i++)
			delete[](NixVB[buffer].glTexCoords[i]);
	}
#endif
	NixVB[buffer].Used = false;
	msg_db_l(1);
}

bool NixVBAddTria(int buffer,	const vector &p1,const vector &n1,float tu1,float tv1,
								const vector &p2,const vector &n2,float tu2,float tv2,
								const vector &p3,const vector &n3,float tu3,float tv3)
{
	if (NixVB[buffer].NumTrias > NixVB[buffer].MaxTrias){
		if (!NixVB[buffer].NotedFull){
			msg_error("too many triangles in the vertex buffer!");
			msg_write(buffer);
			NixVB[buffer].NotedFull = true;
		}
		return false;
	}
	//msg_write("VertexBufferAddTriangle");
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		unsigned char *pVerts=NULL;
		DXVertex3D Vert[3];
		Vert[0].x=p1.x;	Vert[0].y=p1.y;	Vert[0].z=p1.z;	Vert[0].nx=n1.x;	Vert[0].ny=n1.y;	Vert[0].nz=n1.z;	Vert[0].tu=tu1;	Vert[0].tv=tv1;
		Vert[1].x=p2.x;	Vert[1].y=p2.y;	Vert[1].z=p2.z;	Vert[1].nx=n2.x;	Vert[1].ny=n2.y;	Vert[1].nz=n2.z;	Vert[1].tu=tu2;	Vert[1].tv=tv2;
		Vert[2].x=p3.x;	Vert[2].y=p3.y;	Vert[2].z=p3.z;	Vert[2].nx=n3.x;	Vert[2].ny=n3.y;	Vert[2].nz=n3.z;	Vert[2].tu=tu3;	Vert[2].tv=tv3;

		Vert[0].tu=tu1;	Vert[0].tv=tv1;
		Vert[1].tu=tu2;	Vert[1].tv=tv2;
		Vert[2].tu=tu3;	Vert[2].tv=tv3;

		DXVBVertices[buffer]->Lock(sizeof(DXVertex3D)*3*VBNumTrias[buffer],sizeof(DXVertex3D)*3,(void**)&pVerts,0);
		memcpy(pVerts,Vert,sizeof(DXVertex3D)*3);
		DXVBVertices[buffer]->Unlock();
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		NixVB[buffer].glVertices[NixVB[buffer].NumTrias*3  ]=p1;
		NixVB[buffer].glVertices[NixVB[buffer].NumTrias*3+1]=p2;
		NixVB[buffer].glVertices[NixVB[buffer].NumTrias*3+2]=p3;
		NixVB[buffer].glNormals[NixVB[buffer].NumTrias*3  ]=n1;
		NixVB[buffer].glNormals[NixVB[buffer].NumTrias*3+1]=n2;
		NixVB[buffer].glNormals[NixVB[buffer].NumTrias*3+2]=n3;
		NixVB[buffer].glTexCoords[0][NixVB[buffer].NumTrias*6  ]=tu1;
		NixVB[buffer].glTexCoords[0][NixVB[buffer].NumTrias*6+1]=1-tv1;
		NixVB[buffer].glTexCoords[0][NixVB[buffer].NumTrias*6+2]=tu2;
		NixVB[buffer].glTexCoords[0][NixVB[buffer].NumTrias*6+3]=1-tv2;
		NixVB[buffer].glTexCoords[0][NixVB[buffer].NumTrias*6+4]=tu3;
		NixVB[buffer].glTexCoords[0][NixVB[buffer].NumTrias*6+5]=1-tv3;
	}
#endif
	NixVB[buffer].NumTrias ++;
	NixVB[buffer].Indexed = false;
	return true;
}

bool NixVBAddTriaM(int buffer,	const vector &p1,const vector &n1,const float *t1,
								const vector &p2,const vector &n2,const float *t2,
								const vector &p3,const vector &n3,const float *t3)
{
	if (buffer<0)	return false;
	if (NixVB[buffer].NumTrias > NixVB[buffer].MaxTrias){
		if (!NixVB[buffer].NotedFull){
			msg_error("too many triangles in the vertex buffer!");
			NixVB[buffer].NotedFull = true;
		}
		return false;
	}
#ifdef NIX_API_DIRECTX9
	if (NixApi==NIX_API_DIRECTX9){
		unsigned char *pVerts=NULL;
		if (VBNumTextures[buffer]==2){
			DXVertex3D2 Vert[3];
			Vert[0].x=p1.x;	Vert[0].y=p1.y;	Vert[0].z=p1.z;	Vert[0].nx=n1.x;	Vert[0].ny=n1.y;	Vert[0].nz=n1.z;	Vert[0].tu0=t1[0];	Vert[0].tv0=t1[1];
			Vert[1].x=p2.x;	Vert[1].y=p2.y;	Vert[1].z=p2.z;	Vert[1].nx=n2.x;	Vert[1].ny=n2.y;	Vert[1].nz=n2.z;	Vert[1].tu0=t2[0];	Vert[1].tv0=t2[1];
			Vert[2].x=p3.x;	Vert[2].y=p3.y;	Vert[2].z=p3.z;	Vert[2].nx=n3.x;	Vert[2].ny=n3.y;	Vert[2].nz=n3.z;	Vert[2].tu0=t3[0];	Vert[2].tv0=t3[1];

			Vert[0].tu1=t1[2];	Vert[0].tv1=t1[3];
			Vert[1].tu1=t2[2];	Vert[1].tv1=t2[3];
			Vert[2].tu1=t3[2];	Vert[2].tv1=t3[3];

			DXVBVertices[buffer]->Lock(sizeof(DXVertex3D2)*3*VBNumTrias[buffer],sizeof(DXVertex3D2)*3,(void**)&pVerts,0);
			memcpy(pVerts,Vert,sizeof(DXVertex3D2)*3);
		}else if (VBNumTextures[buffer]==3){
			DXVertex3D3 Vert[3];
			Vert[0].x=p1.x;	Vert[0].y=p1.y;	Vert[0].z=p1.z;	Vert[0].nx=n1.x;	Vert[0].ny=n1.y;	Vert[0].nz=n1.z;	Vert[0].tu0=t1[0];	Vert[0].tv0=t1[1];
			Vert[1].x=p2.x;	Vert[1].y=p2.y;	Vert[1].z=p2.z;	Vert[1].nx=n2.x;	Vert[1].ny=n2.y;	Vert[1].nz=n2.z;	Vert[1].tu0=t2[0];	Vert[1].tv0=t2[1];
			Vert[2].x=p3.x;	Vert[2].y=p3.y;	Vert[2].z=p3.z;	Vert[2].nx=n3.x;	Vert[2].ny=n3.y;	Vert[2].nz=n3.z;	Vert[2].tu0=t3[0];	Vert[2].tv0=t3[1];

			Vert[0].tu1=t1[2];	Vert[0].tv1=t1[3];
			Vert[1].tu1=t2[2];	Vert[1].tv1=t2[3];
			Vert[2].tu1=t3[2];	Vert[2].tv1=t3[3];

			Vert[0].tu2=t1[4];	Vert[0].tv2=t1[5];
			Vert[1].tu2=t2[4];	Vert[1].tv2=t2[5];
			Vert[2].tu2=t3[4];	Vert[2].tv2=t3[5];

			DXVBVertices[buffer]->Lock(sizeof(DXVertex3D3)*3*VBNumTrias[buffer],sizeof(DXVertex3D3)*3,(void**)&pVerts,0);
			memcpy(pVerts,Vert,sizeof(DXVertex3D3)*3);
		}else if (VBNumTextures[buffer]==4){
			DXVertex3D4 Vert[3];
			Vert[0].x=p1.x;	Vert[0].y=p1.y;	Vert[0].z=p1.z;	Vert[0].nx=n1.x;	Vert[0].ny=n1.y;	Vert[0].nz=n1.z;	Vert[0].tu0=t1[0];	Vert[0].tv0=t1[1];
			Vert[1].x=p2.x;	Vert[1].y=p2.y;	Vert[1].z=p2.z;	Vert[1].nx=n2.x;	Vert[1].ny=n2.y;	Vert[1].nz=n2.z;	Vert[1].tu0=t2[0];	Vert[1].tv0=t2[1];
			Vert[2].x=p3.x;	Vert[2].y=p3.y;	Vert[2].z=p3.z;	Vert[2].nx=n3.x;	Vert[2].ny=n3.y;	Vert[2].nz=n3.z;	Vert[2].tu0=t3[0];	Vert[2].tv0=t3[1];

			Vert[0].tu1=t1[2];	Vert[0].tv1=t1[3];
			Vert[1].tu1=t2[2];	Vert[1].tv1=t2[3];
			Vert[2].tu1=t3[2];	Vert[2].tv1=t3[3];

			Vert[0].tu2=t1[4];	Vert[0].tv2=t1[5];
			Vert[1].tu2=t2[4];	Vert[1].tv2=t2[5];
			Vert[2].tu2=t3[4];	Vert[2].tv2=t3[5];

			Vert[0].tu3=t1[6];	Vert[0].tv3=t1[7];
			Vert[1].tu3=t2[6];	Vert[1].tv3=t2[7];
			Vert[2].tu3=t3[6];	Vert[2].tv3=t3[7];

			DXVBVertices[buffer]->Lock(sizeof(DXVertex3D4)*3*VBNumTrias[buffer],sizeof(DXVertex3D4)*3,(void**)&pVerts,0);
			memcpy(pVerts,Vert,sizeof(DXVertex3D4)*3);
		}
		DXVBVertices[buffer]->Unlock();
	}
#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		int n = NixVB[buffer].NumTrias;
		NixVB[buffer].glVertices[n*3  ]=p1;
		NixVB[buffer].glVertices[n*3+1]=p2;
		NixVB[buffer].glVertices[n*3+2]=p3;
		NixVB[buffer].glNormals[n*3  ]=n1;
		NixVB[buffer].glNormals[n*3+1]=n2;
		NixVB[buffer].glNormals[n*3+2]=n3;
		for (int i=0;i<NixVB[buffer].NumTextures;i++){
			NixVB[buffer].glTexCoords[i][n*6  ]=t1[i*2  ];
			NixVB[buffer].glTexCoords[i][n*6+1]=1-t1[i*2+1];
			NixVB[buffer].glTexCoords[i][n*6+2]=t2[i*2  ];
			NixVB[buffer].glTexCoords[i][n*6+3]=1-t2[i*2+1];
			NixVB[buffer].glTexCoords[i][n*6+4]=t3[i*2  ];
			NixVB[buffer].glTexCoords[i][n*6+5]=1-t3[i*2+1];
		}
	}
#endif
	NixVB[buffer].NumTrias ++;
	NixVB[buffer].Indexed = false;
	return true;
}

// for each triangle there have to be 3 vertices (p[i],n[i],t[i*2],t[i*2+1])
void NixVBAddTrias(int buffer,int num_trias,const vector *p,const vector *n,const float *t)
{
	#ifdef NIX_API_DIRECTX9
		if (NixApi==NIX_API_DIRECTX9){
			// fill our vertex buffer
			unsigned char *pVerts=NULL;
			DXVertex3D Vert;
			DXVBVertices[buffer]->Lock(0,sizeof(DXVertex3D)*num_trias*3,(void**)&pVerts,0);
			for (int i=0;i<num_trias*3;i++){
				Vert.x=p[i].x;	Vert.y=p[i].y;	Vert.z=p[i].z;
				Vert.nx=n[i].x;	Vert.ny=n[i].y;	Vert.nz=n[i].z;
				Vert.tu=t[i*2];	Vert.tv=t[i*2+1];
				//memcpy(pVerts,&Vert,sizeof(DXVertex3D));
				*(DXVertex3D*)pVerts=Vert;
				pVerts+=sizeof(DXVertex3D);
			}
			DXVBVertices[buffer]->Unlock();
		}
	#endif
#ifdef NIX_API_OPENGL
	if (NixApi==NIX_API_OPENGL){
		memcpy(NixVB[buffer].glVertices, p, sizeof(vector) * num_trias * 3);
		memcpy(NixVB[buffer].glNormals, n, sizeof(vector) * num_trias * 3);
		//memcpy(OGLVBTexCoords[buffer][0],t,sizeof(float)*num_trias*6);
		for (int i=0;i<num_trias*3;i++){
			NixVB[buffer].glTexCoords[0][i*2  ] = t[i*2];
			NixVB[buffer].glTexCoords[0][i*2+1] = t[i*2+1];
		}
	}
#endif
	NixVB[buffer].NumTrias += num_trias;
	NixVB[buffer].NumPoints += num_trias * 3;
}

void NixVBAddTriasIndexed(int buffer,int num_points,int num_trias,const vector *p,const vector *n,const float *tu,const float *tv,const unsigned short *indices)
{
	#ifdef ENABLE_INDEX_BUFFERS
		#ifdef NIX_API_DIRECTX9
		if (NixApi==NIX_API_DIRECTX9){
			int i;
			// VertexBuffer
			unsigned char *pVerts=NULL;
			Vertex3D Vert;
			VBVertices[buffer]->Lock(0,sizeof(Vertex3D)*num_points,&pVerts,0);
			for (i=0;i<num_points;i++){
				Vert.x=p[i].x;	Vert.y=p[i].y;	Vert.z=p[i].z;
				Vert.nx=n[i].x;	Vert.ny=n[i].y;	Vert.nz=n[i].z;
				Vert.tu=tu[i];	Vert.tv=tv[i];
				memcpy(pVerts,&Vert,sizeof(Vertex3D));
				pVerts+=sizeof(Vertex3D);
			}
			VBVertices[buffer]->Unlock();
			// IndexBuffer
			unsigned char *pIndex=NULL;
			VBIndex[buffer]->Lock(0,2*3*num_trias,&pIndex,0);
			for (i=0;i<num_trias*3;i++){
				pIndex[i*2  ]=indices[i]/256;
				pIndex[i*2+1]=indices[i]%256;
			}
			VBIndex[buffer]->Unlock();
		}
		#endif
		VBNumTrias[buffer]=num_trias;
		VBNumPoints[buffer]=num_points;
		VBIndexed[buffer]=true;
	#endif
}

void NixVBClear(int buffer)
{
	NixVB[buffer].NumTrias = 0;
	NixVB[buffer].NumPoints = 0;
	NixVB[buffer].Indexed = false;
	NixVB[buffer].NotedFull = false;
}

