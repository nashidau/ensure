#include <errno.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/signalfd.h>


#include <Eina.h>
#include <Elementary.h>

#define ensure_unused	__attribute__((unused))

struct error {
	const char *msg;
};


Evas_Object *window_add(char **argv);

static void select_cb(void *data, Evas_Object *obj, void *event);

static void on_run(void *data, Evas_Object *button, void *event_info);

static void on_check(void *data, Evas_Object *button, void *event_info);

static void dochild(char **args);

static int signal_init(void);
static int signalfd_child(void *data, Ecore_Fd_Handler *fd_handler);

static Evas_Object *runbutton;
static Evas_Object *checkbutton;
static pid_t childid;

int
elm_main(int argc, char **argv){
        Evas_Object *win;

        if (argc < 2){
               printf("Usage: %s <program>\n", argv[0]);
                exit(0);
        }
        win = window_add(argv);

	signal_init();

        elm_list_go(win);
        elm_run();
        elm_shutdown();
        return 0;
}
ELM_MAIN()



Evas_Object *
window_add(char **args){
        Evas_Object *win,*bg,*li,*bx,*ctrls, *run, *check;

        win = elm_win_add(NULL, "Ensure", ELM_WIN_BASIC);
        elm_win_title_set(win, "Ensure");
        //evas_object_smart_callback_add(win,"delete,request", win_del,NULL);

        bg = elm_bg_add(win);
        evas_object_size_hint_weight_set(bg,1,1);
        elm_win_resize_object_add(win, bg);
        evas_object_show(bg);

        bx = elm_box_add(win);
        evas_object_size_hint_weight_set(bx, 1.0, 1.0);
	elm_win_resize_object_add(win, bg);
        evas_object_show(bg);

        ctrls = elm_box_add(bx);
	elm_box_horizontal_set(ctrls, true);
        evas_object_size_hint_weight_set(ctrls, 1.0, 0.0);
        evas_object_show(ctrls);

	/* Buttons: Main event is the 'clicked' event */
	run = elm_button_add(ctrls);
	elm_button_label_set(run, "Run");
	elm_button_autorepeat_set(run, false);
	elm_box_pack_end(ctrls,run);
	evas_object_show(run);
	evas_object_smart_callback_add(run, "clicked", on_run, args + 1);
	runbutton = run;

	check = elm_button_add(ctrls);
	elm_button_label_set(check, "Check");
	elm_button_autorepeat_set(check, false);
	elm_object_disabled_set(check, true);
	elm_box_pack_end(ctrls,check);
	evas_object_show(check);
	evas_object_smart_callback_add(check, "clicked", on_check, NULL);
	checkbutton = check;

        li = elm_list_add(bx);
        evas_object_size_hint_weight_set(li, 1, 1);
        evas_object_size_hint_align_set(li, -1, -1);
        elm_box_pack_end(bx, li);
        evas_object_show(li);

evas_object_resize(win, 300, 300);
evas_object_show(win);
        /* Urh.. the list widget for now */
        return li;

}

/**
 * Initialise our signal handler
 */
static int
signal_init(void){
	int fd;
	sigset_t sigchld;

	sigemptyset(&sigchld);
	sigaddset(&sigchld, SIGCHLD);

	fd = signalfd(-1, &sigchld, SFD_CLOEXEC | SFD_NONBLOCK);
	ecore_main_fd_handler_add(fd,ECORE_FD_READ|ECORE_FD_ERROR,
			signalfd_child, NULL,
			NULL, NULL);

	return 0;
}

static int
signalfd_child(void *data ensure_unused, Ecore_Fd_Handler *fdh){
	int fd;
	struct signalfd_siginfo siginfo;
	printf("Child exited\n");

	fd = ecore_main_fd_handler_fd_get(fdh);

	read(fd, &siginfo, sizeof(struct signalfd_siginfo));

	elm_object_disabled_set(runbutton, false);
	elm_object_disabled_set(checkbutton, true);

	return 1;
}





/**
 * The user clicked the 'check' button: Start checking the application.
 */
static void
on_check(void *data, Evas_Object *button, void *event_info ensure_unused){
	printf("Doing check!!\n");

	kill(childid, SIGUSR2);

}





/**
 * The user clicked the 'run' button: Start running the application.
 */
static void
on_run(void *data, Evas_Object *button, void *event_info ensure_unused){
	pid_t pid;
	char **args = data;

	elm_object_disabled_set(runbutton, true);
	elm_object_disabled_set(checkbutton, false);

	printf("Running %s\n", args[0]);

	/* I'm sure someone will complain I'm doing this myself... but anyway
	 */
	switch ((pid = fork())){
	case -1:
		/* FIXME: error message handling */
		fprintf(stderr,"Unable to fork\n");
		exit(7);
		break;
	case 0:
		dochild(args);
		exit(7);
	default:
		childid = pid;
		/* Parent */
		break;
	}
}

/**
 * Run the child process
 */
static void
dochild(char **args){
	/* FIXME: Should append: Not overwrite */
	setenv("LD_PRELOAD", "./libensure.so.0",1);
	execvp(args[0],args);
	perror("execvp");
	exit(7);
}


/* Signal handler for sig child */

static void
select_cb(void *data ensure_unused, Evas_Object *obj ensure_unused, void *event){
        /* FIXME: We should use a peruse pointer here */
        //Evas_Object *li = data;
        Elm_List_Item *itm = event;
        struct error *err;

        err = elm_list_item_data_get(itm);
}





