#include <stdint.h>

#define PI 3.14159265359

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

#include <windows.h>
#include <xinput.h>
#include <dsound.h>
#include <math.h>
#include <stdio.h>

struct win32_offscreen_buffer
{
	BITMAPINFO Info;
	void* Memory;
	int Width;
	int Height;
	int Pitch;
	int BytesPerPixel;
};
#include "Raavanan.cpp"

#pragma region X_INPUT
// XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(X_InputGetState);
X_INPUT_GET_STATE(XInputGetStateStub) 
{
	return (ERROR_DEVICE_NOT_CONNECTED);
}
static X_InputGetState *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_;

// XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(X_InputSetState);
X_INPUT_SET_STATE(XInputSetStateStub)
{
	return (ERROR_DEVICE_NOT_CONNECTED);
}
static X_InputSetState *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_;

static void Win32LoadXInput(void)
{
	HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
	if (!XInputLibrary)
	{
		XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
	}
	if (!XInputLibrary)
	{
		XInputLibrary = LoadLibraryA("xinput1_3.dll");
	}
	if (XInputLibrary)
	{
		XInputGetState_ = (X_InputGetState *)GetProcAddress(XInputLibrary, "XInputGetState");
		XInputSetState_ = (X_InputSetState *)GetProcAddress(XInputLibrary, "XInputSetState");
	}
}
#pragma endregion X_INPUT

#pragma region DIRECT_SOUND
static LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;
#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID lpGUID,LPDIRECTSOUND *ppDS,LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

static void Win32IntDirectSound (HWND Window, int32 SamplesPerSecond, int32 BufferSize)
{
	HMODULE DSound = LoadLibraryA("dsound.dll");
	if(DSound)
	{
		direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSound, "DirectSoundCreate");
		LPDIRECTSOUND DirectSound;
		if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
		{
			WAVEFORMATEX WaveFormat = {};
			WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
			WaveFormat.nChannels = 2;
			WaveFormat.nSamplesPerSec = SamplesPerSecond;
			WaveFormat.wBitsPerSample = 16;
			WaveFormat.nBlockAlign = (WaveFormat.nChannels * WaveFormat.wBitsPerSample)/8;
			WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec * WaveFormat.nBlockAlign;
			WaveFormat.cbSize = 0;
			if(SUCCEEDED (DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY)))
			{
				DSBUFFERDESC BufferDesc = {};
				BufferDesc.dwSize = sizeof(BufferDesc);
				BufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
				LPDIRECTSOUNDBUFFER PrimaryBuffer;
				if(SUCCEEDED (DirectSound->CreateSoundBuffer(&BufferDesc, &PrimaryBuffer, 0)))
				{				
					HRESULT Err = PrimaryBuffer->SetFormat (&WaveFormat);
					if(SUCCEEDED (PrimaryBuffer->SetFormat (&WaveFormat)))
					{
						OutputDebugStringA("Primary Buffer format was set \n");
					}
					else
					{
						// Log
					}
				}
				else
				{
					// Log
				}
			}
			else
			{
				// Log failure
			}
			DSBUFFERDESC BufferDesc = {};
			BufferDesc.dwSize = sizeof(BufferDesc);
			BufferDesc.dwFlags = 0;
			BufferDesc.dwBufferBytes = BufferSize;
			BufferDesc.lpwfxFormat = &WaveFormat;			
			if(SUCCEEDED (DirectSound->CreateSoundBuffer(&BufferDesc, &GlobalSecondaryBuffer, 0)))
			{
				OutputDebugStringA("Secondary Buffer format was set \n");
			}
		}
	}
}
#pragma endregion DIRECT_SOUND

win32_offscreen_buffer  backBuffer;
bool bIsRunning;

struct win32_window_dimension
{
	int Width;
	int Height;
};

win32_window_dimension GetWindowDimension (HWND hwnd)
{
	win32_window_dimension Result;
	RECT clientRect;
	GetClientRect(hwnd, &clientRect);
	Result.Width = clientRect.right - clientRect.left;
	Result.Height = clientRect.bottom - clientRect.top;
	return (Result);
}

void Win32ResizeDBISection (win32_offscreen_buffer *Buffer, int width, int height)
{
	if(Buffer->Memory)
	{
		VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
	}
	Buffer->Width = width;
	Buffer->Height = height;
	Buffer->BytesPerPixel = 4;
	Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
	Buffer->Info.bmiHeader.biWidth = Buffer->Width;
	// if biHeight is +ve bitmap is Bottom-up DIB. origin is lower-left,
	//				  -ve then, bitmap is Top-down DIB, origin is upper-left
	Buffer->Info.bmiHeader.biHeight = -Buffer->Height;	
	Buffer->Info.bmiHeader.biPlanes = 1;
	Buffer->Info.bmiHeader.biBitCount = 32;
	Buffer->Info.bmiHeader.biCompression = BI_RGB;
		
	int BitMemorysize = (Buffer->Height * Buffer->Width) * Buffer->BytesPerPixel;
	Buffer->Memory = VirtualAlloc(0, BitMemorysize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

	Buffer->Pitch = width * Buffer->BytesPerPixel;
}

void Win32UpdateBufferInWindow (win32_offscreen_buffer *Buffer, HDC DeviceContext, int windowWidth, int windowHeight)
{
	// int windowWidth = WindowRect->right - WindowRect->left;
	// int windowHeight = WindowRect->bottom - WindowRect->top;
	//StretchDIBits(DeviceContext, x, y, width, height, x, y, width, height, BitmapMemory, &BitmapInfo, DIB_RGB_COLORS,SRCCOPY);
	StretchDIBits(DeviceContext, 
					0, 0, windowWidth, windowHeight, 
					0, 0, Buffer->Width, Buffer->Height, 
					Buffer->Memory, 
					&Buffer->Info, 
					DIB_RGB_COLORS, SRCCOPY);
}

LRESULT Win32Wndproc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT Result = 0;
	switch (uMsg)
	{
		case WM_SIZE:
		{
			// win32_window_dimension dimension = GetWindowDimension(hwnd);
			// Win32ResizeDBISection(&backBuffer, dimension.Width, dimension.Height);
		} break;
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			OutputDebugStringA("WM_DESTROY\n");
		} break;
		case WM_CLOSE:
		{
			PostQuitMessage(0);
			OutputDebugStringA("WM_CLOSE\n");
		} break;
		case WM_ACTIVATEAPP:
		{
			OutputDebugStringA("WM_ACTIVATEAPP\n");
		} break;
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_KEYDOWN:
		case WM_KEYUP:
		{
			int32 VKCode = wParam;
			bool KeyWasDown = ((lParam & (1 << 30)) != 0);	// Check the key used to be down 
			bool KeyIsDown = ((lParam & (1 << 31)) == 0);	// Check the key is currently down
			if(KeyIsDown != KeyWasDown) 
			{
				if(VKCode == VK_UP || VKCode == 'W')
				{
					OutputDebugStringA("W or Up pressed\n");
				} else if(VKCode == VK_DOWN || VKCode == 'S')
				{
					OutputDebugStringA("S or Down pressed\n");
				} else if(VKCode == VK_LEFT || VKCode == 'A')
				{
					OutputDebugStringA("A or LEft pressed\n");
				} else if(VKCode == VK_RIGHT || VKCode == 'D')
				{
					OutputDebugStringA("D or Right pressed\n");
				} else if(VKCode == 'Q')
				{
					OutputDebugStringA("Q pressed\n");
				} else if(VKCode == 'E')
				{
					OutputDebugStringA("E pressed\n");
				}
				else if(VKCode == VK_ESCAPE)
				{
					OutputDebugStringA("Escape ");
					if(KeyIsDown)  {
						OutputDebugStringA("isDown");
					}
					if (KeyWasDown) {
						OutputDebugStringA("was Down");
					}
					OutputDebugStringA("\n");
				}
				else if(VKCode == VK_SPACE)
				{
					OutputDebugStringA("Space pressed\n");
				}
			}
			bool AltDown = ((lParam & (1 << 29)) != 0);
			if(VKCode == VK_F4 && AltDown) {
				bIsRunning = false;
			}
		}
		break;
		case WM_PAINT:
		{
			PAINTSTRUCT paint;
			HDC DeviceContext = BeginPaint(hwnd, &paint);
			int x = paint.rcPaint.left;
			int y = paint.rcPaint.top;
			int width = paint.rcPaint.right - paint.rcPaint.left;
			int height = paint.rcPaint.bottom - paint.rcPaint.top;
			
			win32_window_dimension dimension = GetWindowDimension(hwnd);
			Win32UpdateBufferInWindow (&backBuffer, DeviceContext, dimension.Width, dimension.Height);
			EndPaint(hwnd, &paint);
		} break;
		default:
			Result = DefWindowProcA(hwnd, uMsg, wParam, lParam);
			break;
	}
	return Result;
}

struct Win32_Sound_Output
{
	int SamplesPerSecond;
	int ToneHz;	// Actual Middle C Hz is 261.6
	int16 ToneVolume;
	uint32 CurrentSampleIndex;
	int wavePeriod;
	int BytesPerSample;
	int SecondaryBufferSize;
	float tSine;
	int LatencySampleCount;
};

static void Win32ClearBuffer (Win32_Sound_Output *SoundOutput)
{
	void *Region1;
	DWORD Region1Size;
	void *Region2;
	DWORD Region2Size;
	if(SUCCEEDED (GlobalSecondaryBuffer->Lock (0, SoundOutput->SecondaryBufferSize, &Region1, &Region1Size, &Region2, &Region2Size, 0)))
	{
		uint8 *DestSample = (uint8 *)Region1;
		for(DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex)
		{
			*DestSample++ = 0;
		}
		DestSample = (uint8 *)Region2;
		for(DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex)
		{
			*DestSample++ = 0;
		}
		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

static void Win32SoundBuffer (Win32_Sound_Output *SoundOutput, DWORD BytesToLock, DWORD BytesToWrite, game_sound_buffer * SrcBuffer)
{
	void *Region1;
	DWORD Region1Size;
	void *Region2;
	DWORD Region2Size;
	if(SUCCEEDED (GlobalSecondaryBuffer->Lock (BytesToLock, BytesToWrite, &Region1, &Region1Size, &Region2, &Region2Size, 0)))
	{
		int16 *DestSample = (int16 *)Region1;
		int16 *SrcSample = SrcBuffer->Samples;
		DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
		for(DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex)
		{
			*DestSample++ = *SrcSample++;
			*DestSample++ = *SrcSample++;
			++SoundOutput->CurrentSampleIndex;
		}
		DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
		DestSample = (int16 *)Region2;
		for(DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex)
		{
			*DestSample++ = *SrcSample++;
			*DestSample++ = *SrcSample++;
			++SoundOutput->CurrentSampleIndex;
		}
		GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
	LARGE_INTEGER PerformanceCounterFreqResult;
	QueryPerformanceFrequency(&PerformanceCounterFreqResult);
	int64 PerfCountFrequency = PerformanceCounterFreqResult.QuadPart;
	uint64 LastCycleCount = __rdtsc();
	Win32LoadXInput();
	WNDCLASSA WindowClass = {};
	//win32_window_dimension dimension = GetWindowDimension(hwnd);
	Win32ResizeDBISection(&backBuffer, 1280, 720);
	WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	WindowClass.lpfnWndProc = Win32Wndproc;
	WindowClass.hInstance = hInstance;
	WindowClass.lpszClassName = "RaavananTheHeroWindowClass";

	if(RegisterClass(&WindowClass))
	{
		HWND WindowHandle = CreateWindowEx(0, WindowClass.lpszClassName, "RaavananTheHero", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 
							CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);
		if (WindowHandle) {
			HDC DeviceContext = GetDC(WindowHandle);
			MSG Message;

			bIsRunning = true;
			int xOffset = 0;
			int yOffset = 0;

			Win32_Sound_Output SoundOutput = {};
			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.ToneHz = 256;	// Actual Middle C Hz is 261.6
			SoundOutput.ToneVolume = 4000;
			SoundOutput.CurrentSampleIndex = 0;
			SoundOutput.wavePeriod = SoundOutput.SamplesPerSecond/SoundOutput.ToneHz;
			SoundOutput.BytesPerSample = sizeof(int16) * 2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
			SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond/15;
			Win32IntDirectSound(WindowHandle, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			Win32ClearBuffer(&SoundOutput);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			int16 *Samples = (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			
			LARGE_INTEGER LastCounter;
			QueryPerformanceCounter(&LastCounter);
			while(bIsRunning)
			{				
				while(PeekMessage(&Message, 0,0,0, PM_REMOVE))	
				{
					if(Message.message == WM_QUIT)
					{
						bIsRunning = false;
					}
					TranslateMessage(&Message);
					DispatchMessage(&Message);
				}

				for(DWORD ControllerIndex = 0; ControllerIndex < XUSER_MAX_COUNT; ++ControllerIndex) {
					XINPUT_STATE ControllerState;
					if(XInputGetState_(ControllerIndex, &ControllerState) == ERROR_SUCCESS) {
						// Controller is plugged-in
						XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
						bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
						bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
						bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
						bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
						bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
						bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
						bool ABtn = (Pad->wButtons & XINPUT_GAMEPAD_A);
						bool BBtn = (Pad->wButtons & XINPUT_GAMEPAD_B);
						bool XBtn = (Pad->wButtons & XINPUT_GAMEPAD_X);
						bool YBtn = (Pad->wButtons & XINPUT_GAMEPAD_Y);
						bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
						bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
						
						int16 StickX = Pad->sThumbLX;
						int16 StickY = Pad->sThumbLY;
						xOffset += StickX / 2048;
						yOffset += StickY / 2048;

						SoundOutput.ToneHz = 512 + (int)(256.0f *((float)StickX/30000.0f));
						SoundOutput.wavePeriod = SoundOutput.SamplesPerSecond/SoundOutput.ToneHz;
					}
					else {
						// Controller is not available
					}
				}
				// XINPUT_VIBRATION Vibration;
				// Vibration.wLeftMotorSpeed = 60000;
				// Vibration.wRightMotorSpeed = 60000;
				// XInputSetState_(0, &Vibration);

				// RenderGradiant(&backBuffer, xOffset, yOffset);
				DWORD ByteTolock = 0;
				DWORD BytesToWrite = 0;
				DWORD TargetCursor = 0;
				DWORD PlayCursor = 0;
				DWORD WriteCursor = 0;
				bool bIsSoundValid = false;
				if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor)))
				{
					ByteTolock = (SoundOutput.CurrentSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
					BytesToWrite = 0;
					TargetCursor = ((PlayCursor + (SoundOutput.LatencySampleCount * SoundOutput.BytesPerSample))% SoundOutput.SecondaryBufferSize);
					if(ByteTolock > TargetCursor)
					{
						BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteTolock);
						BytesToWrite += TargetCursor;
					}
					else
					{
						BytesToWrite = TargetCursor - ByteTolock;
					}
					bIsSoundValid = true;					
				}
				
				game_sound_buffer SoundBuffer = {};
				SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
				SoundBuffer.Samples = Samples;
				SoundBuffer.SampleCount = BytesToWrite/SoundOutput.BytesPerSample;

				game_offscreen_buffer GBuffer = {};
				GBuffer.Memory = backBuffer.Memory;
				GBuffer.Width = backBuffer.Width;
				GBuffer.Height = backBuffer.Height;
				GBuffer.Pitch = backBuffer.Pitch;			
				
				GameUpdateAndRender(&GBuffer, xOffset, yOffset, &SoundBuffer, SoundOutput.ToneHz);				
				
				if(bIsSoundValid)
				{
					Win32SoundBuffer(&SoundOutput, ByteTolock, BytesToWrite, &SoundBuffer);
				}
				win32_window_dimension dimension = GetWindowDimension(WindowHandle);
				Win32UpdateBufferInWindow (&backBuffer, DeviceContext, dimension.Width, dimension.Height);
				ReleaseDC(WindowHandle, DeviceContext);

				uint64 EndCycleCount = __rdtsc();
				uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
				float MegaCyclePerFrame = (float)((float)CyclesElapsed/(1000.0f * 1000.0f));

				LARGE_INTEGER EndCounter;
				QueryPerformanceCounter(&EndCounter);
				int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
				float MSPerFrame = (float)((1000.0f * (float)CounterElapsed)/(float)PerfCountFrequency);
				float FPS = (float)PerfCountFrequency/(float)CounterElapsed;
				char Buffer[256];
				sprintf(Buffer, "%0.2f_ms, %0.2f_fps %0.2f_mc/f\n", MSPerFrame, FPS, MegaCyclePerFrame);
				OutputDebugStringA(Buffer);
				LastCounter = EndCounter;
				LastCycleCount = EndCycleCount;
			}
		}
	}
	return 0;
}