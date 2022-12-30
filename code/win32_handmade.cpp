#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>
#include <math.h>

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

typedef float real32;
typedef double real64;

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
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

global_variable float Pi32 = 3.14159265359;

struct win32_window_dimension {
	int Width;
	int Height;
};

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE* pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
	return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void
Win32LoadXInput(void)
{
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
	if (!XInputLibrary)
	{
		XInputLibrary = LoadLibraryA("xinput1_3.dll");
	}
	if(XInputLibrary)
	{
		XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
	}
}

internal void
Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
	// Load the Library
	HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

	if(DSoundLibrary)
	{
		// Get a DirectSound object!
		direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");

		// "Create" a primary buffer
		LPDIRECTSOUND DirectSound;
		if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
		{
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
			WaveFormat.cbSize = 0;

			if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC BufferDescription = {};
				BufferDescription.dwSize = sizeof(BufferDescription);
				BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;

				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				HRESULT BufferCreation = DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0);	
				if(SUCCEEDED(BufferCreation))
				{
					if(SUCCEEDED(PrimaryBuffer->SetFormat(&WaveFormat)))
					{

					}
					else
					{
						// Diagnostic
					}
				}
				else
				{
					// Diagnostic
				}
			}
			else
			{
				// Diagnostic
			}

			// "Create" a secondary buffer
			DSBUFFERDESC BufferDescription = {};
			BufferDescription.dwSize = sizeof(BufferDescription);
			BufferDescription.dwFlags = 0;
			BufferDescription.dwBufferBytes = BufferSize;
			BufferDescription.lpwfxFormat = &WaveFormat;

			HRESULT BufferCreation = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0);
			if(SUCCEEDED(BufferCreation))
			{
				OutputDebugStringA("Buffers created successfully\n");
			}
			else
			{
				OutputDebugStringA("Buffers not created successfully\n");
			}
			// Start it playing!
		} 
		else
		{
			// Diagnostic
		}
	}
	else
	{
		// Diagnostic
	}
}

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
RenderWeirdGradient(win32_offscreen_buffer *Buffer, int BlueOffset, int GreenOffset, int RedOffset)
{
	uint8 *Row = (uint8 *)Buffer->BitmapMemory;

	for (int Y = 0; Y < Buffer->BitmapHeight; ++Y)
	{
		uint32 *Pixel = (uint32 *)Row;
		for (int X = 0; X < Buffer->BitmapWidth; ++X)
		{
			uint8 Blue = (X + BlueOffset) ;
			uint8 Green = (Y + GreenOffset);
			uint8 Red = (Y + X + RedOffset);

			*Pixel++ = ((Red << 16) | (Green << 8) | Blue);
		}

		Row += Buffer->Pitch;
	}
}

internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
	// TODO(Alexis): Bulletproof this.
	// Maybe don't free first, free after, the free first if that fails

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
							win32_offscreen_buffer *Buffer,
							int X, int Y)
{

	StretchDIBits(DeviceContext,
					/*
					X, Y, Width, Height
					X, Y, Width, Height
					*/
					0, 0, WindowWidth, WindowHeight,
					0, 0, Buffer->BitmapWidth, Buffer->BitmapHeight,
	  				Buffer->BitmapMemory,
	  				&Buffer->BitmapInfo,
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

		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			uint32 VKCode = WParam;
			bool WasDown = ((LParam & (1 << 30)) != 0);
			bool IsDown = ((LParam & (1 << 31)) == 0);

			if(VKCode == 'W')
			{
			}
			else if(VKCode == 'A')
			{
			}
			else if(VKCode == 'S')
			{
			}
			else if(VKCode == 'D')
			{
			}
			else if(VKCode == 'Q')
			{
			}
			else if(VKCode == 'E')
			{
			}
			else if(VKCode == VK_UP)
			{
			}
			else if(VKCode == VK_DOWN)
			{
			}
			else if(VKCode == VK_LEFT)
			{
			}
			else if(VKCode == VK_RIGHT)
			{
			}
			else if(VKCode == VK_ESCAPE)
			{
			}
			else if(VKCode == VK_SPACE)
			{
			}

			bool AltKeyWasDown = ((LParam & (1 << 29)) != 0);
			if((VKCode == VK_F4) && AltKeyWasDown)
			{
				Running = false;
			}
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
			Win32DisplayBufferToWindow(DeviceContext, WDimension.Width, WDimension.Height, &GlobalBackBuffer, X, Y);
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

struct win32_sound_output
{
	int SamplesPerSecond;
	int Tone;
	int16 ToneVolume;
	uint32 RunningSampleIndex;
	int WavePeriod;
	int HalfWavePeriod;
	int BytesPerSample;
	int32 SecondaryBufferSize;
};

void
Win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite)
{
	VOID *Region1;
	DWORD Region1Size;
	VOID *Region2;
	DWORD Region2Size;

	if(SUCCEEDED(GlobalSecondaryBuffer->Lock(ByteToLock, BytesToWrite,
								&Region1, &Region1Size,
								&Region2, &Region2Size,
								0)))
	{
		DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
		int16 *SampleOut = (int16 *)Region1;
		for (DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
		{
			real32 t = 2.0f * Pi32 * (real32)SoundOutput->RunningSampleIndex / (real32)SoundOutput->WavePeriod;
			real32 SineValue = sinf(t);
			int16 SampleValue = (int16)(SineValue * SoundOutput->ToneVolume);
			*SampleOut++ = SampleValue;
			*SampleOut++ = SampleValue;
			
			++SoundOutput->RunningSampleIndex;
		}

		SampleOut = (int16 *)Region2;
		DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
		for (DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
		{
			real32 t = 2.0f * Pi32 * (real32)SoundOutput->RunningSampleIndex / (real32)SoundOutput->WavePeriod;
			real32 SineValue = sinf(t);
			int16 SampleValue = (int16)(SineValue * SoundOutput->ToneVolume);
			*SampleOut++ = SampleValue;
			*SampleOut++ = SampleValue;
			
			++SoundOutput->RunningSampleIndex;
		}

		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}


int CALLBACK WinMain(
	HINSTANCE Instance,
	HINSTANCE PrevInstance,
	LPSTR CmdLine,
	int nCmdShow
)
{
	Win32LoadXInput();

	WNDCLASSA WindowClass = {};

	Win32ResizeDIBSection(&GlobalBackBuffer, 1280, 720);

	WindowClass.style = CS_OWNDC;
  	WindowClass.lpfnWndProc = Win32MainWindowCallback;
  	WindowClass.hInstance = Instance;
//  	WindowClass.hIcon;
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
			HDC DeviceContext = GetDC(Window);
			int BlueOffset = 0;
  			int GreenOffset = 0;
			int RedOffset = 0;

			win32_sound_output SoundOutput = {};

			SoundOutput.SamplesPerSecond = 44100;
			SoundOutput.Tone = 256;
			SoundOutput.ToneVolume = 1000;
			SoundOutput.RunningSampleIndex = 0;
			SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond/SoundOutput.Tone;
			SoundOutput.HalfWavePeriod = SoundOutput.WavePeriod/2;
			SoundOutput.BytesPerSample = sizeof(int16)*2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;

			bool SoundIsPlaying = false;

			Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			Win32FillSoundBuffer(&SoundOutput, 0, SoundOutput.SecondaryBufferSize);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

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

				// Gamepad controls
				for (DWORD ControllerIndex;
					ControllerIndex < XUSER_MAX_COUNT;
					++ControllerIndex)
				{
					XINPUT_STATE ControllerState;
					ZeroMemory(&ControllerState, sizeof(XINPUT_STATE));

					DWORD Result = XInputGetState(ControllerIndex, &ControllerState);
					if(Result == ERROR_SUCCESS)
					{
						// Connected
						OutputDebugStringA("CONNECTED");
						XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

						bool Up 			= (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool Down 			= (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool Left 			= (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool Right 			= (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool Start 			= (Pad->wButtons & XINPUT_GAMEPAD_START);
						bool Back 			= (Pad->wButtons & XINPUT_GAMEPAD_BACK);
						bool LeftShoulder 	= (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool RightShoulder 	= (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						bool AButton 		= (Pad->wButtons & XINPUT_GAMEPAD_A);
						bool BButton 		= (Pad->wButtons & XINPUT_GAMEPAD_B);
						bool XButton 		= (Pad->wButtons & XINPUT_GAMEPAD_X);
						bool YButton 		= (Pad->wButtons & XINPUT_GAMEPAD_Y);

						int16 StickX = Pad->sThumbLX;
						int16 StickY = Pad->sThumbLY;

					}
					else
					{
						// Not connected
					}
				}

  				RenderWeirdGradient(&GlobalBackBuffer, BlueOffset, GreenOffset, RedOffset);

				// DirectSound Output
				DWORD PlayCursor;
				DWORD WriteCursor;
				if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
				{
					DWORD ByteToLock = (SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
					DWORD BytesToWrite;
					if(ByteToLock == PlayCursor)
					{
						if(!SoundIsPlaying)
						{
							BytesToWrite = SoundOutput.SecondaryBufferSize;
						}
						else
						{
							BytesToWrite = 0;
						}
					}
					else if(ByteToLock > PlayCursor)
					{
						BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
						BytesToWrite += PlayCursor;
					}
					else
					{
						BytesToWrite = PlayCursor - ByteToLock;
					}

				}
				
				if(!SoundIsPlaying)
				{
					SoundIsPlaying = true;
				}

  				win32_window_dimension WDimension = Win32GetWindowDimension(Window);
				Win32DisplayBufferToWindow(DeviceContext, WDimension.Width, WDimension.Height, &GlobalBackBuffer, 0, 0);
				ReleaseDC(Window, DeviceContext);

  				++BlueOffset;
  				++GreenOffset;
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