#include <windows.h>
#include <stdint.h>
#include <stddef.h>

#define internal static
#define local_persist static
#define global_variable static

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef i32 b32;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef size_t memory_index;

typedef float r32;
typedef double r64;

struct win32_offscreen_buffer
{
    BITMAPINFO Info;
    void *Memory;
    i32 Width;
    i32 Height;
    i32 Pitch;
    i32 BytesPerPixel;
    
    i32 TileOffset;
};

struct game_tile
{
    r32 Hostility;
    r32 Stability;
    r32 LeftSpread;
    r32 RightSpread;
    r32 UpSpread;
    r32 DownSpread;
};

struct game_room
{
    i32 Width;
    i32 Height;
    game_tile *Tiles;
};

struct game_state
{
    b32 Running;
    u32 Seed;
    i32 X;
    i32 Y;
    game_room Room;
    win32_offscreen_buffer Buffer;
};

global_variable game_state *GlobalGameState;

internal u32
AdvanceRandomNumber(u32 Number)
{
    u32 Result = Number;
    u32 Subtraction = (Number & 7187) * 941083981;
    u32 Addition = (Number & 141650963) * 433024223;
    Result += 23 + Addition - Subtraction;
    return(Result);
}

internal game_tile*
GetTile(game_state *GameState, int X, int Y)
{
    game_tile *Tile = (GameState->Room.Tiles + X + Y * GameState->Room.Width);
    return(Tile);
}

internal u32
ComputeColor(r32 Value, u32 Continuum)
{
    u32 Result;
    r32 Red;
    r32 Green;
    r32 Blue;
    u32 Spectrum = Continuum % 600;
    if(Spectrum < 100)
    {
        Red = 1.0f;
        Green = Spectrum / 100.0f;
        Blue = 0.0f;
    }
    else if(Spectrum < 200)
    {
        Red = (Spectrum - 100) / 100.0f;
        Green = 1.0f;
        Blue = 0.0f;
    }
    else if(Spectrum < 300)
    {
        Red = 0.0f;
        Green = 1.0f;
        Blue = (Spectrum - 200) / 100.0f;
    }
    else if(Spectrum < 400)
    {
        Red = 0.0f;
        Green = (Spectrum - 300) / 100.0f;
        Blue = 1.0f;
    }
    else if(Spectrum < 500)
    {
        Red = (Spectrum - 400) / 100.0f;
        Green = 0.0f;
        Blue = 1.0f;
    }
    else
    {
        Red = 1.0f;
        Green = 0.0f;
        Blue = (Spectrum - 500) / 100.0f;
    }
    Result = ((u32)(Red * Value) << 16) + ((u32)(Green * Value) << 8) + ((u32)(Blue * Value) << 0);
    return(Result);
}

internal void
RedrawRoom(game_state *GameState)
{
    win32_offscreen_buffer *Buffer = &GameState->Buffer;
    
    u8 *PixelRow = (u8 *)Buffer->Memory;
    int TileY = 0;
    int SubY = 0;
    for(int Y = 0;
        Y < Buffer->Height;
        ++Y)
    {
        u32 *Pixel = (u32 *)PixelRow;
        int TileX = 0;
        int SubX = 0;
        for(int X = 0;
            X < Buffer->Width;
            ++X)
        {
            if(SubX == 0 || SubY == 0)
            {
                *Pixel++ = 0x00000000;
            }
            else if(TileX == GameState->X && TileY == GameState->Y)
            {
                if((X & 1) == (Y & 1))
                {
                    *Pixel++ = 0x006F6F6F;
                }
                else
                {
                    *Pixel++ = 0x008F8F8F;
                }
            }
            else
            {
                game_tile *Tile = GetTile(GameState, TileX, TileY);
                u32 Red = (u32)(Tile->Hostility * 256.0);
                u32 Blue = (u32)(Tile->Stability * 256.0);
                if(Red < 0)
                    Red = 0;
                if(Red > 255)
                    Red = 255;
                if(Blue < 0)
                    Blue = 0;
                if(Blue > 255)
                    Blue= 255;
                *Pixel++ = (Red  << 16) + (Blue << 0);
            }
            
            if(++SubX == Buffer->TileOffset)
            {
                SubX = 0;
                ++TileX;
            }
        }
        if(++SubY == Buffer->TileOffset)
        {
            SubY = 0;
            ++TileY;
        }
        PixelRow += Buffer->Pitch;
    }
}

internal void
SetStage(game_state *GameState)
{
    i32 Height = 64;
    i32 Width = 64;
    i32 CenterX = Width / 2;
    i32 CenterY = Height / 2;
    GameState->X = CenterX;
    GameState->Y = CenterY;
    
    game_room *Room = &GameState->Room;
    Room->Height = Height;
    Room->Width = Width;
    Room->Tiles = (game_tile *)(GameState + 1);
    
    u32 Random = AdvanceRandomNumber(GameState->Seed);
    
    game_tile *TileRow = Room->Tiles;
    for(i32 Y = 0;
        Y < Height;
        ++Y)
    {
        game_tile *Tile = TileRow;
        for(i32 X = 0;
            X < Width;
            ++X)
        {
            r32 RelativeX = (r32)(X - CenterX) / (r32)CenterX;
            r32 RelativeY = (r32)(Y - CenterY) / (r32)CenterY;
            Tile->Hostility =  RelativeX * RelativeX + RelativeY * RelativeY;
            if(X == CenterX && Y == CenterY)
            {
                Tile->Stability = 1.0;
            }
            else
            {
                Tile->Stability = 0.0;
            }
            r32 Spread = 0.5f * (r32)0x10000;
            r32 Shift = 0.5f * Spread;
            Tile->LeftSpread = ((r32)(Random & 0x10000) - Shift) / Spread;
            Random = AdvanceRandomNumber(Random);
            Tile->RightSpread = ((r32)(Random & 0x10000) - Shift) / Spread;
            Random = AdvanceRandomNumber(Random);
            Tile->UpSpread = ((r32)(Random & 0x10000) - Shift) / Spread;
            Random = AdvanceRandomNumber(Random);
            Tile->DownSpread = ((r32)(Random & 0x10000) - Shift) / Spread;
            Random = AdvanceRandomNumber(Random);
            
            ++Tile;
        }
        
        TileRow += Width;
    }
    
    win32_offscreen_buffer *Buffer = &GameState->Buffer;
    int BytesPerPixel = 4;
    int TileWidth = 5;
    int TileSpace = 1;
    int TileOffset = TileWidth + TileSpace;
    Buffer->Width = Width * TileOffset + TileSpace;
    Buffer->Height = Height * TileOffset + TileSpace;
    Buffer->BytesPerPixel = BytesPerPixel;
    Buffer->Pitch = (Buffer->Width * BytesPerPixel + 15) & ~15;
    Buffer->TileOffset = TileOffset;
    Buffer->Memory = (void *)(Room->Tiles + Height * Width);
    
    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Pitch / BytesPerPixel;
    Buffer->Info.bmiHeader.biHeight = Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;
    
    RedrawRoom(GameState);
}

internal void
PlayerMoveFor(game_state *GameState, int RelativeX, int RelativeY)
{
    int NewX = GameState->X + RelativeX;
    int NewY = GameState->Y + RelativeY;
    
    if(NewX >= 0 && NewX < GameState->Room.Width && NewY >= 0 && NewY < GameState->Room.Height)
    {
        GameState->X += RelativeX;
        GameState->Y += RelativeY;
        RedrawRoom(GameState);
    }
}

LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
                        UINT Message,
                        WPARAM WParam,
                        LPARAM LParam)
{
    LRESULT Result = 0;
    
    switch(Message)
    {
        case WM_DESTROY:
        case WM_CLOSE:
        {
            GlobalGameState->Running = false;
        } break;
        
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            u32 VKCode = (u32)WParam;// TODO(Zyonji): Should I switch to scan codes instead?
            b32 WasDown = ((LParam & (1 << 30)) != 0);
            b32 IsDown = ((LParam & (1 << 31)) == 0);
            if(IsDown && !WasDown)
            {
                if(VKCode == 'W')
                {
                    PlayerMoveFor(GlobalGameState, 0, 1);
                }
                else if(VKCode == 'A')
                {
                    PlayerMoveFor(GlobalGameState, -1, 0);
                }
                else if(VKCode == 'S')
                {
                    PlayerMoveFor(GlobalGameState, 0, -1);
                }
                else if(VKCode == 'D')
                {
                    PlayerMoveFor(GlobalGameState, 1, 0);
                }
                else if(VKCode == VK_UP)
                {
                    PlayerMoveFor(GlobalGameState, 0 ,1);
                }
                else if(VKCode == VK_LEFT)
                {
                    PlayerMoveFor(GlobalGameState, -1, 0);
                }
                else if(VKCode == VK_DOWN)
                {
                    PlayerMoveFor(GlobalGameState, 0, -1);
                }
                else if(VKCode == VK_RIGHT)
                {
                    PlayerMoveFor(GlobalGameState, 1, 0);
                }
                
                RedrawWindow(Window, 0, 0, RDW_INVALIDATE);
            }
            
            b32 AltKeyWasDown = (LParam & (1 << 29));
            if((VKCode == VK_F4) && AltKeyWasDown)
            {
                GlobalGameState->Running = false;
            }
        } break;
        
        case WM_PAINT:
        {
            RECT ClientRect;
            GetClientRect(Window, &ClientRect);
            u32 WindowWidth = ClientRect.right - ClientRect.left;
            u32 WindowHeight = ClientRect.bottom - ClientRect.top;
            
            win32_offscreen_buffer *Buffer = &GlobalGameState->Buffer;
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            StretchDIBits(DeviceContext,
                          0, 0, WindowWidth, WindowHeight,
                          0, 0, Buffer->Width, Buffer->Height,
                          Buffer->Memory,
                          &Buffer->Info,
                          DIB_RGB_COLORS, SRCCOPY);
            EndPaint(Window, &Paint);
        } break;
        
        default:
        {
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }
    
    return(Result);
}

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    int MemorySize = sizeof(*GlobalGameState) + 15000 * sizeof(*GlobalGameState->Room.Tiles) + 601 * 901 * 4;
    void *Memory = VirtualAlloc(0, MemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    GlobalGameState = (game_state *)Memory;
    GlobalGameState->Running = true;
    GlobalGameState->Seed = 4223;
    
    SetStage(GlobalGameState);
    
    WNDCLASS WindowClass = {};
    
    WindowClass.style = CS_HREDRAW|CS_VREDRAW|CS_OWNDC;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.hCursor = LoadCursor(0, IDC_ARROW);
    //    WindowClass.hIcon;
    WindowClass.lpszClassName = "PathsWindowClass";
    
    if(RegisterClassA(&WindowClass))
    {
        HWND Window =
            CreateWindowExA(0,
                            WindowClass.lpszClassName,
                            "Paths",
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
        {
            UpdateWindow(Window);
            BOOL MessageResult;
            MSG Message;
            
            while(GlobalGameState->Running && (MessageResult = GetMessageA(&Message, 0, 0, 0)))
            {
                if(MessageResult > 0)
                {
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }
                else
                {
                    GlobalGameState->Running= false;
                }
            }
        }
        else
        {
            MessageBoxA(0, "A window could not be created.", 0, MB_OK|MB_ICONERROR);
        }
    }
    else
    {
        MessageBoxA(0, "A window class could not be registered.", 0, MB_OK|MB_ICONERROR);
    }
    
    return(0);
}