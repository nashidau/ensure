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

#include "enobj.h"
#include "enasn.h"
#include "ensure.h"

struct error {
	const char *msg;
};

struct asninfo {
	bool enabled;
	struct assurance *asn;
};

struct severityinfo {
	const char *name;
	const char *icon;
	Elm_Genlist_Item *item;
	Eina_List *asninfo;
} severity[] = {
	[ENSURE_CRITICAL] = {
		.name = "Critical",
	},
	[ENSURE_BUG] = {
		.name = "Bug",
	},
	[ENSURE_BADFORM] = {
		.name = "Bad Form",
	},
	[ENSURE_POLICY] = {
		.name = "Local Policy",
	},
	[ENSURE_PEDANTRY] = {
		.name = "Pedantry",
	}
};





Evas_Object *window_add(char **argv);

static void select_cb(void *data, Evas_Object *obj, void *event);

static void on_run(void *data, Evas_Object *button, void *event_info);
static void on_check(void *data, Evas_Object *button, void *event_info);
static void on_switch_config(void *data, Evas_Object *button, void *event_info);

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

static void config_add_classes(Evas_Object *gl);
static void cfg_sel(void *data, Evas_Object *obj, void *event);
static void cfg_exp(void *data, Evas_Object *obj, void *event);
static void cfg_con(void *data, Evas_Object *obj, void *event);
static void cfg_exp_req(void *data, Evas_Object *obj, void *event);
static void cfg_con_req(void *data, Evas_Object *obj, void *event);


static char *cfg_label_get(const void *, Evas_Object *, const char *part);
static Evas_Object *cfg_icon_get(const void *, Evas_Object *, const char *part);
static Eina_Bool cfg_state_get(const void *, Evas_Object *, const char *part);
static void cfg_del(const void *, Evas_Object *obj);

static char *asn_label_get(const void *data, Evas_Object *, const char *);
static Evas_Object *asn_icon_get(const void *data, Evas_Object *, const char *);
static Eina_Bool asn_state_get(const void *data, Evas_Object *, const char *);
static void asn_del(const void *data, Evas_Object *obj);
static void asn_select(void *data, Evas_Object *obj, void *event);
static void asn_select_toggle(void *data, Evas_Object *obj, void *event);



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


int
elm_main(int argc, char **argv){
        Evas_Object *win;

        if (argc < 2){
               printf("Usage: %s <program>\n", argv[0]);
                exit(0);
        }
        win = window_add(argv);

	signal_init();

	enasn_load("assurances");

        elm_list_go(win);
        elm_run();
        elm_shutdown();
        return 0;
}
ELM_MAIN()



Evas_Object *
window_add(char **args){
        Evas_Object *win,*bg,*bx,*ctrls,*run,*check,*gl,*gl2,*config,*flip;

        win = elm_win_add(NULL, "Ensure", ELM_WIN_BASIC);
        elm_win_title_set(win, "Ensure");
        //evas_object_smart_callback_add(win,"delete,request", win_del,NULL);

        bg = elm_bg_add(win);
        elm_win_resize_object_add(win, bg);
        evas_object_size_hint_weight_set(bg,EVAS_HINT_EXPAND,EVAS_HINT_EXPAND);
        evas_object_show(bg);

        bx = elm_box_add(win);
        evas_object_size_hint_weight_set(bx, EVAS_HINT_EXPAND,EVAS_HINT_EXPAND);
	elm_win_resize_object_add(win, bx);
        evas_object_show(bx);

	flip = elm_flip_add(win);
	evas_object_size_hint_align_set(flip, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(flip, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	elm_box_pack_end(bx, flip);

	gl = elm_genlist_add(win);
	elm_genlist_always_select_mode_set(gl, true);
	evas_object_size_hint_align_set(gl, EVAS_HINT_FILL, EVAS_HINT_FILL);
	evas_object_size_hint_weight_set(gl, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	config_add_classes(gl);
	evas_object_show(gl);
	elm_flip_content_back_set(flip, gl);

	ctrls = elm_box_add(win);
	elm_box_horizontal_set(ctrls, true);
        evas_object_size_hint_weight_set(ctrls, EVAS_HINT_EXPAND, 0);
        evas_object_size_hint_align_set(ctrls, EVAS_HINT_FILL, EVAS_HINT_FILL);


	/* Buttons: Main event is the 'clicked' event */
	config = elm_button_add(ctrls);
	elm_button_label_set(config, "Config");
	elm_button_autorepeat_set(config, false);
	elm_box_pack_start(ctrls, config);
	evas_object_show(config);
	evas_object_smart_callback_add(config, "clicked", on_switch_config,
			flip);

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

	elm_box_pack_end(bx, ctrls);
	evas_object_show(ctrls);



evas_object_resize(win, 300, 300);
evas_object_show(win);
        /* Urh.. the list widget for now */
        return gl;

}

static const Elm_Genlist_Item_Class clc = {
	.item_style = "default",
	.func = {
		.label_get = cfg_label_get,
		.icon_get = cfg_icon_get,
		.state_get = cfg_state_get,
		.del = cfg_del,
	},
};
static const Elm_Genlist_Item_Class asnclass = {
	.item_style = "default",
	.func = {
		.label_get = asn_label_get,
		.icon_get = asn_icon_get,
		.state_get = asn_state_get,
		.del = asn_del
	},
};

static void
config_add_classes(Evas_Object *gl){
	int i;
	for (i = 0 ; i < ENSURE_N_SEVERITIES ; i ++){
		printf("Add %s\n",severity[i].name);
		severity[i].item = elm_genlist_item_append(gl,&clc,
				severity + i,
				NULL, /* No parent */
				ELM_GENLIST_ITEM_SUBITEMS,
				cfg_sel,
				severity + i /* data */);

	}

	evas_object_smart_callback_add(gl, "expand,request", cfg_exp_req, gl);
	evas_object_smart_callback_add(gl, "contract,request", cfg_con_req, gl);
	evas_object_smart_callback_add(gl, "expanded", cfg_exp, gl);
	evas_object_smart_callback_add(gl, "contracted", cfg_con, gl);

}

static void
cfg_sel(void *data, Evas_Object *obj ensure_unused, void *event ensure_unused){
	const struct severityinfo *info = data;

	printf("Item selected! %s\n",info->name);
}
static void
cfg_exp(void *data, Evas_Object *obj, void *event){
	Elm_Genlist_Item *parent = event;
	Evas_Object *gl = elm_genlist_item_genlist_get(parent);
	struct severityinfo *sev;
	struct asninfo *ai;
	Eina_List *l;


	sev = (struct severityinfo *)elm_genlist_item_data_get(parent);

	EINA_LIST_FOREACH(sev->asninfo, l, ai){
		elm_genlist_item_append(gl, &asnclass,
				ai, parent,
				ELM_GENLIST_ITEM_NONE,
				asn_select, ai);
	}

}
static void
cfg_con(void *data, Evas_Object *obj, void *event){
	Elm_Genlist_Item *it = event;
	elm_genlist_item_subitems_clear(it);
}
static void 
cfg_exp_req(void *data, Evas_Object *obj, void *event){
	Elm_Genlist_Item *it = event;
	printf("Expand request!\n");
	elm_genlist_item_expanded_set(it, 1);
}
static void cfg_con_req(void *data, Evas_Object *obj, void *event){
   Elm_Genlist_Item *it = event;
   elm_genlist_item_expanded_set(it, 0);
}
static void cfg_del(const void *data, Evas_Object *obj){

}


static char *
cfg_label_get(const void *data, Evas_Object *obj, const char *part){
	const struct severityinfo *info;

	info = data;
	return strdup(info->name);
}
static Evas_Object *
cfg_icon_get(const void *data, Evas_Object *obj, const char *part){
	return NULL;
}
static Eina_Bool
cfg_state_get(const void *data, Evas_Object *obj, const char *part){
	return false;
}

/** Handlers for the subitems in the assurance list */
static char *
asn_label_get(const void *data, Evas_Object *obj ensure_unused,
		const char *part ensure_unused){
	const struct asninfo *info;

	info = data;
	return strdup(info->asn->summary);
}

static Evas_Object *
asn_icon_get(const void *data, Evas_Object *obj, const char *part){
	const struct asninfo *ai = data;
	Evas_Object *ck;

	if (strcmp(part, "elm.swallow.end") == 0){
		ck = elm_check_add(obj);
		evas_object_show(ck);
		elm_check_state_set(ck, ai->enabled);
		evas_object_smart_callback_add(ck, "changed",
				asn_select_toggle, ai);
		return ck;
	}

	return NULL;
}
static Eina_Bool
asn_state_get(const void *data, Evas_Object *obj ensure_unused,
		const char *part ensure_unused){
	const struct asninfo *ai;

	ai = data;
	return ai->enabled;
}
static void
asn_del(const void *data ensure_unused, Evas_Object *obj ensure_unused){
	/* FIXME: REmove  item ref */
}

static void
asn_select(void *data, Evas_Object *obj ensure_unused, void *event ensure_unused){
	struct asninfo *info = data;

	info->enabled = !info->enabled;
}

static void
asn_select_toggle(void *data, Evas_Object *obj, void *event){
	struct asninfo *ai = data;

	ai->enabled = elm_check_state_get(obj);
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




static void
on_switch_config(void *data, Evas_Object *button, void *event_info){
	Evas_Object *flip = data;
	bool state;

	state = elm_flip_front_get(flip);
	if (state){
		/* Front currently: Change to back */
		elm_button_label_set(button, "Report");
	} else {
		elm_button_label_set(button, "Config");
	}

	elm_flip_go(flip, ELM_FLIP_ROTATE_Y_CENTER_AXIS);
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
	} else if (strncmp(line, "Ensure done",11) == 0){
		printf("Got to the end...\n");
		enasn_check();
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
	enobj_add(eno);

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
	//*linep ++;
	return 0;
}
static int
parse_text(struct enobj *eno, const char *prefix, char **linep){
	//*linep ++;
	return 0;
}




int
ensure_assurance_add(struct assurance *enasn){
	struct asninfo *info;

	assert((int)enasn->severity >= 0);
	assert(enasn->severity < ENSURE_N_SEVERITIES);
	/* FIXME: Do check! 
	if ((int)enasn->severity < 0 && enasn->severity >= ENSURE_N_SEVERITIES){
		return -1;
	}*/

	info = calloc(1,sizeof(struct asninfo));
	info->enabled = true;
	info->asn = enasn;
	severity[enasn->severity].asninfo = eina_list_append(
			severity[enasn->severity].asninfo, info);

	return 0;
}


static Eina_Bool
check_obj(const Eina_Hash *hash ensure_unused, const void *key ensure_unused,
		void *data, void *fdata ensure_unused){
	struct enobj *enobj = data;
	Eina_List *l;
	struct asninfo *ai;
	int i;
	assert(enobj->magic == ENOBJMAGIC);

	for (i = 0 ; i < ENSURE_N_SEVERITIES ; i ++){
		EINA_LIST_FOREACH(severity->asninfo, l, ai){
			if (ai->asn->object)
				ai->asn->object(NULL, enobj, NULL);
		}
	}
	return 1;
}


int
enasn_check(void){
	eina_hash_foreach(objdb, check_obj, NULL);
	return 1;
}
