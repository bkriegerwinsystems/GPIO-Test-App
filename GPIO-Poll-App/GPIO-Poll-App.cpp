//
//0 input 1 output
//
//UIO MAPPING WITH LOOPBACK
//
//INPUTS   0    1    2    3    4    5    6    7    20    21   22   23  
//         |    |    |    |    |    |    |    |    |     |    |    |    
//         |    |    |    |    |    |    |    |    |     |    |    |    
//         |    |    |    |    |    |    |    |    |     |    |    |    
//OUTPUTS  8    9    10   11   12   13   14   15   16    17   18   19

#include <tchar.h>
#include <stdio.h>
#include <stdlib.h>
#include "..\uio48DLL.h"
#include <conio.h>
#include <Windows.h>
#include <iostream>
#include <bitset>
#include <process.h>


//loopback plug connects nibbles of third port, and then connects the other two to each other.

#define FALLING_EDGE 0
#define RISING_EDGE 1
#define PORTCNT 3
#define PORTSZ 8
#define UIOCNT 24

BOOL quitFlag = FALSE;

//Waits for interrupts, and prints which line they are detected on.(Inputs should be port 0, and the second half of port 2)
void readInputsThread(void *)
{
	int errcode = 0;
	unsigned int timeout = 50000; //5 seconds
	unsigned int irqArr[PORTCNT];

	if(GetInterrupt(irqArr))
		std::cout << "Failed to get interrupts\n";

	std::cout << "Should see interrupts occur on all of port 0 (bits[0-7]).\n";

	for (int i = 0; i < UIOCNT; ++i)
    {
		if (((irqArr[i / PORTSZ]) & (1 << (i % PORTSZ))) > 0)
			std::cout << "Interrupt occured on bit : " << i << "\n";
	}

	while (!quitFlag)
	{
		errcode = 0;
		if (errcode = WaitForInterrupt(irqArr, timeout))
		{
			std::cout << "Failed to wait for interrupt with error code : " << errcode << "\n";
			continue;
		}

		for (int i = 0; i < UIOCNT; ++i)
		{
			if (((irqArr[i / PORTSZ]) & (1 << (i % PORTSZ))) > 0)
				std::cout << "Interrupt occured on bit :"<< i <<"\n";
		}

		Sleep(100);
	}
	_endthread();
}



int _tmain(int argc, _TCHAR* argv[])
{
	int errCode = 0;

	//Setting up 
	unsigned int testread = 1;
	unsigned int irqArr[PORTCNT];
	unsigned int portTest[PORTCNT];
	unsigned int maskTest[PORTCNT];
	unsigned int testTimeout = 1000;
	portTest[0] = 0x00;					//setting up iomask (Port 0 Input, Port 1 Output, Port 2[1-4] output, Port 2[5-8] input)
	portTest[1] = 0xFF;
	portTest[2] = 0x0F;

	try
    {
		if (errCode = InitializeSession())
		{
			std::cout << "Failed to initialize session.\n";
			throw errCode;
		}
		else
			std::cout << "Driver Initialized.\n";

		if (errCode = ResetDevice()) //make sure in clean state after initialization
		{ 
			std::cout << "Failed to reset ports.\n";
			throw errCode;
		}

		if (errCode = SetIoMask(portTest))
		{
			std::cout << "Failed to set mask.\n";
			throw errCode;
		}

		if (errCode = GetIoMask(maskTest))
		{
			std::cout << "Failed to get mask.\n";
			throw errCode;
		}

		for (int i = 0; i < PORTCNT; ++i) //ensure get and set are functioning as expected
		{
			if (portTest[i] != maskTest[i])
			{
				printf("Mask does not match what it was set to. Exiting . . .\n");
				return 1;
			}
		}

		//set first port to have rising edge interrupts


		for (int i = 0; i < PORTSZ; ++i){
			if (errCode = EnableInterrupt(i, RISING_EDGE))
			{
				std::cout << "Failed to enable interrupt.\n";
				throw errCode;
			}
		}

		//set second half of third port to have falling edge interrupts
		
		for (int i = 20; i < 24; ++i)
        {
			if (errCode = EnableInterrupt(i, FALLING_EDGE))
			{
				std::cout << "Failed to enable interrupt.\n";
				throw errCode;
			}
		}

		std::cout << "Testing Wait for Interrupt's timeout with "<<testTimeout<<"  milliseconds\n";

		if (WaitForInterrupt(irqArr, testTimeout) == TIMEOUT_ERROR)
		{
			std::cout << "WaitForInterrupt successfully threw a timeout error\n";
		}
		else{
			std::cout << "Timeout failed to work correctly\n";
		}

		if (errCode = WritePort(1, 0xFF)) //make interrupts occur on all of port 0 by making rising edge occur on all of port 1 (should see after get interrupt)
		{
			std::cout << "Failed to write port\n";
			throw errCode;
		}

		_beginthread(readInputsThread, 0, NULL); //kicks off thread that reads and prints inputs


		if (errCode = WritePort(1, 0x00)) //Setting outputs back to 0, so rising edge interrupts trigger
		{
			std::cout << "Failed to write port\n";
			throw errCode;
		}

		Sleep(1000);

		while (1)
		{
            //loops through all outputs
			
			for (int i = 8; i < 20; ++i) //each loop sets a bit and then clears it after one second
			{

				if (errCode = WriteBit(i, 1)) //set
				{
					std::cout << "Failed to set bit : " << i << "\n";
					throw errCode;
				}
				else
					std::cout << "Bit " << i << " set\n";

				Sleep(1000); //sleep one second

				if (errCode = WriteBit(i, 0)) //clear
				{
					std::cout << "Failed to clear bit: " << i << "\n";
					throw errCode;
				}
				else
					std::cout << "Bit " << i << " cleared\n";

				Sleep(1000);
			}

			std::cout << "\n**********\nInterrupt walk complete. Press q to quit, or any other key to restart walk\n***********\n";

			_kbhit();
			if (_getch() == 'q')
			{
				quitFlag = TRUE; 
				if (errCode = ResetDevice())
				{
					std::cout << "Failed to disable timer.\n";
					throw errCode;
				}

				if (errCode = CloseSession())
				{
					std::cout << "Failed to close Driver.\n";
					throw errCode;
				}
				std::cout << "WDT closed. Exiting\n";
				break;
			}
            
		}

	}
	catch (int err)
	{
		switch (err)
		{
		case DRIVER_ERROR:
		{
			std::cout << "Failed to communicate with driver\n";
			break;
		}
		case ACCESS_ERROR:
		{
			std::cout << "Access Error\n";
			break;
		}
		case INVALID_HANDLE:
		{
			std::cout << "Failed to obtain handle to driver\n";
			break;
		}
		case INVALID_PARAMETER:
		{
			std::cout << "Bad parameter\n";
			break;
		}
		default:
			std::cout << "Unkown error!\n";
		}
		system("pause");
		return err;
	}
	return 0;
}

