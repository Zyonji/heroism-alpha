#include <windows.h>
#include <stdint.h>
#include <stddef.h>

#include <stdio.h>

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
    r32 Volatility;
    r32 X;
    r32 Y;
    r32 Pull;
    r32 Strenght;
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
    r32 Health;
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
                    i32 Green = (i32)(GameState->Health * 256.0);
                    if(Green < 0)
                        Green= 0;
                    if(Green > 255)
                        Green= 255;
                    *Pixel++ = Green << 8;
                }
            }
            else
            {
                game_tile *Tile = GetTile(GameState, TileX, TileY);
                i32 Red = (i32)(Tile->Hostility * 256.0);
                i32 Blue = 255 - (i32)(Tile->Volatility * 256.0);
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
    game_room *Room = &GameState->Room;
    i32 Height = Room->Height;
    i32 Width = Room->Width;
    i32 CenterX = Width / 2;
    i32 CenterY = Height / 2;
    GameState->X = CenterX;
    GameState->Y = CenterY;
    
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
                Tile->Volatility = 0.0;
            }
            else
            {
                Tile->Volatility = 1.0;
            }
            r32 Full = (r32)0x10000;
            r32 Half = 0.5f * Full;
            
            Tile->X = ((r32)(Random & 0x10000) - Half) / Half;
            Random = AdvanceRandomNumber(Random);
            Tile->Y = ((r32)(Random & 0x10000) - Half) / Half;
            Random = AdvanceRandomNumber(Random);
            Tile->Pull = 0.8f + 0.2f * ((r32)(Random & 0x10000)) / Full;
            Random = AdvanceRandomNumber(Random);
            Tile->Strenght = 0.5f * ((r32)(Random & 0x10000) + Half) / Full;
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

internal game_tile*
GetTile(game_tile *Tiles, i32 Width, i32 Height, i32 X, i32 Y)
{
    i32 WrappedX = X;
    if(WrappedX < 0)
        WrappedX += Width;
    if(WrappedX >= Width)
        WrappedX -= Width;
    i32 WrappedY = Y;
    if(WrappedY < 0)
        WrappedY += Height;
    if(WrappedY >= Height)
        WrappedY -= Height;
    game_tile *Tile = Tiles + WrappedY * Width + WrappedX;
    return Tile;
}

internal void
AddToTileHostility(game_tile *Tiles, i32 Width, i32 Height, i32 X, i32 Y, r32 Hostility)
{
    game_tile *Tile = GetTile(Tiles, Width, Height, X, Y);
    Tile->Hostility += Hostility * Tile->Volatility;
    Tile->Volatility += 0.05f * Hostility;
    if(Tile->Volatility > 1.0f)
        Tile->Volatility = 1.0f;
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

inline LARGE_INTEGER
Win32GetAbsoluteTime(void)
{    
    LARGE_INTEGER Result;
    QueryPerformanceCounter(&Result);
    return(Result);
}

global_variable i64 GlobalPerfCountFrequency;
inline r32
Win32GetSecondsElapsed(LARGE_INTEGER Start, LARGE_INTEGER End)
{
    r32 Result = ((r32)(End.QuadPart - Start.QuadPart) /
                  (r32)GlobalPerfCountFrequency);
    return(Result);
}

int CALLBACK
WinMain(HINSTANCE Instance,
        HINSTANCE PrevInstance,
        LPSTR CommandLine,
        int ShowCode)
{
    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    GlobalPerfCountFrequency = PerfCountFrequencyResult.QuadPart;
    
    b32 SleepIsGranular = (timeBeginPeriod(1) == TIMERR_NOERROR);
    
    i32 Height = 64;
    i32 Width = 64;
    int MemorySize = sizeof(*GlobalGameState) + (4 * 6 * 6 + sizeof(*GlobalGameState->Room.Tiles)) * (Height + 1) * (Width + 1) + 15;
    void *Memory = VirtualAlloc(0, MemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    GlobalGameState = (game_state *)Memory;
    GlobalGameState->Running = true;
    GlobalGameState->Seed = 420023;
    GlobalGameState->Health = 1.0f;
    GlobalGameState->Room.Height = Height;
    GlobalGameState->Room.Width = Width;
    
    SetStage(GlobalGameState);
    game_tile *Tiles = GlobalGameState->Room.Tiles;
    
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
            MSG Message;
            
            LARGE_INTEGER LastCounter = Win32GetAbsoluteTime();
            r32 DesiredSecondsPerFrame = 1.0 / 30.0;
            
            while(GlobalGameState->Running)
            {
                while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if(Message.message == WM_QUIT)
                    {
                        GlobalGameState->Running= false;
                    }
                    else
                    {
                        TranslateMessage(&Message);
                        DispatchMessageA(&Message);
                    }
                }
                // NOTE(Zyonji): Game logic.
                game_tile *Location = Tiles + GlobalGameState->Y * Width + GlobalGameState->X;
                GlobalGameState->Health += (0.25f - 0.25f * Location->Volatility - Location->Hostility) * DesiredSecondsPerFrame;
                if(GlobalGameState->Health < 0.0f)
                {
                    MessageBoxA(0, "You died.", 0, MB_OK|MB_ICONERROR);
                    GlobalGameState->Running = false;
                }
                if(GlobalGameState->Health > 1.0f)
                {
                    GlobalGameState->Health = 1.0f;
                }
                Location->Volatility -= 0.5f * DesiredSecondsPerFrame;
                if(Location->Volatility < 0.0f)
                    Location->Volatility = 0.0f;
                
                game_tile *TileRow = Tiles;
                for(i32 Y = 0;
                    Y < Height;
                    ++Y)
                {
                    game_tile *Tile = TileRow;
                    for(i32 X = 0;
                        X < Width;
                        ++X)
                    {
                        if(Tile->Hostility > 1.0f)
                        {
                            Tile->Hostility = 0.5f * (Tile->Hostility - 1.0f) + 1.0f;
                        }
                        if(Tile->Hostility < 0.0f)
                        {
                            Tile->Hostility *= 0.1f;
                        }
                        
                        r32 HostileDelta = Tile->Hostility * Tile->Strenght * DesiredSecondsPerFrame;
                        r32 Loss = 0.0;
                        if(Tile->X > 0)
                        {
                            AddToTileHostility(Tiles, Width, Height, X + 1, Y, Tile->X * HostileDelta);
                            Loss += Tile->X;
                        }
                        else
                        {
                            AddToTileHostility(Tiles, Width, Height, X - 1, Y, -Tile->X * HostileDelta);
                            Loss -= Tile->X;
                        }
                        if(Tile->Y > 0)
                        {
                            AddToTileHostility(Tiles, Width, Height, X, Y + 1, Tile->Y * HostileDelta);
                            Loss += Tile->Y;
                        }
                        else
                        {
                            AddToTileHostility(Tiles, Width, Height, X, Y - 1, -Tile->Y * HostileDelta);
                            Loss -= Tile->Y;
                        }
                        game_tile *LeftTile = GetTile(Tiles, Width, Height, X - 1, Y);
                        game_tile *RightTile = GetTile(Tiles, Width, Height, X + 1, Y);
                        Tile->X -= (RightTile->Pull * RightTile->Hostility - LeftTile->Pull * LeftTile->Hostility) * DesiredSecondsPerFrame;
                        game_tile *DownTile = GetTile(Tiles, Width, Height, X, Y - 1);
                        game_tile *UpTile = GetTile(Tiles, Width, Height, X, Y + 1);
                        Tile->Y -= (UpTile->Pull * UpTile->Hostility - DownTile->Pull * DownTile->Hostility) * DesiredSecondsPerFrame;
                        
                        Tile->Hostility -= 0.9f * HostileDelta * Loss;
                        if(Loss > 1.0f)
                        {
                            Loss = 0.5f * (Loss - 1.0f) + 1.0f;
                            Tile->X /= Loss;
                            Tile->Y /= Loss;
                        }
                        ++Tile;
                    }
                    
                    TileRow += Width;
                }
                // TODO(Zyonji): Wait to get stable framerate.
                r32 SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetAbsoluteTime());
                if(SecondsElapsedForFrame < DesiredSecondsPerFrame)
                {                        
                    if(SleepIsGranular)
                    {
                        DWORD SleepMS = (DWORD)(1000.0f * (DesiredSecondsPerFrame - SecondsElapsedForFrame));
                        if(SleepMS > 0)
                        {
                            Sleep(SleepMS);
                        }
                    }
                    while(SecondsElapsedForFrame < DesiredSecondsPerFrame)
                    {                            
                        SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32GetAbsoluteTime());
                    }
                }
                LastCounter = Win32GetAbsoluteTime();
                // NOTE(Zyonji): Render frame.
                RedrawRoom(GlobalGameState);
                RedrawWindow(Window, 0, 0, RDW_INVALIDATE);
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