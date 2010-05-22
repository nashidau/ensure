
#ifndef ensure_unused
#define ensure_unused	__attribute__((unused))
#endif

enum ensure_severity;
struct enobj;

/* report a bug */
int ensure_bug(struct enobj *enobj, enum ensure_severity sev,
		const char *fmt, ...);// __attribute__((printf(3,4)));
