
#include <assert.h>
#include <dirent.h>
#include <dlfcn.h>
#include <stdio.h>
#include <sys/types.h>


#include <Eina.h>

#include "enasn.h"
#include "ensure.h"
#include "enobj.h"

struct enasn {
	void *dlh;
	struct assurance *asn;
};

static Eina_List *asns;

int
enasn_load(const char *path){
	struct dirent *de;
	struct enasn *enasn;
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

		printf("Found: %s\n",de->d_name);

		snprintf(buf,sizeof(buf),"%s/%s",path,de->d_name);
		enasn = calloc(1,sizeof(struct enasn));
		enasn->dlh = dlopen(buf,RTLD_NOW|RTLD_LOCAL);
		if (!enasn->dlh){
			printf("Unable to open %s\n",buf);
			free(enasn);
			continue;
		}
		enasn->asn = dlsym(enasn->dlh,"assurance");
		/* Check for sanity on the assurance */
		err = 0;
		if (!enasn->asn){
			printf("Unable to find 'assurance' in %s (%s)\n",buf,
					dlerror());
			err++;
		} else if (enasn->asn->summary == NULL){
			printf("Need summary in assurance '%s'\n",buf);
			err ++;
		} else if (!enasn->asn->object && !enasn->asn->init &&
					!enasn->asn->fini){
			printf("Need at least one function!\n");
			err++;
		}

		if (err){
			dlclose(enasn->dlh);
			free(enasn);
			continue;
		}
		asns = eina_list_prepend(asns, enasn);

		printf("Loaded %s\n",enasn->asn->summary);
	}

	return 0;

}

static Eina_Bool
check_obj(const Eina_Hash *hash ensure_unused, const void *key ensure_unused,
		void *data, void *fdata ensure_unused){
	struct enobj *enobj = data;
	Eina_List *l;
	struct enasn *ea;
	assert(enobj->magic == ENOBJMAGIC);

	EINA_LIST_FOREACH(asns, l, ea){
		if (ea->asn->object)
			ea->asn->object(NULL, enobj, NULL);
	}
	return 1;
}


int
enasn_check(void){
	eina_hash_foreach(objdb, check_obj, NULL);
	return 1;
}

int ensure_bug(struct enobj *enobj, enum ensure_severity sev,
		const char *fmt, ...){
	char *buf;
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

	printf("Adding error %s\n",buf);

	return 0;
}
