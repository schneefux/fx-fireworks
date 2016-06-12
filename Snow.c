#include "mathf.h"

char *Disp_GetVRAMPtr(void);
int Timer_Install(int TimerID, void (*handler)(void), int delay);
int Timer_Start(int TimerID);
void App_PROG();
int RTC_GetTicks();
void AllClear_VRAM();

typedef struct{
    unsigned int x1;
    unsigned int y1;
    unsigned int x2;
    unsigned int y2;
    unsigned char f[4];
    unsigned int on_bits;
    unsigned int off_bits;
} TShape;

void ShapeToVRAM( TShape* x ); 

char *vram;

const int SysCallWrapper[] = {0xD201422B,0x60F20000,0x80010070};
const int (*iSysCallFuncPtr)( int R4, int R5, int R6, int R7, int FNo ) = (void*)&SysCallWrapper;
#define Alpha_GetData(VarName,Dest) (void)(*iSysCallFuncPtr)( VarName,(int)Dest, 0, 0, 0x04DF)
#define Alpha_ClearAll() (void)(*iSysCallFuncPtr)( 0,0, 0, 0, 0x004E1)

int InputString( unsigned char*buff, unsigned char *heading, int maxlen );

static unsigned int lastrandom=0x12345678;

unsigned int random( int seed){
    if (seed) lastrandom=seed;
    lastrandom = ( 0x41C64E6D*lastrandom ) + 0x3039;
    return ( lastrandom >> 16 );
}

const char sprite[9][3] = {
{ 0 , 1 , 0 },
{ 1 , 1 , 1 },
{ 1 , 0 , 1 },
{ 1 , 0 , 1 },
{ 1 , 0 , 1 },
{ 1 , 1 , 1 },
{ 0 , 0 , 1 },
{ 0 , 0 , 1 },
{ 0 , 0 , 1 }
};

#define abs(x) (x < 0)?-x:x
#define sgn(x) (x < 0)?-1:(x == 0)?0:1

void setpixel(unsigned char x, char y)
{
	TShape t;
	if(x&~127 || y&~63) return;
	t.x1 = x;
	t.y1 = y;
	t.f[0] = 2;
	t.f[1] = 1;
	t.f[2] = 1;
	t.f[3] = 1;
	
	ShapeToDD(&t);
}

void line(int x1, int y1, int x2, int y2)
{
	int i, x, y, dx, dy, sx, sy, cumul;
	x = x1;
	y = y1;
	dx = x2 - x1;
	dy = y2 - y1;
	sx = sgn(dx);
	sy = sgn(dy);
	dx = abs(dx);
	dy = abs(dy);
	setpixel(x, y);
	if(dx > dy)
	{
		cumul = dx / 2;
		for(i=1 ; i<dx ; i++)
		{
			x += sx;
			cumul += dy;
			if(cumul > dx)
			{
				cumul -= dx;
				y += sy;
			}
			setpixel(x, y);
		}
	}
	else
	{
		cumul = dy / 2;
		for(i=1 ; i<dy ; i++)
		{
			y += sy;
			cumul += dx;
			if(cumul > dy)
			{
				cumul -= dy;
				x += sx;
			}
			setpixel(x, y);
		}
	}
}

void drawsprite(unsigned char px, char py, float dirx)
{
	char x;
	float y;
	for(y = 0; y < 9; y += 1)
	for(x = 0; x < 3; x++)
		if(sprite[(int)y][x] == 1)
			setpixel(px + x + ((9 - (int)y) * dirx), py + (int)y);
}

#define P2I  6.28318531

void drawboom(unsigned char px, char py, char boom, float radius)
{
	float cnt;
	
	for(cnt = 0; cnt < P2I; cnt += radius)
		line(px, py, px + (int)(cosf(cnt) * boom), py + (int)(sinf(cnt) * boom));
}

#define CNT 15

struct {unsigned char active;char boom;float dirx, y, x, radius;} fire[CNT];

void display_vram()
{
	char *LCD_register_selector = (char*)0xB4000000, *LCD_data_register = (char*)0xB4010000, *mvram;
	int i, j;
	mvram = vram;
	for(i=0 ; i<64 ; i++)
	{
		*LCD_register_selector = 4;
		*LCD_data_register = i|192;
		*LCD_register_selector = 4;
		*LCD_data_register = 0;
		*LCD_register_selector = 7;
		for(j=0 ; j<16 ; j++) *LCD_data_register = *mvram++;
	}
}

char isMainMenu()
{
	unsigned int ea;
	unsigned int j;
	ea = *(unsigned int*)0x8001007C;
	ea += 0x0490*4;
	ea = *(unsigned int*)( ea );
	j = *(unsigned char*)( ea + 1 );
	j *= 4;
	j = ( ea + j + 4 ) & 0xFFFFFFFC;
	j = *(unsigned int*)( j ) + 1;
	
	return *(unsigned char*)j;
}

char variable[24]={0};

void snowtimer(void)
{
	unsigned char x;
	
	if(!isMainMenu())
	{
		display_vram();
		
		for(x =CNT - 1; x > 0 ; x--)
		{
			if(fire[x].active == 1)
			{
				if(fire[x].boom == 0)
				{
					drawsprite(fire[x].x, fire[x].y, fire[x].dirx);
					fire[x].y -= fire[x].y / 64;
					fire[x].x += fire[x].dirx;
					if(fire[x].y == 0 || fire[x].x > 127 || fire[x].x < 0)
						fire[x].active = 0;
					if(random(0) % (int)fire[x].y == 0 && fire[x].boom == 0)
						fire[x].boom++;
				}
				else
				{
					drawboom(fire[x].x, fire[x].y, fire[x].boom++, fire[x].radius);
					if(fire[x].boom >= (random(0) % 16) + 16)
						fire[x].active = 0;
				}
			}
			else
			{
				if(random(0) % 32 == 0)
				{
					fire[x].active = 1;
					fire[x].x = random(0) % 128;
					fire[x].dirx = (random(0) % 20) / 10.0 - 1.0;
					fire[x].y = 64;
					fire[x].boom = 0;
					fire[x].radius = P2I / ((random(0) % 16) + 8);
				}
			}
		}	
	}
	
	Alpha_GetData('A',variable);
	if(variable[0])
	{
		KillTimer(1);
	}
}

int AddIn_main(int isAppli, unsigned short OptionNum)
{
	unsigned int key;
	unsigned char text[3] = {0xE6, 0x91, 0};
	random(RTC_GetTicks());
	vram = Disp_GetVRAMPtr();
	Alpha_ClearAll();
	AllClr_VRAM();
	locate(3,3);
	Print(&"~*| Firework |*~");
	locate(4,1);
	Print(&"(C) by Casimo");
	locate(4,5);
	Print(&"press any key");
	locate(6,6);
	Print(&"to start");
	locate(4,8);
	Print(&"exit with 1 A");
	locate(15,8);
	Print(&text);
	display_vram();
	GetKey(&key);
	Timer_Install(6, &snowtimer, 300);
	Timer_Start(6);
	App_PROG();
}

#pragma section _BR_Size
unsigned long BR_Size;
#pragma section
#pragma section _TOP
int InitializeSystem(int isAppli, unsigned short OptionNum)
{return INIT_ADDIN_APPLICATION(isAppli, OptionNum);}
#pragma section

