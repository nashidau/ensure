#include "assurance.h"

static int object_check(struct ensure*,struct enobj*,void *data);

struct assurance assurance = {
	.summary = "Clipped to Same Smart",
	.description = "If the clip is a smart member, object should be too",
	.severity = ENSURE_BADFORM,
	.object = object_check
};


static int
object_check(struct ensure *en ensure_unused, struct enobj *obj,
		void *data ensure_unused){
	struct enobj *clip;
	assert(obj);

	if (!obj->clip) return 0;

	clip = enobj_clip_get(obj);

	if (!clip->parent) return 0;


	if (!obj->parent || obj->parent != clip->parent){
		ensure_bug(obj, assurance.severity,
				"Objects' clip different parent to object"
				"Obj: %llx  Clip: %llx",obj->parent,clip->parent);
		return 1;
	}

	return 0;
}
