struct ensure;
struct enobj;

enum ensure_severity {
	ENSURE_CRITICAL,
	ENSURE_BUG,
	ENSURE_BADFORM,
	ENSURE_PEDANTRY,
};


struct assurance {
	const char *summary;
	const char *description;
	enum ensure_severity severity;

	void *(*init)(struct ensure *);
	int (*object)(struct ensure *, struct enobj *, void *data);
	int (*fini)(struct ensure *, void *data);
};


int enasn_load(const char *);
int enasn_check(void);
