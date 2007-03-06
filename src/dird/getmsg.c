/*
 *
 *   Bacula Director -- routines to receive network data and
 *    handle network signals. These routines handle the connections
 *    to the Storage daemon and the File daemon.
 *
 *     Kern Sibbald, August MM
 *
 *    This routine runs as a thread and must be thread reentrant.
 *
 *  Basic tasks done here:
 *    Handle  network signals (signals).
 *       Signals always have return status 0 from bnet_recv() and
 *       a zero or negative message length.
 *    Pass appropriate messages back to the caller (responses).
 *       Responses always have a digit as the first character.
 *    Handle requests for message and catalog services (requests).
 *       Requests are any message that does not begin with a digit.
 *       In affect, they are commands.
 *
 *   Version $Id: getmsg.c,v 1.27 2005/09/26 14:37:35 kerns Exp $
 */
/*
   Copyright (C) 2000-2005 Kern Sibbald

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
#include "dird.h"

/* Forward referenced functions */
static char *find_msg_start(char *msg);

static char Job_status[] = "Status Job=%127s JobStatus=%d\n";
#ifdef needed
static char Device_update[]   = "DevUpd Job=%127s "
   "device=%127s "
   "append=%d read=%d num_writers=%d "
   "open=%d labeled=%d offline=%d "
   "reserved=%d max_writers=%d "
   "autoselect=%d autochanger=%d "
   "changer_name=%127s media_type=%127s volume_name=%127s\n";
#endif


static char OK_msg[] = "1000 OK\n";

/*
 * Get a message
 *  Call appropriate processing routine
 *  If it is not a Jmsg or a ReqCat message,
 *   return it to the caller.
 *
 *  This routine is called to get the next message from
 *  another daemon. If the message is in canonical message
 *  format and the type is known, it will be dispatched
 *  to the appropriate handler.  If the message is
 *  in any other format, it will be returned.
 *
 *  E.g. any message beginning with a digit will be passed   
 *       through to the caller.
 *  All other messages are expected begin with some identifier
 *    -- for the moment only the first character is checked, but
 *    at a later time, the whole identifier (e.g. Jmsg, CatReq, ...)
 *    could be checked. This is followed by Job=Jobname <user-defined>
 *    info. The identifier is used to dispatch the message to the right
 *    place (Job message, catalog request, ...). The Job is used to lookup
 *    the JCR so that the action is performed on the correct jcr, and
 *    the rest of the message is up to the user.  Note, DevUpd uses
 *    *System* for the Job name, and hence no JCR is obtained. This   
 *    is a *rare* case where a jcr is not really needed.
 *
 */
int bget_dirmsg(BSOCK *bs)
{
   int32_t n;
   char Job[MAX_NAME_LENGTH];
   char MsgType[20];
   int type, level;
   JCR *jcr;
   char *msg;

   for (;;) {
      n = bnet_recv(bs);
      Dmsg2(900, "bget_dirmsg %d: %s", n, bs->msg);

      if (is_bnet_stop(bs)) {
         return n;                    /* error or terminate */
      }
      if (n == BNET_SIGNAL) {          /* handle signal */
         /* BNET_SIGNAL (-1) return from bnet_recv() => network signal */
         switch (bs->msglen) {
         case BNET_EOD:            /* end of data */
            return n;
         case BNET_EOD_POLL:
            bnet_fsend(bs, OK_msg);/* send response */
            return n;              /* end of data */
         case BNET_TERMINATE:
            bs->terminated = 1;
            return n;
         case BNET_POLL:
            bnet_fsend(bs, OK_msg); /* send response */
            break;
         case BNET_HEARTBEAT:
//          encode_time(time(NULL), Job);
//          Dmsg1(100, "%s got heartbeat.\n", Job);
            break;
         case BNET_HB_RESPONSE:
            break;
         case BNET_STATUS:
            /* *****FIXME***** Implement more completely */
            bnet_fsend(bs, "Status OK\n");
            bnet_sig(bs, BNET_EOD);
            break;
         case BNET_BTIME:             /* send Bacula time */
            char ed1[50];
            bnet_fsend(bs, "btime %s\n", edit_uint64(get_current_btime(),ed1));
            break;
         default:
            Emsg1(M_WARNING, 0, _("bget_dirmsg: unknown bnet signal %d\n"), bs->msglen);
            return n;
         }
         continue;
      }

      /* Handle normal data */

      if (n > 0 && B_ISDIGIT(bs->msg[0])) {      /* response? */
         return n;                    /* yes, return it */
      }

      /*
       * If we get here, it must be a request.  Either
       *  a message to dispatch, or a catalog request.
       *  Try to fulfill it.
       */
      if (sscanf(bs->msg, "%020s Job=%127s ", MsgType, Job) != 2) {
         Emsg1(M_ERROR, 0, _("Malformed message: %s\n"), bs->msg);
         continue;
      }
      if (strcmp(Job, "*System*") == 0) {
         jcr = NULL;                  /* No jcr */
      } else if (!(jcr=get_jcr_by_full_name(Job))) {
         Emsg1(M_ERROR, 0, _("Job not found: %s\n"), bs->msg);
         continue;
      }
      Dmsg1(900, "Getmsg got jcr 0x%x\n", jcr);

      /* Skip past "Jmsg Job=nnn" */
      if (!(msg=find_msg_start(bs->msg))) {
         Emsg1(M_ERROR, 0, _("Malformed message: %s\n"), bs->msg);
         free_jcr(jcr);
         continue;
      }

      /*
       * Here we are expecting a message of the following format:
       *   Jmsg Job=nnn type=nnn level=nnn Message-string
       */
      if (bs->msg[0] == 'J') {           /* Job message */
         if (sscanf(bs->msg, "Jmsg Job=%127s type=%d level=%d",
                    Job, &type, &level) != 3) {
            Emsg1(M_ERROR, 0, _("Malformed message: %s\n"), bs->msg);
            free_jcr(jcr);
            continue;
         }
         Dmsg1(900, "Got msg: %s\n", bs->msg);
         skip_spaces(&msg);
         skip_nonspaces(&msg);        /* skip type=nnn */
         skip_spaces(&msg);
         skip_nonspaces(&msg);        /* skip level=nnn */
         if (*msg == ' ') {
            msg++;                    /* skip leading space */
         }
         Dmsg1(900, "Dispatch msg: %s", msg);
         dispatch_message(jcr, type, level, msg);
         free_jcr(jcr);
         continue;
      }
      /*
       * Here we expact a CatReq message
       *   CatReq Job=nn Catalog-Request-Message
       */
      if (bs->msg[0] == 'C') {        /* Catalog request */
         Dmsg2(900, "Catalog req jcr 0x%x: %s", jcr, bs->msg);
         catalog_request(jcr, bs);
         Dmsg1(900, "Calling freejcr 0x%x\n", jcr);
         free_jcr(jcr);
         continue;
      }
      if (bs->msg[0] == 'U') {        /* Catalog update */
         Dmsg2(900, "Catalog upd jcr 0x%x: %s", jcr, bs->msg);
         catalog_update(jcr, bs);
         Dmsg1(900, "Calling freejcr 0x%x\n", jcr);
         free_jcr(jcr);
         continue;
      }
      if (bs->msg[0] == 'M') {        /* Mount request */
         Dmsg1(900, "Mount req: %s", bs->msg);
         mount_request(jcr, bs, msg);
         free_jcr(jcr);
         continue;
      }
      if (bs->msg[0] == 'S') {       /* Status change */
         int JobStatus;
         char Job[MAX_NAME_LENGTH];
         if (sscanf(bs->msg, Job_status, &Job, &JobStatus) == 2) {
            jcr->SDJobStatus = JobStatus; /* current status */
         } else {
            Emsg1(M_ERROR, 0, _("Malformed message: %s\n"), bs->msg);
         }
         free_jcr(jcr);
         continue;
      }
#ifdef needed
      /* No JCR for Device Updates! */
      if (bs->msg[0] = 'D') {         /* Device update */
         DEVICE *dev;
         POOL_MEM dev_name, changer_name, media_type, volume_name;
         int dev_open, dev_append, dev_read, dev_labeled;
         int dev_offline, dev_autochanger, dev_autoselect;
         int dev_num_writers, dev_max_writers, dev_reserved;
         uint64_t dev_PoolId;
         Dmsg1(100, "<stored: %s", bs->msg);
         if (sscanf(bs->msg, Device_update,
             &Job, dev_name.c_str(),
             &dev_append, &dev_read,
             &dev_num_writers, &dev_open,
             &dev_labeled, &dev_offline, &dev_reserved,
             &dev_max_writers, &dev_autoselect, 
             &dev_autochanger, 
             changer_name.c_str(), media_type.c_str(),
             volume_name.c_str()) != 15) {
            Emsg1(M_ERROR, 0, _("Malformed message: %s\n"), bs->msg);
         } else {
            unbash_spaces(dev_name);
            dev = (DEVICE *)GetResWithName(R_DEVICE, dev_name.c_str());
            if (!dev) {
               continue;
            }
            unbash_spaces(changer_name);
            unbash_spaces(media_type);
            unbash_spaces(volume_name);
            bstrncpy(dev->ChangerName, changer_name.c_str(), sizeof(dev->ChangerName));
            bstrncpy(dev->MediaType, media_type.c_str(), sizeof(dev->MediaType));
            bstrncpy(dev->VolumeName, volume_name.c_str(), sizeof(dev->VolumeName));
            /* Note, these are copied because they are boolean rather than
             *  integer.
             */
            dev->open = dev_open;
            dev->append = dev_append;
            dev->read = dev_read;
            dev->labeled = dev_labeled;
            dev->offline = dev_offline;
            dev->autoselect = dev_autoselect;
            dev->autochanger = dev_autochanger > 0;
            dev->num_drives = dev_autochanger;    /* does double duty */
            dev->PoolId = dev_PoolId;
            dev->num_writers = dev_num_writers;
            dev->max_writers = dev_max_writers;
            dev->reserved = dev_reserved;
            dev->found = true;
         }
         continue;
      }
#endif
      return n;
   }
}

static char *find_msg_start(char *msg)
{
   char *p = msg;

   skip_nonspaces(&p);                /* skip message type */
   skip_spaces(&p);
   skip_nonspaces(&p);                /* skip Job */
   skip_spaces(&p);                   /* after spaces come the message */
   return p;
}

/*
 * Get response from FD or SD to a command we
 * sent. Check that the response agrees with what we expect.
 *
 *  Returns: false on failure
 *           true  on success
 */
bool response(JCR *jcr, BSOCK *bs, char *resp, const char *cmd, e_prtmsg prtmsg)
{
   int n;

   if (is_bnet_error(bs)) {
      return false;
   }
   if ((n = bget_dirmsg(bs)) >= 0) {
      Dmsg0(900, bs->msg);
      if (strcmp(bs->msg, resp) == 0) {
         return true;
      }
      Dmsg1(900, "Bad response: ERR=%s", bs->msg);
      if (prtmsg == DISPLAY_ERROR) {
         Jmsg(jcr, M_FATAL, 0, _("Bad response to %s command: wanted %s, got %s\n"),
            cmd, resp, bs->msg);
      }
      return false;
   }
   Jmsg(jcr, M_FATAL, 0, _("Socket error on %s command: ERR=%s\n"),
         cmd, bnet_strerror(bs));
   return false;
}