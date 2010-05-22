
#include <assert.h>
#include <dirent.h>
#include <dlfcn.h>
#include <stdio.h>
#include <sys/types.h>


#include <Eina.h>
#include <Elementary.h>

#include "enasn.h"
#include "ensure.h"
#include "enobj.h"

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

		printf("Found: %s\n",de->d_name);

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

		printf("Loaded %s\n",asn->summary);
	}

	return 0;

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
