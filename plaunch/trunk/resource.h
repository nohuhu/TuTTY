#ifndef RESOURCE_H
#define RESOURCE_H

#ifndef RT_MANIFEST
#define	RT_MANIFEST				    24
#endif /* RT_MANIFEST */

#ifndef IDC_STATIC
#define IDC_STATIC				    -1
#endif /* IDC_STATIC */

#define IDI_MAINICON				    100

/*
 * dialog id definitions
 */
#define IDD_OPTIONSBOX				    101
#define IDD_ABOUTBOX				    102
#define IDD_LICENSEBOX				    103
#define IDD_WINDOWLISTBOX			    104
#define IDD_LAUNCHBOX				    105
#define IDD_LAUNCHBOX_TAB0			    106
#define IDD_LAUNCHBOX_TAB1			    107
#define IDD_LAUNCHBOX_TAB2			    108
#define IDD_LAUNCHBOX_TABGENERIC1		    109
#define IDD_LAUNCHBOX_TABGENERIC2		    110

/*
 * about box dialog controls
 */
#define IDC_ABOUTBOX_BUTTON_LICENSE		    201
#define IDC_ABOUTBOX_BUTTON_HOMEPAGE		    202
#define IDC_ABOUTBOX_STATIC_VERSION		    203

/*
 * window list box dialog controls
 */
#define IDC_WINDOWLISTBOX_STATIC		    210
#define IDC_WINDOWLISTBOX_LISTBOX		    211
#define IDC_WINDOWLISTBOX_BUTTON_HIDE		    212
#define IDC_WINDOWLISTBOX_BUTTON_SHOW		    213
#define IDC_WINDOWLISTBOX_BUTTON_KILL		    214

/*
 * options box dialog controls
 */
#define IDC_OPTIONSBOX_GROUPBOX			    220
#define IDC_OPTIONSBOX_STATIC_PUTTYPATH		    221
#define IDC_OPTIONSBOX_EDITBOX_PUTTYPATH	    222
#define IDC_OPTIONSBOX_BUTTON_PUTTYPATH		    223
#define IDC_OPTIONSBOX_STATIC_HOTKEY_WLIST	    224
#define IDC_OPTIONSBOX_EDITBOX_HOTKEY_WLIST	    225
#define IDC_OPTIONSBOX_STATIC_HOTKEY_LBOX	    226
#define IDC_OPTIONSBOX_EDITBOX_HOTKEY_LBOX	    227
#define IDC_OPTIONSBOX_CHECKBOX_STARTUP		    228
#define IDC_OPTIONSBOX_CHECKBOX_DRAGDROP	    229
#define IDC_OPTIONSBOX_CHECKBOX_SAVECUR		    230
#define IDC_OPTIONSBOX_CHECKBOX_SHOWONQUIT	    231
#define IDC_OPTIONSBOX_STATIC_HOTKEY_HIDEWINDOW	    232
#define IDC_OPTIONSBOX_EDITBOX_HOTKEY_HIDEWINDOW    233
#define IDC_OPTIONSBOX_CHECKBOX_MENUSESSIONS	    234
#define IDC_OPTIONSBOX_CHECKBOX_MENURUNNING	    235
#define IDC_OPTIONSBOX_EDITBOX_HOTKEY_CYCLEWINDOW   236

/*
 * launch box main dialog controls
 */
#define IDC_LAUNCHBOX_STATIC_TREEVIEW		    250
#define IDC_LAUNCHBOX_GROUPBOX_TREEVIEW		    251
#define	IDC_LAUNCHBOX_GROUPBOX_TABVIEW		    252
#define IDC_LAUNCHBOX_BUTTON_NEW_FOLDER		    253
#define IDC_LAUNCHBOX_BUTTON_NEW_SESSION	    254
#define IDC_LAUNCHBOX_BUTTON_EDIT		    255
#define IDC_LAUNCHBOX_BUTTON_RENAME		    256
#define IDC_LAUNCHBOX_BUTTON_DELETE		    257
#define IDC_LAUNCHBOX_BUTTON_MORE		    258
#define IDC_LAUNCHBOX_STATIC_DIVIDER		    259

/*
 * launch box child dialog controls
 * tab0: hotkeys
 */
#define	IDC_LAUNCHBOX_TAB0_EDITBOX_LAUNCH	    270
#define	IDC_LAUNCHBOX_TAB0_BUTTON_LAUNCH	    271
#define IDC_LAUNCHBOX_TAB0_EDITBOX_EDIT		    272
#define IDC_LAUNCHBOX_TAB0_BUTTON_EDIT		    273
#define IDC_LAUNCHBOX_TAB0_EDITBOX_KILL		    274
#define IDC_LAUNCHBOX_TAB0_BUTTON_KILL		    275

/*
 * launch box child dialog controls
 * tab1: auto actions
 */
#define IDC_LAUNCHBOX_TAB1_CHECKBOX_ATSTART	    280
#define	IDC_LAUNCHBOX_TAB1_BUTTON_ATSTART	    281
#define IDC_LAUNCHBOX_TAB1_CHECKBOX_ATNETWORKUP	    282
#define IDC_LAUNCHBOX_TAB1_BUTTON_ATNETWORKUP	    283
#define IDC_LAUNCHBOX_TAB1_CHECKBOX_ATNETWORKDOWN   284
#define IDC_LAUNCHBOX_TAB1_BUTTON_ATNETWORKDOWN	    285
#define IDC_LAUNCHBOX_TAB1_CHECKBOX_ATSTOP	    286
#define IDC_LAUNCHBOX_TAB1_BUTTON_ATSTOP	    287

/*
 * launch box child dialog controls
 * tab2: instance limits
 */
#define IDC_LAUNCHBOX_TAB2_CHECKBOX_ENABLE	    290
#define IDC_LAUNCHBOX_TAB2_CHECKBOX_LOWER	    291
#define IDC_LAUNCHBOX_TAB2_EDITBOX_LOWER	    292
#define IDC_LAUNCHBOX_TAB2_SPIN_LOWER		    293
#define IDC_LAUNCHBOX_TAB2_BUTTON_LOWER		    294
#define IDC_LAUNCHBOX_TAB2_CHECKBOX_UPPER	    295
#define IDC_LAUNCHBOX_TAB2_EDITBOX_UPPER	    296
#define IDC_LAUNCHBOX_TAB2_SPIN_UPPER		    297
#define IDC_LAUNCHBOX_TAB2_BUTTON_UPPER		    298

/*
 * launch box child dialog controls
 * generic tab1: actions, first page
 */
#define IDC_LAUNCHBOX_TABGENERIC1_COMBOBOX_FIND	    300
#define	IDC_LAUNCHBOX_TABGENERIC1_EDITBOX_FIND      301
#define IDC_LAUNCHBOX_TABGENERIC1_SPIN_FIND         302
#define IDC_LAUNCHBOX_TABGENERIC1_COMBOBOX_ACTION1  303
#define IDC_LAUNCHBOX_TABGENERIC1_COMBOBOX_ACTION2  304
#define IDC_LAUNCHBOX_TABGENERIC1_COMBOBOX_ACTION3  305
#define IDC_LAUNCHBOX_TABGENERIC1_COMBOBOX_ACTION4  306
#define IDC_LAUNCHBOX_TABGENERIC1_COMBOBOX_ACTION5  307
#define IDC_LAUNCHBOX_TABGENERIC1_COMBOBOX_ACTION6  308
#define IDC_LAUNCHBOX_TABGENERIC1_BUTTON_BACK	    309
#define IDC_LAUNCHBOX_TABGENERIC1_BUTTON_NEXT	    310
#define IDC_LAUNCHBOX_TABGENERIC1_BUTTON_MOREACTIONS 311

/*
 * launch box child dialog controls
 * generic tab2: actions, second and all successing pages
 */
#define IDC_LAUNCHBOX_TABGENERIC2_COMBOBOX_ACTION1  321
#define IDC_LAUNCHBOX_TABGENERIC2_COMBOBOX_ACTION2  322
#define IDC_LAUNCHBOX_TABGENERIC2_COMBOBOX_ACTION3  323
#define IDC_LAUNCHBOX_TABGENERIC2_COMBOBOX_ACTION4  324
#define IDC_LAUNCHBOX_TABGENERIC2_COMBOBOX_ACTION5  325
#define IDC_LAUNCHBOX_TABGENERIC2_COMBOBOX_ACTION6  326
#define IDC_LAUNCHBOX_TABGENERIC2_COMBOBOX_ACTION7  327
#define IDC_LAUNCHBOX_TABGENERIC2_COMBOBOX_ACTION8  328
#define IDC_LAUNCHBOX_TABGENERIC2_COMBOBOX_ACTION9  329
#define IDC_LAUNCHBOX_TABGENERIC2_COMBOBOX_ACTION10 330
#define IDC_LAUNCHBOX_TABGENERIC2_BUTTON_BACK	    331
#define IDC_LAUNCHBOX_TABGENERIC2_BUTTON_NEXT	    332
#define IDC_LAUNCHBOX_TABGENERIC2_STATIC_AND	    333
#define IDC_LAUNCHBOX_TABGENERIC2_STATIC_IT	    334

#endif				/* RESOURCE_H */
