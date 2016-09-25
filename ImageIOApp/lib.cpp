
#pragma comment(lib, "Gdi32.lib")
#pragma comment(lib, "winmm.lib")

#ifdef _DEBUG
#pragma comment(lib, "zlibd.lib")
#pragma comment(lib, "LibPNGD.lib")
#pragma comment(lib, "libjpegd.lib")
#pragma comment(lib, "libtiffd.lib")
// #pragma comment(lib, "sqlited.lib")

#else
#pragma comment(lib, "zlib.lib")
#pragma comment(lib, "libpng.lib")
#pragma comment(lib, "libjpeg.lib")
#pragma comment(lib, "libtiff.lib")
// #pragma comment(lib, "sqlite.lib")

#endif

