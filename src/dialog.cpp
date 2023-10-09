#include <Windows.h>

void get_file_path(char** path)
{
	char file[1024] = {};

	OPENFILENAMEA ofn = {};
	ofn.lStructSize = sizeof(ofn);
	ofn.Flags = OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;
	ofn.lpstrFile = file;
	ofn.nMaxFile = sizeof(file);

	if (GetOpenFileNameA(&ofn))
	{
		size_t size = strlen(file);
		*path = (char*)malloc(size + 1);
		memcpy(*path, file, size + 1);
	}
}


struct dialog_data
{
	char** text;
	const char* message;
	HWND edit;
};

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	dialog_data* data;

	switch (msg) {
		case WM_CREATE:
			SetWindowLongPtrA(hwnd, GWLP_USERDATA, (LONG_PTR)((LPCREATESTRUCT)lParam)->lpCreateParams);
			data = (dialog_data*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

			CreateWindowA("static", data->message, WS_VISIBLE | WS_CHILD,
						  10, 10, 200, 25, hwnd, (HMENU)103, NULL, NULL);
			data->edit = CreateWindowA("edit", NULL, WS_CHILD | WS_VISIBLE,
									   10, 40, 200, 20, hwnd, (HMENU)102, NULL, NULL);
			CreateWindowA("button", "Set", WS_VISIBLE | WS_CHILD,
						  10, 75, 60, 25, hwnd, (HMENU)103, NULL, NULL);
			break;
		case WM_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED) {
				data = (dialog_data*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);
				int len = GetWindowTextLengthA(data->edit) + 1;
				if (len > 1) {
					*data->text = (char*)malloc(len + 1);
					GetWindowTextA(data->edit, *data->text, len);
				}
				DestroyWindow(hwnd);
			}
			break;
		case WM_DESTROY:
			PostQuitMessage(0);
			break;
	}
	return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void get_text(char** text, const char* title, const char* message)
{
	HINSTANCE hInstance = GetModuleHandleA(NULL);
	MSG msg;
	WNDCLASSA wc = { 0 };
	wc.lpszClassName = title;
	wc.hInstance = hInstance;
	wc.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
	wc.lpfnWndProc = WndProc;
	wc.hCursor = LoadCursor(0, IDC_ARROW);
	RegisterClassA(&wc);

	int win_pos_x = GetSystemMetrics(SM_CXSCREEN) / 2;
	int win_pos_y = GetSystemMetrics(SM_CYSCREEN) / 2;
	int win_size_x = 240;
	int win_size_y = 150;

	dialog_data data = { text, message };

	CreateWindowExA(0, wc.lpszClassName, title,
					WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_DLGFRAME | WS_VISIBLE,
					win_pos_x - win_size_x / 2, win_pos_y - win_size_y / 2, win_size_x, win_size_y,
					0, 0, hInstance, &data);

	while (GetMessageA(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessageA(&msg);
	}
}