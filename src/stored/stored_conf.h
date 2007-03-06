/*
 * Resource codes -- they must be sequential for indexing
 *
 *   Version $Id: stored_conf.h,v 1.36.2.2 2006/03/14 21:41:45 kerns Exp $
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

enum {
   R_DIRECTOR = 3001,
   R_STORAGE,
   R_DEVICE,
   R_MSGS,
   R_AUTOCHANGER,
   R_FIRST = R_DIRECTOR,
   R_LAST  = R_AUTOCHANGER            /* keep this updated */
};

enum {
   R_NAME = 3020,
   R_ADDRESS,
   R_PASSWORD,
   R_TYPE,
   R_BACKUP
};


/* Definition of the contents of each Resource */
class DIRRES {
public:
   RES   hdr;

   char *password;                    /* Director password */
   char *address;                     /* Director IP address or zero */
   int monitor;                       /* Have only access to status and .status functions */
   int tls_enable;                    /* Enable TLS */
   int tls_require;                   /* Require TLS */
   int tls_verify_peer;              /* TLS Verify Client Certificate */
   char *tls_ca_certfile;             /* TLS CA Certificate File */
   char *tls_ca_certdir;              /* TLS CA Certificate Directory */
   char *tls_certfile;                /* TLS Server Certificate File */
   char *tls_keyfile;                 /* TLS Server Key File */
   char *tls_dhfile;                  /* TLS Diffie-Hellman Parameters */
   alist *tls_allowed_cns;            /* TLS Allowed Clients */

   TLS_CONTEXT *tls_ctx;              /* Shared TLS Context */
};


/* Storage daemon "global" definitions */
class s_res_store {
public:
   RES   hdr;

   dlist *sdaddrs;
   dlist *sddaddrs;
   char *working_directory;           /* working directory for checkpoints */
   char *pid_directory;
   char *subsys_directory;
   char *scripts_directory;
   uint32_t max_concurrent_jobs;      /* maximum concurrent jobs to run */
   MSGS *messages;                    /* Daemon message handler */
   utime_t heartbeat_interval;        /* Interval to send hb to FD */
   int tls_enable;                    /* Enable TLS */
   int tls_require;                   /* Require TLS */
   int tls_verify_peer;               /* TLS Verify Client Certificate */
   char *tls_ca_certfile;             /* TLS CA Certificate File */
   char *tls_ca_certdir;              /* TLS CA Certificate Directory */
   char *tls_certfile;                /* TLS Server Certificate File */
   char *tls_keyfile;                 /* TLS Server Key File */
   char *tls_dhfile;                  /* TLS Diffie-Hellman Parameters */
   alist *tls_allowed_cns;            /* TLS Allowed Clients */

   TLS_CONTEXT *tls_ctx;              /* Shared TLS Context */
};
typedef struct s_res_store STORES;

class AUTOCHANGER {
public:
   RES hdr;
   alist *device;                     /* List of DEVRES device pointers */
   char *changer_name;                /* Changer device name */
   char *changer_command;             /* Changer command  -- external program */
   pthread_mutex_t changer_mutex;     /* One changer operation at a time */
};

/* Device specific definitions */
class DEVRES {
public:
   RES   hdr;

   char *media_type;                  /* User assigned media type */
   char *device_name;                 /* Archive device name */
   char *changer_name;                /* Changer device name */
   char *changer_command;             /* Changer command  -- external program */
   char *alert_command;               /* Alert command -- external program */
   char *spool_directory;             /* Spool file directory */
   int   dev_type;                    /* device type */
   int   label_type;                  /* label type */
   int   autoselect;                  /* Automatically select from AutoChanger */
   uint32_t drive_index;              /* Autochanger drive index */
   uint32_t cap_bits;                 /* Capabilities of this device */
   utime_t max_changer_wait;          /* Changer timeout */
   utime_t max_rewind_wait;           /* maximum secs to wait for rewind */
   utime_t max_open_wait;             /* maximum secs to wait for open */
   uint32_t max_open_vols;            /* maximum simultaneous open volumes */
   uint32_t min_block_size;           /* min block size */
   uint32_t max_block_size;           /* max block size */
   uint32_t max_volume_jobs;          /* max jobs to put on one volume */
   uint32_t max_network_buffer_size;  /* max network buf size */
   utime_t  vol_poll_interval;        /* interval between polling volume during mount */
   int64_t max_volume_files;          /* max files to put on one volume */
   int64_t max_volume_size;           /* max bytes to put on one volume */
   int64_t max_file_size;             /* max file size in bytes */
   int64_t volume_capacity;           /* advisory capacity */
   int64_t max_spool_size;            /* Max spool size for all jobs */
   int64_t max_job_spool_size;        /* Max spool size for any single job */
   
   int64_t max_part_size;             /* Max part size */
   char *mount_point;                 /* Mount point for require mount devices */
   char *mount_command;               /* Mount command */
   char *unmount_command;             /* Unmount command */
   char *write_part_command;          /* Write part command */
   char *free_space_command;          /* Free space command */
   
   /* The following are set at runtime */
   DEVICE *dev;                       /* Pointer to phyical dev -- set at runtime */
   AUTOCHANGER *changer_res;          /* pointer to changer res if any */
};


union URES {
   DIRRES      res_dir;
   STORES      res_store;
   DEVRES      res_dev;
   MSGS        res_msgs;
   AUTOCHANGER res_changer;
   RES         hdr;
};