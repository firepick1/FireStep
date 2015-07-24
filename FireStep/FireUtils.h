#ifndef FIREUTILS_H
#define FIREUTILS_H

#ifndef ARDUINO
#include <cstdio>
#endif

#ifndef FIRELOG_H
#define LOGERROR3(a,b,c,d)
#endif

////////////////// ASSERTFAIL(msg) implements assertion failure ////////////

#ifdef ASSERTFAIL_THROW
	// Throw an exception when assertions fail
	#define ASSERTFAIL(msg) \
		{LOGERROR3("***ASSERTION FAILED (THROW) *** %s in %s:%d",msg,__FILE__,__LINE__); \
		throw "***ASSERTION FAILED*** " msg;}
#endif

#ifdef TEST
#define ASSERTFAIL_KILL
#endif
#ifdef ASSERTFAIL_KILL
	// kill the program to get a core dump upon assertion failure
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
	#define ASSERTFAIL(msg) \
		{LOGERROR3("***ASSERTION FAILED (EXIT) *** %s in %s:%d",msg,__FILE__,__LINE__); \
		kill(getpid(), SIGABRT);}
#endif

#ifndef ASSERTFAIL
	#define ASSERTFAIL(msg) \
		{LOGERROR3("***ASSERTION FAILED (EXIT) *** %s in %s:%d",msg,__FILE__,__LINE__); \
		}
#endif

#ifdef ARDUINO
#define ASSERTFAILBUF(buf) { Serial.println(buf); }
#endif
#ifndef ASSERTFAILBUF
#include <iostream>
#define ASSERTFAILBUF(buf) { std::cerr << "***ASSERT FAILED*** " << buf << std::endl; }
#endif

////////////////// ASSERT() macros  ////////////////

#ifndef ASSERT_BUFSIZE
#define ASSERT_BUFSIZE 255
#endif

#define ASSERT(e) ASSERTNONZERO(e)
#define ASSERTMSG(e,msg) ASSERTNONZEROMSG(e,msg)
#define ASSERTNOERRNO(exp) assertnoerrno((long) (exp), __FILE__,__LINE__)
#define ASSERTNONZERO(exp) assertnonzero((long) (exp), __FILE__, __LINE__)
#define ASSERTNONZEROMSG(exp,msg) assertnonzero((long) (exp), __FILE__, __LINE__,msg)
#define ASSERTZERO(exp) assertzero((long) (exp), __FILE__, __LINE__)
#define ASSERTEQUAL(e,a) assertEqual((double)(e),(double)(a),0,__FILE__,__LINE__)
#define ASSERTEQUALP(e,a) assertEqualPtr((e),(a),__FILE__,__LINE__)
#define ASSERTEQUALT(e,a,t) assertEqual((e),(a),t,__FILE__,__LINE__)

#ifdef ASSERT_ERRNO
#include <errno.h>
inline void
assertnoerrno(long actual, const char* fname, long line) {
    if (actual>=0) {
        return;
    }

    const char *errstr;
    switch (errno) {
    case 13 /* EACCES */: errstr = "EACCESS"; break;
    case 17 /* EEXIST */: errstr = "EEXIST"; break;
    case 22 /* EINVAL */: errstr = "EINVAL"; break;
    case 23 /* ENFILE */: errstr = "ENFILE"; break;
    case 2 /* ENOENT */: errstr = "ENOENT"; break;
    case 12 /* ENOMEM */: errstr = "ENOMEM"; break;
    case 28 /* ENOSPC */: errstr = "ENOSPC"; break;
    case 1 /* EPERM */: errstr = "EPERM"; break;
    default: errstr = ""; break;
    }
    char buf[255];
    snprintf(buf, sizeof(buf), "%s@%ld return:%ld errno:%s (%d) ", 
		fname, line, actual, errstr, errno);
    ASSERTFAILBUF(buf);
    ASSERTFAIL("system errno");
}
#endif

inline void
assertzero(long actual, const char* fname, long line) {
    if (actual==0) {
        return;
    }

    char buf[255];
    snprintf(buf, sizeof(buf), "%s@%ld expected zero", fname, line);
    ASSERTFAILBUF(buf);
    ASSERTFAIL("expected zero");
}

inline void
assertnonzero(long actual, const char* fname, long line, const char *msg="(no message)") {
    if (actual) {
        return;
    }

    char buf[512];
    snprintf(buf, sizeof(buf), "%s@%ld expected non-zero: %s", fname, line, msg);
    ASSERTFAILBUF(buf);
    ASSERTFAIL("expected nonzero");
}

inline void
assertEqualPtr(void* expected, void* actual, const char* context, long line)
{
	if (expected == actual) {
        return;
    }

    char buf[255];
	snprintf(buf, sizeof(buf), "%s expected:%lx actual:%lx line:%ld",
			 context, (long) (size_t) expected, (long)(size_t) actual, line);
    ASSERTFAILBUF(buf);
    ASSERTFAIL("expected equal");
}

inline void
assertEqual(double expected, double actual, double tolerance, const char* context, long line)
{
    double diff = expected - actual;
    if (-tolerance <= diff && diff <= tolerance) {
        return;
    }

    char buf[255];
	if (tolerance == 0) {
		snprintf(buf, sizeof(buf), "%s expected:%ld actual:%ld line:%ld",
				 context, (long) expected, (long) actual, line);
	} else {
		snprintf(buf, sizeof(buf), "%s expected:%g actual:%g tolerance:%g line:%ld",
				 context, expected, actual, tolerance, line);
	}
    ASSERTFAILBUF(buf);
    ASSERTFAIL("expected equal");
}

#define ASSERTEQUALS(e,a) assertEqual(e,a,__FILE__,__LINE__)
#define ASSERTNOERRSTR(a) assertEqual("",a,__FILE__,__LINE__)
inline void
assertEqual(const char * expected, const char * actual, const char* context, int line) {
	if (actual == NULL) {
		if (expected == NULL || *expected == '\0') {
			return;
		}
	} else if (strcmp(expected, actual)==0) {
        return;
    }

    char buf[ASSERT_BUFSIZE];
    if (actual) {
        snprintf(buf, sizeof(buf), "%s@%d expected:\n\"%s\"\nactual:\n\"%s\"\n", context, line, expected, actual);
    } else {
        snprintf(buf, sizeof(buf), "%s@%d expected:\n\"%s\" \nactual:\nNULL\n", context, line, expected);
    }
    ASSERTFAILBUF(buf);
    ASSERTFAIL("expected equal strings");
}

#endif
