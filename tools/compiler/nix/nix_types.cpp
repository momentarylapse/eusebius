/*----------------------------------------------------------------------------*\
| Nix types                                                                    |
| -> mathematical types and functions                                          |
|   -> vector, matrix, matrix3, quaternion, plane, color                       |
|                                                                              |
| vital properties:                                                            |
|                                                                              |
| last update: 2008.10.26 (c) by MichiSoft TM                                  |
\*----------------------------------------------------------------------------*/

 
 #include "nix.h"




//------------------------------------------------------------------------------------------------//
//                                      for using the types                                       //
//------------------------------------------------------------------------------------------------//

// ZXY -> Objekte und Figur-Teile / Modell-Transformationen
// der Vektor nach vorne (0,0,1) wird
// 1. um die z-Achse gedreht (um sich selbst)
// 2. um die x-Achse gedreht (nach oben oder unten "genickt")
// 3. um die y-Achse gedreht (nach links oder rechts)
// (alle Drehungen um Achsen, die nicht veraendert werden!!!)

// YXZ -> Kamera-/Projektions-Transformation
// der Vektor nach vorne (0,0,1) wird
// ... aber in die jeweils andere Richtung (-ang)

// YXZ-Matrix ist die Inverse zur ZXY-Matrix!!!


//------------------------------------------------------------------------------------------------//
//                                             int                                                //
//------------------------------------------------------------------------------------------------//


// force <i> within a boundary by cutting it off
void clampi(int &i,int min,int max)
{
	if (i<min)
		i=min;
	if (i>max)
		i=max;
}

// force <i> within a boundary by modulo-ing
void loopi(int &i,int min,int max)
{
	int d=max-min+1;
	if (i<min){
		int n= (int)( (min-i-1) / d ) + 1;
		i+=d*n;
	}
	if (i>max){
		int n= (int)( (i-max-1) / d ) + 1;
		i-=d*n;
	}
}

// random int
int randi(int m)
{
	return int((float)rand()*m/(float)RAND_MAX);
}


//------------------------------------------------------------------------------------------------//
//                                             float                                              //
//------------------------------------------------------------------------------------------------//


// square
float sqr(float f)
{
	return f*f;
}

// force <f> within a boundary by cutting it off
void clampf(float &f,float min,float max)
{
	if (f<min)
		f=min;
	if (f>max)
		f=max;
}

// force <f> within a boundary by modulo-ing
void loopf(float &f,float min,float max)
{
	float d=max-min;
	if (f<min){
		int n= (int)( (min-f) / d ) + 1;
		f+=d*(float)n;
	}
	if (f>max){
		int n= (int)( (f-max) / d ) + 1;
		f-=d*(float)n;
	}
}

// random float
float randf(float m)
{
	return (float)rand()*m/(float)RAND_MAX;
}

//------------------------------------------------------------------------------------------------//
//                                            vectors                                             //
//------------------------------------------------------------------------------------------------//
// real length of the vector
float VecLength(const vector &v)
{
	return sqrtf( v*v );
}

// gibt die laengste Seite zurueck (="unendlich-Norm")
// (immer <= <echte Laenge>)
float VecLengthFuzzy(const vector &v)
{
	float l=fabs(v.x);
	float a=fabs(v.y);
	if (a>l)
		l=a;
	a=fabs(v.z);
	if (a>l)
		l=a;
	return l;
}

float VecLengthSqr(const vector &v)
{
	return v.x*v.x + v.y*v.y + v.z*v.z;
}

// v in cube(a,b) ?
bool VecBetween(const vector &v,const vector &a,const vector &b)
{
	/*if ((v.x>a.x)&&(v.x>b.x))	return false;
	if ((v.x<a.x)&&(v.x<b.x))	return false;
	if ((v.y>a.y)&&(v.y>b.y))	return false;
	if ((v.y<a.y)&&(v.y<b.y))	return false;
	if ((v.z>a.z)&&(v.z>b.z))	return false;
	if ((v.z<a.z)&&(v.z<b.z))	return false;
	return true;*/
	float f=VecDotProduct(v-a,b-a);
	if (f<0)
		return false;
	f/=VecDotProduct(b-a,b-a);
	return (f<=1);
}

// v = a + f*( b - a )
//   get f
float VecFactorBetween(const vector &v,const vector &a,const vector &b)
{
	if (a.x!=b.x)
		return ((v.x-a.x)/(b.x-a.x));
	else if (a.y!=b.y)
		return ((v.y-a.y)/(b.y-a.y));
	else if (a.z!=b.z)
		return ((v.z-a.z)/(b.z-a.z));
	return 0;
}

// a im Wuerfel mit Radius=d um b ?
bool VecBoundingBox(const vector &a,const vector &b,float d)
{
	if ((a.x-b.x>d)||(a.x-b.x<-d))
		return false;
	if ((a.y-b.y>d)||(a.y-b.y<-d))
		return false;
	if ((a.z-b.z>d)||(a.z-b.z<-d))
		return false;
	return true;
}

// auf die Laenge 1 bringen
void VecNormalize(vector &vo,const vector &vi)
{
	float l=sqrtf(vi*vi);
	if (l>0)
		vo=vi/l;
	else
		vo=vector(0,0,0);
}

// cos( Winkel zwischen Vektoren) * Laenge1 * Laenge2
float VecDotProduct(const vector &v1,const vector &v2)
{
	return v1.x*v2.x+v1.y*v2.y+v1.z*v2.z;
}

// Richtung: zu beiden orthogonal!!
// Laenge: sin( Winkel zwischen Vektoren) * Laenge1 * Laenge2
// (0,0,0) bei: ( v1 parallel v2 )
vector VecCrossProduct(const vector &v1,const vector &v2)
{
	vector v;
    v.x = v1.y * v2.z - v1.z * v2.y;
    v.y = v1.z * v2.x - v1.x * v2.z;
    v.z = v1.x * v2.y - v1.y * v2.x;
	return v;

}

// Koordinaten-Transformation
// matrix * vector(x,y,z,1)
void VecTransform(vector &vo,const matrix &m,const vector &vi)
{
	vector vi_=vi;
	vo.x= vi_.x*m._00 + vi_.y*m._01 + vi_.z*m._02 + m._03;
	vo.y= vi_.x*m._10 + vi_.y*m._11 + vi_.z*m._12 + m._13;
	vo.z= vi_.x*m._20 + vi_.y*m._21 + vi_.z*m._22 + m._23;
}

// Transformation eines Normalenvektors
// matrix * vector(x,y,z,0)
void VecNormalTransform(vector &vo,const matrix &m,const vector &vi)
{
	vector vi_=vi;
	vo.x= vi_.x*m._00 + vi_.y*m._01 + vi_.z*m._02;
	vo.y= vi_.x*m._10 + vi_.y*m._11 + vi_.z*m._12;
	vo.z= vi_.x*m._20 + vi_.y*m._21 + vi_.z*m._22;
}

// Transformation eines Richtungsvektors
// matrix * vector(x,y,z,0)
void VecTransform3(vector &vo,const matrix3 &m,const vector &vi)
{
	vector vi_=vi;
	vo.x= vi_.x*m._00 + vi_.y*m._01 + vi_.z*m._02;
	vo.y= vi_.x*m._10 + vi_.y*m._11 + vi_.z*m._12;
	vo.z= vi_.x*m._20 + vi_.y*m._21 + vi_.z*m._22;
}

// vector(0,0,1) wird um ang rotiert
// ZXY, da nur im Spiel-Koordinaten-System
vector VecAng2Dir(const vector &ang)
{
	return vector(		sinf(ang.y)*cos(ang.x),
					-	sinf(ang.x),
						cosf(ang.y)*cos(ang.x));
}

// um welche Winkel wurde vector(0,0,1) rotiert?
vector VecDir2Ang(const vector &dir)
{
	return vector(	-	atan2f(dir.y,sqrt(dir.x*dir.x+dir.z*dir.z)),
						atan2f(dir.x,dir.z),
						0); // too few information to get z!
}

// um welche Winkel wurde vector(0,0,1) rotiert?
//    
vector VecDir2Ang2(const vector &dir,const vector &up)
{
	vector right=VecCrossProduct(up,dir);
	return vector(	-	atan2f(dir.y,sqrt(dir.x*dir.x+dir.z*dir.z)),
						atan2f(dir.x,dir.z),
						atan2f(right.y,up.y)); // atan2( < up, (0,1,0) >, < right, (0,1,0) > )    where: right = up x dir
/*	// aus den 3 Basis-Vektoren eine Basis-Wechsel-Matrix erzeugen
	matrix m;
	m._00=right.x;	m._01=up.x;	m._02=dir.x;	m._03=0;
	m._10=right.y;	m._11=up.y;	m._12=dir.y;	m._13=0;
	m._20=right.z;	m._21=up.z;	m._22=dir.z;	m._23=0;
	m._30=0;		m._31=0;	m._32=0;		m._33=1;
	quaternion q;
	msg_todo("VecDir2Ang2 fuer OpenGL");
	QuaternionRotationM(q,m);
	msg_db_out(1,"");
	return QuaternionToAngle(q);*/
}

// adds two angles (juxtaposition of rotations)
vector VecAngAdd(const vector &ang1,const vector &ang2)
{
	quaternion q,q1,q2;
	QuaternionRotationV(q1,ang1);
	QuaternionRotationV(q2,ang2);
	QuaternionMultiply(q,q2,q1);
	return QuaternionToAngle(q);
}

vector VecAngInterpolate(const vector &ang1,const vector &ang2,float t)
{
	quaternion q1,q2,r;
	QuaternionRotationV(q1,ang1);
	QuaternionRotationV(q2,ang2);
	QuaternionInterpolate(r,q1,q2,t);
	return QuaternionToAngle(r);
}

// rotate a vector by an angle
vector VecRotate(const vector &v, const vector &ang)
{
	// slow...indirect
	matrix m;
	MatrixRotation(m, ang);
	vector r;
	VecNormalTransform(r, m, v);
	return r;
}

// kuerzeste Entfernung von p zur Geraden(l1,l2)
float VecLineDistance(const vector &p,const vector &l1,const vector &l2)
{
	return (float)sqrt( VecLengthSqr(p-l1) - sqr(VecDotProduct(l2-l1,p-l1))/VecLengthSqr(l2-l1) );
}

// Punkt der Geraden(l1,l2), der p am naechsten ist
vector VecLineNearestPoint(const vector &p,const vector &l1,const vector &l2)
{
	vector n=l2-l1,np;
	VecNormalize(n,n);
	plane pl;
	PlaneFromPointNormal(pl,p,n);
	PlaneIntersectLine(np,pl,l1,l2);
	return np;
}

void VecMin(vector &v,const vector &test_partner)
{
	if (test_partner.x<v.x)	v.x=test_partner.x;
	if (test_partner.y<v.y)	v.y=test_partner.y;
	if (test_partner.z<v.z)	v.z=test_partner.z;
}

void VecMax(vector &v,const vector &test_partner)
{
	if (test_partner.x>v.x)	v.x=test_partner.x;
	if (test_partner.y>v.y)	v.y=test_partner.y;
	if (test_partner.z>v.z)	v.z=test_partner.z;
}

//------------------------------------------------------------------------------------------------//
//                                           matrices                                             //
//------------------------------------------------------------------------------------------------//

#define _ps(a,b,i,j)	(a._e(i,0)*b._e(0,j) + a._e(i,1)*b._e(1,j) + a._e(i,2)*b._e(2,j) + a._e(i,3)*b._e(3,j))

// combining two transformation matrices (first do m1, then m2:   m = m2 * m1 )
void MatrixMultiply(matrix &m,const matrix &m2,const matrix &m1)
{
	// m_ij = (sum k) m2_ik * m1_kj
	matrix _m;
	_m._00=_ps(m2,m1,0,0);	_m._01=_ps(m2,m1,0,1);	_m._02=_ps(m2,m1,0,2);	_m._03=_ps(m2,m1,0,3);
	_m._10=_ps(m2,m1,1,0);	_m._11=_ps(m2,m1,1,1);	_m._12=_ps(m2,m1,1,2);	_m._13=_ps(m2,m1,1,3);
	_m._20=_ps(m2,m1,2,0);	_m._21=_ps(m2,m1,2,1);	_m._22=_ps(m2,m1,2,2);	_m._23=_ps(m2,m1,2,3);
	_m._30=_ps(m2,m1,3,0);	_m._31=_ps(m2,m1,3,1);	_m._32=_ps(m2,m1,3,2);	_m._33=_ps(m2,m1,3,3);
	m=_m;
}

static const float _IdentityMatrix[16]={	1,0,0,0,	0,1,0,0,	0,0,1,0,	0,0,0,1	};

// identity (no transformation: m*v=v)
void MatrixIdentity(matrix &m)
{
	m = _IdentityMatrix;
	//memcpy(&m,&_IdentityMatrix,sizeof(matrix));
	/*
	m._00=1;	m._01=0;	m._02=0;	m._03=0;
	m._10=0;	m._11=1;	m._12=0;	m._13=0;
	m._20=0;	m._21=0;	m._22=1;	m._23=0;
	m._30=0;	m._31=0;	m._32=0;	m._33=1;*/
}

float _Determinant(float m[16])
{
	return
		m[12]*m[9]*m[6]*m[3]-
		m[8]*m[13]*m[6]*m[3]-
		m[12]*m[5]*m[10]*m[3]+
		m[4]*m[13]*m[10]*m[3]+
		m[8]*m[5]*m[14]*m[3]-
		m[4]*m[9]*m[14]*m[3]-
		m[12]*m[9]*m[2]*m[7]+
		m[8]*m[13]*m[2]*m[7]+
		m[12]*m[1]*m[10]*m[7]-
		m[0]*m[13]*m[10]*m[7]-
		m[8]*m[1]*m[14]*m[7]+
		m[0]*m[9]*m[14]*m[7]+
		m[12]*m[5]*m[2]*m[11]-
		m[4]*m[13]*m[2]*m[11]-
		m[12]*m[1]*m[6]*m[11]+
		m[0]*m[13]*m[6]*m[11]+
		m[4]*m[1]*m[14]*m[11]-
		m[0]*m[5]*m[14]*m[11]-
		m[8]*m[5]*m[2]*m[15]+
		m[4]*m[9]*m[2]*m[15]+
		m[8]*m[1]*m[6]*m[15]-
		m[0]*m[9]*m[6]*m[15]-
		m[4]*m[1]*m[10]*m[15]+
		m[0]*m[5]*m[10]*m[15];
}

// inverting the transformation
void MatrixInverse(matrix &mo,const matrix &mi)
{
	float *m=(float*)&mi;
	float *i=(float*)&mo;
	float x=_Determinant(m);

	/*msg_write("Matrix Inverse");
	mout(mi);
	msg_write(f2s(x,3));*/

	if (x==0){
		msg_write("MatrixInverse:  matrix not invertible");
		MatrixIdentity(mo);
		return;
	}

	i[0]= (-m[13]*m[10]*m[7] +m[9]*m[14]*m[7] +m[13]*m[6]*m[11] -m[5]*m[14]*m[11] -m[9]*m[6]*m[15] +m[5]*m[10]*m[15])/x;
	i[4]= ( m[12]*m[10]*m[7] -m[8]*m[14]*m[7] -m[12]*m[6]*m[11] +m[4]*m[14]*m[11] +m[8]*m[6]*m[15] -m[4]*m[10]*m[15])/x;
	i[8]= (-m[12]*m[9]* m[7] +m[8]*m[13]*m[7] +m[12]*m[5]*m[11] -m[4]*m[13]*m[11] -m[8]*m[5]*m[15] +m[4]*m[9]* m[15])/x;
	i[12]=( m[12]*m[9]* m[6] -m[8]*m[13]*m[6] -m[12]*m[5]*m[10] +m[4]*m[13]*m[10] +m[8]*m[5]*m[14] -m[4]*m[9]* m[14])/x;
	i[1]= ( m[13]*m[10]*m[3] -m[9]*m[14]*m[3] -m[13]*m[2]*m[11] +m[1]*m[14]*m[11] +m[9]*m[2]*m[15] -m[1]*m[10]*m[15])/x;
	i[5]= (-m[12]*m[10]*m[3] +m[8]*m[14]*m[3] +m[12]*m[2]*m[11] -m[0]*m[14]*m[11] -m[8]*m[2]*m[15] +m[0]*m[10]*m[15])/x;
	i[9]= ( m[12]*m[9]* m[3] -m[8]*m[13]*m[3] -m[12]*m[1]*m[11] +m[0]*m[13]*m[11] +m[8]*m[1]*m[15] -m[0]*m[9]* m[15])/x;
	i[13]=(-m[12]*m[9]* m[2] +m[8]*m[13]*m[2] +m[12]*m[1]*m[10] -m[0]*m[13]*m[10] -m[8]*m[1]*m[14] +m[0]*m[9]* m[14])/x;
	i[2]= (-m[13]*m[6]* m[3] +m[5]*m[14]*m[3] +m[13]*m[2]*m[7]  -m[1]*m[14]*m[7] -m[5]*m[2]*m[15] +m[1]*m[6]* m[15])/x;
	i[6]= ( m[12]*m[6]* m[3] -m[4]*m[14]*m[3] -m[12]*m[2]*m[7]  +m[0]*m[14]*m[7] +m[4]*m[2]*m[15] -m[0]*m[6]* m[15])/x;
	i[10]=(-m[12]*m[5]* m[3] +m[4]*m[13]*m[3] +m[12]*m[1]*m[7]  -m[0]*m[13]*m[7] -m[4]*m[1]*m[15] +m[0]*m[5]* m[15])/x;
	i[14]=( m[12]*m[5]* m[2] -m[4]*m[13]*m[2] -m[12]*m[1]*m[6]  +m[0]*m[13]*m[6] +m[4]*m[1]*m[14] -m[0]*m[5]* m[14])/x;
	i[3]= ( m[9]* m[6]* m[3] -m[5]*m[10]*m[3] -m[9]* m[2]*m[7]  +m[1]*m[10]*m[7] +m[5]*m[2]*m[11] -m[1]*m[6]* m[11])/x;
	i[7]= (-m[8]* m[6]* m[3] +m[4]*m[10]*m[3] +m[8]* m[2]*m[7]  -m[0]*m[10]*m[7] -m[4]*m[2]*m[11] +m[0]*m[6]* m[11])/x;
	i[11]=( m[8]* m[5]* m[3] -m[4]*m[9]* m[3] -m[8]* m[1]*m[7]  +m[0]*m[9]* m[7] +m[4]*m[1]*m[11] -m[0]*m[5]* m[11])/x;
	i[15]=(-m[8]* m[5]* m[2] +m[4]*m[9]* m[2] +m[8]* m[1]*m[6]  -m[0]*m[9]* m[6] -m[4]*m[1]*m[10] +m[0]*m[5]* m[10])/x;
}

// transposes a matrix
void MatrixTranspose(matrix &mo,const matrix &mi)
{
	matrix _m;
	_m._00=mi._00;	_m._01=mi._10;	_m._02=mi._20;	_m._03=mi._30;
	_m._10=mi._01;	_m._11=mi._11;	_m._12=mi._21;	_m._13=mi._31;
	_m._20=mi._02;	_m._21=mi._12;	_m._22=mi._22;	_m._23=mi._32;
	_m._30=mi._03;	_m._31=mi._13;	_m._32=mi._23;	_m._33=mi._33;
	mo=_m;
}

// translation by a vector ( m * v = v + t )
void MatrixTranslation(matrix &m,const vector &t)
{
	m._00=1;	m._01=0;	m._02=0;	m._03=t.x;
	m._10=0;	m._11=1;	m._12=0;	m._13=t.y;
	m._20=0;	m._21=0;	m._22=1;	m._23=t.z;
	m._30=0;	m._31=0;	m._32=0;	m._33=1;
}

// Rotation um die X-Achse (nach unten)
void MatrixRotationX(matrix &m,float w)
{
	float sw=sinf(w);
	float cw=cosf(w);
	m._00=1;	m._01=0;	m._02=0;	m._03=0;
	m._10=0;	m._11=cw;	m._12=-sw;	m._13=0;
	m._20=0;	m._21=sw;	m._22=cw;	m._23=0;
	m._30=0;	m._31=0;	m._32=0;	m._33=1;
}

// Rotation um die Y-Achse (nach rechts)
void MatrixRotationY(matrix &m,float w)
{
	float sw=sinf(w);
	float cw=cosf(w);
	m._00=cw;	m._01=0;	m._02=sw;	m._03=0;
	m._10=0;	m._11=1;	m._12=0;	m._13=0;
	m._20=-sw;	m._21=0;	m._22=cw;	m._23=0;
	m._30=0;	m._31=0;	m._32=0;	m._33=1;
}

// Rotation um die Z-Achse (gegen den Uhrzeigersinn)
void MatrixRotationZ(matrix &m,float w)
{
	float sw=sinf(w);
	float cw=cosf(w);
	m._00=cw;	m._01=-sw;	m._02=0;	m._03=0;
	m._10=sw;	m._11=cw;	m._12=0;	m._13=0;
	m._20=0;	m._21=0;	m._22=1;	m._23=0;
	m._30=0;	m._31=0;	m._32=0;	m._33=1;
}

// ZXY -> fuer alles IM Spiel
void MatrixRotation(matrix &m,const vector &ang)
{
	/*matrix x,y,z;
	MatrixRotationX(x,ang.x);
	MatrixRotationY(y,ang.y);
	MatrixRotationZ(z,ang.z);
	// m=y*x*z
	MatrixMultiply(m,y,x);
	MatrixMultiply(m,m,z);*/
	float sx=sinf(ang.x);
	float cx=cosf(ang.x);
	float sy=sinf(ang.y);
	float cy=cosf(ang.y);
	float sz=sinf(ang.z);
	float cz=cosf(ang.z);
	m._00= sx*sy*sz + cy*cz;	m._01= sx*sy*cz - cy*sz;	m._02= cx*sy;	m._03=0;
	m._10= cx*sz;				m._11= cx*cz;				m._12=-sx;		m._13=0;
	m._20= sx*cy*sz - sy*cz;	m._21= sx*cy*cz + sy*sz;	m._22= cx*cy;	m._23=0;
	m._30= 0;					m._31= 0;					m._32=0;		m._33=1;
}

// YXZ -> fuer Kamera-Rotationen
// ist die Inverse zu MatrixRotation!!
void MatrixRotationView(matrix &m,const vector &ang)
{
	/*matrix x,y,z;
	MatrixRotationX(x,-ang.x);
	MatrixRotationY(y,-ang.y);
	MatrixRotationZ(z,-ang.z);
	// z*x*y
	MatrixMultiply(m,z,x);
	MatrixMultiply(m,m,y);*/
	float sx=sinf(ang.x);
	float cx=cosf(ang.x);
	float sy=sinf(ang.y);
	float cy=cosf(ang.y);
	float sz=sinf(ang.z);
	float cz=cosf(ang.z);
	// the transposed (=inverted) of MatrixView
	m._00= sx*sy*sz + cy*cz;	m._01= cx*sz;	m._02= sx*cy*sz - sy*cz;	m._03=0;
	m._10= sx*sy*cz - cy*sz;	m._11= cx*cz;	m._12= sx*cy*cz + sy*sz;	m._13=0;
	m._20= cx*sy;				m._21=-sx;		m._22= cx*cy;				m._23=0;
	m._30= 0;					m._31= 0;		m._32=0;					m._33=1;
}

// Rotations-Matrix aus Quaternion erzeugen
void MatrixRotationQ(matrix &m,const quaternion &q)
{
	m._00=1-2*q.y*q.y-2*q.z*q.z;	m._01=  2*q.x*q.y-2*q.w*q.z;	m._02=  2*q.x*q.z+2*q.w*q.y;	m._03=0;
	m._10=  2*q.x*q.y+2*q.w*q.z;	m._11=1-2*q.x*q.x-2*q.z*q.z;	m._12=  2*q.y*q.z-2*q.w*q.x;	m._13=0;
	m._20=  2*q.x*q.z-2*q.w*q.y;	m._21=  2*q.y*q.z+2*q.w*q.x;	m._22=1-2*q.x*q.x-2*q.y*q.y;	m._23=0;
	m._30=0;						m._31=0;						m._32=0;						m._33=1;
	/* [ 1 - 2y2 - 2z2        2xy - 2wz        2xz + 2wy
	         2xy + 2wz    1 - 2x2 - 2z2        2yz - 2wx
		     2xz - 2wy        2yz + 2wx    1 - 2x2 - 2y2 ] */
}

// scale orthogonally in 3 dimensions
void MatrixScale(matrix &m,float fx,float fy,float fz)
{
	m._00=fx;	m._01=0;	m._02=0;	m._03=0;
	m._10=0;	m._11=fy;	m._12=0;	m._13=0;
	m._20=0;	m._21=0;	m._22=fz;	m._23=0;
	m._30=0;	m._31=0;	m._32=0;	m._33=1;
}

// create a transformation that reflects at a <plane pl>
void MatrixReflect(matrix &m,const plane &pl)
{
	vector n=vector(pl.a,pl.b,pl.c);
	vector p=-n*pl.d;
	// mirror: matrix s from transforming the basis vectors:
	//    e_i' = e_i - 2 < n, e_i >
	//     or thinking of it as a tensor product (projection): s = id - 2n (x) n
	// translation by p: t_p
	// complete reflection is: r = t_p * s * t_(-p) = t_(2p) * s
	m._00=1-2*n.x*n.x;	m._01= -2*n.y*n.x;	m._02= -2*n.z*n.x;	m._03=2*p.x;
	m._10= -2*n.x*n.y;	m._11=1-2*n.y*n.y;	m._12= -2*n.z*n.y;	m._13=2*p.y;
	m._20= -2*n.x*n.z;	m._21= -2*n.y*n.z;	m._22=1-2*n.z*n.z;	m._23=2*p.z;
	m._30=0;			m._31=0;			m._32=0;			m._33=1;
	//msg_todo("TestMe: MatrixReflect");
}


//------------------------------------------------------------------------------------------------//
//                                          3x3-matrices                                          //
//------------------------------------------------------------------------------------------------//

void Matrix3Identity(matrix3 &m)
{
	m._00=1;	m._01=0;	m._02=0;
	m._10=0;	m._11=1;	m._12=0;
	m._20=0;	m._21=0;	m._22=1;
}

void Matrix3Inverse(matrix3 &mo,const matrix3 &mi)
{
	float det=   mi._00*mi._11*mi._22 + mi._01*mi._12*mi._20 + mi._02*mi._10*mi._21
				-mi._02*mi._11*mi._20 - mi._00*mi._12*mi._21 - mi._01*mi._10*mi._22;

	float *m=(float*)&mi;

	if (det==0){
		msg_write("Matrix3Inverse:  matrix not invertible");
		Matrix3Identity(mo);
		return;
	}
	float idet=1.0f/det;

	mo._00= ( -mi._12*mi._21 +mi._11*mi._22 )*idet;
	mo._01= ( +mi._02*mi._21 -mi._01*mi._22 )*idet;
	mo._02= ( -mi._02*mi._11 +mi._01*mi._12 )*idet;

	mo._10= ( +mi._12*mi._20 -mi._10*mi._22 )*idet;
	mo._11= ( -mi._02*mi._20 +mi._00*mi._22 )*idet;
	mo._12= ( +mi._02*mi._10 -mi._00*mi._12 )*idet;

	mo._20= ( -mi._11*mi._20 +mi._10*mi._21 )*idet;
	mo._21= ( +mi._01*mi._20 -mi._00*mi._21 )*idet;
	mo._22= ( -mi._01*mi._10 +mi._00*mi._11 )*idet;
}

void Matrix3Transpose(matrix3 &mo,const matrix3 &mi)
{
	matrix3 _m;
	_m._00=mi._00;	_m._01=mi._10;	_m._02=mi._20;
	_m._10=mi._01;	_m._11=mi._11;	_m._12=mi._21;
	_m._20=mi._02;	_m._21=mi._12;	_m._22=mi._22;
	mo=_m;
}

void Matrix3Rotation(matrix3 &m,const vector &ang)
{
	float sx=sinf(ang.x);
	float cx=cosf(ang.x);
	float sy=sinf(ang.y);
	float cy=cosf(ang.y);
	float sz=sinf(ang.z);
	float cz=cosf(ang.z);
	m._00= sx*sy*sz + cy*cz;	m._01= sx*sy*cz - cy*sz;	m._02= cx*sy;
	m._10= cx*sz;				m._11= cx*cz;				m._12=-sx;
	m._20= sx*cy*sz - sy*cz;	m._21= sx*cy*cz + sy*sz;	m._22= cx*cy;
}

void Matrix3RotationQ(matrix3 &m,const quaternion &q)
{
	m._00=1-2*q.y*q.y-2*q.z*q.z;	m._01=  2*q.x*q.y-2*q.w*q.z;	m._02=  2*q.x*q.z+2*q.w*q.y;
	m._10=  2*q.x*q.y+2*q.w*q.z;	m._11=1-2*q.x*q.x-2*q.z*q.z;	m._12=  2*q.y*q.z-2*q.w*q.x;
	m._20=  2*q.x*q.z-2*q.w*q.y;	m._21=  2*q.y*q.z+2*q.w*q.x;	m._22=1-2*q.x*q.x-2*q.y*q.y;
}

//------------------------------------------------------------------------------------------------//
//                                          quaternions                                           //
//------------------------------------------------------------------------------------------------//

void QuaternionIdentity(quaternion &q)
{
	q.w=1;
	q.x=q.y=q.z=0;
}

// rotation with an <angle w> and an <axis axis>
void QuaternionRotationA(quaternion &q,const vector &axis,float w)
{
	float w_half=w*0.5f;
	float s=sinf(w_half);
	q.w=cosf(w_half);
	q.x=axis.x*s;
	q.y=axis.y*s;
	q.z=axis.z*s;
}

// ZXY -> everything IN the game (world transformation)
void QuaternionRotationV(quaternion &q,const vector &ang)
{
	float wx_2=ang.x*0.5f;
	float wy_2=ang.y*0.5f;
	float wz_2=ang.z*0.5f;
	float cx=cosf(wx_2);
	float cy=cosf(wy_2);
	float cz=cosf(wz_2);
	float sx=sinf(wx_2);
	float sy=sinf(wy_2);
	float sz=sinf(wz_2);
	q.w=(cy*cx*cz) + (sy*sx*sz);
	q.x=(cy*sx*cz) + (sy*cx*sz);
	q.y=(sy*cx*cz) - (cy*sx*sz);
	q.z=(cy*cx*sz) - (sy*sx*cz);

	/*quaternion x,y,z;
	QuaternionRotationA(x,vector(1,0,0),ang.x);
	QuaternionRotationA(y,vector(0,1,0),ang.y);
	QuaternionRotationA(z,vector(0,0,1),ang.z);
	// y*x*z
	QuaternionMultiply(q,x,z);
	QuaternionMultiply(q,y,q);*/
}

// create a quaternion from a (rotation-) matrix
void QuaternionRotationM(quaternion &q,const matrix &m)
{
	float tr=m._00+m._11+m._22;
	float w=acosf((tr-1)/2);
	if ((w<0.00000001f)&&(w>-0.00000001f))
		QuaternionIdentity(q);
	else{
		float s=0.5f/sinf(w);
		vector n;
		n.x=(m._21-m._12)*s;
		n.y=(m._02-m._20)*s;
		n.z=(m._10-m._01)*s;
		VecNormalize(n,n);
		QuaternionRotationA(q,n,w);
	}
}

// invert a quaternion rotation
void QuaternionInverse(quaternion &qo,const quaternion &qi)
{
	float l=(qi.x*qi.x)+(qi.y*qi.y)+(qi.z*qi.z)+(qi.w*qi.w);
	l=1.0f/l;
	qo.w= qi.w*l;
	qo.x=-qi.x*l;
	qo.y=-qi.y*l;
	qo.z=-qi.z*l;
}

// unite 2 rotations (first rotate by q1, then by q2: q = q2*q1)
void QuaternionMultiply(quaternion &q,const quaternion &q2,const quaternion &q1)
{
	quaternion _q;
	_q.w = q2.w*q1.w - q2.x*q1.x - q2.y*q1.y - q2.z*q1.z;
	_q.x = q2.w*q1.x + q2.x*q1.w + q2.y*q1.z - q2.z*q1.y;
	_q.y = q2.w*q1.y + q2.y*q1.w + q2.z*q1.x - q2.x*q1.z;
	_q.z = q2.w*q1.z + q2.z*q1.w + q2.x*q1.y - q2.y*q1.x;
	q=_q;
}

// q = q1 + t*( q2 - q1)
void QuaternionInterpolate(quaternion &q,const quaternion &q1,const quaternion &q2,float t)
{
	//msg_todo("TestMe: QuaternionInterpolate(2q) for OpenGL");
	q=q1;

	t=1-t; // ....?

	// dot product = cos angle(q1,q2)
	float c = q1.x*q2.x + q1.y*q2.y + q1.z*q2.z + q1.w*q2.w;
	float t2;
	bool flip=false;
	// flip, if q1 and q2 on opposite hemispheres
	if (c<0){
		c=-c;
		flip=true;
	}
	// q1 and q2 "too equal"?
	if (c>0.9999f)
		t2=1.0f-t;
	else{
		float theta=acosf(c);
		float phi=theta;//+spin*pi; // spin for additional circulations...
		float s=sinf(theta);
		t2=sinf(theta-t*phi)/s;
		t=sinf(t*phi)/s;
	}
	if (flip)
		t=-t;

	q.x = t*q1.x + t2*q2.x;
	q.y = t*q1.y + t2*q2.y;
	q.z = t*q1.z + t2*q2.z;
	q.w = t*q1.w + t2*q2.w;
}

void QuaternionInterpolate(quaternion &q,const quaternion &q1,const quaternion &q2,const quaternion &q3,const quaternion &q4,float t)
{
	/*#ifdef NIX_TYPES_BY_DIRECTX9
		D3DXQUATERNION A,B,C;
		D3DXQuaternionSquadSetup(&A,&B,&C,(D3DXQUATERNION*)&q1,(D3DXQUATERNION*)&q2,(D3DXQUATERNION*)&q3,(D3DXQUATERNION*)&q4);
		D3DXQuaternionSquad((D3DXQUATERNION*)&q,(D3DXQUATERNION*)&q2,&A,&B,&C,t);
	#else*/
	q=q2;
	msg_todo("QuaternionInterpolate(4q)");
}

// convert a quaternion into 3 angles (ZXY)
vector QuaternionToAngle(const quaternion &q)
{
	// really bad!
	vector ang,v;
	v=vector(0,0,1000.0f);
	matrix m,x,y;
	MatrixRotationQ(m,q);
	VecTransform(v,m,v);
	ang.y= atan2f(v.x,v.z);
	ang.x=-atan2f(v.y,sqrt(v.x*v.x+v.z*v.z));
	MatrixRotationX(x,-ang.x);
	MatrixRotationY(y,-ang.y);
	MatrixMultiply(m,y,m);
	MatrixMultiply(m,x,m);
	v=vector(1000.0f,0,0);
	VecTransform(v,m,v);
	ang.z=atan2f(v.y,v.x);
	return ang;
}

// scale the angle of the rotation
void QuaternionScale(quaternion &q,float f)
{
	float w=GetAngle(q);
	if (w==0)	return;

	q.w=cosf(w*f/2);
	float factor=sinf(w*f/2)/sinf(w/2);
	q.x=q.x*factor;
	q.y=q.y*factor;
	q.z=q.z*factor;
}

// quaternion correction
void QuaternionNormalize(quaternion &qo,const quaternion &qi)
{
	float l=sqrtf((qi.x*qi.x)+(qi.y*qi.y)+(qi.z*qi.z)+(qi.w*qi.w));
	l=1.0f/l;
	qo.x=qi.x*l;
	qo.y=qi.y*l;
	qo.z=qi.z*l;
	qo.w=qi.w*l;
}

// the axis of our quaternion rotation
vector GetAxis(const quaternion &q)
{
	vector ax=vector(q.x,q.y,q.z);
	VecNormalize(ax,ax);
	return ax;
}

// angle value of the quaternion
float GetAngle(const quaternion &q)
{
	return acosf(q.w)*2;
}

//------------------------------------------------------------------------------------------------//
//                                             planes                                             //
//------------------------------------------------------------------------------------------------//

// plane containing a, b, c
void PlaneFromPoints(plane &pl,const vector &a,const vector &b,const vector &c)
{
	vector ba=b-a,ca=c-a,n;
	n=VecCrossProduct(ba,ca);
	VecNormalize(n,n);
	pl.a=n.x;	pl.b=n.y;	pl.c=n.z;
	pl.d=-(n*a);
}

// plane containing p an having the normal vector n
void PlaneFromPointNormal(plane &pl,const vector &p,const vector &n)
{
	pl.a=n.x;	pl.b=n.y;	pl.c=n.z;
	pl.d=-(n*p);
}

// rotate and move a plane with a matrix
//    please don't scale...
void PlaneTransform(plane &plo,const matrix &m,const plane &pli)
{
	// transform the normalvector  (n' = R n)
	plo.a= pli.a*m._00 + pli.b*m._01 + pli.c*m._02;
	plo.b= pli.a*m._10 + pli.b*m._11 + pli.c*m._12;
	plo.c= pli.a*m._20 + pli.b*m._21 + pli.c*m._22;
	// offset (d' = d - < T, n' >)
	plo.d= pli.d - plo.a*m._03 - plo.b*m._13 - plo.c*m._23;
}

// return the normal vector of a plane
vector GetNormal(const plane &pl)
{
	vector n=vector(pl.a,pl.b,pl.c);
	return n;
}

// intersection of plane <pl> and the line through l1 and l2
// (false if parallel!)
bool PlaneIntersectLine(vector &i,const plane &pl,const vector &l1,const vector &l2)
{
	vector n=vector(pl.a,pl.b,pl.c);
	float d=-pl.d;
	float e=VecDotProduct(n,l1);
	float f=VecDotProduct(n,l2);
	if (e==f) // parallel?
		return false;
	float t=(d-f)/(e-f);
	//if ((t>=0)&&(t<=1)){
		//i = l1 + t*(l2-l1);
		i = l2 + t*(l1-l2);
		return true;
}

// reflect the plane on itself
void PlaneInverse(plane &pl)
{
	pl.a=-pl.a;
	pl.b=-pl.b;
	pl.c=-pl.c;
}

// which one is the largest coordinate of this vector
int ImportantPlane(const vector &v)
{
	vector w;
	w.x=fabs(v.x);
	w.y=fabs(v.y);
	w.z=fabs(v.z);
	if ((w.x<=w.y)&&(w.x<=w.z))
		return 1;	// Y-Z-Ebene
	if (w.y<=w.z)
		return 2;	// X-Z-Ebene
	return 3;		// X-Y-Ebene
}

// P = A + f*( B - A ) + g*( C - A )
void GetBaryCentric(const vector &P,const vector &A,const vector &B,const vector &C,float &f,float &g)
{
	// Bezugs-System: A
	vector ba=B-A,ca=C-A,dir;
	plane pl;
	PlaneFromPoints(pl,A,B,C); // Ebene des Dreiecks
	dir=GetNormal(pl);//vector(pl.a,pl.b,pl.c); // Normalen-Vektor
	vector pvec;
	pvec=VecCrossProduct(dir,ca); // Laenge: |ca|         Richtung: Dreiecks-Ebene, orth zu ca
	float det=VecDotProduct(ba,pvec); // = |ba| * |ca| * cos( pvec->ba )   -> =Flaeche des Parallelogramms
	vector pa;
	if (det>0)
		pa=P-A;
	else
	{
		pa=A-P;
		det=-det;
	}
	f=VecDotProduct(pa,pvec);
	vector qvec;
	qvec=VecCrossProduct(pa,ba);
	g=VecDotProduct(dir,qvec);
	//float t=VecDotProduct(ca,qvec);
	float InvDet=1.0f/det;
	//t*=InvDet;
	f*=InvDet;
	g*=InvDet;
}

// wird das Dreieck(t1,t2,t3) von der Geraden(l1,l2) geschnitten?
// Schnittpunkt = col
bool LineIntersectsTriangle(const vector &t1,const vector &t2,const vector &t3,const vector &l1,const vector &l2,vector &col,bool vm)
{
	plane p;
	PlaneFromPoints(p,t1,t2,t3);
	if (!PlaneIntersectLine(col,p,l1,l2))
		return false;
	GetBaryCentric(col,t1,t2,t3,LineIntersectsTriangleF,LineIntersectsTriangleG);
	if ((LineIntersectsTriangleF>0)&&(LineIntersectsTriangleG>0)&&(LineIntersectsTriangleF+LineIntersectsTriangleG<1))
		return true;
	return false;
}

// distance <point p> to <plane pl>
float PlaneDistance(const plane &pl,const vector &p)
{
	return pl.a*p.x + pl.b*p.y + pl.c*p.z + pl.d;
}

//------------------------------------------------------------------------------------------------//
//                                             colors                                             //
//------------------------------------------------------------------------------------------------//

// create a color from (alpha, red, green blue)
// (values of set [0..1])
color SetColorSave(float a,float r,float g, float b)
{
	if (a<0)	a=0;	if (a>1)	a=1;
	if (r<0)	r=0;	if (r>1)	r=1;
	if (g<0)	g=0;	if (g>1)	g=1;
	if (b<0)	b=0;	if (b>1)	b=1;
	color c;
	c.a=a;	c.r=r;	c.g=g;	c.b=b;
	return c;
}

color SetColorHSB(float a,float hue,float saturation,float brightness)
{
	int h=int(hue*6)%6;
	float f=hue*6.0f-int(hue*6);
	float p=brightness*(1-saturation);
	float q=brightness*(1-saturation*f);
	float t=brightness*(1-saturation*(1-f));
	color c;
	if (h==0)	c=color(a,brightness,t,p);
	if (h==1)	c=color(a,q,brightness,p);
	if (h==2)	c=color(a,p,brightness,t);
	if (h==3)	c=color(a,p,q,brightness);
	if (h==4)	c=color(a,t,p,brightness);
	if (h==5)	c=color(a,brightness,p,q);
	return c;
}

// scale the elements of a color
color ColorScale(const color &c,float f)
{
	return color(c.a*f,c.r*f,c.g*f,c.b*f);
}

// create a mixed color = a * (1-t)  +  b * t
color ColorInterpolate(const color &a,const color &b,float t)
{
	return color(	a.a*(1-t)+b.a*t,
						a.r*(1-t)+b.r*t,
						a.g*(1-t)+b.g*t,
						a.b*(1-t)+b.b*t	);
}

color ColorMultiply(const color &a,const color &b)
{
	return color(	a.a*b.a,
						a.r*b.r,
						a.g*b.g,
						a.b*b.b	);
}

color ColorFromIntRGB(int *i)
{
	return color(	1,
					(float)i[0]/255.0f,
					(float)i[1]/255.0f,
					(float)i[2]/255.0f);
}

color ColorFromIntARGB(int *i)
{
	return color(	(float)i[0]/255.0f,
					(float)i[1]/255.0f,
					(float)i[2]/255.0f,
					(float)i[3]/255.0f);
}

void Color2IntRGB(const color &c,int *i)
{
	i[0]=int(c.r*255.0f);
	i[1]=int(c.g*255.0f);
	i[2]=int(c.b*255.0f);
}

void Color2IntARGB(const color &c,int *i)
{
	i[0]=int(c.a*255.0f);
	i[1]=int(c.r*255.0f);
	i[2]=int(c.g*255.0f);
	i[3]=int(c.b*255.0f);
}




