#include <assert.h>
#include <inttypes.h>

#include <Elementary.h>
#include <Eina.h>

#include "enobj.h"

Eina_Hash *objdb;

void
enobj_clear(void){

}

int
enobj_add(struct enobj *eno){

	if (objdb == NULL)
		objdb = eina_hash_pointer_new(enobj_free);

	if (eno->magic != ENOBJMAGIC)
		eno->magic = ENOBJMAGIC;
	eina_hash_add(objdb, &eno->id, eno);

	return 0;
}

struct enobj *
enobj_parent_get(struct enobj *eno){
	struct enobj *parent;
	if (!eno) return NULL;
	if (eno->cache.parent) return eno->cache.parent;
	if (eno->parent == 0) return NULL;

	parent = enobj_get(eno->id);
	eno->cache.parent = parent;
	return parent;
}

struct enobj *
enobj_clip_get(struct enobj *eno){
	struct enobj *clip;
	if (!eno) return NULL;
	if (eno->cache.clip) return eno->cache.clip;
	if (eno->clip == 0) return NULL;

	clip = enobj_get(eno->id);
	eno->cache.clip = clip;
	return clip;
}

struct enobj *
enobj_get(uintptr_t id){
	struct enobj *obj;
	obj = eina_hash_find(objdb, &id);
	assert(obj->magic == ENOBJMAGIC);
	assert(id == obj->id);
	return obj;
}

void
enobj_free(void *enobj){
	printf("Free! %p\n",enobj);
}
