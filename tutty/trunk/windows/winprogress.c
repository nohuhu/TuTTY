#include <windows.h>
#include <commctrl.h>
#include "puttymem.h"
#include "progress.h"
#include "win_res.h"

typedef struct _progress_dialog_t {
    HWND dlg;
    HWND static1,
	 static2,
	 static3,
	 progressbar,
	 ok_button,
	 cancel_button,
	 help_button;
    void *ctx_static1,
	 *ctx_static2,
	 *ctx_static3,
	 *ctx_progressbar,
	 *ctx_ok_button,
	 *ctx_cancel_button,
	 *ctx_help_button;
    callback_proc cb_static1,
		  cb_static2,
		  cb_static3,
		  cb_progressbar,
		  cb_ok_button,
		  cb_cancel_button,
		  cb_help_button;
} progress_dialog_t;

INT_PTR CALLBACK ProgressDialogProc(HWND hwnd, UINT msg, 
				    WPARAM wParam, LPARAM lParam)
{
    progress_dialog_t *dlg;

    switch (msg) {
    case WM_INITDIALOG:
	{
	    dlg = snewn(1, progress_dialog_t);
	    memset(dlg, 0, sizeof(progress_dialog_t));

	    dlg->static1 = GetDlgItem(hwnd, IDC_PROGRESSTEXT1);
	    SetWindowText(dlg->static1, "");

	    dlg->static2 = GetDlgItem(hwnd, IDC_PROGRESSTEXT2);
	    SetWindowText(dlg->static2, "");

	    dlg->static3 = GetDlgItem(hwnd, IDC_PROGRESSTEXT3);
	    SetWindowText(dlg->static3, "");

	    dlg->progressbar = GetDlgItem(hwnd, IDC_PROGRESSBAR);
	    SendMessage(dlg->progressbar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
	    SendMessage(dlg->progressbar, PBM_SETSTEP, (WPARAM) 1, 0);

	    dlg->ok_button = GetDlgItem(hwnd, IDOK);

	    dlg->cancel_button = GetDlgItem(hwnd, IDCANCEL);

	    dlg->help_button = GetDlgItem(hwnd, IDHELP);

	    SetWindowLong(hwnd, GWL_USERDATA, (LONG) dlg);
	};
	break;
    case WM_COMMAND:
	{
	    callback_proc cb;
	    void *ctx;
	    HWND ctrl;
	    int ctl;

	    dlg = (progress_dialog_t *) GetWindowLong(hwnd, GWL_USERDATA);

	    switch (LOWORD(wParam)) {
	    case IDOK:
		ctl = PDI_OKBTN;
		ctrl = dlg->ok_button;
		ctx = dlg->ctx_ok_button;
		cb = dlg->cb_ok_button;
		break;
	    case IDCANCEL:
		ctl = PDI_CANCELBTN;
		ctrl = dlg->cancel_button;
		ctx = dlg->ctx_cancel_button;
		cb = dlg->cb_cancel_button;
		break;
	    case IDHELP:
		ctl = PDI_HELPBTN;
		ctrl = dlg->help_button;
		ctx = dlg->ctx_help_button;
		cb = dlg->cb_help_button;
		break;
	    };

	    if (cb)
		cb(ctl, PDC_BUTTONPUSHED, ctx);
	};
	break;
    };
    return 0;
};

void *progress_dialog_init(void)
{
    HWND dlg;

    dlg = CreateDialog(NULL, MAKEINTRESOURCE(IDD_PROGRESSBOX), 
		       NULL, ProgressDialogProc);

    return dlg;
};

int progress_dialog_action(void *pd, int action, int control, void *data)
{
    progress_dialog_t *dlg;
    HWND hwnd, ctrl;

    if (!pd)
	return FALSE;

    hwnd = (HWND) pd;
    dlg = (progress_dialog_t *) GetWindowLong(hwnd, GWL_USERDATA);

    switch (control) {
    case PDI_DIALOGBOX:
	ctrl = hwnd;
	break;
    case PDI_STATIC1:
	ctrl = dlg->static1;
	break;
    case PDI_STATIC2:
	ctrl = dlg->static2;
	break;
    case PDI_STATIC3:
	ctrl = dlg->static3;
	break;
    case PDI_PROGRESSBAR:
	ctrl = dlg->progressbar;
	break;
    case PDI_OKBTN:
	ctrl = dlg->ok_button;
	break;
    case PDI_CANCELBTN:
	ctrl = dlg->cancel_button;
	break;
    case PDI_HELPBTN:
	ctrl = dlg->help_button;
	break;
    default:
	ctrl = NULL;
    };

    if (!ctrl)
	return FALSE;

    switch (action) {
    case PDA_SETSTATUS:
	{
	    int status = (int) data;

	    if (status & PDS_DISABLED)
		EnableWindow(ctrl, FALSE);
	    if (status & PDS_ENABLED)
		EnableWindow(ctrl, TRUE);
	    if (status & PDS_VISIBLE)
		ShowWindow(ctrl, SW_SHOW);
	    if (status & PDS_HIDDEN)
		ShowWindow(ctrl, SW_HIDE);
	};
	return TRUE;
    case PDA_SETTEXT:
	SetWindowText(ctrl, (char *) data);
	return TRUE;
    case PDA_SETMINIMUM:
    case PDA_SETMAXIMUM:
	{
	    int limit, new_limit;

	    if (control != PDI_PROGRESSBAR)
		return FALSE;

	    limit = SendMessage(ctrl, PBM_GETRANGE, action == PDA_SETMAXIMUM ?
				TRUE : FALSE, 0);
	    new_limit = action == PDA_SETMINIMUM ?
		MAKELPARAM((int) data, limit) :
		MAKELPARAM(limit, (int) data);

	    SendMessage(ctrl, PBM_SETRANGE, 0, (LPARAM) new_limit);
	};
	return TRUE;
    case PDA_SETSTEP:
	{
	    if (control != PDI_PROGRESSBAR)
		return FALSE;

	    SendMessage(ctrl, PBM_SETSTEP, (WPARAM) data, 0);
	};
	return TRUE;
    case PDA_SETPERCENT:
	{
	    int min, max, step, abs;

	    if (control != PDI_PROGRESSBAR)
		return FALSE;

	    min = SendMessage(ctrl, PBM_GETRANGE, TRUE, 0);
	    max = SendMessage(ctrl, PBM_GETRANGE, FALSE, 0);

	    step = (max - min) / 100;
	    abs = (int) data * step;

	    SendMessage(ctrl, PBM_SETPOS, (WPARAM) abs, 0);
	};
	return TRUE;
    case PDA_SETABSOLUTE:
	{
	    if (control != PDI_PROGRESSBAR)
		return FALSE;

	    SendMessage(ctrl, PBM_SETPOS, (WPARAM) (int) data, 0);
	};
	return TRUE;
    case PDA_STEPIT:
	{
	    if (control != PDI_PROGRESSBAR)
		return FALSE;

	    SendMessage(ctrl, PBM_STEPIT, 0, 0);
	};
	return TRUE;
    case PDA_SETCBCONTEXT:
	{
	    switch (control) {
	    case PDI_STATIC1:
		dlg->ctx_static1 = data;
		break;
	    case PDI_STATIC2:
		dlg->ctx_static2 = data;
		break;
	    case PDI_STATIC3:
		dlg->ctx_static3 = data;
		break;
	    case PDI_PROGRESSBAR:
		dlg->ctx_progressbar = data;
		break;
	    case PDI_OKBTN:
		dlg->ctx_ok_button = data;
		break;
	    case PDI_CANCELBTN:
		dlg->ctx_cancel_button = data;
		break;
	    case PDI_HELPBTN:
		dlg->ctx_help_button = data;
		break;
	    default:
		return FALSE;
	    };
	};
	return TRUE;
    case PDA_SETCALLBACK:
	{
	    switch (control) {
	    case PDI_STATIC1:
		dlg->cb_static1 = (callback_proc) data;
		break;
	    case PDI_STATIC2:
		dlg->cb_static2 = (callback_proc) data;
		break;
	    case PDI_STATIC3:
		dlg->cb_static3 = (callback_proc) data;
		break;
	    case PDI_OKBTN:
		dlg->cb_ok_button = (callback_proc) data;
		break;
	    case PDI_CANCELBTN:
		dlg->cb_cancel_button = (callback_proc) data;
		break;
	    case PDI_HELPBTN:
		dlg->cb_help_button = (callback_proc) data;
		break;
	    default:
		return FALSE;
	    };
	};
	return TRUE;
    };

    return FALSE;
};

void progress_dialog_free(void *pd)
{
    progress_dialog_t *dlg;

    if (!pd)
	return;

    dlg = (progress_dialog_t *) GetWindowLong((HWND) pd, GWL_USERDATA);
    DestroyWindow((HWND) pd);
    sfree(dlg);
};
