/*	This is pstree written by Fred Hucht (c) 1993-1997	*
 *	EMail: fred@thp.Uni-Duisburg.DE				*
 *	Feel free to copy and redistribute in terms of the	*
 * 	GNU public license. 					*
 *
 * $Id: pstree.c,v 2.9 1998-01-07 16:55:26+01 fred Exp $
 *
 * $Log: pstree.c,v $
 * Revision 2.9  1998-01-07 16:55:26+01  fred
 * Added support for getprocs()
 *
 * Revision 2.9  1998-01-06 17:13:19+01  fred
 * Added support for getprocs() under AIX
 *
 * Revision 2.8  1997-10-22 15:09:39+02  fred
 * Cosmetic
 *
 * Revision 2.7  1997-10-22 15:01:40+02  fred
 * Minor changes in getprocs for AIX
 *
 * Revision 2.6  1997/10/16 16:35:19  fred
 * Added uid2name() caching username lookup, added patch for Solaris 2.x
 *
 * Revision 2.5  1997-02-05 14:24:53+01  fred
 * return PrintTree when nothing to do.
 *
 * Revision 2.4  1997/02/05 09:54:08  fred
 * Fixed bug when P[i].cmd is empty
 *
 * Revision 2.3  1997-02-04 18:40:54+01  fred
 * Cosmetic
 *
 * Revision 2.2  1997-02-04 14:11:17+01  fred
 * *** empty log message ***
 *
 * Revision 2.1  1997-02-04 13:55:14+01  fred
 * Rewritten
 *
 * Revision 1.13  1997-02-04 09:01:59+01  fred
 * Start of rewrite
 *
 * Revision 1.12  1996-09-17 21:54:05+02  fred
 * *** empty log message ***
 *
 * Revision 1.11  1996-09-17 21:52:52+02  fred
 * revision added
 *
 * Revision 1.10  1996-09-17 21:45:35+02  fred
 * replace \n and \t with ? in output
 *
 * Revision 1.4  1996-09-17 21:43:14+02  fred
 * Moved under RCS, replace \n and \t with ?
 *
 *
 */

static char *WhatString[]= {
  "@(#)pstree $Revision: 2.9 $ by Fred Hucht (C) 1993-1997",
  "@(#)EMail:fred@thp.Uni-Duisburg.DE",
  "$Id: pstree.c,v 2.9 1998-01-07 16:55:26+01 fred Exp $"
};

#define MAXLINE 256

#if defined(_AIX) || defined(___AIX)	/* AIX >= 3.1 */
/* Under AIX, we directly read the process table from the kernel */
#  define HAS_TERMDEF
extern char *termdef(int, char);
#  define UID2USER
#  define _ALL_SOURCE
#  include <procinfo.h>
#  define USE_GETPROCS

#  ifdef USE_GETPROCS
#    define IFNEW(a,b) a
#    define ProcInfo procsinfo
extern getprocs(struct procsinfo *, int, struct fdsinfo *, int, pid_t *, int);
#  else /*USE_GETPROCS*/
#    define IFNEW(a,b) b
#    define ProcInfo procinfo
extern getproc(struct procinfo *, int, int);
extern getuser(struct procinfo *, int, void *, int);
#  endif /*USE_GETPROCS*/

extern getargs(struct ProcInfo *, int, char *, int);

/*
 * #define PSCMD 	"ps -ekf"
 * #define PSFORMAT 	"%s %ld %ld %*20c %*s %[^\n]"
 * #define PSVARS	P[i].name, &P[i].pid, &P[i].ppid, P[i].cmd
 */
#elif defined(__linux)	/* Linux */
#  define UID2USER
#  define PSCMD 	"ps laxw"
#  define PSFORMAT 	"%*d %ld %ld %ld %*35c %*s %[^\n]"
/* #elif defined(sparc)	SunOS */
#elif defined(sun) && (!defined(__SVR4)) /* Solaris 1.x */
/* contributed by L. Mark Larsen <mlarsen@ptdcs2.intel.com> */
/* new cpp criteria by Pierre Belanger <belanger@risq.qc.ca> */
#  define solaris1x
#  define UID2USER
#  define PSCMD 	"ps jaxw"
#  define PSFORMAT 	"%ld %ld %*d %*d %*s %*d %*s %ld %*s %[^\n]"
#  define PSVARS 	&P[i].ppid, &P[i].pid, &P[i].uid, P[i].cmd
#elif defined(sun) && (defined(__SVR4)) /* Solaris 2.x */
/* contributed by Pierre Belanger <belanger@risq.qc.ca> */
#  define solaris2x
#  define PSCMD         "ps -ef"
#  define PSFORMAT      "%s %ld %ld %*d %*s %*s %*s %[^\n]"
#elif defined(bsdi)
/* contributed by Dean Gaudet <dgaudet@hotwired.com> */
#  define UID2USER
#  define PSCMD 	"ps laxw"
#  define PSFORMAT 	"%ld %ld %ld %*d %*d %*d %*d %*d %*s %*s %*s %*s %[^\n]"
#elif defined(_BSD)	/* Untested */
#  define UID2USER
#  define PSCMD 	"ps laxw"
#  define PSFORMAT 	"%*d %*c %ld %ld %ld %*d %*d %*d %*x %*d %d %*15c %*s %[^\n]"
#elif defined(__convex)	/* ConvexOS */
#  define UID2USER
#  define PSCMD 	"ps laxw"
#  define PSFORMAT 	"%*s %ld %ld %ld %*d %*g %*d %*d %*21c %*s %[^\n]"
#else			/* HP-UX, A/UX etc. */
#  define PSCMD 	"ps -ef"
#  define PSFORMAT 	"%s %ld %ld %*20c %*s %[^\n]"
#endif

#ifndef PSVARS 		/* Set default */
# ifdef UID2USER
#  define PSVARS	&P[i].uid, &P[i].pid, &P[i].ppid, P[i].cmd
# else
#  define PSVARS	P[i].name, &P[i].pid, &P[i].ppid, P[i].cmd
# endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>		/* For str...() */
#include <unistd.h>		/* For getopt */

#ifdef UID2USER
#include <pwd.h>
#endif

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

struct TreeChars {
  char *s2, 		/* String between header and pid */
    *p, 		/* dito, when parent of printed childs */
    pgl,		/* Process group leader */
    npgl,		/* No process group leader */
    *barc, 		/* bar for line with child */
    *bar, 		/* bar for line without child */
    *barl;		/* bar for last child */
};

static struct TreeChars
  Ascii = { "--",       "-+",       '=',    '-',    "|",    "|",    "\\"   },
  Pc850 = { "\304\304", "\304\302", '\315', '\304', "\303", "\263", "\300" },
  *C;

int MyPid, NProc, /*Ns1, Maxlevel = 0, Alignlen, */Columns;
short /*align = FALSE, */showall = TRUE, soption = FALSE, Uoption = FALSE;
char *name = "", *str = NULL, *Progname;
long ipid = -1;

#ifdef DEBUG
int debug = FALSE;
#endif

struct Proc {
  long uid, pid, ppid;
  int pgl;
  char name[9], cmd[MAXLINE];
  int  print;
  long parent, child, sister;
  unsigned long thcount;
} *P;

#ifdef UID2USER
void uid2name(uid_t uid, char *name, int len) {
#define NUMUN 128
  static struct un_ {
    uid_t uid;
    char name[9];
  } un[NUMUN];
  static short n = 0;
  short i;
  char *found;
#ifdef DEBUG
  if (name == NULL) {
    for (i = 0; i < n; i++)
      fprintf(stderr, "uid = %3d, name = %s\n", un[i].uid, un[i].name);
    return;
  }
#endif
  for (i = n - 1; i >= 0 && un[i].uid != uid; i--);
  if (i >= 0) { /* found locally */
    found = un[i].name;
  } else {
    struct passwd *pw = getpwuid(uid);
    found = pw->pw_name;
    if (n < NUMUN) {
      un[n].uid = uid;
      strncpy(un[n].name, found, 9);
      un[n].name[8] = '\0';
      n++;
    }
  }
  strncpy(name, found, len);
  name[len-1] = '\0';
}
#endif

#if defined(_AIX) || defined(___AIX)	/* AIX 3.x / 4.x */
int GetProcesses(void) {
  int i, nproc, maxnproc = 1024;
  
  struct ProcInfo *proc;
  int idx;
#ifndef USE_GETPROCS
  struct userinfo user;
#endif
  
  do {
    proc = malloc(maxnproc * sizeof(struct ProcInfo));
    if (proc == NULL) {
      fprintf(stderr, "Problems with malloc.\n");
      exit(1);
    }
    
    /* Get process table */
    idx = 0;
    nproc = IFNEW(getprocs(proc, sizeof(struct procsinfo), NULL, 0,
			   &idx, maxnproc),
		  getproc(proc, maxnproc, sizeof(struct procinfo))
		  );
#ifdef DEBUG
    idx = errno; /* Don't ask... */
    if (debug)
      fprintf(stderr,
	      "nproc = %d maxnproc = %d" IFNEW(" idx = %d ","") "\n",
	      nproc, maxnproc, idx);
    errno = idx;
#endif
#ifdef USE_GETPROCS
    if (nproc == -1) {
      perror("getprocs");
      exit(1);
    } else if (nproc == maxnproc) {
      nproc = -1;
    }
#endif
    if (nproc == -1) {
      free(proc);
      maxnproc *= 2;
    } 
  } while (nproc == -1);
  
  P = malloc((nproc+1) * sizeof(struct Proc));
  if (P == NULL) {
    fprintf(stderr, "Problems with malloc.\n");
    exit(1);
  }
  
  for (i = 0; i < nproc; i++) {
#ifndef USE_GETPROCS
    getuser(&proc[i],sizeof(struct procinfo),
	    &user,   sizeof(struct userinfo));
#endif
    P[i].uid     = proc[i].pi_uid;
    P[i].pid     = proc[i].pi_pid;
    P[i].ppid    = proc[i].pi_ppid;
    P[i].pgl     = proc[i].pi_pid == proc[i].pi_pgrp;
    P[i].thcount = IFNEW(proc[i].pi_thcount, 1);
    
    uid2name(P[i].uid, P[i].name, sizeof(P[i].name));
    
    if (IFNEW(proc[i].pi_state,proc[i].pi_stat) == SZOMB) {
      strcpy(P[i].cmd, "<defunct>");
    } else {
      char *c = P[i].cmd;
      int ci = 0;
      getargs(&proc[i], sizeof(struct procinfo), c, MAXLINE - 2);
      c[MAXLINE-2] = c[MAXLINE-1] = '\0';

      /* Collect args. Stop when we encounter two '\0' */
      while (c[ci] != '\0' && (ci += strlen(&c[ci])) < MAXLINE - 2)
	c[ci++] = ' ';
      
      /* Drop trailing blanks */
      ci = strlen(c);
      while (ci > 0 && c[ci-1] == ' ') ci--;
      c[ci] = '\0';
      
      /* Replace some unprintables with '?' */
      for (ci = 0; c[ci] != '\0'; ci++)
	if (c[ci] == '\n' || c[ci] == '\t') c[ci] = '?';
      
      /* Insert [ui_comm] when getargs returns nothing */
      if (c[0] == '\0') {
	int l = strlen(IFNEW(proc[i].pi_comm,user.ui_comm));
	c[0] = '[';
	strcpy(c+1, IFNEW(proc[i].pi_comm,user.ui_comm));
	c[l+1] = ']';
	c[l+2] = '\0';
      }
    }
#ifdef DEBUG
    if (debug)
      fprintf(stderr,
	      "%d: uid=%5ld, name=%8s, pid=%5ld, ppid=%5ld, pgl=%d, tsize=%7u, dvm=%4u, "
	      "thcount=%2d, cmd[%d]='%s'\n",
	      i, P[i].uid, P[i].name, P[i].pid, P[i].ppid,
	      P[i].pgl,
	      IFNEW(proc[i].pi_tsize,user.ui_tsize),
	      IFNEW(proc[i].pi_dvm,user.ui_dvm),
	      proc[i].pi_thcount,
	      strlen(P[i].cmd),P[i].cmd);
#endif
    P[i].parent = P[i].child = P[i].sister = -1;
    P[i].print = FALSE;
  }
  free(proc);
  return nproc;
}

#else /* _AIX */

int GetProcesses(void) {
  FILE *tn;
  int len, i = 0;  extern int errno; /* For popen() */
#ifdef UID2USER
  int xx; /* For testing... */
#endif
  char line[MAXLINE], command[] = PSCMD;
  
  if (NULL == (tn = (FILE*)popen(command,"r"))) {
    fprintf(stderr, "Problems with pipe, errno = %d\n",errno);
    exit(1);
  }
#ifdef DEBUG
  if (debug) fprintf(stderr, "popen:errno = %d\n", errno);
#endif
  
  fgets(line, MAXLINE, tn); /* Throw away header line */
#ifdef DEBUG
  if (debug) fputs(line, stderr);
#endif
  
  P = malloc(sizeof(struct Proc));
  if (P == NULL) {
    fprintf(stderr, "Problems with malloc.\n");
    exit(1);
  }
  
  while (NULL != fgets(line, MAXLINE, tn) && 10 < (len = strlen(line))) {
#ifdef DEBUG
    if (debug) {
      fprintf(stderr, "len=%3d ", len);
      fputs(line, stderr);
    }
#endif
    
    P = realloc(P, (i+1) * sizeof(struct Proc));
    if (P == NULL) {
      fprintf(stderr, "Problems with realloc.\n");
      exit(1);
    }
    
    memset(&P[i], 0, sizeof(*P));
    
#ifdef solaris1x
    { /* SunOS allows columns to run together.  With the -j option, the CPU
       * time used can run into the numeric user id, so make sure there is
       * space between these two columns.  Also, the order of the desired
       * items is different. (L. Mark Larsen <mlarsen@ptdcs2.intel.com>)
       */
      char buf1[45], buf2[MAXLINE];
      buf1[44] = '\0';
      sscanf(line, "%44c%[^\n]", buf1, buf2);
      sprintf(line, "%s %s", buf1, buf2);
    }
#endif
    
    sscanf(line, PSFORMAT, PSVARS);
    
#ifdef UID2USER	/* get username */
    uid2name(P[i].uid, P[i].name, sizeof(P[i].name));
#endif

#ifdef DEBUG
    if (debug) fprintf(stderr,
		      "uid=%5ld, name=%8s, pid=%5ld, ppid=%5ld, cmd='%s'\n",
		      P[i].uid, P[i].name, P[i].pid, P[i].ppid, P[i].cmd);
#endif
    P[i].pgl = FALSE;
    P[i].parent = P[i].child = P[i].sister = -1;
    P[i].print = FALSE;
    i++;
  }
  pclose(tn);
  return i;
}
#endif /* _AIX */

int get_pid_index(long pid) {
  int i;
  for (i = NProc - 1;i >= 0 && P[i].pid != pid; i--); /* Search process */
  return i;
}

#define EXIST(idx) ((idx) != -1)

void MakeTree(void) {
  /* Build the process hierarchy. Every process marks itself as first child
   * of it's parent or as sister of first child of it's parent */
  int i, idx;
  
  for (i = 0; i < NProc; i++) {
    idx = get_pid_index(P[i].ppid);
    if (idx != i && idx != -1) { /* valid process, not me */
      P[i].parent = idx;
      if (P[idx].child == -1)
	P[idx].child = i;
      else {
	for (idx = P[idx].child; EXIST(P[idx].sister); idx = P[idx].sister);
	P[idx].sister = i;
      }
    }
  }
}

void MarkChildren(int i) {
  int idx;
  P[i].print = TRUE;
  for (idx = P[i].child; EXIST(idx); idx = P[idx].sister)
    MarkChildren(idx);
}

void MarkProcs(void) {
  int i;
  for (i = 0; i < NProc; i++) {
    if (showall) {
      P[i].print = TRUE;
    } else {
      int parent;
      if (0 == strcmp(P[i].name, name) 		/* for -u */
	 || (Uoption &&
	     0 != strcmp(P[i].name, "root"))	/* for -U */
	 || P[i].pid == ipid			/* for -p */
	 || (soption
	     && NULL != strstr(P[i].cmd, str)
	     && P[i].pid != MyPid)		/* for -s */
	 ) {
	/* Mark parents */
	for (parent = P[i].parent; EXIST(parent); parent = P[parent].parent) {
	  P[parent].print = TRUE;
	}
	/* Mark children */
	MarkChildren(i);
      }
    }
  }
}

void DropProcs(void) {
  int i;
  for (i = 0; i < NProc; i++) if (P[i].print) {
    int idx;
    /* Drop children that won't print */
    for (idx = P[i].child;
	EXIST(idx) && !P[idx].print; idx = P[idx].sister);
    P[i].child = idx;
    /* Drop sisters that won't print */
    for (idx = P[i].sister;
	EXIST(idx) && !P[idx].print; idx = P[idx].sister);
    P[i].sister = idx;
  }
}

void PrintTree(int idx, const char *head) {
  char nhead[MAXLINE], out[4 * MAXLINE], thread[16] = {'\0'};
  int child;
  
  if (head[0] == '\0' && !P[idx].print) return;
  
  if (P[idx].thcount > 1) sprintf(thread, "[%d]", P[idx].thcount);
  
  sprintf(out,
	  "%s%s%s%c %05d %s %s%s" /*" (ch=%d, si=%d, pr=%d)"*/,
	  head,
	  head[0] == '\0' ? "" : EXIST(P[idx].sister) ? C->barc : C->barl,
	  EXIST(P[idx].child)  ? C->p    : C->s2,
	  P[idx].pgl ? C->pgl : C->npgl,
	  P[idx].pid, P[idx].name,
	  thread,
	  P[idx].cmd
	  /*,P[idx].child,P[idx].sister,P[idx].print*/);
  
  out[Columns-1] = '\0';
  puts(out);
  
  /* Process children */
  sprintf(nhead, "%s%s ", head,
	  head[0] == '\0' ? "" : EXIST(P[idx].sister) ? C->bar : " ");
  
  for (child = P[idx].child; EXIST(child); child = P[child].sister)
    PrintTree(child, nhead);
}

void Usage(void) {
  fprintf(stderr,
	  "%s\n"
	  "%s\n\n"
	  "Usage: %s "
#ifdef DEBUG
	  "[-d] "
#endif
	  "[-g] [-u user] [-U] [-s string] [-p pid] [-w] [pid ...]\n"
	  /*"   -a        align output\n"*/
#ifdef DEBUG
	  "   -d        print debugging info to stderr\n"
#endif
	  "   -g        use IBM-850 graphics chars for tree\n"
	  "   -u user   show only parts containing processes of <user>\n"
	  "   -U        don't show parts containing only root processes\n"
          "   -s string show only parts containing process with <string> in commandline\n"
          "   -p pid    show only parts containing process <pid>\n"
	  "   -w        wide output, not truncated to window width\n"
	  "   pid ...   process ids to start from, default is 1 (init)\n"
	  "             use 0 to also show kernel processes\n"
	  , WhatString[0] + 4, WhatString[1] + 4, Progname);
#if defined(_AIX) || defined(___AIX)	/* AIX 3.x */
  fprintf(stderr, "\nProcess group leaders are marked with '='.\n");
#endif
  exit(1);
}

int main(int argc, char **argv) {
  extern int optind;
  extern char *optarg;
  int ch;
  long pid;
  int graph = FALSE, wide = FALSE;
  Progname = strrchr(argv[0],'/');
  Progname = (NULL == Progname) ? argv[0] : Progname+1;
  
  while ((ch = getopt(argc, argv, "dghp:s:u:Uw?")) != EOF)
    switch(ch) {
      /*case 'a':
	align   = TRUE;
	break;*/
#ifdef DEBUG
    case 'd':
      debug   = TRUE;
      break;
#endif
    case 'g':
      graph   = TRUE;
      break;
    case 'p':
      showall = FALSE;
      ipid    = atoi(optarg);
      break;
    case 's':
      showall = FALSE;
      soption = TRUE;
      str     = optarg;
      break;
    case 'u':
      showall = FALSE;
      name    = optarg;
      if (
#ifdef solaris2x
	 (int)
#endif
	 NULL == getpwnam(name)) {
	fprintf(stderr, "%s: User '%s' does not exist.\n",
		Progname, name);
	exit(1);
      }
      break;
    case 'U':
      showall = FALSE;
      Uoption = TRUE;
      break;
    case 'w':
      wide    = TRUE;
      break;
    case 'h':
    case '?':
    default :
      Usage();
      break;
    }
  
  NProc = GetProcesses();
#if defined(UID2USER) && defined(DEBUG)
  if (debug) uid2name(0,NULL,0);
#endif
  MyPid = getpid();
  if (wide)
    Columns = MAXLINE - 1;
  else {
#ifdef HAS_TERMDEF
    Columns = atoi((char*)termdef(fileno(stdout),'c'));
#else
    Columns = 80;
#endif
  }
  if (Columns == 0 || Columns >= MAXLINE) Columns = MAXLINE - 1;
  
  C = graph ? &Pc850 : &Ascii;
  
  MakeTree();
  MarkProcs();
  DropProcs();
  
  if (argc == optind) /* No pids */
    PrintTree(get_pid_index(1), "");
  else while (optind < argc) {
    pid = (long)atoi(argv[optind]);
    PrintTree(get_pid_index(pid), "");
    optind++;
  }
  free(P);
  return 0;
}
