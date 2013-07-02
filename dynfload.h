#pragma once

#ifndef DYNFLOAD_H_GUARD
#define DYNFLOAD_H_GUARD

#include <string>
#if defined(_WIN32)
#include <windows.h>
#elif defined(__unix__)
#include <dlfcn.h>
#else
#define DYNFLOAD_UNSUPPORTED
#endif

#ifndef DYNFLOAD_UNSUPPORTED
namespace DynFLoad
{

class DynamicModule
{
public:
    enum
    {
        LOCAL = 1,
        LAZY = 2
    };

    class Exception: public std::exception
    {
    public:
        Exception(){}
        Exception(const char* str):descr(str)
        {
#if defined(_WIN32)
            LPSTR lpMsgBuf = NULL;
            const DWORD res = ::FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                                               NULL,::GetLastError(),
                                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                                               reinterpret_cast<LPSTR>(&lpMsgBuf),0,NULL );
            if (res!=0)
            {
                descr.append(": ");
                descr.append(lpMsgBuf);
                ::LocalFree(lpMsgBuf);
            }
#else
            const char* description = dlerror();
            if (description)
            {
                descr.append(": ");
                descr.append(description);
            }
#endif
        }

        virtual ~Exception() throw(){}
        virtual const char* what() const throw()
        {
            return descr.c_str();
        }

    private:
        std::string descr;
    };

    DynamicModule(){}
    DynamicModule(const char* path, int flags = 0)
    {
        load(path, flags);
    }

    void load(const char * path, int flags = 0)
    {
#if defined(_WIN32)
        library = ::LoadLibraryExA(path,NULL,flags&LAZY?DONT_RESOLVE_DLL_REFERENCES:0);
#else
        library = dlopen(path,(flags&LOCAL==0?0:RTLD_GLOBAL)|(flags&LAZY==0?RTLD_NOW:RTLD_LAZY));
#endif
        if (!library) throw Exception("Loading library error");
        this->path = path;
    }

    void unload()
    {
        if (library)
        {
#if defined(_WIN32)
         bool err = ::FreeLibrary(library)==FALSE;
#else
         bool err = dlclose(library)!=0;
#endif
         if (err) throw Exception("Unloading library error");
         library = 0;
        }
    }

    bool isLoaded()
    {
        return library != 0;
    }

    template<class T>
    class Caster
    {
    public:
        Caster(void *ptr) { data.rawptr = ptr; }
        T get() { return data.ptr; }
    private:
        union CastUnion
        {
            void* rawptr;
            T ptr;
        } data;
    };

    template<class T>
    T getSymbol(const char* name)
    {
        if (!library) throw Exception("Library was not loaded");
#if defined(_WIN32)
        void* ptr = reinterpret_cast<void*>(::GetProcAddress(library,name));
#else
        void* ptr = dlsym(library,name);
#endif
        if (!ptr) throw Exception("Loading symbol error");
        return Caster<T>(ptr).get();
    }

    ~DynamicModule()
    {
        try
        {
            unload();
        }
        catch(...) {}
    }

private:
    std::string path;
#if defined(_WIN32)
    HMODULE library;
#else
    void * library;
#endif
};


}

#endif // not supported

#endif //DYNFLOAD_H_GUARD
