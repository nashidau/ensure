#include <limits.h>
#include <stdio.h>
#include <signal.h>
#include <stdbool.h>
#include <sys/signalfd.h>
#include <unistd.h>


#include <Eina.h>
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Evas.h>


#define ensure_unused	__attribute__((unused))

static int libensure_dump(void *data ensure_unused, Ecore_Fd_Handler *fdh);
static void libensure_objdump(Evas_Object *o, Evas_Object *parent);

struct Ecore_Evas_Wrap {
	EINA_INLIST;
};

extern struct Ecore_Evas_Wrap *ecore_evases;

static FILE *outfile;

__attribute__((constructor)) void
libensure_init(void){
	int fd;
	int sendfd;
	const char *p;
	sigset_t sigusr2;

	ecore_init();

	sigemptyset(&sigusr2);
	sigaddset(&sigusr2, SIGUSR2);

	fd = signalfd(-1, &sigusr2, SFD_CLOEXEC | SFD_NONBLOCK);
	ecore_main_fd_handler_add(fd,ECORE_FD_READ|ECORE_FD_ERROR,
			libensure_dump, NULL,
			NULL, NULL);

	p = getenv("ENSURE_FD");
	if (p){
		sendfd = strtol(p, NULL, 0);
		fprintf(stderr,"Ensure: Using fd %d!\n",sendfd);
		outfile = fdopen(sendfd, "w");
	}

	if (!outfile) {
		fprintf(stderr,"Ensure: Warning using stdout!\n");
		outfile = stdout;
	}
	fprintf(outfile,"ensure init\n");

	return;
}


static int
libensure_dump(void *data ensure_unused, Ecore_Fd_Handler *fdh){
	int fd;
	struct signalfd_siginfo siginfo;
	Eina_List *os,*ot;
	Evas_Object *o;
	struct Ecore_Evas_Wrap *eew;

	fd = ecore_main_fd_handler_fd_get(fdh);

	read(fd, &siginfo, sizeof(struct signalfd_siginfo));

	fprintf(outfile,"Ensure dump!\n");

	EINA_INLIST_FOREACH(ecore_evases, eew){
		Evas *e;
		e = ecore_evas_get((Ecore_Evas *)eew);
		fprintf(outfile,"E: %p\n",e);
		os = evas_objects_in_rectangle_get(e,SHRT_MIN, SHRT_MIN,
				USHRT_MAX, USHRT_MAX, true, true);
		EINA_LIST_FOREACH(os, ot, o){
			libensure_objdump(o, NULL);
		}

	}

	fprintf(outfile,"Ensure done\n");

	return 1;
}

static void
libensure_objdump(Evas_Object *o, Evas_Object *parent){
	Eina_List *children, *tmp;
	Evas_Object *child,*clip;
	const char *type;
	int x,y,w,h,a,r,g,b;

	type = evas_object_type_get(o);
	fprintf(outfile,"Object: %p '%s' ",o,type);


	if (parent) fprintf(outfile,"Parent: %p ",parent);
	evas_object_geometry_get(o,&x,&y,&w,&h);
	fprintf(outfile,"Geometry: %+d%+d(%dx%d) ",x,y,w,h);
	clip = evas_object_clip_get(o);
	if (clip) fprintf(outfile, "Clip: %p ",parent);
	evas_object_color_get(o, &r,&g,&b,&a);
	fprintf(outfile, "Color (%d,%d,%d,%d) ",r,g,b,a);

	/* Type specific things here */
	if (strcmp(type, "image") == 0){
		const char *file,*key;
		evas_object_image_file_get(o,&file,&key);
		if (key)
			fprintf(outfile, "Image: '%s' (%s) ",file,key);
		else
			fprintf(outfile, "Image: '%s' ",file);
	} else if (strcmp(type, "text") == 0){
		fprintf(outfile,"Text: [[%s]] ",evas_object_text_text_get(o));
	} else if (strcmp(type, "rectangle") == 0){

	} else if (strcmp(type, "edje") == 0){

	}


	fprintf(outfile,"\n");

	children = evas_object_smart_members_get(o);
	EINA_LIST_FOREACH(children, tmp, child){
		libensure_objdump(child,o);
	}

}

__attribute__((destructor)) void
libensure_fini(void){
	ecore_shutdown();
}
