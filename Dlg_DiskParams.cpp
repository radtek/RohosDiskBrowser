/**********************************************************************
** Copyright (C) 2005-2015 Tesline-Service S.R.L. All rights reserved.
**
** Rohos Disk Browser, Rohos Mini Drive Portable.
** 
**
** This file may be distributed and/or modified under the terms of the
** GNU Affero General Public license version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file. If not, see <http://www.gnu.org/licenses/>
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.rohos.com/ for GPL licensing information.
**
** Contact info@rohos.com http://rohos.com
**/


#include "stdafx.h"
#include "DiskBrowserApp.h"
#include "Dlg_DiskParams.h"
#include "io.h"

// CDlg_DiskParams dialog


/////////////////////////////////////////////////////////////////////////////
 // lpszCanon = C:\MYAPP\DEBUGS\C\TESWIN.C
 //
 // cchMax    Result
 // ------    ---------
 // 15-16     C:\...\TESWIN.C
//			  C:\...\...cole.c
 // 17-23     C:\...\C\TESWIN.C
 // 24-25     C:\...\DEBUGS\C\TESWIN.C
 // 26+       C:\MYAPP\DEBUGS\C\TESWIN.C
 
 void _AfxAbbreviateName(LPTSTR lpszCanon, int cchMax)
 {
	 /*
	 char drive[_MAX_DRIVE];
	 char dir[_MAX_DIR];
	 char fname[_MAX_FNAME];
	 char ext[_MAX_EXT];

	 char path[200] = {""};	 

	 if ( strlen(lpszCanon) > cchMax ) {
		 
		 _splitpath( lpszCanon, drive, dir, fname, ext );

		 strcpy(path, drive);
		 if ( strlen(drive) )
			 strcat(path,"\\...\\");

		 strncat(path,fname, cchMax - (strlen(path)+4));
		 //if ( strlen(fname) > cchMax - 
		 strncat(path,ext, _MAX_EXT);

		 strcpy(lpszCanon, path);
	 }

	 return;*/

	 
 	int cchFullPath, cchFileName, cchVolName;
 	const TCHAR* lpszCur;
 	const TCHAR* lpszBase;
 	//const TCHAR* lpszFileName;

	char drive[_MAX_DRIVE];
   char dir[_MAX_DIR];
   char fname[_MAX_FNAME];
   char ext[_MAX_EXT];

 

   TCHAR szPath[MAX_PATH]=TEXT("");	
	SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, szPath);
	if ( _tcsncmp(szPath,lpszCanon, _tcslen(szPath) ) ==0 ) {
		// leave only "My Documents"
		//SHGetFolderPath(NULL, CSIDL_PROFILE, NULL, 0, szPath);
		//LPTSTR c = _tcsrchr( lpszCanon + _tcslen(szPath) - , '\\' );
		LPTSTR c = lpszCanon + _tcslen(szPath) + 1;
		if (c) {
			_tcscpy(szPath, c);
			_tcscpy(lpszCanon, szPath);
			return;
		}
	}



 	lpszBase = lpszCanon;
 	cchFullPath = _tcslen(lpszCanon);

	_splitpath( lpszCanon, drive, dir, fname, ext );
 
 	
	cchFileName = _tcslen(fname);

 	//lpszFileName = lpszBase + (cchFullPath-cchFileName);
 
 	// If cchMax is more than enough to hold the full path name, we're done.
 	// This is probably a pretty common case, so we'll put it first.
 	if (cchMax >= cchFullPath)
 		return;


 
 	// If cchMax isn't enough to hold at least the basename, we're done
 	if (cchMax < cchFileName)
 	{
		_tcscpy(lpszCanon, drive);
		_tcscat(lpszCanon, TEXT("\\..."));
		CString s1; s1= fname;
		_tcscat(lpszCanon, s1.Mid(0, cchMax-6) ); //fname + cchFileName/3);
		_tcscat(lpszCanon, TEXT("."));
 		_tcscat(lpszCanon, ext);
 		return;
 	}
 
 	// Calculate the length of the volume name.  Normally, this is two characters
 	// (e.g., "C:", "D:", etc.), but for a UNC name, it could be more (e.g.,
 	// "\\server\share").
 	//
 	// If cchMax isn't enough to hold at least <volume_name>\...\<base_name>, the
 	// result is the base filename.
 
 	lpszCur = lpszBase + 2;                 // Skip "C:" or leading "\\"
 
 	if ( _tcscmp(dir, TEXT("\\\\")) == 0) // UNC pathname
 	{
		// set drive as \\server\s
 		// First skip to the '\' between the server name and the share name,
 		while (*lpszCur != '\\')
 		{
 			lpszCur = _tcsinc(lpszCur);
 		//	ASSERT9(*lpszCur != '\0', "_AfxAbbreviateName");
 		}
 	}
 	// if a UNC get the share name, if a drive get at least one directory
 	//ASSERT9(*lpszCur == '\\', "_AfxAbbreviateName" );
 	// make sure there is another directory, not just c:\filename.ext
 	if (cchFullPath - cchFileName > 3)
 	{
 		lpszCur = _tcsinc(lpszCur);
 		while (*lpszCur != '\\')
 		{
 			lpszCur = _tcsinc(lpszCur);
 			//ASSERT9(*lpszCur != '\0', "_AfxAbbreviateName");
 		}
 	}
 	//A/SSERT9(*lpszCur == '\\', "_AfxAbbreviateName");
 
 	cchVolName = int(lpszCur - lpszBase);
 	if (cchMax < cchVolName + 5 + cchFileName)
 	{
 		lstrcpy(lpszCanon, fname);
 		return;
 	}
 
 	// Now loop through the remaining directory components until something
 	// of the form <volume_name>\...\<one_or_more_dirs>\<base_name> fits.
 	//
 	// Assert that the whole filename doesn't fit -- this should have been
 	// handled earlier.
 
 	//ASSERT9(cchVolName + (int)lstrlen(lpszCur) > cchMax, "_AfxAbbreviateName");
 	while (cchVolName + 4 + (int)lstrlen(lpszCur) > cchMax)
	//while (cchVolName + 4 + (int)lstrlen(lpszCur) < cchMax)
 	{
 		do
 		{
 			lpszCur = _tcsinc(lpszCur);
 			//ASSERT9(*lpszCur != '\0', "_AfxAbbreviateName");
 		}
 		while (*lpszCur != '\\');
 	}
 
 	// Form the resultant string and we're done.
 	lpszCanon[cchVolName] = '\0';
 	lstrcat(lpszCanon, _T("\\..."));
 	lstrcat(lpszCanon, lpszCur);
	
 }





IMPLEMENT_DYNAMIC(CDlg_DiskParams, CDialog)

CDlg_DiskParams::CDlg_DiskParams(CWnd* pParent /*=NULL*/)
	: CDialog(CDlg_DiskParams::IDD, pParent)
	, _size(0)
{
	_label = _T("");
	//show_password_box = false;
	_name=TEXT("");
	_path = "";
	_was_shown = false;
	_partition_based = false;
	myFileDialog = new CFileDialog(TRUE);

}

CDlg_DiskParams::~CDlg_DiskParams()
{
}

void CDlg_DiskParams::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlg_DiskParams)		
	DDX_Text(pDX, IDC_EDIT1, _size);
	DDV_MinMaxUInt(pDX, _size, 15, 22000000);
	DDX_Text(pDX, IDC_EDIT2, _path);
	DDX_Text(pDX, IDC_STATIC_DISK_INFO, m_disk_info);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlg_DiskParams, CDialog)
	ON_BN_CLICKED(IDOK, &CDlg_DiskParams::OnBnClickedOk)
	ON_BN_CLICKED(IDOK2, &CDlg_DiskParams::OnBnClickedOk2)
//	ON_EN_CHANGE(IDC_EDIT1, &CDlg_DiskParams::OnEnChangeEdit1)
END_MESSAGE_MAP()


// CDlg_DiskParams message handlers

void CDlg_DiskParams::OnBnClickedOk()
{
	// TODO: Add your control notification handler code here

	UpdateData();

	GetDlgItemText( IDC_COMBO_FAT, _fs);
	GetDlgItemText( IDC_COMBO_ALGO, _algoritm);
	GetDlgItemText( IDC_COMBO_LETTER, _name);
	//_name = path;
	
	TCHAR path[600];
	_tcscpy(path, _path);
	_AfxAbbreviateName(path, 35);
	_ImagePathForDisplay = path;

	// ñîçäàòü ïóòü
	CString only_path = _path;
	only_path = _path.Mid(0, _path.ReverseFind('\\') );
	//CreateAllDirectories(&only_path);

	_tcscpy(path, _path);
	
	if (path[0]=='\\')  {
		LPTSTR ch = _tcsrchr(path, '\\');
		if ( ch ) *(ch+1) = 0;
	}
	else
		path[3]=0;

	__int64 free_space = GetDiskFreeSpaceMB(path);

	if (_partition_based)
	{
		free_space = _theDiskPartition.GetDriveSpace().QuadPart;
	}

	if (free_space<_size) {
		CString s1;
		s1.Format(IDS_SMALL_SPACE_X_DRIVE, path);
		if ( IDNO == AfxMessageBox(s1, MB_ICONWARNING | MB_YESNO ) )
			return;
	}

	_sizemb.Format("%d", _size);

	// ïðîâåðèòü íà îãðàíè÷åíèÿ FS , à òî íå îòôîðìàòèòñÿ.
	if (_size<50 && _fs == "FAT32") {
		_fs = TEXT("FAT");
	}

	if (_size>2000 && _fs == "FAT") {
		_fs = TEXT("FAT32");
	}

	if (_size>4000 && _fs == "FAT32") {
		_fs = TEXT("NTFS");
	}

	if (_partition_based) // all other params doesnt matter for re-partitioning
	{
		_fs = TEXT("NTFS");
		path[3]=0;
		_ImagePathForDisplay = path;

		_DiskSizeInBytes.QuadPart = 0;
		_DiskSizeInBytes.u.LowPart = _size;
		_DiskSizeInBytes.QuadPart *= 1048576;/* bytes in 1 mb*/;	


		LARGE_INTEGER drive_space = _theDiskPartition.GetDriveSpace();
		int drive_space_mb = drive_space.QuadPart / (1024*1024);

		if ( _size > 3600 && _size < drive_space_mb )
		{
			AfxMessageBox("Encrypted partition size is limited to 3.5 GB");
			return;
		}

		if ( _DiskSizeInBytes.QuadPart > drive_space.QuadPart)
		{
			_DiskSizeInBytes.QuadPart = _theDiskPartition.GetDriveSpace().QuadPart;
			_DiskSizeInBytes.QuadPart -= (1 * 1024*1024);

			_size /= 1048576;

			TCHAR str[50];	
			StrFormatByteSize64( _DiskSizeInBytes.QuadPart,  str, 40);      	
			_sizemb = str; 

			WriteLog("CDlg_DiskParams::onOK Adjust disk size to max value %d", _size);
		}

		OnOK();
		return;
	}
	
	
	if ( IsFileUseSteganos(_path) )  {		
		
		DWORD begin_mb, last_mb, file_size_MB;
		if ( !GetSteganosOffset(_path, &begin_mb, &last_mb, &file_size_MB) ) {
			CString s1;
			s1.Format(IDS_WRONG_FOR_STEGANOS, _path);
			AfxMessageBox(s1, MB_ICONINFORMATION);
			_path="";
			UpdateData(false);
			return;
		}		
		
		if (_size > file_size_MB - begin_mb -last_mb -1) {
			_size = file_size_MB;
			_size -= begin_mb; // óïóùåíèå â íà÷àëå ôàéëà
			_size -= last_mb; // óïóùåíèå â êîíöå ôàéëà
			_size -= 1;
		}

		_sizemb.Format("%d", _size);
		UpdateData(false);
	}

	if ( _size>MAX_DISK_SIZEMB_MINIDRIVE && IsMiniDriveKey(NULL) ){
		_size = MAX_DISK_SIZEMB_MINIDRIVE;
		_sizemb.Format("%d", _size);
	}	
	

	TCHAR fsname[12] = {0}; // file system name (FAT NTFS)
	DWORD dw1, dw2;

	// åñëè FS íà host äèñêå = FAT òî óâåëè÷òâàòü ìîæíî äî 4 gb !
	if ( _size > 4000 && GetVolumeInformation(path, NULL, 0, NULL, &dw1, &dw2, fsname, 10) ) {
		if ( _tcsstr(fsname, "NTFS") ==0)  {
			AfxMessageBox("Host drive filesystem is FAT/FAT32. Maximum disk size is 4gb.", MB_OK | MB_ICONINFORMATION);
			_size = 3900;
			_sizemb.Format("%d", _size);
		}

	}
	_DiskSizeInBytes.QuadPart = 0;
	_DiskSizeInBytes.u.LowPart = _size;
	_DiskSizeInBytes.QuadPart *= 1048576;/* bytes in 1 mb*/;	

	if (free_space <= _size && _encrypt_USB_drive !="")  
	{		
		
		if ( GetDiskFreeSpaceEx( _encrypt_USB_drive, NULL, NULL, &_DiskSizeInBytes) ) 
		{
			_DiskSizeInBytes.QuadPart -= 2103700; // îñòàâèòü òîëüêî ìåñòî äëÿ rbrowser.exe, agent.exe ... 
			DWORD dskb  = _DiskSizeInBytes.QuadPart / 1024;
			WriteLog("New Disk size in KB %d", dskb );

		} else 
		{
			_DiskSizeInBytes.QuadPart = 0;
			_DiskSizeInBytes.u.LowPart = _size;
			_DiskSizeInBytes.QuadPart *= 1048576;/* bytes in 1 mb*/;	
		}		

	}

	//UpdateData();	

	OnOK();
}

// browse for a disk-image file path 
void CDlg_DiskParams::OnBnClickedOk2()
{
		TCHAR path[255];
	_tcscpy(path, _path);
	
	if ( SelectFolderDialog(m_hWnd, (LPTSTR)LS(IDS_DISK_FOLDER), path ) ) {
		
		int i=0;	

		// remove last backslash 
		if ( path[strlen(path)-1]=='\\')
			path[strlen(path)-1]=0;

		// òàêîé ôàéë íå äîëæåí ñóùåñòâîââàòü , ñóùåñòâóþùèå îáðàçû íå óäàëÿòü		
		do {
			// ôàéë ñóùåñòâóåò		
			_path.Format("%s\\rdisk%d.rdi",path, i);			
			i++;
		} while (_taccess(_path, 0)==0 );				
		
		
		UpdateData(false);
	}	
}


void CDlg_DiskParams::SetDefaultValues_Partition()
{
	_theDiskPartition.SetDriveLetter(_encrypt_USB_drive[0]);

	_theDiskPartition.CheckDisk();

	LARGE_INTEGER li = _theDiskPartition.GetDriveSpace();
	
	// Total USB drive Size / 2 
	_DiskSizeInBytes.QuadPart = li.QuadPart; 
	_DiskSizeInBytes.QuadPart /= 2;

	_size = _DiskSizeInBytes.QuadPart / (1024*1024);

	if ( _size > 3600  )
	{
		_DiskSizeInBytes.QuadPart = 3584;
		_DiskSizeInBytes.QuadPart *= (1024*1024);
		_size = 3584;
		WriteLog("set max 3gb");
	}
	

	//if ( _size>MAX_DISK_SIZEMB_MINIDRIVE && IsMiniDriveKey(_regkey) )
	//	_size = MAX_DISK_SIZEMB_MINIDRIVE;

	TCHAR str[50];	
	StrFormatByteSize64( _DiskSizeInBytes.QuadPart,  str, 45);      	
	_sizemb = str; 

	//AfxMessageBox(_sizemb);
	
	if (_label.IsEmpty())
		_label = TEXT("Rohos Disk");

	_tcscpy(str, _encrypt_USB_drive);
	str[3]=0;

	_ImagePathForDisplay = str;
	_path = str;

	_theDiskPartition.CloseDisk();

	//CString s1;
	//s1 =LS(IDS_HDD_FREE_SPACE);
	//SetDlgItemText(IDC_STATIC_DISK_INFO, s1);


}

/** Âû÷èñëèòü è ïðîñ÷èòàòü çíà÷åíèÿ ïî óìîë÷àíèþ

  \todo max disk size	
À èìåííî, ìàêñèìàëüíûé ðàçìåð ôàéëà-êîíòåéíåðà: 
• íà òîìå FAT . 2047 Ìáàéò; 
• íà òîìå FAT32 . 4095 Ìáàéò; 
• íà òîìå NTFS . ïðàêòè÷åñêè íå îãðàíè÷åí. 

*/
void CDlg_DiskParams::SetDefaultValues()
{

	// scan for available Drives Name
	TCHAR drive[4] = TEXT("A:");
	//memset( &drive, 0, sizeof( drives ) );
	int sel=0;
	_name=TEXT("");
	for( int i = 2; i < 26; i++ )
	{
		drive[0] = (char)( 'A' + i );
		//drive[1] = ':';
		//SendDlgItemMessage(IDC_COMBO_LETTER, CB_GETCOUNT, 0, (LPARAM)drive[i] );
		if( !( ( GetLogicalDrives() >> i ) & 1 ) ) {
			
			// by default select R: (if its not occupied)
			if (drive[0]=='R') {
				_name = drive;
			}

			if (_name.GetLength() == 0)
				_name = drive;
		}

	}
	
	/*if (get_windows_version() == W2000)
	_fs =TEXT("NTFS");
		else*/
	_fs =TEXT("NTFS");
	_algoritm = TEXT("AES 256");

	if (_partition_based)
	{
		return SetDefaultValues_Partition();
	}


	//GetDlgItemText( IDC_COMBO_FAT, _fs);

	// Ïðåäëîæèòü ïóòü äëÿ ImageFile 
	//  My Documents
	
	TCHAR szPath[MAX_PATH]=TEXT("");	
	if (_encrypt_USB_drive !="") {
		// ýòî çàïðîñ íà øèôðîâàíèå USB íàêîïèòåëÿ
		_tcscpy(szPath, _encrypt_USB_drive);

	} else 
		SHGetFolderPath(NULL, CSIDL_PERSONAL, NULL, 0, szPath);

	if (_tcslen(szPath) == 0) {
		WriteLog("Fail to get My Docs folder");
		GetWindowsDirectory(szPath, MAX_PATH);
		szPath[3]=0; // c:
	}

	ULARGE_INTEGER avail_free, avail_;

	GetDiskFreeSpaceEx(szPath, &avail_free, &avail_, NULL);

	ULONGLONG sizeMB = avail_free.QuadPart / 1048576;

	if (sizeMB < 500) {
		// åñòü ëè íà äð âèíòàõ ñâîáîäíîå ìåñòî >500 Ìá  ?
		WriteLog("CDlg_CreateDisk2::GetDiskSuggestedImagePath, lowdisk space %d mb, %s", sizeMB, szPath);
	}

	int i=0;
	if (_path.IsEmpty()) {

		// åñëè ïóòü íå çàäàí ñâåðõó òî ïîäîáðàòü 
		_path = szPath ;	
		if (_encrypt_USB_drive !="") {
			// ýòî çàïðîñ íà øèôðîâàíèå USB íàêîïèòåëÿ
			//    îáðàç äèñêà ëåæèò â ïàïêå _rohos
			_path += "_rohos";
			if ( !CreateDirectory(_path, NULL) ) {
				if (GetLastError() != ERROR_ALREADY_EXISTS)
					WriteLog("CDlg_CreateDisk2::SetDefaultValues error create USB folder %X", GetLastError());
			}
		} 
		_path += "\\rdisk.rdi";

		i=0;
		// òàêîé ôàéë íå äîëæåí ñóùåñòâîââàòü , ñóùåñòâóþùèå îáðàçû íå óäàëÿòü		
		while (_taccess(_path, 0)==0 ) {
			// ôàéë ñóùåñòâóåò		
			_path.Format("%s\\rdisk%d.rdi",szPath,i);
			if (_encrypt_USB_drive !="")
				_path.Format("%s_rohos\\rdisk%d.rdi",szPath,i);

			i++;
		};
		
	}
	

	//_path = _path.Right(_path.GetLength() -  _path.ReverseFind('\\')-1 ) + "\\my_disk.rdi";

	_tcscpy( szPath, _path);
	_AfxAbbreviateName(szPath, 25);
	_ImagePathForDisplay = szPath;

	// ïîëó÷èòü ñâîáîäíîå ìåñòî äëÿ ImagePath	
	szPath[3]=0;
	GetDiskFreeSpaceEx(szPath, &avail_free, &avail_, NULL);

	sizeMB = avail_free.QuadPart / 1048576;
	if ( sizeMB )
		_size= sizeMB/2; // default is 200 Mb

	if (sizeMB == 0) {
		WriteLog("CDlg_CreateDisk2: fail to get free space %X %s", GetLastError(), szPath );
		_size = 30;
	}	

	if (sizeMB >1000) 
		_size = 500;
	if (sizeMB >2000) 
		_size = 900;
	if (sizeMB >5000) 
		_size = 2000;	
	if (sizeMB >9000) 
		_size = 3000;	
	if (sizeMB >15000) 
		_size = 5000;
	if (sizeMB >45000) 
		_size = 9000;
	
	// åñëè FS íà host äèñêå = FAT òî óâåëè÷òâàòü ìîæíî äî 4 gb !
	TCHAR fsname[12] = {0}; // file system name (FAT NTFS)
	DWORD dw1, dw2;		

	if ( _size>4000 && GetVolumeInformation(szPath/*host drive*/, NULL, 0, NULL, &dw1, &dw2, fsname, 10) ) {

		if ( _tcsstr(fsname, "NTFS") ==0)  {			
			_size = 3900;
		}

	}

		
	
	if ( IsFileUseSteganos(_path) )  {

		WriteLog("CDlg_CreateDisk2: using steganos");
		
		DWORD begin_mb, last_mb, file_size_MB;
		if ( GetSteganosOffset(_path, &begin_mb, &last_mb, &file_size_MB) ) {
			
			_size = file_size_MB;
			_size -= last_mb; // óïóùåíèå â íà÷àëå ôàéëà
			_size -= begin_mb; // óïóùåíèå â êîíöå ôàéëà
			if (file_size_MB>5)
				_size -= 1;
			else 
				_fs =TEXT("FAT");
			
			_label = _ImagePathForDisplay.Mid(3, 30);

		} else
			WriteLog("CDlg_CreateDisk2: fail to calculate steganos");

	}

	if ( _size>MAX_DISK_SIZEMB_MINIDRIVE && IsMiniDriveKey(_regkey) )
		_size = MAX_DISK_SIZEMB_MINIDRIVE;

	_DiskSizeInBytes.QuadPart = 0;
	_DiskSizeInBytes.u.LowPart = _size;
	_DiskSizeInBytes.QuadPart *= 1048576;/* bytes in 1 mb*/;
	

	TCHAR str[50];	
	StrFormatByteSize64( _DiskSizeInBytes.QuadPart,  str, 40);      	

	_sizemb = str;//.Format("%d", _size);


	
	if (_label.IsEmpty())
		_label = TEXT("Rohos Disk");

	if (_encrypt_USB_drive !="") {

		//_fs =TEXT("FAT32");		// äëÿ FAT32 ó íàñ - read/write support in Disk Browser/
		_fs =TEXT("NTFS");		// äëÿ NTFS ó íàñ - read/write support since Disk Browser v 1.6 

		// øèôðóåì USB - çíà÷èò èìÿ äèñêà F: hidden volume
		_label = LS(IDS_HIDDEN_VOLUME);
		if (_label.GetLength())
			_label.SetAt(0, _encrypt_USB_drive[0] );
		else
			_label = TEXT("Rohos Disk");
	}
}



BOOL CDlg_DiskParams::OnInitDialog()
{
	CDialog::OnInitDialog();

	CString s1;
	s1 =LS(IDS_HDD_FREE_SPACE);

	if (_encrypt_USB_drive !="") {
		// 
		//  detect free space on usb drive
		s1 += " ";
		s1 += GetFormatedFreeDiskSpace(_encrypt_USB_drive);
		SetDlgItemText(IDC_STATIC_DISK_INFO, s1);
		
	}

	if (_partition_based)
	{
		TCHAR str[50];
		StrFormatByteSize64( _theDiskPartition.GetDriveSpace().QuadPart,  str, 40);      

		s1 += " ";
		s1 += str; 
		
		//GetFormatedFreeDiskSpace(_encrypt_USB_drive);
		SetDlgItemText(IDC_STATIC_DISK_INFO, s1);

	}

	SetWindowText(LS(IDS_CREATE_DISK2_DLG));
	SetDlgItemText(IDC_STATIC1, LS(IDS_DRIVE_LETER) );
	SetDlgItemText(IDC_STATIC2, LS(IDS_DISK_SIZE_MB)) ;
	SetDlgItemText(IDC_STATIC3, LS(IDS_DISK_FS));
	SetDlgItemText(IDC_STATIC4, LS(IDS_ENCYPT_ALGORITM));
	SetDlgItemText(IDC_STATIC5, LS(IDS_DISK_LOCATION));
	SetDlgItemText(IDCANCEL, LS(IDS_CANCEL));
	
	
	if (!_was_shown)
	{		
		SetDefaultValues();
	}

	// scan for available Drives Name
	TCHAR drive[4] = TEXT("A:");
	int sel=0;
	for( int i = 2; i < 26; i++ )
	{
		drive[0] = (char)( 'A' + i );
		if( !( ( GetLogicalDrives() >> i ) & 1 ) ) {			
				SendDlgItemMessage( IDC_COMBO_LETTER, CB_ADDSTRING, 0, (LPARAM)drive );
				SendDlgItemMessage( IDC_COMBO_LETTER, CB_SETITEMDATA, 
					SendDlgItemMessage(IDC_COMBO_LETTER, CB_GETCOUNT, 0, 0 )-1
					, i );										
			}			

			if (_name == drive)
				sel = SendDlgItemMessage(IDC_COMBO_LETTER, CB_GETCOUNT, 0, 0) -1;		

	}

	SendDlgItemMessage( IDC_COMBO_LETTER, CB_SETCURSEL, sel, 0L );		
	
	if (_fs=="FAT32")
		SendDlgItemMessage( IDC_COMBO_FAT, CB_SETCURSEL, 1, 0L ); // FAT32
	if (_fs=="NTFS")
		SendDlgItemMessage( IDC_COMBO_FAT, CB_SETCURSEL, 2, 0L ); // FAT32
	if (_fs=="FAT")
		SendDlgItemMessage( IDC_COMBO_FAT, CB_SETCURSEL, 0, 0L ); // FAT32


	SendDlgItemMessage( IDC_COMBO_ALGO, CB_SETCURSEL, 0, 0L ); // AES 256
	

	_was_shown = true;

	//CreateToolTip(m_hWnd, IDC_HELP1, LS(IDS_SHOW_HELP_CREATE_DISK), 100);

	
	UpdateData(false);


	// TODO:  Add extra initialization here

	return TRUE;  // return TRUE unless you set the focus to a control
	// EXCEPTION: OCX Property Pages should return FALSE
}
