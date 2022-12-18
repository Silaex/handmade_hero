#include <windows.h>
#include <stdint.h>

#define internal static; // Internal function
#define local_persist static;
#define global_variable static;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

// TODO(Alexis): This is a global for now.
struct win32_offscreen_buffer
{
	BITMAPINFO BitmapInfo;
	void *BitmapMemory;
	int BitmapWidth;
	int BitmapHeight;
	int Pitch;
	int BytesPerPixel;
}

global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackBuffer;

internal void
RenderWeirdGradient(win32_offscreen_buffer Buffer, int xOffset, int yOffset)
{
	uint8 *Row = (uint8 *)Buffer.BitmapMemory;

	for (int Y = 0; Y < Buffer.BitmapHeight; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int X = 0; X < Buffer.BitmapWidth; ++X)
		{
			uint8 Blue = (X + xOffset) * Y;
			uint8 Green = (Y + yOffset) * X;

			*Pixel = ((Green << 8) | Blue);
			++Pixel;
		}

		Row += Buffer.Pitch;
	}
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
	// TODO(Alexis): Bulletproof this.
	// Maybe don't free first, free after, teh free first if that fails

	if (Buffer->BitmapMemory)
	{
		// Freeing the memory
		VirtualFree(Buffer->BitmapMemory, 0, MEM_RELEASE);
	}

	Buffer->BitmapWidth = Width;
	Buffer->BitmapHeight = Height;
	Buffer->BytesPerPixel = 4;

	Buffer->BitmapInfo.bmiHeader.biSize = sizeof(Buffer->BitmapInfo.bmiHeader);
	Buffer->BitmapInfo.bmiHeader.biWidth = Buffer->BitmapWidth;
	Buffer->BitmapInfo.bmiHeader.biHeight = -Buffer->BitmapHeight;
	Buffer->BitmapInfo.bmiHeader.biPlanes = 1;
	Buffer->BitmapInfo.bmiHeader.biBitCount = 32;
	Buffer->BitmapInfo.bmiHeader.biCompression = BI_RGB;

	// Allocate our own memory
	int BitmapMemorySize = (Buffer->BitmapWidth * Buffer->BitmapHeight) * Buffer->BytesPerPixel;
	Buffer->BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

	Buffer->Pitch = Buffer->BitmapWidth * Buffer->BytesPerPixel;
}

internal void
Win32DisplayBufferToWindow(HDC DeviceContext, RECT ClientRect, 
							win32_offscreen_buffer Buffer,
							int X, int Y, int Width, int Height)
{

	int WindowWidth = ClientRect.right - ClientRect.left;
	int WindowHeight = ClientRect.bottom - ClientRect.top; 
	StretchDIBits(DeviceContext,
					/*
					X, Y, Width, Height
					X, Y, Width, Height
					*/
					0, 0, Buffer.BitmapWidth, Buffer.BitmapHeight,
					0, 0, WindowWidth, WindowHeight,
	  				Buffer.BitmapMemory,
	  				&Buffer.BitmapInfo,
	  				DIB_RGB_COLORS, SRCCOPY);
}

LRESULT CALLBACK 
Win32MainWindowCallback(HWND Window,
	  					UINT Message,
						WPARAM WParam,
	  					LPARAM LParam) // WindowProc function
{
	LRESULT Result = 0;

	switch(Message)
	{
		case WM_SIZE:
		{
			RECT ClientRect;
			GetClientRect(Window, &ClientRect);
			int Width = ClientRect.right - ClientRect.left;
			int Height = ClientRect.bottom - ClientRect.top;
			Win32ResizeDIBSection(&GlobalBackBuffer, Width, Height);
		} break;

		case WM_DESTROY:
		{
			Running = false;
		}break;

		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVEAPP\n");
		}break;

		case WM_CLOSE:
		{
			Running = false;
		}break;

		case WM_PAINT:
		{
			PAINTSTRUCT Paint;
			HDC DeviceContext = BeginPaint(Window, &Paint);
			local_persist DWORD Operation = WHITENESS;

			int X = Paint.rcPaint.left;
			int Y = Paint.rcPaint.top;
			int Width = Paint.rcPaint.right - Paint.rcPaint.left;
			int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;

			RECT ClientRect;
			GetClientRect(Window, &ClientRect);

			Win32DisplayBufferToWindow(DeviceContext, ClientRect, GlobalBackBuffer, X, Y, Width, Height);
			EndPaint(Window, &Paint);
		}break;

		default:
		{
//			OutputDebugStringA("default\n");
			Result = DefWindowProc(Window, Message, WParam, LParam);
		}break;
	}

	return(Result);
}

int CALLBACK WinMain(
	HINSTANCE Instance,
	HINSTANCE PrevInstance,
	LPSTR CmdLine,
	int nCmdShow
)
{
	WNDCLASS WindowClass = {0};

	WindowClass.style = CS_OWNDC;
  	WindowClass.lpfnWndProc = Win32MainWindowCallback;
  	WindowClass.hInstance = Instance;
//  	WindowClass.hIcon;
  	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

  	ATOM regClass = RegisterClass(&WindowClass);

  	if(regClass)
  	{
  		HWND Window = 
  			CreateWindowExA(
  				0,								// Window Extended Style
  			 	WindowClass.lpszClassName,		// Window Class Name
  			 	"Handmade Hero",				// Window Name
             	WS_OVERLAPPEDWINDOW|WS_VISIBLE, // Window Style
             	CW_USEDEFAULT,					// X Coordinate
             	CW_USEDEFAULT,					// Y Coordinate
             	CW_USEDEFAULT,					// Width
             	CW_USEDEFAULT,					// Height
  			 	0,								// Window Parent
  			 	0,								// Window Menu Bar
  			 	Instance,						// Handle to the Instance of the module to be
  			 									// associated with the window
  			 	0								// Parameter that could be used in the "WM_CREATE" Message
  			);
  		if(Window)
  		{
  			int xOffset = 0;
  			int yOffset = 0;

  			Running = true;
  			while (Running)
  			{
	  			MSG Message;
  				while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
  				{
  					if (Message.message == WM_QUIT)
  					{
  						Running = false;
  					}

  					TranslateMessage(&Message);
  					DispatchMessageA(&Message);
  				}
  				RenderWeirdGradient(GlobalBackBuffer, xOffset, yOffset);

  				HDC DeviceContext = GetDC(Window);
  				RECT ClientRect;
  				GetClientRect(Window, &ClientRect);
  				int WindowWidth = ClientRect.right - ClientRect.left;
				int WindowHeight = ClientRect.bottom - ClientRect.top;
				Win32DisplayBufferToWindow(DeviceContext, ClientRect, GlobalBackBuffer, 0, 0, WindowWidth, WindowHeight);
				ReleaseDC(Window, DeviceContext);

  				++xOffset;
  				yOffset += 2;
  			}

  		} 
  		else
  		{
  			// TODO(Alexis): Logging
  		}
  	} 
  	else 
  	{
  		// TODO(Alexis): Logging 
  	}

	return(0);
}