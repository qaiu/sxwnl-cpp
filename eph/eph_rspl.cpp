#include <cmath>
#include "eph.h"
#include "eph0.h"
#include "eph_rsgs.h"
#include "eph_rspl.h"
#include "../tool.h"

bool rsPL::nasa_r = 0;//为1表示采用NASA的视径比
std::array<double, 5> rsPL::sT;//地方日食时间表
std::string rsPL::LX;
double rsPL::sf;
double rsPL::b1;
double rsPL::dur;
double rsPL::sun_s;
double rsPL::sun_j;
double rsPL::P1;
double rsPL::V1;
double rsPL::P2;
double rsPL::V2;

std::array<double,3> rsPL::A={};
std::array<double,3> rsPL::B={}; //本半影锥顶点坐标
_ZB rsPL::P={};//t1时刻的日月坐标,g为恒星时
_ZB rsPL::Q={};//t2时刻的日月坐标
std::array<double,10> rsPL::V;//食界表
std::string rsPL::Vc = "";
std::string rsPL::Vb = "";  //食中心类型,本影南北距离

void rsPL::secXY(double jd,double L,double fa,double high,_SECXY &re)
{ //日月xy坐标计算。参数：jd是力学时,站点经纬L,fa,海拔high(千米)
  //基本参数计算
  double deltat = dt_T(jd); //TD-UT
  std::array<double,2> zd=nutation2(jd/36525.0);
  double gst= pGST(jd-deltat,deltat) + zd[0]*cos(hcjj(jd/36525.0) + zd[1]); //真恒星时(不考虑非多项式部分)

  std::array<double,3> z;
  //=======月亮========
  z=rsGS::moon(jd); re.mCJ=z[0]; re.mCW=z[1]; re.mR=z[2]; //月亮视赤经,月球赤纬
  double mShiJ = rad2rrad(gst + L - z[0]); //得到此刻月亮时角
  z=parallax(z, mShiJ,fa, high); re.mCJ2=z[0], re.mCW2=z[1], re.mR2=z[2]; //修正了视差的赤道坐标

  //=======太阳========
  z=rsGS::sun(jd); re.sCJ=z[0]; re.sCW=z[1]; re.sR=z[2]; //太阳视赤经,太阳赤纬
  double sShiJ = rad2rrad(gst + L - z[0]); //得到此刻太阳时角
  z=parallax(z,sShiJ,fa,high); re.sCJ2=z[0], re.sCW2=z[1], re.sR2=z[2]; //修正了视差的赤道坐标

  //=======视半径========
  re.mr = cs_sMoon/re.mR2/rad;
  re.sr = 959.63/re.sR2/rad*cs_AU;
  if(rsPL::nasa_r) re.mr*=cs_sMoon2/cs_sMoon; //0.99925;
  //=======日月赤经纬差转为日面中心直角坐标(用于日食)==============
  re.x = rad2rrad(re.mCJ2-re.sCJ2) * cos((re.mCW2+re.sCW2)/2.0);
  re.y = re.mCW2-re.sCW2;
  re.t = jd;
}

double rsPL::lineT(_SECXY G, double v,double u, double r, bool n)
{//已知t1时刻星体位置、速度，求x*x+y*y=r*r时,t的值
  double b=G.y*v-G.x*u;
  double A=u*u+v*v;
  double B=u*b;
  double C=b*b-r*r*v*v;
  double D=B*B-A*C;
  if(D<0) return 0;
  D=sqrt(D); if(!n) D=-D;
  return G.t+((-B+D)/A-G.x)/v;
}

void rsPL::secMax(double jd,double L,double fa,double high)
{ //日食的食甚计算(jd为近朔的力学时,误差几天不要紧)
  int i;
  for(i=0;i<5;i++) rsPL::sT[i]=0; //分别是:食甚,初亏,复圆,食既,生光
  rsPL::LX=""; //类型
  rsPL::sf=0;  //食分
  rsPL::b1=1.0;  //月日半径比(食甚时刻)
  rsPL::dur = 0; //持续时间
  rsPL::P1 = rsPL::V1 = 0;  //初亏方位,P北点起算,V顶点起算
  rsPL::P2 = rsPL::V2 = 0;  //复圆方位,P北点起算,V顶点起算
  rsPL::sun_s = rsPL::sun_j = 0; //日出日没
  
  rsGS::init(jd,7);
  jd=rsGS::Zjd; //食甚初始估值为插值表中心时刻(粗朔)

  _SECXY G={}, g={};
  rsPL::secXY(jd,L,fa,high,G);
  jd -= G.x/0.2128; //与食甚的误差在20分钟以内

  double u,v,dt=60/86400.0,dt2;
  for (i=0;i<2;i++)
  {
   rsPL::secXY(jd,L,fa,high,G);
   rsPL::secXY(jd+dt,L,fa,high,g);
   u = (g.y-G.y)/dt;
   v = (g.x-G.x)/dt;
   dt2 = -(G.y*u+G.x*v)/(u*u+v*v);
   jd += dt2; //极值时间
   }

  //求直线到太阳中心的最小值
  double x=G.x+dt2*v, y=G.y+dt2*u, rmin=sqrt(x*x+y*y);

  if(rmin<=G.mr+G.sr)
  { //偏食计算
   rsPL::sT[1] = jd; //食甚
   rsPL::LX="偏";
   rsPL::sf=(G.mr+G.sr-rmin)/G.sr/2.0; //食分
   rsPL::b1=G.mr/G.sr;

   rsPL::sun_s = sunShengJ(jd-dt_T(jd)+L/pi2,L,fa,-1); //日出
   rsPL::sun_j = sunShengJ(jd-dt_T(jd)+L/pi2,L,fa, 1); //日没

   rsPL::sT[0] = rsPL::lineT(G,v,u, G.mr+G.sr, 0); //初亏
   for(i=0;i<3;i++)
   { //初亏再算3次
    rsPL::secXY(rsPL::sT[0],L,fa,high,g);
    rsPL::sT[0] = rsPL::lineT(g,v,u, g.mr+g.sr, 0);
   }

   rsPL::P1 = rad2mrad(atan2(g.x,g.y)); //初亏位置角
   rsPL::V1 = rad2mrad(rsPL::P1-shiChaJ(pGST2(rsPL::sT[0]),L,fa,g.sCJ,g.sCW)); //这里g.sCJ与g.sCW对应的时间与sT[0]还差了一点，所以有一小点误差，不采用真恒星时也误差一点

   rsPL::sT[2] = rsPL::lineT(G,v,u, G.mr+G.sr, 1); //复圆
   for(i=0;i<3;i++)
   { //复圆再算3次
    rsPL::secXY(rsPL::sT[2],L,fa,high,g);
    rsPL::sT[2] = rsPL::lineT(g,v,u, g.mr+g.sr, 1);
   }
   rsPL::P2 = rad2mrad(atan2(g.x,g.y));
   rsPL::V2 = rad2mrad(rsPL::P2-shiChaJ(pGST2(rsPL::sT[2]),L,fa,g.sCJ,g.sCW)); //这里g.sCJ与g.sCW对应的时间与sT[2]还差了一点，所以有一小点误差，不采用真恒星时也误差一点
  }
  if(rmin<=G.mr-G.sr){ //全食计算
   rsPL::LX="全";
   rsPL::sT[3] = rsPL::lineT(G,v,u, G.mr-G.sr, 0); //食既
   rsPL::secXY(rsPL::sT[3],L,fa,high,g);
   rsPL::sT[3] = rsPL::lineT(g,v,u, g.mr-g.sr, 0); //食既再算1次

   rsPL::sT[4] = rsPL::lineT(G,v,u, G.mr-G.sr, 1); //生光
   rsPL::secXY(rsPL::sT[4],L,fa,high,g);
   rsPL::sT[4] = rsPL::lineT(g,v,u, g.mr-g.sr, 1); //生光再算1次
   rsPL::dur = rsPL::sT[4]-rsPL::sT[3];
  }
  if(rmin<=G.sr-G.mr){ //环食计算
   rsPL::LX="环";
   rsPL::sT[3] = rsPL::lineT(G,v,u, G.sr-G.mr, 0); //食既
   rsPL::secXY(rsPL::sT[3],L,fa,high,g);
   rsPL::sT[3] = rsPL::lineT(g,v,u, g.sr-g.mr, 0); //食既再算1次

   rsPL::sT[4] = rsPL::lineT(G,v,u, G.sr-G.mr, 1); //生光
   rsPL::secXY(rsPL::sT[4],L,fa,high,g);
   rsPL::sT[4] = rsPL::lineT(g,v,u, g.sr-g.mr, 1); //生光再算1次
   rsPL::dur = rsPL::sT[4]-rsPL::sT[3];
  }
}

 //以下涉及南北界计算
 std::array<double,3> A={}, B={}; //本半影锥顶点坐标
 _ZB P={};//t1时刻的日月坐标,g为恒星时
 _ZB Q={};//t2时刻的日月坐标
 std::array<double,10> V;//食界表
 std::string Vc = "", Vb = "";  //食中心类型,本影南北距离

void rsPL::zb0(double jd)
{
  //基本参数计算
  double deltat = dt_T(jd); //TD-UT
  double E=hcjj(jd/365251.0);
  std::array<double,2> zd=nutation2(jd/36525.0);
  
  rsPL::P.g = pGST(jd-deltat, deltat) + zd[0]*cos(E+zd[1]); //真恒星时(不考虑非多项式部分)
  rsPL::P.S=rsGS::sun(jd);
  rsPL::P.M=rsGS::moon(jd);

  double t2=jd+60/86400.0;
  rsPL::Q.g = pGST(t2-deltat,deltat) + zd[0]*cos(E+zd[1]);
  rsPL::Q.S=rsGS::sun(t2);
  rsPL::Q.M=rsGS::moon(t2);

  //转为直角坐标
  std::array<double,3> z1={}, z2={};
  z1 = llr2xyz(rsPL::P.S);
  z2 = llr2xyz(rsPL::P.M);

  double k=959.63/cs_sMoon*cs_AU; //k为日月半径比
  //本影锥顶点坐标计算
  std::array<double,3> F = {
   (z1[0]-z2[0])/(1-k)+z2[0],
   (z1[1]-z2[1])/(1-k)+z2[1],
   (z1[2]-z2[2])/(1-k)+z2[2]};
  rsPL::A = xyz2llr(F);
  //半影锥顶点坐标计算
  std::array<double,3> FF = {
   (z1[0]-z2[0])/(1+k)+z2[0],
   (z1[1]-z2[1])/(1+k)+z2[1],
   (z1[2]-z2[2])/(1+k)+z2[2]};
  rsPL::B = xyz2llr(FF);
 }

void rsPL::zbXY(_ZB &p,double L,double fa)
{
  std::array<double,3> s = {p.S[0],p.S[1],p.S[2]};
  std::array<double,3> m = {p.M[0],p.M[1],p.M[2]};
  s=parallax(s, p.g+L-p.S[0],fa, 0); //修正了视差的赤道坐标
  m=parallax(m, p.g+L-p.M[0],fa, 0); //修正了视差的赤道坐标
  //=======视半径========
  p.mr = cs_sMoon/m[2]/rad;
  p.sr = 959.63/s[2]/rad*cs_AU;
  //=======日月赤经纬差转为日面中心直角坐标(用于日食)==============
  p.x = rad2rrad(m[0]-s[0]) * cos((m[1]+s[1])/2.0);
  p.y = m[1]-s[1];
}
 
void rsPL::p2p(double L,double fa,_GJW &re,bool fAB,int f)
{ //f取+-1
  _ZB &p=rsPL::P, &q=rsPL::Q;
  rsPL::zbXY(rsPL::P,L,fa);
  rsPL::zbXY(rsPL::Q,L,fa);

  double u=q.y-p.y, v=q.x-p.x, a=sqrt(u*u+v*v),r=959.63/p.S[2]/rad*cs_AU;
  double W=p.S[1]+f*r*v/a, J=p.S[0]-f*r*u/a/cos((W+p.S[1])/2.0), R=p.S[2];
  std::array<double,3> AA = fAB ? rsPL::A : rsPL::B;
  
  COORDP pp = lineEar({J,W,R}, AA, p.g );
  re.J = pp.J;
  re.W = pp.W;
 }
void rsPL::pp0(_GJW &re)
 { //食中心点计算
  _ZB p=rsPL::P;
  COORDP pp = lineEar( p.M, p.S, p.g );
  re.J = pp.J;
  re.W = pp.W; //无解返回值是100
  
  if(re.W==100) { re.c = ""; return; }
  re.c="全";
  rsPL::zbXY(p,re.J,re.W);
  if(p.sr>p.mr) re.c="环";
 }
void rsPL::nbj(double jd)
 { //南北界计算
  rsGS::init(jd,7);
  _GJW G={};
  std::array<double,10> &V=rsPL::V;
  int i;
  for(i=0;i<10;i++) V[i]=100; rsPL::Vc="",rsPL::Vb=""; //返回初始化,纬度值为100表示无解,经度100也是无解,但在以下程序中经度会被转为-PI到+PI

  rsPL::zb0(jd);
  rsPL::pp0(G); V[0]=G.J, V[1]=G.W, rsPL::Vc=G.c; //食中心

  G.J=G.W=0; for(i=0;i<2;i++) rsPL::p2p(G.J,G.W,G,1, 1); V[2]=G.J, V[3]=G.W; //本影北界,环食为南界(本影区之内,变差u,v基本不变,所以计算两次足够)
  G.J=G.W=0; for(i=0;i<2;i++) rsPL::p2p(G.J,G.W,G,1,-1); V[4]=G.J, V[5]=G.W; //本影南界,环食为北界
  G.J=G.W=0; for(i=0;i<3;i++) rsPL::p2p(G.J,G.W,G,0,-1); V[6]=G.J, V[7]=G.W; //半影北界
  G.J=G.W=0; for(i=0;i<3;i++) rsPL::p2p(G.J,G.W,G,0, 1); V[8]=G.J, V[9]=G.W; //半影南界

  if(V[3]!=100&&V[5]!=100)
  { //粗算本影南北距离
    double x=(V[2]-V[4])*cos((V[3]+V[5])/2. ), y=V[3]-V[5];
    rsPL::Vb = to_str(cs_rEarA*sqrt(x*x+y*y),0)+"千米";
  }
}