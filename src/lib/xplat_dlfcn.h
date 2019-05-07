#ifdef WINDOWS
    #include <windows.h>
    #define dlopen(a,b) LoadLibrary(a)
    #define dlsym(a,b) GetProcAddress(a,b)
    #define dlclose(a) /* CloseLibrary(a) */
    #define dlerror() "Error loading library"
    #define LIBHANDLE_T HINSTANCE__
#else
    #include <dlfcn.h>
    #define LIBHANDLE_T void
#endif

