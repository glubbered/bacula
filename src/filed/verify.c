/*
 *  Bacula File Daemon  verify.c  Verify files.
 *
 *    Kern Sibbald, October MM
 *
 *   Version $Id: verify.c,v 1.44.2.1 2006/03/14 21:41:40 kerns Exp $
 *
 */
/*
   Copyright (C) 2000-2004 Kern Sibbald

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public
   License along with this program; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
   MA 02111-1307, USA.

 */

#include "bacula.h"
#include "filed.h"

static int verify_file(FF_PKT *ff_pkt, void *my_pkt, bool);
static int read_chksum(BFILE *bfd, CHKSUM *chksum, JCR *jcr);

/*
 * Find all the requested files and send attributes
 * to the Director.
 *
 */
void do_verify(JCR *jcr)
{
   set_jcr_job_status(jcr, JS_Running);
   jcr->buf_size = DEFAULT_NETWORK_BUFFER_SIZE;
   if ((jcr->big_buf = (char *) malloc(jcr->buf_size)) == NULL) {
      Jmsg1(jcr, M_ABORT, 0, _("Cannot malloc %d network read buffer\n"),
         DEFAULT_NETWORK_BUFFER_SIZE);
   }
   set_find_options((FF_PKT *)jcr->ff, jcr->incremental, jcr->mtime);
   Dmsg0(10, "Start find files\n");
   /* Subroutine verify_file() is called for each file */
   find_files(jcr, (FF_PKT *)jcr->ff, verify_file, (void *)jcr);
   Dmsg0(10, "End find files\n");

   if (jcr->big_buf) {
      free(jcr->big_buf);
      jcr->big_buf = NULL;
   }
   set_jcr_job_status(jcr, JS_Terminated);
}

/*
 * Called here by find() for each file.
 *
 *  Find the file, compute the MD5 or SHA1 and send it back to the Director
 */
static int verify_file(FF_PKT *ff_pkt, void *pkt, bool top_level)
{
   char attribs[MAXSTRING];
   char attribsEx[MAXSTRING];
   int stat;
   BFILE bfd;
   struct CHKSUM chksum;
   BSOCK *dir;
   JCR *jcr = (JCR *)pkt;

   if (job_canceled(jcr)) {
      return 0;
   }

   dir = jcr->dir_bsock;
   jcr->num_files_examined++;         /* bump total file count */

   switch (ff_pkt->type) {
   case FT_LNKSAVED:                  /* Hard linked, file already saved */
      Dmsg2(30, "FT_LNKSAVED saving: %s => %s\n", ff_pkt->fname, ff_pkt->link);
      break;
   case FT_REGE:
      Dmsg1(30, "FT_REGE saving: %s\n", ff_pkt->fname);
      break;
   case FT_REG:
      Dmsg1(30, "FT_REG saving: %s\n", ff_pkt->fname);
      break;
   case FT_LNK:
      Dmsg2(30, "FT_LNK saving: %s -> %s\n", ff_pkt->fname, ff_pkt->link);
      break;
   case FT_DIRBEGIN:
      return 1;                       /* ignored */
   case FT_DIREND:
      Dmsg1(30, "FT_DIR saving: %s\n", ff_pkt->fname);
      break;
   case FT_SPEC:
      Dmsg1(30, "FT_SPEC saving: %s\n", ff_pkt->fname);
      break;
   case FT_RAW:
      Dmsg1(30, "FT_RAW saving: %s\n", ff_pkt->fname);
      break;
   case FT_FIFO:
      Dmsg1(30, "FT_FIFO saving: %s\n", ff_pkt->fname);
      break;
   case FT_NOACCESS: {
      berrno be;
      be.set_errno(ff_pkt->ff_errno);
      Jmsg(jcr, M_NOTSAVED, 1, _("     Could not access %s: ERR=%s\n"), ff_pkt->fname, be.strerror());
      jcr->Errors++;
      return 1;
   }
   case FT_NOFOLLOW: {
      berrno be;
      be.set_errno(ff_pkt->ff_errno);
      Jmsg(jcr, M_NOTSAVED, 1, _("     Could not follow link %s: ERR=%s\n"), ff_pkt->fname, be.strerror());
      jcr->Errors++;
      return 1;
   }
   case FT_NOSTAT: {
      berrno be;
      be.set_errno(ff_pkt->ff_errno);
      Jmsg(jcr, M_NOTSAVED, 1, _("     Could not stat %s: ERR=%s\n"), ff_pkt->fname, be.strerror());
      jcr->Errors++;
      return 1;
   }
   case FT_DIRNOCHG:
   case FT_NOCHG:
      Jmsg(jcr, M_SKIPPED, 1, _("     Unchanged file skipped: %s\n"), ff_pkt->fname);
      return 1;
   case FT_ISARCH:
      Jmsg(jcr, M_SKIPPED, 1, _("     Archive file skipped: %s\n"), ff_pkt->fname);
      return 1;
   case FT_NORECURSE:
      Jmsg(jcr, M_SKIPPED, 1, _("     Recursion turned off. Directory skipped: %s\n"), ff_pkt->fname);
      return 1;
   case FT_NOFSCHG:
      Jmsg(jcr, M_SKIPPED, 1, _("     File system change prohibited. Directory skipped: %s\n"), ff_pkt->fname);
      return 1;
   case FT_NOOPEN: {
      berrno be;
      be.set_errno(ff_pkt->ff_errno);
      Jmsg(jcr, M_NOTSAVED, 1, _("     Could not open directory %s: ERR=%s\n"), ff_pkt->fname, be.strerror());
      jcr->Errors++;
      return 1;
   }
   default:
      Jmsg(jcr, M_NOTSAVED, 0, _("     Unknown file type %d: %s\n"), ff_pkt->type, ff_pkt->fname);
      jcr->Errors++;
      return 1;
   }

   /* Encode attributes and possibly extend them */
   encode_stat(attribs, ff_pkt, 0);
   encode_attribsEx(jcr, attribsEx, ff_pkt);

   jcr->lock();
   jcr->JobFiles++;                  /* increment number of files sent */
   pm_strcpy(jcr->last_fname, ff_pkt->fname);
   jcr->unlock();

   /*
    * Send file attributes to Director
    *   File_index
    *   Stream
    *   Verify Options
    *   Filename (full path)
    *   Encoded attributes
    *   Link name (if type==FT_LNK)
    * For a directory, link is the same as fname, but with trailing
    * slash. For a linked file, link is the link.
    */
   /* Send file attributes to Director (note different format than for Storage) */
   Dmsg2(400, "send ATTR inx=%d fname=%s\n", jcr->JobFiles, ff_pkt->fname);
   if (ff_pkt->type == FT_LNK || ff_pkt->type == FT_LNKSAVED) {
      stat = bnet_fsend(dir, "%d %d %s %s%c%s%c%s%c", jcr->JobFiles,
            STREAM_UNIX_ATTRIBUTES, ff_pkt->VerifyOpts, ff_pkt->fname,
            0, attribs, 0, ff_pkt->link, 0);
   } else if (ff_pkt->type == FT_DIREND) {
         /* Here link is the canonical filename (i.e. with trailing slash) */
         stat = bnet_fsend(dir,"%d %d %s %s%c%s%c%c", jcr->JobFiles,
               STREAM_UNIX_ATTRIBUTES, ff_pkt->VerifyOpts, ff_pkt->link,
               0, attribs, 0, 0);
   } else {
      stat = bnet_fsend(dir,"%d %d %s %s%c%s%c%c", jcr->JobFiles,
            STREAM_UNIX_ATTRIBUTES, ff_pkt->VerifyOpts, ff_pkt->fname,
            0, attribs, 0, 0);
   }
   Dmsg2(20, "bfiled>bdird: attribs len=%d: msg=%s\n", dir->msglen, dir->msg);
   if (!stat) {
      Jmsg(jcr, M_FATAL, 0, _("Network error in send to Director: ERR=%s\n"), bnet_strerror(dir));
      return 0;
   }

   /*
    * The remainder of the function is all about getting the checksum.
    * First we initialise, then we read files, other streams and Finder Info.
    */
   if (ff_pkt->type != FT_LNKSAVED && (S_ISREG(ff_pkt->statp.st_mode) &&
            ff_pkt->flags & (FO_MD5|FO_SHA1))) {
      chksum_init(&chksum, ff_pkt->flags);
      binit(&bfd);

      if (ff_pkt->statp.st_size > 0 || ff_pkt->type == FT_RAW
            || ff_pkt->type == FT_FIFO) {
         if ((bopen(&bfd, ff_pkt->fname, O_RDONLY | O_BINARY, 0)) < 0) {
            ff_pkt->ff_errno = errno;
            berrno be;
            be.set_errno(bfd.berrno);
            Jmsg(jcr, M_NOTSAVED, 1, _("     Cannot open %s: ERR=%s.\n"),
                 ff_pkt->fname, be.strerror());
            jcr->Errors++;
            return 1;
         }
         read_chksum(&bfd, &chksum, jcr);
         bclose(&bfd);
      }

#ifdef HAVE_DARWIN_OS
      /* Open resource fork if necessary */
      if (ff_pkt->flags & FO_HFSPLUS && ff_pkt->hfsinfo.rsrclength > 0) {
         if (bopen_rsrc(&bfd, ff_pkt->fname, O_RDONLY | O_BINARY, 0) < 0) {
            ff_pkt->ff_errno = errno;
            berrno be;
            Jmsg(jcr, M_NOTSAVED, -1, _("     Cannot open resource fork for %s: ERR=%s.\n"),
                  ff_pkt->fname, be.strerror());
            jcr->Errors++;
            if (is_bopen(&ff_pkt->bfd)) {
               bclose(&ff_pkt->bfd);
            }
            return 1;
         }
         read_chksum(&bfd, &chksum, jcr);
         bclose(&bfd);
      }
      if (ff_pkt->flags & FO_HFSPLUS) {
         chksum_update(&chksum, ((unsigned char *)ff_pkt->hfsinfo.fndrinfo), 32);
      }
#endif

      /* compute MD5 or SHA1 hash */
      if (chksum.updated) {
         char chksumbuf[40];                  /* 24 should do */
         int stream = 0;

         chksum_final(&chksum);
         if (chksum.type == CHKSUM_MD5) {
            stream = STREAM_MD5_SIGNATURE;
         } else if (chksum.type == CHKSUM_SHA1) {
            stream = STREAM_SHA1_SIGNATURE;
         }
         bin_to_base64(chksumbuf, (char *)chksum.signature, chksum.length);
         Dmsg3(400, "send inx=%d %s=%s\n", jcr->JobFiles, chksum.name, chksumbuf);
         bnet_fsend(dir, "%d %d %s *%s-%d*", jcr->JobFiles, stream, chksumbuf,
               chksum.name, jcr->JobFiles);
         Dmsg3(20, "bfiled>bdird: %s len=%d: msg=%s\n", chksum.name,
               dir->msglen, dir->msg);
      }
   }

   return 1;
}

/*
 * Read checksum of bfd, updating chksum
 * In case of errors we need the job control record and file name.
 */
int read_chksum(BFILE *bfd, CHKSUM *chksum, JCR *jcr)
{
   int64_t n;

   while ((n=bread(bfd, jcr->big_buf, jcr->buf_size)) > 0) {
      chksum_update(chksum, ((unsigned char *)jcr->big_buf), (int)n);
      jcr->JobBytes += n;
      jcr->ReadBytes += n;
   }
   if (n < 0) {
      berrno be;
      be.set_errno(bfd->berrno);
      Jmsg(jcr, M_ERROR, 1, _("Error reading file %s: ERR=%s\n"),
            jcr->last_fname, be.strerror());
      jcr->Errors++;
      return -1;
   }
   return 0;
}