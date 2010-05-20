#include <assert.h>
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

//enum entype {
//	ENTYPE_
//};


struct enobj {
	/** ID of object (its address normally) */
	uintptr_t	id;

	/** ID of parent */
	uintptr_t	parent;

	/** ID of clip */
	uintptr_t	clip;

	/** Geo */
	int x,y,w,h;

	/** Colour */
	unsigned char r,g,b,a;


	struct {
		struct enobj *parent,*clip;
	} cache;
};


Evas_Object *window_add(char **argv);

static void select_cb(void *data, Evas_Object *obj, void *event);

static void on_run(void *data, Evas_Object *button, void *event_info);
static void on_check(void *data, Evas_Object *button, void *event_info);

static int child_data(void *data, Ecore_Fd_Handler *hdlr);
static void parse_line(char *line);

static void dochild(char **args, int fd);

static int signal_init(void);
static int signalfd_child(void *data, Ecore_Fd_Handler *fd_handler);

static int parse_object(char *line, struct enobj *eno);

static int parse_objid(struct enobj *eno, const char *prefix, char **linep);
static int parse_parent(struct enobj *eno, const char *prefix, char **linep);
static int parse_geo(struct enobj *eno, const char *prefix, char **linep);
static int parse_color(struct enobj *eno, const char *prefix, char **linep);
static int parse_image(struct enobj *eno, const char *prefix, char **linep);
static int parse_text(struct enobj *eno, const char *prefix, char **linep);

static void enobj_free(void *enobj);

static const struct parser {
	const char *prefix;
	int (*parser)(struct enobj *eno, const char *prefix, char **linep);
} parsers[] = {
	{ "Object:",	parse_objid },
	{ "Parent:",	parse_parent },
	{ "Clip:",	parse_parent },
	{ "Geometry:",  parse_geo },
	{ "Color",	parse_color },
	{ "Image:",	parse_image },
	{ "Text:",	parse_text },
};
#define N_PARSERS ((int)(sizeof(parsers)/sizeof(parsers[0])))


static Evas_Object *runbutton;
static Evas_Object *checkbutton;
static pid_t childid;
static Eina_Hash *objdb;




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
on_check(void *data ensure_unused, Evas_Object *button ensure_unused,
		void *event_info ensure_unused){
	printf("Doing check!!\n");

	kill(childid, SIGUSR2);

}





/**
 * The user clicked the 'run' button: Start running the application.
 */
static void
on_run(void *data, Evas_Object *button ensure_unused, void *event_info ensure_unused){
	pid_t pid;
	char **args = data;
	int pipefd[2];

	elm_object_disabled_set(runbutton, true);
	elm_object_disabled_set(checkbutton, false);

	printf("Running %s\n", args[0]);

	if (pipe(pipefd)){
		fprintf(stderr,"Unable to create pipe\n");
		return;
	}

	/* I'm sure someone will complain I'm doing this myself... but anyway
	 */
	switch ((pid = fork())){
	case -1:
		/* FIXME: error message handling */
		fprintf(stderr,"Unable to fork\n");
		exit(7);
		break;
	case 0:
		close(pipefd[0]);
		dochild(args,pipefd[1]);
		exit(7);
	default:
		/* Parent */
		close(pipefd[1]);
		childid = pid;
		break;
	}

	/* Watch the fd */
	ecore_main_fd_handler_add(pipefd[0], ECORE_FD_READ, child_data,
					NULL, NULL, NULL);

	return;
}

/**
 * Run the child process
 */
static void
dochild(char **args, int fd){
	char buf[4];
	/* FIXME: Should append: Not overwrite */
	setenv("LD_PRELOAD", "./libensure.so.0",1);
	snprintf(buf, sizeof(buf), "%d",fd);
	setenv("ENSURE_FD", buf, 1);
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


static void
enobj_free(void *enobj){
	printf("Free! %p\n",enobj);
}

/*
 * Child data received: Hopefully an object
 */
static int
child_data(void *data ensure_unused, Ecore_Fd_Handler *hdlr){
	int fd;
	static char buf[BUFSIZ];
	static int buffered;
	char *p, *start;
	int len;

	assert(hdlr);

	fd = ecore_main_fd_handler_fd_get(hdlr);
	assert(fd >= 0);

	len = read(fd, buf + buffered, sizeof(buf) - 1 - buffered);
	if (len == 0){
		/* FIXME: child exit */
		printf("No data ready??\n");
		return 0;
	}
	if (len < 0){
		perror("Error reading from pipe");
		return 0;
	}
	buf[buffered + len] = 0;

	for (p = buf, start = buf ; *p ; p ++){
		if (*p != '\n') continue;

		*p = 0;
		parse_line(start);
		start = p + 1;
	}

	if (*start){
		/* Need to buffer some */
		memmove(buf, start, p - start);
		buffered = p - start;
		printf("Buffering: %s\n",start);
	}

	return 1;
}

static void
parse_line(char *line){
	if (strncmp(line,"Object", 6) == 0){
		printf("Got object\n");
		parse_object(line,NULL);
	} else {
		printf("Line was nothing...'%s'\n",line);
	}
}

static int
parse_object(char *line, struct enobj *eno){
	char *p;
	int i;

	if (eno == NULL){
		eno = calloc(1,sizeof(struct enobj));

		if (objdb == NULL)
			objdb = eina_hash_pointer_new(enobj_free);
		assert(eno);
	}

	p = line;
	while (*p){
		for (i = 0 ; i < N_PARSERS ; i ++){
			if (strncmp(parsers[i].prefix, p,
						strlen(parsers[i].prefix)))
				continue;

			p += strlen(parsers[i].prefix);
			while (isspace(*p)) p ++;

			parsers[i].parser(eno, parsers[i].prefix, &p);
			line = p;
			break;
		}
		if (i == N_PARSERS){
			printf("Don't handle %10.10s\n",p);
			p ++;
		}
		while (*p && isspace(*p)) p ++;
	}

	assert(eno->id);
	eina_hash_add(objdb, eno->id, eno);

	return 0;

}



static int
parse_objid(struct enobj *eno, const char *prefix, char **linep){
	char *p;
	assert(eno);assert(prefix);assert(linep);
	assert(eno->id == 0);

	eno->id = strtol(*linep,&p,0);
	p ++;
	if (*p == '\''){
		/* FIXME: Ignoring type */
		p ++;
		while (*p != '\'') p ++;
		p ++;
	}

	/* FIXME: Check it parsed okay: Need safe strtol */

	/* FIXME: Insert into objdb */

	*linep = p;
	return 0;
}


static int
parse_parent(struct enobj *eno, const char *prefix, char **linep){
	uintptr_t id;
	char *p;
	assert(eno);assert(prefix);assert(linep);

	/* FIXME: safe strtol */
	id = strtol(*linep, &p, 0);

	if (strncmp(prefix, "Parent",4) == 0){
		eno->parent = id;
	} else {
		eno->clip = id;
	}
	*linep = p;
	return 0;
}

static int
parse_geo(struct enobj *eno, const char *prefix, char **linep){
	int len,ct;
	assert(eno);assert(prefix);assert(linep);

	ct = sscanf(*linep, "%d%d(%dx%d)%n", &eno->x, &eno->y,
			&eno->w,&eno->h,&len);
	/* FIXME: Check count (ct) */
	//printf("Geo: %s -> %d %d %d %d\n",*linep,eno->x,eno->y,eno->w,eno->h);
	*linep += len;
	return 0;
}
static int
parse_color(struct enobj *eno, const char *prefix, char **linep){
	int len,ct;
	assert(eno);assert(prefix);assert(linep);

	ct = sscanf(*linep, "(%hhd,%hhd,%hhd,%hhd)%n",&eno->r,&eno->g,
			&eno->b,&eno->a,&len);

	*linep += len;

	return 0;
}
static int
parse_image(struct enobj *eno, const char *prefix, char **linep){
	*linep ++;
	return 0;
}
static int
parse_text(struct enobj *eno, const char *prefix, char **linep){
	*linep ++;
	return 0;
}
