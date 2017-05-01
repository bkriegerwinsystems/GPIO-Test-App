//****************************************************************************
//	
//	Copyright 2017 by WinSystems Inc.
//
//****************************************************************************
//
//	Name	 : uio48DLL.cpp
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

#include "stdafx.h"
#include <iostream>
#include <winioctl.h>
#define DLL_EXPORT
#include "uio48DLL.h"
#include "..\uio48\Ioctl.h"
#include "..\uio48\Config.h"

static HANDLE uio48Handle;
static MASK_PARAMS maskp;
static BYTE_PARAMS bytep;
static BIT_PARAMS bitp;
static PORT_PARAMS portp;
static IRQ_PARAMS irqp;
static DWORD junk;
static unsigned int port_locked = 0;

// Defines the exported functions for the DLL application
extern "C"
{
    DECLDIR int InitializeSession()
    {
        int status = SUCCESS;

        uio48Handle = CreateFile(DeviceName,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL);

        if (uio48Handle == INVALID_HANDLE_VALUE)
            status = INVALID_HANDLE;

        return status;
    }

    DECLDIR int ResetDevice()
    {
        int i, status = SUCCESS;

        // set all pins as outputs
        maskp.devNum = 1;
        if (!DeviceIoControl(uio48Handle,
            IOCTL_CLR_IO_MASK,
            &maskp,
            sizeof(maskp),
            NULL,
            0,
            &junk,
            NULL))
        {
            return status = DRIVER_ERROR;
        }

        // clear all ports
        bytep.devNum = 1;
        bytep.byteValue = 0;
        for (i = 0; i < MAX_IO_PORTS; i++) {
            bytep.portNum = i;

            if (!DeviceIoControl(uio48Handle,
                IOCTL_WRITE_PORT,
                &bytep,
                sizeof(bytep),
                NULL,
                0,
                &junk,
                NULL))
            {
                return status = DRIVER_ERROR;
            }
        }

        // disable all interrupts
        bitp.devNum = 1;
        for (i = 1; i <= MAX_INT_POINTS; i++) {
            bitp.bitNum = i;

            if (!DeviceIoControl(uio48Handle,
                IOCTL_DISAB_INT,
                &bitp,
                sizeof(bitp),
                NULL,
                0,
                &junk,
                NULL))
            {
                return status = DRIVER_ERROR;
            }
        }

        // unlock all ports
        bytep.devNum = 1;
        for (i = 0; i < MAX_IO_PORTS; i++) {
            bytep.portNum = i;

            if (!DeviceIoControl(uio48Handle,
                IOCTL_UNLOCK_PORT,
                &bytep,
                sizeof(bytep),
                NULL,
                0,
                &junk,
                NULL))
            {
                return status = DRIVER_ERROR;
            }
        }

        // update locked status
        port_locked = 0;

        return status;
    }

    DECLDIR int SetIoMask(unsigned int *portState)
    {
        int status = SUCCESS;

        // check parameters
        if (portState == NULL)
        {
            return status = INVALID_PARAMETER;
        }

        maskp.devNum = 1;
        maskp.Mask1 = (BYTE) *portState;
        maskp.Mask2 = (BYTE) *(portState + 1);
    #if MAX_IO_PORTS > 2
        maskp.Mask3 = (BYTE) *(portState + 2);
    #endif
    #if MAX_IO_PORTS > 3
        maskp.Mask4 = (BYTE) *(portState + 3);
        maskp.Mask5 = (BYTE) *(portState + 4);
        maskp.Mask6 = (BYTE) *(portState + 5);
    #endif

        // set masks
        if (!DeviceIoControl(uio48Handle,
            IOCTL_SET_IO_MASK,
            &maskp,
            sizeof(maskp),
            NULL,
            0,
            &junk,
            NULL))
        {
            status = DRIVER_ERROR;
        }

        return status;
    }

    DECLDIR int GetIoMask(unsigned int *portState)
    {
        int status = SUCCESS;

        // check parameters
        if (portState == NULL)
        {
            return status = INVALID_PARAMETER;
        }

        // retrieve masks
        maskp.devNum = 1;
        if (!DeviceIoControl(uio48Handle,
            IOCTL_GET_IO_MASK,
            &maskp,
            sizeof(maskp),
            &maskp,
            sizeof(maskp),
            &junk,
            NULL))
        {
            status = DRIVER_ERROR;
        }

        *portState = (unsigned int) maskp.Mask1;
        *(portState + 1) = (unsigned int) maskp.Mask2;
    #if MAX_IO_PORTS > 2
        *(portState + 2) = (unsigned int) maskp.Mask3;
    #endif
    #if MAX_IO_PORTS > 3
        *(portState + 3) = (unsigned int) maskp.Mask4;
        *(portState + 4) = (unsigned int) maskp.Mask5;
        *(portState + 5) = (unsigned int) maskp.Mask6;
    #endif

        return status;
    }

    DECLDIR int ReadAllPorts(unsigned int *readValueArray)
    {
        int status = SUCCESS;

        // check parameters
        if (readValueArray == NULL)
        {
            return status = INVALID_PARAMETER;
        }

        // read all ports
        portp.devNum = 1;
        if (!DeviceIoControl(uio48Handle,
            IOCTL_READ_ALL_PORTS,
            &portp,
            sizeof(portp),
            &portp,
            sizeof(portp),
            &junk,
            NULL))
        {
            return status = DRIVER_ERROR;
        }

        *(readValueArray) = (unsigned int) portp.Byte1 & 0xff;
        *(readValueArray + 1) = (unsigned int) portp.Byte2 & 0xff;
    #if MAX_IO_PORTS > 2
        *(readValueArray + 2) = (unsigned int) portp.Byte3 & 0xff;
    #endif
    #if MAX_IO_PORTS > 3
        *(readValueArray + 3) = (unsigned int) portp.Byte4 & 0xff;
        *(readValueArray + 4) = (unsigned int) portp.Byte5 & 0xff;
        *(readValueArray + 5) = (unsigned int) portp.Byte6 & 0xff;
    #endif

        return status;
    }

    DECLDIR int ReadPort(int port, unsigned int *readValue)
    {
        int status = SUCCESS;

        // check parameters
        if ((port < 0 || port > MAX_IO_PORTS - 1) || (readValue == NULL))
        {
            return status = INVALID_PARAMETER;
        }

        // read port
        bytep.devNum = 1;
        bytep.portNum = port;
        if (!DeviceIoControl(uio48Handle,
            IOCTL_READ_PORT,
            &bytep,
            sizeof(bytep),
            &bytep,
            sizeof(bytep),
            &junk,
            NULL))
        {
            return status = DRIVER_ERROR;
        }

        *readValue = (unsigned int)bytep.byteValue & 0xff;

        return status;
    }

    DECLDIR int ReadBit(int bit, unsigned int *bitValue)
    {
        int status = SUCCESS;

        // check parameters
        if ((bit < 0 || bit > MAX_IO_POINTS - 1) || (bitValue == NULL))
        {
            return status = INVALID_PARAMETER;
        }

        // read bit
        bitp.devNum = 1;
        bitp.bitNum = ++bit;
        if (!DeviceIoControl(uio48Handle,
            IOCTL_READ_BIT,
            &bitp,
            sizeof(bitp),
            &bitp,
            sizeof(bitp),
            &junk,
            NULL))
        {
            return status = DRIVER_ERROR;
        }

        *bitValue = (unsigned int) bitp.bitValue & 0x01;

        return status;
    }

    DECLDIR int SetBit(int bit)
    {
        int status = SUCCESS;

        // check parameters
        if (bit < 0 || bit > MAX_IO_POINTS - 1)
        {
            return status = INVALID_PARAMETER;
        }

        // check for port lock
        if (port_locked & 1 << (bit % MAX_BITS_BYTE))
        {
            return status = ACCESS_ERROR;
        }

        // set bit
        bitp.devNum = 1;
        bitp.bitNum = ++bit;
        if (!DeviceIoControl(uio48Handle,
            IOCTL_SET_BIT,
            &bitp,
            sizeof(bitp),
            NULL,
            0,
            &junk,
            NULL))
        {
            if (GetLastError() == STATUS_ACCESS_VIOLATION)
                status = ACCESS_ERROR;
            else
                status = DRIVER_ERROR;
        }
        
       return status;
    }

    DECLDIR int ClearBit(int bit)
    {
        int status = SUCCESS;

        // check parameters
        if (bit < 0 || bit > MAX_IO_POINTS - 1)
        {
            return status = INVALID_PARAMETER;
        }

        // check for port lock
        if (port_locked & 1 << (bit % MAX_BITS_BYTE))
        {
            return status = ACCESS_ERROR;
        }

        // clear bit
        bitp.devNum = 1;
        bitp.bitNum = ++bit;
        if (!DeviceIoControl(uio48Handle,
            IOCTL_CLR_BIT,
            &bitp,
            sizeof(bitp),
            NULL,
            0,
            &junk,
            NULL))
        {
            if (GetLastError() == STATUS_ACCESS_VIOLATION)
                status = ACCESS_ERROR;
            else
                status = DRIVER_ERROR;
        }

        return status;
    }

    DECLDIR int WritePort(int port, unsigned int writeValue)
    {
        int status = SUCCESS;

        // check parameters
        if ((port < 0 || port > MAX_IO_PORTS - 1) || (writeValue > 255))
        {
            return status = INVALID_PARAMETER;
        }

        // check for port lock
        if (port_locked & 1 << port)
        {
            return status = ACCESS_ERROR;
        }

        // write port
        bytep.devNum = 1;
        bytep.portNum = port;
        bytep.byteValue = (BYTE) writeValue;
        if (!DeviceIoControl(uio48Handle,
            IOCTL_WRITE_PORT,
            &bytep,
            sizeof(bytep),
            &bytep,
            sizeof(bytep),
            &junk,
            NULL))
        {
            if (GetLastError() == STATUS_ACCESS_VIOLATION)
                status = ACCESS_ERROR;
            else
                status = DRIVER_ERROR;
        }

        return status;
    }

    DECLDIR int WriteBit(int bit, unsigned int bitValue)
    {
        int status = SUCCESS;

        // check parameters
        if ((bit < 0 || bit > MAX_IO_POINTS - 1) || (bitValue > 1))
        {
            return status = INVALID_PARAMETER;
        }

        // check for port lock
        if (port_locked & 1 << (bit % MAX_BITS_BYTE))
        {
            return status = ACCESS_ERROR;
        }

        // write bit
        bitp.devNum = 1;
        bitp.bitNum = ++bit;
        bitp.bitValue = (BYTE) bitValue;
        if (!DeviceIoControl(uio48Handle,
            IOCTL_WRITE_BIT,
            &bitp,
            sizeof(bitp),
            NULL,
            0,
            &junk,
            NULL))
        {
            if (GetLastError() == STATUS_ACCESS_VIOLATION)
                status = ACCESS_ERROR;
            else
                status = DRIVER_ERROR;
        }

        return status;
    }

    DECLDIR int EnableInterrupt(int bit, int edge)
    {
        int status = SUCCESS;

        // check parameters
        if ((bit < 0 || bit > MAX_INT_POINTS - 1) || (edge < FALLING_EDGE || edge > RISING_EDGE))
        {
            return status = INVALID_PARAMETER;
        }

        // enable interrupt
        bitp.devNum = 1;
        bitp.bitNum = ++bit;
        bitp.bitValue = (BYTE) edge; // polarity
        if (!DeviceIoControl(uio48Handle,
            IOCTL_ENAB_INT,
            &bitp,
            sizeof(bitp),
            NULL,
            0,
            &junk,
            NULL))
        {
            if (GetLastError() == STATUS_ACCESS_VIOLATION)
                status = ACCESS_ERROR;
            else
                status = DRIVER_ERROR;
        }

        return status;
    }

    DECLDIR int DisableInterrupt(int bit)
    {
        int status = SUCCESS;

        // check parameters
        if (bit < 0 || bit > MAX_INT_POINTS - 1)
        {
            return status = INVALID_PARAMETER;
        }

        // disable interrupt
        bitp.devNum = 1;
        bitp.bitNum = ++bit;
        if (!DeviceIoControl(uio48Handle,
            IOCTL_DISAB_INT,
            &bitp,
            sizeof(bitp),
            NULL,
            0,
            &junk,
            NULL))
        {
            status = DRIVER_ERROR;
        }

        return status;
    }

    DECLDIR int GetInterrupt(unsigned int *irqArray)
    {
        int status = SUCCESS;

        // check parameters
        if (irqArray == NULL)
        {
            return status = INVALID_PARAMETER;
        }

        // get interrupt
        irqp.devNum = 1;
        if (!DeviceIoControl(uio48Handle,
            IOCTL_GET_INT,
            &irqp,
            sizeof(irqp),
            &irqp,
            sizeof(irqp),
            &junk,
            NULL))
        {
            status = DRIVER_ERROR;
        }

        *(irqArray) = (unsigned int) irqp.irq[0] & 0xff;
        *(irqArray + 1) = (unsigned int) irqp.irq[1] & 0xff;
    #if MAX_INT_PORTS > 2
        * (irqArray + 2) = (unsigned int)irqp.irq[2] & 0xff;
    #endif

        return status;
    }

    DECLDIR int WaitForInterrupt(unsigned int *irqArray)
    {
        int status = SUCCESS;

        // check parameters
        if (irqArray == NULL)
        {
            return status = INVALID_PARAMETER;
        }

        // start wait
        irqp.devNum = 1;
        if (!DeviceIoControl(uio48Handle,
            IOCTL_WAIT_INT,
            &irqp,
            sizeof(irqp),
            &irqp,
            sizeof(irqp),
            &junk,
            NULL))
        {
            status = DRIVER_ERROR;
        }

        return status;
    }

    DECLDIR int LockPort(int port)
    {
        int status = SUCCESS;

        // check parameters
        if (port < 0 || port > MAX_IO_PORTS - 1)
        {
            return status = INVALID_PARAMETER;
        }

        // lock port
        bytep.devNum = 1;
        bytep.portNum = port;
        if (!DeviceIoControl(uio48Handle,
            IOCTL_LOCK_PORT,
            &bytep,
            sizeof(bytep),
            NULL,
            0,
            &junk,
            NULL))
        {
            status = DRIVER_ERROR;
        }
        else
        {
            // update locked status
            port_locked |= (1 << port);
        }

        return status;
    }

    DECLDIR int UnlockPort(int port)
    {
        int status = SUCCESS;

        // check parameters
        if (port < 0 || port > MAX_IO_PORTS - 1)
        {
            return status = INVALID_PARAMETER;
        }

        // unlock port
        bytep.devNum = 1;
        bytep.portNum = port;
        if (!DeviceIoControl(uio48Handle,
            IOCTL_UNLOCK_PORT,
            &bytep,
            sizeof(bytep),
            NULL,
            0,
            &junk,
            NULL))
        {
            status = DRIVER_ERROR;
        }
        else
        {
            // update locked status
            port_locked &= ~(1 << port);
        }

        return status;
    }

    DECLDIR int CloseSession()
    {
        int status = SUCCESS;

        // Close the device.
        if (!CloseHandle(uio48Handle))
            status = INVALID_HANDLE;

        return status;
    }
}
