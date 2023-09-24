
#include<windows.h>
#include<stdint.h>
#include<xinput.h>

// unsigned integers
typedef uint8_t u8;     // 1-byte long unsigned integer
typedef uint16_t u16;   // 2-byte long unsigned integer
typedef uint32_t u32;   // 4-byte long unsigned integer
typedef uint64_t u64;   // 8-byte long unsigned integer
// signed integers
typedef int8_t s8;      // 1-byte long signed integer
typedef int16_t s16;    // 2-byte long signed integer
typedef int32_t s32;    // 4-byte long signed integer
typedef int64_t s64;    // 8-byte long signed integer

// static for different purposes
#define local_persist static;
#define global_variable static;
#define internal static;

global_variable bool GlobalRunning;

// window dimensions
struct win32_window_dimension
{
  int Width;
  int Height;
};

// gets window dimensions
internal win32_window_dimension 
Win32GetWindowDimension(HWND Window)
{
  win32_window_dimension Result;
  RECT ClientRect;
  GetClientRect(Window, &ClientRect);    
  Result.Width = ClientRect.right - ClientRect.left;
  Result.Height = ClientRect.bottom - ClientRect.top;    
  return(Result);  
}

//buffer information
struct win32_offscreen_buffer
{
  BITMAPINFO Info;
  void *Memory;
  int Width;
  int Height;
  int Pitch;
  int BytesPerPixel;
};

//state buffer and window dimensions initialized
global_variable bool Running;
global_variable win32_offscreen_buffer GlobalBackbuffer;
internal win32_window_dimension;

// Create an "alias" to be able to call it with its old name
#define XInputGetState XInputGetState_ 
#define XInputSetState XInputSetState_

// Define a type of a function
typedef DWORD WINAPI x_input_get_state(DWORD dwUserIndex, XINPUT_STATE *pState);
typedef DWORD WINAPI x_input_set_state(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration);

// Get Input State
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) 
{ 
    return (ERROR_DEVICE_NOT_CONNECTED); 
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;


// Set Input State
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) 
{ 
    return (ERROR_DEVICE_NOT_CONNECTED); 
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;


internal void
Win32LoadXInput()
{
  HMODULE XInputLibrary = LoadLibraryA("Xinput1_4.dll"); 
  if (!XInputLibrary)
    {
        XInputLibrary = LoadLibraryA("Xinput1_3.dll"); 
    }
  if (!XInputLibrary)
    {
      XInputLibrary = LoadLibraryA("Xinput9_1_0.dll"); 
    }
  if(XInputLibrary)
    {
      XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
      if(!XInputGetState) { XInputGetState = XInputGetStateStub; }
      XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
      if(!XInputSetState) { XInputSetState = XInputSetStateStub; }
      else
	{
	  // We still don't have any XInputLibrary
	  XInputGetState = XInputGetStateStub;
	  XInputSetState = XInputSetStateStub;
	  // TODO(casey): Diagnostic
	}
    }
}


//function that renders
internal void
RenderWeirdGradient(win32_offscreen_buffer *Buffer, int XOffset, int YOffset)
{
    u8 *Row = (u8 *)Buffer->Memory;
    for (int Y = 0;
         Y < Buffer->Height;
         ++Y)
      {  
	Row += Buffer->Pitch;
      }
}

// allocates memory and initializes buffer
internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height)
{
  if(Buffer->Memory)
    {
    VirtualFree(Buffer->Memory, 0, MEM_RELEASE); 
    }
  Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
  Buffer->Info.bmiHeader.biWidth = Buffer->Width;
  Buffer->Info.bmiHeader.biHeight = -Buffer->Height; //negative for top-down rendering
  Buffer->Info.bmiHeader.biPlanes = 1;
  Buffer->Info.bmiHeader.biBitCount = 32;
  Buffer->Info.bmiHeader.biCompression = BI_RGB;
  Buffer->Width = Width;
  Buffer->Height = Height;
  Buffer->BytesPerPixel = 4;
  Buffer->Pitch = Buffer->Width * Buffer->BytesPerPixel;

  int BitmapMemorySize = Buffer->BytesPerPixel * (Buffer->Width * Buffer->Height);

  Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
}

// handles displaying from buffer to window
internal void
Win32DisplayBufferInWindow(win32_offscreen_buffer *Buffer, HDC DeviceContext, int WindowWidth, int WindowHeight)
{
  //TODO : aspect ratio correction, curr: fixed buffer
  StretchDIBits(DeviceContext,
		0, 0, WindowWidth, WindowHeight,
		0, 0, Buffer->Width, Buffer->Height,
		Buffer->Memory,
		&Buffer->Info,
		DIB_RGB_COLORS, SRCCOPY);      
}

// callback to define messages sent to the window
LRESULT CALLBACK
Win32MainWindowCallback(HWND Window, 
                   UINT Message,
                   WPARAM WParam,
                   LPARAM LParam)
{
  LRESULT Result = 0;
  switch (Message)
    {
    case WM_SIZE:
      {
        win32_window_dimension Dimension = Win32GetWindowDimension(Window);
	Win32ResizeDIBSection(&GlobalBackbuffer, Dimension.Width, Dimension.Height);
      } break;
      
    case WM_DESTROY:
      {
	GlobalRunning = false;	
	OutputDebugStringA("WM_DESTROY\n");
      } break;
      
    case WM_CLOSE:
      {
	PostQuitMessage(0);
	GlobalRunning = false;
	OutputDebugStringA("WM_CLOSE\n");
      } break;
      
    case WM_ACTIVATEAPP:
      {
	OutputDebugStringA("WM_ACTIVATEAPP\n");
      } break;

    case WM_PAINT:
      {
	//paints on window
	PAINTSTRUCT Paint;
        HDC DeviceContext = BeginPaint(Window, &Paint);
	win32_window_dimension Dimension = Win32GetWindowDimension(Window);
	Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);	
        EndPaint(Window, &Paint);
      } break;
      
    default:
      {
	// Do something in case of any other message
	Result = DefWindowProc(Window, Message, WParam, LParam);
      } break;
    }
  return (Result);
    
}

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR     CommandeLine,
        int       ShowCode)

{
  //calling libraries
  Win32LoadXInput();
  //creating a window-class
  WNDCLASSA WindowClass = {};
  WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  WindowClass.hInstance = Instance;
  // WindowClass.hIcon;
  WindowClass.lpszClassName = "HandmadeHeroWindowClass";

  if(RegisterClassA(&WindowClass))
    {
      //creating window
      HWND Window =
	CreateWindowExA(
			0,
			WindowClass.lpszClassName,
			"Handmade Hero",
			WS_OVERLAPPEDWINDOW|WS_VISIBLE,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			CW_USEDEFAULT,
			0,
			0,
			Instance,
			0);
      
      //if window has been created
      if(Window)
	{
	  //allocating buffer with dimensions 1280 x 720
	  Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);
	  HDC DeviceContext = GetDC(Window);
	  int XOffset = 0;
	  int YOffset = 0;
	  GlobalRunning = true;

	  //game-loop
	  while(GlobalRunning)
	    {
	      MSG Message;
	      while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
		{
		  if (Message.message == WM_QUIT)
		    {
		      GlobalRunning = false;
		    }
		  TranslateMessage(&Message);
		  DispatchMessageA(&Message);
		}

	      //should this be polled more frequently 
	      for (DWORD ControllerIndex = 0;
		   ControllerIndex < XUSER_MAX_COUNT;
		   ++ControllerIndex)
		{
		  XINPUT_STATE ControllerState;
		  if (XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
		    {
		      //controller is plugged in
		      //see if ControllerState.dwPacketNumber increments too quickly
		      XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
		      bool Up = Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP;
		      bool Down          = Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN;
		      bool Left          = Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT;
		      bool Right         = Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT;
		      bool Start         = Pad->wButtons & XINPUT_GAMEPAD_START;
		      bool Back          = Pad->wButtons & XINPUT_GAMEPAD_BACK;
		      bool LeftShoulder  = Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER;
		      bool RightShoulder = Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
		      bool A             = Pad->wButtons & XINPUT_GAMEPAD_A;
		      bool B             = Pad->wButtons & XINPUT_GAMEPAD_B;
		      bool X             = Pad->wButtons & XINPUT_GAMEPAD_X;
		      bool Y             = Pad->wButtons & XINPUT_GAMEPAD_Y;
		      s16 StickX = Pad->sThumbLX;
		      s16 StickY = Pad->sThumbLY;
		      if(A)
			{
			  YOffset += 2;
			}
		    }
		  else
		    {
		      // controller is not available
		    }
		}
	      
	      //rendering 
	      RenderWeirdGradient(&GlobalBackbuffer, XOffset, YOffset);
	      ++XOffset;
	      
	      win32_window_dimension Dimension = Win32GetWindowDimension(Window);
	      Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);
	    
	    }
	  
	    
	}
      else
	{
	}
      
    }
  else
    {
      // Window Class Registration failed
      // TODO: Logging
    }
  
  return (0);
  
  
}
