#pragma once
//setttings

//controller
int AxisLeftsens;
int AxisRightsens;
int AxisUpsens;
int AxisDownsens;
int scrollspeed3;


int InitialMode;
int Modechange;

int sendfocus;
int responsetime;
int doubleclicks;
int scrollenddelay;
int quickMW;

int ShoulderNextbmp;

int scrolloutsidewindow;
int mode = InitialMode;
/////////////////
bool movedmouse;

HMODULE g_hModule = nullptr;

typedef BOOL(WINAPI* GetCursorPos_t)(LPPOINT lpPoint);
typedef BOOL(WINAPI* SetCursorPos_t)(int X, int Y);

typedef SHORT(WINAPI* GetAsyncKeyState_t)(int vKey);
typedef SHORT(WINAPI* GetKeyState_t)(int nVirtKey);
typedef BOOL(WINAPI* ClipCursor_t)(const RECT*);
typedef HCURSOR(WINAPI* SetCursor_t)(HCURSOR hCursor);

typedef BOOL(WINAPI* SetRect_t)(LPRECT lprc, int xLeft, int yTop, int xRight, int yBottom);
typedef BOOL(WINAPI* AdjustWindowRect_t)(LPRECT lprc, DWORD  dwStyle, BOOL bMenu);



CRITICAL_SECTION critical; //window thread
//CRITICAL_SECTION criticalA; //Scannning thread

GetCursorPos_t fpGetCursorPos = nullptr;
GetCursorPos_t fpSetCursorPos = nullptr;
GetAsyncKeyState_t fpGetAsyncKeyState = nullptr;
GetKeyState_t fpGetKeyState = nullptr;
ClipCursor_t fpClipCursor = nullptr;
SetCursor_t fpSetCursor = nullptr;
SetRect_t fpSetRect = nullptr;
AdjustWindowRect_t fpAdjustWindowRect = nullptr;



//POINT fakecursor;
POINT fakecursorW;
POINT startdrag;
POINT activatewindow;
POINT scroll;
bool loop = true;
HWND hwnd;
int showmessage = 0; //0 = no message, 1 = initializing, 2 = bmp mode, 3 = bmp and cursor mode, 4 = edit mode   
int showmessageW = 0; //0 = no message, 1 = initializing, 2 = bmp mode, 3 = bmp and cursor mode, 4 = edit mode 
int counter = 0;

//syncronization control
HANDLE hMutex;

int getmouseonkey = 0;
int message = 0;
auto hInstance = nullptr;

//hooks
bool hooksinited = false;
int keystatesend = 0; //key to send
int clipcursorhook = 0;
int getkeystatehook = 0;
int getasynckeystatehook = 0;
int getcursorposhook = 0;
int setcursorposhook = 0;
int setcursorhook = 0;

int ignorerect = 0;
POINT rectignore = { 0,0 }; //for getcursorposhook
int setrecthook = 0;

int leftrect = 0;
int toprect = 0;
int rightrect = 0;
int bottomrect = 0;

int userealmouse = 0;

int resize = 1;
int numphotoA = -1;
int numphotoB = -1;
int numphotoX = -1;
int numphotoY = -1;
int numphotoC = -1;
int numphotoD = -1;
int numphotoE = -1;
int numphotoF = -1;

int numphotoAbmps;
int numphotoBbmps;
int numphotoXbmps;
int numphotoYbmps;

bool AuseStatic = 1;
bool BuseStatic = 1;
bool XuseStatic = 1;
bool YuseStatic = 1;
//int needatick = 0;
//bool movedmouse = false;
//std::vector<POINT> staticPointA;
//std::vector<POINT> staticPointB;
//std::vector<POINT> staticPointX;
//std::vector<POINT> staticPointY;

//fake cursor
int controllerID = 0;
int Xf = 20;
int Yf = 20;
int OldX = 0;
int OldY = 0;
int ydrag;
int xdrag;
int Xoffset = 0; //offset for cursor    
int Yoffset = 0;
bool scrollmap = false;
bool pausedraw = false;
bool gotcursoryet = false;
int drawfakecursor = 0;
int alwaysdrawcursor = 0; //always draw cursor even if setcursor set cursor NULL
HICON hCursor = 0;
DWORD lastClickTime;
HDC PointerWnd;
int WoldX, WoldY;
//bmp search
bool foundit = false;

int cursoroffsetx, cursoroffsety;
int offsetSET; //0:sizing 1:offset 2:done
int cursorWidth = 40;
int cursorHeight = 40;
HWND pointerWindow = nullptr;
bool DrawFakeCursorFix = false;
static int transparencyKey = RGB(0, 0, 1);

HCURSOR oldhCursor = NULL;
HCURSOR hCursorW = NULL;
bool nochange = false;

bool oldHadShowCursor = true;


//scroll type 3
int tick = 0;
bool doscrollyes = false;

// 
//beautiful cursor
int colorfulSword[20][20] = {
{1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
{1,2,2,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
{1,2,2,2,2,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0},
{1,2,2,2,2,2,2,1,1,0,0,0,0,0,0,0,0,0,0,0},
{1,2,2,2,2,2,2,2,2,1,1,0,0,0,0,0,0,0,0,0},
{1,2,2,2,2,2,2,2,2,2,2,1,1,0,0,0,0,0,0,0},
{1,2,2,2,2,2,2,2,2,2,2,2,2,1,1,0,0,0,0,0},
{1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,1,0,0,0},
{1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,0,0},
{1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1,0,0,0},
{1,2,2,2,2,2,2,2,2,2,2,2,2,1,1,1,0,0,0,0},
{1,2,2,2,2,2,2,2,2,2,2,2,2,1,0,0,0,0,0,0},
{1,2,2,2,2,2,2,2,2,2,2,2,1,0,0,0,0,0,0,0},
{1,2,2,2,2,2,2,2,2,2,2,2,1,0,0,0,0,0,0,0},
{1,2,2,2,2,2,2,1,1,2,2,2,1,0,0,0,0,0,0,0},
{1,2,2,2,2,2,1,0,0,1,2,2,2,2,1,0,0,0,0,0},
{1,2,2,2,2,1,0,0,0,0,1,2,2,2,1,0,0,0,0,0},
{1,1,2,2,1,0,0,0,0,0,0,1,2,2,2,1,0,0,0,0},
{1,2,2,1,0,0,0,0,0,0,0,1,2,2,2,1,0,0,0,0},
{1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0},
};
//temporary cursor on success

COLORREF colors[5] = {
    RGB(0, 0, 0),          // Transparent - won't be drawn
    RGB(140, 140, 140),    // Gray for blade
    RGB(255, 255, 255),    // White shine
    RGB(139, 69, 19),       // Brown handle
    RGB(50, 150, 255)     // Light blue accent

};


bool onoroff = true;

//remember old keystates
int oldscrollrightaxis = false; //reset 
int oldscrollleftaxis = false; //reset 
int oldscrollupaxis = false; //reset 
int oldscrolldownaxis = false; //reset 
bool Apressed = false;
bool Bpressed = false;
bool Xpressed = false;
bool Ypressed = false;
bool leftPressedold;
bool rightPressedold;
bool oldA = false;
bool oldB = false;
bool oldX = false;
bool oldY = false;
bool oldC = false;
bool oldD = false;
bool oldE = false;
bool oldF = false;
bool oldup = false;
bool olddown = false;
bool oldleft = false;
bool oldright = false;

int startsearch = 0;
int startsearchA = 0;
int startsearchB = 0;
int startsearchX = 0;
int startsearchY = 0;
int startsearchC = 0;
int startsearchD = 0;
int startsearchE = 0;
int startsearchF = 0;

int righthanded = 0;
int scanoption = 0;

int Atype = 0;
int Btype = 0;
int Xtype = 0;
int Ytype = 0;
int Ctype = 0;
int Dtype = 0;
int Etype = 0;
int Ftype = 0;

POINT PointA;
POINT PointB;
POINT PointX;
POINT PointY;
int scantick = 0;

int bmpAtype = 0;
int bmpBtype = 0;
int bmpXtype = 0;
int bmpYtype = 0;
int bmpCtype = 0;
int bmpDtype = 0;
int bmpEtype = 0;
int bmpFtype = 0;

int uptype = 0;
int downtype = 0;
int lefttype = 0;
int righttype = 0;


int x = 0;

HBITMAP hbm;

std::vector<BYTE> largePixels, smallPixels;
SIZE screenSize;
int strideLarge, strideSmall;
int smallW, smallH;

//int sovetid = 16;
int knappsovetid = 100;

int samekey = 0;
int samekeyA = 0;