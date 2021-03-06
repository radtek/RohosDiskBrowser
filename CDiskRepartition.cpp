#include "stdafx.h"
#include <windows.h>
//#include <iostream>
//#include <stdio.h>
#include <Cfgmgr32.h>
#include "tchar.h"
#include "CDiskRepartition.h"
//#include "iostream"
//#include <fstream> 
//using namespace std;



#define BUFSIZE 206

#define IOCTL_VOLUME_ONLINE                     CTL_CODE(IOCTL_VOLUME_BASE, 2, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS) 
#define IOCTL_VOLUME_OFFLINE                    CTL_CODE(IOCTL_VOLUME_BASE, 3, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS) 


/*int WriteLog(WCHAR* format, ... )		
{										
	return 0;							
}*/										

bool CDiskRepartition::WriteData(LPBYTE buffer, DWORD len, PLARGE_INTEGER Offset)
{
	DWORD written;
	DWORD bytewritten; 
	if (len%512)
	{
		WriteLog("len is no aliquot to 512");
		return false;		
	}
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		WriteLog("Drive is no opened");
		return false;
	}

	LARGE_INTEGER liOffset;
	liOffset.QuadPart =  Offset->QuadPart + _partitioned_space.QuadPart + partOffset2.QuadPart;
	if (liOffset.QuadPart%512)
	{
		liOffset.QuadPart = 512 - (liOffset.QuadPart%512);
	}
	if (!SetFilePointer(hDevice, liOffset.LowPart, &liOffset.HighPart, FILE_BEGIN))
	{		
		WriteLog("Error to set file pointer %d",GetLastError());
		return false;
	}
	WriteFile(hDevice, buffer, len, &bytewritten, NULL);
	if (bytewritten!=len)
	{
		DWORD gle = GetLastError();
		WriteLog("Error to write file %d",gle);
		return false;
	}

	//WriteLog("Write at %X ", liOffset.QuadPart);

	return true;
}

bool CDiskRepartition::ReadData(LPBYTE buffer, DWORD len, PLARGE_INTEGER Offset)
{
	DWORD written;
	if (len%512)
	{
		WriteLog("len is no aliquot to 512");
		return false;		
	}
	if (hDevice == INVALID_HANDLE_VALUE)
	{
		WriteLog("Drive is no opened");
		return false;
	}
	LARGE_INTEGER liOffset;
	liOffset.QuadPart =  Offset->QuadPart/*FS data offset*/ + _partitioned_space.QuadPart/*USB partition offset*/ + partOffset2.QuadPart/*Rohos disk data offset*/;
	if (liOffset.QuadPart%512)
	{
		liOffset.QuadPart = 512 - (liOffset.QuadPart%512);
	}
	//SetFilePointer(hDevice, Offset.LowPart, &Offset.HighPart, FILE_BEGIN);
	if (!SetFilePointer(hDevice, liOffset.LowPart, &liOffset.HighPart, FILE_BEGIN))
	{		
		WriteLog("Error to set file pointer %d",GetLastError());
		return false;
	}
	DWORD bytewritten; 
	BOOL b=ReadFile(hDevice, buffer, len, &bytewritten, NULL);
	if (bytewritten!=len)
	{
		DWORD gle = GetLastError();
		WriteLog("Error to read file %d",gle);
		return false;
	}

	//WriteLog("Read at %X ", liOffset.QuadPart);
	return true;
}

int CDiskRepartition::GetVolumesLetter(PCHAR VolumeName)
{
    DWORD  CharCount = MAX_PATH + 1;
    char Names[20];
    PWCHAR NameIdx   = NULL;
    BOOL   Success   = FALSE;
    for (;;) 
    {
        Success = GetVolumePathNamesForVolumeNameA(
            VolumeName, (LPCH)Names, sizeof(Names), &CharCount
            );

        if ( Success ) 
        {
            return Names[0];
        }

        if ( GetLastError() != ERROR_MORE_DATA ) 
        {
            return -1;
        }
		else
        {
            return Names[0];
        }
    }
    return -1;
}

//function check for new partition
//get volume letter for each partition
//and set free letter for it
int CDiskRepartition::enumeratevolumes(void)
{	 
	//buffer for dos device volume name
	char Let[4];		
	//buffer for GUID volume name
	char buf[BUFSIZE]; 
	HANDLE hVol; 
	BOOL bFlag = true; 
	sprintf(Let,"%c:\\",DiskName);
	hVol = FindFirstVolumeA(buf, BUFSIZE );
	if (hVol == INVALID_HANDLE_VALUE)
	{
		printf (TEXT("No volumes found!\n"));
		return (-1);
	}
	while (bFlag)
	{
		bFlag = FindNextVolumeA(hVol, 
								buf, 
								BUFSIZE 
		);
		if (GetVolumesLetter(buf)<65)
		{
			OutputDebugStringA(buf);
			SetVolumeMountPoint(Let,buf);
			bFlag = FindVolumeClose(hVol);
			return false;			
		}
	}	
	bFlag = FindVolumeClose(hVol);
	return true;
}

//check is the bit with given number set
bool IsBitSet(DWORD val, int bit)
{
	return (val & (1 << bit)) != 0;
}

void CDiskRepartition::SetDriveNum(int num)
{
	 DriveNum = num;
}


void CDiskRepartition::SetUnpartitionedSpace(LARGE_INTEGER _unpartitioned_space)
{
	this->_unpartitioned_space = _unpartitioned_space;
}

void CDiskRepartition::SetDataOffset(PLARGE_INTEGER offs2)
{
	this->partOffset2.QuadPart = offs2->QuadPart;
}

void CDiskRepartition::SetFilePointer2(PLARGE_INTEGER offs2)
{
	this->partOffset.QuadPart = offs2->QuadPart;
}

LARGE_INTEGER CDiskRepartition::GetUnpartitionedSpace()
{
	return _unpartitioned_space;
}

LARGE_INTEGER CDiskRepartition::GetPartitionedSpace()
{
	return _partitioned_space;
}

void CDiskRepartition::SetDriveLetter(char driveletter)
{
		this->DiskName = driveletter;
}


LARGE_INTEGER CDiskRepartition::GetDriveSpace()
{
	LARGE_INTEGER li = _drive_space;
	//li.QuadPart -= 50 * 1024 * 1024;
	return li;
}

bool CDiskRepartition::RePartition(LARGE_INTEGER unpart)
{
	SetUnpartitionedSpace(unpart);
	return CreatePrimaryPartition();
}



int CDiskRepartition::CreatePrimaryPartition()
{
	DWORD ReadedBytes;
	LARGE_INTEGER oldunpart = _unpartitioned_space;
	PARTITION_INFORMATION_EX* tmppartition;
	CheckDisk();	
	//OpenDisk();
	_unpartitioned_space = oldunpart;
	_partitioned_space.QuadPart = _drive_space.QuadPart - _unpartitioned_space.QuadPart;
	_partitioned_space.QuadPart -= (5 * 1024 * 1024); // leave 5 mb gap 

	int _open_part_kb = _partitioned_space.QuadPart / 1024;
	WriteLog("CDiskRepartition: Part0 %d kb. Total %d kb", _open_part_kb, _drive_space.QuadPart / 1024);
	

	if ( _open_part_kb <=10 )
	{
		_partitioned_space.QuadPart = 32 * 1024; // Rohos Logon Key takes 25Kb
		_open_part_kb = _partitioned_space.QuadPart / 1024;
		WriteLog("CDiskRepartition: Part0 fix %d kb", _open_part_kb);
	}

	if (hDevice == INVALID_HANDLE_VALUE)
	{
		return false;
	}
	if (!DiskName)
	{
		return false;	
	}
	tmppartition = new PARTITION_INFORMATION_EX[4];
	LARGE_INTEGER maxoffset;
	maxoffset.QuadPart = 0;


	SetFilePointer(hDevice, 0, NULL, FILE_BEGIN);
	char buffer[512] = {'0'}; 
	DWORD bytewritten; 
	WriteFile(hDevice, buffer, 512, &bytewritten, NULL);

	DeviceIoControl((HANDLE) hDevice, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, 
                          NULL, 0, (LPVOID) &driveinfo, (DWORD) 4096*16, (LPDWORD) &ReadedBytes, NULL); 
		for (int i = 0; i < 4; i++)
		{
			tmppartition[i].PartitionNumber = 0; 
			tmppartition[i].RewritePartition = 1; 
			tmppartition[i].StartingOffset.QuadPart = 0;
			tmppartition[i].PartitionLength.QuadPart = 0; 
			tmppartition[i].Mbr.BootIndicator = 0; 
			tmppartition[i].Mbr.RecognizedPartition = 0; 
			tmppartition[i].Mbr.HiddenSectors = 0; 
			tmppartition[i].Mbr.PartitionType = 0;
			tmppartition[i].PartitionStyle = (PARTITION_STYLE)0;		
		}
		driveinfo.PartitionStyle = PARTITION_STYLE_MBR;
		driveinfo.PartitionCount = 4;
		driveinfo.Mbr.Signature = 0xA4B57300;


	CopyMemory(&driveinfo.PartitionEntry[0],tmppartition,sizeof(PARTITION_INFORMATION_EX)*4);
	if (!DeviceIoControl(hDevice,IOCTL_DISK_SET_DRIVE_LAYOUT_EX, &driveinfo,sizeof(DRIVE_LAYOUT_INFORMATION_EX)+driveinfo.PartitionCount * sizeof(PARTITION_INFORMATION_EX), 
                        NULL,0,&ReadedBytes, (LPOVERLAPPED) NULL))
	{
		//log[0]<<"IOCTL_DISK_SET_DRIVE_LAYOUT_EX "<<GetLastError()<<endl<<flush;
		WriteLog(L"IOCTL_DISK_SET_DRIVE_LAYOUT_EX %d \n",GetLastError());
		return false;
	}
	if (!DeviceIoControl(hDevice,IOCTL_DISK_UPDATE_PROPERTIES, 0,0, 
                        NULL,0,&ReadedBytes, (LPOVERLAPPED) NULL))
	{
		WriteLog(L"IOCTL_DISK_UPDATE_PROPERTIES %d \n",GetLastError());
		//log[0]<<"IOCTL_DISK_UPDATE_PROPERTIES "<<GetLastError()<<endl<<flush;
		return false;
	}

	//////////CREATE NEW PARTITION
	DeviceIoControl((HANDLE) hDevice, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, 
                          NULL, 0, (LPVOID) &driveinfo, (DWORD) 4096*16, (LPDWORD) &ReadedBytes, NULL); 
		tmppartition[0].PartitionNumber = 0; 
        tmppartition[0].RewritePartition = 1; 
		tmppartition[0].StartingOffset.QuadPart = 32256;
        tmppartition[0].PartitionLength.QuadPart = _partitioned_space.QuadPart; 
        tmppartition[0].Mbr.BootIndicator = 0; 
        tmppartition[0].Mbr.RecognizedPartition = 1; 
        tmppartition[0].Mbr.HiddenSectors = (DWORD)tmppartition[0].StartingOffset.QuadPart / 512; 
        tmppartition[0].Mbr.PartitionType = 7;
        tmppartition[0].PartitionStyle = (PARTITION_STYLE)0;
		for (int i = 1; i < 4; i++)
		{
			tmppartition[i].PartitionNumber = 0; 
			tmppartition[i].RewritePartition = 1; 
			tmppartition[i].StartingOffset.QuadPart = 0;
			tmppartition[i].PartitionLength.QuadPart = 0; 
			tmppartition[i].Mbr.BootIndicator = 0; 
			tmppartition[i].Mbr.RecognizedPartition = 0; 
			tmppartition[i].Mbr.HiddenSectors = 0; 
			tmppartition[i].Mbr.PartitionType = 0;
			tmppartition[i].PartitionStyle = (PARTITION_STYLE)0;		
		}
		driveinfo.PartitionStyle = PARTITION_STYLE_MBR;
		driveinfo.PartitionCount = 4;
		driveinfo.Mbr.Signature = 0xA4B57300;


	CopyMemory(&driveinfo.PartitionEntry[0],tmppartition,sizeof(PARTITION_INFORMATION_EX)*4);
	if (!DeviceIoControl(hDevice,IOCTL_DISK_SET_DRIVE_LAYOUT_EX, &driveinfo,sizeof(DRIVE_LAYOUT_INFORMATION_EX)+driveinfo.PartitionCount * sizeof(PARTITION_INFORMATION_EX), 
                        NULL,0,&ReadedBytes, (LPOVERLAPPED) NULL))
	{
		//log[0]<<"IOCTL_DISK_SET_DRIVE_LAYOUT_EX "<<GetLastError()<<endl<<flush;
		WriteLog(L"IOCTL_DISK_SET_DRIVE_LAYOUT_EX %d \n",GetLastError());
		return false;
	}
	if (!DeviceIoControl(hDevice,IOCTL_DISK_UPDATE_PROPERTIES, 0,0, 
                        NULL,0,&ReadedBytes, (LPOVERLAPPED) NULL))
	{
		WriteLog(L"IOCTL_DISK_UPDATE_PROPERTIES %d \n",GetLastError());
//		log[0]<<"IOCTL_DISK_UPDATE_PROPERTIES "<<GetLastError()<<endl<<flush;
		return false;
	}
	delete tmppartition;
	//CloseHandle(hDevice);

	return true;
}

bool CDiskRepartition::CheckDisk()
{
	DWORD ReadedBytes;
	PARTITION_INFORMATION_EX tmppartition;
	if(!(OpenDisk() == INVALID_HANDLE_VALUE))
	{
		DeviceIoControl((HANDLE) hDevice, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, 
                          NULL, 0, (LPVOID) &driveinfo, (DWORD) 4096*16, (LPDWORD) &ReadedBytes, NULL); 
		CopyMemory(&tmppartition,&driveinfo.PartitionEntry[0],sizeof(PARTITION_INFORMATION_EX));
		_partitioned_space.QuadPart  = tmppartition.StartingOffset.QuadPart + tmppartition.PartitionLength.QuadPart; /*+ 30208*/;
		LARGE_INTEGER startOff = tmppartition.StartingOffset;
		_unpartitioned_space.QuadPart  = _drive_space.QuadPart - _partitioned_space.QuadPart ;
		//_unpartitioned_space.QuadPart  -= ( * 1024 * 1024); // leave 10mb gap 
		//CloseDisk();

		WriteLog("Hidden part kb %d. Total %d kb", _unpartitioned_space.QuadPart / 1024, _drive_space.QuadPart / 1024);
		return true;
	}
	else
	{
		_partitioned_space.QuadPart = 0;
		_unpartitioned_space.QuadPart = 0;
		_drive_space.QuadPart = 0;	
	}
	return false;
}


void CDiskRepartition::CloseDisk()
{
	CloseHandle(hDevice);
}

HANDLE CDiskRepartition::OpenDisk()
{
	DWORD BytesReturned;
	PARTITION_INFORMATION_EX tmppartition;
	VOLUME_DISK_EXTENTS vde;
//	Device\Harddisk1\DP(1)0-0+10"	char [200]
	ZeroMemory(DriveName,20);
	sprintf(DriveName,"\\\\.\\%c:",DiskName);
	//BytesReturned = QueryDosDevice(DriveName,DriveName,200);
	hDevice = CreateFileA(DriveName,GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,(HANDLE)-1);
	if (hDevice  == INVALID_HANDLE_VALUE)
	{
		WriteLog("CreateFile %d \n",GetLastError());
		//log[0]<<"CreateFileA "<<GetLastError()<<endl<<flush;
		return INVALID_HANDLE_VALUE;
	}
	if (DeviceIoControl(hDevice, // device to be queried
							IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, // operation to perform
							NULL, 0, // no input buffer
							&vde, sizeof(vde), // output buffer
							&BytesReturned, // # bytes returned
							(LPOVERLAPPED) NULL)) // synchronous I/O
	
	{
	}
	else
	{
		WriteLog("IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS %d \n",GetLastError());
		//log[0]<<"IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS "<<GetLastError()<<endl<<flush;
		return INVALID_HANDLE_VALUE;
	} 
	
	if (vde.NumberOfDiskExtents>0)
	{
		sprintf(DriveName,"\\\\.\\PHYSICALDRIVE%d",vde.Extents[0].DiskNumber);
	}
	else
	{
		WriteLog("IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS didn't return data %d \n",GetLastError());
//		log[0]<<"IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS didn't return data"<<GetLastError()<<endl<<flush;
		return INVALID_HANDLE_VALUE;	
	}

	CloseHandle(hDevice);

	hDevice = CreateFileA(DriveName,GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,(HANDLE)-1);
		//log[0]<<"CreateFileA "<<GetLastError()<<endl<<flush;
	if (hDevice == INVALID_HANDLE_VALUE) 
	{
		WriteLog("CreateFileA %X (%s)\n",GetLastError(), DriveName);
		return INVALID_HANDLE_VALUE;
	}

	if (DeviceIoControl(hDevice, // device to be queried
							IOCTL_DISK_GET_DRIVE_GEOMETRY, // operation to perform
							NULL, 0, // no input buffer
							&DiskGeometry, sizeof(DiskGeometry), // output buffer
							&BytesReturned, // # bytes returned
							(LPOVERLAPPED) NULL)) // synchronous I/O
	
	{
		_drive_space.QuadPart = DiskGeometry.Cylinders.QuadPart * (ULONG)DiskGeometry.TracksPerCylinder *
			  (ULONG)DiskGeometry.SectorsPerTrack * (ULONG)DiskGeometry.BytesPerSector;
	}
	else
	{
		//log[0]<<"IOCTL_DISK_GET_DRIVE_GEOMETRY "<<GetLastError()<<endl<<flush;
		WriteLog(L"IOCTL_DISK_GET_DRIVE_GEOMETRY \n",GetLastError());
		return INVALID_HANDLE_VALUE;
	} 

	DeviceIoControl((HANDLE) hDevice, IOCTL_DISK_GET_DRIVE_LAYOUT_EX, 
                        NULL, 0, (LPVOID) &driveinfo, (DWORD) 4096*16, (LPDWORD) &BytesReturned, NULL); 
	CopyMemory(&tmppartition,&driveinfo.PartitionEntry[0],sizeof(PARTITION_INFORMATION_EX));

	// open partition space
	_partitioned_space.QuadPart  = tmppartition.StartingOffset.QuadPart + tmppartition.PartitionLength.QuadPart;

	// encrypted partition space
	_unpartitioned_space.QuadPart  = _drive_space.QuadPart - _partitioned_space.QuadPart ;

	partOffset2.QuadPart =  0; 
	partOffset.QuadPart =  0; 


	return hDevice;
}



CDiskRepartition::CDiskRepartition()
{
	
	this->DriveNum = -1;
	this->DiskName = 0;
	this->_unpartitioned_space.QuadPart = 0;
	this->_partitioned_space.QuadPart = 0;
	this->_drive_space.QuadPart = 0;
	partOffset2.QuadPart =  0; 
	partOffset.QuadPart =  0; 
	
	
}

/*
int main()
{
	CDiskRepartition *cdp;
	LARGE_INTEGER _unpartitioned_size;
	_unpartitioned_size.QuadPart = 500*1024*1024;
	cdp = new CDiskRepartition(1,'Z',_unpartitioned_size);
	cdp->RePartition();
	delete cdp;

}
*/
