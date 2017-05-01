//****************************************************************************
//	
//	Copyright 2017 by WinSystems Inc.
//
//****************************************************************************
//
//	Name	 : uio48DLL.h
//
//	Project	 : UIO48 Windows DLL
//
//	Author	 : Paul DeMetrotion
//
//****************************************************************************
//
//	  Date		  Rev	                Description
//	--------    -------	   ---------------------------------------------
//	04/14/17	  1.0		Original Release of DLL	
//
//****************************************************************************

#ifndef _UIO48_DLL_H_
#define _UIO48_DLL_H_

#if defined DLL_EXPORT
#define DECLDIR __declspec(dllexport)
#else
#define DECLDIR __declspec(dllimport)
#endif

extern "C"
{
    DECLDIR int InitializeSession(); //
    DECLDIR int ResetDevice(); //
    DECLDIR int SetIoMask(unsigned int *portState); //
    DECLDIR int GetIoMask(unsigned int *portState); //
    DECLDIR int ReadAllPorts(unsigned int *readValueArray);
    DECLDIR int ReadPort(int port, unsigned int *readValue); //
    DECLDIR int ReadBit(int bit, unsigned int *bitValue); //
    DECLDIR int SetBit(int bit); //
    DECLDIR int ClearBit(int bit); //
    DECLDIR int WritePort(int port, unsigned int writeValue);
    DECLDIR int WriteBit(int bit, unsigned int bitValue);
    DECLDIR int EnableInterrupt(int bit, int edge);
    DECLDIR int DisableInterrupt(int bit);
    DECLDIR int GetInterrupt(unsigned int *irqArray);
    DECLDIR int WaitForInterrupt(unsigned int *irqArray);
    //DECLDIR int ClearWaitForInterrupt();
    DECLDIR int LockPort(int port);
    DECLDIR int UnlockPort(int port);
    DECLDIR int CloseSession(); //
}

typedef enum {
    SUCCESS = 0,
    DRIVER_ERROR,
    ACCESS_ERROR,
    INVALID_HANDLE,
    INVALID_PARAMETER
} ErrorCodes;

#endif