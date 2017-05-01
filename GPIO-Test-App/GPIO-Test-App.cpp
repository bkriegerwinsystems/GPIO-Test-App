// GPIO-Test-App.cpp : Defines the entry point for the console application.
//0 input 1 output
//
#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include "uio48DLL.h"
#include <conio.h>
#include <Windows.h>
#include <iostream>
#include <bitset>
#include <process.h>

//loopback plug connects nibbles of third port, and then connects the other two to each other.

//SetIOMask -- should you be able to toggle input  output on locked port?

//Reads and prints inputs (Inputs should be port 0, and the second half of port 2)
void readInputsThread(void *)
{
	unsigned int reading=0;
	unsigned int readnib=0;
	unsigned int readbit = 0;
	while (1)
	{
		readnib = 0;
		if (ReadPort(0,&reading))
			std::cout << "Failed to read\n";

		for (int j = 20; j < 24; ++j) //reads second nibble of port 2
		{
			if (ReadBit(j, &readbit))
				std::cout << "Failed to read\n";
			if (readbit)
				readnib = readnib | (readbit << j-20);
		}

		std::bitset<8> binreadbyte(reading);
		std::bitset<4> binreadnib(readnib);
		std::cout << "UIO 8-1 + 24-21: " << binreadbyte << " + "<<binreadnib<<"\n";
		Sleep(1000); //1 second sleep
	}
}



int _tmain(int argc, _TCHAR* argv[])
{
	int errCode = 0;

	//Setting up 
	unsigned int testread = 1;
	unsigned int portTest[3];
	unsigned int maskTest[3];
	portTest[0] = 0x00;
	portTest[1] = 0xFF;
	portTest[2] = 0x0F;
	try{
		if (errCode = InitializeSession())
		{
			std::cout << "Failed to initialize session.\n";
			throw errCode;
		}
		else
			std::cout << "Driver Initialized.\n";

		if (errCode = ResetDevice()){ //make sure in clean state on open
			std::cout << "Failed to reset ports.\n";
			throw errCode;
		}
		
		/*if (errCode = LockPort(0)){
			std::cout << "Failed to write port.\n";
			throw errCode;
		}*/

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

		for (int i = 0; i < 3; ++i) //ensure get and set are functioning as expected
		{
			if (portTest[i] != maskTest[i])
			{
				printf("Mask does not match what it was set to. Exiting . . .\n");
				return 1;
			}
		}

		_beginthread(readInputsThread, 0, NULL); //kicks off thread that reads and prints inputs

		

		while (1)
		{
			for (int i = 8; i < 20; ++i) //each loop does a set bit and then clears it after one second
			{

				if (errCode = SetBit(i))
				{
					std::cout << "Failed to set bit : " << i << "\n";
					throw errCode;
				}
				Sleep(1000); //sleep one second
				if (errCode = ClearBit(i))
				{
					std::cout << "Failed to clear bit: " << i << "\n";
					throw errCode;
				}
			}
		}

		if (errCode = ResetDevice()){ //clean gpio states before closing
			std::cout << "Failed to reset ports.\n";
			throw errCode;
		}

		if (errCode = CloseSession())
		{
			std::cout << "Failed to close session.\n";
			throw errCode;
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
		//system("pause");
		return err;
	}
	//system("pause");
	return 0;
}

