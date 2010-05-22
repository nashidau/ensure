
#ifndef ENOBJMAGIC
#define ENOBJMAGIC 0x8d882abc
#endif

struct enobj {
	int	magic;

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

	Elm_Genlist_Item *genitem;

	struct {
		struct enobj *parent,*clip;
	} cache;
};

extern Eina_Hash *objdb;


void enobj_clear(void);
int enobj_add(struct enobj *eno);
struct enobj *enobj_parent_get(struct enobj *eno);
struct enobj * enobj_clip_get(struct enobj *eno);
struct enobj *enobj_get(uintptr_t id);

void enobj_free(void *enobj);
