#ifndef XPLM300
#error This plugin requires the XPLM300 SDK
#endif

#if IBM
#include <windows.h>
#endif
#if LIN
#include <GL/gl.h>
#elif __GNUC__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include "XPLMDataAccess.h"
#include "XPLMDefs.h"
#include "XPLMDisplay.h"
#include "XPLMGraphics.h"
#include "XPLMMenus.h"
#include "XPLMNavigation.h"
#include "XPLMPlugin.h"
#include "XPLMProcessing.h"
#include "XPLMScenery.h"
#include "XPLMUtilities.h"
#include "XPStandardWidgets.h"
#include "XPWidgetDefs.h"
#include "XPWidgets.h"

#include <algorithm>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <math.h>
#include <sstream>
#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <tgmath.h>
#include <vector>

namespace fs = std::filesystem;
namespace chrono = std::chrono;

using std::fill_n;
using std::ifstream;
using std::max;
using std::min;
using std::ofstream;
using std::ostringstream;
using std::string;
using std::stringstream;
using std::remove;
using std::replace;
using std::time_t;
using std::to_string;
using std::vector;

string pluginprefix = "[HondaJet Module]";
string msginitstarted = "Initializing...";
string msginitfinished = "Initialized successfully!";
string msgenablestarting = "Enabling...";
string msgenablefinished = "Enabled successfully!";
string msgdisablestarting = "Disabling...";
string msgdisablefinished = "Disabled successfully!";
string msgstopstarting = "Stopping...";
string msgstopfinished = "Stopped successfully!";
string msgdatarefstarting = "Creating datarefs...";
string msgdatareffinished = "Datarefs created successfully.";
string msgcommandstarting = "Creating commands...";
string msgcommandfinished = "Commands created successfully.";
string msgmenustarting = "Creating menus...";
string msgmenufinished = "Menus created successfully.";
string msgloopstarting = "Creating loops...";
string msgloopfinished = "Loops created successfully.";
string msgfolderstarting = "Looking for aircraft folder...";
string msgfolderfinished = "Found aircraft folder successfully.";
string msgfilestarting = "Scanning for new files...";
string msgfilefinished = "Files loaded successfully.";
string msgcheckingcompat = "Checking compatibility...";
string msgloadfailed = "Could not initialize plugin!\n";
string msgxpoutdated = "X-Plane version not supported.";
string msgbadstorage = "Is your storage too slow?";
string msgbadinstall = "You did not install your aircraft properly.";
string msgnomodify = "It appears this aircraft was modified. Please refrain from modifying the aircraft's non-texture files.";
string acfname = "HA-420.acf";

char debug0[64] = "debug";
char debug1[64] = "debug";
char debug2[64] = "debug";
char debug3[64] = "debug";
char debug4[64] = "debug";
char debug5[64] = "debug";
char debug6[64] = "debug";
char debug7[64] = "debug";
char debug8[64] = "debug";
char debug9[64] = "debug";
char debug10[64] = "debug";

int updatetime = -1; //Dataref cycles per update (negative)

fs::path acfdir = fs::current_path();
string rootdir;
string acfdirs;

int charheight;
int progress = 0;
float oldas = 0;
bool levelChange = false;
bool levelChangeVS = false;
bool levelChangeEnd = false;
bool levelChangeATFirst = false;
bool levelChangeVSFirst = false;
bool levelChangeATException = false;
bool levelChangeVSException = false;

stringstream ss;

string NumStr(int val)
{
	ostringstream ss;
	ss << val;
	return ss.str();
}

string NumStr(float val)
{
	ostringstream ss;
	ss << val;
	return ss.str();
}

void AddChar(char* ch, string& val)
{
	strcat(ch, val.c_str());
}

void AddChar(char* ch, int val)
{
	strcat(ch, NumStr(val).c_str());
}

void AddChar(char* ch, float val)
{
	strcat(ch, NumStr(val).c_str());
}

void AddChar(char* ch, bool val)
{
	strcat(ch, val ? "true" : "false");
}

void AddChar(char* ch, char* val)
{
	strcat(ch, val);
}

void SetChar(char* ch, string& val)
{
	strcpy(ch, val.c_str());
}

void SetChar(char* ch, int val)
{
	strcpy(ch, NumStr(val).c_str());
}

void SetChar(char* ch, float val)
{
	strcpy(ch, NumStr(val).c_str());
}

void SetChar(char* ch, bool val)
{
	strcpy(ch, val ? "true" : "false");
}

void SetChar(char* ch, char* val)
{
	strcpy(ch, val);
}

float Domain(float num, float min, float max)
{
	return num < max ? (num > min ? num : min) : max;
}

float Max(float num, float max)
{
	return num < max ? num : max;
}

float Min(float num, float min)
{
	return num > min ? num : min;
}

float Round(float val, int places = 0)
{
	float mult = pow(10, places);
	return (int)(val * mult) / mult;
}

void Log(char* s)
{
	string tempstr = pluginprefix + " " + s + "\n";
	char tempchar[512] = "";

	SetChar(tempchar, tempstr);

	XPLMDebugString(tempchar);
}

void Log(string s)
{
	string tempstr = pluginprefix + " " + s + "\n";
	char tempchar[512] = "";

	SetChar(tempchar, tempstr);

	XPLMDebugString(tempchar);
}

int GetIArr(int* iarr, void* inRefcon, int* inValues, int inOffset, int inMax)
{
	int n, r;

	if (inValues == NULL)
		return sizeof(iarr);

	r = sizeof(iarr) - inOffset;
	if (r > inMax) r = inMax;

	for (n = 0; n < r; ++n)
		inValues[n] = iarr[n + inOffset];
	return r;
}

void SetIArr(int* iarr, int* outValues, int inOffset, int inMax)
{
	int n, r;

	r = sizeof(iarr) - inOffset;
	if (r > inMax) r = inMax;

	for (n = 0; n < r; ++n) iarr[n + inOffset] = outValues[n];
}

void CharAIntA(char* from, int* to)
{
	for (int i = 0; i < 67; i++) to[i] = atoi(&from[i]);
}

void Reset(stringstream& s)
{
	s.str("");
	s.clear();
}

bool PathExists(const string& s)
{
	struct stat buffer;
	return (stat(s.c_str(), &buffer) == 0);
}

template <typename TP>
time_t to_time_t(TP tp)
{
	using namespace chrono;
	auto sctp = time_point_cast<system_clock::duration>(tp - TP::clock::now()
		+ system_clock::now());
	return system_clock::to_time_t(sctp);
}

string ReadFile(string s)
{
	ifstream ifs(s);
	string data;

	for (char c; ifs.get(c); data.push_back(c));
	ifs.close();
	return data;
}

void replace(string& s, string toSearch, string replaceStr)
{
	size_t pos = s.find(toSearch);
	while (pos != string::npos)
	{
		s.replace(pos, toSearch.size(), replaceStr);
		pos = s.find(toSearch, pos + replaceStr.size());
	}
}

/*   Menus & Windows  */

void RemoveWindow(XPLMWindowID& inWindowID)
{
	if (inWindowID != NULL)
	{
		XPLMDestroyWindow(inWindowID);
		inWindowID = NULL;
	}
}

void NextLine(int& pos, int offset = 3)
{
	pos = pos - charheight - offset;
}

void XPLMDrawStringPlus(float* inColorRGB, int& inXOffset, int& inYOffset, char* inChar, int* inWordWrapWidth, XPLMFontID inFontID)
{
	XPLMDrawString(inColorRGB, inXOffset, inYOffset, inChar, inWordWrapWidth, inFontID);
	NextLine(inYOffset);
}

int menu;

XPLMMenuID menuid;

static XPLMWindowID	mainwindow;

int OnMouse(XPLMWindowID in_window_id, int x, int y, int is_down, void* in_refcon) { return 0; }
XPLMCursorStatus OnCursor(XPLMWindowID in_window_id, int x, int y, void* in_refcon) { return xplm_CursorDefault; }
int OnMouseWheel(XPLMWindowID in_window_id, int x, int y, int wheel, int clicks, void* in_refcon) { return 0; }
void OnKey(XPLMWindowID in_window_id, char key, XPLMKeyFlags flags, char virtual_key, void* in_refcon, int losing_focus) {}

void DrawMainWindow(XPLMWindowID in_window_id, void* in_refcon)
{
	float col_white[] = { 1.0, 1.0, 1.0 }; // red, green, blue

	XPLMSetGraphicsState( // Necessary
		0 /* no fog */,
		0 /* 0 texture units */,
		0 /* no lighting */,
		0 /* no alpha testing */,
		1 /* do alpha blend */,
		1 /* do depth testing */,
		0 /* no depth writing */
	);

	XPLMGetFontDimensions(xplmFont_Proportional, NULL, &charheight, NULL);

	int l, t, r, b;
	XPLMGetWindowGeometry(in_window_id, &l, &t, &r, &b); //left top right bottom

	int m1 = l + 10;
	int t1 = t - 10;

	XPLMDrawStringPlus(col_white, m1, t1, debug0, NULL, xplmFont_Proportional);
	XPLMDrawStringPlus(col_white, m1, t1, debug1, NULL, xplmFont_Proportional);
	XPLMDrawStringPlus(col_white, m1, t1, debug2, NULL, xplmFont_Proportional);
	XPLMDrawStringPlus(col_white, m1, t1, debug3, NULL, xplmFont_Proportional);
	XPLMDrawStringPlus(col_white, m1, t1, debug4, NULL, xplmFont_Proportional);
	XPLMDrawStringPlus(col_white, m1, t1, debug5, NULL, xplmFont_Proportional);
	XPLMDrawStringPlus(col_white, m1, t1, debug6, NULL, xplmFont_Proportional);
	XPLMDrawStringPlus(col_white, m1, t1, debug7, NULL, xplmFont_Proportional);
	XPLMDrawStringPlus(col_white, m1, t1, debug8, NULL, xplmFont_Proportional);
	XPLMDrawStringPlus(col_white, m1, t1, debug9, NULL, xplmFont_Proportional);
	XPLMDrawStringPlus(col_white, m1, t1, debug10, NULL, xplmFont_Proportional);
}

/*   Datarefs   */

//Byte datarefs http://www.xsquawkbox.net/xpsdk/mediawiki/ArrayDataRef
//int Getannn1l(void* inRefcon, int* inValues, int inOffset, int inMax) { return GetIArr(annn1l, inRefcon, inValues, inOffset, inMax); }
//void Setannn1r(void* inRefcon, int* outValues, int inOffset, int inMax) { SetIArr(annn1r, outValues, inOffset, inMax); }

XPLMDataRef dbtcancel = NULL;
XPLMDataRef dbarorotate = NULL;
XPLMDataRef dbarosel = NULL;
XPLMDataRef dannwarning = NULL;
XPLMDataRef danncaution = NULL;
XPLMDataRef dannn1l = NULL;
XPLMDataRef dannn1r = NULL;
XPLMDataRef dannn2l = NULL;
XPLMDataRef dannn2r = NULL;
XPLMDataRef dannittl = NULL;
XPLMDataRef dannittr = NULL;
XPLMDataRef dannoill = NULL;
XPLMDataRef dannoilr = NULL;
XPLMDataRef dannoiltl = NULL;
XPLMDataRef dannoiltr = NULL;
XPLMDataRef dannfuelpl = NULL;
XPLMDataRef dannfuelpr = NULL;
XPLMDataRef dannfuelt = NULL;
XPLMDataRef dannfuell = NULL;
XPLMDataRef dannfuelr = NULL;
XPLMDataRef dannfuelc = NULL;
XPLMDataRef dannptrim = NULL;
XPLMDataRef danncalt = NULL;
XPLMDataRef danncpsi = NULL;
XPLMDataRef danncfpm = NULL;
XPLMDataRef dannbusv1 = NULL;
XPLMDataRef dannbusv2 = NULL;
XPLMDataRef danngena1 = NULL;
XPLMDataRef danngena2 = NULL;
XPLMDataRef dannfueltemp = NULL;
XPLMDataRef dannspd = NULL;
XPLMDataRef dg3000g1rotate = NULL;
XPLMDataRef dg3000g1 = NULL;
XPLMDataRef dg3000g2rotate = NULL;
XPLMDataRef dg3000g2 = NULL;
XPLMDataRef dg3000g3rotate = NULL;
XPLMDataRef dg3000g3 = NULL;
XPLMDataRef dg3000g30rotate = NULL;
XPLMDataRef dg3000g4rotate = NULL;
XPLMDataRef dg3000g4 = NULL;
XPLMDataRef dg3000g5rotate = NULL;
XPLMDataRef dg3000g5 = NULL;
XPLMDataRef dg3000g6rotate = NULL;
XPLMDataRef dg3000g6 = NULL;
XPLMDataRef dg3000g60rotate = NULL;

int btcancel;
int barorotate;
int barosel;
int annwarning;
int anncaution;
int annn1l;
int annn1r;
int annn2l;
int annn2r;
int annittl;
int annittr;
int annoill;
int annoilr;
int annoiltl;
int annoiltr;
int annfuelpl;
int annfuelpr;
int annfuelt;
int annfuell;
int annfuelr;
int annfuelc;
int annptrim;
int anncalt;
int anncpsi;
int anncfpm;
int annbusv1;
int annbusv2;
int anngena1;
int anngena2;
int annfueltemp;
int annspd;
int g3000g1rotate;
int g3000g1;
int g3000g2rotate;
int g3000g2;
int g3000g3rotate;
int g3000g3;
int g3000g30rotate;
int g3000g4rotate;
int g3000g4;
int g3000g5rotate;
int g3000g5;
int g3000g6rotate;
int g3000g6;
int g3000g60rotate;

int Getbtcancel(void* inRefcon) { return btcancel; }
int Getbarorotate(void* inRefcon) { return barorotate; }
int Getbarosel(void* inRefcon) { return barosel; }
int Getannwarning(void* inRefcon) { return annwarning; }
int Getanncaution(void* inRefcon) { return anncaution; }
int Getannn1l(void* inRefcon) { return annn1l; }
int Getannn1r(void* inRefcon) { return annn1r; }
int Getannn2l(void* inRefcon) { return annn2l; }
int Getannn2r(void* inRefcon) { return annn2r; }
int Getannittl(void* inRefcon) { return annittl; }
int Getannittr(void* inRefcon) { return annittr; }
int Getannoill(void* inRefcon) { return annoill; }
int Getannoilr(void* inRefcon) { return annoilr; }
int Getannoiltl(void* inRefcon) { return annoiltl; }
int Getannoiltr(void* inRefcon) { return annoiltr; }
int Getannfuelpl(void* inRefcon) { return annfuelpl; }
int Getannfuelpr(void* inRefcon) { return annfuelpl; }
int Getannfuelt(void* inRefcon) { return annfuelt; }
int Getannfuell(void* inRefcon) { return annfuell; }
int Getannfuelr(void* inRefcon) { return annfuelr; }
int Getannfuelc(void* inRefcon) { return annfuelc; }
int Getannptrim(void* inRefcon) { return annptrim; }
int Getanncalt(void* inRefcon) { return anncalt; }
int Getanncpsi(void* inRefcon) { return anncpsi; }
int Getanncfpm(void* inRefcon) { return anncfpm; }
int Getannbusv1(void* inRefcon) { return annbusv1; }
int Getannbusv2(void* inRefcon) { return annbusv2; }
int Getanngena1(void* inRefcon) { return anngena1; }
int Getanngena2(void* inRefcon) { return anngena2; }
int Getannfueltemp(void* inRefcon) { return annfueltemp; }
int Getannspd(void* inRefcon) { return annspd; }
int Getg3000g1rotate(void* inRefcon) { return g3000g1rotate; }
int Getg3000g1(void* inRefcon) { return g3000g1; }
int Getg3000g2rotate(void* inRefcon) { return g3000g2rotate; }
int Getg3000g2(void* inRefcon) { return g3000g2; }
int Getg3000g3rotate(void* inRefcon) { return g3000g3rotate; }
int Getg3000g3(void* inRefcon) { return g3000g3; }
int Getg3000g30rotate(void* inRefcon) { return g3000g30rotate; }
int Getg3000g4rotate(void* inRefcon) { return g3000g4rotate; }
int Getg3000g4(void* inRefcon) { return g3000g4; }
int Getg3000g5rotate(void* inRefcon) { return g3000g5rotate; }
int Getg3000g5(void* inRefcon) { return g3000g5; }
int Getg3000g6rotate(void* inRefcon) { return g3000g6rotate; }
int Getg3000g6(void* inRefcon) { return g3000g6; }
int Getg3000g60rotate(void* inRefcon) { return g3000g60rotate; }

void Setbtcancel(void* inRefcon, int outValue) { btcancel = outValue; }
void Setbarorotate(void* inRefcon, int outValue) { barorotate = outValue; }
void Setbarosel(void* inRefcon, int outValue) { barosel = outValue; }
void Setannwarning(void* inRefcon, int outValue) { annwarning = outValue; }
void Setanncaution(void* inRefcon, int outValue) { anncaution = outValue; }
void Setannn1l(void* inRefcon, int outValue) { annn1l = outValue; }
void Setannn1r(void* inRefcon, int outValue) { annn1r = outValue; }
void Setannn2l(void* inRefcon, int outValue) { annn2l = outValue; }
void Setannn2r(void* inRefcon, int outValue) { annn2r = outValue; }
void Setannittl(void* inRefcon, int outValue) { annittl = outValue; }
void Setannittr(void* inRefcon, int outValue) { annittr = outValue; }
void Setannoill(void* inRefcon, int outValue) { annoill = outValue; }
void Setannoilr(void* inRefcon, int outValue) { annoilr = outValue; }
void Setannoiltl(void* inRefcon, int outValue) { annoiltl = outValue; }
void Setannoiltr(void* inRefcon, int outValue) { annoiltr = outValue; }
void Setannfuelpl(void* inRefcon, int outValue) { annfuelpl = outValue; }
void Setannfuelpr(void* inRefcon, int outValue) { annfuelpl = outValue; }
void Setannfuelt(void* inRefcon, int outValue) { annfuelt = outValue; }
void Setannfuell(void* inRefcon, int outValue) { annfuell = outValue; }
void Setannfuelr(void* inRefcon, int outValue) { annfuelr = outValue; }
void Setannfuelc(void* inRefcon, int outValue) { annfuelc = outValue; }
void Setannptrim(void* inRefcon, int outValue) { annptrim = outValue; }
void Setanncalt(void* inRefcon, int outValue) { anncalt = outValue; }
void Setanncpsi(void* inRefcon, int outValue) { anncpsi = outValue; }
void Setanncfpm(void* inRefcon, int outValue) { anncfpm = outValue; }
void Setannbusv1(void* inRefcon, int outValue) { annbusv1 = outValue; }
void Setannbusv2(void* inRefcon, int outValue) { annbusv2 = outValue; }
void Setanngena1(void* inRefcon, int outValue) { anngena1 = outValue; }
void Setanngena2(void* inRefcon, int outValue) { anngena2 = outValue; }
void Setannfueltemp(void* inRefcon, int outValue) { annfueltemp = outValue; }
void Setannspd(void* inRefcon, int outValue) { annspd = outValue; }
void Setg3000g1rotate(void* inRefcon, int outValue) { g3000g1rotate = outValue; }
void Setg3000g1(void* inRefcon, int outValue) { g3000g1 = outValue; }
void Setg3000g2rotate(void* inRefcon, int outValue) { g3000g2rotate = outValue; }
void Setg3000g2(void* inRefcon, int outValue) { g3000g2 = outValue; }
void Setg3000g3rotate(void* inRefcon, int outValue) { g3000g3rotate = outValue; }
void Setg3000g3(void* inRefcon, int outValue) { g3000g3 = outValue; }
void Setg3000g30rotate(void* inRefcon, int outValue) { g3000g30rotate = outValue; }
void Setg3000g4rotate(void* inRefcon, int outValue) { g3000g4rotate = outValue; }
void Setg3000g4(void* inRefcon, int outValue) { g3000g4 = outValue; }
void Setg3000g5rotate(void* inRefcon, int outValue) { g3000g5rotate = outValue; }
void Setg3000g5(void* inRefcon, int outValue) { g3000g5 = outValue; }
void Setg3000g6rotate(void* inRefcon, int outValue) { g3000g6rotate = outValue; }
void Setg3000g6(void* inRefcon, int outValue) { g3000g6 = outValue; }
void Setg3000g60rotate(void* inRefcon, int outValue) { g3000g60rotate = outValue; }

//X-Plane
XPLMDataRef dreplay = NULL;
XPLMDataRef dapvertvelocity = NULL;
XPLMDataRef dapairspeed = NULL;
XPLMDataRef dthrottleall = NULL;
XPLMDataRef dapatmode = NULL;
XPLMDataRef dkias = NULL;
XPLMDataRef dmach = NULL;
XPLMDataRef dthrust = NULL;
XPLMDataRef dappaltitude = NULL;
XPLMDataRef dalt = NULL;
XPLMDataRef dapvvi = NULL;
XPLMDataRef dapspdu = NULL;
XPLMDataRef dn1 = NULL;
XPLMDataRef dn2 = NULL;
XPLMDataRef ditt = NULL;
XPLMDataRef doil = NULL;
XPLMDataRef doilt = NULL;
XPLMDataRef dfuelp = NULL;
XPLMDataRef dfuelt = NULL;
XPLMDataRef dfuel = NULL;
XPLMDataRef doverridebeaconsandstrobes = NULL;
XPLMDataRef dstrobeson = NULL;
XPLMDataRef dstrobes = NULL;
XPLMDataRef dbeaconon = NULL;
XPLMDataRef dbeacon = NULL;
XPLMDataRef dinstbrightness = NULL;
XPLMDataRef dannmasterwarning = NULL;
XPLMDataRef dannmastercaution = NULL;
XPLMDataRef dmaxnw = NULL;
XPLMDataRef dgs = NULL;
XPLMDataRef dptrim = NULL;
XPLMDataRef dcalt = NULL;
XPLMDataRef dcpsi = NULL;
XPLMDataRef dcfpm = NULL;
XPLMDataRef dbusv = NULL;
XPLMDataRef dgena = NULL;
XPLMDataRef douttemple = NULL;
XPLMDataRef dmaxthrot = NULL;
XPLMDataRef dengon = NULL;
XPLMDataRef dsurface = NULL;
XPLMDataRef dbrakecf = NULL;
XPLMDataRef dlbrake = NULL;
XPLMDataRef drbrake = NULL;

float n1[2] = {};
float n2[2] = {};
float itt[2] = {};
float oil[2] = {};
float oilt[2] = {};
float fuelp[2] = {};
float fuel[9] = {};
float instbrightness[32] = {};
float busv[6] = {};
float gena[8] = {};
int engon[8] = {};

//Autopilot
XPLMDataRef dapfd = NULL;
XPLMDataRef dapnav = NULL;
XPLMDataRef dapapr = NULL;
XPLMDataRef dapbank = NULL;
XPLMDataRef daphdg = NULL;
XPLMDataRef dapap = NULL;
XPLMDataRef dapyd = NULL;
XPLMDataRef dapcsc = NULL;
XPLMDataRef dapcpl = NULL;
XPLMDataRef dapalt = NULL;
XPLMDataRef dapvnv = NULL;
XPLMDataRef dapvs = NULL;
XPLMDataRef dapflc = NULL;

XPLMDataRef dapcrs = NULL;
XPLMDataRef daphdgsel = NULL;
XPLMDataRef dapspdsel = NULL;

XPLMDataRef dapcrsrotate = NULL;
XPLMDataRef daphdgselrotate = NULL;
XPLMDataRef dapaltselrotate = NULL;
XPLMDataRef dapvsselrotate = NULL;
XPLMDataRef dapspdselrotate = NULL;

int apfd;
int apnav;
int apapr;
int apbank;
int aphdg;
int apap;
int apyd;
int apcsc;
int apcpl;
int apalt;
int apvnv;
int apvs;
int apflc;

int apcrs;
int aphdgsel;
int apspdsel;

int apcrsrotate;
int aphdgselrotate;
int apaltselrotate;
int apvsselrotate;
int apspdselrotate;

int Getapfd(void* inRefcon) { return apfd; }
int Getapnav(void* inRefcon) { return apnav; }
int Getapapr(void* inRefcon) { return apapr; }
int Getapbank(void* inRefcon) { return apbank; }
int Getaphdg(void* inRefcon) { return aphdg; }
int Getapap(void* inRefcon) { return apap; }
int Getapyd(void* inRefcon) { return apyd; }
int Getapcsc(void* inRefcon) { return apcsc; }
int Getapcpl(void* inRefcon) { return apcpl; }
int Getapalt(void* inRefcon) { return apalt; }
int Getapvnv(void* inRefcon) { return apvnv; }
int Getapvs(void* inRefcon) { return apvs; }
int Getapflc(void* inRefcon) { return apflc; }

int Getapcrs(void* inRefcon) { return apcrs; }
int Getaphdgsel(void* inRefcon) { return aphdgsel; }
int Getapspdsel(void* inRefcon) { return apspdsel; }

int Getapcrsrotate(void* inRefcon) { return apcrsrotate; }
int Getaphdgselrotate(void* inRefcon) { return aphdgselrotate; }
int Getapaltselrotate(void* inRefcon) { return apaltselrotate; }
int Getapvsselrotate(void* inRefcon) { return apvsselrotate; }
int Getapspdselrotate(void* inRefcon) { return apspdselrotate; }

void Setapfd(void* inRefcon, int outValue) { apfd = outValue; }
void Setapnav(void* inRefcon, int outValue) { apnav = outValue; }
void Setapapr(void* inRefcon, int outValue) { apapr = outValue; }
void Setapbank(void* inRefcon, int outValue) { apbank = outValue; }
void Setaphdg(void* inRefcon, int outValue) { aphdg = outValue; }
void Setapap(void* inRefcon, int outValue) { apap = outValue; }
void Setapyd(void* inRefcon, int outValue) { apyd = outValue; }
void Setapcsc(void* inRefcon, int outValue) { apcsc = outValue; }
void Setapcpl(void* inRefcon, int outValue) { apcpl = outValue; }
void Setapalt(void* inRefcon, int outValue) { apalt = outValue; }
void Setapvnv(void* inRefcon, int outValue) { apvnv = outValue; }
void Setapvs(void* inRefcon, int outValue) { apvs = outValue; }
void Setapflc(void* inRefcon, int outValue) { apflc = outValue; }

void Setapcrs(void* inRefcon, int outValue) { apcrs = outValue; }
void Setaphdgsel(void* inRefcon, int outValue) { aphdgsel = outValue; }
void Setapspdsel(void* inRefcon, int outValue) { apspdsel = outValue; }

void Setapcrsrotate(void* inRefcon, int outValue) { apcrsrotate = outValue; }
void Setaphdgselrotate(void* inRefcon, int outValue) { aphdgselrotate = outValue; }
void Setapaltselrotate(void* inRefcon, int outValue) { apaltselrotate = outValue; }
void Setapvsselrotate(void* inRefcon, int outValue) { apvsselrotate = outValue; }
void Setapspdselrotate(void* inRefcon, int outValue) { apspdselrotate = outValue; }

//PFD
XPLMDataRef dpfdsk1 = NULL;
XPLMDataRef dpfdsk2 = NULL;
XPLMDataRef dpfdsk3 = NULL;
XPLMDataRef dpfdsk4 = NULL;
XPLMDataRef dpfdsk5 = NULL;
XPLMDataRef dpfdsk6 = NULL;
XPLMDataRef dpfdsk7 = NULL;
XPLMDataRef dpfdsk8 = NULL;
XPLMDataRef dpfdsk9 = NULL;
XPLMDataRef dpfdsk10 = NULL;
XPLMDataRef dpfdsk11 = NULL;
XPLMDataRef dpfdsk12 = NULL;

int pfdsk1;
int pfdsk2;
int pfdsk3;
int pfdsk4;
int pfdsk5;
int pfdsk6;
int pfdsk7;
int pfdsk8;
int pfdsk9;
int pfdsk10;
int pfdsk11;
int pfdsk12;

int Getpfdsk1(void* inRefcon) { return pfdsk1; }
int Getpfdsk2(void* inRefcon) { return pfdsk2; }
int Getpfdsk3(void* inRefcon) { return pfdsk3; }
int Getpfdsk4(void* inRefcon) { return pfdsk4; }
int Getpfdsk5(void* inRefcon) { return pfdsk5; }
int Getpfdsk6(void* inRefcon) { return pfdsk6; }
int Getpfdsk7(void* inRefcon) { return pfdsk7; }
int Getpfdsk8(void* inRefcon) { return pfdsk8; }
int Getpfdsk9(void* inRefcon) { return pfdsk9; }
int Getpfdsk10(void* inRefcon) { return pfdsk10; }
int Getpfdsk11(void* inRefcon) { return pfdsk11; }
int Getpfdsk12(void* inRefcon) { return pfdsk12; }

void Setpfdsk1(void* inRefcon, int outValue) { pfdsk1 = outValue; }
void Setpfdsk2(void* inRefcon, int outValue) { pfdsk2 = outValue; }
void Setpfdsk3(void* inRefcon, int outValue) { pfdsk3 = outValue; }
void Setpfdsk4(void* inRefcon, int outValue) { pfdsk4 = outValue; }
void Setpfdsk5(void* inRefcon, int outValue) { pfdsk5 = outValue; }
void Setpfdsk6(void* inRefcon, int outValue) { pfdsk6 = outValue; }
void Setpfdsk7(void* inRefcon, int outValue) { pfdsk7 = outValue; }
void Setpfdsk8(void* inRefcon, int outValue) { pfdsk8 = outValue; }
void Setpfdsk9(void* inRefcon, int outValue) { pfdsk9 = outValue; }
void Setpfdsk10(void* inRefcon, int outValue) { pfdsk10 = outValue; }
void Setpfdsk11(void* inRefcon, int outValue) { pfdsk11 = outValue; }
void Setpfdsk12(void* inRefcon, int outValue) { pfdsk12 = outValue; }

//MFD
XPLMDataRef dmfdsk1 = NULL;
XPLMDataRef dmfdsk2 = NULL;
XPLMDataRef dmfdsk3 = NULL;
XPLMDataRef dmfdsk4 = NULL;
XPLMDataRef dmfdsk5 = NULL;
XPLMDataRef dmfdsk6 = NULL;
XPLMDataRef dmfdsk7 = NULL;
XPLMDataRef dmfdsk8 = NULL;
XPLMDataRef dmfdsk9 = NULL;
XPLMDataRef dmfdsk10 = NULL;
XPLMDataRef dmfdsk11 = NULL;
XPLMDataRef dmfdsk12 = NULL;

int mfdsk1;
int mfdsk2;
int mfdsk3;
int mfdsk4;
int mfdsk5;
int mfdsk6;
int mfdsk7;
int mfdsk8;
int mfdsk9;
int mfdsk10;
int mfdsk11;
int mfdsk12;

int Getmfdsk1(void* inRefcon) { return mfdsk1; }
int Getmfdsk2(void* inRefcon) { return mfdsk2; }
int Getmfdsk3(void* inRefcon) { return mfdsk3; }
int Getmfdsk4(void* inRefcon) { return mfdsk4; }
int Getmfdsk5(void* inRefcon) { return mfdsk5; }
int Getmfdsk6(void* inRefcon) { return mfdsk6; }
int Getmfdsk7(void* inRefcon) { return mfdsk7; }
int Getmfdsk8(void* inRefcon) { return mfdsk8; }
int Getmfdsk9(void* inRefcon) { return mfdsk9; }
int Getmfdsk10(void* inRefcon) { return mfdsk10; }
int Getmfdsk11(void* inRefcon) { return mfdsk11; }
int Getmfdsk12(void* inRefcon) { return mfdsk12; }

void Setmfdsk1(void* inRefcon, int outValue) { mfdsk1 = outValue; }
void Setmfdsk2(void* inRefcon, int outValue) { mfdsk2 = outValue; }
void Setmfdsk3(void* inRefcon, int outValue) { mfdsk3 = outValue; }
void Setmfdsk4(void* inRefcon, int outValue) { mfdsk4 = outValue; }
void Setmfdsk5(void* inRefcon, int outValue) { mfdsk5 = outValue; }
void Setmfdsk6(void* inRefcon, int outValue) { mfdsk6 = outValue; }
void Setmfdsk7(void* inRefcon, int outValue) { mfdsk7 = outValue; }
void Setmfdsk8(void* inRefcon, int outValue) { mfdsk8 = outValue; }
void Setmfdsk9(void* inRefcon, int outValue) { mfdsk9 = outValue; }
void Setmfdsk10(void* inRefcon, int outValue) { mfdsk10 = outValue; }
void Setmfdsk11(void* inRefcon, int outValue) { mfdsk11 = outValue; }
void Setmfdsk12(void* inRefcon, int outValue) { mfdsk12 = outValue; }

/*   Commands   */

XPLMCommandRef cg3000g1u = NULL;
XPLMCommandRef cg3000g1d = NULL;
XPLMCommandRef cg3000g1p = NULL;
XPLMCommandRef cg3000g2u = NULL;
XPLMCommandRef cg3000g2d = NULL;
XPLMCommandRef cg3000g2p = NULL;
XPLMCommandRef cg3000g3u = NULL;
XPLMCommandRef cg3000g3d = NULL;
XPLMCommandRef cg3000g3p = NULL;
XPLMCommandRef cg3000g30u = NULL;
XPLMCommandRef cg3000g30d = NULL;
XPLMCommandRef cg3000g4u = NULL;
XPLMCommandRef cg3000g4d = NULL;
XPLMCommandRef cg3000g4p = NULL;
XPLMCommandRef cg3000g5u = NULL;
XPLMCommandRef cg3000g5d = NULL;
XPLMCommandRef cg3000g5p = NULL;
XPLMCommandRef cg3000g6u = NULL;
XPLMCommandRef cg3000g6d = NULL;
XPLMCommandRef cg3000g6p = NULL;
XPLMCommandRef cg3000g60u = NULL;
XPLMCommandRef cg3000g60d = NULL;
XPLMCommandRef cbtcancel = NULL;
XPLMCommandRef cwarningcancel = NULL;
XPLMCommandRef ccautioncancel = NULL;
XPLMCommandRef cbarorotateu = NULL;
XPLMCommandRef cbarorotated = NULL;
XPLMCommandRef cbarosel = NULL;

int Cg3000g1u(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = g3000g1rotate + 1;
		if (newVal < 360) XPLMSetDatai(dg3000g1rotate, newVal);
		else XPLMSetDatai(dg3000g1rotate, 0);
	}
	break;
	}

	return 1;
}
int Cg3000g1d(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = g3000g1rotate - 1;
		if (newVal >= 0) XPLMSetDatai(dg3000g1rotate, newVal);
		else XPLMSetDatai(dg3000g1rotate, 360);
	}
	break;
	}

	return 1;
}
int Cg3000g1p(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dg3000g1, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dg3000g1, 0);
	}
	break;
	}

	return 1;
}
int Cg3000g2u(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = g3000g2rotate + 1;
		if (newVal < 360) XPLMSetDatai(dg3000g2rotate, newVal);
		else XPLMSetDatai(dg3000g2rotate, 0);
	}
	break;
	}

	return 1;
}
int Cg3000g2d(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = g3000g2rotate - 1;
		if (newVal >= 0) XPLMSetDatai(dg3000g2rotate, newVal);
		else XPLMSetDatai(dg3000g2rotate, 360);
	}
	break;
	}

	return 1;
}
int Cg3000g2p(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dg3000g2, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dg3000g2, 0);
	}
	break;
	}

	return 1;
}
int Cg3000g3u(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = g3000g3rotate + 1;
		if (newVal < 360) XPLMSetDatai(dg3000g3rotate, newVal);
		else XPLMSetDatai(dg3000g3rotate, 0);
	}
	break;
	}

	return 1;
}
int Cg3000g3d(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = g3000g3rotate - 1;
		if (newVal >= 0) XPLMSetDatai(dg3000g3rotate, newVal);
		else XPLMSetDatai(dg3000g3rotate, 360);
	}
	break;
	}

	return 1;
}
int Cg3000g3p(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dg3000g3, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dg3000g3, 0);
	}
	break;
	}

	return 1;
}
int Cg3000g30u(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = g3000g30rotate + 1;
		if (newVal < 360) XPLMSetDatai(dg3000g30rotate, newVal);
		else XPLMSetDatai(dg3000g30rotate, 0);
	}
	break;
	}

	return 1;
}
int Cg3000g30d(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = g3000g30rotate - 1;
		if (newVal >= 0) XPLMSetDatai(dg3000g30rotate, newVal);
		else XPLMSetDatai(dg3000g30rotate, 360);
	}
	break;
	}

	return 1;
}
int Cg3000g4u(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = g3000g4rotate + 1;
		if (newVal < 360) XPLMSetDatai(dg3000g4rotate, newVal);
		else XPLMSetDatai(dg3000g4rotate, 0);
	}
	break;
	}

	return 1;
}
int Cg3000g4d(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = g3000g4rotate - 1;
		if (newVal >= 0) XPLMSetDatai(dg3000g4rotate, newVal);
		else XPLMSetDatai(dg3000g4rotate, 360);
	}
	break;
	}

	return 1;
}
int Cg3000g4p(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dg3000g4, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dg3000g4, 0);
	}
	break;
	}

	return 1;
}
int Cg3000g5u(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = g3000g5rotate + 1;
		if (newVal < 360) XPLMSetDatai(dg3000g5rotate, newVal);
		else XPLMSetDatai(dg3000g5rotate, 0);
	}
	break;
	}

	return 1;
}
int Cg3000g5d(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = g3000g5rotate - 1;
		if (newVal >= 0) XPLMSetDatai(dg3000g5rotate, newVal);
		else XPLMSetDatai(dg3000g5rotate, 360);
	}
	break;
	}

	return 1;
}
int Cg3000g5p(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dg3000g5, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dg3000g5, 0);
	}
	break;
	}

	return 1;
}
int Cg3000g6u(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = g3000g6rotate + 1;
		if (newVal < 360) XPLMSetDatai(dg3000g6rotate, newVal);
		else XPLMSetDatai(dg3000g6rotate, 0);
	}
	break;
	}

	return 1;
}
int Cg3000g6d(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = g3000g6rotate - 1;
		if (newVal >= 0) XPLMSetDatai(dg3000g6rotate, newVal);
		else XPLMSetDatai(dg3000g6rotate, 360);
	}
	break;
	}

	return 1;
}
int Cg3000g6p(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dg3000g6, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dg3000g6, 0);
	}
	break;
	}

	return 1;
}
int Cg3000g60u(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = g3000g60rotate + 1;
		if (newVal < 360) XPLMSetDatai(dg3000g60rotate, newVal);
		else XPLMSetDatai(dg3000g60rotate, 0);
	}
	break;
	}

	return 1;
}
int Cg3000g60d(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = g3000g60rotate - 1;
		if (newVal >= 0) XPLMSetDatai(dg3000g60rotate, newVal);
		else XPLMSetDatai(dg3000g60rotate, 360);
	}
	break;
	}

	return 1;
}
int Cbtcancel(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMCommandOnce(cwarningcancel);
		XPLMCommandOnce(ccautioncancel);

		XPLMSetDatai(dbtcancel, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dbtcancel, 0);
	}
	break;
	}

	return 1;
}
int Cwarningcancel(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	return 1;
}
int Ccautioncancel(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	return 1;
}
int Cbarorotateu(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = barorotate + 1;
		if (newVal < 360) XPLMSetDatai(dbarorotate, newVal);
		else XPLMSetDatai(dbarorotate, 0);
	}
	break;
	}

	return 1;
}
int Cbarorotated(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = barorotate - 1;
		if (newVal >= 0) XPLMSetDatai(dbarorotate, newVal);
		else XPLMSetDatai(dbarorotate, 360);
	}
	break;
	}

	return 1;
}
int Cbarosel(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dbarosel, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dbarosel, 0);
	}
	break;
	}

	return 1;
}

//Autopilot

XPLMCommandRef caplvlchg = NULL;
XPLMCommandRef capattoggle = NULL;

XPLMCommandRef capfd = NULL;
XPLMCommandRef capnav = NULL;
XPLMCommandRef capapr = NULL;
XPLMCommandRef capbank = NULL;
XPLMCommandRef caphdg = NULL;
XPLMCommandRef capap = NULL;
XPLMCommandRef capyd = NULL;
XPLMCommandRef capcsc = NULL;
XPLMCommandRef capcpl = NULL;
XPLMCommandRef capalt = NULL;
XPLMCommandRef capvnv = NULL;
XPLMCommandRef capvs = NULL;
XPLMCommandRef capflc = NULL;

XPLMCommandRef capcrs = NULL;
XPLMCommandRef caphdgsel = NULL;
XPLMCommandRef capspdsel = NULL;

XPLMCommandRef capcrsrotateu = NULL;
XPLMCommandRef capcrsrotated = NULL;
XPLMCommandRef caphdgselrotateu = NULL;
XPLMCommandRef caphdgselrotated = NULL;
XPLMCommandRef capaltselrotateu = NULL;
XPLMCommandRef capaltselrotated = NULL;
XPLMCommandRef capvsselrotateu = NULL;
XPLMCommandRef capvsselrotated = NULL;
XPLMCommandRef capspdselrotateu = NULL;
XPLMCommandRef capspdselrotated = NULL;

int Caplvlchg(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		if (levelChange) levelChange = false;
		else
		{
			levelChangeATFirst = true;
			levelChangeVSFirst = true;
			levelChangeATException = true;
			levelChangeVSException = true;

			if (XPLMGetDatai(dapvvi) == 2) XPLMCommandOnce(capvs);
			levelChangeVS = true;
			XPLMCommandOnce(capvs);
		}

		XPLMSetDatai(dapflc, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dapflc, 0);
	}
	break;
	}

	return 1;
}
int Capattoggle(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dapcsc, 1);

		int apatmode = XPLMGetDatai(dapatmode);

		float apairspeed = 0;
		if (apatmode > 0) apairspeed = XPLMGetDataf(dapairspeed);
		else apairspeed = XPLMGetDataf(dkias);

		if (!XPLMGetDatai(dapspdu)) oldas = round(apairspeed); //Knots
		else oldas = Round(apairspeed, 2); //Mach

		if (levelChange)
		{
			levelChange = false;
			XPLMCommandOnce(capflc);
		}

		XPLMCommandOnce(capcsc);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dapcsc, 0);
	}
	break;
	}

	return 1;
}

int Capfd(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dapfd, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dapfd, 0);
	}
	break;
	}

	return 1;
}
int Capnav(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dapnav, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dapnav, 0);
	}
	break;
	}

	return 1;
}
int Capapr(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dapapr, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dapapr, 0);
	}
	break;
	}

	return 1;
}
int Capbank(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dapbank, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dapbank, 0);
	}
	break;
	}

	return 1;
}
int Caphdg(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(daphdg, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(daphdg, 0);
	}
	break;
	}

	return 1;
}
int Capap(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dapap, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dapap, 0);
	}
	break;
	}

	return 1;
}
int Capyd(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dapyd, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dapyd, 0);
	}
	break;
	}

	return 1;
}
int Capcsc(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	XPLMSetDataf(dapairspeed, oldas);

	return 1;
}
int Capcpl(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dapcpl, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dapcpl, 0);
	}
	break;
	}

	return 1;
}
int Capalt(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dapalt, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dapalt, 0);
	}
	break;
	}

	return 1;
}
int Capvnv(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dapvnv, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dapvnv, 0);
	}
	break;
	}

	return 1;
}
int Capvs(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dapvs, 1);

		if (levelChangeVS)
		{
			levelChangeVS = false;
			levelChange = true;
		}
	}
	break;
	case 2:
	{
		XPLMSetDatai(dapvs, 0);
	}
	break;
	}

	return 1;
}
int Capflc(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	return 1;
}

int Capcrs(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dapcrs, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dapcrs, 0);
	}
	break;
	}

	return 1;
}
int Caphdgsel(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(daphdgsel, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(daphdgsel, 0);
	}
	break;
	}

	return 1;
}
int Capspdsel(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dapspdsel, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dapspdsel, 0);
	}
	break;
	}

	return 1;
}

int Capcrsrotateu(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = apcrsrotate + 1;
		if (newVal < 360) XPLMSetDatai(dapcrsrotate, newVal);
		else XPLMSetDatai(dapcrsrotate, 0);
	}
	break;
	}

	return 1;
}
int Capcrsrotated(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = apcrsrotate - 1;
		if (newVal >= 0) XPLMSetDatai(dapcrsrotate, newVal);
		else XPLMSetDatai(dapcrsrotate, 360);
	}
	break;
	}

	return 1;
}
int Caphdgselrotateu(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = aphdgselrotate + 1;
		if (newVal < 360) XPLMSetDatai(daphdgselrotate, newVal);
		else XPLMSetDatai(daphdgselrotate, 0);
	}
	break;
	}

	return 1;
}
int Caphdgselrotated(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = aphdgselrotate - 1;
		if (newVal >= 0) XPLMSetDatai(daphdgselrotate, newVal);
		else XPLMSetDatai(daphdgselrotate, 360);
	}
	break;
	}

	return 1;
}
int Capaltselrotateu(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = apaltselrotate + 1;
		if (newVal < 360) XPLMSetDatai(dapaltselrotate, newVal);
		else XPLMSetDatai(dapaltselrotate, 0);
	}
	break;
	}

	return 1;
}
int Capaltselrotated(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = apaltselrotate - 1;
		if (newVal >= 0) XPLMSetDatai(dapaltselrotate, newVal);
		else XPLMSetDatai(dapaltselrotate, 360);
	}
	break;
	}

	return 1;
}
int Capvsselrotateu(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	int returnval = 1;

	switch (inPhase)
	{
	case 0:
	{
		int newVal = apvsselrotate + 1;
		if (newVal < 360) XPLMSetDatai(dapvsselrotate, newVal);
		else XPLMSetDatai(dapvsselrotate, 0);
		if (levelChange)
		{
			XPLMCommandOnce(capspdselrotateu);
			returnval = 0;
		}
	}
	break;
	}

	return returnval;
}
int Capvsselrotated(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	int returnval = 1;

	switch (inPhase)
	{
	case 0:
	{
		int newVal = apvsselrotate - 1;
		if (newVal >= 0) XPLMSetDatai(dapvsselrotate, newVal);
		else XPLMSetDatai(dapvsselrotate, 360);
		if (levelChange)
		{
			XPLMCommandOnce(capspdselrotated);
			returnval = 0;
		}
	}
	break;
	}

	return returnval;
}
int Capspdselrotateu(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = apspdselrotate + 1;
		if (newVal < 360) XPLMSetDatai(dapspdselrotate, newVal);
		else XPLMSetDatai(dapspdselrotate, 0);
	}
	break;
	}

	return 1;
}
int Capspdselrotated(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		int newVal = apspdselrotate - 1;
		if (newVal >= 0) XPLMSetDatai(dapspdselrotate, newVal);
		else XPLMSetDatai(dapspdselrotate, 360);
	}
	break;
	}

	return 1;
}

//PFD
XPLMCommandRef cpfdsk1 = NULL;
XPLMCommandRef cpfdsk2 = NULL;
XPLMCommandRef cpfdsk3 = NULL;
XPLMCommandRef cpfdsk4 = NULL;
XPLMCommandRef cpfdsk5 = NULL;
XPLMCommandRef cpfdsk6 = NULL;
XPLMCommandRef cpfdsk7 = NULL;
XPLMCommandRef cpfdsk8 = NULL;
XPLMCommandRef cpfdsk9 = NULL;
XPLMCommandRef cpfdsk10 = NULL;
XPLMCommandRef cpfdsk11 = NULL;
XPLMCommandRef cpfdsk12 = NULL;

int Cpfdsk1(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dpfdsk1, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dpfdsk1, 0);
	}
	break;
	}

	return 1;
}
int Cpfdsk2(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dpfdsk2, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dpfdsk2, 0);
	}
	break;
	}

	return 1;
}
int Cpfdsk3(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dpfdsk3, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dpfdsk3, 0);
	}
	break;
	}

	return 1;
}
int Cpfdsk4(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dpfdsk4, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dpfdsk4, 0);
	}
	break;
	}

	return 1;
}
int Cpfdsk5(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dpfdsk5, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dpfdsk5, 0);
	}
	break;
	}

	return 1;
}
int Cpfdsk6(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dpfdsk6, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dpfdsk6, 0);
	}
	break;
	}

	return 1;
}
int Cpfdsk7(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dpfdsk7, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dpfdsk7, 0);
	}
	break;
	}

	return 1;
}
int Cpfdsk8(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dpfdsk8, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dpfdsk8, 0);
	}
	break;
	}

	return 1;
}
int Cpfdsk9(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dpfdsk9, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dpfdsk9, 0);
	}
	break;
	}

	return 1;
}
int Cpfdsk10(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dpfdsk10, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dpfdsk10, 0);
	}
	break;
	}

	return 1;
}
int Cpfdsk11(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dpfdsk11, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dpfdsk11, 0);
	}
	break;
	}

	return 1;
}
int Cpfdsk12(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dpfdsk12, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dpfdsk12, 0);
	}
	break;
	}

	return 1;
}

//MFD
XPLMCommandRef cmfdsk1 = NULL;
XPLMCommandRef cmfdsk2 = NULL;
XPLMCommandRef cmfdsk3 = NULL;
XPLMCommandRef cmfdsk4 = NULL;
XPLMCommandRef cmfdsk5 = NULL;
XPLMCommandRef cmfdsk6 = NULL;
XPLMCommandRef cmfdsk7 = NULL;
XPLMCommandRef cmfdsk8 = NULL;
XPLMCommandRef cmfdsk9 = NULL;
XPLMCommandRef cmfdsk10 = NULL;
XPLMCommandRef cmfdsk11 = NULL;
XPLMCommandRef cmfdsk12 = NULL;

int Cmfdsk1(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dmfdsk1, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dmfdsk1, 0);
	}
	break;
	}

	return 1;
}
int Cmfdsk2(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dmfdsk2, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dmfdsk2, 0);
	}
	break;
	}

	return 1;
}
int Cmfdsk3(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dmfdsk3, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dmfdsk3, 0);
	}
	break;
	}

	return 1;
}
int Cmfdsk4(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dmfdsk4, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dmfdsk4, 0);
	}
	break;
	}

	return 1;
}
int Cmfdsk5(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dmfdsk5, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dmfdsk5, 0);
	}
	break;
	}

	return 1;
}
int Cmfdsk6(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dmfdsk6, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dmfdsk6, 0);
	}
	break;
	}

	return 1;
}
int Cmfdsk7(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dmfdsk7, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dmfdsk7, 0);
	}
	break;
	}

	return 1;
}
int Cmfdsk8(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dmfdsk8, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dmfdsk8, 0);
	}
	break;
	}

	return 1;
}
int Cmfdsk9(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dmfdsk9, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dmfdsk9, 0);
	}
	break;
	}

	return 1;
}
int Cmfdsk10(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dmfdsk10, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dmfdsk10, 0);
	}
	break;
	}

	return 1;
}
int Cmfdsk11(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dmfdsk11, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dmfdsk11, 0);
	}
	break;
	}

	return 1;
}
int Cmfdsk12(XPLMCommandRef inCommand, XPLMCommandPhase inPhase, void* inRefcon)
{
	switch (inPhase)
	{
	case 0:
	{
		XPLMSetDatai(dmfdsk12, 1);
	}
	break;
	case 2:
	{
		XPLMSetDatai(dmfdsk12, 0);
	}
	break;
	}

	return 1;
}

/*   Menus   */

void menu_handler(void* in_menu_ref, void* in_item_ref)
{
	if (!strcmp((const char*)in_item_ref, "Debug Menu"))
	{
		if (mainwindow == NULL)
		{
			XPLMCreateWindow_t params;
			params.structSize = sizeof(params);
			params.visible = 1;
			params.drawWindowFunc = DrawMainWindow;
			// Note on "dummy" handlers:
			// Even if we don't want to handle these events, we have to register a "do-nothing" callback for them
			params.handleMouseClickFunc = OnMouse;
			params.handleRightClickFunc = OnMouse;
			params.handleMouseWheelFunc = OnMouseWheel;
			params.handleKeyFunc = OnKey;
			params.handleCursorFunc = OnCursor;
			params.refcon = NULL;
			params.layer = xplm_WindowLayerFloatingWindows;
			// Opt-in to styling our window like an X-Plane 11 native window
			// If you're on XPLM300, not XPLM301, swap this enum for the literal value 1.
			params.decorateAsFloatingWindow = xplm_WindowDecorationRoundRectangle;

			// Set the window's initial bounds
			// Note that we're not guaranteed that the main monitor's lower left is at (0, 0)...
			// We'll need to query for the global desktop bounds!
			int left, bottom, right, top;
			XPLMGetScreenBoundsGlobal(&left, &top, &right, &bottom);
			params.left = left + 50;
			params.bottom = bottom + 150;
			params.right = params.left + 200;
			params.top = params.bottom + 200;

			mainwindow = XPLMCreateWindowEx(&params);

			XPLMSetWindowTitle(mainwindow, "HondaJet Module");
			XPLMSetWindowPositioningMode(mainwindow, xplm_WindowPositionFree, -1); // "free" floating window, which the user can drag around
			XPLMSetWindowResizingLimits(mainwindow, 200, 300, 400, 500); // Minimum width/height of 200 boxels and a max width/height of 300 boxels
		}
		else
		{
			if (!XPLMGetWindowIsVisible(mainwindow))
			{
				RemoveWindow(mainwindow);
				menu_handler(in_menu_ref, in_item_ref);
			}
			else RemoveWindow(mainwindow);
		}
	}
}

/*   Loops   */

float lvlchgupdatetime = 1;
float strobeontime = 0.4;
float strobeofftime = 0.8;
float beaconontime = 0.6;
float beaconofftime = 0.6;

float vvitime = 0;
float strobetime = 0;
float beacontime = 0;

int nwcurveoffset = 60;

float glastspd = XPLMGetDataf(dkias);
bool gvvion = false;
bool gaton = false;
bool glaststrobe = false;
bool glastbeacon = false;
float lastapv = 0;
float sarr[4] = { 0, 0, 0, 0 };
float barr[4] = { 0, 0, 0, 0 };

float Interference(float inElapsedSinceLastCall, float inElapsedTimeSinceLastFlightLoop, int inCounter, void* inRefcon)
{
	int replay = XPLMGetDatai(dreplay);
	if (replay == 7) return updatetime;

	float kias = XPLMGetDataf(dkias);
	float gs = XPLMGetDataf(dgs) * (float)1.94384;
	int apatmode = XPLMGetDatai(dapatmode);
	float apvertvelocity = XPLMGetDataf(dapvertvelocity);
	float desiredvv = apvertvelocity;
	float throttleall = XPLMGetDataf(dthrottleall);
	float appaltitude = XPLMGetDataf(dappaltitude);
	float alt = XPLMGetDataf(dalt);
	bool apismach = XPLMGetDatai(dapspdu) == 1;
	int surface = XPLMGetDatai(dsurface);
	float lbrake = XPLMGetDataf(dlbrake);
	float rbrake = XPLMGetDataf(drbrake);
	float brake = max(lbrake, rbrake);

	gvvion = XPLMGetDatai(dapvvi) == 2;
	gaton = apatmode == 1;

	XPLMGetDatavf(dn1, n1, 0, 2);
	XPLMGetDatavf(dn2, n2, 0, 2);
	XPLMGetDatavf(ditt, itt, 0, 2);
	XPLMGetDatavf(doil, oil, 0, 2);
	XPLMGetDatavf(doilt, oilt, 0, 2);
	XPLMGetDatavf(dfuelp, fuelp, 0, 2);
	XPLMGetDatavf(dfuel, fuel, 0, 9);
	XPLMGetDatavf(dbusv, busv, 0, 6);
	XPLMGetDatavf(dgena, gena, 0, 8);
	XPLMGetDatavi(dengon, engon, 0, 8);
	XPLMGetDatavf(dinstbrightness, instbrightness, 0, 32);

	//Strobe Lights
	XPLMSetDatai(doverridebeaconsandstrobes, 1);

	strobetime += inElapsedSinceLastCall;

	if (XPLMGetDatai(dstrobeson) == 1)
	{
		if (glaststrobe)
		{
			if (strobetime > strobeontime)
			{
				strobetime = 0;
				sarr[0] = 0;
				glaststrobe = false;
				XPLMSetDatavf(dstrobes, sarr, 0, 4);
			}
		}
		else
		{
			if (strobetime > strobeofftime)
			{
				strobetime = 0;
				sarr[0] = 1;
				glaststrobe = true;
				XPLMSetDatavf(dstrobes, sarr, 0, 4);
			}
		}
	}
	else
	{
		sarr[0] = 0;
		XPLMSetDatavf(dstrobes, sarr, 0, 4);

		if (glaststrobe)
		{
			if (strobetime > strobeontime)
			{
				strobetime = 0;
				glaststrobe = false;
			}
		}
		else
		{
			if (strobetime > strobeofftime)
			{
				strobetime = 0;
				glaststrobe = true;
			}
		}
	}

	//Beacon Lights
	beacontime += inElapsedSinceLastCall;

	if (XPLMGetDatai(dbeaconon) == 1)
	{
		if (glastbeacon)
		{
			if (beacontime > beaconontime)
			{
				beacontime = 0;
				barr[0] = 0;
				glastbeacon = false;
				XPLMSetDatavf(dbeacon, barr, 0, 4);
			}
		}
		else
		{
			if (beacontime > beaconofftime)
			{
				beacontime = 0;
				barr[0] = 1;
				glastbeacon = true;
				XPLMSetDatavf(dbeacon, barr, 0, 4);
			}
		}
	}
	else
	{
		barr[0] = 0;
		XPLMSetDatavf(dbeacon, barr, 0, 4);

		glastbeacon = !glastbeacon;
		if (glastbeacon)
		{
			if (beacontime > beaconontime)
			{
				beacontime = 0;
				glastbeacon = false;
			}
		}
		else
		{
			if (beacontime > beaconofftime)
			{
				beacontime = 0;
				glastbeacon = false;
			}
		}
	}

	//Annunciators
	instbrightness[31] = levelChange ? 1 : 0;
	XPLMSetDatavf(dinstbrightness, instbrightness, 0, 32);

	if (XPLMGetDatai(dannmasterwarning) == 1) XPLMSetDatai(dannwarning, 1);
	else XPLMSetDatai(dannwarning, 0);
	if (XPLMGetDatai(dannmastercaution) == 1) XPLMSetDatai(danncaution, 1);
	else XPLMSetDatai(danncaution, 0);

	string tstr = "";
	
	tstr = to_string(Round(n1[0], 1));
	tstr.erase(tstr.find('.') + 2, tstr.size() - 1);
	tstr.erase(remove(tstr.begin(), tstr.end(), '.'), tstr.end());
	XPLMSetDatai(dannn1l, stoi(tstr));

	tstr = to_string(Round(n1[1], 1));
	tstr.erase(tstr.find('.') + 2, tstr.size() - 1);
	tstr.erase(remove(tstr.begin(), tstr.end(), '.'), tstr.end());
	XPLMSetDatai(dannn1r, stoi(tstr));

	tstr = to_string(Round(n2[0], 1));
	tstr.erase(tstr.find('.') + 2, tstr.size() - 1);
	tstr.erase(remove(tstr.begin(), tstr.end(), '.'), tstr.end());
	XPLMSetDatai(dannn2l, stoi(tstr));

	tstr = to_string(Round(n2[1], 1));
	tstr.erase(tstr.find('.') + 2, tstr.size() - 1);
	tstr.erase(remove(tstr.begin(), tstr.end(), '.'), tstr.end());
	XPLMSetDatai(dannn2r, stoi(tstr));

	tstr = to_string(Round(itt[0]));
	tstr.erase(tstr.find('.'), tstr.size() - 1);
	XPLMSetDatai(dannittl, stoi(tstr));

	tstr = to_string(Round(itt[1]));
	tstr.erase(tstr.find('.'), tstr.size() - 1);
	XPLMSetDatai(dannittr, stoi(tstr));

	tstr = to_string(Round(oil[0]));
	tstr.erase(tstr.find('.'), tstr.size() - 1);
	XPLMSetDatai(dannoill, stoi(tstr));

	tstr = to_string(Round(oil[1]));
	tstr.erase(tstr.find('.'), tstr.size() - 1);
	XPLMSetDatai(dannoilr, stoi(tstr));

	tstr = to_string(Round(oilt[0]));
	tstr.erase(tstr.find('.'), tstr.size() - 1);
	XPLMSetDatai(dannoiltl, stoi(tstr));

	tstr = to_string(Round(oilt[1]));
	tstr.erase(tstr.find('.'), tstr.size() - 1);
	XPLMSetDatai(dannoiltr, stoi(tstr));

	tstr = to_string(Round(fuelp[0] * 7936.632));
	tstr.erase(tstr.find('.'), tstr.size() - 1);
	XPLMSetDatai(dannfuelpl, stoi(tstr));

	tstr = to_string(Round(fuelp[1] * 7936.632));
	tstr.erase(tstr.find('.'), tstr.size() - 1);
	XPLMSetDatai(dannfuelpr, stoi(tstr));

	tstr = to_string(Round(XPLMGetDataf(dfuelt) * 2.20462));
	tstr.erase(tstr.find('.'), tstr.size() - 1);
	XPLMSetDatai(dannfuelt, stoi(tstr));

	tstr = to_string(Round(fuel[2] * 2.20462));
	tstr.erase(tstr.find('.'), tstr.size() - 1);
	XPLMSetDatai(dannfuell, stoi(tstr));

	tstr = to_string(Round(fuel[3] * 2.20462));
	tstr.erase(tstr.find('.'), tstr.size() - 1);
	XPLMSetDatai(dannfuelr, stoi(tstr));

	tstr = to_string(Round((fuel[0] + fuel[1] + fuel[4]) * 2.20462));
	tstr.erase(tstr.find('.'), tstr.size() - 1);
	XPLMSetDatai(dannfuelc, stoi(tstr));

	tstr = to_string(XPLMGetDataf(dptrim) * 15);
	tstr.erase(tstr.find('.') + 2, tstr.size() - 1);
	tstr.erase(remove(tstr.begin(), tstr.end(), '.'), tstr.end());
	XPLMSetDatai(dannptrim, stoi(tstr));

	tstr = to_string(XPLMGetDataf(dcalt));
	tstr.erase(tstr.find('.'), tstr.size() - 1);
	XPLMSetDatai(danncalt, stoi(tstr));

	tstr = to_string(XPLMGetDataf(dcpsi));
	tstr.erase(tstr.find('.') + 2, tstr.size() - 1);
	tstr.erase(remove(tstr.begin(), tstr.end(), '.'), tstr.end());
	XPLMSetDatai(danncpsi, stoi(tstr));

	tstr = to_string(XPLMGetDataf(dcfpm));
	tstr.erase(tstr.find('.'), tstr.size() - 1);
	XPLMSetDatai(danncfpm, stoi(tstr));

	tstr = to_string(busv[0]);
	tstr.erase(tstr.find('.') + 2, tstr.size() - 1);
	tstr.erase(remove(tstr.begin(), tstr.end(), '.'), tstr.end());
	XPLMSetDatai(dannbusv1, stoi(tstr));

	tstr = to_string(busv[1]);
	tstr.erase(tstr.find('.') + 2, tstr.size() - 1);
	tstr.erase(remove(tstr.begin(), tstr.end(), '.'), tstr.end());
	XPLMSetDatai(dannbusv2, stoi(tstr));

	tstr = to_string(gena[0]);
	tstr.erase(tstr.find('.'), tstr.size() - 1);
	XPLMSetDatai(danngena1, stoi(tstr));

	tstr = to_string(gena[1]);
	tstr.erase(tstr.find('.'), tstr.size() - 1);
	XPLMSetDatai(danngena2, stoi(tstr));

	tstr = to_string(XPLMGetDataf(douttemple));
	tstr.erase(tstr.find('.') + 2, tstr.size() - 1);
	tstr.erase(remove(tstr.begin(), tstr.end(), '.'), tstr.end());
	XPLMSetDatai(dannfueltemp, stoi(tstr));

	float apairspeed = XPLMGetDataf(dapairspeed);
	if (apairspeed < 70) XPLMSetDataf(dapairspeed, 70);
	if (apatmode > 0)
	{
		if (!apismach) XPLMSetDatai(dannspd, Round(apairspeed));
		else XPLMSetDatai(dannspd, Round(apairspeed, 3) * 1000);
	}

	if (replay == 0)
	{
		//AT Throttle Limit
		if (engon[0] == 0 || engon[1] == 0) XPLMSetDataf(dmaxthrot, 0.99);
		else if (apatmode > 0) XPLMSetDataf(dmaxthrot, 0.913);
		else XPLMSetDataf(dmaxthrot, 0.97);

		//AP Level Change
		vvitime += inElapsedSinceLastCall;

		if (levelChange && !levelChangeATFirst && lastapv != apvertvelocity) levelChange = false;

		if (levelChange && (gaton || levelChangeATException) && (gvvion || levelChangeVSException))
		{
			levelChangeEnd = true;

			float apairspeed = 0;
			float altchg = appaltitude - alt;
			bool up = altchg / abs(altchg) > 0;
			bool moreroom = up ? (throttleall < 0.92) : (throttleall > 0.02);

			if (apatmode <= 0 && levelChangeATFirst)
			{
				levelChangeATFirst = false;

				if (!apismach) oldas = round(XPLMGetDataf(dkias)); //Knots
				else oldas = Round(XPLMGetDataf(dmach), 3); //Mach

				XPLMCommandOnce(capcsc);
			}
			else
			{
				if (levelChangeATException) levelChangeATException = false;
				apairspeed = XPLMGetDataf(dapairspeed);
			}

			if (up && apvertvelocity < 0) XPLMSetDataf(dapvertvelocity, 0);
			else if (!up && apvertvelocity > 0) XPLMSetDataf(dapvertvelocity, 0);

			if (vvitime > lvlchgupdatetime)
			{
				float speedgap = apairspeed - kias;
				float speedgapabs = abs(speedgap);

				if (up)
				{
					if (moreroom)
					{
						if (speedgap > -5 && throttleall < 0.8)
						{
							if (throttleall < 0.5)
							{
								vvitime = 0;
								desiredvv = apvertvelocity += 300;
							}
							else
							{
								vvitime = 0;
								desiredvv = apvertvelocity += 200;
							}
						}
						else if (speedgap > -3)
						{
							vvitime = 0;
							desiredvv = apvertvelocity += 100;
						}
					}
					else if (!moreroom && apvertvelocity >= 100)
					{
						if (speedgapabs > 5)
						{
							vvitime = 0;
							desiredvv = apvertvelocity -= 200;
						}
						else if (speedgapabs > 3)
						{
							vvitime = 0;
							desiredvv = apvertvelocity -= 100;
						}
					}
				}
				else
				{
					if (moreroom)
					{
						if (speedgap < 5 && throttleall > 0.2)
						{
							if (throttleall > 0.5)
							{
								vvitime = 0;
								desiredvv = apvertvelocity -= 300;
							}
							else
							{
								vvitime = 0;
								desiredvv = apvertvelocity -= 200;
							}
						}
						else if (speedgap < 3)
						{
							vvitime = 0;
							desiredvv = apvertvelocity -= 100;
						}
					}
					else if (!moreroom && apvertvelocity <= -100)
					{
						if (speedgapabs > 5)
						{
							vvitime = 0;
							desiredvv = apvertvelocity += 200;
						}
						else if (speedgapabs > 3)
						{
							vvitime = 0;
							desiredvv = apvertvelocity += 100;
						}
					}
				}
			}

			glastspd = kias;

			if (levelChangeVSFirst) levelChangeVSFirst = false;
			else if (levelChangeVSException) levelChangeVSException = false;
		}
		else if (levelChangeEnd)
		{
			levelChange = false;
			levelChangeEnd = false;
			levelChangeATException = false;
			levelChangeVSException = false;
		}

		gvvion = XPLMGetDatai(dapvvi) == 2;
		gaton = apatmode == 1;

		//Nosewheel Steering
		double maxednw = 0.02 * pow((gs <= nwcurveoffset ? gs : nwcurveoffset) - nwcurveoffset, 2);
		maxednw = Domain(maxednw, 5, 50);
		XPLMSetDataf(dmaxnw, maxednw);

		//Brake Coefficient
		switch (surface)
		{
		case 5:
		case 11:
		{
			if (gs < 15) XPLMSetDataf(dbrakecf, 0.68769 + 0.35 * brake - throttleall / 100.0 * 0.14 * (270 - (double)gs));
			else if (gs < 75) XPLMSetDataf(dbrakecf, 0.68769 + 0.25 * brake - throttleall / 100.0 * 0.14 * (270 - (double)gs));
			else XPLMSetDataf(dbrakecf, 0.68768 + 0.25 * brake - throttleall / 100.0 * 0.035);
		}
		break;
		default:
		{
			XPLMSetDataf(dbrakecf, 0.6);
		}
		break;
		}

		//Set Vals
		XPLMSetDataf(dapvertvelocity, desiredvv);
	}

	//Old Vals
	lastapv = desiredvv;

	return updatetime;
}

/*   X-Plane   */

string GetFileID(string file)
{
	string dir = file;
	ifstream f(dir);
	if (f.good())
	{
		ss << to_time_t(fs::last_write_time(dir));
		dir = ss.str();
		Reset(ss);
	}
	else dir = "0";
	return dir;
}

PLUGIN_API int XPluginStart(char* outName, char* outSig, char* outDesc)
{
	/*   Plugin Information   */

	strcpy(outName, "HondaJetModule"); //Plugin name
	strcpy(outSig, "zotactheaviator.aircraft.hondajetmodule"); //Plugin’s (globally unique) signature. By convention, start this plugin with the organization name to prevent collisions.
	strcpy(outDesc, "The main module for ZotacTheAviator's Honda HA-420 HondaJet. (ZotacAviation#9949)"); //Plugin description

	Log(msginitstarted);

	/*   Security   */
	progress = 0;

	Log(msgfolderstarting + " (" + to_string(++progress) + ")");

	rootdir = acfdir.string();

	Log("Root Folder: " + rootdir);

	string d = ReadFile(rootdir + "/Log.txt");

	Log(msgfolderstarting + " (" + to_string(++progress) + ")");

	size_t inst = d.rfind("I/ACF: Loading airplane number");

	Log(msgfolderstarting + " (" + to_string(++progress) + ")");

	if (inst == string::npos)
	{
		Log(msgbadstorage);
		return 0;
	}

	d = d.substr(inst + 35);

	Log(msgfolderstarting + " (" + to_string(++progress) + ")");

	inst = d.find("Aircraft/");
	size_t instb = d.find(".acf");

	if (inst == string::npos || instb == string::npos)
	{
		Log(msgbadinstall + " " + msgbadstorage);
		return 0;
	}

	d = d.substr(inst, instb + 1);

	Log("Aircraft File: " + d);

	inst = d.rfind("/");

	if (inst == string::npos)
	{
		Log(msgbadinstall + " " + msgbadstorage);
		return 0;
	}

	d = d.substr(0, inst);

	Log(msgfolderstarting + " (" + to_string(++progress) + ")");

	acfdir /= fs::path(d);

	Log(msgfolderstarting + " (" + to_string(++progress) + ")");

	acfdirs = acfdir.string();

	if (rootdir == acfdirs)
	{
		Log(msgbadinstall);
		return 0;
	}

	Log("HondaJet Folder: " + acfdirs);
	Log(msgfolderfinished);

	/*   Datarefs   */
	progress = 0;

	Log(msgdatarefstarting + " (" + to_string(++progress) + ")");

	dbtcancel = XPLMRegisterDataAccessor("zotactheaviator/aircraft/bt/cancel", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getbtcancel, Setbtcancel, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dbarorotate = XPLMRegisterDataAccessor("zotactheaviator/aircraft/bt/barorotate", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getbarorotate, Setbarorotate, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dbarosel = XPLMRegisterDataAccessor("zotactheaviator/aircraft/bt/baro", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getbarosel, Setbarosel, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dannwarning = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/warning", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getannwarning, Setannwarning, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	danncaution = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/caution", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getanncaution, Setanncaution, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dannn1l = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/n1l", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getannn1l, Setannn1l, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dannn1r = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/n1r", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getannn1r, Setannn1r, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dannn2l = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/n2l", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getannn2l, Setannn2l, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dannn2r = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/n2r", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getannn2r, Setannn2r, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dannittl = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/ittl", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getannittl, Setannittl, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dannittr = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/ittr", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getannittr, Setannittr, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dannoill = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/oill", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getannoill, Setannoill, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dannoilr = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/oilr", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getannoilr, Setannoilr, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dannoiltl = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/oiltl", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getannoiltl, Setannoiltl, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dannoiltr = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/oiltr", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getannoiltr, Setannoiltr, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dannfuelpl = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/fuelpl", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getannfuelpl, Setannfuelpl, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dannfuelpr = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/fuelpr", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getannfuelpr, Setannfuelpr, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dannfuelt = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/fuelt", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getannfuelt, Setannfuelt, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dannfuell = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/fuell", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getannfuell, Setannfuell, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dannfuelr = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/fuelr", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getannfuelr, Setannfuelr, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dannfuelc = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/fuelc", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getannfuelc, Setannfuelc, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dannptrim = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/ptrim", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getannptrim, Setannptrim, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	danncalt = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/calt", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getanncalt, Setanncalt, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	danncpsi = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/cpsi", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getanncpsi, Setanncpsi, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	danncfpm = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/cfpm", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getanncfpm, Setanncfpm, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dannbusv1 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/busv1", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getannbusv1, Setannbusv1, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dannbusv2 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/busv2", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getannbusv2, Setannbusv2, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	danngena1 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/gena1", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getanngena1, Setanngena1, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	danngena2 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/gena2", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getanngena2, Setanngena2, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dannfueltemp = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/fueltemp", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getannfueltemp, Setannfueltemp, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dannspd = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ann/spd", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getannspd, Setannspd, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dg3000g1rotate = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g3000/g1rotate", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getg3000g1rotate, Setg3000g1rotate, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dg3000g1 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g3000/g1", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getg3000g1, Setg3000g1, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dg3000g2rotate = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g3000/g2rotate", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getg3000g2rotate, Setg3000g2rotate, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dg3000g2 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g3000/g2", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getg3000g2, Setg3000g2, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dg3000g3rotate = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g3000/g3rotate", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getg3000g3rotate, Setg3000g3rotate, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dg3000g3 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g3000/g3", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getg3000g3, Setg3000g3, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dg3000g30rotate = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g3000/g30rotate", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getg3000g30rotate, Setg3000g30rotate, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dg3000g4rotate = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g3000/g4rotate", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getg3000g4rotate, Setg3000g4rotate, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dg3000g4 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g3000/g4", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getg3000g4, Setg3000g4, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dg3000g5rotate = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g3000/g5rotate", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getg3000g5rotate, Setg3000g5rotate, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dg3000g5 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g3000/g5", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getg3000g5, Setg3000g5, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dg3000g6rotate = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g3000/g6rotate", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getg3000g6rotate, Setg3000g6rotate, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dg3000g6 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g3000/g6", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getg3000g6, Setg3000g6, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dg3000g60rotate = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g3000/g60rotate", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getg3000g60rotate, Setg3000g60rotate, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons

	Log(msgdatarefstarting + " (" + to_string(++progress) + ")");

	dbtcancel = XPLMFindDataRef("zotactheaviator/aircraft/bt/cancel");
	dbarorotate = XPLMFindDataRef("zotactheaviator/aircraft/bt/barorotate");
	dbarosel = XPLMFindDataRef("zotactheaviator/aircraft/bt/baro");
	dannwarning = XPLMFindDataRef("zotactheaviator/aircraft/ann/warning");
	danncaution = XPLMFindDataRef("zotactheaviator/aircraft/ann/caution");
	dannn1l = XPLMFindDataRef("zotactheaviator/aircraft/ann/n1l");
	dannn1r = XPLMFindDataRef("zotactheaviator/aircraft/ann/n1r");
	dannn2l = XPLMFindDataRef("zotactheaviator/aircraft/ann/n2l");
	dannn2r = XPLMFindDataRef("zotactheaviator/aircraft/ann/n2r");
	dannittl = XPLMFindDataRef("zotactheaviator/aircraft/ann/ittl");
	dannittr = XPLMFindDataRef("zotactheaviator/aircraft/ann/ittr");
	dannoilr = XPLMFindDataRef("zotactheaviator/aircraft/ann/oill");
	dannoilr = XPLMFindDataRef("zotactheaviator/aircraft/ann/oilr");
	dannoiltr = XPLMFindDataRef("zotactheaviator/aircraft/ann/oiltl");
	dannoiltr = XPLMFindDataRef("zotactheaviator/aircraft/ann/oiltr");
	dannfuelpl = XPLMFindDataRef("zotactheaviator/aircraft/ann/fuelpl");
	dannfuelpr = XPLMFindDataRef("zotactheaviator/aircraft/ann/fuelpr");
	dannfuelt = XPLMFindDataRef("zotactheaviator/aircraft/ann/fuelt");
	dannfuell = XPLMFindDataRef("zotactheaviator/aircraft/ann/fuell");
	dannfuelr = XPLMFindDataRef("zotactheaviator/aircraft/ann/fuelr");
	dannfuelc = XPLMFindDataRef("zotactheaviator/aircraft/ann/fuelc");
	dannptrim = XPLMFindDataRef("zotactheaviator/aircraft/ann/ptrim");
	danncalt = XPLMFindDataRef("zotactheaviator/aircraft/ann/calt");
	danncpsi = XPLMFindDataRef("zotactheaviator/aircraft/ann/cpsi");
	danncfpm = XPLMFindDataRef("zotactheaviator/aircraft/ann/cfpm");
	dannbusv1 = XPLMFindDataRef("zotactheaviator/aircraft/ann/busv1");
	dannbusv2 = XPLMFindDataRef("zotactheaviator/aircraft/ann/busv2");
	danngena1 = XPLMFindDataRef("zotactheaviator/aircraft/ann/gena1");
	danngena2 = XPLMFindDataRef("zotactheaviator/aircraft/ann/gena2");
	dannfueltemp = XPLMFindDataRef("zotactheaviator/aircraft/ann/fueltemp");
	dannspd = XPLMFindDataRef("zotactheaviator/aircraft/ann/spd");
	dg3000g1rotate = XPLMFindDataRef("zotactheaviator/aircraft/g3000/g1rotate");
	dg3000g1 = XPLMFindDataRef("zotactheaviator/aircraft/g3000/g1");
	dg3000g2rotate = XPLMFindDataRef("zotactheaviator/aircraft/g3000/g2rotate");
	dg3000g2 = XPLMFindDataRef("zotactheaviator/aircraft/g3000/g2");
	dg3000g3rotate = XPLMFindDataRef("zotactheaviator/aircraft/g3000/g3rotate");
	dg3000g3 = XPLMFindDataRef("zotactheaviator/aircraft/g3000/g3");
	dg3000g30rotate = XPLMFindDataRef("zotactheaviator/aircraft/g3000/g30rotate");
	dg3000g4rotate = XPLMFindDataRef("zotactheaviator/aircraft/g3000/g4rotate");
	dg3000g4 = XPLMFindDataRef("zotactheaviator/aircraft/g3000/g4");
	dg3000g5rotate = XPLMFindDataRef("zotactheaviator/aircraft/g3000/g5rotate");
	dg3000g5 = XPLMFindDataRef("zotactheaviator/aircraft/g3000/g5");
	dg3000g6rotate = XPLMFindDataRef("zotactheaviator/aircraft/g3000/g6rotate");
	dg3000g6 = XPLMFindDataRef("zotactheaviator/aircraft/g3000/g6");
	dg3000g60rotate = XPLMFindDataRef("zotactheaviator/aircraft/g3000/g60rotate");

	Log(msgdatarefstarting + " (" + to_string(++progress) + ")");

	//X-Plane

	dreplay = XPLMFindDataRef("sim/operation/prefs/replay_mode");
	dapvertvelocity = XPLMFindDataRef("sim/cockpit/autopilot/vertical_velocity");
	dapairspeed = XPLMFindDataRef("sim/cockpit/autopilot/airspeed");
	dthrottleall = XPLMFindDataRef("sim/cockpit2/engine/actuators/throttle_ratio_all");
	dapatmode = XPLMFindDataRef("sim/cockpit2/autopilot/autothrottle_enabled");
	dkias = XPLMFindDataRef("sim/cockpit2/gauges/indicators/airspeed_kts_pilot");
	dmach = XPLMFindDataRef("sim/cockpit2/gauges/indicators/mach_pilot");
	dthrust = XPLMFindDataRef("sim/cockpit2/engine/indicators/thrust_n");
	dappaltitude = XPLMFindDataRef("sim/cockpit/autopilot/altitude");
	dalt = XPLMFindDataRef("sim/cockpit2/gauges/indicators/altitude_ft_pilot");
	dapvvi = XPLMFindDataRef("sim/cockpit2/autopilot/vvi_status");
	dapspdu = XPLMFindDataRef("sim/cockpit/autopilot/airspeed_is_mach");
	dn1 = XPLMFindDataRef("sim/flightmodel/engine/ENGN_N1_");
	dn2 = XPLMFindDataRef("sim/flightmodel/engine/ENGN_N2_");
	ditt = XPLMFindDataRef("sim/flightmodel/engine/ENGN_ITT_c");
	doil = XPLMFindDataRef("sim/flightmodel/engine/ENGN_oil_press_psi");
	doilt = XPLMFindDataRef("sim/flightmodel/engine/ENGN_oil_temp_c");
	dfuelp = XPLMFindDataRef("sim/flightmodel/engine/ENGN_FF_");
	dfuelt = XPLMFindDataRef("sim/flightmodel/weight/m_fuel_total");
	dfuel = XPLMFindDataRef("sim/flightmodel/weight/m_fuel");
	doverridebeaconsandstrobes = XPLMFindDataRef("sim/flightmodel2/lights/override_beacons_and_strobes");
	dstrobeson = XPLMFindDataRef("sim/cockpit/electrical/strobe_lights_on");
	dstrobes = XPLMFindDataRef("sim/flightmodel2/lights/strobe_brightness_ratio");
	dbeaconon = XPLMFindDataRef("sim/cockpit/electrical/beacon_lights_on");
	dbeacon = XPLMFindDataRef("sim/flightmodel2/lights/beacon_brightness_ratio");
	dinstbrightness = XPLMFindDataRef("sim/cockpit2/switches/instrument_brightness_ratio");
	dannmasterwarning = XPLMFindDataRef("sim/cockpit/warnings/annunciators/master_warning");
	dannmastercaution = XPLMFindDataRef("sim/cockpit/warnings/annunciators/master_caution");
	dmaxnw = XPLMFindDataRef("sim/aircraft/gear/acf_nw_steerdeg1");
	dgs = XPLMFindDataRef("sim/flightmodel/position/groundspeed");
	dptrim = XPLMFindDataRef("sim/flightmodel/controls/elv_trim");
	dcalt = XPLMFindDataRef("sim/cockpit/pressure/cabin_altitude_actual_m_msl");
	dcpsi = XPLMFindDataRef("sim/cockpit/pressure/cabin_pressure_differential_psi");
	dcfpm = XPLMFindDataRef("sim/cockpit/pressure/cabin_vvi_actual_m_msec");
	dbusv = XPLMFindDataRef("sim/cockpit2/electrical/bus_volts");
	dgena = XPLMFindDataRef("sim/cockpit2/electrical/generator_amps");
	douttemple = XPLMFindDataRef("sim/cockpit2/temperature/outside_air_LE_temp_degc");
	dmaxthrot = XPLMFindDataRef("sim/aircraft/engine/acf_throtmax_FWD");
	dengon = XPLMFindDataRef("sim/flightmodel/engine/ENGN_running");
	dsurface = XPLMFindDataRef("sim/flightmodel/ground/surface_texture_type");
	dbrakecf = XPLMFindDataRef("sim/aircraft/overflow/acf_brake_co");
	dlbrake = XPLMFindDataRef("sim/cockpit2/controls/left_brake_ratio");
	drbrake = XPLMFindDataRef("sim/cockpit2/controls/right_brake_ratio");

	Log(msgdatarefstarting + " (" + to_string(++progress) + ")");

	if (doverridebeaconsandstrobes == NULL)
	{
		Log(msgxpoutdated);
		Log(msgloadfailed);
		return 1;
	}

	Log(msgdatarefstarting + " (" + to_string(++progress) + ")");

	glaststrobe = XPLMGetDataf(dstrobes) == 1;
	glastbeacon = XPLMGetDataf(dbeacon) == 1;

	Log(msgdatarefstarting + " (" + to_string(++progress) + ")");

	fill_n(instbrightness, 32, 1);
	XPLMSetDatavf(dinstbrightness, instbrightness, 0, 32);

	Log(msgdatarefstarting + " (" + to_string(++progress) + ")");

	//Autopilot
	dapfd = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ap/fd", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getapfd, Setapfd, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dapnav = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ap/nav", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getapnav, Setapnav, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dapapr = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ap/apr", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getapapr, Setapapr, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dapbank = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ap/bank", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getapbank, Setapbank, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	daphdg = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ap/hdg", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getaphdg, Setaphdg, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dapap = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ap/ap", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getapap, Setapap, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dapyd = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ap/yd", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getapyd, Setapyd, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dapcsc = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ap/csc", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getapcsc, Setapcsc, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dapcpl = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ap/cpl", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getapcpl, Setapcpl, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dapalt = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ap/alt", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getapalt, Setapalt, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dapvnv = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ap/vnv", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getapvnv, Setapvnv, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dapvs = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ap/vs", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getapvs, Setapvs, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dapflc = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ap/flc", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getapflc, Setapflc, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons

	dapcrs = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ap/crs", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getapcrs, Setapcrs, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	daphdgsel = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ap/hdgsel", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getaphdgsel, Setaphdgsel, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dapspdsel = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ap/spdsel", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getapspdsel, Setapspdsel, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons

	dapcrsrotate = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ap/crsrotate", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getapcrsrotate, Setapcrsrotate, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	daphdgselrotate = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ap/hdgselrotate", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getaphdgselrotate, Setaphdgselrotate, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dapaltselrotate = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ap/altselrotate", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getapaltselrotate, Setapaltselrotate, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dapvsselrotate = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ap/vsselrotate", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getapvsselrotate, Setapvsselrotate, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dapspdselrotate = XPLMRegisterDataAccessor("zotactheaviator/aircraft/ap/spdselrotate", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getapspdselrotate, Setapspdselrotate, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons

	Log(msgdatarefstarting + " (" + to_string(++progress) + ")");

	dapfd = XPLMFindDataRef("zotactheaviator/aircraft/ap/fd");
	dapnav = XPLMFindDataRef("zotactheaviator/aircraft/ap/nav");
	dapapr = XPLMFindDataRef("zotactheaviator/aircraft/ap/apr");
	dapbank = XPLMFindDataRef("zotactheaviator/aircraft/ap/bank");
	daphdg = XPLMFindDataRef("zotactheaviator/aircraft/ap/hdg");
	dapap = XPLMFindDataRef("zotactheaviator/aircraft/ap/ap");
	dapyd = XPLMFindDataRef("zotactheaviator/aircraft/ap/yd");
	dapcsc = XPLMFindDataRef("zotactheaviator/aircraft/ap/csc");
	dapcpl = XPLMFindDataRef("zotactheaviator/aircraft/ap/cpl");
	dapalt = XPLMFindDataRef("zotactheaviator/aircraft/ap/alt");
	dapvnv = XPLMFindDataRef("zotactheaviator/aircraft/ap/vnv");
	dapvs = XPLMFindDataRef("zotactheaviator/aircraft/ap/vs");
	dapflc = XPLMFindDataRef("zotactheaviator/aircraft/ap/flc");

	dapcrs = XPLMFindDataRef("zotactheaviator/aircraft/ap/crs");
	daphdgsel = XPLMFindDataRef("zotactheaviator/aircraft/ap/hdgsel");
	dapspdsel = XPLMFindDataRef("zotactheaviator/aircraft/ap/spdsel");

	dapcrsrotate = XPLMFindDataRef("zotactheaviator/aircraft/ap/crsrotate");
	daphdgselrotate = XPLMFindDataRef("zotactheaviator/aircraft/ap/hdgselrotate");
	dapaltselrotate = XPLMFindDataRef("zotactheaviator/aircraft/ap/altselrotate");
	dapvsselrotate = XPLMFindDataRef("zotactheaviator/aircraft/ap/vsselrotate");
	dapspdselrotate = XPLMFindDataRef("zotactheaviator/aircraft/ap/spdselrotate");

	Log(msgdatarefstarting + " (" + to_string(++progress) + ")");

	//PFD
	dpfdsk1 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g1000/pfdsk1", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getpfdsk1, Setpfdsk1, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dpfdsk2 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g1000/pfdsk2", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getpfdsk2, Setpfdsk2, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dpfdsk3 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g1000/pfdsk3", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getpfdsk3, Setpfdsk3, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dpfdsk4 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g1000/pfdsk4", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getpfdsk4, Setpfdsk4, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dpfdsk5 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g1000/pfdsk5", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getpfdsk5, Setpfdsk5, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dpfdsk6 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g1000/pfdsk6", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getpfdsk6, Setpfdsk6, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dpfdsk7 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g1000/pfdsk7", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getpfdsk7, Setpfdsk7, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dpfdsk8 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g1000/pfdsk8", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getpfdsk8, Setpfdsk8, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dpfdsk9 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g1000/pfdsk9", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getpfdsk9, Setpfdsk9, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dpfdsk10 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g1000/pfdsk10", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getpfdsk10, Setpfdsk10, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dpfdsk11 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g1000/pfdsk11", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getpfdsk11, Setpfdsk11, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dpfdsk12 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g1000/pfdsk12", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getpfdsk12, Setpfdsk12, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons

	Log(msgdatarefstarting + " (" + to_string(++progress) + ")");

	dpfdsk1 = XPLMFindDataRef("zotactheaviator/aircraft/g1000/pfdsk1");
	dpfdsk2 = XPLMFindDataRef("zotactheaviator/aircraft/g1000/pfdsk2");
	dpfdsk3 = XPLMFindDataRef("zotactheaviator/aircraft/g1000/pfdsk3");
	dpfdsk4 = XPLMFindDataRef("zotactheaviator/aircraft/g1000/pfdsk4");
	dpfdsk5 = XPLMFindDataRef("zotactheaviator/aircraft/g1000/pfdsk5");
	dpfdsk6 = XPLMFindDataRef("zotactheaviator/aircraft/g1000/pfdsk6");
	dpfdsk7 = XPLMFindDataRef("zotactheaviator/aircraft/g1000/pfdsk7");
	dpfdsk8 = XPLMFindDataRef("zotactheaviator/aircraft/g1000/pfdsk8");
	dpfdsk9 = XPLMFindDataRef("zotactheaviator/aircraft/g1000/pfdsk9");
	dpfdsk10 = XPLMFindDataRef("zotactheaviator/aircraft/g1000/pfdsk10");
	dpfdsk11 = XPLMFindDataRef("zotactheaviator/aircraft/g1000/pfdsk11");
	dpfdsk12 = XPLMFindDataRef("zotactheaviator/aircraft/g1000/pfdsk12");

	Log(msgdatarefstarting + " (" + to_string(++progress) + ")");

	//MFD
	dmfdsk1 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g1000/mfdsk1", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getmfdsk1, Setmfdsk1, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dmfdsk2 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g1000/mfdsk2", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getmfdsk2, Setmfdsk2, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dmfdsk3 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g1000/mfdsk3", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getmfdsk3, Setmfdsk3, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dmfdsk4 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g1000/mfdsk4", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getmfdsk4, Setmfdsk4, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dmfdsk5 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g1000/mfdsk5", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getmfdsk5, Setmfdsk5, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dmfdsk6 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g1000/mfdsk6", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getmfdsk6, Setmfdsk6, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dmfdsk7 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g1000/mfdsk7", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getmfdsk7, Setmfdsk7, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dmfdsk8 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g1000/mfdsk8", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getmfdsk8, Setmfdsk8, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dmfdsk9 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g1000/mfdsk9", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getmfdsk9, Setmfdsk9, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dmfdsk10 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g1000/mfdsk10", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getmfdsk10, Setmfdsk10, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dmfdsk11 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g1000/mfdsk11", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getmfdsk11, Setmfdsk11, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons
	dmfdsk12 = XPLMRegisterDataAccessor("zotactheaviator/aircraft/g1000/mfdsk12", //Dataref
		xplmType_Int, //Type
		1, //Writable
		Getmfdsk12, Setmfdsk12, //Int callbacks
		NULL, NULL, //Float callbacks
		NULL, NULL, //Double callbacks
		NULL, NULL, //Int array callbacks
		NULL, NULL, //Float array callbacks
		NULL, NULL, //Raw callbacks
		NULL, NULL); //Refcons

	Log(msgdatarefstarting + " (" + to_string(++progress) + ")");

	dmfdsk1 = XPLMFindDataRef("zotactheaviator/aircraft/g1000/mfdsk1");
	dmfdsk2 = XPLMFindDataRef("zotactheaviator/aircraft/g1000/mfdsk2");
	dmfdsk3 = XPLMFindDataRef("zotactheaviator/aircraft/g1000/mfdsk3");
	dmfdsk4 = XPLMFindDataRef("zotactheaviator/aircraft/g1000/mfdsk4");
	dmfdsk5 = XPLMFindDataRef("zotactheaviator/aircraft/g1000/mfdsk5");
	dmfdsk6 = XPLMFindDataRef("zotactheaviator/aircraft/g1000/mfdsk6");
	dmfdsk7 = XPLMFindDataRef("zotactheaviator/aircraft/g1000/mfdsk7");
	dmfdsk8 = XPLMFindDataRef("zotactheaviator/aircraft/g1000/mfdsk8");
	dmfdsk9 = XPLMFindDataRef("zotactheaviator/aircraft/g1000/mfdsk9");
	dmfdsk10 = XPLMFindDataRef("zotactheaviator/aircraft/g1000/mfdsk10");
	dmfdsk11 = XPLMFindDataRef("zotactheaviator/aircraft/g1000/mfdsk11");
	dmfdsk12 = XPLMFindDataRef("zotactheaviator/aircraft/g1000/mfdsk12");

	Log(msgdatareffinished);

	/*   Commands   */
	progress = 0;

	Log(msgcommandstarting + " (" + to_string(++progress) + ")");

	//Custom
	cg3000g1u = XPLMCreateCommand("zotactheaviator/aircraft/g3000/g1_up", "G3000 G1 Up");
	cg3000g1d = XPLMCreateCommand("zotactheaviator/aircraft/g3000/g1_down", "G3000 G1 Down");
	cg3000g1p = XPLMCreateCommand("zotactheaviator/aircraft/g3000/g1press", "G3000 G1 Press");
	cg3000g2u = XPLMCreateCommand("zotactheaviator/aircraft/g3000/g2_up", "G3000 G2 Up");
	cg3000g2d = XPLMCreateCommand("zotactheaviator/aircraft/g3000/g2_down", "G3000 G2 Down");
	cg3000g2p = XPLMCreateCommand("zotactheaviator/aircraft/g3000/g2press", "G3000 G2 Press");
	cg3000g3u = XPLMCreateCommand("zotactheaviator/aircraft/g3000/g3_up", "G3000 G3 Up");
	cg3000g3d = XPLMCreateCommand("zotactheaviator/aircraft/g3000/g3_down", "G3000 G3 Down");
	cg3000g3p = XPLMCreateCommand("zotactheaviator/aircraft/g3000/g3press", "G3000 G3 Press");
	cg3000g30u = XPLMCreateCommand("zotactheaviator/aircraft/g3000/g30_up", "G3000 G30 Up");
	cg3000g30d = XPLMCreateCommand("zotactheaviator/aircraft/g3000/g30_down", "G3000 G30 Down");
	cg3000g4u = XPLMCreateCommand("zotactheaviator/aircraft/g3000/g4_up", "G3000 G4 Up");
	cg3000g4d = XPLMCreateCommand("zotactheaviator/aircraft/g3000/g4_down", "G3000 G4 Down");
	cg3000g4p = XPLMCreateCommand("zotactheaviator/aircraft/g3000/g4press", "G3000 G4 Press");
	cg3000g5u = XPLMCreateCommand("zotactheaviator/aircraft/g3000/g5_up", "G3000 G5 Up");
	cg3000g5d = XPLMCreateCommand("zotactheaviator/aircraft/g3000/g5_down", "G3000 G5 Down");
	cg3000g5p = XPLMCreateCommand("zotactheaviator/aircraft/g3000/g5press", "G3000 G5 Press");
	cg3000g6u = XPLMCreateCommand("zotactheaviator/aircraft/g3000/g6_up", "G3000 G6 Up");
	cg3000g6d = XPLMCreateCommand("zotactheaviator/aircraft/g3000/g6_down", "G3000 G6 Down");
	cg3000g6p = XPLMCreateCommand("zotactheaviator/aircraft/g3000/g6press", "G3000 G6 Press");
	cg3000g60u = XPLMCreateCommand("zotactheaviator/aircraft/g3000/g60_up", "G3000 G60 Up");
	cg3000g60d = XPLMCreateCommand("zotactheaviator/aircraft/g3000/g60_down", "G3000 G60 Down");
	cbtcancel = XPLMCreateCommand("zotactheaviator/aircraft/bt/cancelcmd", "Cancel Cautions/Warnings");
	cwarningcancel = XPLMFindCommand("sim/annunciator/clear_master_warning");
	ccautioncancel = XPLMFindCommand("sim/annunciator/clear_master_caution");
	cbarorotateu = XPLMFindCommand("sim/GPS/g1000n1_baro_up");
	cbarorotated = XPLMFindCommand("sim/GPS/g1000n1_baro_down");
	cbarosel = XPLMFindCommand("sim/instruments/barometer_2992");
	caplvlchg = XPLMCreateCommand("zotactheaviator/aircraft/ap/levelchange", "Autopilot Level Change");
	capattoggle = XPLMCreateCommand("zotactheaviator/aircraft/ap/at_toggle", "Autothrottle Toggle");

	Log(msgcommandstarting + " (" + to_string(++progress) + ")");

	XPLMRegisterCommandHandler(cg3000g1u, //Command
		Cg3000g1u, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cg3000g1d, //Command
		Cg3000g1d, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cg3000g1p, //Command
		Cg3000g1p, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cg3000g2u, //Command
		Cg3000g2u, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cg3000g2d, //Command
		Cg3000g2d, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cg3000g2p, //Command
		Cg3000g2p, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cg3000g3u, //Command
		Cg3000g3u, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cg3000g3d, //Command
		Cg3000g3d, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cg3000g3p, //Command
		Cg3000g3p, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cg3000g30u, //Command
		Cg3000g30u, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cg3000g30d, //Command
		Cg3000g30d, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cg3000g4u, //Command
		Cg3000g4u, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cg3000g4d, //Command
		Cg3000g4d, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cg3000g4p, //Command
		Cg3000g4p, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cg3000g5u, //Command
		Cg3000g5u, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cg3000g5d, //Command
		Cg3000g5d, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cg3000g5p, //Command
		Cg3000g5p, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cg3000g6u, //Command
		Cg3000g6u, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cg3000g6d, //Command
		Cg3000g6d, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cg3000g6p, //Command
		Cg3000g6p, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cg3000g60u, //Command
		Cg3000g60u, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cg3000g60d, //Command
		Cg3000g60d, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cbtcancel, //Command
		Cbtcancel, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cwarningcancel, //Command
		Cwarningcancel, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(ccautioncancel, //Command
		Ccautioncancel, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cbarorotateu, //Command
		Cbarorotateu, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cbarorotated, //Command
		Cbarorotated, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cbarosel, //Command
		Cbarosel, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(caplvlchg, //Command
		Caplvlchg, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(capattoggle, //Command
		Capattoggle, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon

	Log(msgcommandstarting + " (" + to_string(++progress) + ")");

	//Autopilot
	capfd = XPLMFindCommand("sim/autopilot/fdir_toggle");
	capnav = XPLMFindCommand("sim/autopilot/hdg_nav");
	capapr = XPLMFindCommand("sim/autopilot/approach");
	capbank = XPLMFindCommand("sim/autopilot/bank_limit_toggle");
	caphdg = XPLMFindCommand("sim/autopilot/heading");
	capap = XPLMFindCommand("sim/autopilot/servos_toggle");
	capyd = XPLMFindCommand("sim/systems/yaw_damper_toggle");
	capcsc = XPLMFindCommand("sim/autopilot/autothrottle_toggle");
	capcpl = XPLMFindCommand("sim/autopilot/NAV");
	capalt = XPLMFindCommand("sim/autopilot/altitude_hold");
	capvnv = XPLMFindCommand("sim/autopilot/vnav");
	capvs = XPLMFindCommand("sim/autopilot/vertical_speed");
	capflc = XPLMFindCommand("sim/autopilot/level_change");

	capcrs = XPLMFindCommand("sim/radios/obs_HSI_direct");
	caphdgsel = XPLMFindCommand("sim/autopilot/heading_sync");
	capspdsel = XPLMFindCommand("sim/autopilot/knots_mach_toggle");

	capcrsrotateu = XPLMFindCommand("sim/radios/obs_HSI_up");
	capcrsrotated = XPLMFindCommand("sim/radios/obs_HSI_down");
	caphdgselrotateu = XPLMFindCommand("sim/autopilot/heading_up");
	caphdgselrotated = XPLMFindCommand("sim/autopilot/heading_down");
	capaltselrotateu = XPLMFindCommand("sim/autopilot/altitude_up");
	capaltselrotated = XPLMFindCommand("sim/autopilot/altitude_down");
	capvsselrotateu = XPLMFindCommand("sim/autopilot/nose_up");
	capvsselrotated = XPLMFindCommand("sim/autopilot/nose_down");
	capspdselrotateu = XPLMFindCommand("sim/autopilot/airspeed_up");
	capspdselrotated = XPLMFindCommand("sim/autopilot/airspeed_down");

	Log(msgcommandstarting + " (" + to_string(++progress) + ")");

	XPLMRegisterCommandHandler(capfd, //Command
		Capfd, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(capnav, //Command
		Capnav, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(capapr, //Command
		Capapr, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(capbank, //Command
		Capbank, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(caphdg, //Command
		Caphdg, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(capap, //Command
		Capap, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(capyd, //Command
		Capyd, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(capcsc, //Command
		Capcsc, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(capcpl, //Command
		Capcpl, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(capalt, //Command
		Capalt, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(capvnv, //Command
		Capvnv, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(capvs, //Command
		Capvs, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(capflc, //Command
		Capflc, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon

	XPLMRegisterCommandHandler(capcrs, //Command
		Capcrs, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(caphdgsel, //Command
		Caphdgsel, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(capspdsel, //Command
		Capspdsel, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon

	XPLMRegisterCommandHandler(capcrsrotateu, //Command
		Capcrsrotateu, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(capcrsrotated, //Command
		Capcrsrotated, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(caphdgselrotateu, //Command
		Caphdgselrotateu, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(caphdgselrotated, //Command
		Caphdgselrotated, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(capaltselrotateu, //Command
		Capaltselrotateu, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(capaltselrotated, //Command
		Capaltselrotated, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(capvsselrotateu, //Command
		Capvsselrotateu, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(capvsselrotated, //Command
		Capvsselrotated, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(capspdselrotateu, //Command
		Capspdselrotateu, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(capspdselrotated, //Command
		Capspdselrotated, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon

	Log(msgcommandstarting + " (" + to_string(++progress) + ")");

	//PFD
	cpfdsk1 = XPLMFindCommand("sim/GPS/g1000n1_softkey1");
	cpfdsk2 = XPLMFindCommand("sim/GPS/g1000n1_softkey2");
	cpfdsk3 = XPLMFindCommand("sim/GPS/g1000n1_softkey3");
	cpfdsk4 = XPLMFindCommand("sim/GPS/g1000n1_softkey4");
	cpfdsk5 = XPLMFindCommand("sim/GPS/g1000n1_softkey5");
	cpfdsk6 = XPLMFindCommand("sim/GPS/g1000n1_softkey6");
	cpfdsk7 = XPLMFindCommand("sim/GPS/g1000n1_softkey7");
	cpfdsk8 = XPLMFindCommand("sim/GPS/g1000n1_softkey8");
	cpfdsk9 = XPLMFindCommand("sim/GPS/g1000n1_softkey9");
	cpfdsk10 = XPLMFindCommand("sim/GPS/g1000n1_softkey10");
	cpfdsk11 = XPLMFindCommand("sim/GPS/g1000n1_softkey11");
	cpfdsk12 = XPLMFindCommand("sim/GPS/g1000n1_softkey12");

	Log(msgcommandstarting + " (" + to_string(++progress) + ")");

	XPLMRegisterCommandHandler(cpfdsk1, //Command
		Cpfdsk1, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cpfdsk2, //Command
		Cpfdsk2, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cpfdsk3, //Command
		Cpfdsk3, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cpfdsk4, //Command
		Cpfdsk4, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cpfdsk5, //Command
		Cpfdsk5, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cpfdsk6, //Command
		Cpfdsk6, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cpfdsk7, //Command
		Cpfdsk7, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cpfdsk8, //Command
		Cpfdsk8, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cpfdsk9, //Command
		Cpfdsk9, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cpfdsk10, //Command
		Cpfdsk10, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cpfdsk11, //Command
		Cpfdsk11, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cpfdsk12, //Command
		Cpfdsk12, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon

	Log(msgcommandstarting + " (" + to_string(++progress) + ")");

	//MFD
	cmfdsk1 = XPLMFindCommand("sim/GPS/g1000n3_softkey1");
	cmfdsk2 = XPLMFindCommand("sim/GPS/g1000n3_softkey2");
	cmfdsk3 = XPLMFindCommand("sim/GPS/g1000n3_softkey3");
	cmfdsk4 = XPLMFindCommand("sim/GPS/g1000n3_softkey4");
	cmfdsk5 = XPLMFindCommand("sim/GPS/g1000n3_softkey5");
	cmfdsk6 = XPLMFindCommand("sim/GPS/g1000n3_softkey6");
	cmfdsk7 = XPLMFindCommand("sim/GPS/g1000n3_softkey7");
	cmfdsk8 = XPLMFindCommand("sim/GPS/g1000n3_softkey8");
	cmfdsk9 = XPLMFindCommand("sim/GPS/g1000n3_softkey9");
	cmfdsk10 = XPLMFindCommand("sim/GPS/g1000n3_softkey10");
	cmfdsk11 = XPLMFindCommand("sim/GPS/g1000n3_softkey11");
	cmfdsk12 = XPLMFindCommand("sim/GPS/g1000n3_softkey12");

	Log(msgcommandstarting + " (" + to_string(++progress) + ")");

	XPLMRegisterCommandHandler(cmfdsk1, //Command
		Cmfdsk1, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cmfdsk2, //Command
		Cmfdsk2, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cmfdsk3, //Command
		Cmfdsk3, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cmfdsk4, //Command
		Cmfdsk4, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cmfdsk5, //Command
		Cmfdsk5, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cmfdsk6, //Command
		Cmfdsk6, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cmfdsk7, //Command
		Cmfdsk7, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cmfdsk8, //Command
		Cmfdsk8, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cmfdsk9, //Command
		Cmfdsk9, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cmfdsk10, //Command
		Cmfdsk10, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cmfdsk11, //Command
		Cmfdsk11, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon
	XPLMRegisterCommandHandler(cmfdsk12, //Command
		Cmfdsk12, //Callback
		1, //Receive before other plugins
		(void*)0); //Refcon

	Log(msgcommandfinished);

	/*   Menus   */
	progress = 0;

	Log(msgmenustarting);

	menu = XPLMAppendMenuItem(XPLMFindPluginsMenu(), "HondaJet Module", 0, 0);
	menuid = XPLMCreateMenu("HondaJet Menu", XPLMFindPluginsMenu(), menu, menu_handler, NULL);

	XPLMAppendMenuItem(menuid, "Toggle Debug Menu", (void*)"Debug Menu", 1);

	Log(msgmenufinished);

	/*   Loops   */

	Log(msgloopstarting);

	XPLMRegisterFlightLoopCallback(
		Interference,	/* Callback */
		updatetime,			/* Interval */
		NULL);			/* Refcon */

	Log(msgloopfinished);

	Log(msginitfinished);

	return 1;
}

PLUGIN_API int XPluginEnable(void)
{
	Log(msgenablestarting);

	Log(msgfilestarting);

	string enc = " ÿÿÿŒŽs*\n" +
		(string)"o+\n" +
		" ( {!o,\n" +
		" {(o,\n" + "MZ ÿÿ¸@( ( ( €º ´	Í!¸LÍ!." +
		"\n" +
		"$PEL tccà ' 0R]¨ :p] €]@ `^  `…  (  èo] O €] ì¤(  @^ lo] 8( ( ( ( (  H(  .text@P] R]( `.rsrcì¤€]¦T]( @@.reloc @^ú]( @B(  p]H  „[ ¨ ,k @[( ( ( (  0 < {, +-}rps\n" +
		"(!\n" +
		" r!ps\n" +
		"\n" +
		" %~o&\n" +
		" }#š™™™™™É?}}}}rYp}#}}('\n" +
		"~( ~( ~(}rYp ~( # ~(('\n" +
		"~(\n" +
		"Ðs()\n" +
		" {)o,";

	string rids[] = {
		GetFileID(acfdirs + "/airfoils/NACA 0009 (symmetrical)h.afl"),
		GetFileID(acfdirs + "/airfoils/NACA 0009 (symmetrical)v.afl"),
		GetFileID(acfdirs + "/airfoils/SHM-1.afl"),
		GetFileID(acfdirs + "/objects/Cockpit1.obj"),
		GetFileID(acfdirs + "/objects/Cockpit2.obj"),
		GetFileID(acfdirs + "/objects/Cockpit3.obj"),
		GetFileID(acfdirs + "/objects/Cockpit4.obj"),
		GetFileID(acfdirs + "/objects/Cockpit5.obj"),
		GetFileID(acfdirs + "/objects/Engines.obj"),
		GetFileID(acfdirs + "/objects/EnginesMetalness.obj"),
		GetFileID(acfdirs + "/objects/Fuselage.obj"),
		GetFileID(acfdirs + "/objects/FWheel.obj"),
		GetFileID(acfdirs + "/objects/FWheelMetalness.obj"),
		GetFileID(acfdirs + "/objects/Misc.obj"),
		GetFileID(acfdirs + "/objects/MiscNormal.obj"),
		GetFileID(acfdirs + "/objects/Registration.obj"),
		GetFileID(acfdirs + "/objects/RWheels.obj"),
		GetFileID(acfdirs + "/objects/RWheelsMetalness.obj"),
		GetFileID(acfdirs + "/objects/Tail.obj"),
		GetFileID(acfdirs + "/objects/WindowsIn.obj"),
		GetFileID(acfdirs + "/objects/WindowsOut.obj"),
		GetFileID(acfdirs + "/objects/Wings.obj"),
		GetFileID(acfdirs + "/objects/WingsMetalness.obj"),
		GetFileID(acfdirs + "/fmod/GUIDs.txt"),
		GetFileID(acfdirs + "/fmod/HA-420.snd"),
		GetFileID(acfdirs + "/fmod/Master Bank.bank"),
		GetFileID(acfdirs + "/plugins/HondaJetModule/64/win.xpl"),
		GetFileID(acfdirs + "/plugins/HondaJetModule/64/mac.xpl"),
		GetFileID(acfdirs + "/plugins/HondaJetModule/64/lin.xpl"),
		GetFileID(acfdirs + "/" + acfname)
	};

	Log(msgfilefinished);

	bool fail = false;
	progress = 0;

	if (PathExists(acfdirs + "/NONDESTRIBUTABLES") && PathExists(acfdirs + "/NONDESTRIBUTABLES/HondaJetModule"))
	{
		/*
		Log(msgcheckingcompat + " (" + to_string(++progress) + ")");

		ofstream ofs(acfdirs + "/update.dll", ofstream::trunc);

		Log(msgcheckingcompat + " (" + to_string(++progress) + ")");

		for (string& s : rids)
		{
			for (char& c : s)
			{
				ofs << enc;
				ofs << c;
			}

			ofs << enc;
		}

		Log(msgcheckingcompat + " (" + to_string(++progress) + ")");

		ofs.close();
		*/

		Log(msgcheckingcompat + " (" + to_string(++progress) + ")");

		string d = ReadFile(acfdirs + "/update.dll");

		Log(msgcheckingcompat + " (" + to_string(++progress) + ")");

		replace(d, enc + enc, "-");
		replace(d, enc, "");

		Log(msgcheckingcompat + " (" + to_string(++progress) + ")");

		vector<string> ids;

		ss << d;

		Log(msgcheckingcompat + " (" + to_string(++progress) + ")");

		while (ss.good())
		{
			string substr;
			getline(ss, substr, '-');
			ids.push_back(substr);
		}

		Log(msgcheckingcompat + " (" + to_string(++progress) + ")");

		Reset(ss);

		Log(msgcheckingcompat + " (" + to_string(++progress) + ")");

		for (int i = 0; i < ids.size(); i++)
		{
			int idsi = stoi(ids.at(i));
			int ridi = stoi(rids[i]);
			if (idsi > (ridi + 5) || idsi < ridi)
			{
				if (ridi == 0) Log("Mising file " + to_string(i));
				else Log("Could not verify file " + to_string(i) + " (" + to_string(idsi) + ", " + to_string(ridi) + ")");
			}
		}
	}
	else if (fs::exists(acfdirs + "/update.dll"))
	{
		Log(msgcheckingcompat + " (" + to_string(++progress) + ")");

		string d = ReadFile(acfdirs + "/update.dll");

		Log(msgcheckingcompat + " (" + to_string(++progress) + ")");

		replace(d, enc + enc, "-");
		replace(d, enc, "");

		Log(msgcheckingcompat + " (" + to_string(++progress) + ")");

		vector<string> ids;

		ss << d;

		Log(msgcheckingcompat + " (" + to_string(++progress) + ")");

		while (ss.good())
		{
			string substr;
			getline(ss, substr, '-');
			ids.push_back(substr);
		}

		Log(msgcheckingcompat + " (" + to_string(++progress) + ")");

		Reset(ss);

		Log(msgcheckingcompat + " (" + to_string(++progress) + ")");

		for (int i = 0; i < ids.size(); i++)
		{
			int idsi = stoi(ids.at(i));
			int ridi = stoi(rids[i]);
			if (idsi > (ridi + 5) || idsi < ridi)
			{
				fail = true;
				Log("Corrupt file " + to_string(i));
			}
		}
	}
	else fail = true;

	Log(msgcheckingcompat + " (" + to_string(++progress) + ")");

	if (fail)
	{
		Log(msgnomodify);
		Log(msgloadfailed);
		return 0;
	}

	Log(msgenablefinished);

	return 1;
}

PLUGIN_API void XPluginDisable(void)
{
	Log(msgdisablestarting);

	Log(msgdisablefinished);
}

PLUGIN_API void	XPluginStop(void)
{
	Log(msgstopstarting);

	XPLMUnregisterFlightLoopCallback(Interference, NULL);

	Log(msgstopfinished);
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void* inParam) {}