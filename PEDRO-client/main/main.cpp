#ifndef UNICODE
#define UNICODE
#endif

#include <Winsock2.h>
#include <windows.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

#include <d3d11.h>

#include "network.cpp"

enum window_state {
    window_state_none = 0,
    window_state_connect = 1 << 0,
    window_state_default = 1 << 1,
    window_state_custom_1 = 1 << 2
};

static ID3D11Device *d3d_device = nullptr;
static ID3D11DeviceContext *d3d_device_context = nullptr;
static IDXGISwapChain *swap_chain = nullptr;
static bool swap_chain_occluded = false;
static UINT resize_width = 0;
static UINT resize_height = 0;
static ID3D11RenderTargetView *main_rtv = nullptr;

bool create_device_d3d(HWND hwnd);
void cleanup_device_d3d();
void create_render_target();
void cleanup_render_target();

LRESULT CALLBACK WindowProc(HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param);

int WINAPI wWinMain(HINSTANCE h_instance, HINSTANCE h_prev_instance, PWSTR p_cmd_line, int n_cmd_show) {
    HRESULT hr;
    const wchar_t CLASS_NAME[] = L"GirlsFrontline 2";

    WNDCLASS window_class = {};
    window_class.lpfnWndProc = WindowProc;
    window_class.hInstance = h_instance;
    window_class.lpszClassName = CLASS_NAME;

    RegisterClass(&window_class);

    HWND hwnd = CreateWindowEx(0, CLASS_NAME, L"GirlsFrontline 2", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                                CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, h_instance, NULL);

    if(!create_device_d3d(hwnd)) {
        cleanup_device_d3d();
        return 1;
    }

    ShowWindow(hwnd, n_cmd_show);
    UpdateWindow(hwnd);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
 
    ImGui::StyleColorsDark();
   
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(d3d_device, d3d_device_context);


    // Initialize, create and connect socket to the server
    WSADATA wsa_data;
    hr = (HRESULT)WSAStartup(MAKEWORD(2, 2), &wsa_data);
    client_socket client = {};
    /*
    if(SUCCEEDED(hr)) {
        client = create_socket();
        connect_socket(&client);
    }
    
    tcp_message test = {};
    test.buffer = (char *)malloc(512);
    test.buffer_length = 512;
    char **data_points = (char **)malloc(3 * strlen("GPIO4"));
    data_points[0] = "GPIO4";
    data_points[1] = "GPIO4";
    data_points[2] = "GPIO4";
    send_data_request(client.connect_socket, &test, data_points, 3);  
    */

    float clear_color[4] = {0, 0, 0, 0};
    char ip_address[32] = {};
    char port[8] = {};

    UINT window_state = 0;
    
    bool done = false;
    while(!done) {
        MSG msg;
        while(PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if(msg.message == WM_QUIT) {
                done = true;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if(done) {
            break;
        }

        if(swap_chain_occluded && swap_chain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED) {
            Sleep(10);
            continue;
        }
        swap_chain_occluded = false;

        if(resize_width != 0 && resize_height != 0) {
            cleanup_render_target();
            swap_chain->ResizeBuffers(0, resize_width, resize_height, DXGI_FORMAT_UNKNOWN, 0);
            resize_width = 0;
            resize_height = 0;
            create_render_target();
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        switch(window_state) {
            case window_state_connect: {
            } break;

            case window_state_default: {
            } break;

            case window_state_custom_1: {
            } break;

            default: {
            } break;
        }

        ImGui::ShowDemoWindow();

        ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove 
                               | ImGuiWindowFlags_NoSavedSettings;
        ImGuiViewport *viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::Begin("Connect Window", NULL);
        ImGui::Text("Server address:");
        ImGui::InputTextWithHint("ip_address_input", "IPv4: 192.168.0.1", ip_address, sizeof(ip_address), 
                                 ImGuiInputTextFlags_CharsDecimal);
        ImGui::SameLine();
        ImGui::InputTextWithHint("port_input", "Port: 7777", port, sizeof(port), ImGuiInputTextFlags_CharsDecimal);

        ImGui::Text("%s : %s", ip_address, port);

        ImGui::End();

        // Rendering
        ImGui::Render();
        d3d_device_context->OMSetRenderTargets(1, &main_rtv, nullptr);
        d3d_device_context->ClearRenderTargetView(main_rtv, clear_color);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        HRESULT hr = swap_chain->Present(1, 0);
        swap_chain_occluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    cleanup_device_d3d();

    WSACleanup();
    network_cleanup();

    return 0;
}


extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WindowProc(HWND hwnd, UINT u_msg, WPARAM w_param, LPARAM l_param) {
    if(ImGui_ImplWin32_WndProcHandler(hwnd, u_msg, w_param, l_param)) {
        return true;
    }

    switch(u_msg) {
        case WM_SIZE: {
            if(w_param == SIZE_MINIMIZED) {
                return 0;
            }

            resize_width = LOWORD(l_param);
            resize_height = HIWORD(l_param);

            return 0;
        } break;

        case WM_DESTROY: { 
            PostQuitMessage(0);
        } break;

        default: {
            DefWindowProc(hwnd, u_msg, w_param, l_param);
        } break;
    }
}

bool create_device_d3d(HWND hwnd) {
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
    sd.OutputWindow = hwnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT create_device_flags = 0;
    create_device_flags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL feature_level;
    const D3D_FEATURE_LEVEL feature_level_arr[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0
    };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, create_device_flags,
                                                feature_level_arr, 2, D3D11_SDK_VERSION, &sd, &swap_chain, &d3d_device,
                                                &feature_level, &d3d_device_context);
    if(res == DXGI_ERROR_UNSUPPORTED) {
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, create_device_flags, 
                                            feature_level_arr, 2, D3D11_SDK_VERSION, &sd, &swap_chain, &d3d_device,
                                            &feature_level, &d3d_device_context);
    }
    if(res != S_OK) {
        return false;
    }

    create_render_target();
    return true;
}

void cleanup_device_d3d() {
    cleanup_render_target();
    if(swap_chain) {
        swap_chain->Release();
        swap_chain = nullptr;
    }
    if(d3d_device_context) {
        d3d_device_context->Release();
        d3d_device_context = nullptr;
    }
    if(d3d_device) {
        d3d_device->Release();
        d3d_device = nullptr;
    }
}

void create_render_target() {
    ID3D11Texture2D *back_buffer;
    swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
    d3d_device->CreateRenderTargetView((ID3D11Resource *)back_buffer, nullptr, &main_rtv);
    back_buffer->Release();
}

void cleanup_render_target() {
    if(main_rtv) {
        main_rtv->Release();
        main_rtv = nullptr;
    }
}
