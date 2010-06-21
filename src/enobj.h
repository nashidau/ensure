
#ifndef ENOBJMAGIC
#define ENOBJMAGIC 0x8d882abc
#endif

enum edjemember {
	EDJEMEMBER_NOTCHECKED,
	EDJEMEMBER_TRUE,
	EDJEMEMBER_FALSE
};


struct enobj {
	int	magic;

	/** ID of object (its address normally) */
	uintptr_t	id;

	/** Name: Optional */
	char *name;

	/** Type of object */
	const char *type;

	/** ID of parent */
	uintptr_t	parent;

	/** ID of clip */
	uintptr_t	clip;

	/** Geo */
	int x,y,w,h;

	/** Colour */
	unsigned char r,g,b,a;

	/** Files */
	union {
		struct {
			char *text;
			const char *font;
			const char *source;
			int size;
		} text;
		struct {
			const char *file;
			const char *key;
			const char *err;
			const char *edjefile;
			enum edjemember edjemember;
		} image;
		struct {
			const char *file;
			const char *group;
			const char *err;
		} edje;
	} data;

	Elm_Genlist_Item *genitem;
	Eina_List *bugs;

	struct {
		struct enobj *parent,*clip;
	} cache;

	Evas_Object *win;
};

extern Eina_Hash *objdb;


void enobj_clear(void);
int enobj_add(struct enobj *eno);
struct enobj *enobj_parent_get(struct enobj *eno);
struct enobj * enobj_clip_get(struct enobj *eno);
struct enobj *enobj_get(uintptr_t id);

void enobj_free(void *enobj);
