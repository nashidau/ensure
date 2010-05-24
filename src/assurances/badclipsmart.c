#include "assurance.h"

static int object_check(struct ensure*,struct enobj*,void *data);

struct assurance assurance = {
	.summary = "Clip in Smart",
	.description = "Object's clip is part of the same smart object",
	.severity = ENSURE_BADFORM,
	.object = object_check
};


static int
object_check(struct ensure *en ensure_unused, struct enobj *obj,
		void *data ensure_unused){
	struct enobj *parent;
	struct enobj *clip, *clipp;
	assert(obj);

	if (!obj->parent) return 0;
	if (!obj->clip) return 0;

	parent = enobj_parent_get(obj);
	clip = enobj_clip_get(obj);
	if (!parent || !clip) return 0;
	if (!clip) return 0;

	clipp = enobj_parent_get(clip);

	if (clipp == NULL){
		ensure_bug(obj, assurance.severity,
				"Object's clip not smart member");
		return 1;
	} else if (clipp != parent){
		ensure_bug(obj, assurance.severity,
				"Object's clip member of difference smart "
				"Obj: %llx  Clip: %llx",parent->id,clip->id);
		return 1;
	}

	return 0;
}
