
#include "Raavanan.h"
#include "win32_Raavananthehero.h"
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

struct Win32_game_code
{
	bool bIsValid;
	HMODULE GameCodeDLL;
	game_update_and_Render *UpdateAndRender;
	get_game_sound_samples *GetSoundSamples;
};

static Win32_game_code Win32LoadGameCode ()
{
	Win32_game_code Result = {};
	CopyFileA("Raavanan.dll", "Raavanan_temp.dll", FALSE);
	Result.GameCodeDLL = LoadLibraryA("Raavanan_temp.dll");
	if(Result.GameCodeDLL)
	{
		Result.UpdateAndRender = (game_update_and_Render *)GetProcAddress(Result.GameCodeDLL, "GameUpdateAndRender");
		Result.GetSoundSamples = (get_game_sound_samples *)GetProcAddress(Result.GameCodeDLL, "GetGameSoundSamples");
		Result.bIsValid = (Result.UpdateAndRender && Result.GetSoundSamples);
	}
	
	if(!Result.bIsValid)
	{
		Result.UpdateAndRender = GameUpdateAndRenderStub;
		Result.GetSoundSamples = GetGameSoundSamplesStub;
	}
	return Result;
}

static void Win32UnLoadGameCode (Win32_game_code *GameCode)
{
	if(GameCode->GameCodeDLL)
	{
		FreeLibrary(GameCode->GameCodeDLL);
	}
	GameCode->bIsValid = false;
	GameCode->GetSoundSamples = GetGameSoundSamplesStub;
	GameCode->UpdateAndRender = GameUpdateAndRenderStub;
}

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

#if RAAVANAN_INTERNAL
DEBUG_FREE_FILE_MEMORY (DEBUGFreeFileMemory)
{
	if(Memory)
	{
		VirtualFree(Memory, 0, MEM_RELEASE);
	}
}

DEBUG_READ_ENTIRE_FILE(DEBUGReadEntireFile)
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

DEBUG_WRITE_ENTIRE_FILE (DEBUGWriteEntireFile)
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

#endif

static void Win32InitDirectSound (HWND Window, int32 SamplesPerSecond, int32 BufferSize)
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
bool bIsPaused;

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

static void Win32FillSoundBuffer (Win32_Sound_Output *SoundOutput, DWORD BytesToLock, DWORD BytesToWrite, game_sound_buffer * SrcBuffer)
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

static void Win32ProcessKeyboardMessage (game_button_state *KeyboardState, bool IsDown)
{
	Assert(KeyboardState->EndedDown != IsDown);
	KeyboardState->EndedDown = IsDown;
	++KeyboardState->HalfTransitionCount;
}

static void Win32ProcessMessage (game_controller_input *KeyboardController)
{
	MSG Message;
	while(PeekMessage(&Message, 0,0,0, PM_REMOVE))	
	{
		switch (Message.message)
		{
			case WM_QUIT:
			{
				bIsRunning = false;
			} break;
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
					if(VKCode == 'W')
					{
						Win32ProcessKeyboardMessage (&KeyboardController->MoveUp, KeyIsDown);
					} 
					else if(VKCode == 'S')
					{
						Win32ProcessKeyboardMessage (&KeyboardController->MoveDown, KeyIsDown);
					} 
					else if(VKCode == 'A')
					{
						Win32ProcessKeyboardMessage (&KeyboardController->MoveLeft, KeyIsDown);
					} 
					else if(VKCode == 'D')
					{
						Win32ProcessKeyboardMessage (&KeyboardController->MoveRight, KeyIsDown);
					}
					else if (VKCode == VK_UP) 
					{
						Win32ProcessKeyboardMessage (&KeyboardController->ActionUp, KeyIsDown);
					}
					else if (VKCode == VK_DOWN) 
					{
						Win32ProcessKeyboardMessage (&KeyboardController->ActionDown, KeyIsDown);
					}
					else if (VKCode == VK_LEFT)
					{
						Win32ProcessKeyboardMessage (&KeyboardController->ActionLeft, KeyIsDown);
					}
					else if (VKCode == VK_RIGHT) 
					{
						Win32ProcessKeyboardMessage (&KeyboardController->ActionRight, KeyIsDown);
					}
					else if(VKCode == 'Q')
					{
						OutputDebugStringA("Q pressed\n");
					} 
					else if(VKCode == 'E')
					{
						OutputDebugStringA("E pressed\n");
					}
					else if(VKCode == VK_ESCAPE)
					{
						Win32ProcessKeyboardMessage (&KeyboardController->Start, KeyIsDown);
					}
					else if(VKCode == VK_SPACE)
					{
						Win32ProcessKeyboardMessage (&KeyboardController->Back, KeyIsDown);
					}
#if RAAVANAN_INTERNAL
					else if (VKCode == 'P')
					{
						if(KeyIsDown)
						{
							bIsPaused = !bIsPaused;
						}
					}
#endif
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
}
static void Win32ProcessXInputDigitalButton (DWORD XInputButtonState,
											game_button_state *OldState, 
											DWORD ButtonBit, 
											game_button_state *NewState)
{
	NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
	NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

static float Win32ProcessXInputStickValue (SHORT Value, SHORT DeadZoneThreashold)
{
	float Result = 0;
	if(Value < -DeadZoneThreashold)
	{
		Result = (float)((Value + DeadZoneThreashold) / (32768.0f - DeadZoneThreashold));
	}
	else if(Value > DeadZoneThreashold)
	{
		Result = (float)((Value - DeadZoneThreashold) / (32767.0f + DeadZoneThreashold));
	}
	return Result;
}

static int64 PerfCountFrequency;
inline LARGE_INTEGER Win32GetWallClock()
{
	LARGE_INTEGER Result;
	QueryPerformanceCounter(&Result);
	return Result;
}

inline float Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
	float Result = ((float)(End.QuadPart - Start.QuadPart) / (float)PerfCountFrequency);
	return Result;
}

static void Win32DebugDrawVerticalLine(win32_offscreen_buffer *Buffer, int X, int Top, int Bottom, uint32 Color)
{
	if(Top < 0)
	{
		Top = 0;
	}
	if(Bottom > Buffer->Height)
	{
		Bottom = Buffer->Height;
	}
	if((X >= 0) && (X < Buffer->Width))
	{
		uint8 *Pixel = ((uint8 *) Buffer->Memory + X * Buffer->BytesPerPixel + Top * Buffer->Pitch);
		for (int i = Top; i < Bottom; ++i)
		{
			*(uint32 *)Pixel = Color;
			Pixel += Buffer->Pitch;
		}
	}
}

inline void Win32DrawSoundBufferMarker (win32_offscreen_buffer *Buffer, Win32_Sound_Output *SoundOutput, float C, int PadX, int Top, int Bottom, DWORD Value, uint32 Color)
{
	int X = PadX + (int)(C * (float)Value);
	Win32DebugDrawVerticalLine(Buffer, X, Top, Bottom, Color);
}

#if RAAVANAN_INTERNAL
static void Win32DebugSyncDisplay(win32_offscreen_buffer *Buffer, int MarkerCount, Win32_debug_time_marker *Markers, int CurrentMarkerIndex, Win32_Sound_Output *SoundOutput, float TargetSecondsPerFrame)
{
	int PadX = 16;
	int PadY = 16;
	int LineHeight = 64;	
	
	float C = (float)(Buffer->Width - 2 * PadX) / ((float)SoundOutput->SecondaryBufferSize);
	for (int MarkerIndex = 0; MarkerIndex < MarkerCount; ++MarkerIndex)
	{
		DWORD PlayColor = 0xFFFFFFFF;
		DWORD WriteColor = 0xFFFF0000;
		DWORD ExpectedFlipColor = 0xFF00FFFF;
		DWORD PlayWindowColor = 0xFF0000FF;
		int Top = PadY;
		int Bottom = PadY + LineHeight;
		Win32_debug_time_marker *ThisMarker = &Markers[MarkerIndex];
		if(MarkerIndex == CurrentMarkerIndex)
		{
			Top += LineHeight + PadY;
			Bottom += LineHeight + PadY;
			int FirstTop = Top;
			Win32DrawSoundBufferMarker (Buffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputPlayCursor, PlayColor);
			Win32DrawSoundBufferMarker (Buffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputWriteCursor, WriteColor);
			
			Top += LineHeight + PadY;
			Bottom += LineHeight + PadY;
			Win32DrawSoundBufferMarker (Buffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputLocation, PlayColor);
			Win32DrawSoundBufferMarker (Buffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputLocation + ThisMarker->OutputByteCount, WriteColor);

			Top += LineHeight + PadY;
			Bottom += LineHeight + PadY;
			Win32DrawSoundBufferMarker (Buffer, SoundOutput, C, PadX, FirstTop, Bottom, ThisMarker->ExpectedFlipPlayCursor, ExpectedFlipColor);
		}
		Win32DrawSoundBufferMarker (Buffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor, PlayColor);
		Win32DrawSoundBufferMarker (Buffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor + 480 * SoundOutput->BytesPerSample, PlayWindowColor);
		Win32DrawSoundBufferMarker (Buffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipWriteCursor, WriteColor);
	}
}
#endif

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{	
	LARGE_INTEGER PerformanceCounterFreqResult;
	QueryPerformanceFrequency(&PerformanceCounterFreqResult);
	PerfCountFrequency = PerformanceCounterFreqResult.QuadPart;	

	UINT DesiredSchedulerMS = 1;
	bool SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

	Win32LoadXInput();
	WNDCLASSA WindowClass = {};
	//win32_window_dimension dimension = GetWindowDimension(hwnd);
	Win32ResizeDBISection(&backBuffer, 1280, 720);
	WindowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	WindowClass.lpfnWndProc = Win32Wndproc;
	WindowClass.hInstance = hInstance;
	WindowClass.lpszClassName = "RaavananTheHeroWindowClass";

	#define MonitorRefreshHz 60
	#define GameUpdateHz (MonitorRefreshHz / 2)
	float TargetSecondsPerFrame = 1.0f / (float)GameUpdateHz;

	if(RegisterClass(&WindowClass))
	{
		HWND WindowHandle = CreateWindowExA(0, WindowClass.lpszClassName, "RaavananTheHero", WS_OVERLAPPEDWINDOW | WS_VISIBLE, 
							CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, hInstance, 0);
		if (WindowHandle) 
		{
			HDC DeviceContext = GetDC(WindowHandle);	
					
			Win32_Sound_Output SoundOutput = {};
			SoundOutput.SamplesPerSecond = 48000;
			SoundOutput.CurrentSampleIndex = 0;
			SoundOutput.BytesPerSample = sizeof(int16) * 2;
			SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample;
			SoundOutput.LatencySampleCount = 3 * (SoundOutput.SamplesPerSecond/GameUpdateHz);
			SoundOutput.SafetyBytes = (SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample / GameUpdateHz) / 2;
			Win32InitDirectSound(WindowHandle, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
			Win32ClearBuffer(&SoundOutput);
			GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);

			bIsRunning = true;	
#if 0
			while (bIsRunning)
			{
				DWORD PlayCursor;
				DWORD WriteCursor;
				GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor);
				char TextBuffer[256];
				sprintf_s(TextBuffer, "PC:%u WC: %u \n", PlayCursor, WriteCursor);
				OutputDebugStringA(TextBuffer);
			}
#endif
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
#if RAAVANAN_INTERNAL		
			GameMemory.DEBUGReadEntireFile = DEBUGReadEntireFile;
			GameMemory.DEBUGFreeFileMemory = DEBUGFreeFileMemory;
			GameMemory.DEBUGWriteEntireFile = DEBUGWriteEntireFile;			
#endif
			DWORD AudioLatencyBytes = 0;
			float AudioLatencySeconds = 0;
			bool bIsSoundValid = false;
			
			if(Samples && GameMemory.PermanentStorage && GameMemory.PermanentStorage)
			{
				game_input Input[2] = {};
				game_input *NewInput = &Input[0];
				game_input *OldInput = &Input[1];

				LARGE_INTEGER LastCounter = Win32GetWallClock();
				LARGE_INTEGER FlipWallClock = Win32GetWallClock();
				int DebugTimeMarkerIndex = 0;
				Win32_debug_time_marker DebugTimeMarkers[GameUpdateHz/2] = {0};
				
				Win32_game_code Game = Win32LoadGameCode();
				uint32 LoadCounter = 120;
				uint64 LastCycleCount = __rdtsc();
				while(bIsRunning)
				{
					if(LoadCounter++ > 120)
					{
						Win32UnLoadGameCode(&Game);
						Game = Win32LoadGameCode();
						LoadCounter = 0;
					}
					game_controller_input *OldKeyboardController = GetController(OldInput, 0);
					game_controller_input *NewKeyboardController = GetController(NewInput, 0);
					*NewKeyboardController = {};
					NewKeyboardController->IsConnected = true;
					for(int BtnIndex = 0; BtnIndex < ArrayCount(NewKeyboardController->Buttons); BtnIndex++)
					{
						NewKeyboardController->Buttons[BtnIndex].EndedDown = OldKeyboardController->Buttons[BtnIndex].EndedDown;
					}
					Win32ProcessMessage(NewKeyboardController);
										
					if(!bIsPaused)
					{
						DWORD MaxControllerCount = XUSER_MAX_COUNT;
						if(MaxControllerCount > (ArrayCount(NewInput->Controllers) - 1))
						{
							MaxControllerCount = (ArrayCount(NewInput->Controllers) - 1);
						}

						for(DWORD ControllerIndex = 0; ControllerIndex < MaxControllerCount; ++ControllerIndex) 
						{
							DWORD OurControllerIndex = ControllerIndex + 1;
							game_controller_input *OldController = GetController(OldInput, OurControllerIndex);
							game_controller_input *NewController = GetController(NewInput, OurControllerIndex);
							XINPUT_STATE ControllerState;
							if(XInputGetState_(ControllerIndex, &ControllerState) == ERROR_SUCCESS) {
								NewController->IsConnected = true;
								// Controller is plugged-in
								XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
															
								NewController->IsAnalog = true;
								NewController->StickAverageX = Win32ProcessXInputStickValue(Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
								NewController->StickAverageY = Win32ProcessXInputStickValue(Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);

								if(NewController->StickAverageX != 0.0f || NewController->StickAverageY != 0.0f)
								{
									NewController->IsAnalog = true;
								}
								if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
								{
									NewController->StickAverageY = 1;
									NewController->IsAnalog = false;
								}
								if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
								{
									NewController->StickAverageY = -1;
									NewController->IsAnalog = false;
								}
								if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
								{
									NewController->StickAverageX = -1;
									NewController->IsAnalog = false;
								}
								if (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
								{
									NewController->StickAverageX = 1;
									NewController->IsAnalog = false;
								}

								float Threshold = 0.5f;
								Win32ProcessXInputDigitalButton ((NewController->StickAverageX < -Threshold) ? 1 : 0, &OldController->MoveDown, 1, &NewController->MoveDown);
								Win32ProcessXInputDigitalButton ((NewController->StickAverageX > Threshold) ? 1 : 0, &OldController->MoveUp, 1, &NewController->MoveUp);
								Win32ProcessXInputDigitalButton ((NewController->StickAverageY < -Threshold) ? 1 : 0, &OldController->MoveRight, 1, &NewController->MoveRight);
								Win32ProcessXInputDigitalButton ((NewController->StickAverageY > -Threshold) ? 1 : 0, &OldController->MoveLeft, 1, &NewController->MoveLeft);

								Win32ProcessXInputDigitalButton (Pad->wButtons, &OldController->ActionDown, XINPUT_GAMEPAD_A, &NewController->ActionDown);
								Win32ProcessXInputDigitalButton (Pad->wButtons, &OldController->ActionRight, XINPUT_GAMEPAD_B, &NewController->ActionRight);
								Win32ProcessXInputDigitalButton (Pad->wButtons, &OldController->ActionLeft, XINPUT_GAMEPAD_X, &NewController->ActionLeft);
								Win32ProcessXInputDigitalButton (Pad->wButtons, &OldController->ActionUp, XINPUT_GAMEPAD_Y, &NewController->ActionUp);
								Win32ProcessXInputDigitalButton (Pad->wButtons, &OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER, &NewController->LeftShoulder);
								Win32ProcessXInputDigitalButton (Pad->wButtons, &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER, &NewController->RightShoulder);

								Win32ProcessXInputDigitalButton (Pad->wButtons, &OldController->Back, XINPUT_GAMEPAD_BACK, &NewController->Back);
								Win32ProcessXInputDigitalButton (Pad->wButtons, &OldController->Start, XINPUT_GAMEPAD_START, &NewController->Start);
							}
							else {
								// Controller is not available
								NewController->IsConnected = false;
							}
						}
						game_offscreen_buffer GBuffer = {};
						GBuffer.Memory = backBuffer.Memory;
						GBuffer.Width = backBuffer.Width;
						GBuffer.Height = backBuffer.Height;
						GBuffer.Pitch = backBuffer.Pitch;						
						Game.UpdateAndRender(&GameMemory, NewInput, &GBuffer);
						
						LARGE_INTEGER AudioWallClock = Win32GetWallClock();
						float FromBeginToAudioSeconds = Win32GetSecondsElapsed(FlipWallClock, AudioWallClock);

						DWORD PlayCursor = 0;
						DWORD WriteCursor = 0;
						if((GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor) == DS_OK))
						{
							if(!bIsSoundValid)
							{
								SoundOutput.CurrentSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
								bIsSoundValid = true;
							}
							
							DWORD ByteTolock = (SoundOutput.CurrentSampleIndex * SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
							DWORD ExpectedSoundBytesPerFrame = (SoundOutput.SamplesPerSecond * SoundOutput.BytesPerSample) / GameUpdateHz;
							
							float SecondsLeftUntilFlip = TargetSecondsPerFrame - FromBeginToAudioSeconds;
							DWORD ExpectedBytesToFlip = (DWORD)((SecondsLeftUntilFlip / TargetSecondsPerFrame) * (float)ExpectedSoundBytesPerFrame);

							DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedSoundBytesPerFrame;
							DWORD SafeWriteCursor = WriteCursor;
							if(SafeWriteCursor < PlayCursor)
							{
								SafeWriteCursor += SoundOutput.SecondaryBufferSize;
							}
							Assert(SafeWriteCursor >= PlayCursor);	// Safewritecursor should alwasy be greater than playcursor
							SafeWriteCursor += SoundOutput.SafetyBytes;
							bool bIsAudiocardLowLatency =  (SafeWriteCursor < ExpectedFrameBoundaryByte);
							DWORD TargetCursor = 0;	
							if(bIsAudiocardLowLatency)
							{
								TargetCursor = (ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame);
							}
							else
							{
								TargetCursor = (WriteCursor + ExpectedSoundBytesPerFrame + SoundOutput.SafetyBytes);
							}
							TargetCursor = (TargetCursor % SoundOutput.SecondaryBufferSize);
													
							DWORD BytesToWrite = 0;
							if(ByteTolock > TargetCursor)
							{
								BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteTolock);
								BytesToWrite += TargetCursor;
							}
							else
							{
								BytesToWrite = TargetCursor - ByteTolock;
							}
								
							game_sound_buffer SoundBuffer = {};
							SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
							SoundBuffer.Samples = Samples;
							SoundBuffer.SampleCount = BytesToWrite/SoundOutput.BytesPerSample;
							Game.GetSoundSamples (&GameMemory, &SoundBuffer);
	#if RAAVANAN_INTERNAL
							Win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
							Marker->OutputPlayCursor = PlayCursor;
							Marker->OutputWriteCursor = WriteCursor;
							Marker->OutputLocation = ByteTolock;
							Marker->OutputByteCount = BytesToWrite;
							Marker->ExpectedFlipPlayCursor = ExpectedFrameBoundaryByte;
							DWORD UnwrappedWriteCursor = WriteCursor;
							if(UnwrappedWriteCursor < PlayCursor)
							{
								UnwrappedWriteCursor += SoundOutput.SecondaryBufferSize;
							}
							AudioLatencyBytes = UnwrappedWriteCursor - PlayCursor;
							AudioLatencySeconds = (((float)AudioLatencyBytes / SoundOutput.BytesPerSample) / SoundOutput.SamplesPerSecond);

							char TxtBuffer[256];
							sprintf_s(TxtBuffer, "BTL: %u TC: %u BTW: %u - PC:%u, WC:%u DELTA: %u (%f	s)\n", ByteTolock, TargetCursor, BytesToWrite, PlayCursor, WriteCursor, AudioLatencyBytes, AudioLatencySeconds);
							OutputDebugStringA(TxtBuffer);
	#endif
							Win32FillSoundBuffer(&SoundOutput, ByteTolock, BytesToWrite, &SoundBuffer);
						}
						else
						{
							bIsSoundValid = false;
						}
						

						LARGE_INTEGER WorkCounter = Win32GetWallClock();
						float WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter);

						float SecondsElapsedForFrame = WorkSecondsElapsed;

						if(SecondsElapsedForFrame < TargetSecondsPerFrame)
						{
							if(SleepIsGranular)
							{
								DWORD SleepInMS = (DWORD)(1000.0f * (TargetSecondsPerFrame - SecondsElapsedForFrame));
								if(SleepInMS > 0)
								{
									Sleep(SleepInMS);	
								}
							}
							float TestSecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock());
							if(TestSecondsElapsedForFrame < TargetSecondsPerFrame)
							{
								// Missed sleep
							}
							while (SecondsElapsedForFrame < TargetSecondsPerFrame)
							{
								SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetWallClock()	);
							}
						}
						else
						{
							// Missed the Frame rate
						}
						
						LARGE_INTEGER EndCounter = Win32GetWallClock();
						float MSPerFrame = 1000.0f * Win32GetSecondsElapsed(LastCounter, EndCounter);
						LastCounter = EndCounter;

						win32_window_dimension dimension = GetWindowDimension(WindowHandle);
	#if RAAVANAN_INTERNAL
						Win32DebugSyncDisplay(&backBuffer, ArrayCount(DebugTimeMarkers), DebugTimeMarkers, DebugTimeMarkerIndex - 1, &SoundOutput, TargetSecondsPerFrame);
	#endif
						Win32UpdateBufferInWindow (&backBuffer, DeviceContext, dimension.Width, dimension.Height);
						//ReleaseDC(WindowHandle, DeviceContext);
						FlipWallClock = Win32GetWallClock();
	#if RAAVANAN_INTERNAL
						{
							DWORD PC = 0;
							DWORD WC = 0;
							if((GlobalSecondaryBuffer->GetCurrentPosition(&PC, &WC) == DS_OK))
							{
								Win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];							
								Marker->FlipPlayCursor = PC;
								Marker->FlipWriteCursor = WC;
							}						
						}
	#endif
						
						game_input *Temp = NewInput;
						NewInput = OldInput;
						OldInput = Temp;

						uint64 EndCycleCount = __rdtsc();
						uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
						LastCycleCount = EndCycleCount;
						#if 0
						float FPS = 0.0f; //(float)PerfCountFrequency/(float)CounterElapsed;
						double MegaCyclePerFrame = ((double)CyclesElapsed / (1000.0f * 1000.0f));
						char TextBuffer[256];
						sprintf_s(TextBuffer, "PC:%u BTL: %u TC: %u BTW: %u\n", LastPlayCursor, ByteTolock, TargetCursor, BytesToWrite);
						OutputDebugStringA(TextBuffer);
						#endif
#if RAAVANAN_INTERNAL
						++DebugTimeMarkerIndex;
						if(DebugTimeMarkerIndex >= ArrayCount(DebugTimeMarkers))
						{
							DebugTimeMarkerIndex = 0;
						}
#endif	
					}					
				}
			}
			else
			{
				
			}			
		}
	}
	return 0;
}