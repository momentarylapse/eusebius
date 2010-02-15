/*----------------------------------------------------------------------------*\
| Nix types                                                                    |
| -> mathematical types and functions                                          |
|   -> vector, matrix, matrix3, quaternion, plane, color                       |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2008.12.19 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

#ifndef _NIX_TYPES_EXISTS_
#define _NIX_TYPES_EXISTS_


#include <math.h>


// types
struct rect
{
public:
	float x1,x2,y1,y2;
	rect(){};
	rect(float x1,float x2,float y1,float y2)
	{	this->x1=x1;	this->x2=x2;	this->y1=y1;	this->y2=y2;	}
};
struct color
{
public:
	float r,g,b,a;
	color(){};
	color(float a,float r,float g,float b)
	{	this->a=a;	this->r=r;	this->g=g;	this->b=b;	}
	/*color& operator = (const color& c)
	{	a=c.a;	r=c.r;	g=c.g;	b=c.b;	return *this;	}*/
	color& operator += (const color& c)
	{	a+=c.a;	r+=c.r;	g+=c.g;	b+=c.b;	return *this;	}
	color operator * (float f) const
	{	return color( a*f , r*f , g*f , b*f );	}
};
struct vector
{
public:
	float x,y,z;
	vector(){};
	vector(float x,float y,float z)
	{	this->x=x;	this->y=y;	this->z=z;	}
	// assignment operators
	vector& operator += (const vector& v)
	{	x+=v.x;	y+=v.y;	z+=v.z;	return *this;	}
	vector& operator -= (const vector& v)
	{	x-=v.x;	y-=v.y;	z-=v.z;	return *this;	}
	vector& operator *= (float f)
	{	x*=f;	y*=f;	z*=f;	return *this;	}
	vector& operator /= (float f)
	{	x/=f;	y/=f;	z/=f;	return *this;	}
	// unitary operator(s)
	vector operator - ()
	{	return vector(-x,-y,-z);	}
	// binary operators
	vector operator + (const vector &v) const
	{	return vector( x+v.x , y+v.y , z+v.z );	}
	vector operator - (const vector &v) const
	{	return vector( x-v.x , y-v.y , z-v.z );	}
	vector operator * (float f) const
	{	return vector( x*f , y*f , z*f );	}
	vector operator / (float f) const
	{	return vector( x/f , y/f , z/f );	}
	friend vector operator * (float f,const vector &v)
	{	return v*f;	}
	bool operator == (const vector &v) const
	{	return ((x==v.x)&&(y==v.y)&&(z==v.z));	}
	bool operator != (const vector &v) const
	{	return !((x==v.x)&&(y==v.y)&&(z==v.z));	}
	float operator * (vector v) const
	{	return x*v.x + y*v.y + z*v.z;	}
	vector operator ^ (vector v) const
	{	return vector( y*v.z-z*v.y, z*v.x-x*v.z, x*v.y-y*v.x );	}
};
#define _element(row,col)	e[row+col*4]
struct matrix
{
public:
	union{
		struct{
			// the squared form of this block is "transposed"!
			float _00,_10,_20,_30;
			float _01,_11,_21,_31;
			float _02,_12,_22,_32;
			float _03,_13,_23,_33;
		};
		float __e[4][4];
#define _e(i,j)		__e[j][i]
		float e[16];
	};

	matrix(){};
	matrix(const float f[16]){
		for (int i=0;i<16;i++)
			this->e[i]=f[i];
	};
	matrix operator + (const matrix &m) const
	{
		matrix r;
		for (int i=0;i<16;i++)
			r.e[i]=e[i]+m.e[i];
		return r;
	}
	matrix operator - (const matrix &m) const
	{
		matrix r;
		for (int i=0;i<16;i++)
			r.e[i]=e[i]-m.e[i];
		return r;
	}
	matrix operator * (float f) const
	{
		matrix r;
		for (int i=0;i<16;i++)
			r.e[i]=e[i]*f;
		return r;
	}
	friend matrix operator * (float f,const matrix &m)
	{	return m*f;	}
	vector operator * (vector v) const
	{
		return vector(	v.x*_00 + v.y*_01 + v.z*_02 + _03,
						v.x*_10 + v.y*_11 + v.z*_12 + _13,
						v.x*_20 + v.y*_21 + v.z*_22 + _23);
	}
	friend vector operator * (vector v, const matrix &m)
	{	return m*v;	}
};
struct matrix3
{
public:
	union{
		struct{
			// the squared form of this block is "transposed"!
			float _00,_10,_20;
			float _01,_11,_21;
			float _02,_12,_22;
		};
		float __e[3][3];
#define _e3(i,j)		__e[j][i]
		float e[9];
	};

	matrix3(){};
	matrix3 operator + (const matrix3 &m) const
	{
		matrix3 r;
		for (int i=0;i<9;i++)
			r.e[i]=e[i]+m.e[i];
		return r;
	}
	matrix3 operator - (const matrix3 &m) const
	{
		matrix3 r;
		for (int i=0;i<9;i++)
			r.e[i]=e[i]-m.e[i];
		return r;
	}
	matrix3 operator * (float f) const
	{
		matrix3 r;
		for (int i=0;i<9;i++)
			r.e[i]=e[i]*f;
		return r;
	}
	friend matrix3 operator * (float f,const matrix3 &m)
	{	return m*f;	}
	matrix3 operator / (float f) const
	{
		matrix3 r;
		for (int i=0;i<9;i++)
			r.e[i]=e[i]/f;
		return r;
	}
	friend matrix3 operator / (float f,const matrix3 &m)
	{	return m/f;	}
	matrix3 operator * (const matrix3 &m) const
	{
		matrix3 r;
		r._00 = _00*m._00 + _01*m._10 + _02*m._20;
		r._01 = _00*m._01 + _01*m._11 + _02*m._21;
		r._02 = _00*m._02 + _01*m._12 + _02*m._22;
		r._10 = _10*m._00 + _11*m._10 + _12*m._20;
		r._11 = _10*m._01 + _11*m._11 + _12*m._21;
		r._12 = _10*m._02 + _11*m._12 + _12*m._22;
		r._20 = _20*m._00 + _21*m._10 + _22*m._20;
		r._21 = _20*m._01 + _21*m._11 + _22*m._21;
		r._22 = _20*m._02 + _21*m._12 + _22*m._22;
		return r;
	}
	vector operator * (vector &v) const
	{
		return vector(	v.x*_00 + v.y*_01 + v.z*_02,
						v.x*_10 + v.y*_11 + v.z*_12,
						v.x*_20 + v.y*_21 + v.z*_22);
	}
	friend vector operator * (vector &v, const matrix3 &m)
	{	return m*v;	}
};
struct plane{	float a,b,c,d;	};
struct quaternion
{
public:
	float x,y,z,w;
	quaternion(){};
	quaternion(const float w,const vector &v)
	{	this->w=w;	this->x=v.x;	this->y=v.y;	this->z=v.z;	}
	bool operator == (const quaternion& q) const
	{	return ((x==q.x)&&(y==q.y)&&(z==q.z)&&(w==q.w));	}
	bool operator != (const quaternion& q) const
	{	return !((x==q.x)&&(y==q.y)&&(z==q.z)&&(w!=q.w));	}
	quaternion& operator += (const quaternion& q)
	{	x+=q.x;	y+=q.y;	z+=q.z;	w+=q.w;	return *this;	}
	quaternion& operator -= (const quaternion& q)
	{	x-=q.x;	y-=q.y;	z-=q.z;	w-=q.w;	return *this;	}
	quaternion operator + (const quaternion &q) const
	{
		quaternion r;
		r.x=q.x+x;
		r.y=q.y+y;
		r.z=q.z+z;
		r.w=q.w+w;
		return r;
	}
	quaternion operator - (const quaternion &q) const
	{
		quaternion r;
		r.x=q.x-x;
		r.y=q.y-y;
		r.z=q.z-z;
		r.w=q.w-w;
		return r;
	}
	quaternion operator * (float f) const
	{
		quaternion r;
		r.x=x*f;
		r.y=y*f;
		r.z=z*f;
		r.w=w*f;
		return r;
	}
	quaternion operator * (const quaternion &q) const
	{
		quaternion r;
		r.w = w*q.w - x*q.x - y*q.y - z*q.z;
		r.x = w*q.x + x*q.w + y*q.z - z*q.y;
		r.y = w*q.y + y*q.w + z*q.x - x*q.z;
		r.z = w*q.z + z*q.w + x*q.y - y*q.x;
		return r;
	}
	friend quaternion operator * (float f,const quaternion &q)
	{	return q*f;	}
};


//--------------------------------------------------------------------//
//                         all about types                            //
//--------------------------------------------------------------------//

// ints
void _cdecl clampi(int &i,int min,int max);
void _cdecl loopi(int &i,int min,int max);
int _cdecl randi(int m);

// floats
float sqr(float f);
void _cdecl clampf(float &f,float min,float max);
void _cdecl loopf(float &f,float min,float max);
float _cdecl randf(float m);

const float pi=3.141592654f;

// rects

const rect r01=rect(0,1,0,1);

// vectors
float _cdecl VecLength(const vector &v);
float VecLengthFuzzy(const vector &v);
float VecLengthSqr(const vector &v);
bool VecBetween(const vector &v,const vector &a,const vector &b);
float VecFactorBetween(const vector &v,const vector &a,const vector &b);
bool VecBoundingBox(const vector &a,const vector &b,float d);
void _cdecl VecNormalize(vector &vo,const vector &vi);
float _cdecl VecDotProduct(const vector &v1,const vector &v2);
vector _cdecl VecCrossProduct(const vector &v1,const vector &v2);
vector _cdecl VecAng2Dir(const vector &ang);
vector _cdecl VecDir2Ang(const vector &dir);
vector _cdecl VecDir2Ang2(const vector &dir,const vector &up);
vector _cdecl VecAngAdd(const vector &ang1,const vector &ang2);
vector _cdecl VecAngInterpolate(const vector &ang1,const vector &ang2,float t);
float VecLineDistance(const vector &p,const vector &l1,const vector &l2);
vector VecLineNearestPoint(vector &p,vector &l1,vector &l2);
void _cdecl VecTransform(vector &vo,const matrix &m,const vector &vi);
void _cdecl VecNormalTransform(vector &vo,const matrix &m,const vector &vi);
void _cdecl VecTransform3(vector &vo,const matrix3 &m,const vector &vi);
void VecMin(vector &v,const vector &test_partner);
void VecMax(vector &v,const vector &test_partner);

const vector v0=vector(0,0,0);
const vector e_x=vector(1,0,0);
const vector e_y=vector(0,1,0);
const vector e_z=vector(0,0,1);

// matrices
void _cdecl MatrixMultiply(matrix &m,const matrix &m2,const matrix &m1);
void _cdecl MatrixIdentity(matrix &m);
void _cdecl MatrixInverse(matrix &mo,const matrix &mi);
void _cdecl MatrixTranspose(matrix &mo,const matrix &mi);
void _cdecl MatrixTranslation(matrix &m,const vector &v);
void _cdecl MatrixRotationX(matrix &m,float w);
void _cdecl MatrixRotationY(matrix &m,float w);
void _cdecl MatrixRotationZ(matrix &m,float w);
void _cdecl MatrixRotation(matrix &m,const vector &ang);
void _cdecl MatrixRotationView(matrix &m,const vector &ang);
void _cdecl MatrixRotationQ(matrix &m,const quaternion &q);
void _cdecl MatrixScale(matrix &m,float fx,float fy,float fz);
void _cdecl MatrixReflect(matrix &m,const plane &pl);

const float f_m_id[16]={ 1,0,0,0 , 0,1,0,0 , 0,0,1,0 , 0,0,0,1 };
const matrix m_id=matrix(f_m_id);

// matrix3s
void _cdecl Matrix3Identity(matrix3 &m);
void _cdecl Matrix3Inverse(matrix3 &mo,const matrix3 &mi);
void _cdecl Matrix3Transpose(matrix3 &mo,const matrix3 &mi);
void _cdecl Matrix3Rotation(matrix3 &m,const vector &ang);
void _cdecl Matrix3RotationQ(matrix3 &m,const quaternion &q);

// qaternions
void _cdecl QuaternionRotationA(quaternion &q,const vector &axis,float w);
void _cdecl QuaternionRotationV(quaternion &q,const vector &ang);
//void QuaternionRotationView(quaternion &q,const vector &ang);
void _cdecl QuaternionRotationM(quaternion &q,const matrix &m);
void _cdecl QuaternionInverse(quaternion &qo,const quaternion &qi);
void _cdecl QuaternionMultiply(quaternion &q,const quaternion &q2,const quaternion &q1);
void _cdecl QuaternionInterpolate(quaternion &q,const quaternion &q1,const quaternion &q2,float t);
void _cdecl QuaternionInterpolate(quaternion &q,const quaternion &q1,const quaternion &q2,const quaternion &q3,const quaternion &q4,float t);
vector _cdecl QuaternionToAngle(const quaternion &q);
void _cdecl QuaternionScale(quaternion &q,float f);
void _cdecl QuaternionNormalize(quaternion &qo,const quaternion &qi);
vector GetAxis(const quaternion &q);
float GetAngle(const quaternion &q);

const quaternion q_id=quaternion(1,v0);

// planes
void PlaneFromPoints(plane &pl,const vector &a,const vector &b,const vector &c);
void PlaneFromPointNormal(plane &pl,const vector &p,const vector &n);
void PlaneTransform(plane &plo,const matrix &m,const plane &pli);
vector GetNormal(const plane &pl);
bool PlaneIntersectLine(vector &i,const plane &pl,const vector &l1,const vector &l2);
int ImportantPlane(const vector &v);
void GetBaryCentric(const vector &P,const vector &A,const vector &B,const vector &C,float &f,float &g);
static float LineIntersectsTriangleF,LineIntersectsTriangleG;
bool LineIntersectsTriangle(const vector &t1,const vector &t2,const vector &t3,const vector &l1,const vector &l2,vector &col,bool vm);
float PlaneDistance(const plane &pl,const vector &p);
void PlaneInverse(plane &pl);

// colors
color _cdecl SetColorSave(float a,float r,float g, float b);
color _cdecl SetColorHSB(float a,float hue,float saturation,float brightness);
color _cdecl ColorScale(const color &c,float f);
color _cdecl ColorInterpolate(const color &a,const color &b,float t);
color _cdecl ColorMultiply(const color &a,const color &b);
color _cdecl ColorFromIntRGB(int *i);
color _cdecl ColorFromIntARGB(int *i);
void _cdecl Color2IntRGB(const color &c,int *i);
void _cdecl Color2IntARGB(const color &c,int *i);

static color White=color(1,1,1,1);
static color Black=color(1,0,0,0);
static color Grey=color(1,0.5f,0.5f,0.5f);
static color Gray=color(1,0.5f,0.5f,0.5f);
static color Red=color(1,1,0,0);
static color Green=color(1,0,1,0);
static color Blue=color(1,0,0,1);
static color Yellow=color(1,1,1,0);
static color Orange=color(1,1,0.5f,0);

// faster functions

inline bool inf_f(float f)
{
	/*int t=*(int*)&f;
	int m=0x7f000000;
	if ((t&m)==m)   return true;
	return (f!=f);*/
#ifdef NIX_OS_WINDOWS
	return false;
#else
	return !isfinite(f);
#endif
}

inline bool inf_v(vector v)
{   return (inf_f(v.x)||inf_f(v.y)||inf_f(v.z));  }

inline bool inf_pl(plane p)
{   return (inf_f(p.a)||inf_f(p.b)||inf_f(p.c)||inf_f(p.d));  }

inline float _vec_length_(const vector &v)
{	return sqrt(v*v);	}

inline float _vec_length_fuzzy_(const vector &v)
{
	float x=fabs(v.x);
	float y=fabs(v.y);
	float z=fabs(v.z);
	float xy=(x>y)?x:y;
	return (xy>z)?xy:z;
}

inline void _vec_normalize_(vector &vo,const vector &vi)
{	vo=vi/sqrt(vi*vi);	}

inline bool _vec_between_(const vector &v,const vector &a,const vector &b)
{
	if ((v.x>a.x)&&(v.x>b.x))	return false;
	if ((v.x<a.x)&&(v.x<b.x))	return false;
	if ((v.y>a.y)&&(v.y>b.y))	return false;
	if ((v.y<a.y)&&(v.y<b.y))	return false;
	if ((v.z>a.z)&&(v.z>b.z))	return false;
	if ((v.z<a.z)&&(v.z<b.z))	return false;
	return true;
}

inline float _vec_factor_between_(const vector &v,const vector &a,const vector &b)
{	return ((v-a)*(b-a)) / ((b-a)*(b-a));	}

#define _get_normal_(pl)	(*(vector*)&pl)

inline void _get_bary_centric_(const vector &p,const plane &pl,const vector &a,const vector &b,const vector &c,float &f,float &g)
{
	vector ba=b-a,ca=c-a;
	vector pvec=_get_normal_(pl)^ca;
	float det=ba*pvec;
	vector pa;
	if (det>0)
		pa=p-a;
	else{
		pa=a-p;
		det=-det;
	}
	f=pa*pvec;
	vector qvec=pa^ba;
	g=_get_normal_(pl)*qvec;
	float inv_det=1.0f/det;
	f*=inv_det;
	g*=inv_det;
}

inline void _plane_from_point_normal_(plane &pl,const vector &p,const vector &n)
{
	pl.a=n.x;
	pl.b=n.y;
	pl.c=n.z;
	pl.d=-(n*p);
}

inline bool _plane_intersect_line_(vector &cp,const plane &pl,const vector &l1,const vector &l2)
{
	float e=_get_normal_(pl)*l1;
	float f=_get_normal_(pl)*l2;
	if (e==f) // parallel?
		return false;
	float t=-(pl.d+f)/(e-f);
	//if ((t>=0)&&(t<=1)){
		//cp = l1 + t*(l2-l1);
		cp = l2 + t*(l1-l2);
	return true;
}

inline float _plane_distance_(const plane &pl,const vector &p)
{	return pl.a*p.x + pl.b*p.y + pl.c*p.z + pl.d;	}

inline vector *_matrix_get_translation_(const matrix &m)
{	return (vector*)&m._03;	} // (_03, _13, _23) happens to be aligned the right way...



#endif
