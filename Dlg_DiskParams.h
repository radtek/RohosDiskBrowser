#pragma once

#include "CDiskRepartition.h"

// CDlg_DiskParams dialog

class CDlg_DiskParams : public CDialog
{
	DECLARE_DYNAMIC(CDlg_DiskParams)

	CFileDialog *myFileDialog;

public:
	TCHAR _regkey[350];
	void*	_parent;			///< an instance of CDiskPage (for threading purposes (v.1.11))
	CString _password;				///< óñòàíàâëèâàåò òîò êòî âûçûâàåò ýòî äèàëîã (HTML îêíî ..)
	void SetDefaultValues();
	void SetDefaultValues_Partition();
	CString _sizemb;
	CString _ImagePathForDisplay;	///< óäîáî÷èòàåìûé ïóòü äî Image ôàéëà .
	CString _name;					///< èìÿ äèñêà
	CString _fs;					///< ôàéëîâàÿ ñèñòåìà
	CString _algoritm;				///< àãîðèòì øèôðîâàíèÿ

	CString	_encrypt_USB_drive;			/// çàäàåòñÿ êëèåíòîì
	ULARGE_INTEGER _DiskSizeInBytes; // íîâûé ðàçìåð äèñêà â áàéòàõ. äëÿ CDisk::CreateDiskEx() 

	CDlg_DiskParams(CWnd* pParent = NULL);   // standard constructor
	virtual ~CDlg_DiskParams();
	bool _was_shown;

	bool _partition_based;			// using repartition of drive at this->_path
	CDiskRepartition _theDiskPartition;
	

// Dialog Data
	enum { IDD = IDD_DISK_CREATE2 };

	//{{AFX_DATA(CDlg_DiskParams)	
	UINT	_size;
	CString	_path;
	CString	_label;
	CString	m_disk_info;
	//}}AFX_DATA


protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support


	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
public:
	afx_msg void OnBnClickedOk2();
public:
//	afx_msg void OnEnChangeEdit1();
public:
	
	
public:
	virtual BOOL OnInitDialog();
};
