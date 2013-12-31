/* Return codes */
#define CODE_OK		0
#define EXCODE_ACTED	77	/* filtering happened, only used if --excode */
#define CODE_TEMPFAIL	75	/* temporary failure, as per EX_TEMPFAIL from sysexits.h */
#define CODE_FAILURE	255	/* critical failure */

/* States for yes/no options loaded from CONF_FILE */
#define OPTION_UNSET	0	/* this MUST be 0 */
#define OPTION_DISABLED	1
#define OPTION_ENABLED	2

/* FILTER MODES. Values MUST be nonzero. */
#define MODE_ALL	1
#define MODE_BAD	2
#define MODE_GOOD	3

/* FILTER ACTIONS. Values MUST be nonzero. */
#define ACTION_RENAME	1
#define ACTION_DELETE	2
#define ACTION_KILL	3
#define ACTION_SKIP	4

#define MAXLINEBUF	1000	/* As per RFC, mail line buffer size, including \n and \0 */

#define MAXFIELD	512
#define CONF_FILE	"renattach.conf"
#define LIST_TOKENS	"\t, \n"

#define TAG_OPTENABLED	"yes"
#define TAG_OPTDISABLED	"no"
#define TAG_ACTRENAME	"rename"
#define TAG_ACTDELETE	"delete"
#define TAG_ACTKILL	"kill"
#define TAG_ACTSKIP	"skip"

/*
	Executable signatures: 'MZ' in base64 and uuencoded form
	base64: compare to bytes 0-2 on line
	uuencode: compare to bytes 1-3 on line (skip byte 0, length code)
*/
#define B64_EXESIG1	"TVo"
#define B64_EXESIG2	"TVp"
#define B64_EXESIG3	"TVq"
#define B64_EXESIG4	"TVr"
#define UUE_EXESIG1	"35H"
#define UUE_EXESIG2	"35I"
#define UUE_EXESIG3	"35J"
#define UUE_EXESIG4	"35K"

#define COPYRIGHT	"Copyright (C) 2003-2006  Jem E. Berkes"

/*
	Configuration directives you would find in .conf file
	Defaults are given in the next set of defines
*/
#define TAG_DELETE_EXE		"delete_exe"		/* delete executable binary attachments, when detected */
#define TAG_KILL_EXE		"kill_exe"		/* (mutually exclusive) kill executable binary attachments */
#define TAG_SEARCH_ZIP		"search_zip"		/* search for file names inside ZIP archives */
#define TAG_PASS_CONTENTID	"pass_contentid"	/* pass MIME Content-ID through, unmodified */
#define TAG_FULL_RENAME		"full_rename"		/* replace all periods during rename if enabled (safer) */
#define TAG_USE_SYSLOG		"use_syslog"		/* use syslog feature */
#define TAG_GENERIC_NAME	"generic_name"		/* filename to use when parsing fails (corrupt, blank) */
#define TAG_NEW_EXTENSION	"new_extension"		/* use this new file extension when modifying */
#define TAG_NEW_MIME_TYPE	"new_mime_type"		/* use this MIME content-type when modifying */
#define TAG_SUBJ_BANNED		"subj_banned"		/* (highest precedence) subject text when file is banned */
#define TAG_SUBJ_EXEC		"subj_exec"		/* subject text when executable file is caught */
#define TAG_SUBJ_DELETED	"subj_deleted"		/* subject text when attachment is deleted */
#define TAG_SUBJ_RENAMED	"subj_renamed"		/* subject text when attachment is renamed */
#define TAG_ADD_SUBJECT		"add_subject"		/* (lowest precedence) subject text when filtering occurs */
#define TAG_HTMLWARN_POS	"htmlwarn_pos"		/* series of HTML tags defining position for warning_html */
#define TAG_WARNING_TEXT	"warning_text"		/* (additive) plain text warning to insert when filtered */
#define TAG_WARNING_HTML	"warning_html"		/* (additive) HTML markup warning to insert when filtered */
#define TAG_ADD_HEADER		"add_header"		/* (additive) new headers to add when filtering occurs */
#define TAG_BANNED_FILES	"banned_files"		/* (additive) specifically banned files, with actions */
#define TAG_GOODLIST		"goodlist"		/* (additive) list of safe/good file extensions */
#define TAG_BADLIST		"badlist"		/* (additive) list of dangerous/bad file extensions */


/*
	Default configuration directives
*/
#define DEF_DELETE_EXE		OPTION_ENABLED
#define DEF_KILL_EXE		OPTION_DISABLED
#define DEF_SEARCH_ZIP		OPTION_DISABLED
#define DEF_PASS_CONTENTID	OPTION_DISABLED
#define DEF_FULL_RENAME		OPTION_ENABLED
#define DEF_USE_SYSLOG		OPTION_DISABLED
#define DEF_GENERIC_NAME	"filename"
#define DEF_NEW_EXTENSION	"bad"
#define DEF_NEW_MIME_TYPE	"application/unknown"
#define DEF_SUBJ_BANNED		""
#define DEF_SUBJ_EXEC		""
#define DEF_SUBJ_DELETED	""
#define DEF_SUBJ_RENAMED	""
#define DEF_ADD_SUBJECT		"[filtered]"
#define DEF_HTMLWARN_POS	"html, body"
#define DEF_WARNING_TEXT	""
#define DEF_WARNING_HTML	""
#define DEF_ADD_HEADER		""
#define DEF_BANNED_FILES	""
#define DEF_GOODLIST		"DOC, PDF, RTF, SXC, SXW, TXT, ZIP"
#define DEF_BADLIST		"ADE, ADP, BAS, BAT, CHM, CMD, COM, CPL, CRT, EML, EXE, "\
				"HLP, HTA, HTM, HTML, INF, INS, ISP, JS, JSE, LNK, MDB, "\
				"MDE, MSC, MSH, MSI, MSP, MST, NWS, OCX, PCD, PIF, REG, "\
				"SCR, SCT, SHB, SHS, URL, VB, VBE, VBS, WSC, WSF, WSH"

#define MIME_CTYPE	"Content-Type:"
#define MIME_TYPETEXT	"Content-Type: text/plain"
#define MIME_TYPEHTML	"Content-Type: text/html"
#define MIME_CTENC	"Content-Transfer-Encoding:"
#define MIME_CDISP	"Content-Disposition:"
#define MIME_CNTID	"Content-ID:"
#define MIME_CDESC	"Content-Description:"
#define MIME_NAME	"name"
#define MIME_NAMEX	"name*"
#define UUENC_START	"begin "
#define UUENC_END	"end"

#define HEAD_FILTERED_WITH	"X-Filtered-With:"
#define HEAD_FILTERED_INFO	"X-RenAttach-Info:"
#define HEAD_SUBJECT		"Subject:"
#define HEAD_MESSAGEID		"Message-ID:"
#define HEAD_DELIVERED_TO	"Delivered-To:"

#define FMT_PLAIN		0
#define FMT_RFC2047		1
#define FMT_RFC2231		2


/*
	These details tell us what we need to know in order
	to rewrite a MIME header after seeing an attached file
*/
struct namespec
{
	char name[MAXFIELD];	/* decoded form */
	char oldname[MAXFIELD];	/* the name before we changed it */
	int specformat;		/* name specification format, FMT_ */
	int langinfo;		/* (if specformat==FMT_RFC2231), is there lang info? */
	char charenc[MAXFIELD];	/* (if specformat==FMT_RFC2047), character encoding */
	char charmode;		/* (if specformat==FMT_RFC2047), mode B or Q */
};


struct attach
{
	int fattach;			/* if nonzero this is a file attachment */
	int within;			/* if nonzero the file was found within (ZIP) */
	char content_type[MAXFIELD];	/* the content type found */
	char content_enc[MAXFIELD];	/* content transfer encoding */
	char content_id[MAXFIELD];	/* content ID field */
	struct namespec* curspec;	/* current namespec */
	struct namespec ct_fname;	/* content type namespec */
	struct namespec cd_fname;	/* content disposition namespec */
};


struct config_opts
{
	char* config_file;	/* Configuration file name */
	char** pipe_cmd;	/* command/args to pipe output to */
	int mode;		/* Filtering mode */
	int def_act;		/* Default action */
	int action;		/* Current action (may differ from def_act) */

	int delete_exe;
	int kill_exe;
	int search_zip;
	int pass_contentid;
	int full_rename;
	int use_syslog;
	char generic_name[MAXFIELD];
	char new_extension[MAXFIELD];
	char new_mime_type[MAXFIELD];
	char subj_banned[MAXFIELD];
	char subj_exec[MAXFIELD];
	char subj_deleted[MAXFIELD];
	char subj_renamed[MAXFIELD];
	char add_subject[MAXFIELD];
	char htmlwarn_pos[MAXFIELD];
	char* warning_text;
	char* warning_html;
	char* add_header;
	char* banned_files;
	char* goodlist;
	char* badlist;
};


const char* opt2str(int state);
const char* mode2str(int mode);
const char* act2str(int action);

int tokenize_list(const char* thelist, int verbose);
int parse_conf(struct config_opts* options);
void directive_defaults(struct config_opts* options);
void show_configuration(struct config_opts* options);
