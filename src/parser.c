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
#include "parser.h"



static int parse_object(char *line, struct enobj *eno);
static void parse_line(char *line);

static int parse_objid(struct enobj *eno, const char *prefix, char **linep);
static int parse_name(struct enobj *eno, const char *prefix, char **linep);
static int parse_parent(struct enobj *eno, const char *prefix, char **linep);
static int parse_geo(struct enobj *eno, const char *prefix, char **linep);
static int parse_color(struct enobj *eno, const char *prefix, char **linep);
static int parse_image(struct enobj *eno, const char *prefix, char **linep);
static int parse_text(struct enobj *eno, const char *prefix, char **linep);



static const struct parser {
	const char *prefix;
	int (*parser)(struct enobj *eno, const char *prefix, char **linep);
} parsers[] = {
	{ "Object:",	parse_objid },
	{ "Parent:",	parse_parent },
	{ "Clip:",	parse_parent },
	{ "Name:",	parse_name },
	{ "Geometry:",  parse_geo },
	{ "Color",	parse_color },
	{ "Image:",	parse_image },
	{ "Text:",	parse_text },
};
#define N_PARSERS ((int)(sizeof(parsers)/sizeof(parsers[0])))

/*
 * Child data received: Hopefully an object
 */
int
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
	char *p,*start;
	assert(eno);assert(prefix);assert(linep);
	assert(eno->id == 0);

	eno->id = strtol(*linep,&p,0);
	p ++;
	if (*p == '\''){
		p ++;
		start = p;
		while (*p != '\'') p ++;
		eno->type = eina_stringshare_add_length(start,p-start);
		p ++;
	}

	/* FIXME: Check it parsed okay: Need safe strtol */

	/* FIXME: Insert into objdb */

	*linep = p;
	return 0;
}

static int
parse_name(struct enobj *eno, const char *prefix ensure_unused, char **linep){
	char *p,*start;

	p = *linep;

	while (*p != '\'') p ++;
	p ++;
	start = p;
	while (*p != '\'') p ++;
	eno->name = strndup(start,p-start);
	p ++;

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

