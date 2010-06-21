
#include <assert.h>
#include <dirent.h>
#include <dlfcn.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/types.h>


#include <Eina.h>
#include <Elementary.h>

#include "enasn.h"
#include "ensure.h"
#include "enobj.h"

static void bug_del(const void *, Evas_Object *obj);
static char *bug_label_get(const void *data, Evas_Object *, const char *);
static Evas_Object *bug_icon_get(const void *data, Evas_Object *, const char *);
static Eina_Bool bug_state_get(const void *data, Evas_Object *, const char *);


static const Elm_Genlist_Item_Class bugc = {
	.item_style = "default",
	.func = {
		.label_get = bug_label_get,
		.icon_get = bug_icon_get,
		.state_get = bug_state_get,
		.del = bug_del
	}
};


int
enasn_load(const char *path){
	struct dirent *de;
	struct assurance *asn;
	void *dlh;
	int err;
	char *p;
	DIR *dir;
	char buf[BUFSIZ];

	if (path == NULL) path = "assurances";

	dir = opendir(path);
	if (!dir) return -1;

	while ((de = readdir(dir))){
		if (de->d_name[0] == '.') continue;
		if (!(p = strstr(de->d_name,".asn"))) continue;
		if (p[4]) continue;

		snprintf(buf,sizeof(buf),"%s/%s",path,de->d_name);
		dlh = dlopen(buf,RTLD_NOW|RTLD_LOCAL);
		if (!dlh){
			printf("Unable to open %s\n",buf);
			continue;
		}
		asn = dlsym(dlh,"assurance");
		/* Check for sanity on the assurance */
		err = 0;
		if (!asn){
			printf("Unable to find 'assurance' in %s (%s)\n",buf,
					dlerror());
			err++;
		} else if (asn->summary == NULL){
			printf("Need summary in assurance '%s'\n",buf);
			err ++;
		} else if (!asn->object && !asn->init && !asn->fini){
			printf("Need at least one function!\n");
			err++;
		}

		if (err){
			dlclose(dlh);
			free(asn);
			continue;
		}

		ensure_assurance_add(asn);

	}

	return 0;

}



int
ensure_bug(struct enobj *enobj, enum ensure_severity sev,
		const char *fmt, ...){
	char *buf;
	struct bug *bug;
	int len;
	va_list ap;
	assert(enobj);

	va_start(ap, fmt);
	len = vsnprintf(NULL,0,fmt,ap);
	va_end(ap);

	if (len < 0){
		printf("Huh?");
		abort();
	}

	len ++;
	buf = malloc(len);

	va_start(ap, fmt);
	vsnprintf(buf,len,fmt,ap);
	va_end(ap);

	bug = calloc(1,sizeof(struct bug));
	bug->severity = sev;
	bug->desc = buf;

	enobj->bugs = eina_list_prepend(enobj->bugs, bug);

	if (!enobj->genitem){
		ensure_enobj_err_list_add(enobj);
	}

	return 0;
}

void
enasn_display_bugs(void *data ensure_unused, Evas_Object *obj ensure_unused,
		void *event){
	Eina_List *l;
	Elm_Genlist_Item *it = event;
	Evas_Object *gl = elm_genlist_item_genlist_get(it);
	struct bug *bug;
	const struct enobj *enobj = elm_genlist_item_data_get(it);

	EINA_LIST_FOREACH(enobj->bugs, l, bug){
		elm_genlist_item_append(gl, &bugc, bug,
				enobj->genitem,
				ELM_GENLIST_ITEM_NONE,
				NULL, NULL); /* No selection */
	}
}



static void
bug_del(const void *data ensure_unused, Evas_Object *obj ensure_unused){

}
static char *
bug_label_get(const void *data, Evas_Object *obj ensure_unused,
		const char *part ensure_unused){
	const struct bug *bug = data;
	return strdup(bug->desc);
}
static Evas_Object *
bug_icon_get(const void *data ensure_unused, Evas_Object *obj ensure_unused,
		const char *part ensure_unused){
	/* FIXME: Severity icon would be good */
	return NULL;
}
static Eina_Bool
bug_state_get(const void *data ensure_unused, Evas_Object *obj ensure_unused,
		const char *part ensure_unused){
	return false;
}
