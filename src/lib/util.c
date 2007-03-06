/*
 *   util.c  miscellaneous utility subroutines for Bacula
 *
 *    Kern Sibbald, MM
 *
 *   Version $Id: util.c,v 1.68.2.3 2006/03/04 11:10:18 kerns Exp $
 */
/*
   Copyright (C) 2000-2006 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as amended with additional clauses defined in the
   file LICENSE in the main source directory.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the 
   the file LICENSE for additional details.

 */

#include "bacula.h"
#include "jcr.h"
#include "findlib/find.h"

/*
 * Various Bacula Utility subroutines
 *
 */

/* Return true of buffer has all zero bytes */
int is_buf_zero(char *buf, int len)
{
   uint64_t *ip;
   char *p;
   int i, len64, done, rem;

   if (buf[0] != 0) {
      return 0;
   }
   ip = (uint64_t *)buf;
   /* Optimize by checking uint64_t for zero */
   len64 = len / sizeof(uint64_t);
   for (i=0; i < len64; i++) {
      if (ip[i] != 0) {
         return 0;
      }
   }
   done = len64 * sizeof(uint64_t);  /* bytes already checked */
   p = buf + done;
   rem = len - done;
   for (i = 0; i < rem; i++) {
      if (p[i] != 0) {
         return 0;
      }
   }
   return 1;
}


/* Convert a string in place to lower case */
void lcase(char *str)
{
   while (*str) {
      if (B_ISUPPER(*str))
         *str = tolower((int)(*str));
       str++;
   }
}

/* Convert spaces to non-space character.
 * This makes scanf of fields containing spaces easier.
 */
void
bash_spaces(char *str)
{
   while (*str) {
      if (*str == ' ')
         *str = 0x1;
      str++;
   }
}

/* Convert spaces to non-space character.
 * This makes scanf of fields containing spaces easier.
 */
void
bash_spaces(POOL_MEM &pm)
{
   char *str = pm.c_str();
   while (*str) {
      if (*str == ' ')
         *str = 0x1;
      str++;
   }
}


/* Convert non-space characters (0x1) back into spaces */
void
unbash_spaces(char *str)
{
   while (*str) {
     if (*str == 0x1)
        *str = ' ';
     str++;
   }
}

/* Convert non-space characters (0x1) back into spaces */
void
unbash_spaces(POOL_MEM &pm)
{
   char *str = pm.c_str();
   while (*str) {
     if (*str == 0x1)
        *str = ' ';
     str++;
   }
}

#if    HAVE_WIN32 && !HAVE_CONSOLE && !HAVE_WXCONSOLE
extern long _timezone;
extern int _daylight;
extern long _dstbias;
extern "C" void __tzset(void);
extern "C" int _isindst(struct tm *);
#endif

char *encode_time(time_t time, char *buf)
{
   struct tm tm;
   int n = 0;

#if    HAVE_WIN32 && !HAVE_CONSOLE && !HAVE_WXCONSOLE
    /*
     * Gross kludge to avoid a seg fault in Microsoft's CRT localtime_r(),
     *  which incorrectly references a NULL returned from gmtime() if
     *  the time (adjusted for the current timezone) is invalid.
     *  This could happen if you have a bad date/time, or perhaps if you
     *  moved a file from one timezone to another?
     */
    struct tm *gtm;
    time_t gtime;
    __tzset();
    gtime = time - _timezone;
    if (!(gtm = gmtime(&gtime))) {
       return buf;
    }
    if (_daylight && _isindst(gtm)) {
       gtime -= _dstbias;
       if (!gmtime(&gtime)) {
          return buf;
       }
    }
#endif
   if (localtime_r(&time, &tm)) {
      n = sprintf(buf, "%04d-%02d-%02d %02d:%02d:%02d",
                   tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                   tm.tm_hour, tm.tm_min, tm.tm_sec);
   }
   return buf+n;
}



/*
 * Convert a JobStatus code into a human readable form
 */
void jobstatus_to_ascii(int JobStatus, char *msg, int maxlen)
{
   const char *jobstat;
   char buf[100];

   switch (JobStatus) {
   case JS_Created:
      jobstat = _("Created");
      break;
   case JS_Running:
      jobstat = _("Running");
      break;
   case JS_Blocked:
      jobstat = _("Blocked");
      break;
   case JS_Terminated:
      jobstat = _("OK");
      break;
   case JS_FatalError:
   case JS_ErrorTerminated:
      jobstat = _("Error");
      break;
   case JS_Error:
      jobstat = _("Non-fatal error");
      break;
   case JS_Canceled:
      jobstat = _("Canceled");
      break;
   case JS_Differences:
      jobstat = _("Verify differences");
      break;
   case JS_WaitFD:
      jobstat = _("Waiting on FD");
      break;
   case JS_WaitSD:
      jobstat = _("Wait on SD");
      break;
   case JS_WaitMedia:
      jobstat = _("Wait for new Volume");
      break;
   case JS_WaitMount:
      jobstat = _("Waiting for mount");
      break;
   case JS_WaitStoreRes:
      jobstat = _("Waiting for Storage resource");
      break;
   case JS_WaitJobRes:
      jobstat = _("Waiting for Job resource");
      break;
   case JS_WaitClientRes:
      jobstat = _("Waiting for Client resource");
      break;
   case JS_WaitMaxJobs:
      jobstat = _("Waiting on Max Jobs");
      break;
   case JS_WaitStartTime:
      jobstat = _("Waiting for Start Time");
      break;
   case JS_WaitPriority:
      jobstat = _("Waiting on Priority");
      break;

   default:
      if (JobStatus == 0) {
         buf[0] = 0;
      } else {
         bsnprintf(buf, sizeof(buf), _("Unknown Job termination status=%d"), JobStatus);
      }
      jobstat = buf;
      break;
   }
   bstrncpy(msg, jobstat, maxlen);
}

/*
 * Convert Job Termination Status into a string
 */
const char *job_status_to_str(int stat)
{
   const char *str;

   switch (stat) {
   case JS_Terminated:
      str = _("OK");
      break;
   case JS_ErrorTerminated:
   case JS_Error:
      str = _("Error");
      break;
   case JS_FatalError:
      str = _("Fatal Error");
      break;
   case JS_Canceled:
      str = _("Canceled");
      break;
   case JS_Differences:
      str = _("Differences");
      break;
   default:
      str = _("Unknown term code");
      break;
   }
   return str;
}


/*
 * Convert Job Type into a string
 */
const char *job_type_to_str(int type)
{
   const char *str;

   switch (type) {
   case JT_BACKUP:
      str = _("Backup");
      break;
   case JT_VERIFY:
      str = _("Verify");
      break;
   case JT_RESTORE:
      str = _("Restore");
      break;
   case JT_ADMIN:
      str = _("Admin");
      break;
   case JT_MIGRATE:
      str = _("Migrate");
      break;
   case JT_COPY:
      str = _("Copy");
      break;
   default:
      str = _("Unknown Type");
      break;
   }
   return str;
}

/*
 * Convert Job Level into a string
 */
const char *job_level_to_str(int level)
{
   const char *str;

   switch (level) {
   case L_BASE:
      str = _("Base");
   case L_FULL:
      str = _("Full");
      break;
   case L_INCREMENTAL:
      str = _("Incremental");
      break;
   case L_DIFFERENTIAL:
      str = _("Differential");
      break;
   case L_SINCE:
      str = _("Since");
      break;
   case L_VERIFY_CATALOG:
      str = _("Verify Catalog");
      break;
   case L_VERIFY_INIT:
      str = _("Verify Init Catalog");
      break;
   case L_VERIFY_VOLUME_TO_CATALOG:
      str = _("Verify Volume to Catalog");
      break;
   case L_VERIFY_DISK_TO_CATALOG:
      str = _("Verify Disk to Catalog");
      break;
   case L_VERIFY_DATA:
      str = _("Verify Data");
      break;
   case L_NONE:
      str = " ";
      break;
   default:
      str = _("Unknown Job Level");
      break;
   }
   return str;
}


/***********************************************************************
 * Encode the mode bits into a 10 character string like LS does
 ***********************************************************************/

char *encode_mode(mode_t mode, char *buf)
{
  char *cp = buf;

  *cp++ = S_ISDIR(mode) ? 'd' : S_ISBLK(mode)  ? 'b' : S_ISCHR(mode)  ? 'c' :
          S_ISLNK(mode) ? 'l' : S_ISFIFO(mode) ? 'f' : S_ISSOCK(mode) ? 's' : '-';
  *cp++ = mode & S_IRUSR ? 'r' : '-';
  *cp++ = mode & S_IWUSR ? 'w' : '-';
  *cp++ = (mode & S_ISUID
               ? (mode & S_IXUSR ? 's' : 'S')
               : (mode & S_IXUSR ? 'x' : '-'));
  *cp++ = mode & S_IRGRP ? 'r' : '-';
  *cp++ = mode & S_IWGRP ? 'w' : '-';
  *cp++ = (mode & S_ISGID
               ? (mode & S_IXGRP ? 's' : 'S')
               : (mode & S_IXGRP ? 'x' : '-'));
  *cp++ = mode & S_IROTH ? 'r' : '-';
  *cp++ = mode & S_IWOTH ? 'w' : '-';
  *cp++ = (mode & S_ISVTX
               ? (mode & S_IXOTH ? 't' : 'T')
               : (mode & S_IXOTH ? 'x' : '-'));
  *cp = '\0';
  return cp;
}


int do_shell_expansion(char *name, int name_len)
{
   static char meta[] = "~\\$[]*?`'<>\"";
   bool found = false;
   int len, i, stat;
   POOLMEM *cmd;
   BPIPE *bpipe;
   char line[MAXSTRING];
   const char *shellcmd;

   /* Check if any meta characters are present */
   len = strlen(meta);
   for (i = 0; i < len; i++) {
      if (strchr(name, meta[i])) {
         found = true;
         break;
      }
   }
   if (found) {
      cmd =  get_pool_memory(PM_FNAME);
      /* look for shell */
      if ((shellcmd = getenv("SHELL")) == NULL) {
         shellcmd = "/bin/sh";
      }
      pm_strcpy(&cmd, shellcmd);
      pm_strcat(&cmd, " -c \"echo ");
      pm_strcat(&cmd, name);
      pm_strcat(&cmd, "\"");
      Dmsg1(400, "Send: %s\n", cmd);
      if ((bpipe = open_bpipe(cmd, 0, "r"))) {
         *line = 0;
         fgets(line, sizeof(line), bpipe->rfd);
         strip_trailing_junk(line);
         stat = close_bpipe(bpipe);
         Dmsg2(400, "stat=%d got: %s\n", stat, line);
      } else {
         stat = 1;                    /* error */
      }
      free_pool_memory(cmd);
      if (stat == 0) {
         bstrncpy(name, line, name_len);
      }
   }
   return 1;
}


/*  MAKESESSIONKEY  --  Generate session key with optional start
                        key.  If mode is TRUE, the key will be
                        translated to a string, otherwise it is
                        returned as 16 binary bytes.

    from SpeakFreely by John Walker */

void make_session_key(char *key, char *seed, int mode)
{
     int j, k;
     struct MD5Context md5c;
     unsigned char md5key[16], md5key1[16];
     char s[1024];

     s[0] = 0;
     if (seed != NULL) {
        bstrncat(s, seed, sizeof(s));
     }

     /* The following creates a seed for the session key generator
        based on a collection of volatile and environment-specific
        information unlikely to be vulnerable (as a whole) to an
        exhaustive search attack.  If one of these items isn't
        available on your machine, replace it with something
        equivalent or, if you like, just delete it. */

     sprintf(s + strlen(s), "%lu", (unsigned long)getpid());
     sprintf(s + strlen(s), "%lu", (unsigned long)getppid());
     (void)getcwd(s + strlen(s), 256);
     sprintf(s + strlen(s), "%lu", (unsigned long)clock());
     sprintf(s + strlen(s), "%lu", (unsigned long)time(NULL));
#ifdef Solaris
     sysinfo(SI_HW_SERIAL,s + strlen(s), 12);
#endif
#ifdef HAVE_GETHOSTID
     sprintf(s + strlen(s), "%lu", (unsigned long) gethostid());
#endif
     gethostname(s + strlen(s), 256);
     sprintf(s + strlen(s), "%u", (unsigned)getuid());
     sprintf(s + strlen(s), "%u", (unsigned)getgid());
     MD5Init(&md5c);
     MD5Update(&md5c, (unsigned char *)s, strlen(s));
     MD5Final(md5key, &md5c);
     sprintf(s + strlen(s), "%lu", (unsigned long)((time(NULL) + 65121) ^ 0x375F));
     MD5Init(&md5c);
     MD5Update(&md5c, (unsigned char *)s, strlen(s));
     MD5Final(md5key1, &md5c);
#define nextrand    (md5key[j] ^ md5key1[j])
     if (mode) {
        for (j = k = 0; j < 16; j++) {
           unsigned char rb = nextrand;

#define Rad16(x) ((x) + 'A')
           key[k++] = Rad16((rb >> 4) & 0xF);
           key[k++] = Rad16(rb & 0xF);
#undef Rad16
           if (j & 1) {
              key[k++] = '-';
           }
        }
        key[--k] = 0;
     } else {
        for (j = 0; j < 16; j++) {
           key[j] = nextrand;
        }
     }
}
#undef nextrand



/*
 * Edit job codes into main command line
 *  %% = %
 *  %c = Client's name
 *  %d = Director's name
 *  %e = Job Exit code
 *  %i = JobId
 *  %j = Unique Job id
 *  %l = job level
 *  %n = Unadorned Job name
 *  %s = Since time
 *  %t = Job type (Backup, ...)
 *  %r = Recipients
 *  %v = Volume name
 *
 *  omsg = edited output message
 *  imsg = input string containing edit codes (%x)
 *  to = recepients list
 *
 */
POOLMEM *edit_job_codes(JCR *jcr, char *omsg, char *imsg, const char *to)
{
   char *p, *q;
   const char *str;
   char add[20];
   char name[MAX_NAME_LENGTH];
   int i;

   *omsg = 0;
   Dmsg1(200, "edit_job_codes: %s\n", imsg);
   for (p=imsg; *p; p++) {
      if (*p == '%') {
         switch (*++p) {
         case '%':
            str = "%";
            break;
         case 'c':
            if (jcr) {
               str = jcr->client_name;
            } else {
               str = _("*none*");
            }
            break;
         case 'd':
            str = my_name;            /* Director's name */
            break;
         case 'e':
            if (jcr) {
               str = job_status_to_str(jcr->JobStatus);
            } else {
               str = _("*none*");
            }
            break;
         case 'i':
            if (jcr) {
               bsnprintf(add, sizeof(add), "%d", jcr->JobId);
               str = add;
            } else {
               str = _("*none*");
            }
            break;
         case 'j':                    /* Job name */
            if (jcr) {
               str = jcr->Job;
            } else {
               str = _("*none*");
            }
            break;
         case 'l':
            if (jcr) {
               str = job_level_to_str(jcr->JobLevel);
            } else {
               str = _("*none*");
            }
            break;
         case 'n':
             if (jcr) {
                bstrncpy(name, jcr->Job, sizeof(name));
                /* There are three periods after the Job name */
                for (i=0; i<3; i++) {
                   if ((q=strrchr(name, '.')) != NULL) {
                       *q = 0;
                   }
                }
                str = name;
             } else {
                str = _("*none*");
             }
             break;
         case 'r':
            str = to;
            break;
         case 's':                    /* since time */
            if (jcr && jcr->stime) {
               str = jcr->stime;
            } else {
               str = _("*none*");
            }
            break;
         case 't':
            if (jcr) {
               str = job_type_to_str(jcr->JobType);
            } else {
               str = _("*none*");
            }
            break;
         case 'v':
            if (jcr) {
               if (jcr->VolumeName && jcr->VolumeName[0]) {
                  str = jcr->VolumeName;
               } else {
                  str = "";
               }
            } else {
               str = _("*none*");
            }
            break;
         default:
            add[0] = '%';
            add[1] = *p;
            add[2] = 0;
            str = add;
            break;
         }
      } else {
         add[0] = *p;
         add[1] = 0;
         str = add;
      }
      Dmsg1(1200, "add_str %s\n", str);
      pm_strcat(&omsg, str);
      Dmsg1(1200, "omsg=%s\n", omsg);
   }
   return omsg;
}

void set_working_directory(char *wd)
{
   struct stat stat_buf;

   if (wd == NULL) {
      Emsg0(M_ERROR_TERM, 0, _("Working directory not defined. Cannot continue.\n"));
   }
   if (stat(wd, &stat_buf) != 0) {
      Emsg1(M_ERROR_TERM, 0, _("Working Directory: \"%s\" not found. Cannot continue.\n"),
         wd);
   }
   if (!S_ISDIR(stat_buf.st_mode)) {
      Emsg1(M_ERROR_TERM, 0, _("Working Directory: \"%s\" is not a directory. Cannot continue.\n"),
         wd);
   }
   working_directory = wd;            /* set global */
}