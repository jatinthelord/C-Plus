#pragma once
#if !defined(_WIN32)
#error "cspWindow currently supports Windows"
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commctrl.h>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>

namespace csp::ui {

struct color {
  std::uint8_t red = 0, green = 0, blue = 0;
  constexpr COLORREF native() const noexcept { return RGB(red, green, blue); }
};
struct size { int width = 0, height = 0; };
struct rect { int x = 0, y = 0, width = 0, height = 0; };
struct palette {
  color background{15, 23, 42};
  color surface{30, 41, 59};
  color foreground{241, 245, 249};
  color accent{14, 165, 233};
};

inline std::wstring wide(std::string_view text) {
  if (text.empty()) return {};
  int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                                  static_cast<int>(text.size()), nullptr, 0);
  if (!count) throw std::runtime_error("invalid UTF-8 text");
  std::wstring result(static_cast<std::size_t>(count), L'\0');
  MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                      static_cast<int>(text.size()), result.data(), count);
  return result;
}

inline std::string utf8(std::wstring_view text) {
  if (text.empty()) return {};
  int count = WideCharToMultiByte(CP_UTF8, 0, text.data(),
                                  static_cast<int>(text.size()), nullptr, 0,
                                  nullptr, nullptr);
  std::string result(static_cast<std::size_t>(count), '\0');
  WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()),
                      result.data(), count, nullptr, nullptr);
  return result;
}

class application {
public:
  application() {
    SetProcessDPIAware();
    INITCOMMONCONTROLSEX value{sizeof(value), ICC_STANDARD_CLASSES |
                                                 ICC_PROGRESS_CLASS};
    InitCommonControlsEx(&value);
  }
  int run() const {
    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
      TranslateMessage(&message);
      DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
  }
  static void quit(int code = 0) noexcept { PostQuitMessage(code); }
};

class widget {
  friend class window;
protected:
  HWND handle_ = nullptr;
  explicit widget(HWND handle) : handle_(handle) {}
public:
  widget() = default;
  HWND native_handle() const noexcept { return handle_; }
  explicit operator bool() const noexcept { return handle_; }
  void set_text(std::string_view value) const {
    auto converted = wide(value);
    SetWindowTextW(handle_, converted.c_str());
  }
  std::string text() const {
    int length = GetWindowTextLengthW(handle_);
    std::wstring value(static_cast<std::size_t>(length + 1), L'\0');
    GetWindowTextW(handle_, value.data(), length + 1);
    value.resize(static_cast<std::size_t>(length));
    return utf8(value);
  }
  void bounds(rect value) const noexcept {
    MoveWindow(handle_, value.x, value.y, value.width, value.height, TRUE);
  }
  void enabled(bool value) const noexcept { EnableWindow(handle_, value); }
  void visible(bool value) const noexcept { ShowWindow(handle_, value ? SW_SHOW : SW_HIDE); }
  void focus() const noexcept { SetFocus(handle_); }
};

class check_box : public widget {
  friend class window;
  explicit check_box(HWND handle) : widget(handle) {}
public:
  check_box() = default;
  bool checked() const noexcept { return SendMessageW(handle_, BM_GETCHECK, 0, 0) == BST_CHECKED; }
  void checked(bool value) const noexcept { SendMessageW(handle_, BM_SETCHECK, value ? BST_CHECKED : BST_UNCHECKED, 0); }
};

class progress_bar : public widget {
  friend class window;
  explicit progress_bar(HWND handle) : widget(handle) {}
public:
  progress_bar() = default;
  void range(int low, int high) const noexcept { SendMessageW(handle_, PBM_SETRANGE32, low, high); }
  void value(int current) const noexcept { SendMessageW(handle_, PBM_SETPOS, current, 0); }
};

struct window_config {
  std::string title = "C+ application";
  int width = 960, height = 640;
  int minimum_width = 480, minimum_height = 320;
  bool resizable = true, maximize = false;
  palette colors{};
};

class window {
  HWND handle_ = nullptr;
  HBRUSH background_ = nullptr;
  HFONT font_ = nullptr;
  window_config config_;
  int next_id_ = 100;
  std::unordered_map<int, std::function<void()>> commands_;
  std::function<void(size)> resize_;
  std::function<bool()> close_;
  std::function<void(int)> key_;

  static const wchar_t *class_name() noexcept { return L"CSPWindowClass"; }
  static void register_class() {
    static bool registered = [] {
      WNDCLASSEXW value{};
      value.cbSize = sizeof(value);
      value.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
      value.lpfnWndProc = &window::procedure;
      value.hInstance = GetModuleHandleW(nullptr);
      value.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
      value.hIcon = LoadIconW(nullptr, MAKEINTRESOURCEW(32512));
      value.lpszClassName = class_name();
      if (!RegisterClassExW(&value) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
        throw std::runtime_error("failed to register C+ window class");
      return true;
    }();
    (void)registered;
  }
  static LRESULT CALLBACK procedure(HWND handle, UINT message, WPARAM wp, LPARAM lp) {
    window *self = reinterpret_cast<window *>(GetWindowLongPtrW(handle, GWLP_USERDATA));
    if (message == WM_NCCREATE) {
      self = static_cast<window *>(reinterpret_cast<CREATESTRUCTW *>(lp)->lpCreateParams);
      self->handle_ = handle;
      SetWindowLongPtrW(handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
    }
    return self ? self->dispatch(message, wp, lp) : DefWindowProcW(handle, message, wp, lp);
  }
  LRESULT dispatch(UINT message, WPARAM wp, LPARAM lp) {
    switch (message) {
    case WM_COMMAND: {
      auto found = commands_.find(LOWORD(wp));
      if (found != commands_.end() && HIWORD(wp) == BN_CLICKED) { found->second(); return 0; }
      break;
    }
    case WM_SIZE: if (resize_) resize_({LOWORD(lp), HIWORD(lp)}); return 0;
    case WM_GETMINMAXINFO: {
      auto *limits = reinterpret_cast<MINMAXINFO *>(lp);
      limits->ptMinTrackSize = {config_.minimum_width, config_.minimum_height};
      return 0;
    }
    case WM_KEYDOWN: if (key_) key_(static_cast<int>(wp)); return 0;
    case WM_ERASEBKGND: {
      RECT area{}; GetClientRect(handle_, &area);
      FillRect(reinterpret_cast<HDC>(wp), &area, background_); return 1;
    }
    case WM_CTLCOLORSTATIC:
    case WM_CTLCOLOREDIT: {
      HDC dc = reinterpret_cast<HDC>(wp);
      SetTextColor(dc, config_.colors.foreground.native());
      SetBkColor(dc, config_.colors.background.native());
      return reinterpret_cast<LRESULT>(background_);
    }
    case WM_CLOSE: if (!close_ || close_()) DestroyWindow(handle_); return 0;
    case WM_DESTROY: PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(handle_, message, wp, lp);
  }
  HWND control(const wchar_t *kind, std::string_view text, rect area, DWORD style, int id) {
    auto label = wide(text);
    HWND result = CreateWindowExW(0, kind, label.c_str(), WS_CHILD | WS_VISIBLE | style,
        area.x, area.y, area.width, area.height, handle_,
        reinterpret_cast<HMENU>(static_cast<std::intptr_t>(id)), GetModuleHandleW(nullptr), nullptr);
    if (!result) throw std::runtime_error("failed to create GUI control");
    SendMessageW(result, WM_SETFONT, reinterpret_cast<WPARAM>(font_), TRUE);
    return result;
  }
public:
  explicit window(window_config config = {}) : config_(std::move(config)) {
    register_class();
    background_ = CreateSolidBrush(config_.colors.background.native());
    font_ = CreateFontW(-18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Segoe UI");
    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    if (config_.resizable) style |= WS_THICKFRAME | WS_MAXIMIZEBOX;
    auto title = wide(config_.title);
    handle_ = CreateWindowExW(0, class_name(), title.c_str(), style, CW_USEDEFAULT,
        CW_USEDEFAULT, config_.width, config_.height, nullptr, nullptr,
        GetModuleHandleW(nullptr), this);
    if (!handle_) throw std::runtime_error("failed to create C+ window");
  }
  window(const window &) = delete;
  window &operator=(const window &) = delete;
  ~window() {
    if (handle_ && IsWindow(handle_)) DestroyWindow(handle_);
    if (font_) DeleteObject(font_);
    if (background_) DeleteObject(background_);
  }
  void show() const noexcept { ShowWindow(handle_, config_.maximize ? SW_MAXIMIZE : SW_SHOW); UpdateWindow(handle_); }
  void title(std::string_view value) const { auto text = wide(value); SetWindowTextW(handle_, text.c_str()); }
  size client_size() const noexcept { RECT r{}; GetClientRect(handle_, &r); return {r.right - r.left, r.bottom - r.top}; }
  void on_resize(std::function<void(size)> value) { resize_ = std::move(value); }
  void on_close(std::function<bool()> value) { close_ = std::move(value); }
  void on_key(std::function<void(int)> value) { key_ = std::move(value); }
  widget label(std::string_view text, rect area) { return widget(control(L"STATIC", text, area, SS_LEFT | SS_CENTERIMAGE, 0)); }
  widget text_box(std::string_view text, rect area, bool multiline = false) {
    DWORD style = multiline ? WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN | WS_VSCROLL
                            : WS_BORDER | ES_AUTOHSCROLL;
    return widget(control(L"EDIT", text, area, style, next_id_++));
  }
  widget button(std::string_view text, rect area, std::function<void()> handler) {
    int id = next_id_++; commands_[id] = std::move(handler);
    return widget(control(L"BUTTON", text, area, BS_PUSHBUTTON, id));
  }
  check_box checkbox(std::string_view text, rect area, std::function<void()> handler = {}) {
    int id = next_id_++; if (handler) commands_[id] = std::move(handler);
    return check_box(control(L"BUTTON", text, area, BS_AUTOCHECKBOX, id));
  }
  progress_bar progress(rect area, int maximum = 100) {
    progress_bar result(control(PROGRESS_CLASSW, "", area, PBS_SMOOTH, next_id_++));
    result.range(0, maximum); return result;
  }
  void message(std::string_view text, std::string_view caption = "C+") const {
    auto body = wide(text), heading = wide(caption);
    MessageBoxW(handle_, body.c_str(), heading.c_str(), MB_OK | MB_ICONINFORMATION);
  }
};

} // namespace csp::ui
