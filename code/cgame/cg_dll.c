#ifdef Q3_VM
#error only for native libraries
#endif

#ifndef _WIN32

//  #error this is only for Windows Dll

#include <stdio.h>

int DllMain (void *h, int r, void *res)
{
    return 1;
}

#if 0
void __attribute__ ((constructor)) Dll_Init (void);
void __attribute__ ((destructor)) Dll_Fini (void);

void Dll_Init (void)
{
    printf("cgame dll init\n");
}

void Dll_Fini (void)
{
    printf("cgame dll fini\n");
}
#endif

#else

#include <windows.h>
#include <stdio.h>

//FIXME
extern void CG_ShutdownLocalEnts (int destructor);

/* Callback for our DLL so we can initialize pthread */
BOOL WINAPI DllMain (HANDLE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        // don't really need this, just make sure CG_SHUTDOWN is called in
        // the right places
        //CG_ShutdownLocalEnts(1);
        //fprintf(stderr, "****  process detach  ****\n");
        break;
    }

    return TRUE;
}

#endif  // else ifndef _WIN32
