// ImGui - standalone example application for DirectX 11
// If you are new to ImGui, see examples/README.txt and documentation at the top of imgui.cpp.
#include <initguid.h> //included before d3d11.h for LNK errors of GUID's 
#include "Base.h"
#include "imgui.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <tchar.h>


#include <ft2Build.h>
#include <icu.h>
//#include <icucommon.h>
#include "../Core/Core.h"
#include "fribidi.h"
#include "VideoPlayerDX.h"


#pragma comment(lib, "dxgi")

Test0* gTest0;

//Unicode has 17 planes each one capable of containing 65536 distinct characters.
//whithin one plane, the range of code points is hexadecimal 0000 to FFFF
char* GetTestUTF8Shaped()
{
	UChar* src = (UChar*)L"سلامaliخوبی؟";

// 	{
// 		char* testDts = new char[32];
// 		size_t shapeSize = u_strlen(src) + 1;
// 		UChar* shaped = new UChar[shapeSize];
// 		UErrorCode err = U_ZERO_ERROR;
// 		u_shapeArabic(src, -1, shaped, shapeSize, U_SHAPE_LETTERS_SHAPE | U_SHAPE_LENGTH_FIXED_SPACES_NEAR | U_SHAPE_TEXT_DIRECTION_LOGICAL, &err);
// 		u_strToUTF8(testDts, 32, nullptr, shaped, -1, &err);
// 		return testDts;
// 	}

	UBiDi* gUBidi = ubidi_open();
	
	UErrorCode err = U_ZERO_ERROR;
	ubidi_setPara(gUBidi, src, -1, UBIDI_DEFAULT_LTR, nullptr, &err);
	VASSERT(err == U_ZERO_ERROR);
	size_t dstSize = u_strlen(src);
	UChar* ordered = new UChar[dstSize + 1];
	err = U_ZERO_ERROR;
	ubidi_writeReordered(gUBidi, ordered, dstSize + 1, UBIDI_DO_MIRRORING, &err);
	VASSERT(err == U_ZERO_ERROR);
	UChar* shaped = new UChar[dstSize + 1];
	err = U_ZERO_ERROR;
	size_t shapedNum = u_shapeArabic(ordered, -1, shaped, dstSize + 1, U_SHAPE_LETTERS_SHAPE | U_SHAPE_TEXT_DIRECTION_VISUAL_LTR, &err);
	VASSERT(err == U_ZERO_ERROR);
	shapedNum += 10;
	char* utf8Shaped = new char[shapedNum * 8];
	err = U_ZERO_ERROR;
	u_strToUTF8(utf8Shaped, shapedNum * 8, nullptr, shaped, -1, &err);
	VASSERT(err == U_ZERO_ERROR);
	ubidi_close(gUBidi);

	return utf8Shaped;
}

void MyUpdate()
{
	gTest0->Update(ImGui::GetIO().DeltaTime);
}
void MyRender()
{
	gTest0->Render(ImGui::GetIO().DeltaTime);
}
void MyShutdown()
{
	delete gTest0;
}
void MyInit()
{
	av_register_all();
}


void CreateRenderTarget()
{
    DXGI_SWAP_CHAIN_DESC sd;
    gSwapChain->GetDesc(&sd);

    // Create the render target
    ID3D11Texture2D* pBackBuffer;
    D3D11_RENDER_TARGET_VIEW_DESC render_target_view_desc;
    ZeroMemory(&render_target_view_desc, sizeof(render_target_view_desc));
    render_target_view_desc.Format = sd.BufferDesc.Format;
    render_target_view_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    gSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
    gDXDevice->CreateRenderTargetView(pBackBuffer, &render_target_view_desc, &gMainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget()
{
    if (gMainRenderTargetView) { gMainRenderTargetView->Release(); gMainRenderTargetView = NULL; }
}
//////////////////////////////////////////////////////////////////////////
void CreateVideoStuff()
{
	HRESULT hr = gDXDevice->QueryInterface(&gDXVideoDevice); //get video device
	VASSERT(SUCCEEDED(hr));
	hr = gDXDeviceCtx->QueryInterface(&gDXVideoDeviceCtx); //get video context
	VASSERT(SUCCEEDED(hr));
}
HRESULT CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	
	//IDXGIFactory* factory = nullptr;
	//CreateDXGIFactory(IID_PPV_ARGS(&factory));
	//IDXGIAdapter* adapter = nullptr;
	//factory->EnumAdapters(0, &adapter);

    UINT createDeviceFlags = 0;
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
	HRESULT hr = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, NULL, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &gSwapChain, &gDXDevice, &featureLevel, &gDXDeviceCtx);
	VASSERT(SUCCEEDED(hr));

    CreateRenderTarget();

	CreateVideoStuff();

	gTest0 = new Test0;

    return S_OK;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();
    if (gSwapChain) { gSwapChain->Release(); gSwapChain = NULL; }
    if (gDXDeviceCtx) { gDXDeviceCtx->Release(); gDXDeviceCtx = NULL; }
    if (gDXDevice) { gDXDevice->Release(); gDXDevice = NULL; }
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_SIZE:
        if (gDXDevice != NULL && wParam != SIZE_MINIMIZED)
        {
            ImGui_ImplDX11_InvalidateDeviceObjects();
            CleanupRenderTarget();
            gSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
            ImGui_ImplDX11_CreateDeviceObjects();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int main(int, char**)
{
	ImGui::CreateContext();

    // Create application window
    WNDCLASSEX wc = { sizeof(WNDCLASSEX), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(NULL), NULL, LoadCursor(NULL, IDC_ARROW), NULL, NULL, _T("ImGui Example"), NULL };
    RegisterClassEx(&wc);
    HWND hwnd = CreateWindow(_T("ImGui Example"), _T("ImGui DirectX11 Example"), WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, NULL, NULL, wc.hInstance, NULL);

    // Initialize Direct3D
    if (CreateDeviceD3D(hwnd) < 0)
    {
        CleanupDeviceD3D();
        UnregisterClass(_T("ImGui Example"), wc.hInstance);
        return 1;
    }

    // Show the window
    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    // Setup ImGui binding
    
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui_ImplDX11_Init(hwnd, gDXDevice, gDXDeviceCtx);
    //io.NavFlags |= ImGuiNavFlags_EnableKeyboard;  // Enable Keyboard Controls

    // Setup style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them. 
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple. 
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Read 'misc/fonts/README.txt' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //io.Fonts->AddFontDefault();
	
	static const ImWchar ranges[] =
	{
		0x0020, 0x00FF, // Basic Latin + Latin Supplement
		0x600, 0x6FF, //Arabic
		0xFB50,0xFDFF, //Arabic Presentation Forms-A
		0xFE70, 0xFEFF, //Arabic Presentation Forms-B
		0,
	};

    io.Fonts->AddFontFromFileTTF("../../../EditorResources/tahoma.ttf", 16.0f, nullptr, ranges);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/ProggyTiny.ttf", 10.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);

    bool show_demo_window = true;
    bool show_another_window = false;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

	MyInit();
    // Main loop
    MSG msg;
    ZeroMemory(&msg, sizeof(msg));
    while (msg.message != WM_QUIT)
    {
        // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
        // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
        // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
        // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
        if (PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            continue;
        }
        ImGui_ImplDX11_NewFrame();

		MyUpdate();
        // 1. Show a simple window.
        // Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets automatically appears in a window called "Debug".
        if(0)
		{
            static float f = 0.0f;
            static int counter = 0;
            ImGui::Text("Hello, world!");                           // Display some text (you can use a format string too)
            ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f    
            ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

            ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our windows open/close state
            ImGui::Checkbox("Another Window", &show_another_window);

            if (ImGui::Button("Button"))                            // Buttons return true when clicked (NB: most widgets return true when edited/activated)
                counter++;
            ImGui::SameLine();
            ImGui::Text("counter = %d", counter);

            ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
        }

        // 2. Show another simple window. In most cases you will use an explicit Begin/End pair to name your windows.
        if (show_another_window)
        {
            ImGui::Begin("Another Window", &show_another_window);
            ImGui::Text("Hello from another window!");
            if (ImGui::Button("Close Me"))
                show_another_window = false;
            ImGui::End();
        }

        // 3. Show the ImGui demo window. Most of the sample code is in ImGui::ShowDemoWindow(). Read its code to learn more about Dear ImGui!
        if (show_demo_window)
        {
            ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiCond_FirstUseEver); // Normally user code doesn't need/want to call this because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
            ImGui::ShowDemoWindow(&show_demo_window);
        }

        // Rendering
        gDXDeviceCtx->OMSetRenderTargets(1, &gMainRenderTargetView, NULL);
        gDXDeviceCtx->ClearRenderTargetView(gMainRenderTargetView, (float*)&clear_color);
		RECT wndRect;
		GetWindowRect(hwnd, &wndRect);
		D3D11_VIEWPORT viewport = { 0,0, wndRect.right - wndRect.left, wndRect.bottom - wndRect.top };
		gDXDeviceCtx->RSSetViewports(1, &viewport);

		MyRender();

        ImGui::Render();

        gSwapChain->Present(0, 0); // Present with vsync
        //g_pSwapChain->Present(0, 0); // Present without vsync
    }

	MyShutdown();

    ImGui_ImplDX11_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    UnregisterClass(_T("ImGui Example"), wc.hInstance);

    return 0;
}
