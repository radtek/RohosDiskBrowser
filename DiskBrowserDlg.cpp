// CDiskBrowserApp.cpp : implementation file
//

#include "pub.h"
#include "DiskBrowserApp.h"
#include "DiskBrowserDlg.h"
#include "TrueColorToolBar.h"

#include "Dlg_Password.h"
#include "Dlg_DiskInfo.h"
#include "Dlg_InfoNoghi.h"
#include "Dlg_About.h"
#include "Dlg_FilesChanged.h" 
#include "Dlg_CopyFiles.h"
#include "Dlg_NewDisk1.h"
#include "Dlg_CheckDsk.h"
#include "Dlg_ChangePassw.h"

#include "CDiskRepartition.h"
#include "common1.h"


#include <windows.h>
#include <direct.h>

#include <stdio.h>
#include <io.h>



#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static PATH_TOKEN	PT_TOK={0};


CImageList			ff_img;
CStatusBarCtrl		ff_stat;
CTrueColorToolBar	ff_tb;/*CToolBar*/
CWnd				*m_wnd=NULL;
HACCEL				h_acl=NULL;

extern char Key[32];
extern char IV[64];
extern int encryption_mode ;
extern CDiskRepartition *theDiskPartition;

TCHAR CDlg_DiskBrowserMain::_lastVirtualFolder_Path[1000] = {0};
HWND _hwnd_ShowFileDialog = 0;



/* Keep track of Folders Navigation changes.
*/
class CBackForwardList 
{
	

	CStringArray _back, _forward;
	
public:
	HMENU _hmenu_back, _hmenu_forward;

	// return last path, if menu_id =0 then the latest item; 
	// remove items from the  Back list.
	//  adds a items to Forward list
	LPCTSTR goBack(LPCTSTR current_path, int menu_id =-1) 
	{
		static TCHAR path[500] = {"/"};
		int c = _back.GetCount();

		if (c==0)
			return NULL;

		// add current path to Forward list		
		_forward.InsertAt(0, current_path);		

		if (menu_id == -1)
		{
			CString last_path = _back[0];
			_back.RemoveAt(0);
			_tcscpy(path, last_path);			
		}
		
		rebuidMenus();
		return path;

	}

	// return the first path from Forward list, if menu_id =0 then the latest item;
	// removes items from the Forward list
	// adds items to the Back list.
	LPCTSTR goForward(LPCTSTR current_path, int menu_id =-1)
	{
		static TCHAR path[500] = {"/"};
		int c = _forward.GetCount();

		if (c==0)
			return NULL;

		// add current path to Forward list		
		_back.InsertAt(0, current_path);		

		if (menu_id == -1)
		{
			CString last_path = _forward[0];
			_forward.RemoveAt(0);
			_tcscpy(path, last_path);			
		}
		
		rebuidMenus();
		return path;

	}

	CBackForwardList()
	{
		_hmenu_back = CreateMenu();
		_hmenu_forward = CreateMenu();

	}

	void rebuidMenus()
	{
		// erase and rebuild Back menu		
		::DeleteMenu( _hmenu_back, 0, MF_BYPOSITION);
		HMENU hmenu_1 = CreatePopupMenu();

		for (int x=0; x<_back.GetCount(); x++)
		{
			int i= _back[x].ReverseFind(PATH_SEP);
			if (i<0) i=0; 
			CString s = _back[x].Mid(i);
			 AppendMenu( hmenu_1, MF_ENABLED | MF_STRING, 1600+x, s);
		}
		AppendMenu(_hmenu_back, MF_STRING | MF_POPUP, (UINT) hmenu_1,         "1"); 


		// erase and rebuild Forward menu		
		::DeleteMenu( _hmenu_forward, 0, MF_BYPOSITION);
		hmenu_1 = CreatePopupMenu(); 

		for (int x=0; x<_forward.GetCount(); x++)
		{
			int i= _forward[x].ReverseFind(PATH_SEP);// finds "\lastfolder" or "root" 
			if (i<0) i=0; //else i+=1;
			CString s = _forward[x].Mid(i);
			 AppendMenu( hmenu_1, MF_ENABLED | MF_STRING, 1500+x, s); 
		}
		AppendMenu(_hmenu_forward, MF_STRING | MF_POPUP, (UINT) hmenu_1,         "1"); 		

	}

	/*
	 add an item to the end of Back list, 
	 clear Forward list;
	 make menu again
	*/
	void addRecentItem(LPCTSTR path) 
	{

		//WriteLog("Add item %s",path );
		_back.InsertAt(0, path);
		_forward.RemoveAll();

		rebuidMenus();
		

	}
};

CBackForwardList theBackForwadList;



/////////////////////////////////////////////////////////////////////////////
// CDlg_DiskBrowserMain dialog

CDlg_DiskBrowserMain::CDlg_DiskBrowserMain(CWnd* pParent /*=NULL*/)
	: CDialog(CDlg_DiskBrowserMain::IDD, pParent)
{
	
	//{{AFX_DATA_INIT(CDlg_DiskBrowserMain)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	pWorkerThread1 = NULL;
	blob.pbData = 0;
	blob.cbData =0;
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

}

void CDlg_DiskBrowserMain::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlg_DiskBrowserMain)
	
	DDX_Control(pDX, IDC_TREE, ff_tree);
	DDX_Control(pDX, IDC_LIST, ff_list);
	DDX_Control(pDX, IDC_PATH, cbox_FavoritesList);
	//}}AFX_DATA_MAP
	
	//DDX_Control(pDX, IDC_RESIZER, m_TreeControl);
}

BEGIN_MESSAGE_MAP(CDlg_DiskBrowserMain, CDialog)
	//{{AFX_MSG_MAP(CDlg_DiskBrowserMain)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_NOTIFY(NM_DBLCLK, IDC_LIST, OnDblclkList)
	ON_NOTIFY(TVN_SELCHANGED, IDC_TREE, OnSelchangedTree)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_NOTIFY(NM_RCLICK, IDC_LIST, OnRclickList)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST, OnItemchangedList)
	ON_NOTIFY(NM_CLICK, IDC_TREE, OnClickTree)
	ON_NOTIFY(NM_RDBLCLK, IDC_LIST, OnDoubleClickLV)
	ON_WM_TIMER()
	ON_WM_DROPFILES()
	
	ON_UPDATE_COMMAND_UI(ID_PARTITION_INFORMATION, &CDlg_DiskBrowserMain::OnUpdateMenu)
	ON_UPDATE_COMMAND_UI(ID_PARTITION_REPAIR, &CDlg_DiskBrowserMain::OnUpdateMenu)
	ON_UPDATE_COMMAND_UI(ID_PARTITION_RESIZE, &CDlg_DiskBrowserMain::OnUpdateMenu)
	ON_UPDATE_COMMAND_UI(ID_PARTITION_CHANGEPASSWORD, &CDlg_DiskBrowserMain::OnUpdateMenu)

	
	//}}AFX_MSG_MAP
	ON_NOTIFY(LVN_BEGINDRAG, IDC_LIST, &CDlg_DiskBrowserMain::OnLvnBegindragList)	
	ON_COMMAND(ID_PARTITION_NEW, &CDlg_DiskBrowserMain::OnPartitionNew)
	//ON_COMMAND(ID_PARTITION_OPEN, &CDlg_DiskBrowserMain::OnOpenDisk)
	ON_WM_INITMENUPOPUP()

	ON_UPDATE_COMMAND_UI(ID_FILE_OPENWITH, &CDlg_DiskBrowserMain::OnUpdateMenuFile)
	ON_UPDATE_COMMAND_UI(ID_OPEN_SIMPLEOPEN, &CDlg_DiskBrowserMain::OnUpdateMenuFile)
	ON_UPDATE_COMMAND_UI(ID_FILE_PREVIEW, &CDlg_DiskBrowserMain::OnUpdateMenuFile)
	ON_UPDATE_COMMAND_UI(ID_FILE_EXPORT, &CDlg_DiskBrowserMain::OnUpdateMenuFile)
	ON_UPDATE_COMMAND_UI(ID_FILE_DELETE, &CDlg_DiskBrowserMain::OnUpdateMenuFile)
	ON_UPDATE_COMMAND_UI(ID_FILE_RENAME, &CDlg_DiskBrowserMain::OnUpdateMenuFile)
	ON_UPDATE_COMMAND_UI(ID_FILE_PROPERTIES, &CDlg_DiskBrowserMain::OnUpdateMenuFile)
	

	ON_UPDATE_COMMAND_UI(ID_A_NEWFOLDER, &CDlg_DiskBrowserMain::OnUpdateMenuFile)
	

	ON_COMMAND(ID__WEB, &CDlg_DiskBrowserMain::OnCheckUpdates)
	ON_COMMAND(ID_A_VIRTUALFOLDER, &CDlg_DiskBrowserMain::OnAVirtualfolder)
	ON_COMMAND(ID_PARTITION_REPAIR, &CDlg_DiskBrowserMain::OnPartitionRepair)
	ON_COMMAND(ID_PARTITION_CHANGEPASSWORD, &CDlg_DiskBrowserMain::OnChangePassword)
	ON_NOTIFY(LVN_ENDLABELEDIT, IDC_LIST, &CDlg_DiskBrowserMain::OnLvnEndlabeleditList)
//	ON_NOTIFY(LVN_BEGINLABELEDIT, IDC_LIST, &CDlg_DiskBrowserMain::OnLvnBeginlabeleditList)
ON_COMMAND(ID_FILE_RENAME, &CDlg_DiskBrowserMain::OnFileRename)
ON_COMMAND(ID_A_NEWFOLDER, &CDlg_DiskBrowserMain::OnNewfolder)
ON_NOTIFY(LVN_KEYDOWN, IDC_LIST, &CDlg_DiskBrowserMain::OnLvnKeydownList)
ON_WM_KEYDOWN()
ON_COMMAND(ID_OPEN_OPENANDPREVENDDATALEAK, &CDlg_DiskBrowserMain::OnOpenAndPreventDataLeak)
ON_COMMAND(ID_OPEN_OPENANDVIRTUALIZEFOLDERTREE, &CDlg_DiskBrowserMain::OnOpenandVirtualizeFoldertree)

ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolBarToolTipText)
ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolBarToolTipText)

ON_BN_CLICKED(IDC_BUTTON_ADD_FAVORITES, &CDlg_DiskBrowserMain::OnBnClickedButtonAddFavorites)
ON_CBN_SELCHANGE(IDC_PATH, &CDlg_DiskBrowserMain::OnCbnSelchangePath)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlg_DiskBrowserMain message handlers
void attach_menu(HMENU hmnu)
{
	::SetMenu(m_wnd->m_hWnd,hmnu);
}
/////////////////////////////////////////////////////////////////////////////
BOOL CDlg_DiskBrowserMain::OnInitDialog()
{


	

	HWND hw,hw1;
	DWORD tmp;
	//WriteLog("22-0");	
	pDiskBlob = 0;

	CDialog::OnInitDialog();

	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	contextMenu2.LoadMenuA(IDR_MENU2);
	// TODO: Add extra initialization here
	//start_log();

	
	m_wnd = this;
	hw=0;

	//WriteLog("22");	
	
		
	_tcscpy(_OpenedDiskName,"");
	_ImageOpened=false;// Изначально образ не является открытым
	_EvaluationStatus=0;// инициализация 

	//this->GetDlgItem(IDC_TREE,&hw);
	//ff_tree.Attach(hw);

	//this->GetDlgItem(IDC_LIST,&hw1);
	//ff_list.Attach(hw1);

	//ctrls_init(ff_tree,ff_list,ff_img,ff_tb,m_wnd);

	{
		LVCOLUMN lvc;
	HWND lv;
	int i_mas[]={3,4,0,2};
	HICON hi;
		SHFILEINFO sfi;

		TCHAR winfolder[500];
		GetWindowsDirectory(winfolder, 490);
	
	

		// get folder and disk icons
		DWORD ImageHandle = SHGetFileInfo(winfolder,
		0,
		&sfi,
		sizeof(sfi),
		SHGFI_SMALLICON |
		SHGFI_ICON |
		SHGFI_SYSICONINDEX);

	if (ImageHandle != 0)
	{
		ff_img.m_hImageList= (HIMAGELIST)ImageHandle;
	}	

	ExtractIconEx("shell32.dll", 4 , NULL, &hi, 1);

	 HICON di;
	ExtractIconEx("shell32.dll", 8 , NULL, &di, 1);


	ff_img.Add(di);
	ff_img.Add(hi);
	
	ff_list.SetImageList(&ff_img, LVSIL_SMALL); 
	ff_list.SetImageList(&ff_img,LVSIL_NORMAL);

	ff_tree.SetImageList(&ff_img, LVSIL_NORMAL);
	}

	set_lr_wnd(ff_tree.m_hWnd, ff_list.m_hWnd);

	ff_stat.Create(WS_CHILD | WS_VISIBLE,CRect(0,0,0,0),this,10234);
	

	settings_ls(&SETT,0);
	
	tmp = GetWindowLong(ff_list.m_hWnd, GWL_STYLE);
	tmp&=~(LVS_ICON|LVS_LIST);
	tmp|=SETT.lst_view;
	SetWindowLong(ff_list.m_hWnd,GWL_STYLE,tmp);
	ff_list.Arrange(LVA_ALIGNTOP);

	// columns

	ff_list.InsertColumn(0, "Name", 0, 200 );
	ff_list.InsertColumn(1, "Size", 0, 50 );
	ff_list.InsertColumn(2, "Type", 0, 60 );
	ff_list.InsertColumn(3, "Change Date", 0, 120 );
	
	
	// Если не удалось загрузить данный файл то выдаю сообщение об ошибке
	TCHAR Fullp[1000];
	// Добывает папку из которой запускается программа	
	GetPodNogamiPath(Fullp,0/*1 - с названием файла; 0 - без*/ );
	// добавляет в конец её -> имя файла языка

	_tcscpy(Fullp, SETT.def_lang/* ".\\settings\\language\\eng.lng"*/);
//	WriteLog("Fullp=%s; ",Fullp);

	// try to locate language file from HKLM\\Software\\Rohos
	TCHAR path[500], lang[100];	
	ReadReg(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Rohos"), TEXT("RohosPath"), path, 300);
	
	/*if ( _tcslen(path) ==0) 
	{
		// диретории Rohos не найдено, это Rohos Mini 
		GetPodNogamiPath(path, false);		
		char *c = strchr(SETT.def_lang, '\\');
		if (c )			
			strcpy(lang, c+1);
		else 
			strcpy(lang, SETT.def_lang);

		_tcscat(path,TEXT("settings\\") );
		_tcscat(path, lang);

	} else
	{		
		_tcscat(path,TEXT("settings\\language\\") );
		ReadReg(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Rohos"), "language", lang, 30);
		
		_tcscat(path, lang);
		_tcscat(path, TEXT(".txt") );

		_tcscpy(SETT.def_lang, path); // сохраняем выбор 
	}

	if ( _taccess(path, 0) ==0 ) { // такой файл есть. загружаем
		
		_tcscpy(Fullp, path);
	}
	WriteLog("langfile %s", path);
*/
	
	/*if( !load_lang_file(Fullp) )
	{ 
		if( !load_lang_file(".\\settings\\language\\english.txt") )
		{ 	
			WriteDefaultLangFile();

			GetPodNogamiPath(Fullp,0 );	
			_tcscat(Fullp,"\\english.txt");

			if( !load_lang_file(Fullp) )
			{ 	
				
				CString str;
				str="The file: \"";
				str=str+Fullp;
				str=str+"\" not found or broken.";
				
				MessageBoxEx(m_hWnd,str, "Rohos Disk Browser",
					MB_ICONWARNING,0		  		);
				//PostQuitMessage(0);// И выхожу из программы
			}
		}
		
	}
	*/

	
	// Load and Localize Menu 
	::SetMenu(m_hWnd, LoadMenu(NULL, (LPCTSTR)IDR_MENU1)) ;
	
	CMenu * menu = GetMenu();

	menu->ModifyMenu(0, MF_BYPOSITION | MF_STRING, 	0, LS(IDS_PARTITION));
	menu->ModifyMenu(1, MF_BYPOSITION | MF_STRING, 	0, LS(IDS_FILE));
	menu->ModifyMenu(2, MF_BYPOSITION | MF_STRING, 	0, LS(IDS_VIEW));
	


	CMenu * pPopup = menu->GetSubMenu(0); // PARTITION
	pPopup->ModifyMenu(ID_PARTITION_NEW, MF_BYCOMMAND | MF_STRING, 			ID_PARTITION_NEW, LS(IDS_CREATE));
	pPopup->ModifyMenu(ID_PARTITION_OPEN, MF_BYCOMMAND | MF_STRING,			ID_PARTITION_OPEN, LS(IDS_OPEN)); 
	pPopup->ModifyMenu(ID_PARTITION_INFORMATION, MF_BYCOMMAND | MF_STRING, 	ID_PARTITION_INFORMATION, LS(IDS_PROPERTIES));
	pPopup->ModifyMenu(ID_PARTITION_EXIT, MF_BYCOMMAND | MF_STRING, 		ID_PARTITION_EXIT, LS(IDS_EXIT));
	pPopup->ModifyMenu(ID_PARTITION_CHANGEPASSWORD, MF_BYCOMMAND | MF_STRING, 		ID_PARTITION_CHANGEPASSWORD, LS(IDS_B_CHANGE_PSWD));
	pPopup->ModifyMenu(ID_PARTITION_RESIZE, MF_BYCOMMAND | MF_STRING, 		ID_PARTITION_RESIZE, LS(IDS_RESIZE));
	pPopup->ModifyMenu(ID_PARTITION_REPAIR, MF_BYCOMMAND | MF_STRING, 		ID_PARTITION_REPAIR, LS(IDS_REPAIR));


	pPopup = menu->GetSubMenu(1); // FILE
	pPopup->ModifyMenu(ID_FILE_PREVIEW, MF_BYCOMMAND | MF_STRING, 			ID_FILE_PREVIEW, LS(IDS_VIEW));
	pPopup->ModifyMenu(1, MF_BYPOSITION | MF_STRING,			1, LS(IDS_OPEN)); 
	pPopup->ModifyMenu(ID_FILE_EXPORT, MF_BYCOMMAND | MF_STRING, 	ID_FILE_EXPORT, LS(IDS_EXPORT));
	pPopup->ModifyMenu(ID_FILE_DELETE, MF_BYCOMMAND | MF_STRING, 		ID_FILE_DELETE, LS(IDS_DELETE));
	pPopup->ModifyMenu(ID_FILE_RENAME, MF_BYCOMMAND | MF_STRING, 		ID_FILE_RENAME, LS(IDS_RENAME));
	pPopup->ModifyMenu(ID_FILE_PROPERTIES, MF_BYCOMMAND | MF_STRING, 		ID_FILE_PROPERTIES, LS(IDS_PROPERTIES));

	//pPopup->ModifyMenu(ID_FILE_OPENWITH, MF_BYCOMMAND | MF_STRING, 		ID_FILE_OPENWITH, LS(IDS_OPEN_WITH));

	pPopup = menu->GetSubMenu(2); // VIEW
	pPopup->ModifyMenu(ID_VIEW_LIST, MF_BYCOMMAND | MF_STRING, 			ID_VIEW_LIST, LS(IDS_LIST));
	pPopup->ModifyMenu(ID_VIEW_ICON, MF_BYCOMMAND | MF_STRING,			ID_VIEW_ICON, LS(IDS_ICON)); 
	pPopup->ModifyMenu(ID_VIEW_SETTINGS, MF_BYCOMMAND | MF_STRING, 	ID_VIEW_SETTINGS, LS(IDS_SETTINGS));

	pPopup = menu->GetSubMenu(3); // about
	pPopup->ModifyMenu(ID__ABOUT, MF_BYCOMMAND | MF_STRING, 	ID__ABOUT, LS(IDS_ABOUT_DLG) );	

	
	this->GetDlgItem(IDC_RESIZER,&hw);
	if ( SETT.x_resz < 20 || SETT.x_resz > 500 ) SETT.x_resz = 320;
	init_resizer(hw, SETT.x_resz);
	
	// Копируегт аргумент ком. стр. в глобалную переменную
	_cmdline_image=AfxGetApp()->m_lpCmdLine;
	
	CString dir=AfxGetApp()->m_lpCmdLine;

	_detect_traveler_mode();


	if(dir.GetLength()>0/*Если есть аргументы в командной строке*/) 
	{
		WriteLog("Timer1 Started......");
		SetTimer(1, 250, NULL);// через 500 мс вызываю OnOpendisk()
	} else
	{
		if (_traveler_mode)
			SetTimer(1, 250, NULL);// через 500 мс вызываю OnOpendisk()
	}

	// tooltips 
	CreateToolTip( m_hWnd, IDC_BUTTON_ADD_FAVORITES, "Add to favorites", 200);  
	
	
	return TRUE;  
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.
/////////////////////////////////////////////////////////////////////////////
void CDlg_DiskBrowserMain::OnPaint() 
{
	if (IsIconic())
	{

		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}
/////////////////////////////////////////////////////////////////////////////
HCURSOR CDlg_DiskBrowserMain::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}


/////////////////////////////////////////////////////////////////////////////
// Double Click левой кнопкой мыши на LIST VIEW
/////////////////////////////////////////////////////////////////////////////
void CDlg_DiskBrowserMain::OnDblclkList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// when user edit list item name by F2 - this may trigger OnDblclkList(null, Null);
	//if (!pNMHDR)
		//return; 
	
	//if (pNMHDR)
	//	WriteLog("CDlg_DiskBrowserMain::OnDblclkList- %d", pNMHDR->code);
		
	
	int n;
	char buf[1024]={0};

//	*pResult = 0;
	n = ff_list.GetNextItem(-1, LVNI_SELECTED);
	if(n==-1)
		return;

	ff_list.GetItemText(n,0,buf,1024);

	path_down(&PT_TOK,buf);
	switch(_FSys.isDirectory(PT_TOK.pth))
	{
	case 1: // folder

		TCHAR txt[750];
		GetDlgItemText(IDC_PATH, txt, 700);
		if (_tcslen(txt)==0)
			_tcscpy(txt,PATH_SEP1);
		theBackForwadList.addRecentItem(txt); // add a folder name that we are going to leave. 


		load_list_files(PT_TOK.pth,ff_tree,ff_list,ff_img);
		this->SetDlgItemText(IDC_PATH,PT_TOK.pth);
		make_folder_tree(&PT_TOK,ff_tree);
		break;

	case 0:			
		WriteLog("Opening file.");

		AfxGetApp()->BeginWaitCursor();
		_FSys.ShellExecute(PT_TOK.pth);
		path_up(&PT_TOK);
		AfxGetApp()->EndWaitCursor();


		break;

	case 3: // error
		path_up(&PT_TOK);
		load_list_files(PT_TOK.pth,ff_tree,ff_list,ff_img);
		break;
	}

	
}
//--------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////
void CDlg_DiskBrowserMain::OnSelchangedTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	NM_TREEVIEW* tviv = (NM_TREEVIEW*)pNMHDR;
	/*
	TVITEM *tvi;
	HTREEITEM hti=0;
	DWORD sz,asz;
	char buf[MAX_PATH]={0};
	char bf[1024]={0};
 	
	tvi  = (TVITEM*)&tviv->itemNew;
	tvi->mask = TVIF_TEXT | TVIF_HANDLE;
	tvi->pszText = bf;
	tvi->cchTextMax = 1024;
	hti = tvi->hItem;
	
	asz = 0;
	
	if(ff_tree.GetParentItem(hti))
	{
		// выбрал подпапку 

		// пройтись по дереву и построить полный путь... 
		while(hti)
		{
			tvi->hItem = hti;
			ff_tree.GetItem(tvi);
			sz = strlen(bf);
			MoveMemory(buf+sz+1,buf,asz);
			asz+=sz+1;
			buf[sz]=PATH_SEP;
			CopyMemory(buf,bf,sz);
			hti = ff_tree.GetParentItem(hti);
		}
		
		TCHAR txt[750];
		GetDlgItemText(IDC_PATH, txt, 700);
		if (_tcslen(txt)==0)
			_tcscpy(txt, "/");
		theBackForwadList.addRecentItem(txt); // add a folder name that we are going to leave. 

		strpath_to_token(buf,&PT_TOK);  //создать токен 
		load_list_files(PT_TOK.pth,ff_tree,ff_list,ff_img); // загрузить список файлов  

		SetDlgItemText(IDC_PATH, PT_TOK.pth);
		make_folder_tree(&PT_TOK, ff_tree);		// загрузить дерево		
		
	}else
	{
		// выбрал корень 
		
		free_path_token(&PT_TOK); // удалить содержимое токена 
		
		path_down(&PT_TOK,"/"); // дополнить токен 
		path_up(&PT_TOK);		// убрать последн токен

		load_list_files("/",ff_tree,ff_list,ff_img); // загрузить список файлов 
		this->SetDlgItemText(IDC_PATH,"");		
		make_folder_tree(&PT_TOK,ff_tree);		// загрузить дерево
	}
	//::MessageBox(0,buf,"",0);
	*/
	*pResult = 0;

}
/////////////////////////////////////////////////////////////////////////////
int CDlg_DiskBrowserMain::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CDialog::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	UINT tbi[7]={TB_BT_UP, TB_BT_LOAD, TB_BT_SAVE, TB_BT_VIEW1 /*TB_BT_BROWSE*/, TB_BT_BACK, TB_BT_FORWARD, TB_BT_UP2};

	ff_tb.CreateEx(this, TBSTYLE_FLAT, CBRS_FLYBY | WS_CHILD | CBRS_GRIPPER | CBRS_TOOLTIPS | WS_VISIBLE | CBRS_TOP | CBRS_SIZE_DYNAMIC | CBRS_FLOATING);//,CRect(0,0,32,32));
	
	
	ff_tb.LoadToolBar(IDR_TOOLBAR1);
	//ff_tb.SetSizes(tsz,tsz);
	ff_tb.SetButtons(tbi,7);
	//ff_tb.SetHeight(64);

	// drop down buttons
	ff_tb.AddDropDownButton(this, TB_BT_BACK, theBackForwadList._hmenu_back);
	ff_tb.AddDropDownButton(this, TB_BT_FORWARD, theBackForwadList._hmenu_forward);

	HMENU _hmenu =  CreateMenu();	// menu for View button
	HMENU _hmenu_pop =  CreatePopupMenu();
	 AppendMenu(_hmenu, MF_STRING | MF_POPUP, (UINT) _hmenu_pop,         "1"); 
	 AppendMenu( _hmenu_pop, MF_ENABLED | MF_STRING, ID_VIEW_LIST, LS(IDS_LIST));
	 AppendMenu( _hmenu_pop, MF_ENABLED | MF_STRING, ID_VIEW_ICON, LS(IDS_ICON));
	 AppendMenu( _hmenu_pop, MF_ENABLED | MF_STRING, ID_VIEW_TABLE, "Table");
	ff_tb.AddDropDownButton(this, TB_BT_VIEW1, _hmenu );

	// disable buttons - there is no opened disk
	ff_tb.GetToolBarCtrl().EnableButton(TB_BT_BACK, false);
	ff_tb.GetToolBarCtrl().EnableButton(TB_BT_FORWARD, false);
	ff_tb.GetToolBarCtrl().EnableButton(TB_BT_UP2, false);
	
	
	///ff_tb.EnableDocking(CBRS_ALIGN_ANY);
	
	ff_tb.LoadTrueColorToolBar(32,IDB_BITMAP1,IDB_BITMAP1, IDB_BITMAP1);
	ff_tb.ShowWindow(SW_SHOW);
	ff_tb.SetWindowPos(0,0,0,164,48,0);	
	return 0;
}
//--------------------------------------------------------------------------
/////////////////////////////////////////////////////////////////////////////
BOOL CDlg_DiskBrowserMain::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	//WriteLog("<<<CDlg_DiskBrowserMain::OnCommand(w:%d l:%d)", LOWORD(wParam), lParam );

	static char tx[2048];
	static char *buf;
	static int n;
	static char *img_pth=NULL;
	static DWORD tmp;
	static PATH_TOKEN l_tok={0};
	static u64 fsz;
	static HANDLE h_thread;
	LRESULT nr=77;// ниже будет вызов: OnDblclkList(NULL,&nr)
	
	switch(LOWORD(wParam))
	{
	case IDOK:
		// open selected path...

		if (lParam==0) // this is simple pressing VK_ENTER
			OnDblclkList(NULL, NULL);

		break;

	case TB_BT_UP:
		_cmdline_image="";
		OnOpenDisk(); // Диалог открытия образа
		
		break;		

		// user press Back toolbar button - navigate to last seen Folder (theBackForwadList)
	case TB_BT_BACK:
		if(_ImageOpened)
		{

			GetDlgItemText(IDC_PATH,tx,2048);
			//if (strchr(tx,PATH_SEP )

			LPCTSTR last_seen_folder = theBackForwadList.goBack(tx);
			strpath_to_token((char*)last_seen_folder,&PT_TOK);			

			switch(_FSys.isDirectory(PT_TOK.pth))
			{
			case 1:
				load_list_files(PT_TOK.pth, ff_tree, ff_list, ff_img);
				select_folder_tree(&PT_TOK, ff_tree);
				break;			
			}
			
			SetDlgItemText(IDC_PATH, PT_TOK.pth);
			break;
			
		}
		break;

	// user press Back toolbar button - navigate to last seen Folder (theBackForwadList)
	case TB_BT_FORWARD:
		if(_ImageOpened)
		{
			

			GetDlgItemText(IDC_PATH,tx,2048);
			//if (strchr(tx,PATH_SEP )

			LPCTSTR last_left_folder = theBackForwadList.goForward(tx);
			strpath_to_token((char*)last_left_folder,&PT_TOK);			

			switch(_FSys.isDirectory(PT_TOK.pth))
			{
			case 1:
				load_list_files(PT_TOK.pth, ff_tree, ff_list, ff_img);
				select_folder_tree(&PT_TOK, ff_tree);
				break;			
			}
			
			SetDlgItemText(IDC_PATH, PT_TOK.pth);
			break;
			
		}
		break;

		// user press UP tollbar button - navigate uppper level folder.
	case TB_BT_UP2:
		if(_ImageOpened)
		{
			GetDlgItemText(IDC_PATH,tx,2048);
			//if (strchr(tx,PATH_SEP )
			strpath_to_token(tx,&PT_TOK);
			path_up(&PT_TOK);

			switch(_FSys.isDirectory(PT_TOK.pth))
			{
			case 1:
				load_list_files(PT_TOK.pth, ff_tree, ff_list, ff_img);
				select_folder_tree(&PT_TOK, ff_tree);
				break;			
			}
			
			SetDlgItemText(IDC_PATH, PT_TOK.pth);
			break;

		}
		break;

		// user clicked on Go button after entered a path
		// see OnOK()
	/*case TB_BT_BROWSE:
		if(_ImageOpened)
		{
			this->GetDlgItemText(IDC_PATH,tx,2048);
			strpath_to_token(tx,&PT_TOK);
			switch(_FSys.isDirectory(PT_TOK.pth))
			{
			case 1:
				load_list_files(tx, ff_tree, ff_list, ff_img);
				make_folder_tree(&PT_TOK, ff_tree);
				break;
			case 0:
				show_hex_dlg(m_wnd->m_hWnd, &PT_TOK);
				path_up(&PT_TOK);
				break;
			}
			
			this->SetDlgItemText(IDC_PATH, PT_TOK.pth);
			break;
		}*/
		
		case ID_PARTITION_INFORMATION:
			//if(_ImageOpened)
			{
				if(GetFileNameOfOpenedImage().GetLength()!=0)// Если образ диска открыт
				{
					onDiskInfo();
				}
			}
			break;
			
		case ID_PARTITION_OPEN:
			_cmdline_image="";
			OnOpenDisk();
			break;			
		
			
		case ID_FILE_PROPERTIES:
			show_prop_dlg(this->m_hWnd,&ff_list,&PT_TOK);
			break;

		case ID_OPEN_SIMPLEOPEN:	
			
			OnDblclkList(NULL,&nr);
			break;			

		case ID_FILE_OPENWITH:				
			OnOpenAs();
			break;			


		case TB_BT_SAVE:		
		case ID_FILE_EXPORT: 
			WriteLog("case MNU_ID+MNU_FILE_SAVE");				
			if(_ImageOpened)
			{
				if(GetFileNameOfOpenedImage().GetLength()!=0)// Если образ диска открыт
				{
					

					memset(tx,0,2000);
					get_folder(tx);
					n=-1;

					if ( strlen(tx) ==0)
						break;

					CDlg_CopyFiles dlg;

					dlg._path = tx;
					dlg._fsys = &_FSys;
					dlg._filesListOut.RemoveAll();
					TCHAR fpath[1500];					

					do
					{
						n = ff_list.GetNextItem(n,LVNI_SELECTED);
						if(n==-1)
							break;
						ff_list.GetItemText(n,0,tx,2000);

						GetDlgItemText(IDC_PATH, fpath, 500);
						strcat(fpath, PATH_SEP1);
						strcat(fpath, tx);

						dlg._filesListOut.Add(fpath);				
						
					}while(n!=-1);

					_FSys.Disable_VirtualIsolation();		
					dlg.DoModal();
					_FSys.Enable_VirtualIsolation();
				}
				else
				{
					WriteLog("Didn't enter.");
				}
			}
			break;
		/*case MNU_ID+MNU_SEARCH:// Главное меню -> Search     или
			if(_ImageOpened)
			{
				if(GetFileNameOfOpenedImage().GetLength()!=0)// Если образ диска открыт
				{
					show_search_dlg(this->m_hWnd,&PT_TOK);
					load_list_files(PT_TOK.pth,ff_tree,ff_list,ff_img);
					make_folder_tree(&PT_TOK,ff_tree);
					this->SetDlgItemText(IDC_PATH,PT_TOK.pth);
				}				
			}
			break;	
		case TB_BT_SEARCH:  // Кнопка Search
			OnNewDisk();
			break;*/

			// switch View mode
		case ID_VIEW_ICON:
			//ff_list.SendMessage(LVM_SETVIEW,LV_VIEW_ICON);
			tmp = GetWindowLong(ff_list.m_hWnd,GWL_STYLE);
			tmp&=~(LVS_LIST |LVS_REPORT);
			tmp|=LVS_ICON;
			SetWindowLong(ff_list.m_hWnd,GWL_STYLE,tmp);
			ff_list.Arrange(LVA_ALIGNTOP);
			SETT.lst_view = LVS_ICON;
			break;
			// switch View mode
		case ID_VIEW_LIST:
			tmp = GetWindowLong(ff_list.m_hWnd,GWL_STYLE);
			tmp&=~(LVS_ICON | LVS_REPORT);
			tmp|=LVS_LIST /*| LVS_SMALLICON*/ ;
			SetWindowLong(ff_list.m_hWnd,GWL_STYLE,tmp);
			ff_list.Arrange(LVA_ALIGNTOP);
			SETT.lst_view = LVS_LIST;
			break;

			// switch View mode
		case ID_VIEW_TABLE:
			tmp = GetWindowLong(ff_list.m_hWnd,GWL_STYLE);
			tmp&=~(LVS_ICON | LVS_LIST);
			tmp|=LVS_REPORT /*| LVS_SMALLICON*/ ;
			SetWindowLong(ff_list.m_hWnd,GWL_STYLE,tmp);
			ff_list.Arrange(LVA_ALIGNTOP);
			SETT.lst_view = LVS_REPORT;
			break;

			// user click on View button on toolbar. switch throug the view modes.
		case TB_BT_VIEW1:
			tmp = GetWindowLong(ff_list.m_hWnd,GWL_STYLE);
		
			if ( (tmp & LVS_LIST) == LVS_LIST)
			{
				OnCommand(ID_VIEW_TABLE, 0);
				
			} else
			if ( (tmp & LVS_REPORT) == LVS_REPORT)
			{
				OnCommand(ID_VIEW_ICON, 0);
				
			} else // (tmp & LVS_ICON)
			{
				OnCommand(ID_VIEW_LIST, 0);
				break;
			}

			break;

		/*case MNU_ID+MNU_HELP_CONT:
			 HtmlHelp2(m_hWnd ,"sdfsd"); //Вызов справки
			break;*/
		case ID__ABOUT:
			OnAbout();// вызов диалога - "О программе"
			break;

			// File Preview
		case ID_FILE_PREVIEW:
			n = ff_list.GetNextItem(-1,LVNI_SELECTED);
			if(n==-1)
				break;
			ff_list.GetItemText(n,0,tx,1024);
			
			path_down(&PT_TOK,tx);
			switch(/*ntfs_is_dir(PT_TOK.pth)*/_FSys.isDirectory(PT_TOK.pth))
			{
			case 1:
				load_list_files(PT_TOK.pth,ff_tree,ff_list,ff_img);
				this->SetDlgItemText(IDC_PATH,PT_TOK.pth);
				make_folder_tree(&PT_TOK,ff_tree);
				break;
			case 0:
				show_hex_dlg(m_wnd->m_hWnd,&PT_TOK);
				path_up(&PT_TOK);
				break;
			}
			break;
			
    	case ID_PARTITION_EXIT:
				_FSys.CloseVolume();
				PostMessage(WM_CLOSE);
				break;

		case ID_A_NEWFOLDER:
			//_FSys.CreateDirectory("new folder333");// !!!!!Пользователь должен выбирать имя папки			
			break;
			
		case TB_BT_LOAD:
		case ID_A_IMPORT:
			OnImportFile();
			break;

		case ID_VIEW_HIDDENFILES:
			//AfxMessageBox("Delete Selected.");
			if ( _FSys._showHiddenSystemFiles )
				_FSys._showHiddenSystemFiles = false; //!= _FSys._showHiddenSystemFiles;
			else 
				_FSys._showHiddenSystemFiles = true;

			free_path_token(&PT_TOK);
			
			path_down(&PT_TOK,"/");
			path_up(&PT_TOK);
			load_list_files("/",ff_tree,ff_list,ff_img);
			
			WriteLog("33 %s", img_pth);
			//getforderlist !!!
			{
				make_folder_tree(&PT_TOK,ff_tree, _OpenedDiskName);
			}
			
			
			this->SetDlgItemText(IDC_PATH, PT_TOK.pth); 
			
			break;
			
		case ID_FILE_DELETE:
			
				OnDeleteFile();
				break;
			
		
	}
	return CDialog::OnCommand(wParam, lParam);
}
/////////////////////////////////////////////////////////////////////////////
void CDlg_DiskBrowserMain::OnSize(UINT nType, int cx, int cy) 
{
	CDialog::OnSize(nType, cx, cy);
	HWND w;
	ff_stat.SetWindowPos(NULL,0,cy-20,cx,cy,0);
	ff_tb.SetWindowPos(NULL,0,0,cx,42,0);

	int x= 325;
	this->GetDlgItem(IDC_PATH)->SetWindowPos(0, x, 11, cx - (x + 4 + 30), 20,0);
	this->GetDlgItem(IDC_BUTTON_ADD_FAVORITES)->SetWindowPos(0, x + cx - (x + 4 + 28), 10, 0, 0, SWP_NOSIZE);
	this->GetDlgItem(IDC_RESIZER,&w);
	resize_wnd(w);
	ff_list.Arrange(LVA_ALIGNTOP);
} 
/////////////////////////////////////////////////////////////////////////////
// // user clicked ENTER after entered a path in Path edit box
void CDlg_DiskBrowserMain::OnOK() 
{
	char tx[1024];
	if(_ImageOpened)
		{
			GetDlgItemText(IDC_PATH,tx,1000);

			if (_tcslen(PT_TOK.pth))
				theBackForwadList.addRecentItem(PT_TOK.pth); // add a folder name that we are going to leave. 


			strpath_to_token(tx,&PT_TOK);
			switch(/*ntfs_is_dir(PT_TOK.pth)*/_FSys.isDirectory(PT_TOK.pth))
			{
			case 1:
				load_list_files(tx, ff_tree, ff_list, ff_img);
				make_folder_tree(&PT_TOK, ff_tree);
				break;
			case 0:
				show_hex_dlg(m_wnd->m_hWnd, &PT_TOK);
				path_up(&PT_TOK);
				break;
			default:
				AfxMessageBox(tx);
			}
			
			//this->SetDlgItemText(IDC_PATH, PT_TOK.pth);
			

		}

}
/////////////////////////////////////////////////////////////////////////////
//  Выход из программы
/////////////////////////////////////////////////////////////////////////////
void CDlg_DiskBrowserMain::OnCancel() 
{
	//WriteLog("CDlg_DiskBrowserMain::OnCancel()");

	AfxGetApp()->BeginWaitCursor();
	_FSys.CloseVolume();
	AfxGetApp()->EndWaitCursor();

	

	KillTimer(2);
	//if( PathFileExists(GetAplicationDirectory(0)) )// Провеояет наличие временной папки
	//{
	//	AskForDeleting();// вывести диалог о удалении
	//}
	//else
	//{
		 //// ничего не делать
	//}
	
	RECT rc;

	this->GetDlgItem(IDC_RESIZER)->GetWindowRect(&rc);
	this->ScreenToClient(&rc);
	SETT.x_resz = rc.left;
	this->GetWindowRect(&SETT.wnd_pos);
	settings_ls(&SETT,1);	
	CDialog::OnCancel();
}
/////////////////////////////////////////////////////////////////////////////
//  Правый клик на LISTVEW
void CDlg_DiskBrowserMain::OnRclickList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	POINT pt;
	GetCursorPos(&pt);// Получение координат курсора


	// Если открыт какойлибо образ 
	if(_ImageOpened)
	{// Можно октрывать первое меню
		  
		// Если курсор находится над какимто элементом
		if( GetLVItemByXY(pt)!=(-1) )
		{	
			
			// Открывает соответствующее меню:
			//  preview
			//  open
			//  save
			//  ---------
			//  properties
			GetMenu()->GetSubMenu(1)->TrackPopupMenu(0,pt.x,pt.y,m_wnd);
		}
		else// Если кликнули в пустом месте
		{ 
			// Можно открыть второе меню
			//  Import Files
			
			// Если образ открыт для записи
			// Можно открывать меню
			CMenu contextMenu;
			contextMenu.LoadMenuA(IDR_MENU2);
			CMenu *pmenu;
			pmenu = contextMenu.GetSubMenu(0);

			pmenu->ModifyMenu(ID_A_IMPORT, MF_BYCOMMAND | MF_STRING, 			ID_A_IMPORT, LS(IDS_IMPORT));
			pmenu->ModifyMenu(3, MF_BYPOSITION | MF_STRING, 	0, LS(IDS_VIEW));

			pmenu = pmenu->GetSubMenu(3);

			pmenu->ModifyMenu(ID_VIEW_LIST, MF_BYCOMMAND | MF_STRING, 			ID_VIEW_LIST, LS(IDS_LIST));
			pmenu->ModifyMenu(ID_VIEW_ICON, MF_BYCOMMAND | MF_STRING,			ID_VIEW_ICON, LS(IDS_ICON)); 

			pmenu = contextMenu.GetSubMenu(0);
			
			//pmenu->AppendMenu(MF_STRING, MNU_NEW_FOLDER,LS(MSG_NEW_FOLDER));
			//pmenu->AppendMenu(MF_STRING,MNU_IMPORT_FILES,LS(MSG_IMPORT_FILES));
			//pmenu->AppendMenu(MF_STRING,MNU_SHOW_HIDDEN_FILES,"Show Hidden Files" );

			if(_FSys._readOnly)
			{
				pmenu->EnableMenuItem(ID_A_IMPORT, 1);	
			}
			pmenu->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON, pt.x, pt.y, this);				
			
			
		}
	}
	
	*pResult = 0;
}
/////////////////////////////////////////////////////////////////////////////
void CDlg_DiskBrowserMain::OnItemchangedList(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* nmv = (NM_LISTVIEW*)pNMHDR;
	
	char buf[1024]={0}, bf[1024]={0};

	if ( ff_list.GetSelectedCount() >1) {

		POSITION pos = ff_list.GetFirstSelectedItemPosition();
		CString  s3;
		int nItem ;
		int nFiles  =0; 
		u64 totalSize= 0;

		TCHAR curr_vpath[500];		
		
		while (pos)
		{
			nItem = ff_list.GetNextSelectedItem(pos);
			ff_list.GetItemText(nItem,0,buf,1024);
			_tcscpy(curr_vpath, PT_TOK.pth);		
			_tcscat(curr_vpath, "/");	 
			_tcscat(curr_vpath, buf);		

			if ( !_FSys.isDirectory(curr_vpath))
				totalSize += _FSys.GetFileSize(curr_vpath);
			
			nFiles++;			
		}

		// X files selected

		TCHAR strSize[90]={"0 Kb."};
		StrFormatByteSize64( totalSize, strSize, 50);
		sprintf(buf, "%d Files. Size: %s", nFiles, strSize);
		ff_stat.SetText(buf,0,0);
		return;
	}
		
	
	ff_list.GetItemText(nmv->iItem,0,buf,1024); // получить имя выделенного файла 
	
	path_down(&PT_TOK, buf); // дополнить токен этим файлом

	switch(/*ntfs_is_dir(PT_TOK.pth)*/_FSys.isDirectory(PT_TOK.pth))
	{
		case 1:
			 // folder
		break;

		case 0:
			StrFormatByteSize64( _FSys.GetFileSize(PT_TOK.pth), bf, 50);
			_tcscat(buf, " ");
			_tcscat(buf, bf);
			
		break;

		default: // error
			_tcscat(buf, " ?");			
			break;

	}
	
	path_up(&PT_TOK);	//убрать последн токен
	ff_stat.SetText(buf,0,0);

	*pResult = 1;
}
/////////////////////////////////////////////////////////////////////////////
void CDlg_DiskBrowserMain::OnClickTree(NMHDR* pNMHDR, LRESULT* pResult) 
{
		NM_TREEVIEW* pNMTreeView = (NM_TREEVIEW*)pNMHDR;
	// TODO: Add your control notification handler code here
	NM_TREEVIEW* tviv = (NM_TREEVIEW*)pNMHDR;
	
	TVITEM tvi;
	HTREEITEM hti=0;
	
	char tvi_str[500]={0};
 	
	tvi.mask = TVIF_TEXT | TVIF_HANDLE;
	tvi.pszText = tvi_str;
	tvi.cchTextMax = 1024;
	
	TVHITTESTINFO htii = {0};
	POINT ptScreen;
	GetCursorPos(&ptScreen);
	htii.pt = ptScreen;
	ff_tree.ScreenToClient(&htii.pt);
	//do hit test on item
	if(ff_tree.HitTest(&htii) != NULL)
		hti = htii.hItem;
	
	if(ff_tree.GetParentItem(hti))
	{
		// выбрал подпапку 

		CString full_path;

		// пройтись по дереву и построить полный путь... 
		while(hti)
		{
			tvi.hItem = hti;
			ff_tree.GetItem(&tvi);
			
			if (strcmpi(tvi_str,PATH_SEP1) ) // doesnt include root.
			{
				full_path.Insert(0, tvi_str);
				full_path.Insert(0, PATH_SEP1);
			}

			hti = ff_tree.GetParentItem(hti);
		}
		
		TCHAR txt[750];
		GetDlgItemText(IDC_PATH, txt, 700);
		if (_tcslen(txt)==0)
			_tcscpy(txt,PATH_SEP1);
		theBackForwadList.addRecentItem(txt); // add a folder name that we are going to leave. 

		strpath_to_token((LPTSTR)(LPCTSTR)full_path,&PT_TOK);  //создать токен 
		load_list_files(PT_TOK.pth,ff_tree,ff_list,ff_img); // загрузить список файлов  

		SetDlgItemText(IDC_PATH, PT_TOK.pth);
		make_folder_tree(&PT_TOK, ff_tree);		// загрузить дерево		
		
		
	}else
	{
		// выбрал корень 
		
		free_path_token(&PT_TOK); // удалить содержимое токена 
		
		path_down(&PT_TOK,"/"); // дополнить токен 
		path_up(&PT_TOK);		// убрать последн токен

		load_list_files("/",ff_tree,ff_list,ff_img); // загрузить список файлов 
		this->SetDlgItemText(IDC_PATH,"");		
		make_folder_tree(&PT_TOK,ff_tree);		// загрузить дерево
	}
	//::MessageBox(0,buf,"",0);
	
	*pResult = 0;
	return;
}

// read a few blobs and check password for them, if match then apply blob - return TRUE
//  init encryption key here
//
bool CDlg_DiskBrowserMain::FindBlob_CheckPassword(LPCTSTR containerName, LPCTSTR password)
{
	
	// try read old style blob - on the last chunk of file or AVI files
	if ( !GetFileBlob_New(_OpenedDiskName) || !setBlob(file_blob, file_blob_size, password)) 
	{		
		if ( !GetFileBlob(_OpenedDiskName) || !setBlob(file_blob, file_blob_size, password)) 
		{		
			// try read backup file
			CString backup_path = containerName; 
			backup_path += ".rdi1";

			if ( !GetFileBlob(backup_path) ||  !setBlob(file_blob, file_blob_size, password)) 
			{
				return false;
			}
		}
	}
	

	WriteLog("OK: %s %s mode:%d", pKeyInfo->FileName,  pKeyInfo->AlgorithmName, pKeyInfo->Reserved1[0] );
	// 'A' + NUMBER 
	memcpy( IV, pKeyInfo->IV, 64 ); 
	memcpy( Key, pKeyInfo->Key, 32 );
	encryption_mode = pKeyInfo->Reserved1[0];

	return true;

}


/* в диалог запроса пароля добавить все образы диска которые можно испльзовать для подключения. 

*/
void CDlg_DiskBrowserMain::AddPossibleDiskImages(CDialog *dlg)
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;

	TCHAR usb_drive[300];
	GetPodNogamiPath(usb_drive, false, NULL);		

	if (GetDriveType(usb_drive) == DRIVE_REMOVABLE)
	{
		usb_drive[3]=0; // g:
		((CDlg_Password*)dlg)->AddPossibleDiskImage(usb_drive); // browse
	}

	
	
	TCHAR findPath[440], path2[440], fileName[500];
	
	_tcscpy(findPath, usb_drive);
	_tcscat(findPath, "_rohos");	

	if ( _taccess(findPath, 0) !=0 )
	{
		_tcscpy(findPath, usb_drive);
		//_tcscat(path, "*.*");	

	} else
		_tcscat(findPath, "\\");	

	_tcscpy(path2, findPath);
	_tcscat(findPath, "*.*");	

	//AfxMessageBox(_cmdline_image);

	_tcscpy(fileName, "");
	
	hFind = FindFirstFile(findPath, &FindFileData);

	//WriteLog("Finding file %s", path);

	if (hFind != INVALID_HANDLE_VALUE) 		
	{	    
		do {

			if ( strstr(FindFileData.cFileName, ".rdi") 
				&& (FindFileData.nFileSizeHigh > 1 || FindFileData.nFileSizeLow > 10 * 1024 * 1024) /* 100 Mb */) {
				_tcscpy(fileName, path2);				
				_tcscat(fileName, FindFileData.cFileName);
				if (_cmdline_image != fileName)
					((CDlg_Password*)dlg)->AddPossibleDiskImage(fileName);
			}	else

			

			if (  FindFileData.nFileSizeHigh > 1 || FindFileData.nFileSizeLow > 100 * 1024 * 1024 /* 100 Mb */) {
				_tcscpy(fileName, path2);				
				_tcscat(fileName, FindFileData.cFileName);
				if (_cmdline_image != fileName)
					((CDlg_Password*)dlg)->AddPossibleDiskImage(fileName);
			}	

		} while ( FindNextFile(hFind, &FindFileData) );		
	} 

	//((CDlg_Password*)dlg)->AddPossibleDiskImage("..."); // browse
	
	return;

}


/////////////////////////////////////////////////////////////////////////////
//	   on 	  File->Open													
/////////////////////////////////////////////////////////////////////////////

void CDlg_DiskBrowserMain::OnOpenDisk()
{
	WriteLog("CDlg_DiskBrowserMain::OnOpenDisk()");
	
	static char *img_pth=NULL;

	CDlg_Password dlg;// Зодание диалога воода пароля				
	dlg.m_FullPath=_OpenedDiskName;			
	AddPossibleDiskImages(&dlg);
	

	if(  _cmdline_image.GetLength()==0 && dlg._possible_disk_images_arr.GetSize() <= 0)// Если не указан конкретный образ
	{
		
		//////////////
		// открываю диалоговое окно выбора файлов
		/////////
		TCHAR def_path[500];
		GetPodNogamiPath(def_path, false );
		def_path[3]=0;
		_tcscat(def_path, "_rohos");
		if ( _taccess(def_path, 0) != 0 )
			def_path[0]=0;

		TCHAR filter[100];
		memset(filter,0, 90);
		memcpy(filter, LS(IDS_ALLFILES_FILTER), 70);
		LPTSTR s1 = strstr(filter, "\\x00");
		if (s1) 
		{ 
			*s1=0;
			strcat(filter, " *.*");
			*s1=0;
		}
		img_pth = file_dlg(filter, (LPSTR)LS(IDS_PLS_CHOOSE_DISK_FILE), 0, NULL, def_path );
		
		if(!img_pth)
		{
			WriteLog("!img_pth");
			return;
		}
		
	}
	else // Если указан аргумент то копирую его в "img_pth"
	{ 
		//img_pth=(LPTSTR)(LPCTSTR)_cmdline_image;

	}

	//WriteLog("img_pth=%s; ", img_pth);
	bool encrypted_partition=true;

	/*if ( _taccess(img_pth, 0) == 0 	&& _tcslen(img_pth) > 3 )
		_tcscpy(_OpenedDiskName, img_pth);
	else 
	{
		// opens partition		
		_tcscpy(_OpenedDiskName, img_pth);
		_OpenedDiskName[3] =0;

	}*/
	////////////////	
	

	if(  _cmdline_image.GetLength() )// Если указан конкретный образ
	{
		dlg.m_FullPath = _cmdline_image;
		dlg.AddPossibleDiskImageAtBegin(_cmdline_image); // 
	}

	if (img_pth && strlen(img_pth) )
		dlg.AddPossibleDiskImage(img_pth); // 

	int i=0;

	do  {     
		

		if ( dlg.DoModal() != IDOK)// открываю диалог ввода пароля
		{
			_tcscpy(_OpenedDiskName, "");
			return;
		}

		if (dlg.m_password.GetLength()==0) break; // open disk 

		_tcscpy(_OpenedDiskName, dlg.m_FullPath);		
		
		// read disk header (blob) from container, verify pwd, init encryption key
		if (FindBlob_CheckPassword(_OpenedDiskName , dlg.m_password) )	
		{
			break;// выход из цикла
		}		
	    AfxMessageBox(LS(IDS_WRONG_DISK_PASSW));
	}while(1);

	WriteLog("1");	

	// close volume
	ff_tree.DeleteAllItems();
	ff_list.DeleteAllItems();
	_FSys.CloseVolume();
	//***********************


	WriteLog("2");	
	DWORD data_offset_mb =0;

	// 1347584
	

	if (dlg.m_password.GetLength()==0)
	{
		setBlob(NULL, 0, NULL); // default blob
		encrypted_partition = false;
		
	} else
	{
		data_offset_mb = MAKELONG(pKeyInfo->Reserved1[5], pKeyInfo->Reserved1[6] )/*in MB*/ ; 

		// data_offset_mb =1347584;
		DWORD data_end_offset_mb =0;

		if (data_offset_mb == 0 && IsFileUseSteganos(_OpenedDiskName) )
		{
			GetSteganosOffset(_OpenedDiskName, &data_offset_mb, &data_end_offset_mb);
		}
	}
	

	
	AfxGetApp()->BeginWaitCursor();	
	
	WriteLog("%s %d", _OpenedDiskName, data_offset_mb);	
	
	// Begin to read FS from disk container
	// encryption key was created here - FindBlob_CheckPassword()
    if(_FSys.OpenVolume(_OpenedDiskName, encrypted_partition, data_offset_mb) )
	{
		_ImageOpened=true;
	}

	if (_FSys._readOnly)
	{
		CString title;
		title = LS(IDS_ROHOS_DISK_BROWSER);
		title += " (";
		title += LS(IDS_MOUNT_AS_READONLY);
		title += ")";		
		SetWindowText( title );
	}
	
	
	free_path_token(&PT_TOK);
	
	path_down(&PT_TOK,"/");
	path_up(&PT_TOK);
	load_list_files("/",ff_tree,ff_list,ff_img);

	AfxGetApp()->EndWaitCursor();
	
	//WriteLog("33 %s", img_pth);
	//getforderlist !!!
	{
		make_folder_tree(&PT_TOK,ff_tree, _OpenedDiskName);
	}


	this->SetDlgItemText(IDC_PATH, PT_TOK.pth);  

	ff_tb.GetToolBarCtrl().EnableButton(TB_BT_BACK, true);
	ff_tb.GetToolBarCtrl().EnableButton(TB_BT_FORWARD, true);
	ff_tb.GetToolBarCtrl().EnableButton(TB_BT_UP2, true);
	
	
	// load favorite folders

	TCHAR *fav_folders_list;
	DWORD len =0;

	CString str = _FSys.ReadWholeFile( "/rohos_favorites.txt", &len);
	if (str.GetLength())
	{
		int curPos= 0;
		CString resToken;

		//AfxMessageBox(str);

		resToken= str.Tokenize("\x0D",curPos);
		while (resToken != "")
		{
			
			cbox_FavoritesList.AddString(resToken);
			resToken= str.Tokenize("\x0D",curPos);
			resToken.Delete(0, 1); // remove first \x0A
		};

	}


	// ПРОВЕРКА РЕГИСТРАЦИИ
	//WriteLog("Registration Check <----------------");

	//Если программа запущенна без параметров
	if(_cmdline_image.GetLength()==0)
	{
		//WriteLog("No Command Line Parametrs");

		// since rohos diks v1.4. removed 
		/*
		// Достать Проверить регистрационный ключ
		TCHAR RegNum[200];
		ReadReg(HKEY_LOCAL_MACHINE, "SOFTWARE\\Rohos", "RegNumber", RegNum, 300);
		
		//Если Ключ не верный
		if(_tcslen(RegNum)<20)
		{
			WriteLog("bad registration name");
			//Значит программа не зарегестрированна
			
			// Проверка дней используется программа
			
			TCHAR ModuleName[500];// Имя выполняемого файла прогрммы
			HMODULE hMod;// Хэндл на него
			DWORD DaysEval=0;// Сколько дней используется программа		
			
			LPFILETIME FileTime=new FILETIME;
			LPSYSTEMTIME ModuleSysTime=new SYSTEMTIME;// Тут - информация о .ехе файле
			LPSYSTEMTIME SystemTime=new SYSTEMTIME;// Тут - вемя на данный момент
			
			hMod=GetModuleHandle(NULL);// Хэндл на Главный Модуль
			GetModuleFileName(hMod, ModuleName, 500);// Выяснение имени файла
			
			_ofList.GetFileTimeByName(ModuleName,1,FileTime);// высасывание из него информации		
			GetSystemTime(SystemTime); // Узнаю который час,днеь,месяц....
			
			// Вычисление: DaysEval
			CTime t1(*FileTime,0);   // Для вычисления разницы
			CTime t2(*SystemTime,0); //....
			CTimeSpan ts;	

			// Вычисление разницы во времени
			ts =  ( t2 - t1 );//CTimeSpan=Ctime-CTime;

			DaysEval = ts.GetDays();// нужны дни

			
			//WriteLog("Days Evaluated=(%d)>>>>>>>>>>>>>",DaysEval);

			// if(Кол. Дней использования<=30)
			
			if(DaysEval<=30)
			{
				WriteLog("DaysEval<=30");
				_EvaluationStatus=1;// программа не будет орать
			}
			else
			{ 
				WriteLog("DaysEval>30");
				_EvaluationStatus=2; // Вывести MessageBox
				
				// потом надо будет добавить поддержку языков
				AfxMessageBox( LS(MSG_EVALUATION_TIME_EXPIRED) );
			}
			
		}
		else
		{ // Если ключ ПРАВЕЛЬНЫЙ
			WriteLog("Program is Registered");
			_EvaluationStatus=3;// программа зарегестрированна
		}
		*/
		
	}
	else
	{ 
		// Пока ничего...
	}

	
	
}
/** decrypt a Rohos Disk header information (blob) and verify it is correct. set pKeyInfo
*/
bool CDlg_DiskBrowserMain::setBlob(LPBYTE pBlob, DWORD blob_size, LPCTSTR passwd)
{	
	

	pDiskBlob= &blob;
	

	if (!pDiskBlob->pbData) {
		pDiskBlob->cbData = sizeof( DISK_BLOB_INFO );
		pDiskBlob->pbData = new BYTE[pDiskBlob->cbData+64];
		strcpy((char*)pDiskBlob->pbData, "this disk bloc");
	}

	if (file_blob_size ==0 && pBlob==NULL)
	{
		memset( pDiskBlob->pbData, 0, sizeof( DISK_BLOB_INFO ) ); 

		pKeyInfo = (DISK_BLOB_INFO*)pDiskBlob->pbData;
		memset( pKeyInfo->IV, 0, 64 ); 
		memset( pKeyInfo->Key, 0, 32 );
		pKeyInfo->DiskNumber = 'R' - 'A';
		strcpy(pKeyInfo->FileName, "");
		return true;
	}
		

	if (file_blob_size ==0 && _tcslen(passwd)==0 ) // opening non-encrypted , plain disk from YSB drive
		return true;

	if (file_blob_size ==0 ) // opening encrypted drive, (but without Admin rights )
		return false;

	BYTE key[KEY_SIZE+16] = "";
	BYTE iv[IV_SIZE+16] = "";
	
	pDiskBlob->cbData  = blob_size;
	memcpy( pDiskBlob->pbData, pBlob, blob_size );
	memset( iv, 0, IV_SIZE );
	memset( key, 0, KEY_SIZE );
	


	DeriveKeyFromPassw_v2(passwd, key);
	if ( !DecryptBuffer( pDiskBlob->pbData, &( pDiskBlob->cbData ), key, iv ) ) 
	{			
		WriteLog("Wrong Passw...");

		// try encryption_mode==1
		memcpy( pDiskBlob->pbData, pBlob, blob_size );
		DeriveKeyFromPassw(passwd, key);
		if ( !DecryptBuffer( pDiskBlob->pbData, &( pDiskBlob->cbData ), key, iv ) ) 
		{		
			pDiskBlob = NULL;
			return false;		
		}
	}
	
	memset( key, 0, KEY_SIZE );
	
	//memset( pBuffer, 0, BufferSize );
	//free( pBuffer );

	
	pKeyInfo = (DISK_BLOB_INFO*)pDiskBlob->pbData;
	if( pDiskBlob->cbData != sizeof( DISK_BLOB_INFO ) || 
		HIWORD(pKeyInfo->Version) < 1 ||
		HIWORD(pKeyInfo->Version) > 30
		)
	{
		//setError(IDS_BAD_DISKINFO, IDS_BAD_DISKINFO_CAUSE);
		WriteLog("CDisk::setBlob, Unknown Disk Info Version, ver:%X (info sz:%d)", pKeyInfo->Version, pDiskBlob->cbData);
		pDiskBlob = NULL;
		return FALSE;
	}

	if ( _tcslen(pKeyInfo->FileName) > DISK_MAX_PATH)
		pKeyInfo->FileName[DISK_MAX_PATH-1] =0;

	if ( _tcslen(pKeyInfo->AlgorithmName) > 11)
		pKeyInfo->FileName[11] =0;

//WriteLog("Disk: %d %d %d", Key[0], Key[1], Key[2] );

	
	return TRUE;

}

/////////////////////////////////////////////////////////////////////////////
BOOL CDlg_DiskBrowserMain::AddDiskBlobToContainer(LPCTSTR containerName, LPBYTE blob1, DWORD blob_size1)
{
	HANDLE hFile =0;

	CString backup_path;
	backup_path = containerName;
	LARGE_INTEGER i;
	DWORD lo;

	if ( _tcslen(containerName) <= 3) /* c:\ - partition based*/
	{
		hFile = 0; //theDiskPartition->hDevice;

		LARGE_INTEGER offs2;
		offs2.QuadPart=0;

		//theDiskPartition->SetFilePointer2(&offs2);
		//theDiskPartition->SetDataOffset(&offs2);

		//theDiskPartition->WriteData(blob1, blob_size1, &theDiskPartition.partOffset)

		i = theDiskPartition->GetPartitionedSpace();

		if (i.QuadPart%512)
		{
			i.QuadPart = 512 - (i.QuadPart%512);
		}

		SetFilePointerEx(theDiskPartition->hDevice, i, NULL, FILE_BEGIN);

		DWORD file_blob_size_512 = blob_size1;

		if ( file_blob_size_512 % 512 ) 
			file_blob_size_512 += (512 - (file_blob_size_512 % 512)); // len should be equal to %512		

		if( !WriteFile( theDiskPartition->hDevice, blob1, file_blob_size_512, &( lo = 0L ), NULL )  )		
		{
			WriteLog(  "AddDiskBlobToContainer. WriteFile Part %08X. %08X.  %08X. %d", GetLastError(), blob_size1, lo, i.QuadPart / (1024 * 1024) );		
			return FALSE;
		}

		backup_path += "_backup.rdi1";

	} else
	{
		backup_path += ".rdi1";

		// getting Container file handle to make  WriteFile() later
		if (_FSys._isNTFS)
		{		
			if (_FSys.ntfs_volume)
				hFile = (HANDLE)_FSys.ntfs_volume->dev->d_handle ;
		}
		else
		{		
			hFile = _FSys.fatvolume_handle;
		}
	}

	if (hFile)
	{
		// only file based containers goes here
		
		
		DWORD data_offset_mb =0; // more that 0 - for new (v.2) containers

		if ( IsFileUseSteganos(containerName)  )
		{
			
			DWORD begin_mb, last_mb;
			GetSteganosOffset(containerName, &begin_mb, &last_mb);			
			
			i.QuadPart =0;
			i.QuadPart -= last_mb*1024*1024;
			i.QuadPart -= blob_size1;

			SetFilePointerEx(hFile, i, NULL, FILE_END);
		} else

		if (pKeyInfo) 
		{
			data_offset_mb = MAKELONG(pKeyInfo->Reserved1[5], pKeyInfo->Reserved1[6] )/*in MB*/ ; 

			if (data_offset_mb)	// new containers where Blob is at the Beggining  
				SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
			else 
			{					// old containers where Blob is at the End  
				i.QuadPart =0;				
				i.QuadPart -= blob_size1;
				SetFilePointerEx(hFile, i, NULL, FILE_END);			

			}
		}			

		if( !WriteFile( hFile, blob1, blob_size1, &( lo = 0L ), NULL ) || lo != blob_size1 )		
		{
			WriteLog(  "AddDiskBlobToContainer. WriteFile %08X. %08X.  %08X.", GetLastError(), blob_size1, lo );		
			return FALSE;
		}

		// for rdi containers we should write Container signature
		if ( IsFileUseSteganos(containerName)  == false)		
		{
			WriteFile( hFile, "ROHO", 4, &( lo = 0L ), NULL );
			WriteFile( hFile, (BYTE*)&blob_size1, 4, &( lo = 0L ), NULL );

			if (data_offset_mb) // new container
			{
				// for new containers where Blob is at the Beggining  
				//  we need also to update block at the end!

				i.QuadPart =0;				
				i.QuadPart -= blob_size1;
				SetFilePointerEx(hFile, i, NULL, FILE_END);

				WriteFile( hFile, blob1, blob_size1, &( lo = 0L ), NULL );
				WriteFile( hFile, "ROHO", 4, &( lo = 0L ), NULL );
				WriteFile( hFile, (BYTE*)&blob_size1, 4, &( lo = 0L ), NULL );

			}
		}	

	}

	try{
		// update backup blob if it exist.
		CFile f(backup_path, /*CFile::modeCreate |*/ CFile::modeWrite | CFile::typeBinary ); 
		f.Write(blob1, blob_size1);
		f.Write("ROHO", 4);
		f.Write(&blob_size1, 4);
		f.Close();			
	}

	catch (...)
	{
	}


	return true;

}


/** вернет TRUE если файл-имадж стеганографический
т.е. это avi 
*/
BOOL IsFileUseSteganos(LPCTSTR FileName)
{
	const char *ext = strrchr(FileName, '.');

	if (ext) {
		if ( strstr(ext, "AVI")  ||
			strstr(ext, "avi") ||
			strstr(ext, "mp3") ||
			strstr(ext, "MP3") ||
			strstr(ext, "mp4") ||
			strstr(ext, "MP4") ||
			strstr(ext, "MPG") ||
			strstr(ext, "mpg") ||
			strstr(ext, "OGG") ||
			strstr(ext, "ogg") ||			
			strstr(ext, "wma") ||
			strstr(ext, "WMA") ||
			strstr(ext, "wmv") ||
			strstr(ext, "WMV") ||
			strstr(ext, "iso") ||
			strstr(ext, "ISO") ||
			strstr(ext, "nrg") ||
			strstr(ext, "NRG") ||
			strstr(ext, "dmg") ||
			strstr(ext, "DMG") ||
			strstr(ext, "RAR") ||
			strstr(ext, "rar") ||
			strstr(ext, "ZIP") ||
			strstr(ext, "zip") 			
			
			)
		{
			
			return true;
		}
	}

	return false;

}

/** вернуть кол-во МегаБайт для отступа, 
через эти № Мб-т начнуться зашифрованные данные
begin_offset - 
*/
BOOL GetSteganosOffset(LPCTSTR FileName, LPDWORD begin_offset_mb, 
							  LPDWORD ending_offset_mb, LPDWORD file_size_mb)
{
	HANDLE hFile;
	LARGE_INTEGER i;

	
	if (!begin_offset_mb || !ending_offset_mb) return false;

	*begin_offset_mb =0;
	*ending_offset_mb=0;

	if (file_size_mb)
		*file_size_mb = 0;

	if ( !IsFileUseSteganos(FileName) ) 
		return false;


	if( ( hFile = CreateFile( FileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL ) ) == INVALID_HANDLE_VALUE )
		{
			WriteLog("CDisk::GetSteganosOffset, fail open file %X, %s", GetLastError(), FileName );
			return false;						
		}

	if ( !GetFileSizeEx(hFile, &i) ) {

		WriteLog("CDisk::GetSteganosOffset, fail GetFileSizeEx %X, %s", GetLastError(), FileName );
			return false;						

	}

	CloseHandle(hFile);

	DWORD MBsize;
	MBsize = i.QuadPart / (1024*1024);

	if (file_size_mb)
		*file_size_mb = MBsize;

	if (MBsize < 3)
		return false;

	*begin_offset_mb = 1; // 1 Mb
	*ending_offset_mb = 1; // 9 Mb	

	if (MBsize > 13)
	{
		*begin_offset_mb =  2; // 10 Mb îòñòóï îò íà÷àëà
		*ending_offset_mb =  9; // 9 Mb  îòñòóï îò êîíöà ôàéëà		
	}

	if (MBsize > 25) {
		*begin_offset_mb =  10; // 10 Mb îòñòóï îò íà÷àëà
		*ending_offset_mb =  9; // 9 Mb  îòñòóï îò êîíöà ôàéëà
	} 

	if (MBsize > 125) {
		*begin_offset_mb =  20; // 20 Mb îòñòóï îò íà÷àëà
		*ending_offset_mb =  20; // 20 Mb  îòñòóï îò êîíöà ôàéëà
	} 

	if (MBsize > 1250) {
		*begin_offset_mb =  50; // 20 Mb îòñòóï îò íà÷àëà
		*ending_offset_mb =  80; // 20 Mb  îòñòóï îò êîíöà ôàéëà
	} 
	
	

	return true;

}

BOOL CDlg_DiskBrowserMain::GetFileBlob_New(LPCTSTR FileName)
{
	//return false;

	HANDLE hFile;
	long hi;
	DWORD lo;
	DWORD len;
	DWORD LastError = ERROR_SUCCESS;

	if ( _tcslen(FileName) <= 3) /* c:\ - partition based*/
	{
		CDiskRepartition *_harddisk;

		_harddisk = new CDiskRepartition();

		_harddisk->SetDriveLetter(FileName[0]);		

		LARGE_INTEGER li; li.QuadPart =0;

		len = 4608;
		if ( !_harddisk->CheckDisk() || !_harddisk->ReadData( file_blob, len, &li ) )
		{
			return false;
		}
		file_blob_size = 2192;

		WriteLog("GetFileBlobNew at %X %X", li.QuadPart / (1023 * 1024), _harddisk->_partitioned_space.QuadPart / (1023 * 1024));

		//_OpenedDiskName

		_harddisk->CloseDisk();

		return true;
	}
		

	if( ( hFile = CreateFile( FileName, GENERIC_READ, /*0*/FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL ) ) == INVALID_HANDLE_VALUE )
	{
		
		WriteLog( "GetFileBlobNew. errorOpneFile1 %08X. %s.", GetLastError(), FileName );

		Sleep( 500 );		

		if( ( hFile = CreateFile( FileName, GENERIC_READ, /*0*/FILE_SHARE_READ| FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL ) ) == INVALID_HANDLE_VALUE ) {
			WriteLog("GetFileBlobNew. errorOpneFile2 %08X. %s.", GetLastError(), FileName );
			SetLastError( LastError );
			return false;
			}
			
	}

	file_blob_size = 2192;
	memset(file_blob, 0, 5000);

	if ( !ReadFile( hFile, file_blob, file_blob_size, &( lo = 0 ), NULL ) || lo != file_blob_size)
	{
		WriteLog(  "GetFileBlobNew, err read blob  %X %d %d", GetLastError(), len, lo );
		CloseHandle( hFile );
		return false;

	}

	CloseHandle( hFile );
	return TRUE;

}

/////////////////////////////////////////////////////////////////////////////
BOOL CDlg_DiskBrowserMain::GetFileBlob(LPCTSTR FileName)
{

	//return false;

	HANDLE hFile;
	long hi;
	DWORD lo;
	DWORD len;
	DWORD LastError = ERROR_SUCCESS;

	if ( _tcslen(FileName) <= 3) /* c:\ - partition based*/
	{
		return false;
	}
		

	if( ( hFile = CreateFile( FileName, GENERIC_READ, /*0*/FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL ) ) == INVALID_HANDLE_VALUE )
	{
		
		WriteLog( "GetFileBlob. errorOpneFile1 %08X. %s.", GetLastError(), FileName );

		Sleep( 500 );		

		if( ( hFile = CreateFile( FileName, GENERIC_READ, /*0*/FILE_SHARE_READ| FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL ) ) == INVALID_HANDLE_VALUE ) {
			WriteLog("GetFileBlob. errorOpneFile2 %08X. %s.", GetLastError(), FileName );
			SetLastError( LastError );
			return false;
			}
			
	}

	lo -= 8L;
	SetLastError( LastError );
	LARGE_INTEGER li;
	li.QuadPart = -8;
	//li.u.LowPart = -8;

	BOOL use_steganos=false;

	if ( IsFileUseSteganos(FileName) ) 
	{

		use_steganos = true;

		// using stegano's , store data inside AVI
		// данный в AVI идут через 10Мб т.е. чтобы фильм показывался.
		// концовка файла это размер AVI - 50 кб. 

		DWORD begin_mb, last_mb;
		if ( GetSteganosOffset(FileName, &begin_mb, &last_mb) ) 
		{		
			li.QuadPart =0;
			li.QuadPart -= last_mb*1024*1024;
		}		
	}


	if( SetFilePointerEx( hFile, li, NULL, FILE_END ) == false ||
		!ReadFile( hFile, (BYTE*)&len, 4, &( lo = 0 ), NULL ) || lo != 4 ||
		memcmp( (BYTE*)&len, "ROHO", 4 ) )
	{
		if (use_steganos && memcmp( (BYTE*)&len, "ROHO", 4 ) ) {
			/// we could ignore this error for AVI (last avi containers doesnt have ROHO signature)

		} else {			

			LastError = GetLastError();
			WriteLog(  "GetFileBlob, image without blob %08X. %08X. %08X", GetLastError(),  lo, len );
			//CloseHandle( hFile );
			if( !LastError && lo == 4 ) \
				LastError = ERROR_FILE_CORRUPT;
			SetLastError( LastError );
			//return FALSE;
		}
	}


	// reading blob len
	if( !ReadFile( hFile, (BYTE*)&len, 4, &( lo = 0), NULL )|| lo != 4 )
	{
		LastError = GetLastError();
		WriteLog("GetFileBlob, err read blob len %08X. %08X.", GetLastError(), lo );
		//CloseHandle( hFile );
		SetLastError( LastError );
		return FALSE;
	}

	
	if( len > 2192 || len < 2192)
	{		
		//CloseHandle( hFile );
		SetLastError( ERROR_MORE_DATA );
		WriteLog("GetFileBlob, blob too long %08X. %s", len , FileName);
		len = 2192;
		//return FALSE;
		//li.QuadPart =0 ;		
	}  	
	
	li.QuadPart -= len;

	file_blob_size = len;
	memset(file_blob, 0, 5000);

	WriteLog ("GetFileBlob, found blob sz=%d, %s", len, FileName);
	

	// считать блоб
	if( SetFilePointerEx( hFile, li, NULL, FILE_END ) == false ||
		!ReadFile( hFile, file_blob, len, &( lo = 0 ), NULL ) || lo != len )
	{
		
		WriteLog(  "GetFileBlob, err read blob  %X %d %d", GetLastError(), len, lo );
		CloseHandle( hFile );
		file_blob_size = 0;		
		return FALSE;
	}

	CloseHandle( hFile );
	
	return TRUE;

}

/////////////////////////////////////////////////////////////////////////////
//  По указанным координатам курсора мыши, определяет индекс элемента ListView
/////////////////////////////////////////////////////////////////////////////
int CDlg_DiskBrowserMain::GetLVItemByXY(POINT xyPoint)
{
	 //WriteLog("CDlg_DiskBrowserMain::GetTVItemByXY");

    int hResult = -1;
	 LV_HITTESTINFO tvHitTestInfo;// переменная с информацией о найденом элементе

	 ff_list.ScreenToClient(&xyPoint);// перевод коордонат с глобальных на ListView-ые
	 tvHitTestInfo.pt = xyPoint;// задаю входной параметр
	 hResult = ListView_HitTest(ff_list.m_hWnd, &tvHitTestInfo); // определение
     
	 
	// WriteLog(" HitTest Result=%d",hResult);
	 

    return hResult;
}
/////////////////////////////////////////////////////////////////////////////
//  Выводиат диалог с информацией о открытом образе /////////////////////////
/////////////////////////////////////////////////////////////////////////////
void CDlg_DiskBrowserMain::onDiskInfo()
{

		CDlg_DiskInfo Dinf ;
		
		Dinf.pKeyInfo = (DISK_BLOB_INFO*)pDiskBlob->pbData;

		_tcscpy(Dinf._image_file_name, GetFileNameOfOpenedImage());
		
		//WriteLog("Dinf.pKeyInfo %s", Dinf.pKeyInfo->FileName);
		
		Dinf.m_chbx_readonly = _FSys._readOnly; // Сообщение окну о состоянии файла образа
		if(_FSys._readOnly)
		{
			Dinf.m_ed8_text=LS(IDS_READONLY);
		}
		else
		{ 
			//Dinf.m_ed8_text;	read-write
		}	

		if(_FSys._isNTFS)		
			Dinf._format = "NTFS";
		else 
			Dinf._format = "FAT";		
		
		Dinf.DoModal();

}
/////////////////////////////////////////////////////////////////////////////
// возвращает имя открытого образа диска
///////////////////////////////////////////////////////////////////////////
CString CDlg_DiskBrowserMain::GetFileNameOfOpenedImage()
{	

  if (pDiskBlob == NULL)
	  return "";

  //return ((DISK_BLOB_INFO*)pDiskBlob->pbData)->FileName;
  return _OpenedDiskName;
  
}
/////////////////////////////////////////////////////////////////////////////
void CDlg_DiskBrowserMain::OnDoubleClickLV(NMHDR* pNMHDR, LRESULT* pResult) 
{
	// TODO: Add your control notification handler code here
	//WriteLog("CDlg_DiskBrowserMain::OnDoubleClickLV");
	*pResult = 0;
}
/////////////////////////////////////////////////////////////////////////////
// Из полного пути к файлу получает только имя файла
/////////////////////////////////////////////////////////////////////////////
CString CDlg_DiskBrowserMain::GetFileNameFromFullPath(CString in_str)
{
  WriteLog("in_str=%s;", in_str);

  for(int i=0;i<= in_str.GetLength()-1; i++)// цикл по всей строке
  {
	   if(in_str[i]=='\\')// при нахождении слэша
	   { 
			in_str.Delete(0,i+1); // Удаляю все символы до него (включая)
			WriteLog("in_str=%s;  <", in_str);
	   }
  } return in_str;
	 
}
/////////////////////////////////////////////////////////////////////////////
//  Выводит окно с предложением: "Удалить временные файлы?" (yes/no)
/////////////////////////////////////////////////////////////////////////////
void CDlg_DiskBrowserMain::AskForDeleting()
{

    WriteLog("AskForDeleting()-");

 	char bf[1024]={0};
	sprintf(bf,"Delete temp folder?");// чтение из *.lng
	
	// Вывод предупреждения
	if( AfxMessageBox(bf,MB_YESNO)==IDYES) //нажата кнопка "Yes"
	{
		// Удалить временную папку с её содержимым 		
		TCHAR OutDir[300];
		TCHAR FullOutDir[510], newFileName[500];

		GetPodNogamiPath(OutDir,0/*1 - с названием файла; 0 - без*/ );
		//WriteLog("Application Directory: %s", OutDir);
		
		memset(FullOutDir, 0, 300);

		_tcscpy(FullOutDir, OutDir);  // "%AppDir%\\"
		_tcscat(FullOutDir, "temporary files\\*.*");// "%AppDir%\\temporary files\\"		
		//WriteLog("FullOutDir=%s",FullOutDir);

		WIN32_FIND_DATA FindFileData;
		HANDLE hFind;

		hFind = FindFirstFile(FullOutDir, &FindFileData);

		//WriteLog("Finding file %s", path);

		if (hFind == INVALID_HANDLE_VALUE) 		
			return;	 

		TCHAR *zero_buff = (TCHAR*)malloc(1024*11);
		memset(zero_buff,0xfffe, 1024*10);

		do {		// shredd files 

			
			_tcscpy(FullOutDir, OutDir);  // "%AppDir%\\"
			_tcscat(FullOutDir, "temporary files\\");// "%AppDir%\\temporary files\\"	
			_tcscat(FullOutDir, FindFileData.cFileName);// "%AppDir%\\temporary files\\"	

			_tcscpy(newFileName, OutDir);  // "%AppDir%\\"
			_tcscat(newFileName, "temporary files\\XXXXXXXXXXXXXXXXXXXXXXXX.roh");// "%AppDir%\\temporary files\\"				

			HANDLE hFile = CreateFile(FullOutDir,    // file to open
				GENERIC_WRITE,          // open for reading and writing
				FILE_SHARE_READ | FILE_SHARE_WRITE,       // share for reading
				NULL,                  // default security
				OPEN_EXISTING,         // existing file only
				FILE_ATTRIBUTE_NORMAL, // normal file
				NULL);                 // no attr. template

			if (hFile!=INVALID_HANDLE_VALUE)
			{
				DWORD szwr;
				DWORD sz = FindFileData.nFileSizeLow;
				if (sz > 1024*10)
					sz = 1024*10;
				WriteFile(hFile, zero_buff, sz, &szwr, NULL); 
				CloseHandle(hFile);
			}
			MoveFile(FullOutDir, newFileName);
			if ( !DeleteFile(newFileName) );
				DeleteFile(FullOutDir);

		} while ( FindNextFile(hFind, &FindFileData) );



		_tcscpy(FullOutDir, OutDir);  // "%AppDir%\\"
		_tcscat(FullOutDir, "temporary files");// "%AppDir%\\temporary files\\"

			
		SHFILEOPSTRUCT lpFileOp;
							// Сбор информации
		lpFileOp.hwnd=this->m_hWnd;
		lpFileOp.wFunc=FO_DELETE;
		lpFileOp.pFrom=FullOutDir;
		lpFileOp.pTo=NULL;
		lpFileOp.fFlags=FOF_SILENT | FOF_NOERRORUI | FOF_NOCONFIRMATION;
							// УДАЛЕНИЕ ПАПКИ "Temporary files" вместе с одержимым
		int res = SHFileOperation(&lpFileOp);

		//WriteLog("SHFileOperation =%d", res);
		
	}
	else//нажата кнопка "No"
	{
		// Ничего не делать
	}

}
/////////////////////////////////////////////////////////////////////////////
///  Возвращяет путь к этому приложению 
/////////////////////////////////////////////////////////////////////////////
CString CDlg_DiskBrowserMain::GetAplicationDirectory(DWORD in_flag)
// in_flag=1 Включая имя файла  
//  in_flag=0 Не включая имя файла
{
	TCHAR OutDir[300];
	CString FullOutDir;

	GetPodNogamiPath(OutDir,in_flag/*1 - с названием файла; 0 - без*/ );
	
	 FullOutDir=OutDir;// "%AppDir%\\"
	 FullOutDir=FullOutDir+"temporary files";// "%AppDir%\\temporary files\\"

	// if ( _access(FullOutDir, 0)
	return FullOutDir;
}
/////////////////////////////////////////////////////////////////////////////
// Выводит диалог о том что выбранный файл будет загружен
/////////////////////////////////////////////////////////////////////////////

/*
bool CDlg_DiskBrowserMain::ShowNoghiInfoIfNeeded()
{
	WriteLog("CDlg_DiskBrowserMain::ShowNoghiInfoIfNeeded()");
    
	
	settings_ls(&SETT,0);// читает из файла структуру SETT 	 
	
	
	//если тут записано 0 
	if(!SETT.ShowNoghi )// Проверят - надо ли показывает диалог про ноги
	{
		CDlg_InfoNoghi Legs;// Создает диалог
		
								// Берет текс из *.lang файла
		Legs.m_LabelInfCaption=LS(MSG_INFO_POD_NOGHI);
		Legs.m_Label2=LS(MSG_DONT_SHOW_AGAIN); 
		

		if( Legs.DoModal()==IDOK)// показывает его и проверяет на OK
		{
		    SETT.ShowNoghi=(bool)Legs.m_cbValue;// записывает в SETT значение CheckBox-а
			settings_ls(&SETT,1);// Записывает в файл

		}
		else
		{
			 return false;
		}
	
	}	

    return true;	
}*/


/////////////////////////////////////////////////////////////////////////////
// возвращяет количество свободных байт на указаном диске
/////////////////////////////////////////////////////////////////////////////
// так как эта функция получена из MSDN постредством: [Ctrl+C] [Ctrl+V],   // 										Своих коментариев не добавляю
/////////////////////////////////////////////////////////////////////////////
typedef BOOL (WINAPI *PGETDISKFREESPACEEX)(LPCSTR,
										   PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);
__int64 CDlg_DiskBrowserMain::MyGetDiskFreeSpaceEx(LPCSTR pszDrive)
{

	PGETDISKFREESPACEEX pGetDiskFreeSpaceEx;
	__int64 i64FreeBytesToCaller, i64TotalBytes, i64FreeBytes;
	
	DWORD dwSectPerClust, 
		dwBytesPerSect, 
		dwFreeClusters, 
		dwTotalClusters;
	
	BOOL fResult;
	
	pGetDiskFreeSpaceEx = (PGETDISKFREESPACEEX) GetProcAddress( 
		GetModuleHandle("kernel32.dll"),
		"GetDiskFreeSpaceExA");
	
	if (pGetDiskFreeSpaceEx)
	{
		fResult = pGetDiskFreeSpaceEx (pszDrive,
			(PULARGE_INTEGER)&i64FreeBytesToCaller,
			(PULARGE_INTEGER)&i64TotalBytes,
			(PULARGE_INTEGER)&i64FreeBytes);
		
		// Process GetDiskFreeSpaceEx results.
		if(fResult) 
		{

			printf("Total free bytes = %I64d\n", i64FreeBytes);
		}
		else
		{
			WriteLog(" Error=%d",GetLastError());
		}
		return i64FreeBytes;
	}
	
	else 
	{
		fResult = GetDiskFreeSpaceA (pszDrive, 
			&dwSectPerClust, 
			&dwBytesPerSect,
			&dwFreeClusters, 
			&dwTotalClusters);
		
		// Process GetDiskFreeSpace results.
		if(fResult) 
		{
			
			printf("Total free bytes = I64d\n", 
				dwFreeClusters*dwSectPerClust*dwBytesPerSect);
		}
		
		
		return (dwFreeClusters*dwSectPerClust*dwBytesPerSect);
	}

}
/////////////////////////////////////////////////////////////////////////////
// говорит поместится ли FileSize на диске с которого запускается Приложение
/////////////////////////////////////////////////////////////////////////////
bool CDlg_DiskBrowserMain::EnoughSpace(__int64 FileSize)
{
    __int64 FreeSpace;
	CString s=GetAplicationDirectory(0);//  X:\adasda\asdasda\asda
	TCHAR ss[100];
	
	_tcscpy(ss,s);//"X:\adasda\asdasda\asda"
	ss[3]=NULL;///*  "X:\"                                */
	FreeSpace=GetDiskFreeSpaceMB(ss); // Определение свободного места на диске
	
	FileSize/=1048576; // преобразование из Байтов в МегаБайты
	//KB (Kilobyte) = 2^10 Byte = 1.024 Byte 
	//MB (Megabyte) = 2^20 Byte = 1.048.576 Byte   <-------------------------
	//GB (Gigabyte) = 2^30 Byte = 1.073.741.824 Byte
	//TB (Terabyte) = 2^40 Byte = 1.099.511.627.776 Byte 
	//PB (Petabyte) = 2^50 Byte = 1.125.899.906.842.624 Byte 
	//EB (Exabyte)  = 2^60 Byte = 1.152.921.504.606.846.976 Byte
	
	WriteLog("Free space on disk %s = %I64d MB",ss, FreeSpace );
			// если выводит - 0, это значит файл меньше одного мегабайта
	WriteLog("File Size is = %I64d MB. (when =0, it means that the filesize is <1 MB )",FileSize );
	
	//					    MB// на всякий случай
	if (FreeSpace<(FileSize+1 )) // Если не хватает места для записи
	{
		char bf[1024]={0};
		sprintf(bf,LS(IDS_NOT_ENOUGH_SPACE),ss);	
		
		AfxMessageBox(bf);// надо Сделать для разных языков
		return false;	   	 
	}
	return true;
	
}
/////////////////////////////////////////////////////////////////////////////
void CDlg_DiskBrowserMain::OnTimer(UINT nIDEvent) 
{
	
	if (nIDEvent==1) //  делается только один раз из OnInitDialog()
	{
		KillTimer(nIDEvent);
		//WriteLog("Timer1 Killed.");

		OnOpenDisk();	
	}

	/*if (nIDEvent==2) // Этот таймер проверяет все подножные файлы на изменение 
	{
		// Если какойто  из файлов был изменён 
		if( _ofList.CheckForChanges() )
		{
				
			
			CDlg_FilesChanged fch;
			fch._ChagedFileName=_ofList._FList[_ofList._Ichel].FileName;
			
			//Вызываю предупреждение
			if(fch.DoModal()==IDOK)
			{
				//::ShowWindow(fch.m_hWnd, SW_SHOWNORMAL ); 
				// Там Нажалась - ДА
				
				TCHAR src[500],dest[500];			

				GetPodNogamiPath_Temp(src);
				
				_tcscat(src,_ofList._FList[_ofList._Ichel].FileName);

				_tcscpy(dest,_ofList._FList[_ofList._Ichel].Path);
				_tcscat(dest,"/");
				_tcscat(dest,_ofList._FList[_ofList._Ichel].FileName);

				
				 if( _FSys.LoadFile(src,dest)==-1)
				 {
					 // Если копирование закончилось неудачей
					  AfxMessageBox(LS(IDS_ERROR_OPENING_FILE), MB_ICONINFORMATION);
					  WriteLog("Error loading file!(%s)",src);
					  return;
				 }
			
				
				// После копирования файла обратно в образ, из списка его можно уничтожить
				//_ofList.Clear();
				_ofList.DeleteElement(_ofList._Ichel);
			 if(!_ofList._Count)
				KillTimer(nIDEvent);

				//WriteLog("Timer 2 Killed");
			}
			else
			{ 
				// Там Нажалась - НЕТ
				
			}
			
		}
	}*/
	
	CDialog::OnTimer(nIDEvent);
}
///////////////////////////////////////////////////////////////////////////////////////
//  делает из такого(documents/vasea/folder1/folder43/file.jpg) вот такое (file.jpg) // 
///////////////////////////////////////////////////////////////////////////////////////
LPCTSTR CDlg_DiskBrowserMain::GetFileNameFromPath(LPCTSTR in)
{ 
	//WriteLog("in=%s",in);
	static TCHAR out[500] = {0};
	

	const  char *last_slash = _tcsrchr( in, '/'); // Scan a string for the last occurrence of a character
	if (last_slash==0)
	{
		last_slash = _tcsrchr(in, '\\');
	}

	if (last_slash)
	{
		_tcscpy(out, last_slash + _tclen( last_slash));
	}
	else	
	{
		_tcscpy(out, in);	
	}

	return out;
	
}
/////////////////////////////////////////////////////////////////////////////
//  Открывает окно: "О программе"
void CDlg_DiskBrowserMain::OnAbout()
{
	CDlg_About abt;
	if( abt.DoModal()==IDOK )
	{ 
		// Если там нажали "ОК"
	}
	
}
// Возвращяет: "%ProgramPath%\temporary files\"
void GetPodNogamiPath_Temp(LPTSTR path)
{
	GetPodNogamiPath(path,0);
	_tcscat(path,"temporary files\\");
}
// listview->popup menu 2-> Create folder
void CDlg_DiskBrowserMain::OnCreateFolder()
{ 

}
// При нажатии на - List view PopUpMenu2->Import File
void CDlg_DiskBrowserMain::OnImportFile()
{
	if (_ImageOpened == false &&  GetFileNameOfOpenedImage().GetLength() == 0)
		return;

	// Если образ открыт для записи
	if(!_FSys._readOnly)
	{
		CFileDialog *myFileDialog = new CFileDialog(TRUE);
		
		//myFileDialog->cre
		
		CString file;		
		TCHAR filter[105] = "*.*\x00*.*\x00\x00";
		
		//memset(filter, 0, sizeof(TCHAR)*101 );
		//LoadString(AfxGetApp()->m_hInstance, IDS_PICTERES_FILTER, filter, 100);
		CString title;
		title = LS(IDS_IMPORT);	
		
		
		myFileDialog->m_ofn.Flags = 0 | OFN_HIDEREADONLY | OFN_PATHMUSTEXIST |OFN_FILEMUSTEXIST | 
										OFN_ALLOWMULTISELECT | OFN_DONTADDTORECENT | OFN_ENABLESIZING | OFN_EXPLORER;
		myFileDialog->m_ofn.hwndOwner = m_hWnd;
		myFileDialog->m_ofn.lpstrFilter = (LPCTSTR)filter;
		myFileDialog->m_ofn.lpstrTitle = (LPCTSTR)title;
		
		
		if (myFileDialog->DoModal() == IDOK) 
		{
			
			CString FileName; //=myFileDialog->GetPathName();		
			TCHAR ddir[5000];// Destiantion "/folder1/folder2"
			//TCHAR dest[5000];//  

			GetDlgItemText(IDC_PATH,ddir,2048);

			CDlg_CopyFiles dlg;

			dlg._path = ddir;
			dlg._fsys = &_FSys;
			dlg._filesListIn.RemoveAll();

			
			//AfxGetApp()->BeginWaitCursor();//  Меняет курсор на песочные часы

			POSITION pos = myFileDialog->GetStartPosition();

			while ( pos  )
			{
				FileName =myFileDialog->GetNextPathName(pos) ;
				dlg._filesListIn.Add(FileName);				
			}

			dlg.DoModal();	
			
			
			
			if(!_tcslen(ddir))// если тут: ""
			{
				_tcscpy(ddir,"/");// Делаю так: "/" ;
			}
			
			load_list_files(ddir,ff_tree,ff_list,ff_img); // загрузить список файлов  
			
			//AfxGetApp()->EndWaitCursor();// меняет курсор обратно
		}
	}//	 End of "if(!_FSys._readOnly)"
	else
	{
		// Предупредить польозователя
		 AfxMessageBox(LS(IDS_READONLY),MB_ICONINFORMATION);
	}
}


// Вызывается когда в список кидают файлы Drag'n'Drop- ом
void CDlg_DiskBrowserMain::OnDropFiles(HDROP hDropInfo) 
{
	
	if(false == _FSys._readOnly)
	{
		UINT  uNumFiles;
		TCHAR szNextFile [MAX_PATH];
		
		// Get the # of files being dropped.
		uNumFiles = DragQueryFile ( hDropInfo, -1, NULL, 0 );
		
		TCHAR ddir[5000];//  "/folder1/folder2"
		TCHAR dest[5000];//  

		if (_ImageOpened == false)
		{
			//CDlg_Password dlg;// Зодание диалога воода пароля				
			DragQueryFile ( hDropInfo, 0, szNextFile, MAX_PATH );
			//dlg.m_FullPath=szNextFile;			
			//dlg.DoModal();
			_cmdline_image = szNextFile;
			OnOpenDisk();
			return;
		}
		
		GetDlgItemText(IDC_PATH,ddir,2048); // get current Virtual Disk path that user browsing now

		CDlg_CopyFiles dlg;

		dlg._path = ddir;
		dlg._fsys = &_FSys;
		dlg._filesListIn.RemoveAll();
		
		for ( UINT uFile = 0; uFile < uNumFiles; uFile++ )
		{
			// Get the next filename from the HDROP info.
			if ( DragQueryFile ( hDropInfo, uFile, szNextFile, MAX_PATH ) > 0 )
			{
				dlg._filesListIn.Add(szNextFile);				
				//::MessageBox(0, szNextFile, "", 0);
			}
		}

		//AfxMessageBox("22");

		dlg.DoModal();
		
		if(!_tcslen(ddir))
		{
			_tcscpy(ddir,"/");
		}

		load_list_files(ddir,ff_tree,ff_list,ff_img); // загрузить список файлов  

		// 

		HTREEITEM hti, htt=0;
		hti = ff_tree.GetSelectedItem();
		ff_tree.SetRedraw(false);
		do 
		{			
			ff_tree.DeleteItem(htt);
			htt=ff_tree.GetChildItem(hti);
		} while(htt);

		make_folder_tree(&PT_TOK,ff_tree);
		ff_tree.SetRedraw(true);
		//OnSelchangedTree(NULL,NULL);
		
		// Free up memory.
		DragFinish ( hDropInfo );	
		
		CDialog::OnDropFiles(hDropInfo);

	}//if(!_FSys._readOnly)
	else
	{
		// Предупредить польозователя
		AfxMessageBox(LS(IDS_READONLY),MB_ICONINFORMATION);
	}
}

BOOL CDlg_DiskBrowserMain::PreTranslateMessage(MSG* pMsg) 
{

	if (pMsg->message == WM_DROPFILES && pMsg->hwnd != m_hWnd)
		OnDropFiles(  (HDROP) (WPARAM)pMsg->wParam );

	return CDialog::PreTranslateMessage(pMsg);
}

// deletes selected files... 
//--------------------------------------------------------------
void CDlg_DiskBrowserMain::OnDeleteFile()
{ 
	TCHAR ddir[5000];
	char buf[1024]={0};
	int n;

	
	if (_FSys._readOnly)
	{
		AfxMessageBox(LS(IDS_READONLY), MB_ICONINFORMATION);
		return;
	}

	if ( ff_list.GetSelectedCount() >0) {

		CString s1;
		s1.Format("Deleting %d files. %s", ff_list.GetSelectedCount(), LS(IDS_AREYOU_SURE) );

		if ( AfxMessageBox(s1, MB_ICONQUESTION | MB_YESNO) == IDNO )
			return;	

	}
	
	POSITION pos = ff_list.GetFirstSelectedItemPosition();
	
	int nItem ;
	int nFiles  =0; 

	TCHAR curr_vpath[500];		
		
		while (pos)
		{
			nItem = ff_list.GetNextSelectedItem(pos);
			ff_list.GetItemText(nItem,0,buf,500);
			//GetDlgItemText(IDC_PATH, curr_vpath, 490);
			_tcscpy(curr_vpath, PT_TOK.pth);		
			_tcscat(curr_vpath, "/");	 
			_tcscat(curr_vpath, buf);		

			if ( !_FSys.isDirectory(curr_vpath))
				_FSys.Delete(curr_vpath);
				
			
			nFiles++;			
		}


/*
	// Найти имя выбранного файла
	{
		n = ff_list.GetNextItem(-1,LVNI_SELECTED);
		if(n==-1)return;		
		ff_list.GetItemText(n,0,buf,1024);
	}
	
	// Path to be deleted: "/folder1/folder2"
	{
		GetDlgItemText(IDC_PATH,ddir,2048);
		_tcscat(ddir, "/" ); // "folder1/folder2/"	
		_tcscat(ddir, buf ); // "folder1/folder2/filename.ext"				
	}
	
	// Удалить файл
	_FSys.Delete(ddir);
	*/
	
	
	// Обновить список
	{
		GetDlgItemText(IDC_PATH,ddir,2048);
		if(!_tcslen(ddir))// если тут: ""
		{
			_tcscpy(ddir,"/");// Делаю так: "/" ;
		}
		
		load_list_files(ddir,ff_tree,ff_list,ff_img); // загрузить список файлов 
	}
}

void CDlg_DiskBrowserMain::WriteDefaultLangFile()
{
	char s1[] = {"[Partition]+\x0D\x0A\
	[Open	<Ctrl+O>]-\x0D\x0A\
	[Information	<Ctrl+I>]-\x0D\x0A\
	[Change Password	<Ctrl+P>]-\x0D\x0A\
	[Search 	<Ctrl+F>]-\x0D\x0A\
	[-]-\x0D\x0A\
	[Exit	<Alt+F4>]/\x0D\x0A\
\x0D\x0A\
[File]+\x0D\x0A\
         [Preview	<Enter>]-\x0D\x0A\
         [Open	<Double Click>]-\x0D\x0A\
         [Save	<CTRL+S>]-\x0D\x0A\
	 [Delete	<Ctrl+D>]-\x0D\x0A\
         [-]-\x0D\x0A\
         [Properties    <Ctrl+P>]/\x0D\x0A\
\x0D\x0A\
[Settings]+\x0D\x0A\
	[View]+\x0D\x0A\
	[Icon]-\x0D\x0A\
	[List]/\x0D\x0A\
	[-]-\x0D\x0A\
	[Language	<Ctrl+L>]/\x0D\x0A\
\x0D\x0A\
\x0D\x0A\
[Help]+\x0D\x0A\
[Help context	<F1>]-\x0D\x0A\
[-]-\x0D\x0A\
[About...	<F2>]/\x0D\x0A\
\x0D\x0A\
[#(#)#]\x0D\x0A\
\x0D\x0A\
[142=Properties]\x0D\x0A\
{\x0D\x0A\
(1011)=Statistic\x0D\x0A\
(1013)=Size\x0D\x0A\
}\x0D\x0A\
\x0D\x0A\
[141=Search]\x0D\x0A\
{\x0D\x0A\
(1011)=Expression\x0D\x0A\
(1008)=Search\x0D\x0A\
(1009)=Goto\x0D\x0A\
}\x0D\x0A\
\x0D\x0A\
[145=Copying]\x0D\x0A\
{\x0D\x0A\
(1018)=Waiting....\x0D\x0A\
}\x0D\x0A\
\x0D\x0A\
[#(#)#]\x0D\x0A\
\x0D\x0A\
{{Folder: %s}}\x0D\x0A\
\x0D\x0A\
{{File: %s   Size: %u}}\x0D\x0A\
\x0D\x0A\
{{The image file '%s' is in use. Please disconnect the Rohos virtual drive and try again.}}\x0D\x0A\
\x0D\x0A\
{{The image file you selected was not formated with NTFS. This program can work only with NTFS volumes.}}\x0D\x0A\
\x0D\x0A\
{{Do you want to empty temporary files folder with encrypted files?}}\x0D\x0A\
\x0D\x0A\
{{This file will be temporary encrypted to the program directory, and opened with the associated Application.}}\x0D\x0A\
\x0D\x0A\
{{Not enough free space on drive %s}}\x0D\x0A\
\x0D\x0A\
{{Enter password:}}\x0D\x0A\
\x0D\x0A\
{{The password is incorrect!}}\x0D\x0A\
\x0D\x0A\
{{Don't show this message next time.}}\x0D\x0A\
\x0D\x0A\
{{Disk image path:}}\x0D\x0A\
\x0D\x0A\
{{Create new folder}}\x0D\x0A\
\x0D\x0A\
{{Import file}}\x0D\x0A\
\x0D\x0A\
{{Opened read only.}}\x0D\x0A\
\x0D\x0A\
{{Not Supported for NTFS.}}\x0D\x0A\
\x0D\x0A\
{{Error loading volume.}}\x0D\x0A\
\x0D\x0A\
{{Error creating file.}}\x0D\x0A\
\x0D\x0A\
{{Error Opening file.}}\x0D\x0A\
\x0D\x0A\
{{Error reading file.}}\x0D\x0A\
\x0D\x0A\
{{GetFileTime error.}}\x0D\x0A\
\x0D\x0A\
{{Opened for Writing.}}\x0D\x0A\
\x0D\x0A\
{{Evaluation Time Has Expired. Please Register.}}\x0D\x0A\
"};


	TCHAR s2[500];

	GetPodNogamiPath(s2, 0);
	strcat(s2, "\\english.txt");

		CFile f(s2, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary );
		f.Write((LPCTSTR)s1, strlen(s1));				
		f.Close();

}

#include "afxole.h"
class COleDataSourceEx: public COleDataSource
{
	BOOL OnRenderGlobalData( LPFORMATETC lpFormatEtc, HGLOBAL* phGlobal );
public:
	HGLOBAL hgDrop;
	BOOL hgBool;

};

WORD g_uCustomClipbrdFormat;

void CDlg_DiskBrowserMain::OnLvnBegindragList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLISTVIEW pNMLV = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	// TODO: Add your control notification handler code here
	*pResult = 0;


//NMLISTVIEW*    pNMLV = (NMLISTVIEW*) pNMHDR;
COleDataSourceEx datasrc;
HGLOBAL        hgDrop;
DROPFILES*     pDrop;
CStringList    lsDraggedFiles;
POSITION       pos;
int            nSelItem;
CString        sFile;
UINT           uBuffSize = 0;
TCHAR*         pszBuff;
FORMATETC      etc = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };

    *pResult = 0;   // return value ignored
	//g_uCustomClipbrdFormat = 

    // For every selected item in the list, put the filename into lsDraggedFiles.

    pos = ff_list.GetFirstSelectedItemPosition();

    while ( NULL != pos )
        {
        nSelItem = ff_list.GetNextSelectedItem ( pos );
        sFile = ff_list.GetItemText ( nSelItem, 0 );

        lsDraggedFiles.AddTail ( sFile );

        // Calculate the # of chars required to hold this string.

        uBuffSize += lstrlen ( sFile ) + 1;
        }

    // Add 1 extra for the final null char, and the size of the DROPFILES struct.

    uBuffSize = sizeof(DROPFILES) + sizeof(TCHAR) * (uBuffSize + 1);

    // Allocate memory from the heap for the DROPFILES struct.

    hgDrop = GlobalAlloc ( GHND | GMEM_SHARE, uBuffSize );

    if ( NULL == hgDrop )
        return;

    pDrop = (DROPFILES*) GlobalLock ( hgDrop );

    if ( NULL == pDrop )
        {
        GlobalFree ( hgDrop );
        return;
        }

    // Fill in the DROPFILES struct.

    pDrop->pFiles = sizeof(DROPFILES);

#ifdef _UNICODE
    // If we're compiling for Unicode, set the Unicode flag in the struct to
    // indicate it contains Unicode strings.

    pDrop->fWide = TRUE;
#endif;

    // Copy all the filenames into memory after the end of the DROPFILES struct.

    pos = lsDraggedFiles.GetHeadPosition();
    pszBuff = (TCHAR*) (LPBYTE(pDrop) + sizeof(DROPFILES));

    while ( NULL != pos )
        {
        lstrcpy ( pszBuff, (LPCTSTR) lsDraggedFiles.GetNext ( pos ) );
        pszBuff = 1 + _tcschr ( pszBuff, '\0' );
        }

    GlobalUnlock ( hgDrop );

    // Put the data in the data source.

    datasrc.hgDrop = hgDrop;
    datasrc.DelayRenderData( CF_HDROP, &etc );

    // Add in our own custom data, so we know that the drag originated from our
    // window.  CMyDropTarget::DragEnter() checks for this custom format, and
    // doesn't allow the drop if it's present.  This is how we prevent the user
    // from dragging and then dropping in our own window.
    // The data will just be a dummy bool.
    // Infact we'r never using the hgBool

	/*HGLOBAL hgBool;

    hgBool = GlobalAlloc ( GHND | GMEM_SHARE, sizeof(bool) );

    if ( NULL == hgBool )
        {
        GlobalFree ( hgDrop );
        return;
        }

    // Put the data in the data source.

    etc.cfFormat = g_uCustomClipbrdFormat;
    datasrc.hgBool = hgBool;
    datasrc.DelayRenderData ( g_uCustomClipbrdFormat, &etc );
	*/

    // Start the drag 'n' drop!

DROPEFFECT dwEffect = datasrc.DoDragDrop ( DROPEFFECT_COPY | DROPEFFECT_MOVE );

    // If the DnD completed OK, we remove all of the dragged items from our
    // list.

    switch ( dwEffect )
        {
        case DROPEFFECT_COPY:
        case DROPEFFECT_MOVE:
            {
           /* // Note: Don't call GlobalFree() because the data will be freed by the drop target.

            for ( nSelItem = c_FileList.GetNextItem ( -1, LVNI_SELECTED );
                  nSelItem != -1;
                  nSelItem = c_FileList.GetNextItem ( nSelItem, LVNI_SELECTED ) )
                {
                c_FileList.DeleteItem ( nSelItem );
                nSelItem--;
                }

            // Resize the list columns.

            c_FileList.SetColumnWidth ( 0, LVSCW_AUTOSIZE_USEHEADER );
            c_FileList.SetColumnWidth ( 1, LVSCW_AUTOSIZE_USEHEADER );
            c_FileList.SetColumnWidth ( 2, LVSCW_AUTOSIZE_USEHEADER );*/
            }
        break;

        case DROPEFFECT_NONE:
            {
				int i =1;
            // This needs special handling, because on NT, DROPEFFECT_NONE
            // is returned for move operations, instead of DROPEFFECT_MOVE.
            // See Q182219 for the details.
            // So if we're on NT, we check each selected item, and if the
            // file no longer exists, it was moved successfully and we can
            // remove it from the list.

           /*if ( g_bNT )
                {
                bool bDeletedAnything = false;

                for ( nSelItem = c_FileList.GetNextItem ( -1, LVNI_SELECTED );
                      nSelItem != -1;
                      nSelItem = c_FileList.GetNextItem ( nSelItem, LVNI_SELECTED ) )
                    {
                    CString sFilename = c_FileList.GetItemText ( nSelItem, 0 );

                    if ( 0xFFFFFFFF == GetFileAttributes ( sFile ) &&
                         GetLastError() == ERROR_FILE_NOT_FOUND )
                        {
                        // We couldn't read the file's attributes, and GetLastError()
                        // says the file doesn't exist, so remove the corresponding
                        // item from the list.

                        c_FileList.DeleteItem ( nSelItem );
                   
                        nSelItem--;
                        bDeletedAnything = true;
                        }
                    }

                // Resize the list columns if we deleted any items.

                if ( bDeletedAnything )
                    {
                    c_FileList.SetColumnWidth ( 0, LVSCW_AUTOSIZE_USEHEADER );
                    c_FileList.SetColumnWidth ( 1, LVSCW_AUTOSIZE_USEHEADER );
                    c_FileList.SetColumnWidth ( 2, LVSCW_AUTOSIZE_USEHEADER );

                    // Note: Don't call GlobalFree() because the data belongs to
                    // the caller.
                    }
                else
                    {
                    // The DnD operation wasn't accepted, or was canceled, so we
                    // should call GlobalFree() to clean up.

                    GlobalFree ( hgDrop );
                    GlobalFree ( hgBool );
                    }
                }   // end if (NT)*/
            
            }
        break;  // end case DROPEFFECT_NONE
        }   // end switch
}


//Implementation of COleDataSourceEx::OnRenderGlobalData
//COleDataSourceEx contains two member variables


BOOL COleDataSourceEx::OnRenderGlobalData( LPFORMATETC lpFormatEtc, HGLOBAL* phGlobal )
{
	if( lpFormatEtc->cfFormat == CF_HDROP )
	{
		*phGlobal = hgDrop;
		return (phGlobal != NULL );
	}
	return FALSE;
}

/** Create New Disk 
*/
void CDlg_DiskBrowserMain::OnNewDisk(void)
{
	
}


void CDlg_DiskBrowserMain::OnPartitionNew()
{
	CDlg_NewDisk1 dlg;

	if ( dlg.DoModal() == IDOK)
	{
		// mount / включить новый образ 

		pDiskBlob= &blob;
		blob.pbData=NULL;

		_FSys.CloseVolume();

		if (!pDiskBlob->pbData) {
			pDiskBlob->cbData = sizeof( DISK_BLOB_INFO );
			pDiskBlob->pbData = new BYTE[pDiskBlob->cbData+64];
			strcpy((char*)pDiskBlob->pbData, "this disk bloc");
		}		

		// копируем расшифрованный блоб 
		pDiskBlob->cbData  = sizeof( DISK_BLOB_INFO );
		memcpy( pDiskBlob->pbData, &dlg.pKeyInfo, pDiskBlob->cbData );

		_tcscpy(_OpenedDiskName, dlg.dlgOptions._path);

		if (dlg.dlgOptions._partition_based)
		{	
			// opens partition of this drive			
			_OpenedDiskName[3] =0;	
		}

		DWORD data_offset_mb = MAKELONG(((DISK_BLOB_INFO*)pDiskBlob->pbData)->Reserved1[5], ((DISK_BLOB_INFO*)pDiskBlob->pbData)->Reserved1[6] )/*in MB*/ ; 

		DWORD data_end_offset_mb =0;

		if (data_offset_mb == 0 && IsFileUseSteganos(_OpenedDiskName) )
		{
			GetSteganosOffset(_OpenedDiskName, &data_offset_mb, &data_end_offset_mb);
		}

		WriteLog("%s %d", _OpenedDiskName, data_offset_mb);	

		if(_FSys.OpenVolume(_OpenedDiskName, true, data_offset_mb ) )
		{
			_ImageOpened=true;
		}		
		
		free_path_token(&PT_TOK);

		path_down(&PT_TOK,"/");
		path_up(&PT_TOK);
		load_list_files("/",ff_tree,ff_list,ff_img);
		
		ff_tree.DeleteAllItems();
		make_folder_tree(&PT_TOK,ff_tree, _OpenedDiskName);		

		SetDlgItemText(IDC_PATH, PT_TOK.pth);  
	}

	// TODO: Add your command handler code here
}

void CDlg_DiskBrowserMain::OnUpdateMenu(CCmdUI *pCmdUI)
{
	// TODO: Add your command update UI handler code here

	if (pCmdUI->m_nID ==ID_PARTITION_RESIZE /*|| pCmdUI->m_nID == ID_PARTITION_CHANGEPASSWORD*/ )
		pCmdUI->Enable(FALSE);
	else
		pCmdUI->Enable(_ImageOpened);

	if (pCmdUI->m_nID == ID_PARTITION_INFORMATION || pCmdUI->m_nID == ID_PARTITION_REPAIR)
	{
		if(GetFileNameOfOpenedImage().GetLength()!=0)// Если образ диска открыт
			pCmdUI->Enable(TRUE);
	}

}

void CDlg_DiskBrowserMain::OnInitMenuPopup(CMenu* pPopupMenu, UINT nIndex, BOOL bSysMenu)
{
	//CDialog::OnInitMenuPopup(pPopupMenu, nIndex, bSysMenu);

	  ASSERT(pPopupMenu != NULL);
    // Check the enabled state of various menu items.

    CCmdUI state;
    state.m_pMenu = pPopupMenu;
    ASSERT(state.m_pOther == NULL);
    ASSERT(state.m_pParentMenu == NULL);

    // Determine if menu is popup in top-level menu and set m_pOther to
    // it if so (m_pParentMenu == NULL indicates that it is secondary popup).
    HMENU hParentMenu;
    if (AfxGetThreadState()->m_hTrackingMenu == pPopupMenu->m_hMenu)
        state.m_pParentMenu = pPopupMenu;    // Parent == child for tracking popup.
    else if ((hParentMenu = ::GetMenu(m_hWnd)) != NULL)
    {
        CWnd* pParent = this;
           // Child windows don't have menus--need to go to the top!
        if (pParent != NULL &&
           (hParentMenu = ::GetMenu(pParent->m_hWnd)) != NULL)
        {
           int nIndexMax = ::GetMenuItemCount(hParentMenu);
           for (int nIndex = 0; nIndex < nIndexMax; nIndex++)
           {
            if (::GetSubMenu(hParentMenu, nIndex) == pPopupMenu->m_hMenu)
            {
                // When popup is found, m_pParentMenu is containing menu.
                state.m_pParentMenu = CMenu::FromHandle(hParentMenu);
                break;
            }
           }
        }
    }

    state.m_nIndexMax = pPopupMenu->GetMenuItemCount();
    for (state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax;
      state.m_nIndex++)
    {
        state.m_nID = pPopupMenu->GetMenuItemID(state.m_nIndex);
        if (state.m_nID == 0)
           continue; // Menu separator or invalid cmd - ignore it.

        ASSERT(state.m_pOther == NULL);
        ASSERT(state.m_pMenu != NULL);
        if (state.m_nID == (UINT)-1)
        {
           // Possibly a popup menu, route to first item of that popup.
           state.m_pSubMenu = pPopupMenu->GetSubMenu(state.m_nIndex);
           if (state.m_pSubMenu == NULL ||
            (state.m_nID = state.m_pSubMenu->GetMenuItemID(0)) == 0 ||
            state.m_nID == (UINT)-1)
           {
            continue;       // First item of popup can't be routed to.
           }
           state.DoUpdate(this, TRUE);   // Popups are never auto disabled.
        }
        else
        {
           // Normal menu item.
           // Auto enable/disable if frame window has m_bAutoMenuEnable
           // set and command is _not_ a system command.
           state.m_pSubMenu = NULL;
           state.DoUpdate(this, FALSE);
        }

        // Adjust for menu deletions and additions.
        UINT nCount = pPopupMenu->GetMenuItemCount();
        if (nCount < state.m_nIndexMax)
        {
           state.m_nIndex -= (state.m_nIndexMax - nCount);
           while (state.m_nIndex < nCount &&
            pPopupMenu->GetMenuItemID(state.m_nIndex) == state.m_nID)
           {
            state.m_nIndex++;
           }
        }
        state.m_nIndexMax = nCount;
    }
}

// enable/disable commands for File Operations 
void CDlg_DiskBrowserMain::OnUpdateMenuFile(CCmdUI *pCmdUI)
{
	if (/*pCmdUI->m_nID == ID_FILE_OPENWITH || pCmdUI->m_nID ==ID_FILE_RENAME ||*/ pCmdUI->m_nID == ID_A_NEWFOLDER )
	{
		pCmdUI->Enable(TRUE);
		return;

	}

	if (!_ImageOpened)
		pCmdUI->Enable(FALSE);
	else
	{
		// get a selected item if any 
		if ( -1 == ff_list.GetNextItem(-1, LVNI_SELECTED) )
			pCmdUI->Enable(FALSE);
		else 
			pCmdUI->Enable(TRUE);


	}
}

void CDlg_DiskBrowserMain::OnCheckUpdates()
{
	ShellExecute(m_hWnd, "Open", "http://www.rohos.com/category/rohos-mini/", "", "", SW_SHOWDEFAULT);
	// TODO: Add your command handler code here
}



int CDlg_DiskBrowserMain::OnOpenAs(void)
{

	int n;
	char buf[1024]={0};


	n = ff_list.GetNextItem(-1,LVNI_SELECTED);
	if(n==-1)
		return 0;

	ff_list.GetItemText(n,0,buf,1024);

	path_down(&PT_TOK,buf);

	WriteLog("Opening file as.");
	_FSys.ShellExecute(PT_TOK.pth, true);
	path_up(&PT_TOK);
	
	return 0;
}

#include ".\SelectDialog.h"

UINT_PTR CALLBACK OFNHookProc(      
    HWND hdlg,
    UINT uiMsg,
    WPARAM wParam,
    LPARAM lParam
)
{

	switch (uiMsg)
	{

	case WM_NOTIFY :
		{
		OFNOTIFY *lpOfNotify = (LPOFNOTIFY) lParam;

		if ( lpOfNotify->hdr.code == CDN_FILEOK)
		{
			if (IsWindowsVista()) // check if run a picture viewer in Vista/7
			{
				LPCTSTR ext = _tcsrchr(lpOfNotify->lpOFN->lpstrFile, '.');
				if (ext)
				{	
					TCHAR extid[350] = {0}; // 
					TCHAR str_exepath[800]= {0};
					ReadReg(HKEY_CLASSES_ROOT, ext, "", extid, 100);

					_tcscat(extid, "\\shell\\open\\command");
					ReadReg(HKEY_CLASSES_ROOT, extid, "", str_exepath, MAX_PATH);

					if (  strlen(str_exepath) && strstr(str_exepath, "PhotoViewer.dll") ) // browse pictures?
					{
						_tcscpy(str_exepath, "rundll32.exe shimgvw.dll,ImageView_Fullscreen ");						
						_tcscat(str_exepath, lpOfNotify->lpOFN->lpstrFile);

						int ret = WinExec(str_exepath, SW_SHOW);
						WriteLog("OFNHookProc run3, %s %d", (LPCTSTR)str_exepath, ret);

						SetWindowLong(hdlg, DWL_MSGRESULT, 1);
						return 1;
					}

				}
			}

			int ret = (int)::ShellExecute(0, "open", lpOfNotify->lpOFN->lpstrFile, "", lpOfNotify->lpOFN->lpstrInitialDir,  SW_SHOW);			
			SetWindowLong(hdlg, DWL_MSGRESULT, 1);
			return 1;
		}

		if ( lpOfNotify->hdr.code == CDN_INITDONE)
		{
			
			HWND wnd = GetParent(hdlg);

			_hwnd_ShowFileDialog = wnd;
			
			// lst2
			CommDlg_OpenSave_HideControl(wnd, IDOK);
			CommDlg_OpenSave_HideControl(wnd, edt1);
			CommDlg_OpenSave_HideControl(wnd, cmb1);
			CommDlg_OpenSave_HideControl(wnd, cmb13);
			CommDlg_OpenSave_HideControl(wnd, stc3);
			CommDlg_OpenSave_HideControl(wnd, stc2);

			/*RECT rectList2, rectCancel; 
			POINT pCancel;
			pCancel.x = 
			GetWindowRect(GetDlgItem(wnd, IDCANCEL), &rectCancel);
			ScreenToClient(wnd, &rectCancel);

			GetWindowRect(GetDlgItem(wnd, lst2), &rectList2);
			ScreenToClient(wnd, &rectList2);
			SetWindowPos(0,0,0, rectList2.bottom, abs( rectList2.top - (rectCancel.top - 5) ), SWP_NOMOVE | SWP_NOZORDER);

			*/

			CWnd *pFD = new CWnd();
			pFD->Attach(wnd);

			CRect rectCancel; 
			pFD->GetDlgItem(IDCANCEL)->GetWindowRect(&rectCancel);
			pFD->ScreenToClient(&rectCancel);

			CRect rectList2; pFD->GetDlgItem(lst1)->GetWindowRect(&rectList2);
			pFD->ScreenToClient(&rectList2);
			pFD->GetDlgItem(lst1)->SetWindowPos(0,0,0,rectList2.Width(), abs(rectList2.top - (rectCancel.top - 5)), SWP_NOMOVE | SWP_NOZORDER);




			//SetTimer(hdlg, 1, 900, NULL);
			return 1;
		}

		break;
		}

	case WM_TIMER:
		KillTimer(hdlg, 1);
		break;
	
	case WM_INITDIALOG :
		showItems(hdlg, 0, IDOK, 0);
		break;

	}

	return 0;

}

UINT CDlg_DiskBrowserMain::ShowFileDialogThread(LPVOID p)
{
	/*CSelectDialog ofd( TRUE, _T("*.*"), NULL,
		OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT,
		_T("All files and folders(*.*)|*.*||") );

	ofd.DoModal();*/

	OPENFILENAME ofn;    

    FillMemory(&ofn,sizeof(ofn),0);
    

	TCHAR filename[500] = "";
	TCHAR curr_folder[500];
	GetPodNogamiPath(curr_folder, false );
	curr_folder[3] = 0; // c: 
	//GetTempPath(450, curr_folder);
	_tcscat(curr_folder, LS(IDS_VIRTUAL_FOLDER) );
	_tcscat(curr_folder, "\\");
	
	CString s2 = _lastVirtualFolder_Path;
	s2.TrimLeft(PATH_SEP);
	s2.Replace(PATH_SEP, '\\'); // replace / wih '\'
	
	_tcscat(curr_folder, s2 );
	//AfxMessageBox(curr_folder);


	ofn.lpstrFile = filename;
    ofn.nMaxFile = 490;
    ofn.lpstrInitialDir = curr_folder;
    ofn.lpstrFilter = "*.*\0*.*\0\0";
	CString s1 = LS(IDS_VIRTUAL_FOLDER);
	ofn.lpstrTitle=s1;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_EXPLORER | OFN_ENABLESIZING | OFN_DONTADDTORECENT | OFN_ALLOWMULTISELECT | OFN_ENABLEHOOK | OFN_HIDEREADONLY ;
	ofn.FlagsEx = OFN_EX_NOPLACESBAR;
	ofn.lpfnHook = OFNHookProc;
	ofn.lStructSize  = sizeof(ofn); 

	ofn.lpstrDefExt="*.*";
	ofn.Flags|=OFN_OVERWRITEPROMPT;
	GetOpenFileName(&ofn);   

	((CDlg_DiskBrowserMain*)p)->pWorkerThread1 = NULL;

	return 0;
	
}

// if no selection in List - virtualize all files in current folder
// if the item is selected - virtualize just 1 file or folder with subfolders.
void CDlg_DiskBrowserMain::OnAVirtualfolder()
{
	/*if ( _FSys.boxed_app )
	{
		if (pWorkerThread1 == NULL)

		pWorkerThread1 = AfxBeginThread( ShowFileDialogThread, NULL,
                                   THREAD_PRIORITY_NORMAL, 0, 0 );


	} */

	CDlg_CopyFiles dlg;
	//dlg._path = tx;
	dlg._fsys = &_FSys;
	TCHAR tx[500], fpath[1500];		
	

	int n = ff_list.GetNextItem(-1,LVNI_SELECTED);

	if(n==-1)
	{
		do // loop throuh the items in current folder
		{
			n = ff_list.GetNextItem(n, LVNI_ALL);
			if(n==-1)
				break;

			if ( ff_list.GetItemData(n) == 0) 
			{ //  virtualize only files
				ff_list.GetItemText(n,0,tx,500);

				GetDlgItemText(IDC_PATH, fpath, 500);
				strcpy(_lastVirtualFolder_Path, fpath); // show this folder in ShowFileDialogThread
				strcat(fpath, PATH_SEP1);
				strcat(fpath, tx);

				dlg._filesListVirtualize.Add(fpath);				
			}

		}while(n!=-1);

		
	}

	else
	{
		// virtualize just 1 folder or file
		
		ff_list.GetItemText(n,0,tx,500);

		GetDlgItemText(IDC_PATH, fpath, 500);
		strcpy(_lastVirtualFolder_Path, fpath); // show this folder in ShowFileDialogThread
		strcat(fpath, PATH_SEP1);
		strcat(fpath, tx);

		//AfxMesageBox(fpath);

		dlg._filesListVirtualize.Add(fpath);	

		switch (_FSys.isDirectory(fpath))
		{
		case 1: // folder
			//AfxGetApp()->BeginWaitCursor();
			strcpy(_lastVirtualFolder_Path, fpath);
			//_FSys.CreateVirtualFolder(fpath);
			//AfxGetApp()->EndWaitCursor();
			break;
		case 0: // file
			//_FSys.CreateVirtualFile(fpath);
			break;
		}

		do // loop throuh the selected items in current folder
		{
			n = ff_list.GetNextItem(n, LVNI_SELECTED);
			
			if ( n >=0) 
			{
				ff_list.GetItemText(n,0,tx,500);

				GetDlgItemText(IDC_PATH, fpath, 500);
				strcpy(_lastVirtualFolder_Path, fpath); // show this folder in ShowFileDialogThread
				strcat(fpath, PATH_SEP1);
				strcat(fpath, tx);

				dlg._filesListVirtualize.Add(fpath);				
			}

		}while(n!=-1);
	
	}
	
	dlg.DoModal(); // do virtualization for all items in dlg._filesListVirtualize

	if (_hwnd_ShowFileDialog)
		::PostMessage(_hwnd_ShowFileDialog, WM_COMMAND, IDCANCEL, 0);

	Sleep(100);


	if (pWorkerThread1 == NULL)
		pWorkerThread1 = AfxBeginThread( ShowFileDialogThread, this,
                                   THREAD_PRIORITY_NORMAL, 0, 0 );

	// TODO: Add your command handler code here
}

void CDlg_DiskBrowserMain::OnPartitionRepair()
{

	CDlg_CheckDsk dlg;

	if (_FSys._readOnly)
	{
		AfxMessageBox(LS(IDS_MOUNT_AS_READONLY), MB_ICONINFORMATION);
		return;
	}


	//Dinf.pKeyInfo = (DISK_BLOB_INFO*)pDiskBlob->pbData;

	if (_ImageOpened == false &&  GetFileNameOfOpenedImage().GetLength() == 0)
		return;

	
	dlg._image_readonly = _FSys._readOnly;
	if (_FSys._isNTFS)
	{
		wcscpy(dlg._filesystem, L"NTFS");
		if (_FSys.ntfs_volume)
			dlg._hImageFilehandle = (HANDLE)_FSys.ntfs_volume->dev->d_handle ;
	}
	else
	{
		wcscpy(dlg._filesystem, L"FAT32");
		dlg._hImageFilehandle = _FSys.fatvolume_handle;
	}

	if (_ImageOpened == false &&  GetFileNameOfOpenedImage().GetLength() > 0)
	{
		WriteLog("unknown volume");
		wcscpy(dlg._filesystem, L"NTFS");
		dlg._hImageFilehandle = CreateFile(GetFileNameOfOpenedImage(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);	 ;		

	}	
	

	dlg.pDiskPartitionClass = _FSys._pDiskPartitionClass;
	dlg.encrypted = _FSys._encrypted_disk;

	dlg._disk_size.HighPart = ((DISK_BLOB_INFO*)pDiskBlob->pbData)->DiskSizeHigh;
	dlg._disk_size.LowPart= ((DISK_BLOB_INFO*)pDiskBlob->pbData)->DiskSizeLow;

	int data_offset_mb = MAKELONG(((DISK_BLOB_INFO*)pDiskBlob->pbData)->Reserved1[5], ((DISK_BLOB_INFO*)pDiskBlob->pbData)->Reserved1[6] )/*in MB*/ ; 

	dlg.disk_data_offset_in_mb = data_offset_mb;

	

	dlg.DoModal();

	if (dlg._re_formated || _ImageOpened == false)
	{
		if (_ImageOpened == false)
			CloseHandle(dlg._hImageFilehandle);

		_FSys.CloseVolume();


		int data_offset_mb = MAKELONG(pKeyInfo->Reserved1[5], pKeyInfo->Reserved1[6] )/*in MB*/ ; 
	

		WriteLog("check done. %s %d", _OpenedDiskName, data_offset_mb);	

		if(_FSys.OpenVolume(GetFileNameOfOpenedImage(), true, data_offset_mb) )
		{
			_ImageOpened=true;
		}
		
		free_path_token(&PT_TOK);
		path_down(&PT_TOK,"/");
		path_up(&PT_TOK);
		load_list_files("/",ff_tree,ff_list,ff_img);
		
		ff_tree.DeleteAllItems();
		make_folder_tree(&PT_TOK,ff_tree, _OpenedDiskName);

		SetDlgItemText(IDC_PATH, PT_TOK.pth);  
		


	}
}

#define PD_BLOCK_SIZE		16

void CDlg_DiskBrowserMain::OnChangePassword()
{
	CDlg_ChangePassw dlg;

	
	BYTE key[KEY_SIZE+16] = "";
	BYTE iv[IV_SIZE+16] = "";

	memcpy(dlg.file_blob, file_blob,	file_blob_size);
	dlg.file_blob_size = file_blob_size;

	if ( dlg.DoModal() == IDOK )
	{
		// current password is OK
		

		// update info structures of Opened Disk by using current password
		if ( !setBlob(file_blob, file_blob_size, dlg._passw) )	
		{
			WriteLog("error setBlob");
			AfxMessageBox(LS(IDS_ERR_CHANGEPASSW_X), MB_ICONWARNING);
			return;
		}

		BYTE buff[10000];
		DWORD buff_len;

		memset( iv, 0, IV_SIZE );
		memset( key, 0, KEY_SIZE );	

		

		if ( !DeriveKeyFromPassw_v2((LPCTSTR)dlg._new_passw, key) ) {		
			return ;
		}	

		// create new blob
		memcpy(buff, pKeyInfo, sizeof(DISK_BLOB_INFO));	
		buff_len = sizeof(DISK_BLOB_INFO);				
		

		if( !EncryptBuffer( buff, &buff_len, sizeof(DISK_BLOB_INFO) + PD_BLOCK_SIZE ,key, iv ) ) {
			WriteLog("error EncryptBuffer");
			AfxMessageBox(LS(IDS_ERR_CHANGEPASSW_X), MB_ICONWARNING);
			return ;
		}	
		

		// write blob back to disk image file. + to backup file
		if ( !AddDiskBlobToContainer(_OpenedDiskName, buff, buff_len) )
		{
			//WriteLog("error AddDiskBlobToContainer");
			AfxMessageBox(LS(IDS_ERR_CHANGEPASSW_X), MB_ICONWARNING);
			return;
		}

		AfxMessageBox(LS(IDS_SUCC_CHANGE_DISKPASS), MB_ICONINFORMATION);

	}
}

#define PATH_SEPP	"/"

void CDlg_DiskBrowserMain::OnLvnEndlabeleditList(NMHDR *pNMHDR, LRESULT *pResult)
{
	NMLVDISPINFO *pDispInfo = reinterpret_cast<NMLVDISPINFO*>(pNMHDR);
	// TODO: Add your control notification handler code here

	if (_FSys._readOnly){
		AfxMessageBox(LS(IDS_MOUNT_AS_READONLY), MB_ICONINFORMATION);
		*pResult = 0;
		return;

	}

	*pResult = 0;

	TCHAR new_item_name[500] = {0};
	TCHAR new_full_item_name[500] = {0};

	if (pDispInfo->item.pszText)
		_tcscpy(new_item_name, pDispInfo->item.pszText);	
	else 
		return;	

	ff_list.GetItemText(pDispInfo->item.iItem,0,edited_item_text,100);

	_tcscpy(new_full_item_name, PT_TOK.pth); 	
	if (_tcslen(new_full_item_name))
		_tcscat(new_full_item_name, PATH_SEPP);
	_tcscat(new_full_item_name, new_item_name);

	path_down(&PT_TOK, edited_item_text);	

	WriteLog("rename %s to %s", edited_item_text, new_item_name);

	if ( _FSys.RenameItem(PT_TOK.pth, new_full_item_name) )
			*pResult = 1; // alow change name

	/*switch(_FSys.isDirectory(PT_TOK.pth))
	{
	case 1: // directory			
		
		else 

		break;

	case 0: // file	
		break;

	case 3:	// error		

		break;
	}*/

	path_up(&PT_TOK);

	
}


// start inplace editiong of selected item folder
// 
void CDlg_DiskBrowserMain::OnFileRename()
{
	int n = ff_list.GetNextItem(-1,LVNI_SELECTED);
	if(n==-1)
		return;

	ff_list.EditLabel(n);
	
}

bool IsWindowsSeven();
bool IsWindowsVista2();
bool IsWindows8();

// creates new folder "New Folder (#)" and start inplace editiong of this folder
// 
void CDlg_DiskBrowserMain::OnNewfolder()
{

	TCHAR new_folder_name[100];
	int i=0;

	if (_FSys._readOnly)
	{
		AfxMessageBox(LS(IDS_READONLY), MB_ICONINFORMATION);
		return;
	}
	
	do {
		_tcscpy(new_folder_name, MUI(IDS_NEW_FOLDER) );

		if (i)
		{
			_stprintf(new_folder_name, TEXT("%s (%d)"),MUI(IDS_NEW_FOLDER), i );			
		}

		path_down(&PT_TOK, new_folder_name);	

		if (_FSys.isDirectory(PT_TOK.pth) ==3)		
		{			
			path_up(&PT_TOK);
			break;
		}
		
		i++;
		path_up(&PT_TOK);

	} while (  /*path not exist*/ true);


	if ( _FSys.CreateDirectory(PT_TOK.pth, new_folder_name) == 0 )
	{
		AfxMessageBox(MUI(IDS_ERR_WHILE_CREATE_FOLDER) );
		return;
	}

	// add list item, and begin its edit

	// For Forlders

	bool is_vista = IsWindowsVista2();
	bool is_seven = IsWindowsSeven();

	LVITEM lvi;

	lvi.mask = LVIF_TEXT | LVIF_IMAGE;// | LVIF_PARAM | LVIF_STATE; 
	lvi.state = 0; 
	lvi.stateMask = 0; 
	lvi.iImage = 0;
	lvi.iSubItem = 0;

	lvi.iItem = i;
	
	lvi.iImage = 4;

	if ( is_vista )
		lvi.iImage = 6; 

	if ( is_seven )
		lvi.iImage = 1; 

	// Folder Icon = 4
	lvi.pszText = new_folder_name;  //LPSTR_TEXTCALLBACK
	lvi.lParam = 1; // folder
	//list.InsertItem(LVIF_TEXT | LVIF_STATE, i, ff.fld[i].ans_name);

	int itm = ff_list.InsertItem(&lvi);
	ff_list.SetItemData(itm, 1/*folder*/);

	ff_list.EditLabel(itm);

	
	
}

void CDlg_DiskBrowserMain::OnLvnKeydownList(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMLVKEYDOWN pLVKeyDow = reinterpret_cast<LPNMLVKEYDOWN>(pNMHDR);
	// TODO: Add your control notification handler code here

	*pResult = 0;
	
	if (pLVKeyDow->wVKey == VK_F2)
	{
		int n = ff_list.GetNextItem(-1,LVNI_SELECTED);
		if(n==-1)
			return;

		ff_list.EditLabel(n);

		*pResult = 1;
		return;
	}

	

	if (pLVKeyDow->wVKey == 'A' && GetKeyState(VK_CONTROL) < 0 )
	{	
		for(int i = 0; i < ff_list.GetItemCount(); i++)
			ff_list.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);	
		return;
	}

	if (pLVKeyDow->wVKey == 'C' && GetKeyState(VK_CONTROL) < 0 )
	{	
		CString all_items;
		for(int i = 0; i < ff_list.GetItemCount(); i++)
		{
			if (ff_list.GetItemState( i, LVIS_SELECTED) ==  LVIS_SELECTED )
			{
				all_items += ff_list.GetItemText(i, 0);
				all_items += "\x0D\x0A";
			}
		}

		// Open the clipboard, and empty it. 

		if (!OpenClipboard()) 
			return ; 
		EmptyClipboard(); 

		// Allocate a global memory object for the text. 

		int cch = all_items.GetLength();		

		HGLOBAL hglbCopy; 
		hglbCopy = GlobalAlloc(GMEM_MOVEABLE, 
			(cch + 1) * sizeof(TCHAR)); 
		if (hglbCopy == NULL) 
		{ 
			CloseClipboard(); 
			return ; 
		} 

		// Lock the handle and copy the text to the buffer. 

		LPTSTR lptstrCopy = (LPTSTR) GlobalLock(hglbCopy); 
		memcpy(lptstrCopy, (LPCTSTR)all_items, cch * sizeof(TCHAR)); 
		lptstrCopy[cch] = (TCHAR) 0;    // null character 
		GlobalUnlock(hglbCopy); 

		// Place the handle on the clipboard. 

		SetClipboardData(CF_TEXT, hglbCopy); 
		  CloseClipboard(); 
		return;
	}

	if (pLVKeyDow->wVKey == 'V' && GetKeyState(VK_CONTROL) < 0 )
	{
		// paste files here ...
		
		HDROP lnHdrop;

		//AfxMessageBox("1");

			if ( OpenClipboard() )
			{
				lnHdrop = (HDROP)GetClipboardData(15);

				
				
				if (lnHdrop)
				{
					//AfxMessageBox("1");
					OnDropFiles(lnHdrop);
				}
				CloseClipboard(); 
			}
			 
			return;

	}

	if (pLVKeyDow->wVKey == VK_DELETE )
	{
		// paste files here ...		
		OnDeleteFile();

	}


	if (pLVKeyDow->wVKey == VK_BACK ) // go UP level
	{
		if(_ImageOpened)
		{

			TCHAR tx[200];
			GetDlgItemText(IDC_PATH,tx,2048);
			strpath_to_token(tx,&PT_TOK);
			path_up(&PT_TOK);

			switch(_FSys.isDirectory(PT_TOK.pth))
			{
			case 1:
				load_list_files(PT_TOK.pth, ff_tree, ff_list, ff_img);
				select_folder_tree(&PT_TOK, ff_tree);
				//break;			
			}
			
			SetDlgItemText(IDC_PATH, PT_TOK.pth);
			//break;
			
			
		}
		
	}

	
}

void CDlg_DiskBrowserMain::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// TODO: Add your message handler code here and/or call default

	/* WriteLog("OnKeyDown %d", nChar);

	

	if (nChar ==VK_RETURN )
	{
		AfxMessageBox("123");

	}*/


	CDialog::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CDlg_DiskBrowserMain::OnOpenAndPreventDataLeak()
{
	WriteLog("Opening file in isolation.");

	TCHAR buf[1201];
	int n = ff_list.GetNextItem(-1,LVNI_SELECTED);
	if(n==-1)
		return;
	ff_list.GetItemText(n,0,buf,1024);
	
	path_down(&PT_TOK, buf);

	AfxGetApp()->BeginWaitCursor();
	_FSys.ShellExecute(PT_TOK.pth, false, true, false);
	path_up(&PT_TOK);
	AfxGetApp()->EndWaitCursor();

	path_up(&PT_TOK);
}

void CDlg_DiskBrowserMain::OnOpenandVirtualizeFoldertree()
{
	WriteLog("Opening file tree.");

	TCHAR buf[1201];
	int n = ff_list.GetNextItem(-1,LVNI_SELECTED);
	if(n==-1)
		return;
	ff_list.GetItemText(n,0,buf,1024);
	
	path_down(&PT_TOK, buf);

	AfxGetApp()->BeginWaitCursor();
	_FSys.ShellExecute(PT_TOK.pth, false, false, true);
	path_up(&PT_TOK);
	AfxGetApp()->EndWaitCursor();

	path_up(&PT_TOK);
}

void CDlg_DiskBrowserMain::PrepareListView_columns(void)
{

}

BOOL CDlg_DiskBrowserMain::OnToolBarToolTipText(UINT, NMHDR* pNMHDR, LRESULT* pResult)
{
    ASSERT(pNMHDR->code == TTN_NEEDTEXTA || pNMHDR->code == TTN_NEEDTEXTW);

    // if there is a top level routing frame then let it handle the message

    if (GetRoutingFrame() != NULL) return FALSE;

    // to be thorough we will need to handle UNICODE versions of the message also !!

    TOOLTIPTEXTA* pTTTA = (TOOLTIPTEXTA*)pNMHDR;
    TOOLTIPTEXTW* pTTTW = (TOOLTIPTEXTW*)pNMHDR;
    TCHAR szFullText[512];
    CString strTipText;
    UINT nID = pNMHDR->idFrom;

    if (pNMHDR->code == TTN_NEEDTEXTA && (pTTTA->uFlags & TTF_IDISHWND) ||
        pNMHDR->code == TTN_NEEDTEXTW && (pTTTW->uFlags & TTF_IDISHWND))
    {
        // idFrom is actually the HWND of the tool 

        nID = ::GetDlgCtrlID((HWND)nID);
    }

    if (nID != 0) // will be zero on a separator

    {
		switch (nID)
		{
		case TB_BT_UP:   strcpy(szFullText, LS(IDS_OPEN) ); break;
		case TB_BT_LOAD:   strcpy(szFullText, LS(IDS_IMPORT)); break;
		case TB_BT_SAVE:   strcpy(szFullText, LS(IDS_EXPORT)); break;
		case TB_BT_VIEW1:   strcpy(szFullText, "View modes"); break;
		case TB_BT_BACK:   strcpy(szFullText, "Go back"); break;
		case TB_BT_FORWARD:   strcpy(szFullText, "Go forward"); break;
		case TB_BT_UP2:   strcpy(szFullText, "Up"); break;
		}

        //AfxLoadString(nID, szFullText);
        strTipText=szFullText;

#ifndef _UNICODE
        if (pNMHDR->code == TTN_NEEDTEXTA)
        {
            lstrcpyn(pTTTA->szText, strTipText, sizeof(pTTTA->szText));
        }
        else
        {
            _mbstowcsz(pTTTW->szText, strTipText, sizeof(pTTTW->szText));
        }
#else
        if (pNMHDR->code == TTN_NEEDTEXTA)
        {
            _wcstombsz(pTTTA->szText, strTipText,sizeof(pTTTA->szText));
        }
        else
        {
            lstrcpyn(pTTTW->szText, strTipText, sizeof(pTTTW->szText));
        }
#endif

        *pResult = 0;

        // bring the tooltip window above other popup windows

        ::SetWindowPos(pNMHDR->hwndFrom, HWND_TOP, 0, 0, 0, 0,
            SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);
        
        return TRUE;
    }

    return FALSE;
}
void CDlg_DiskBrowserMain::OnBnClickedButtonAddFavorites()
{
	if (_ImageOpened == false &&  GetFileNameOfOpenedImage().GetLength() == 0)
		return;


	char tx[1000];
	GetDlgItemText(IDC_PATH, tx, 990);

	SendDlgItemMessage( IDC_PATH, CB_ADDSTRING, 0, (LPARAM)tx );

	// write favorites file.

	CString fav_list;
	for (int x=0; x< cbox_FavoritesList.GetCount(); x++)
	{
		CString str;
		cbox_FavoritesList.GetLBText(x, str);
		fav_list+=str;
		fav_list+="\x0D\x0A";

	}
	if (fav_list.GetLength() )
		_FSys.WriteWholeFile( (LPBYTE)(LPCTSTR)fav_list, fav_list.GetLength(), "/rohos_favorites.txt", NULL);

	AfxMessageBox("Added to Favorites", MB_ICONINFORMATION);

	// TODO: добавьте свой код обработчика уведомлений
}

void CDlg_DiskBrowserMain::OnCbnSelchangePath()
{
	//char tx[1000];
	//GetDlgItemText(IDC_PATH, tx, 990);

	int sel = cbox_FavoritesList.GetCurSel();

	if (sel>=0)
	{
		CString str;
		//cbox_FavoritesList.GetLBText(sel, str);
		//AfxMessageBox(str);
		PostMessage(WM_COMMAND, IDOK, 0);
	}

	//SendDlgItemMessage( IDC_PATH, CB_ADDSTRING, 0, (LPARAM)tx );
	
}

void CDlg_DiskBrowserMain::_detect_traveler_mode(void)
{
	_traveler_mode = false;
	TCHAR path[500];
	TCHAR rohospath[500];

	GetPodNogamiPath(path, false, NULL);	
	path[3]=0;

	bool run_from_removable_drive=false;
	bool no_center_exe_file=false;

	if (GetDriveType(path) == DRIVE_REMOVABLE)  // çàïóñê ñ  removable drive
		run_from_removable_drive = true;

	if (GetDriveType(path) == DRIVE_CDROM)  // çàïóñê ñ  cd rom 
		run_from_removable_drive = true;

	GetPodNogamiPath(path, false, NULL);	// ïîä íîãàìè íåò center.exe 
	_tcscat(path, TEXT("center.exe") );	
	if ( _taccess(path,0)==-1 ) 
		no_center_exe_file=true;

	GetPodNogamiPath(path, true, NULL);	// Agent.exe ëåæèò â ïàïêå êóäà åãî èíñòàëèðîâàëè - 
	ReadReg(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Rohos"), _T("RohosPath"), rohospath, 300);
	if ( StrStrI(path, rohospath) ) 
		no_center_exe_file=false;	
	
	
	if (run_from_removable_drive || no_center_exe_file)
	{
		_traveler_mode=true;		
		_tcscpy(_traveler_usb_drive,"c:\\");
		_traveler_usb_drive[0]= path[0];
		}
			    	
	
	if ( _traveler_mode ) {
		//TRACER.m_enabled = false;
	}
	else 
	{
		GetPodNogamiPath(path, false, NULL);	
		_tcscat(path, TEXT("rohos_dv.log") );	
		TRACE_INIT( path, 500,FILE_ONLY , true     );
	} 

	//TRACE_INIT( "c:\\rohos_dv.log", 500,FILE_ONLY , true     );

	//_regkey_USB[0]=0;
	//_traveler_rohos_disk_name[0]=0;
	
}
