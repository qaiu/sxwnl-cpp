#include <iostream>
#include "tool.h"

//实现字符分割
std:: vector<std::string> split(const std::string& src, const std::string& separator)
{
	std::vector<std::string> dest;
	std::string str = src;
	std::string substring;
	std::string::size_type start = 0, index;
 
	do
	{
		index = str.find_first_of(separator,start);
		if (index != std::string::npos)
		{    
			substring = str.substr(start,index-start);
			dest.push_back(substring);
		#if 0
			start = str.find_first_not_of(separator,index);
		#else
			start = index+separator.length();
		#endif
			if (start == std::string::npos) return dest;
		}
	}while(index != std::string::npos);
 
	//the last token
	substring = str.substr(start);
	dest.push_back(substring);
	return dest;
}

//实现字符替换
void string_replace( std::string &strBig, const std::string &strsrc, const std::string &strdst)
{
    std::string::size_type pos = 0;
    std::string::size_type srclen = strsrc.size();
    std::string::size_type dstlen = strdst.size();

    while( (pos=strBig.find(strsrc, pos)) != std::string::npos )
    {
        strBig.replace( pos, srclen, strdst );
        pos += dstlen;
    }
}

//提取jd中的时间(去除日期)
std::string timeStr(double jd)
{
	int h, m, s;
	jd += 0.5;
	jd = (jd - int2(jd));
	s = int2(jd * 86400 + 0.5);
	h = int2(s / 3600);
	s -= h * 3600;
	m = int2(s / 60);
	s -= m * 60;
	std::string H, M, S;
	H = "0" + std::to_string(h);
	M = "0" + std::to_string(m);
	S = "0" + std::to_string(s);
	return H.substr(H.length() - 2, 2) + ":" + M.substr(M.length() - 2, 2) + ":" + S.substr(S.length() - 2, 2);
}

//===============角度格式化==================
std::string rad2strE(double d, bool flag, int ext)
{	
	//将弧度转为字串,ext为小数保留位数
	//flag=0输出格式示例: -23°59" 48.23"
	//flag=1输出格式示例:  18h 29m 44.52s
	std::string s = " ", w1 = "°", w2 = "\'", w3 = "\"";
	if (d < 0)
		d = -d, s = "-";
	if (flag)
	{
		d *= 12 / M_PI;
		w1 = "h", w2 = "m", w3 = "s";
	}
	else
		d *= 180 / M_PI;
	int a = floor(d);
	d = (d - a) * 60;
	int b = floor(d);
	d = (d - b) * 60;
	int c = floor(d);

	double Q = pow(10, ext);

	d = floor((d - c) * Q + 0.5);
	if (d >= Q)
		d -= Q, c++;
	if (c >= 60)
		c -= 60, b++;
	if (b >= 60)
		b -= 60, a++;

	std::string A, B, C, D;
	A = "   " + std::to_string(a);
	B = "0" + std::to_string(b);
	C = "0" + std::to_string(c);
	D = "00000" + std::to_string((int)d);
	s += A.substr(A.length() - 3, 3) + w1;
	s += B.substr(B.length() - 2, 2) + w2;
	s += C.substr(C.length() - 2, 2);
	if (ext)
		s += "." + D.substr(D.length() - ext, ext) + w3;
	return s;
}

//将弧度转为字串,保留2位
std::string rad2str(double d, bool tim)
{	
	return rad2strE(d, tim, 2);
}

//将弧度转为字串,精确到分
std::string rad2str2(double d)
{	
	//输出格式示例: -23°59"
	std::string s = "+", w1 = "°", w2 = "\'", w3 = "\"";
	if (d < 0)
		d = -d, s = "-";
	d *= 180 / M_PI;
	int a = floor(d);
	int b = floor((d - a) * 60 + 0.5);
	if (b >= 60)
		b -= 60, a++;
	std::string A = "   " + std::to_string(a), B = "0" + std::to_string(b);
	s += A.substr(A.length() - 3, 3) + w1;
	s += B.substr(B.length() - 2, 2) + w2;
	return s;
}

//秒转为分秒,fx为小数点位数,fs为1转为"分秒"格式否则转为"角度分秒"格式
std::string m2fm(double v, int fx, int fs)
{
	std::string gn = "";
	if (v < 0)
		v = -v, gn = "-";
	int f = floor(v / 60);
	double m = v - f * 60;
	if (!fs)
		return gn + std::to_string(f) + "\'" + to_str(m, fx) + "\"";
	if (fs == 1)
		return gn + std::to_string(f) + "分" + to_str(m, fx) + "秒";
	if (fs == 2)
		return gn + std::to_string(f) + "m" + to_str(m, fx) + "s";
	else
		return "error";
}

//公历转儒略日
double toJD(Date date)
{
	double y = date.Y, m = date.M, n = 0;	//取出年月
	if (m <= 2)
		m += 12, y--;
	if (date.Y * 372 + date.M * 31 + date.D >= 588829)
		//判断是否为格里高利历日1582*372+10*31+15
		n = (int) (y / 100), n = 2 - n + (int) (n / 4);	//加百年闰
	n += (int) (365.25 * (y + 4716) + 0.01);	//加上年引起的偏移日数
	n += (int) (30.6 * (m + 1)) + date.D;	//加上月引起的偏移日数及日偏移数
	n += ((date.s / 60.0 + date.m) / 60.0 + date.h) / 24.0 - 1524.5;
	return n;
}

//儒略日数转公历
Date setFromJD(double jd)
{
	Date r = { 0 };
	int D = int2(jd + 0.5), c;
	double F = jd + 0.5 - D;	//取得日数的整数部份A及小数部分F
	if (D >= 2299161)
		c = int2((D - 1867216.25) / 36524.25), D += 1 + c - int2(c / 4.0);
	D += 1524;
	r.Y = int2((D - 122.1) / 365.25);	//年数
	D -= int2(365.25 * r.Y);
	r.M = int2(D / 30.601);		//月数
	D -= int2(30.601 * r.M);
	r.D = D;					//日数
	if (r.M > 13)
		r.M -= 13, r.Y -= 4715;
	else
		r.M -= 1, r.Y -= 4716;

	//日的小数转为时分秒
	F *= 24.0;
	r.h = int2(F);
	F -= r.h;
	F *= 60.0;
	r.m = int2(F);
	F -= r.m;
	F *= 60.0;
	r.s = F;
	return r;
}

//日期转为串
std::string DD2str(Date r)
{ 
	std::string
	Y = "     " + std::to_string(r.Y), M = "0" + std::to_string(r.M), D = "0" + std::to_string(r.D);
	int h = r.h, m = r.m, s = int2(r.s + .5);
	if (s >= 60)
		s -= 60, m++;
	if (m >= 60)
		m -= 60, h++;

	std::string _h, _m, _s;
	_h = "0" + std::to_string(h);
	_m = "0" + std::to_string(m);
	_s = "0" + std::to_string(s);
	Y = Y.substr(Y.length() - 5, 5);
	M = M.substr(M.length() - 2, 2);
	D = D.substr(D.length() - 2, 2);
	_h = _h.substr(_h.length() - 2, 2);
	_m = _m.substr(_m.length() - 2, 2);
	_s = _s.substr(_s.length() - 2, 2);
	return Y + "-" + M + "-" + D + " " + _h + ":" + _m + ":" + _s;
}

//JD转为串
std::string JD2str(double jd)
{
	Date r=setFromJD(jd);
	return DD2str(r);
}