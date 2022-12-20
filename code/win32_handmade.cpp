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

struct win32_window_dimension {
	int Width;
	int Height;
};

win32_window_dimension
Win32GetWindowDimension(HWND Window)
{
	win32_window_dimension Result;

	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Result.Width = ClientRect.right - ClientRect.left;
	Result.Height = ClientRect.bottom - ClientRect.top;

	return(Result);
}

internal void
RenderWeirdGradient(win32_offscreen_buffer Buffer, int BlueOffset, int GreenOffset, int RedOffset)
{
	uint8 *Row = (uint8 *)Buffer.BitmapMemory;

	for (int Y = 0; Y < Buffer.BitmapHeight; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int X = 0; X < Buffer.BitmapWidth; ++X)
		{
			uint8 Blue = (X + BlueOffset);
			uint8 Green = (Y + GreenOffset);
			uint8 Red = (Y + X + RedOffset);

			*Pixel++ = ((Red << 16) | (Green << 8) | Blue);
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
Win32DisplayBufferToWindow(HDC DeviceContext, 
							int WindowWidth, int WindowHeight, 
							win32_offscreen_buffer Buffer,
							int X, int Y)
{

	StretchDIBits(DeviceContext,
					/*
					X, Y, Width, Height
					X, Y, Width, Height
					*/
					0, 0, WindowWidth, WindowHeight,
					0, 0, Buffer.BitmapWidth, Buffer.BitmapHeight,
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

			win32_window_dimension WDimension = Win32GetWindowDimension(Window);
			Win32DisplayBufferToWindow(DeviceContext, WDimension.Width, WDimension.Height, GlobalBackBuffer, X, Y);
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
	WNDCLASSA WindowClass = {};

	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

	WindowClass.style = CS_OWNDC;
  	WindowClass.lpfnWndProc = Win32MainWindowCallback;
  	WindowClass.hInstance = Instance;
//  	WindowClass.hIcon;
//		It will have an error at this place on VSCODE
//		[a value of type "const char *" cannot be 
//		assigned to an entity of type "LPCWSTR"]
  	WindowClass.lpszClassName = "HandmadeHeroWindowClass";

  	if(RegisterClassA(&WindowClass))
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
  			int BlueOffset = 0;
  			int GreenOffset = 0;
			int RedOffset = 0;

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
  				RenderWeirdGradient(GlobalBackBuffer, BlueOffset, GreenOffset, RedOffset);

  				HDC DeviceContext = GetDC(Window);
  				win32_window_dimension WDimension = Win32GetWindowDimension(Window);
				Win32DisplayBufferToWindow(DeviceContext, WDimension.Width, WDimension.Height, GlobalBackBuffer, 0, 0);
				ReleaseDC(Window, DeviceContext);

  				++BlueOffset;
  				GreenOffset += 2;
				++RedOffset;
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