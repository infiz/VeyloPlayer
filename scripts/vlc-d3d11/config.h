#define _GNU_SOURCE 1
#define N_(s) (s)
#define _(s) vlc_gettext(s)
#define _WIN32_WINNT 0x0601
#define WINVER 0x0601
#define VLC_WINSTORE_APP 0
#define HAVE_ID3D11VIDEODECODER 1
#define HAVE_D3D11_4_H 1
#define HAVE_DXGI1_6_H 1
#define HAVE_ATTRIBUTE_PACKED 1
#define HAVE_STDATOMIC_H 1
#define PACKAGE_NAME "VLC"
#define PACKAGE_VERSION "3.0.24"
#define PACKAGE_STRING "VLC 3.0.24"
#define MODULE_STRING "direct3d11"
#define MODULE_NAME direct3d11
#define MODULE_NAME_IS_direct3d11 1
#define __PLUGIN__ 1
#define NDEBUG 1
#include <winsock2.h>
struct pollfd;
int poll(struct pollfd *, unsigned, int);
