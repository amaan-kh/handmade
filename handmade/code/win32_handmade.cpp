#include<windows.h>

#define local_persist static;
#define global_variable static;
#define internal static;

global_variable bool Running;

global_variable BITMAPINFO BitmapInfo;
global_variable void *BitmapMemory;
global_variable int BitmapHeight;
global_variable int BitmapWidth;


internal void
Win32ResizeDIBSection(int Width, int Height)
{
  if(BitmapMemory)
    {
      VirtualFree(BitmapMemory, 0, MEM_RELEASE); 
    }
  BitmapInfo.bmiHeader.biSize = sizeof(BitmapInfo.bmiHeader);
  BitmapInfo.bmiHeader.biWidth = BitmapWidth;
  BitmapInfo.bmiHeader.biHeight = BitmapHeight;
  BitmapInfo.bmiHeader.biPlanes = 1;
  BitmapInfo.bmiHeader.biBitCount = 32;
  BitmapInfo.bmiHeader.biCompression = BI_RGB;

  BitmapWidth = Width;
  BitmapHeight = Height;

  int BytesPerPixel = 4;
  int BitmapMemorySize = BytesPerPixel * (Width * Height);

  BitmapMemory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void
Win32UpdateWindow(HDC DeviceContext, RECT *ClientRect)
{
  int WindowWidth = ClientRect->right - ClientRect->left;
  int WindowHeight = ClientRect->bottom - ClientRect->top;
  StretchDIBits(DeviceContext,
		/*
		X, Y, Width, Height,
		X, Y, Width, Height,
		*/
		0, 0, BitmapWidth, BitmapHeight,
		0, 0, WindowWidth, WindowHeight,
		BitmapMemory,
		&BitmapInfo,
		DIB_RGB_COLORS, SRCCOPY);    
  
}

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
	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	int Width = ClientRect.right - ClientRect.left;
	int Height = ClientRect.bottom - ClientRect.top;
	Win32ResizeDIBSection(Width, Height);
      } break;
      
    case WM_DESTROY:
      {
		Running = false;	
	OutputDebugStringA("WM_DESTROY\n");
      } break;
      
    case WM_CLOSE:
      {
	PostQuitMessage(0);
		Running = false;
	OutputDebugStringA("WM_CLOSE\n");
      } break;
      
    case WM_ACTIVATEAPP:
      {
	OutputDebugStringA("WM_ACTIVATEAPP\n");
      } break;

    case WM_PAINT:
      {
	PAINTSTRUCT Paint;
        HDC DeviceContext = BeginPaint(Window, &Paint);
	
        // Do our painting here
	RECT ClientRect;
	GetClientRect(Window, &ClientRect);
	Win32UpdateWindow(DeviceContext, &ClientRect);
	
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
  
  WNDCLASSA WindowClass = {};
  WindowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
  WindowClass.lpfnWndProc = Win32MainWindowCallback;
  WindowClass.hInstance = Instance;
  // WindowClass.hIcon;
  WindowClass.lpszClassName = "HandmadeHeroWindowClass";

  if(RegisterClassA(&WindowClass))
    {
      
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
      if(Window)
	{ Running = true;
	  while(Running)                            // a for loop which would run forever
	    {
	      MSG Message;
	      BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
	      if (MessageResult > 0)          // 0 is the WM_QUIT message, -1 is invalid
		{
		  // Do work in your window
		  TranslateMessage(&Message);
		  DispatchMessageA(&Message);
		}
	      else
		{
		  break;                      // break out of the loop
		}
	    
	    }
	}
      else
	{
	}

    }
  else
    {
      // Window Class Registration failed
      // TODO(casey): Logging
    }
  
  return (0);
  
  
}
