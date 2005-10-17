#ifndef MISC_H
#define MISC_H

#include <windows.h>

#define CFG_SAVE_PUTTY_PATH		0x0001
#define CFG_SAVE_HOTKEY_LB		0x0002
#define CFG_SAVE_HOTKEY_WL		0x0004
#define	CFG_SAVE_HOTKEY_HIDEWINDOW	0x0008
#define	CFG_SAVE_HOTKEY_CYCLEWINDOW	0x0010
#define CFG_SAVE_DRAGDROP		0x0020
#define CFG_SAVE_SAVECURSOR		0x0040
#define	CFG_SAVE_SHOWONQUIT		0x0080
#define	CFG_SAVE_MENUSESSIONS		0x0100
#define	CFG_SAVE_MENURUNNING		0x0200

#define PLAUNCH_REGISTRY_ROOT		"Software\\SimonTatham\\PuTTY\\PLaunch"
#define DEFAULTSETTINGS			"Default Settings"
#define REGROOT				"Software\\SimonTatham\\PuTTY\\Sessions"
#define ISFOLDER			"IsFolder"
#define PLAUNCH_PUTTY_PATH		"PuTTYPath"
#define PLAUNCH_HOTKEY_LB		"HotKeyLaunchBox"
#define PLAUNCH_HOTKEY_WL		"HotKeyWindowList"
#define	PLAUNCH_HOTKEY_HIDEWINDOW	"HotKeyHideForegroundWindow"
#define PLAUNCH_HOTKEY_CYCLEWINDOW	"HotKeyCycleThroughWindows"
#define PLAUNCH_ENABLEDRAGDROP		"EnableDragDrop"
#define PLAUNCH_ENABLESAVECURSOR	"EnableSaveCursor"
#define PLAUNCH_SAVEDCURSORPOS		"SavedCursorPosition"
#define PLAUNCH_SAVEMOREBUTTON		"SavedMoreListBoxOptions"
#define	PLAUNCH_SHOWONQUIT		"ShowAllHiddenWindowsOnQuit"
#define	PLAUNCH_MENUSESSIONS		"ShowSavedSessionsAsASubmenu"
#define	PLAUNCH_MENURUNNING		"ShowRunningSessionsAsASubmenu"

#define	HOTKEY_ACTION_NONE		0
#define HOTKEY_ACTION_MESSAGE		1
#define	HOTKEY_ACTION_LAUNCH		2
#define	HOTKEY_ACTION_EDIT		3
#define	HOTKEY_ACTION_KILL		4
#define	HOTKEY_ACTION_HIDE		5

#define	HOTKEY_MAX_ACTION		5

extern const char *const HOTKEY_STRINGS[];

#define ISEXPANDED			"IsExpanded"
#define	SESSIONICON			"WindowIcon"
#define HOTKEY				"PLaunchHotKey"
#define PLAUNCH_LIMIT_ENABLE		"PLaunchInstanceLimitEnable"
#define PLAUNCH_LIMIT_NUMBER		"PLaunchInstanceLimitNumber"
#define PLAUNCH_LIMIT_SEARCHFOR		"PLaunchInstanceLimitSearchFor"
#define	PLAUNCH_LIMIT_ACTION1		"PLaunchInstanceLimitAction1"
#define PLAUNCH_LIMIT_ACTION2		"PLaunchInstanceLimitAction2"
#define	PLAUNCH_AUTORUN_ENABLE		"PLaunchAutoRunEnable"
#define	PLAUNCH_AUTORUN_WHEN		"PLaunchAutoRunCondition"
#define	PLAUNCH_AUTORUN_ACTION		"PLaunchAutoRunAction"

#define	LIMIT_SEARCHFOR_LAST		0
#define	LIMIT_SEARCHFOR_FIRST		1
#define	LIMIT_SEARCHFOR_SECOND		2
#define	LIMIT_SEARCHFOR_THIRD		3
#define	LIMIT_SEARCHFOR_FOURTH		4
#define	LIMIT_SEARCHFOR_FIFTH		5
#define	LIMIT_SEARCHFOR_SIXTH		6
#define	LIMIT_SEARCHFOR_SEVENTH		7
#define	LIMIT_SEARCHFOR_EIGHTH		8
#define	LIMIT_SEARCHFOR_NINTH		9
#define	LIMIT_SEARCHFOR_TENTH		10
#define	LIMIT_SEARCHFOR_ELEVENTH	11
#define	LIMIT_SEARCHFOR_TVELFTH		12
#define	LIMIT_SEARCHFOR_UMPTEENTH	13
#define	LIMIT_SEARCHFOR_MAX		14

extern const char *const LIMIT_SEARCHFOR_STRINGS[];

#define	LIMIT_ACTION_NOTHING		0
#define	LIMIT_ACTION_HIDE		1
#define	LIMIT_ACTION_SHOW		2
#define	LIMIT_ACTION_MINIMIZE		3
#define	LIMIT_ACTION_MAXIMIZE		4
#define	LIMIT_ACTION_CENTER		5
#define	LIMIT_ACTION_KILL		6
#define	LIMIT_ACTION_MURDER		7
#define	LIMIT_ACTION_RUN		8
#define LIMIT_ACTION_MAX		9

extern const char *const LIMIT_ACTION_STRINGS[];

#define	AUTORUN_WHEN_START		0
#define	AUTORUN_WHEN_NETWORKSTART	1
#define	AUTORUN_WHEN_QUIT		2
#define	AUTORUN_WHEN_MAX		3

extern const char *const AUTORUN_WHEN_STRINGS[];

#define	AUTORUN_ACTION_NOTHING		0
#define	AUTORUN_ACTION_HIDE		1
#define	AUTORUN_ACTION_SHOW		2
#define	AUTORUN_ACTION_MINIMIZE		3
#define	AUTORUN_ACTION_MAXIMIZE		4
#define	AUTORUN_ACTION_CENTER		5
#define	AUTORUN_ACTION_MAX		6

extern const char *const AUTORUN_ACTION_STRINGS[];

int GetSystemImageLists(HMODULE * hShell32, HIMAGELIST * phLarge,
			HIMAGELIST * phSmall);
void FreeSystemImageLists(HMODULE hShell32);

HTREEITEM treeview_additem(HWND treeview, HTREEITEM parent,
			   void *scbt);
HTREEITEM treeview_addtree(HWND hwndTV, HTREEITEM _parent, char *root);

unsigned int treeview_getitemname(HWND treeview, HTREEITEM item, char *buf,
				  unsigned int bufsize);
unsigned int treeview_getitempath(HWND treeview, HTREEITEM item,
				  char *buf);
HTREEITEM treeview_getitemfrompath(HWND treeview, char *path);

void center_window(HWND window);

char *dupstr(const char *s);
unsigned int get_putty_path(char *buf, unsigned int bufsize);

unsigned int read_config(struct _config *cfg);
unsigned int save_config(struct _config *cfg, int what);

HWND launch_putty(int action, char *path);
void free_process_records(void);

int launch_autoruns(char *root, int when);

int enum_process_records(void **array, int nrecords, char *path);
void *get_nth_process_record(void **array, int nrecords, char *path,
			     int n);
void *get_process_record_by_window(void **array, int nrecords,
				   HWND window);

struct process_record {
    DWORD pid;
    DWORD tid;
    HANDLE hprocess;
    HWND window;
    char *path;
};

extern unsigned int nprocesses;
extern HANDLE *process_handles;
extern struct process_record **process_records;

void item_insert(void **array, int *anum, void *item);
int item_find(void *array, int anum, void *item);
void item_remove(void **array, int *anum, void *item);

int small_atoi(char *a);

#endif /* MISC_H */
