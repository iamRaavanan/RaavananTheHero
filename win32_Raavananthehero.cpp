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

// #include <windows.h>
#include <math.h>
#include "win32_Raavananthehero.h"
#include "Raavanan.cpp"
#include <xinput.h>
#include <dsound.h>
#include <stdio.h>


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

static debug_read_file_result DEBUGReadEntireFile(char *Filename)
{
	debug_read_file_result Result = {};
	HANDLE FileHandle = CreateFileA(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if(FileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER FileSize;
		if(GetFileSizeEx(FileHandle, &FileSize))
		{
			uint32 FileSize32 = SafeTruncateUInt64 (FileSize.QuadPart);
			Result.Content = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			if(Result.Content)
			{
				DWORD BytesRead;
				if(ReadFile(FileHandle, Result.Content, FileSize32, &BytesRead, 0) && (FileSize32 == BytesRead))
				{
					// Successfully read the file.
					Result.ContentSize = FileSize32;
				}
				else
				{
					DEBUGFreeFileMemory(Result.Content);
					Result.Content = 0;
				}
			}
			else
			{

			}
		}
		CloseHandle(FileHandle);
	}
	return Result;
}

static void DEBUGFreeFileMemory(void *Memory)
{
	if(Memory)
	{
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}

static bool DEBUGWriteEntireFile(char *Filename, uint32 Memorysize, void *Memory)
{
	bool Result = false;
	HANDLE FileHandle = CreateFileA(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if(FileHandle != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER FileSize;
		if(GetFileSizeEx(FileHandle, &FileSize))
		{
			DWORD BytesWrite;
			if(ReadFile(FileHandle, Memory, Memorysize, &BytesWrite, 0))
			{
				Result = (BytesWrite == Memorysize);
			}
			else
			{
				Result = false;
			}
		}
		CloseHandle(FileHandle);
	}
	return Result;
}

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

static LRESULT CALLBACK Win32Wndproc (HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
			Assert(!"Keyboard events from non-dispatch events are restricted!");
			uint32 VKCode = (uint32)wParam;
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

static void Win32ProcessKeyboardMessage (game_button_state *KeybaordState, bool IsDown)
{
	KeybaordState->EndedDown = IsDown;
	++KeybaordState->HalfTransitionCount;
}

static void Win32ProcessXInputDigitalButton (DWORD XInputButtonState,
											game_button_state *OldState, 
											DWORD ButtonBit, 
											game_button_state *NewState)
{
	NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
	NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
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
			Win32_Sound_Output SoundOutput = {};
			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.CurrentSampleIndex = 0;
			SoundOutput.BytesPerSample = sizeof(int16) * 2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
			SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond/15;
			Win32IntDirectSound(WindowHandle, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			Win32ClearBuffer(&SoundOutput);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			int16 *Samples = (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			
#if RAAVANAN_INTERNAL
			LPVOID BaseAddress = (LPVOID)Terabytes (2);
#else
			LPVOID BaseAddress = 0;
#endif
			game_memory GameMemory = {};
			GameMemory.PermanentStorageSize = Megabytes(64);
			GameMemory.TransientStorageSize = Gigabytes(1);
			uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;

			GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, (size_t)TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			GameMemory.TransientStorage = ((uint8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);

			if(Samples && GameMemory.PermanentStorage && GameMemory.PermanentStorage)
			{
				game_input Input[2] = {};
				game_input *NewInput = &Input[0];
				game_input *OldInput = &Input[1];

				LARGE_INTEGER LastCounter;
				QueryPerformanceCounter(&LastCounter);

				game_controller_input *KeyboardController = &NewInput->Controllers[0];
				game_controller_input ZeroController = {};
				*KeyboardController = ZeroController;

				while(bIsRunning)
				{
					while(PeekMessage(&Message, 0,0,0, PM_REMOVE))	
					{
						if(Message.message == WM_QUIT)
						{
							bIsRunning = false;
						}
						switch (Message.message)
						{
							case WM_SYSKEYDOWN:
							case WM_SYSKEYUP:
							case WM_KEYDOWN:
							case WM_KEYUP:
							{
								uint32 VKCode = (uint32)Message.wParam;
								bool KeyWasDown = ((Message.lParam & (1 << 30)) != 0);	// Check the key used to be down 
								bool KeyIsDown = ((Message.lParam & (1 << 31)) == 0);	// Check the key is currently down
								if(KeyIsDown != KeyWasDown) 
								{
									if(VKCode == VK_UP || VKCode == 'W')
									{
										Win32ProcessKeyboardMessage (&KeyboardController->Up, KeyIsDown);
									} else if(VKCode == VK_DOWN || VKCode == 'S')
									{
										Win32ProcessKeyboardMessage (&KeyboardController->Down, KeyIsDown);
									} else if(VKCode == VK_LEFT || VKCode == 'A')
									{
										Win32ProcessKeyboardMessage (&KeyboardController->Left, KeyIsDown);
									} else if(VKCode == VK_RIGHT || VKCode == 'D')
									{
										Win32ProcessKeyboardMessage (&KeyboardController->Right, KeyIsDown);
									} else if(VKCode == 'Q')
									{
										OutputDebugStringA("Q pressed\n");
									} else if(VKCode == 'E')
									{
										OutputDebugStringA("E pressed\n");
									}
									else if(VKCode == VK_ESCAPE)
									{
										bIsRunning = false;
									}
									else if(VKCode == VK_SPACE)
									{
										OutputDebugStringA("Space pressed\n");
									}
								}
								bool AltDown = ((Message.lParam & (1 << 29)) != 0);
								if(VKCode == VK_F4 && AltDown) {
									bIsRunning = false;
								}
							} break;
							default:
							{		
								TranslateMessage(&Message);
								DispatchMessage(&Message);
							} break;
						}
					}
					
					DWORD MaxControllerCount = XUSER_MAX_COUNT;
					if(MaxControllerCount > ArrayCount(NewInput->Controllers))
					{
						MaxControllerCount = ArrayCount(NewInput->Controllers);
					}

					for(DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ++ControllerIndex) 
					{
						game_controller_input *OldController = &OldInput->Controllers[ControllerIndex];
						game_controller_input *NewController = &NewInput->Controllers[ControllerIndex];
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
							
							NewController->IsAnalog = true;
							NewController->StartX = OldController->EndX;
							NewController->StartY = OldController->EndY;
							float X = (Pad->sThumbLX < 0) ? (float)Pad->sThumbLX / 32768.0f : (float)Pad->sThumbLX / 32767.0f;
							NewController->MinX = NewController->MaxX = NewController->EndX = X;
							float Y = (Pad->sThumbLY < 0) ? (float)Pad->sThumbLY / 32768.0f : (float)Pad->sThumbLY / 32767.0f;
							NewController->MinY = NewController->MaxY = NewController->EndY = Y;

							Win32ProcessXInputDigitalButton (Pad->wButtons, &OldController->Down, XINPUT_GAMEPAD_A, &NewController->Down);
							Win32ProcessXInputDigitalButton (Pad->wButtons, &OldController->Right, XINPUT_GAMEPAD_B, &NewController->Right);
							Win32ProcessXInputDigitalButton (Pad->wButtons, &OldController->Left, XINPUT_GAMEPAD_X, &NewController->Left);
							Win32ProcessXInputDigitalButton (Pad->wButtons, &OldController->Up, XINPUT_GAMEPAD_Y, &NewController->Up);
							Win32ProcessXInputDigitalButton (Pad->wButtons, &OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER, &NewController->LeftShoulder);
							Win32ProcessXInputDigitalButton (Pad->wButtons, &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER, &NewController->RightShoulder);

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
					
					GameUpdateAndRender(&GameMemory, NewInput, &GBuffer, &SoundBuffer);
					
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
					printf(Buffer, "%0.2f_ms, %0.2f_fps %0.2f_mc/f\n", MSPerFrame, FPS, MegaCyclePerFrame);
					OutputDebugStringA(Buffer);
					LastCounter = EndCounter;
					LastCycleCount = EndCycleCount;

					game_input *Temp = NewInput;
					NewInput = OldInput;
					OldInput = Temp;
				}
			}
			else
			{
				
			}			
		}
	}
	return 0;
}