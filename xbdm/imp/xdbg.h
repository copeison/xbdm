/*++

Copyright (c) 1989-2000  Microsoft Corporation

Module Name:

    xdbg.h

Abstract:

    Internal debug header for XBox OS and library components
    
--*/


#ifndef _XDBG_H
#define _XDBG_H

#ifdef _DEBUG
#define DBG 1
#endif

#ifndef ASSERT
#if DBG
    #define ASSERT( exp ) \
        if (!(exp)) \
            RtlAssert( #exp, __FILE__, __LINE__, NULL )
#else  // DBG
    #define ASSERT( exp )
#endif // DBG
#endif // ! ASSERT

#ifndef ASSERTMSG
#if DBG
    #define ASSERTMSG( msg, exp ) \
        if (!(exp)) \
            RtlAssert( #exp, __FILE__, __LINE__, msg )
#else  // DBG
    #define ASSERTMSG( msg, exp )
#endif // DBG
#endif // ! ASSERTMSG

#if 0
#ifndef RIP
#if DBG
    #define RIP(msg)                         \
        do{ LPCSTR _ = (msg);                \
            _asm { mov eax, BREAKPOINT_RIP } \
            _asm { mov ecx, _ }              \
            _asm { int 2dh }                 \
            _asm { int 3 }                   \
        } while(0)

#else  // DBG
    #define RIP(msg) {}
#endif // DBG
#endif  // ! RIP

#ifndef RIP_ON_NOT_TRUE
#if DBG
    #define RIP_ON_NOT_TRUE(ApiName, Expression)  \
        do{                                       \
            if(!(Expression))                     \
            {                                     \
                UCHAR _[512];                     \ 
                _snprintf(_, sizeof(_), "%s: parameter check failed. (%s) was not true.\n", ApiName, #Expression);\
                _[511]='\0';                      \
                _asm { mov eax, BREAKPOINT_RIP }  \
                _asm { mov ecx, _ }               \
                _asm { int 2dh }                  \
                _asm { int 3 }                    \
            }                                     \
        }while(0)
#else  // DBG
#define RIP_ON_NOT_TRUE(ApiName, Expression)
#endif // DBG
#endif  // !RIP_ON_NOT_TRUE

#ifndef RIP_ON_NOT_TRUE_WITH_MESSAGE
#if DBG
    #define RIP_ON_NOT_TRUE_WITH_MESSAGE(Expression, Message)  \
    do{                                                        \
        if(!(Expression))                                      \
        {                                                      \
            RIP(Message);                                      \
        }                                                      \
    }while(0)                                   
#else  // DBG
    #define RIP_ON_NOT_TRUE_WITH_MESSAGE(Expression, Message)
#endif // DBG
#endif  // !RIP_ON_NOT_TRUE_WITH_MESSAGE
#endif // 0

#define XDBG_ERROR   1
#define XDBG_WARNING 2
#define XDBG_TRACE   3
#define XDBG_ENTRY   4
#define XDBG_EXIT    5

#ifndef XDBG_COMPILE_LEVEL
#define XDBG_COMPILE_LEVEL XDBG_WARNING
#endif // ! XDBG_COMPILE_LEVEL

#ifndef XDBGPRINT
#if DBG
#define XDBGPRINT RtlDebugPrint
#else  // DBG
#define XDBGPRINT 1 ? (void)0 : (void)
#endif // DBG
#endif // ! XDBGPRINT

#ifndef XDBGERR
#if DBG && (XDBG_COMPILE_LEVEL >= XDBG_ERROR)
#define XDBGERR RtlDebugError
#else  // DBG
#define XDBGERR 1 ? (void)0 : (void)
#endif // DBG
#endif // ! XDBGERR

#ifndef XDBGWRN
#if DBG && (XDBG_COMPILE_LEVEL >= XDBG_WARNING)
#define XDBGWRN RtlDebugWarning
#else  // DBG
#define XDBGWRN 1 ? (void)0 : (void)
#endif // DBG
#endif // ! XDBGWRN

#ifndef XDBGTRC
#if DBG && (XDBG_COMPILE_LEVEL >= XDBG_TRACE)
#define XDBGTRC RtlDebugTrace
#else  // DBG
#define XDBGTRC 1 ? (void)0 : (void)
#endif // DBG
#endif // ! XDBGTRC

#ifndef XDBGENT
#if DBG && (XDBG_COMPILE_LEVEL >= XDBG_ENTRY)
#define XDBGENT RtlDebugEntry
#else  // DBG
#define XDBGENT 1 ? (void)0 : (void)
#endif // DBG
#endif // ! XDBGENT

#ifndef XDBGEXT
#if DBG && (XDBG_COMPILE_LEVEL >= XDBG_EXIT)
#define XDBGEXT RtlDebugExit
#else  // DBG
#define XDBGEXT 1 ? (void)0 : (void)
#endif // DBG
#endif // ! XDBGEXT

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _NTSYSTEM_
#define XKERNELAPI
#else  // _NTSYSTEM_
#define XKERNELAPI     DECLSPEC_IMPORT
#endif // _NTSYSTEM_

#if (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED)
#define XDBGAPI      __stdcall
#define XDBGAPIV     __cdecl
#else
#define XDBGAPI
#define XDBGAPIV
#endif

XKERNELAPI
VOID
XDBGAPI
RtlAssert(
    LPCSTR FailedAssertion,
    LPCSTR FileName,
    ULONG LineNumber,
    PCHAR Message
    );

ULONG
_cdecl
DbgPrint(
    PCH Format,
    ...
    );

VOID
RtlDebugError(const CHAR* Module, 
              const CHAR* Format, 
              ...
    );

VOID
_cdecl
RtlDebugWarning(const CHAR* Module, 
                const CHAR* Format, 
                ...
    );

VOID
_cdecl
RtlDebugTrace(const CHAR* Module, 
              const CHAR* Format, 
              ...
    );


VOID
_cdecl
RtlDebugEntry(const CHAR* Module, 
              const CHAR* Format, 
              ...
    );


VOID
_cdecl
RtlDebugExit(const CHAR* Module, 
             const CHAR* Format, 
             ...
    );


VOID
_cdecl
RtlDebugPrint(ULONG Level, 
              const CHAR* Module, 
              const CHAR* Format, 
              ...
    );

int
_cdecl
RtlSprintf( CHAR *Buffer,
			const CHAR *Format,
			...
	);
int
_cdecl
RtlSnprintf(CHAR *Buffer,
			int SizeInBytes,
			const CHAR *Format,
			...
	);

int
_cdecl
RtlVsnprintf(CHAR *Buffer,
				int SizeInBytes,
				const CHAR *Format,
				va_list va
	);

int
_cdecl
RtlVsprintf(CHAR *Buffer,
			const CHAR *Format,
			va_list va
	);


#ifdef __cplusplus
}   // extern "C"
#endif

#endif // _XDBG_H
