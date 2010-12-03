#ifndef _PROGRESS_H
#define _PROGRESS_H

/*
 * Progress dialog control status.
 */

#define	PDS_DISABLED	    0x01
#define PDS_ENABLED	    0x02
#define	PDS_VISIBLE	    0x04
#define	PDS_HIDDEN	    0x08

/*
 * Progress dialog control identifiers.
 */

#define	PDI_DIALOGBOX	    0
#define PDI_STATIC1	    1
#define	PDI_STATIC2	    2
#define	PDI_STATIC3	    3
#define	PDI_PROGRESSBAR	    4
#define	PDI_OKBTN	    5
#define	PDI_CANCELBTN	    6
#define	PDI_HELPBTN	    7

/*
 * Progress dialog control actions.
 */

#define PDA_SETSTATUS	    0
#define	PDA_SETTEXT	    1
#define PDA_SETTIMER	    2
#define PDA_SETCBCONTEXT    3
#define PDA_SETCALLBACK	    4
#define PDA_UNSETCALLBACK   5
#define PDA_SETMINIMUM	    6
#define PDA_SETMAXIMUM	    7
#define PDA_SETSTEP	    8
#define	PDA_SETPERCENT	    9
#define	PDA_SETABSOLUTE	    10
#define PDA_STEPIT	    11

/*
 * Progress dialog control callback actions.
 */

#define PDC_TIMEREXPIRED    0
#define PDC_BUTTONPUSHED    1

typedef void (*callback_proc) (int control, int action, void *ctx);

void *progress_dialog_init(void);
int progress_dialog_action(void *pd, int action, int control, void *data);
void progress_dialog_free(void *pd);

#endif /* _PROGRESS_H */
