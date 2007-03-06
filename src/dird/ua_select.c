/*
 *
 *   Bacula Director -- User Agent Prompt and Selection code
 *
 *     Kern Sibbald, October MMI
 *
 *   Version  $Id: ua_select.c,v 1.65.2.4 2006/03/04 11:10:18 kerns Exp $
 */
/*
   Copyright (C) 2001-2006 Kern Sibbald

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

/* Imported variables */
extern struct s_jl joblevels[];


/*
 * Confirm a retention period
 */
int confirm_retention(UAContext *ua, utime_t *ret, const char *msg)
{
   char ed1[100];

   for ( ;; ) {
       bsendmsg(ua, _("The current %s retention period is: %s\n"),
          msg, edit_utime(*ret, ed1, sizeof(ed1)));
       if (!get_cmd(ua, _("Continue? (yes/mod/no): "))) {
          return 0;
       }
       if (strcasecmp(ua->cmd, _("mod")) == 0) {
          if (!get_cmd(ua, _("Enter new retention period: "))) {
             return 0;
          }
          if (!duration_to_utime(ua->cmd, ret)) {
             bsendmsg(ua, _("Invalid period.\n"));
             continue;
          }
          continue;
       }
       if (strcasecmp(ua->cmd, _("yes")) == 0) {
          return 1;
       }
       if (strcasecmp(ua->cmd, _("no")) == 0) {
          return 0;
       }
    }
    return 1;
}

/*
 * Given a list of keywords, find the first one
 *  that is in the argument list.
 * Returns: -1 if not found
 *          index into list (base 0) on success
 */
int find_arg_keyword(UAContext *ua, const char **list)
{
   for (int i=1; i<ua->argc; i++) {
      for(int j=0; list[j]; j++) {
         if (strcasecmp(_(list[j]), ua->argk[i]) == 0) {
            return j;
         }
      }
   }
   return -1;
}

/*
 * Given one keyword, find the first one that
 *   is in the argument list.
 * Returns: argk index (always gt 0)
 *          -1 if not found
 */
int find_arg(UAContext *ua, const char *keyword)
{
   for (int i=1; i<ua->argc; i++) {
      if (strcasecmp(keyword, ua->argk[i]) == 0) {
         return i;
      }
   }
   return -1;
}

/*
 * Given a single keyword, find it in the argument list, but
 *   it must have a value
 * Returns: -1 if not found or no value
 *           list index (base 0) on success
 */
int find_arg_with_value(UAContext *ua, const char *keyword)
{
   for (int i=1; i<ua->argc; i++) {
      if (strcasecmp(keyword, ua->argk[i]) == 0) {
         if (ua->argv[i]) {
            return i;
         } else {
            return -1;
         }
      }
   }
   return -1;
}

/*
 * Given a list of keywords, prompt the user
 * to choose one.
 *
 * Returns: -1 on failure
 *          index into list (base 0) on success
 */
int do_keyword_prompt(UAContext *ua, const char *msg, const char **list)
{
   int i;
   start_prompt(ua, _("You have the following choices:\n"));
   for (i=0; list[i]; i++) {
      add_prompt(ua, list[i]);
   }
   return do_prompt(ua, "", msg, NULL, 0);
}


/*
 * Select a Storage resource from prompt list
 */
STORE *select_storage_resource(UAContext *ua)
{
   char name[MAX_NAME_LENGTH];
   STORE *store;

   start_prompt(ua, _("The defined Storage resources are:\n"));
   LockRes();
   foreach_res(store, R_STORAGE) {
      if (acl_access_ok(ua, Storage_ACL, store->hdr.name)) {
         add_prompt(ua, store->hdr.name);
      }
   }
   UnlockRes();
   if (do_prompt(ua, _("Storage"),  _("Select Storage resource"), name, sizeof(name)) < 0) {
      return NULL;
   }
   store = (STORE *)GetResWithName(R_STORAGE, name);
   return store;
}

/*
 * Select a FileSet resource from prompt list
 */
FILESET *select_fileset_resource(UAContext *ua)
{
   char name[MAX_NAME_LENGTH];
   FILESET *fs;

   start_prompt(ua, _("The defined FileSet resources are:\n"));
   LockRes();
   foreach_res(fs, R_FILESET) {
      if (acl_access_ok(ua, FileSet_ACL, fs->hdr.name)) {
         add_prompt(ua, fs->hdr.name);
      }
   }
   UnlockRes();
   if (do_prompt(ua, _("FileSet"), _("Select FileSet resource"), name, sizeof(name)) < 0) {
      return NULL;
   }
   fs = (FILESET *)GetResWithName(R_FILESET, name);
   return fs;
}


/*
 * Get a catalog resource from prompt list
 */
CAT *get_catalog_resource(UAContext *ua)
{
   char name[MAX_NAME_LENGTH];
   CAT *catalog = NULL;
   int i;

   for (i=1; i<ua->argc; i++) {
      if (strcasecmp(ua->argk[i], _("catalog")) == 0 && ua->argv[i]) {
         if (acl_access_ok(ua, Catalog_ACL, ua->argv[i])) {
            catalog = (CAT *)GetResWithName(R_CATALOG, ua->argv[i]);
            break;
         }
      }
   }
   if (!catalog) {
      start_prompt(ua, _("The defined Catalog resources are:\n"));
      LockRes();
      foreach_res(catalog, R_CATALOG) {
         if (acl_access_ok(ua, Catalog_ACL, catalog->hdr.name)) {
            add_prompt(ua, catalog->hdr.name);
         }
      }
      UnlockRes();
      if (do_prompt(ua, _("Catalog"),  _("Select Catalog resource"), name, sizeof(name)) < 0) {
         return NULL;
      }
      catalog = (CAT *)GetResWithName(R_CATALOG, name);
   }
   return catalog;
}


/*
 * Select a Job resource from prompt list
 */
JOB *select_job_resource(UAContext *ua)
{
   char name[MAX_NAME_LENGTH];
   JOB *job;

   start_prompt(ua, _("The defined Job resources are:\n"));
   LockRes();
   foreach_res(job, R_JOB) {
      if (acl_access_ok(ua, Job_ACL, job->hdr.name)) {
         add_prompt(ua, job->hdr.name);
      }
   }
   UnlockRes();
   if (do_prompt(ua, _("Job"), _("Select Job resource"), name, sizeof(name)) < 0) {
      return NULL;
   }
   job = (JOB *)GetResWithName(R_JOB, name);
   return job;
}

/*
 * Select a Restore Job resource from prompt list
 */
JOB *select_restore_job_resource(UAContext *ua)
{
   char name[MAX_NAME_LENGTH];
   JOB *job;

   start_prompt(ua, _("The defined Restore Job resources are:\n"));
   LockRes();
   foreach_res(job, R_JOB) {
      if (job->JobType == JT_RESTORE && acl_access_ok(ua, Job_ACL, job->hdr.name)) {
         add_prompt(ua, job->hdr.name);
      }
   }
   UnlockRes();
   if (do_prompt(ua, _("Job"), _("Select Restore Job"), name, sizeof(name)) < 0) {
      return NULL;
   }
   job = (JOB *)GetResWithName(R_JOB, name);
   return job;
}



/*
 * Select a client resource from prompt list
 */
CLIENT *select_client_resource(UAContext *ua)
{
   char name[MAX_NAME_LENGTH];
   CLIENT *client;

   start_prompt(ua, _("The defined Client resources are:\n"));
   LockRes();
   foreach_res(client, R_CLIENT) {
      if (acl_access_ok(ua, Client_ACL, client->hdr.name)) {
         add_prompt(ua, client->hdr.name);
      }
   }
   UnlockRes();
   if (do_prompt(ua, _("Client"),  _("Select Client (File daemon) resource"), name, sizeof(name)) < 0) {
      return NULL;
   }
   client = (CLIENT *)GetResWithName(R_CLIENT, name);
   return client;
}

/*
 *  Get client resource, start by looking for
 *   client=<client-name>
 *  if we don't find the keyword, we prompt the user.
 */
CLIENT *get_client_resource(UAContext *ua)
{
   CLIENT *client = NULL;
   int i;

   for (i=1; i<ua->argc; i++) {
      if ((strcasecmp(ua->argk[i], N_("client")) == 0 ||
           strcasecmp(ua->argk[i], N_("fd")) == 0) && ua->argv[i]) {
         if (!acl_access_ok(ua, Client_ACL, ua->argv[i])) {
            break;
         }
         client = (CLIENT *)GetResWithName(R_CLIENT, ua->argv[i]);
         if (client) {
            return client;
         }
         bsendmsg(ua, _("Error: Client resource %s does not exist.\n"), ua->argv[i]);
         break;
      }
   }
   return select_client_resource(ua);
}

/* Scan what the user has entered looking for:
 *
 *  client=<client-name>
 *
 *  if error or not found, put up a list of client DBRs
 *  to choose from.
 *
 *   returns: 0 on error
 *            1 on success and fills in CLIENT_DBR
 */
int get_client_dbr(UAContext *ua, CLIENT_DBR *cr)
{
   int i;

   if (cr->Name[0]) {                 /* If name already supplied */
      if (db_get_client_record(ua->jcr, ua->db, cr)) {
         return 1;
      }
      bsendmsg(ua, _("Could not find Client %s: ERR=%s"), cr->Name, db_strerror(ua->db));
   }
   for (i=1; i<ua->argc; i++) {
      if ((strcasecmp(ua->argk[i], _("client")) == 0 ||
           strcasecmp(ua->argk[i], _("fd")) == 0) && ua->argv[i]) {
         if (!acl_access_ok(ua, Client_ACL, ua->argv[i])) {
            break;
         }
         bstrncpy(cr->Name, ua->argv[i], sizeof(cr->Name));
         if (!db_get_client_record(ua->jcr, ua->db, cr)) {
            bsendmsg(ua, _("Could not find Client \"%s\": ERR=%s"), ua->argv[i],
                     db_strerror(ua->db));
            cr->ClientId = 0;
            break;
         }
         return 1;
      }
   }
   if (!select_client_dbr(ua, cr)) {  /* try once more by proposing a list */
      return 0;
   }
   return 1;
}

/*
 * Select a Client record from the catalog
 *  Returns 1 on success
 *          0 on failure
 */
int select_client_dbr(UAContext *ua, CLIENT_DBR *cr)
{
   CLIENT_DBR ocr;
   char name[MAX_NAME_LENGTH];
   int num_clients, i;
   uint32_t *ids;


   cr->ClientId = 0;
   if (!db_get_client_ids(ua->jcr, ua->db, &num_clients, &ids)) {
      bsendmsg(ua, _("Error obtaining client ids. ERR=%s\n"), db_strerror(ua->db));
      return 0;
   }
   if (num_clients <= 0) {
      bsendmsg(ua, _("No clients defined. You must run a job before using this command.\n"));
      return 0;
   }

   start_prompt(ua, _("Defined Clients:\n"));
   for (i=0; i < num_clients; i++) {
      ocr.ClientId = ids[i];
      if (!db_get_client_record(ua->jcr, ua->db, &ocr) ||
          !acl_access_ok(ua, Client_ACL, ocr.Name)) {
         continue;
      }
      add_prompt(ua, ocr.Name);
   }
   free(ids);
   if (do_prompt(ua, _("Client"),  _("Select the Client"), name, sizeof(name)) < 0) {
      return 0;
   }
   memset(&ocr, 0, sizeof(ocr));
   bstrncpy(ocr.Name, name, sizeof(ocr.Name));

   if (!db_get_client_record(ua->jcr, ua->db, &ocr)) {
      bsendmsg(ua, _("Could not find Client \"%s\": ERR=%s"), name, db_strerror(ua->db));
      return 0;
   }
   memcpy(cr, &ocr, sizeof(ocr));
   return 1;
}



/* Scan what the user has entered looking for:
 *
 *  pool=<pool-name>
 *
 *  if error or not found, put up a list of pool DBRs
 *  to choose from.
 *
 *   returns: false on error
 *            true  on success and fills in POOL_DBR
 */
bool get_pool_dbr(UAContext *ua, POOL_DBR *pr)
{
   if (pr->Name[0]) {                 /* If name already supplied */
      if (db_get_pool_record(ua->jcr, ua->db, pr) &&
          acl_access_ok(ua, Pool_ACL, pr->Name)) {
         return true;
      }
      bsendmsg(ua, _("Could not find Pool \"%s\": ERR=%s"), pr->Name, db_strerror(ua->db));
   }
   if (!select_pool_dbr(ua, pr)) {  /* try once more */
      return false;
   }
   return true;
}

/*
 * Select a Pool record from the catalog
 */
bool select_pool_dbr(UAContext *ua, POOL_DBR *pr)
{
   POOL_DBR opr;
   char name[MAX_NAME_LENGTH];
   int num_pools, i;
   uint32_t *ids;

   for (i=1; i<ua->argc; i++) {
      if (strcasecmp(ua->argk[i], N_("pool")) == 0 && ua->argv[i] &&
          acl_access_ok(ua, Pool_ACL, ua->argv[i])) {
         bstrncpy(pr->Name, ua->argv[i], sizeof(pr->Name));
         if (!db_get_pool_record(ua->jcr, ua->db, pr)) {
            bsendmsg(ua, _("Could not find Pool \"%s\": ERR=%s"), ua->argv[i],
                     db_strerror(ua->db));
            pr->PoolId = 0;
            break;
         }
         return true;
      }
   }

   pr->PoolId = 0;
   if (!db_get_pool_ids(ua->jcr, ua->db, &num_pools, &ids)) {
      bsendmsg(ua, _("Error obtaining pool ids. ERR=%s\n"), db_strerror(ua->db));
      return 0;
   }
   if (num_pools <= 0) {
      bsendmsg(ua, _("No pools defined. Use the \"create\" command to create one.\n"));
      return false;
   }

   start_prompt(ua, _("Defined Pools:\n"));
   for (i=0; i < num_pools; i++) {
      opr.PoolId = ids[i];
      if (!db_get_pool_record(ua->jcr, ua->db, &opr) ||
          !acl_access_ok(ua, Pool_ACL, opr.Name)) {
         continue;
      }
      add_prompt(ua, opr.Name);
   }
   free(ids);
   if (do_prompt(ua, _("Pool"),  _("Select the Pool"), name, sizeof(name)) < 0) {
      return false;
   }
   memset(&opr, 0, sizeof(opr));
   bstrncpy(opr.Name, name, sizeof(opr.Name));

   if (!db_get_pool_record(ua->jcr, ua->db, &opr)) {
      bsendmsg(ua, _("Could not find Pool \"%s\": ERR=%s"), name, db_strerror(ua->db));
      return false;
   }
   memcpy(pr, &opr, sizeof(opr));
   return true;
}

/*
 * Select a Pool and a Media (Volume) record from the database
 */
int select_pool_and_media_dbr(UAContext *ua, POOL_DBR *pr, MEDIA_DBR *mr)
{

   if (!select_media_dbr(ua, mr)) {
      return 0;
   }
   memset(pr, 0, sizeof(POOL_DBR));
   pr->PoolId = mr->PoolId;
   if (!db_get_pool_record(ua->jcr, ua->db, pr)) {
      bsendmsg(ua, "%s", db_strerror(ua->db));
      return 0;
   }
   if (!acl_access_ok(ua, Pool_ACL, pr->Name)) {
      bsendmsg(ua, _("No access to Pool \"%s\"\n"), pr->Name);
      return 0;
   }
   return 1;
}

/* Select a Media (Volume) record from the database */
int select_media_dbr(UAContext *ua, MEDIA_DBR *mr)
{
   int i;

   memset(mr, 0, sizeof(MEDIA_DBR));

   i = find_arg_with_value(ua, "volume");
   if (i >= 0) {
      bstrncpy(mr->VolumeName, ua->argv[i], sizeof(mr->VolumeName));
   }
   if (mr->VolumeName[0] == 0) {
      POOL_DBR pr;
      memset(&pr, 0, sizeof(pr));
      /* Get the pool from pool=<pool-name> */
      if (!get_pool_dbr(ua, &pr)) {
         return 0;
      }
      mr->PoolId = pr.PoolId;
      db_list_media_records(ua->jcr, ua->db, mr, prtit, ua, HORZ_LIST);
      if (!get_cmd(ua, _("Enter MediaId or Volume name: "))) {
         return 0;
      }
      if (is_a_number(ua->cmd)) {
         mr->MediaId = str_to_int64(ua->cmd);
      } else {
         bstrncpy(mr->VolumeName, ua->cmd, sizeof(mr->VolumeName));
      }
   }

   if (!db_get_media_record(ua->jcr, ua->db, mr)) {
      bsendmsg(ua, "%s", db_strerror(ua->db));
      return 0;
   }
   return 1;
}


/*
 * Select a pool resource from prompt list
 */
POOL *select_pool_resource(UAContext *ua)
{
   char name[MAX_NAME_LENGTH];
   POOL *pool;

   start_prompt(ua, _("The defined Pool resources are:\n"));
   LockRes();
   foreach_res(pool, R_POOL) {
      if (acl_access_ok(ua, Pool_ACL, pool->hdr.name)) {
         add_prompt(ua, pool->hdr.name);
      }
   }
   UnlockRes();
   if (do_prompt(ua, _("Pool"), _("Select Pool resource"), name, sizeof(name)) < 0) {
      return NULL;
   }
   pool = (POOL *)GetResWithName(R_POOL, name);
   return pool;
}


/*
 *  If you are thinking about using it, you
 *  probably want to use select_pool_dbr()
 *  or get_pool_dbr() above.
 */
POOL *get_pool_resource(UAContext *ua)
{
   POOL *pool = NULL;
   int i;

   i = find_arg_with_value(ua, "pool");
   if (i >= 0 && acl_access_ok(ua, Pool_ACL, ua->argv[i])) {
      pool = (POOL *)GetResWithName(R_POOL, ua->argv[i]);
      if (pool) {
         return pool;
      }
      bsendmsg(ua, _("Error: Pool resource \"%s\" does not exist.\n"), ua->argv[i]);
   }
   return select_pool_resource(ua);
}

/*
 * List all jobs and ask user to select one
 */
int select_job_dbr(UAContext *ua, JOB_DBR *jr)
{
   db_list_job_records(ua->jcr, ua->db, jr, prtit, ua, HORZ_LIST);
   if (!get_pint(ua, _("Enter the JobId to select: "))) {
      return 0;
   }
   jr->JobId = ua->int64_val;
   if (!db_get_job_record(ua->jcr, ua->db, jr)) {
      bsendmsg(ua, "%s", db_strerror(ua->db));
      return 0;
   }
   return jr->JobId;

}


/* Scan what the user has entered looking for:
 *
 *  jobid=nn
 *
 *  if error or not found, put up a list of Jobs
 *  to choose from.
 *
 *   returns: 0 on error
 *            JobId on success and fills in JOB_DBR
 */
int get_job_dbr(UAContext *ua, JOB_DBR *jr)
{
   int i;

   for (i=1; i<ua->argc; i++) {
      if (strcasecmp(ua->argk[i], N_("ujobid")) == 0 && ua->argv[i]) {
         jr->JobId = 0;
         bstrncpy(jr->Job, ua->argv[i], sizeof(jr->Job));
      } else if (strcasecmp(ua->argk[i], N_("jobid")) == 0 && ua->argv[i]) {
         jr->JobId = str_to_int64(ua->argv[i]);
         jr->Job[0] = 0;
      } else {
         continue;
      }
      if (!db_get_job_record(ua->jcr, ua->db, jr)) {
         bsendmsg(ua, _("Could not find Job \"%s\": ERR=%s"), ua->argv[i],
                  db_strerror(ua->db));
         jr->JobId = 0;
         break;
      }
      return jr->JobId;
   }

   jr->JobId = 0;
   jr->Job[0] = 0;

   for (i=1; i<ua->argc; i++) {
      if ((strcasecmp(ua->argk[i], N_("jobname")) == 0 ||
           strcasecmp(ua->argk[i], N_("job")) == 0) && ua->argv[i]) {
         jr->JobId = 0;
         bstrncpy(jr->Name, ua->argv[i], sizeof(jr->Name));
         break;
      }
   }
   if (!select_job_dbr(ua, jr)) {  /* try once more */
      return 0;
   }
   return jr->JobId;
}

/*
 * Implement unique set of prompts
 */
void start_prompt(UAContext *ua, const char *msg)
{
  if (ua->max_prompts == 0) {
     ua->max_prompts = 10;
     ua->prompt = (char **)bmalloc(sizeof(char *) * ua->max_prompts);
  }
  ua->num_prompts = 1;
  ua->prompt[0] = bstrdup(msg);
}

/*
 * Add to prompts -- keeping them unique
 */
void add_prompt(UAContext *ua, const char *prompt)
{
   int i;
   if (ua->num_prompts == ua->max_prompts) {
      ua->max_prompts *= 2;
      ua->prompt = (char **)brealloc(ua->prompt, sizeof(char *) *
         ua->max_prompts);
    }
    for (i=1; i < ua->num_prompts; i++) {
       if (strcmp(ua->prompt[i], prompt) == 0) {
          return;
       }
    }
    ua->prompt[ua->num_prompts++] = bstrdup(prompt);
}

/*
 * Display prompts and get user's choice
 *
 *  Returns: -1 on error
 *            index base 0 on success, and choice
 *               is copied to prompt if not NULL
 *             prompt is set to the chosen prompt item string
 */
int do_prompt(UAContext *ua, const char *automsg, const char *msg, char *prompt, int max_prompt)
{
   int i, item;
   char pmsg[MAXSTRING];

   if (prompt) {
      *prompt = 0;
   }
   if (ua->num_prompts == 2) {
      item = 1;
      if (prompt) {
         bstrncpy(prompt, ua->prompt[1], max_prompt);
      }
      bsendmsg(ua, _("Automatically selected %s: %s\n"), automsg, ua->prompt[1]);
      goto done;
   }
   /* If running non-interactive, bail out */
   if (ua->batch) {
      bsendmsg(ua, _("Cannot select %s in batch mode.\n"), automsg);
      item = -1;
      goto done;
   }
// bnet_sig(ua->UA_sock, BNET_START_SELECT);
   bsendmsg(ua, ua->prompt[0]);
   for (i=1; i < ua->num_prompts; i++) {
      bsendmsg(ua, "%6d: %s\n", i, ua->prompt[i]);
   }
// bnet_sig(ua->UA_sock, BNET_END_SELECT);

   for ( ;; ) {
      /* First item is the prompt string, not the items */
      if (ua->num_prompts == 1) {
         bsendmsg(ua, _("Selection is empty!\n"));
         item = -1;                    /* list is empty ! */
         break;
      }
      if (ua->num_prompts == 2) {
         item = 1;
         bsendmsg(ua, _("Item 1 selected automatically.\n"));
         if (prompt) {
            bstrncpy(prompt, ua->prompt[1], max_prompt);
         }
         break;
      } else {
         sprintf(pmsg, "%s (1-%d): ", msg, ua->num_prompts-1);
      }
      /* Either a . or an @ will get you out of the loop */
      if (!get_pint(ua, pmsg)) {
         item = -1;                   /* error */
         bsendmsg(ua, _("Selection aborted, nothing done.\n"));
         break;
      }
      item = ua->pint32_val;
      if (item < 1 || item >= ua->num_prompts) {
         bsendmsg(ua, _("Please enter a number between 1 and %d\n"), ua->num_prompts-1);
         continue;
      }
      if (prompt) {
         bstrncpy(prompt, ua->prompt[item], max_prompt);
      }
      break;
   }

done:
   for (i=0; i < ua->num_prompts; i++) {
      free(ua->prompt[i]);
   }
   ua->num_prompts = 0;
   return item>0 ? item-1 : item;
}


/*
 * We scan what the user has entered looking for
 *    storage=<storage-resource>
 *    job=<job_name>
 *    jobid=<jobid>
 *    ?              (prompt him with storage list)
 *    <some-error>   (prompt him with storage list)
 *
 * If use_default is set, we assume that any keyword without a value
 *   is the name of the Storage resource wanted.
 */
STORE *get_storage_resource(UAContext *ua, bool use_default)
{
   char *store_name = NULL;
   STORE *store = NULL;
   int jobid;
   JCR *jcr;
   int i;
   char ed1[50];

   for (i=1; i<ua->argc; i++) {
      if (use_default && !ua->argv[i]) {
         /* Ignore slots, scan and barcode(s) keywords */
         if (strcasecmp("scan", ua->argk[i]) == 0 ||
             strcasecmp("barcode", ua->argk[i]) == 0 ||
             strcasecmp("barcodes", ua->argk[i]) == 0 ||
             strcasecmp("slots", ua->argk[i]) == 0) {
            continue;
         }
         /* Default argument is storage */
         if (store_name) {
            bsendmsg(ua, _("Storage name given twice.\n"));
            return NULL;
         }
         store_name = ua->argk[i];
         if (*store_name == '?') {
            *store_name = 0;
            break;
         }
      } else {
         if (strcasecmp(ua->argk[i], N_("storage")) == 0 ||
             strcasecmp(ua->argk[i], N_("sd")) == 0) {
            store_name = ua->argv[i];
            break;

         } else if (strcasecmp(ua->argk[i], N_("jobid")) == 0) {
            jobid = str_to_int64(ua->argv[i]);
            if (jobid <= 0) {
               bsendmsg(ua, _("Expecting jobid=nn command, got: %s\n"), ua->argk[i]);
               return NULL;
            }
            if (!(jcr=get_jcr_by_id(jobid))) {
               bsendmsg(ua, _("JobId %s is not running.\n"), edit_int64(jobid, ed1));
               return NULL;
            }
            store = jcr->store;
            free_jcr(jcr);
            break;

         } else if (strcasecmp(ua->argk[i], N_("job")) == 0 ||
                    strcasecmp(ua->argk[i], N_("jobname")) == 0) {
            if (!ua->argv[i]) {
               bsendmsg(ua, _("Expecting job=xxx, got: %s.\n"), ua->argk[i]);
               return NULL;
            }
            if (!(jcr=get_jcr_by_partial_name(ua->argv[i]))) {
               bsendmsg(ua, _("Job \"%s\" is not running.\n"), ua->argv[i]);
               return NULL;
            }
            store = jcr->store;
            free_jcr(jcr);
            break;
         } else if (strcasecmp(ua->argk[i], N_("ujobid")) == 0) {
            if (!ua->argv[i]) {
               bsendmsg(ua, _("Expecting ujobid=xxx, got: %s.\n"), ua->argk[i]);
               return NULL;
            }
            if (!(jcr=get_jcr_by_full_name(ua->argv[i]))) {
               bsendmsg(ua, _("Job \"%s\" is not running.\n"), ua->argv[i]);
               return NULL;
            }
            store = jcr->store;
            free_jcr(jcr);
            break;
        }
      }
   }
   if (store && !acl_access_ok(ua, Storage_ACL, store->hdr.name)) {
      store = NULL;
   }

   if (!store && store_name && store_name[0] != 0) {
      store = (STORE *)GetResWithName(R_STORAGE, store_name);
      if (!store) {
         bsendmsg(ua, _("Storage resource \"%s\": not found\n"), store_name);
      }
   }
   if (store && !acl_access_ok(ua, Storage_ACL, store->hdr.name)) {
      store = NULL;
   }
   /* No keywords found, so present a selection list */
   if (!store) {
      store = select_storage_resource(ua);
   }
   return store;
}

/* Get drive that we are working with for this storage */
int get_storage_drive(UAContext *ua, STORE *store)
{
   int i, drive = -1;
   /* Get drive for autochanger if possible */
   i = find_arg_with_value(ua, "drive");
   if (i >=0) {
      drive = atoi(ua->argv[i]);
   } else if (store && store->autochanger) {
      /* If our structure is not set ask SD for # drives */
      if (store->drives == 0) {
         store->drives = get_num_drives_from_SD(ua);
      }
      /* If only one drive, default = 0 */
      if (store->drives == 1) {
         drive = 0;
      } else {
         /* Ask user to enter drive number */
         ua->cmd[0] = 0;
         if (!get_cmd(ua, _("Enter autochanger drive[0]: "))) {
            drive = -1;  /* None */
         } else {
            drive = atoi(ua->cmd);
         }
     }
   }
   return drive;
}


/*
 * Scan looking for mediatype=
 *
 *  if not found or error, put up selection list
 *
 *  Returns: 0 on error
 *           1 on success, MediaType is set
 */
int get_media_type(UAContext *ua, char *MediaType, int max_media)
{
   STORE *store;
   int i;

   i = find_arg_with_value(ua, "mediatype");
   if (i >= 0) {
      bstrncpy(MediaType, ua->argv[i], max_media);
      return 1;
   }

   start_prompt(ua, _("Media Types defined in conf file:\n"));
   LockRes();
   foreach_res(store, R_STORAGE) {
      add_prompt(ua, store->media_type);
   }
   UnlockRes();
   return (do_prompt(ua, _("Media Type"), _("Select the Media Type"), MediaType, max_media) < 0) ? 0 : 1;
}

bool get_level_from_name(JCR *jcr, const char *level_name)
{
   /* Look up level name and pull code */
   bool found = false;
   for (int i=0; joblevels[i].level_name; i++) {
      if (strcasecmp(level_name, joblevels[i].level_name) == 0) {
         jcr->JobLevel = joblevels[i].level;
         found = true;
         break;
      }
   }
   return found;
}