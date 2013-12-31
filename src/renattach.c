/*
 renattach 1.2.4 - Filter that renames/deletes dangerous email attachments
 Copyright (C) 2003-2006  Jem E. Berkes <jberkes@pc-tools.net>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

	Warning: renattach 'breaks' MIME because it rewrites MIME headers!
	Whenever a MIME attachment with filename is encountered, MIME headers
	are rewritten to a safe format (even if filenames are unchanged).
	MIME headers that aren't attachments with filenames are left alone.
*/

#include "config.h"
#include "renattach.h"
#include "strings-en.h"
#include "utility.h"
/*
#include <ctype.h>
#include <getopt.h>
*/
#include "getopt.h"
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

#ifdef HAVE_SYSLOG_H
#include <syslog.h>
#endif

#ifdef __EMX__
#include <io.h>
#endif

#ifndef HAVE_STRCASECMP
#define strcasecmp stricmp
#endif

#ifndef HAVE_STRNCASECMP
#define strncasecmp strnicmp
#endif

/* Function prototypes */
int filter_pass(FILE*);
int header_pass(FILE*);
int is_line_mime(const char*, int, struct attach*);
int filter_decision(struct attach*);
int match_filename(const char*, int);
int decode_2047(char*, struct namespec*);
int reencode_name(struct namespec*, const char*, char*);
int position_html(FILE*, FILE*);
void kill_exit(const char*);
void taking_action(const char*);
void not_taking_action(const char*, const char*);
int explain_subject(FILE*, char*);
void abnormalproc(int);
FILE* openpipe(char**, pid_t*);
int closepipe(FILE*, pid_t, int*);


/* Global variables */
struct config_opts configuration;
FILE* tempfile = NULL;
int loopguard=0, verbose=0, justsettings=0, excode=CODE_OK;
int filtcount=0, ban_count=0, exe_count=0, del_count=0, ren_count=0;
char messageid[MAXFIELD];

static char shortopts[] = "abc:deghklp:rsvV";
static struct option long_options[] =
{
	{"all", no_argument, NULL, 'a'},
	{"badlist", no_argument, NULL, 'b'},
	{"config", required_argument, NULL, 'c'},
	{"delete", no_argument, NULL, 'd'},
	{"excode", no_argument, NULL, 'e'},
	{"goodlist", no_argument, NULL, 'g'},
	{"help", no_argument, NULL, 'h'},
	{"kill", no_argument, NULL, 'k'},
	{"loop", no_argument, NULL, 'l'},
	{"pipe", required_argument, NULL, 'p'},
	{"rename", no_argument, NULL, 'r'},
	{"settings", no_argument, NULL, 's'},
	{"verbose", no_argument, NULL, 'v'},
 	{"version", no_argument, NULL, 'V'},
	{0, 0, 0, 0}
};


int main(int argc, char** argv)
{
	int foundopt;
	memset(&configuration, 0, sizeof(struct config_opts));	/* set all options to 'unset' state */

	while ((foundopt = getopt_long(argc, argv, shortopts, long_options, NULL)) != -1)
	{
		switch (foundopt)
		{
			case 'a':
				if (configuration.mode)
				{
					fputs(TXT_ERR_MULTMODES, stderr);
					return CODE_FAILURE;
				}
				configuration.mode = MODE_ALL;
				break;

			case 'b':
				if (configuration.mode)
				{
					fputs(TXT_ERR_MULTMODES, stderr);
					return CODE_FAILURE;
				}
				configuration.mode = MODE_BAD;
				break;

			case 'c':
				configuration.config_file = optarg;
				break;

			case 'd':
				if (configuration.def_act)
				{
					fputs(TXT_ERR_MULTACTIONS, stderr);
					return CODE_FAILURE;
				}
				configuration.def_act = ACTION_DELETE;
				break;

			case 'e':
				excode = EXCODE_ACTED;	/* instead of default CODE_OK */
				break;

			case 'g':
				if (configuration.mode)
				{
					fputs(TXT_ERR_MULTMODES, stderr);
					return CODE_FAILURE;
				}
				configuration.mode = MODE_GOOD;
				break;

			case 'h':
				printf(PACKAGE_STRING "\n" COPYRIGHT "\n\n");
				printf(TXT_ERR_USAGE);
				printf(TXT_HELP_A);
				printf(TXT_HELP_B);
				printf(TXT_HELP_C);
				printf(TXT_HELP_D);
				printf(TXT_HELP_E);
				printf(TXT_HELP_G);
				printf(TXT_HELP_H);
				printf(TXT_HELP_K);
				printf(TXT_HELP_L);
				printf(TXT_HELP_P);
				printf(TXT_HELP_R);
				printf(TXT_HELP_S);
				printf(TXT_HELP_V1);
				printf(TXT_HELP_V2);
				return CODE_OK;
				break;

			case 'k':
				if (configuration.def_act)
				{
					fputs(TXT_ERR_MULTACTIONS, stderr);
					return CODE_FAILURE;
				}
				configuration.def_act = ACTION_KILL;
				break;
				
			case 'l':
				loopguard = 1;
				break;

			case 'r':
				if (configuration.def_act)
				{
					fputs(TXT_ERR_MULTACTIONS, stderr);
					return CODE_FAILURE;
				}
				configuration.def_act = ACTION_RENAME;
				break;

			case 'p':
			{
				/* The pipe command is everything else on the command line, as is */
				int argnum = 0;
				configuration.pipe_cmd = (char**)calloc(argc, sizeof(char*));
				for (optind--; optind<argc; optind++)
					configuration.pipe_cmd[argnum++] = argv[optind];
				break;			
			}
			
			case 's':
				justsettings = 1;
				break;

			case 'v':
				verbose = 1;
				break;

			case 'V':
				printf(PACKAGE_STRING "\n" COPYRIGHT "\n\n");
				return CODE_OK;
				break;

			default:
				fprintf(stderr, TXT_ERR_USAGE TXT_HELP_HINT);
				return CODE_FAILURE;
		}
	}

	/* If unspecified on command line, use default configuration file */
	if (!configuration.config_file)
		configuration.config_file = CONF_DIR "/" CONF_FILE;

	/* If unspecified on command line, set defaults */
	if (!configuration.mode)
		configuration.mode = MODE_BAD;		/* default filter mode is badlist */
	if (!configuration.def_act)
		configuration.def_act = ACTION_RENAME;	/* default filter action is to rename */

	/* Start filter with current action = default action */
	configuration.action = configuration.def_act;

	if (access(configuration.config_file, F_OK) == 0)
	{
		/* Only if configuration file exists, attempt to parse */
		if (parse_conf(&configuration) != CODE_OK)
			return CODE_FAILURE;	/* parse_conf already displayed specific error */
	}
	else
		fprintf(stderr, "%s%s\n", TXT_WARN_DEFAULTS, configuration.config_file);

	/* Use defaults if any directives were unset */
	directive_defaults(&configuration);
	if (justsettings || verbose)
	{
		show_configuration(&configuration);
		if (justsettings)
			return CODE_OK;
	}

	/* Generate secure temporary file, and go! */
	umask(077);
	tempfile = tmpfile();
	if (tempfile)
	{
		if (filter_pass(tempfile) && header_pass(tempfile))
		{
			fclose(tempfile);
			if (filtcount)
				return excode;	/* if --excode used, indicates filtering happened */
			else
				return CODE_OK;
		}
		else
		{
			fclose(tempfile);
			return CODE_TEMPFAIL;	/* temporary failure, e.g. out of resources */
		}
	}
	else
	{
		perror("tmpfile");
		return CODE_TEMPFAIL;		/* temporary failure, e.g. out of resources */
	}
}



/*
	This filtering pass does the actual mail attachment filtering.
	Output is written to dest, the temporary file. Verbose messages written to stderr.
	Returns nonzero on success, zero on failure.
*/
int filter_pass(FILE* dest)
{
	char linebuf[MAXLINEBUF];
	int onheaders = 1;			/* await end of message headers for kill/delete_exe checks */
	int on_mime = 0;			/* are we processing a MIME header block? */
	struct attach m_attachment;		/* for MIME attachments found  */
	struct attach u_attachment;		/* for uuencoded attachments found */
	struct zip_info zipstate;		/* for decoding of ZIP files encountered (containing filename) */
	char uuscanspec[MAXFIELD];		/* safe scanf spec for finding uuencoded attachments */
	long last_mime_pos = 0;			/* stream position of start of MIME headers */
	int sigcheckcount = 0;			/* for delete_exe and kill_exe, check for sig when > 0 */
	int skip_encoded = 0;			/*	0=no effect on message
							1=waiting for blank line
							2=skipping lines while waiting for blank line/boundary
							3=skipping lines while waiting for uuencode 'end'
						*/
	sprintf(uuscanspec, UUENC_START "%%*d %%%d[^\r\n]", MAXFIELD-1);
	memset(&m_attachment, 0, sizeof(struct attach));
	memset(&u_attachment, 0, sizeof(struct attach));
	init_zip(&zipstate);
	memset(messageid, 0, sizeof(messageid));

	/* Read line at a time from input */
	while (fgets(linebuf, sizeof(linebuf), stdin))
	{
		if (onheaders && ((*linebuf == '\n') || (*linebuf == '\r')) )
		{
			onheaders = 0;
			/* Watch for end of message headers, if using delete_exe or kill_exe features */
			if ((configuration.delete_exe == OPTION_ENABLED) || (configuration.kill_exe == OPTION_ENABLED))
				sigcheckcount = 10;	/* check binary signatures on next few lines */
		}

		/* Record message-id for logging purposes */
		if (onheaders && (strncasecmp(linebuf, HEAD_MESSAGEID, sizeof(HEAD_MESSAGEID)-1)==0))
			strncpy(messageid, linebuf, sizeof(messageid)-1);

		/* Remove Delivered-To from headers to prevent spammer's mail forwarding loop trick */
		if (onheaders && loopguard && (strncasecmp(linebuf, HEAD_DELIVERED_TO, sizeof(HEAD_DELIVERED_TO)-1)==0))
		{
			configuration.action = ACTION_SKIP;
			taking_action(TXT_DELIVEREDTO);
			continue;
		}

		/* First check for UUencode begin line */
		if ( (strncmp(linebuf, UUENC_START, sizeof(UUENC_START)-1)==0) &&
			(sscanf(linebuf, uuscanspec, u_attachment.cd_fname.name)==1) )
		{	/*
				Found a uuencoded attachment, and parsed its filename to the structure.
				Note that unlike MIME, there are no details, and filtering can be done NOW
			*/
			if ((configuration.delete_exe==OPTION_ENABLED) || (configuration.kill_exe==OPTION_ENABLED))
				sigcheckcount = 5;	/* check binary signatures on next few lines */
			if (filter_decision(&u_attachment))
			{
				if (configuration.action == ACTION_DELETE)
				{
					skip_encoded = 3;	/* skip body lines until 'end' is seen */
					sigcheckcount = 0;	/* don't bother checking; already deleting */
				}
				taking_action(u_attachment.curspec->oldname);
			}
			/* filter_decision has made sure curspec points to good data, so write fresh */
			fprintf(dest, UUENC_START "600 %s\n", u_attachment.curspec->name);
			memset(&u_attachment, 0, sizeof(struct attach));
			continue;	/* Important; we need to drop old begin line */
		}
		else if (is_line_mime(linebuf, on_mime, &m_attachment))	/* also parses attach values */
		{
			if (!on_mime)
			{
				/* just entered a MIME header block */
				on_mime = 1;
				last_mime_pos = ftell(dest);	/* note stream pos for possible overwriting */
				if (last_mime_pos == -1)
					return 0;	/* ftell error, can't continue */
			}
		}
		else if (on_mime)
		{
			/* Now past end of MIME headers. Decision time, "do we do anything?" */
			on_mime = 0;
			if ((configuration.delete_exe==OPTION_ENABLED) || (configuration.kill_exe==OPTION_ENABLED))
				sigcheckcount = 10;	/* check binary signatures on next few lines */
			if (m_attachment.fattach)
			{
				/*
					There was an attached file. We will write the MIME header
					fresh. The call to filter_decision ensures that curspec
					points to safe or now-safe data, which should be rewritten.
				*/
				char nameout[MAXFIELD];	/* the (possibly re-encoded) name to write out */
				if (filter_decision(&m_attachment))
				{
					if (configuration.action == ACTION_DELETE)
					{
						skip_encoded = 1;	/* once we find blank line, start skipping */
						sigcheckcount = 0;	/* don't bother checking; already deleting */
					}
					taking_action(m_attachment.curspec->oldname);
				}
				if (fseek(dest, last_mime_pos, SEEK_SET) != 0)
					return 0;	/* fseek error, can't continue */

				/* Write fresh MIME header; filter_decision modified dangerous parts */
				if (*m_attachment.content_type)
				{
					if (reencode_name(m_attachment.curspec, "\tname", nameout) != 0)
						sprintf(nameout, "\tname=\"%s\"", configuration.generic_name);
					fprintf(dest, MIME_CTYPE " %s;\n%s\n", m_attachment.content_type, nameout);
				}
				if (reencode_name(m_attachment.curspec, "\tfilename", nameout) != 0)
					sprintf(nameout, "\tfilename=\"%s\"", configuration.generic_name);
				fprintf(dest, MIME_CDISP " attachment;\n%s\n", nameout);
				if (*m_attachment.content_enc)
					fprintf(dest, MIME_CTENC " %s\n", m_attachment.content_enc);
				/* If user wants to pass Content-ID, add back that header unfiltered */
				if ((configuration.pass_contentid == OPTION_ENABLED) && *m_attachment.content_id)
					fprintf(dest, MIME_CNTID " %s\n", m_attachment.content_id);
			}
			memset(&m_attachment, 0, sizeof(struct attach));
		}

		/* Check for encoded executable signatures (delete_exe, kill_exe) */
		if (sigcheckcount > 0)
		{
			sigcheckcount--;
			if (	(strncmp(linebuf, B64_EXESIG1, 3) == 0) ||
				(strncmp(linebuf, B64_EXESIG2, 3) == 0) ||
				(strncmp(linebuf, B64_EXESIG3, 3) == 0) ||
				(strncmp(linebuf, B64_EXESIG4, 3) == 0) )
			{
				if (configuration.kill_exe == OPTION_ENABLED)
				{
					configuration.action = ACTION_KILL;
					kill_exit(TXT_ENCODEDBODY);	/* kill email and exit */
				}
				skip_encoded = 2;
				configuration.action = ACTION_DELETE;
				exe_count++;
				taking_action(TXT_ENCODEDBODY);
			}
			else if ( (strncmp(linebuf+1, UUE_EXESIG1, 3) == 0) ||
				(strncmp(linebuf+1, UUE_EXESIG2, 3) == 0) ||
				(strncmp(linebuf+1, UUE_EXESIG3, 3) == 0) ||
				(strncmp(linebuf+1, UUE_EXESIG4, 3) == 0) )
			{
				if (configuration.kill_exe == OPTION_ENABLED)
				{
					configuration.action = ACTION_KILL;
					kill_exit(TXT_ENCODEDBODY);	/* kill email and exit */
				}
				skip_encoded = 3;
				configuration.action = ACTION_DELETE;
				exe_count++;
				taking_action(TXT_ENCODEDBODY);
			}
		}

		/*
			Check for filenames inside ZIPs. Any filenames found inside are subject
			to the standard goodlist/badlist tests and deleted or killed, except that
				NOTHING IS DONE IF CURRENT ACTION IS 'RENAME' (dummy action)
		*/
		if (configuration.search_zip == OPTION_ENABLED)
		{
			int decoded;
			char textbuf[MAXLINEBUF], binbuf[MAXLINEBUF];

			strncpy(textbuf, linebuf, sizeof(textbuf));
			trim_trailch(textbuf, '\r', '\n', ' ');
			if (*textbuf == 0)
				init_zip(&zipstate);
			else if( (skip_encoded<2) && (decoded=base64_decode_line(textbuf, binbuf)) )
			{
				char* zip_pos = binbuf;
				int available = decoded;
				while ((zip_pos=unzip_filename(zip_pos, available, &zipstate)))
				{
					/* Found a filename inside ZIP archive. Synthesize struct attach */
					struct attach inzip;
					memset(&inzip, 0, sizeof(struct attach));
					inzip.within = 1;
					strncpy(inzip.ct_fname.name, zipstate.filename, sizeof(inzip.ct_fname.name));
					if (filter_decision(&inzip))
					{
						if (configuration.action == ACTION_DELETE)
						{
							skip_encoded = 2;	/* skip rest of attachment */
							taking_action(zipstate.filename);
						}
						else
							not_taking_action(TXT_INFO_CANTRENZIP, zipstate.filename);
					}
					available = (binbuf+decoded) - zip_pos;
				}
			}
		}

		if ( (skip_encoded==1) && ((*linebuf == '\n')||(*linebuf == '\r')) )
			skip_encoded = 2;	/* now we're actually skipping body lines */
		else if (skip_encoded == 2)	/* Encoded body lines will be skipped if 'deleting' an attachment. */
		{
			if ( (*linebuf == '\n') || (*linebuf == '\r') || (strncmp(linebuf, "--", 2)==0) )
				skip_encoded = 0;
			else
				continue;
		}
		else if (skip_encoded == 3)	/* Body lines skipped until uuencode 'end' is seen */
		{
			if (strncmp(linebuf, UUENC_END, sizeof(UUENC_END)-1) == 0)
				skip_encoded = 0;
			else
				continue;
		}

		/* Write (non-skipped) lines to dest; some may be overwritten if we fseek back */
		if (fputs(linebuf, dest) == EOF)
			return 0;	/* fputs error, can't continue */

	}
	return 1;
}



/*
	This header modification pass doesn't do any actual filtering.
	The previously filtered mail is read back from source, and some headers
	are added and modified before writing to destination (stdout or pipe).
	A warning message (warning_text and/or warning_html) may also be added.
	source is file handle to temporary file
	Returns nonzero on success, zero on failure
*/
int header_pass(FILE* source)
{
	char linebuf[MAXLINEBUF];
	long endpos;
	int onheaders = 1;
	int subject_changed = 0;
	int plain_part = 0;
	int html_part = 0;
	int plain_warning = (configuration.warning_text && *configuration.warning_text);
	int html_warning = (configuration.warning_html && *configuration.warning_html);
	FILE* dest = stdout;
	pid_t pid;
	
	if (plain_warning)
		plain_part = 1;

	/* Stream data may have been overwritten, so we need to set new end of file */
	endpos = ftell(source);
	if (endpos == -1)
		return 0;	/* ftell error, can't continue */
	rewind(source);
	if (ftruncate(fileno(source), endpos) != 0)
		return 0;	/* ftruncate error, can't continue */
	if (fflush(source) != 0)
		return 0;	/* fflush error, can't continue */

	/* Open output destination pipe, if requested */
	if (configuration.pipe_cmd)
	{
		dest = openpipe(configuration.pipe_cmd, &pid);
		if (dest == NULL)
		{
			fprintf(stderr, TXT_ERR_OPENPIPE ": %s\n", configuration.pipe_cmd[0]);
			return 0;	/* error opening pipe */
		}
		signal(SIGPIPE, abnormalproc);	/* establish signal handler for pipe error */
	}

	while (fgets(linebuf, sizeof(linebuf), source))
	{
		/* Reached blank line */
		if ((*linebuf == '\n') || (*linebuf == '\r'))
		{
			/* If we were still on headers, then append our new headers */
			if (onheaders)
			{
				onheaders = 0;
				fprintf(dest, "X-Filtered-With: %s\n", PACKAGE_STRING);
				fprintf(dest, "X-RenAttach-Info: "TXT_HEAD_MODE"=%s " TXT_HEAD_ACTION"=%s " \
					TXT_HEAD_FILTCOUNT"=%d", mode2str(configuration.mode),
					act2str(configuration.action), filtcount);
				if (filtcount)
				{
					fprintf(dest, " (%s)\n", TXT_HEAD_FILTERED);
					if (!subject_changed)
						explain_subject(dest, NULL);	/* add new Subject */
				}
				else
					fprintf(dest, "\n");
				/* If filtering occurred and there are custom headers, add them */
				if (filtcount && configuration.add_header && *configuration.add_header)
					fprintf(dest, "%s\n", configuration.add_header);
			}
			
			/* We've reached start of body part, insert warning if desired */
			if (filtcount && plain_part)
			{
				fprintf(dest, "\n%s\n", configuration.warning_text);
				plain_part = 0;
			}
			else if (filtcount && html_part)
			{
				fprintf(dest, "\n");
				if (position_html(source, dest) != 0)
					return 0;	/* error during seeking */
				fprintf(dest, "%s", configuration.warning_html);
				fgets(linebuf, sizeof(linebuf), source);	/* because source shifted */
				html_part = 0;
			}
		}
		
		/* Processing of message headers */
		if (onheaders)
		{
			/* Modify subject if required */
			if (filtcount && (strncasecmp(linebuf, HEAD_SUBJECT, sizeof(HEAD_SUBJECT)-1)==0))
			{
				subject_changed = 1;
				if (explain_subject(dest, linebuf+sizeof(HEAD_SUBJECT)-1))
					continue;
			}
	
			/* Keep existing X- headers specific to RenAttach, by renaming as Old- */
			if ( (strncasecmp(linebuf, HEAD_FILTERED_WITH, sizeof(HEAD_FILTERED_WITH)-1)==0)
				|| (strncasecmp(linebuf, HEAD_FILTERED_INFO, sizeof(HEAD_FILTERED_INFO)-1)==0) )
			{
				fprintf(dest, "Old-%s", linebuf);
				continue;
			}
		}
		
		/* If warning_text or warning_html is enabled, search for next parts */
		if (plain_warning && (strncasecmp(linebuf, MIME_TYPETEXT, sizeof(MIME_TYPETEXT)-1) == 0))
		{
			plain_part = 1;
			html_part = 0;
		}
		else if (html_warning && (strncasecmp(linebuf, MIME_TYPEHTML, sizeof(MIME_TYPEHTML)-1) == 0))
		{
			plain_part = 0;
			html_part = 1;
		}
		
		if (fputs(linebuf, dest) == EOF)
			return 0;	/* fputs error */
	}

	/* Make sure output gets written */
	if (dest != stdout)
	{
		int retcode = -1;
		if (closepipe(dest, pid, &retcode) != 0)
			fprintf(stderr, TXT_ERR_CLOSEPIPE ": %s\n", configuration.pipe_cmd[0]);
		if (verbose)
			fprintf(stderr, "%s: 0x%X\n", TXT_INFO_PIPEXITCODE, retcode);
		if (retcode != 0)	/* by convention, nonzero exitcode indicates error condition */
			return 0;	/* header pass failed due to pipe error */
	}
	return 1;
}



/*
	For any given line (considering the 'on_mime' boolean state) returns
	false (zero) if line does not indicate MIME presence, and true (nonzero)
	if line indicates MIME presence. In the process of seeking MIME data,
	the m_attachment structure is filled in with any parsed values.
*/
int is_line_mime(const char* line, int on_mime, struct attach* m_attachment)
{
	const char* orgline = line;
	int found_mime = 0;

	/* Is there something worth trying to parse? */
	if (on_mime && ((*line == '\t') || (*line == ' ')) )
		found_mime = 1;
	else if	(	(strncasecmp(line, MIME_CTYPE, sizeof(MIME_CTYPE)-1) == 0) ||
			(strncasecmp(line, MIME_CTENC, sizeof(MIME_CTENC)-1) == 0) ||
			(strncasecmp(line, MIME_CDISP, sizeof(MIME_CDISP)-1) == 0) ||
			(strncasecmp(line, MIME_CNTID, sizeof(MIME_CNTID)-1) == 0) ||
			(strncasecmp(line, MIME_CDESC, sizeof(MIME_CDESC)-1) == 0) )
		found_mime = 1;
	
	if (found_mime)
	{
		int parsed_name = 0;				/* flag, was name parsed? */
		char* nameloc = stristr(line, MIME_NAME);	/* case-insensitive search */

		/* MIME_NAME should be before the first '=' sign */
		if (nameloc >= strchr(line, '='))
			nameloc = NULL;
		
		if (strncasecmp(line, MIME_CTYPE, sizeof(MIME_CTYPE)-1) == 0)
		{
			/* Trying to parse content type */
			char scanspec[MAXFIELD];
			sprintf(scanspec, "%%%d[^;\r\n\t]", MAXFIELD-1);	/* generate safe scanf spec */
			if (sscanf(line+sizeof(MIME_CTYPE)-1, scanspec, m_attachment->content_type) == 1)
			{
				trim_leading(m_attachment->content_type);
				trim_trailing(m_attachment->content_type);
			}
			m_attachment->curspec = &m_attachment->ct_fname;
		}
		else if (strncasecmp(line, MIME_CDISP, sizeof(MIME_CDISP)-1) == 0)
		{
			m_attachment->curspec = &m_attachment->cd_fname;
		}
		else if (strncasecmp(line, MIME_CTENC, sizeof(MIME_CTENC)-1) == 0)
		{
			/* Trying to parse content transfer encoding */
			char scanspec[MAXFIELD];
			sprintf(scanspec, "%%%d[^;\r\n\t]", MAXFIELD-1);	/* generate safe scanf spec */
			if (sscanf(line+sizeof(MIME_CTENC)-1, scanspec, m_attachment->content_enc) == 1)
			{
				trim_leading(m_attachment->content_enc);
				trim_trailing(m_attachment->content_enc);
			}
		}
                else if (strncasecmp(line, MIME_CNTID, sizeof(MIME_CNTID)-1) == 0)
		{
			/* Trying to parse Content-ID */
			char scanspec[MAXFIELD];
			sprintf(scanspec, "%%%d[^;\r\n\t]", MAXFIELD-1);	/* generate safe scanf spec */
			if (sscanf(line+sizeof(MIME_CNTID)-1, scanspec, m_attachment->content_id) == 1)
			{
				trim_leading(m_attachment->content_id);
				trim_trailing(m_attachment->content_id);
			}
		}
		
		/* Now hunt for filenames! */
		while (m_attachment->curspec && nameloc)
		{
			char parsed[MAXFIELD];
			char scanspec1[MAXFIELD], scanspec2[MAXFIELD], scanspec3[MAXFIELD], scanspec4[MAXFIELD];

			/* Strong suspicion that this is a file attachment, even if later parsing fails */
			m_attachment->fattach = 1;	/* renattach must rewrite a fresh MIME header */
			line = nameloc + sizeof(MIME_NAME);	/* advance pointer for next search */

			/* Check for extension; RFC 2231 fields */
			if (strncasecmp(nameloc, MIME_NAMEX, sizeof(MIME_NAMEX)-1) == 0)
			{
				m_attachment->curspec->specformat = FMT_RFC2231;
				/* Check to see if field carries charset/lang info */
				if (strstr(nameloc, "*="))
				     m_attachment->curspec->langinfo = 1;
			}

			sprintf(scanspec1, "%%*[ *0-9]%%*[ =]\"%%%d[^\r\n\"]", MAXFIELD-1);
			sprintf(scanspec2, "%%*[ *0-9]%%*[ =]%%%d[^\r\n\";]", MAXFIELD-1);
			sprintf(scanspec3, "%%*[ =]\"%%%d[^\r\n\"]", MAXFIELD-1);
			sprintf(scanspec4, "%%*[ =]%%%d[^\r\n\";]", MAXFIELD-1);
			if (	(sscanf(nameloc+sizeof(MIME_NAME)-1, scanspec1, parsed) == 1) ||
				(sscanf(nameloc+sizeof(MIME_NAME)-1, scanspec2, parsed) == 1) ||
				(sscanf(nameloc+sizeof(MIME_NAME)-1, scanspec3, parsed) == 1) ||
				(sscanf(nameloc+sizeof(MIME_NAME)-1, scanspec4, parsed) == 1) )
			{
				int remaining;
				parsed_name = 1;
				line += strlen(parsed);	/* further advance pointer for next search */
				if (m_attachment->curspec->specformat == FMT_RFC2231)
					decode_hex(parsed, '%', 0, NULL);
				else if ((strstr(parsed, "=?") || strstr(parsed, "?=")) && (decode_2047(parsed, m_attachment->curspec)==0) )
					m_attachment->curspec->specformat = FMT_RFC2047;
				else
				{
					trim_leading(parsed);
					trim_trailing(parsed);
				}
				remaining = MAXFIELD - strlen(m_attachment->curspec->name) - 1;
				strncat(m_attachment->curspec->name, parsed, remaining);
			}

			/* Prepare for next iteration */
			nameloc = stristr(line, MIME_NAME);	/* case-insensitive search */
		}
		
		/* Might have encountered (obsolete format) line continuation on encoded filename */
		if (!parsed_name && m_attachment->curspec && (m_attachment->curspec->specformat == FMT_RFC2047))
		{
			char parsed[MAXFIELD], scanspec[MAXFIELD];
			sprintf(scanspec, "%%*[\t ]%%%d[^\r\n\"]", MAXFIELD-1);
			if ((sscanf(orgline, scanspec, parsed)==1) && (decode_2047(parsed, m_attachment->curspec)==0))
			{
				int remaining = MAXFIELD - strlen(m_attachment->curspec->name) - 1;
				strncat(m_attachment->curspec->name, parsed, remaining);
			}
		}
	}
	return found_mime;
}


/*
	By checking each parsed filename, determined whether filter action is necessary.
	If a filename is deemed dangerous, substitutions are performed.

	m_attach->curspec will point to namespec that MUST be used for everything after!
		(this is the namespec that is known to be safe for rewriting)
	m_attach->curspec will point to valid data, but the name may still be blank

	Returns zero if no filter action
	Returns nonzero if filter action
*/
int filter_decision(struct attach* m_attach)
{
	char* dotsearch;

	/* remove padding whitespace, and trailing periods */
	trim_leading(m_attach->ct_fname.name);
	trim_trailch(m_attach->ct_fname.name, '\t', ' ', '.');
	trim_leading(m_attach->cd_fname.name);
	trim_trailch(m_attach->cd_fname.name, '\t', ' ', '.');

	/* See if either of the parsed filenames gets caught by filter */
	if (match_filename(m_attach->ct_fname.name, m_attach->within))
		m_attach->curspec = &m_attach->ct_fname;
	else if (match_filename(m_attach->cd_fname.name, m_attach->within))
		m_attach->curspec = &m_attach->cd_fname;
	else
	{
		/* Nothing to filter. Point curspec toward preferred field */
		if (*(m_attach->cd_fname.name))
			m_attach->curspec = &m_attach->cd_fname;
		else
			m_attach->curspec = &m_attach->ct_fname;
		return 0;	/* nothing to filter */
	}

	/* Caught a filename! Record the original filename */
	strcpy(m_attach->curspec->oldname, m_attach->curspec->name);

	if (configuration.action == ACTION_KILL)
		kill_exit(m_attach->curspec->oldname);	/* absorbs email, and exits */

	if (configuration.full_rename == OPTION_ENABLED)
	{
		/* Full rename replaces all periods with underscores (safest) */
		for (dotsearch = m_attach->curspec->name; *dotsearch; dotsearch++)
		{
			if (*dotsearch == '.')
				*dotsearch = '_';
		}
	}
	else
	{
		/* Partial rename just replaces final period with underscore */
		dotsearch = strrchr(m_attach->curspec->name, '.');
		if (dotsearch)
			*dotsearch = '_';
	}

	/*
		underscores alone make extension safe
		Also tack on new extension if there is enough buffer space
	*/
	/* bhoc@pentagroup.ch <mailto:bhoc@pentagroup.ch> : if new_exension is "#" then do nothing */
	if (strcmp(configuration.new_extension, "#") != 0)
	{
		if (1 + strlen(m_attach->curspec->name) + strlen(configuration.new_extension)
			< sizeof(m_attach->curspec->name))
		{
			strcat(m_attach->curspec->name, ".");
			strcat(m_attach->curspec->name, configuration.new_extension);
		}
	}
	/* Replace the Content-Type, since that is now suspicious */
	strcpy(m_attach->content_type, configuration.new_mime_type);
	return 1;	/* this MIME attachment was filtered */
}


/*
	Does this filename match (by whatever the current criteria are)
	Provide flag if filename was found inside zip
	returns 0 - do not act (file poses no risk)
	returns 1 - do act, something should be done to this file
*/
int match_filename(const char* filename, int inzip)
{
	char extension[MAXFIELD];
	char *lastdot, *listcopy, *token;

	if (*filename == '\0')
		return 0;	/* don't act (even if MODE_ALL); no file name */
	if (configuration.mode == MODE_ALL)
		return 1;	/* do act, regardless */

	/* If there is a banned_files list, we must check it first */
	if (configuration.banned_files && tokenize_list(configuration.banned_files, 0))
	{
		listcopy = malloc(strlen(configuration.banned_files) + 1);
		strcpy(listcopy, configuration.banned_files);
		token = strtok(listcopy, LIST_TOKENS);
		while (token)
		{
			int action=0, submatch=0;
			char* actswitch;

			if (*token == '/')
			{
				submatch = 1;			/* match substring instead of exact */
				token++;
			}
			actswitch = strchr(token, '/');		/* see if action is specified */
			if (actswitch)
			{
				switch (actswitch[1])
				{
					case 'r':
					case 'R':
						action = ACTION_RENAME;
						break;
					case 'd':
					case 'D':
						action = ACTION_DELETE;
						break;
					case 'k':
					case 'K':
						action = ACTION_KILL;
						break;
				}
				*actswitch = '\0';	/* drop the action specification */
			}
			/* Now see if we have found a banned file name */
			if ((submatch && stristr(filename, token)) || (strcasecmp(filename, token)==0))
			{
				ban_count++;
				if (action)	/* if provided, use specific action */
					configuration.action = action;	/* reset by filter_action() */
				free(listcopy);
				return 1;
			}
			token = strtok(NULL, LIST_TOKENS);
		}
		free(listcopy);
	}

	lastdot = strrchr(filename, '.');
	if (!lastdot || lastdot[1] == '\0')
		return 0;	/* don't act; no file extension */

        memset(extension, 0, sizeof(extension));
	strncpy(extension, lastdot+1, sizeof(extension)-1);
	trim_leading(extension);
	trim_trailing(extension);

	/* We now have a cleaned up file extension to compare */

	if (configuration.mode == MODE_BAD)
	{
		/* do badlist comparison */
		listcopy = malloc(strlen(configuration.badlist) + 1);
		strcpy(listcopy, configuration.badlist);
		token = strtok(listcopy, LIST_TOKENS);
		while (token)
		{
			int action=0;		/* use default action, unless overridden */
			char *switchar, *actswitch, *inswitch = NULL;

			actswitch = strchr(token, '/');		/* see if action is specified */
			if (actswitch)
				inswitch = strchr(actswitch+1, '/');
			if (inzip && inswitch)			/* if specific action found for inside ZIP */
				switchar = inswitch;		/* 	use that action */
			else
				switchar = actswitch;		/* otherwise, use first action found */

			if (switchar)
			{
				switch (switchar[1])	/* select specific action */
				{
					case 'r':
					case 'R':
						action = ACTION_RENAME;
						break;
					case 'd':
					case 'D':
						action = ACTION_DELETE;
						break;
					case 'k':
					case 'K':
						action = ACTION_KILL;
						break;
				}
			}
			if (actswitch)
				*actswitch = '\0';	/* terminate string at first switch */

			if (strcasecmp(token, extension) == 0)
			{
				if (action)	/* if provided, use specific action */
					configuration.action = action;	/* reset by filter_action() */
				free(listcopy);
				return 1;	/* file extension is on badlist */
			}
			token = strtok(NULL, LIST_TOKENS);
		}
		free(listcopy);
		return 0;	/* file extension is NOT on badlist */
	}
	else if (configuration.mode == MODE_GOOD)
	{
		/* do goodlist comparison */
		listcopy = malloc(strlen(configuration.goodlist) + 1);
		strcpy(listcopy, configuration.goodlist);
		token = strtok(listcopy, LIST_TOKENS);
		while (token)
		{
			if (strcasecmp(token, extension) == 0)
			{
				free(listcopy);
				return 0;	/* file extension is on goodlist */
			}
			token = strtok(NULL, LIST_TOKENS);
		}
		free(listcopy);
		return 1;	/* file extension is NOT on goodlist */
	}
	else
		return 1;	/* should never be reached */
}


/*
	Attempts to decode an RFC-2047 encoded field
	Fills in information about fieldspec

	Returns 0 if successful, nonzero otherwise
*/
int decode_2047(char* field, struct namespec* fieldspec)
{
	char scanspec[MAXFIELD];
	char* result;	/* result of decoding */
	int pos;	/* position marker */
	int field_len;	/* length of field */
	int improper;	/* flag set if improper coding found */
	int valid = 0;	/* flag set when valid coding found */
	int state = 0;	/*	0 = normal
				1 = just inside encoded word
				2 = found pre-encoding ? */

	sprintf(scanspec, "=?%%%d[^?]", MAXFIELD-1);
	improper = 0;
	field_len = strlen(field);
	result = calloc(field_len+1, 1);

	for (pos=0; ( pos<field_len ) && !improper; pos++)
	{
		/* Spaces kill decoding */
		if ( (state>0) && (field[pos]==' ') )
		{
			improper = 1;
			break;
		}
		
		switch(state)
		{
			case 0:
				/* Make a note of the character encoding */
				if (	(sscanf(field+pos, scanspec, fieldspec->charenc) == 1) ||
					(sscanf(field+pos, scanspec+1, fieldspec->charenc) == 1) )
				{
					valid = 1;
					state++;
					pos++;
				}
				else
					sprintf(result+strlen(result), "%c", field[pos]);
				break;
			
			case 1:
				if (field[pos] == '?')
					state++;
				break;
				
			case 2:
				if (strncasecmp(field+pos, "Q?", 2) == 0)
					state = 3;
				else if (strncasecmp(field+pos, "B?", 2) == 0)
					state = 4;
				else
					improper = 1;
				if (state > 2)
				{
					char* termin;
					pos += 2;	/* skip past encoding field */
					termin = strstr(field+pos, "?=");
					if (!termin)
						improper = 1;
					else
					{
						int whitesp = 0;
						*termin = 0;	/* end here, for now */
						if (state == 3)
						{
							fieldspec->charmode = 'Q';
							decode_hex(field+pos, '=', 1, result+strlen(result));
						}
						else
						{
							fieldspec->charmode = 'B';
							base64_decode_line(field+pos, result+strlen(result));
						}
						*termin = '?';	/* put back old char */
						pos = termin - field + 2;
						while ((field[pos]==' ') || (field[pos]=='\t'))
						{
							whitesp = 1;
							pos++;	/* delay printing whitespace */
						}
						if (whitesp && (strncmp(field+pos, "=?", 2) != 0))
							strcat(result, " ");
						if (pos > 0) pos--;	/* loop fixup */
						state = 0;
					}
				}
				break;
			
			default:
				improper = 1;
		}
	}
	
	if (improper || !valid)
	{
		free(result);
		*field = 0;	/* clear the invalid field */
		return 1;
	}
	else
	{
		strcpy(field, result);	/* result is always shorter than original */
		free(result);
		return 0;
	}
}


/*
	Re-encode a filename back to its original encoding format.
	The details of the format are supplied by struct namespec.
	reencode_name recreates the original format as close to spec
	as possible, storing the output (including MIME tags based on
	tagbase) to result, assume to hold MAXFIELD

	Returns 0 if all went well, and result contains the encoded form
		result may contain intermediate newlines, but no terminating newline
	Returns nonzero on failure (buffer filled, corrupt coding...)

*/
int reencode_name(struct namespec* fieldspec, const char* tagbase, char* result)
{
	int namelen = strlen(fieldspec->name);
	if (namelen == 0)
		return 1;	/* empty argument */
	memset(result, 0, MAXFIELD);	/* clear result */

	if (fieldspec->specformat == FMT_PLAIN)
	{
		/* Simple case, no encoding required */
		if (strlen(tagbase)+3+namelen < MAXFIELD)
		{
			sprintf(result, "%s=\"%s\"", tagbase, fieldspec->name);
			return 0;	/* success */
		}
		else
			return 1;	/* insufficient buffer space */
	}
	else if (fieldspec->specformat == FMT_RFC2047)
	{
		int remain = MAXFIELD - strlen(tagbase) - 4 - strlen(fieldspec->charenc) - 3;
		if (remain <= 1)
			return 1;	/* insufficient buffer space */
		sprintf(result, "%s=\"=?%s?%c?", tagbase, fieldspec->charenc, fieldspec->charmode);
		if (fieldspec->charmode == 'B')	/* going to base64 convert */
		{
			remain -= (1+namelen/3)*4;
			if (remain <= 1)
				return 1;	/* failure */
			base64_encode_line(fieldspec->name, namelen, result+strlen(result));
		}
		else	/* quoted printable encoding */
		{
			int pos;
			remain -= 3*namelen;	/* worst case */
			if (remain <= 1)
				return 1;	/* failure */
			for (pos=0; pos<namelen; pos++)
			{
				/* Must not contain unescaped 8-bit, space, tab, question mark */
				if (	isalnum((int)fieldspec->name[pos]) || (fieldspec->name[pos] == '.')
					|| (fieldspec->name[pos] == '-') || (fieldspec->name[pos] == '!')
					|| (fieldspec->name[pos] == ':') || (fieldspec->name[pos] == '\\') )
				{
					sprintf(result+strlen(result), "%c", fieldspec->name[pos]);
				}
				else if (fieldspec->name[pos] == ' ')
					strcat(result, "_");	/* nice space */
				else
				{
					sprintf(result+strlen(result), "=%02X",
						(unsigned char)fieldspec->name[pos]);
				}
			}
		}
		remain -= 3;
		if (remain <= 1)
			return 1;	/* failure */
		strcat(result, "?=\"");
		return 0;		/* success */
	}
	else if (fieldspec->specformat == FMT_RFC2231)
	{
		int pos, remain, linecount = 0;
		int linelen = 0;	/* output line length */
		int singquote = 0;	/* count two single quotes (relevant when langinfo exists) */

		if (fieldspec->langinfo)
			sprintf(result, "%s*%u*=", tagbase, linecount++);
		else
			sprintf(result, "%s*%u=\"", tagbase, linecount++);
		remain = MAXFIELD - strlen(result);

		/* Write characters out, folding lines and encoding as necessary */
		for (pos=0; pos<namelen; pos++)
		{
			if (linelen > 50)
			{
				linelen = 0;
				remain -= strlen(tagbase) + 10;	/* for tag we're about to write */
				if (remain <= 1)
					return 1;	/* failure */

				/* Fold, begin new line */
				if (fieldspec->langinfo)
					sprintf(result+strlen(result), ";\n%s*%u*=", tagbase, linecount++);
				else
					sprintf(result+strlen(result), "\";\n%s*%u=\"", tagbase, linecount++);
			}

			/*
				Write character directly without coding if: there's no language info,
				or character is alpha numeric, or we're delaying encoding until single quotes
			*/
			if (!fieldspec->langinfo || isalnum((int)fieldspec->name[pos]) || (singquote<2))
			{
				if (fieldspec->name[pos] == '\'')
					singquote++;
				linelen++;
				remain--;
				if (remain <= 1)
					return 1;	/* failure */
				sprintf(result+strlen(result), "%c", fieldspec->name[pos]);
			}
			else
			{	/* Encoding is required... write 3 characters */
				linelen += 3;
				remain -= 3;
				if (remain <= 1)
					return 1;	/* failure */
				sprintf(result+strlen(result), "%%%02X", (unsigned char)fieldspec->name[pos]);
			}
		}
		/* If we were writing without encoding (no langinfo) then terminate quotes */
		if (!fieldspec->langinfo)
		{
			remain--;
			if (remain <= 1)
				return 1;	/* failure */
			strcat(result, "\"");
		}
		return 0;	/* success */
	}
	else
	{
		/* That's strange, there's a flag we don't recognize */
		return 1;
	}
}


/*
	This function finds the "right place" (based on htmlwarn_pos from .conf file) to insert warning_html.
	The current source_pos is recorded, then source is read through in search of all htmlwarn_pos tags.
	Once the source_htmlspot is found, the region from source_pos to source_htmlpos is written to dest.
	When the function returns, dest points to the correct spot to output the warning_html.
	Returns 0 on success, nonzero if seeking fails
*/
int position_html(FILE* source, FILE* dest)
{
	char* tag = NULL;
	char linebuf[MAXFIELD];
	char taglist[MAXFIELD];
	long dump_size;
	long source_start, source_htmlspot;

	source_start = ftell(source);
	if (source_start == -1)
		return -1;
	source_htmlspot = source_start;
	strcpy(taglist, configuration.htmlwarn_pos);
	tag = strtok(taglist, LIST_TOKENS);
	while (tag && fgets(linebuf, sizeof(linebuf), source))
	{
		char *tagstart, *tagend;
		char fulltag[MAXFIELD];
		sprintf(fulltag, "<%s", tag);
		if ((tagstart = stristr(linebuf, fulltag)) && (tagend = strchr(tagstart, '>')))
		{
			/* Located one more tag, note the best spot found so far */
			source_htmlspot = ftell(source);
			if (source_htmlspot == -1)
				return -1;
			source_htmlspot = source_htmlspot - strlen(linebuf) + (tagend - linebuf) + 1;
			if (fseek(source, source_htmlspot, SEEK_SET) != 0)	/* backtrack for next fgets */
				return -1;
			tag = strtok(NULL, LIST_TOKENS);
		}
		if (strncasecmp(linebuf, MIME_CTYPE, sizeof(MIME_CTYPE)-1) == 0)
			break;
	}
	/* Either advanced through all tags (ideal) or reached end of source */
	if (fseek(source, source_start, SEEK_SET) != 0)
		return -1;
	dump_size = source_htmlspot - source_start;
	if (dump_size > 0)
	{
		char* dump_buf = calloc(1, 1+dump_size);
		if (!dump_buf)
			return -1;
		if (fread(dump_buf, dump_size, 1, source) < 1)
			return -1;
		fprintf(dest, "%s", dump_buf);
		free(dump_buf);
	}
	return 0;
}


/*
	Kill this email and exit the program, taking_action along the way
*/
void kill_exit(const char* text)
{
	/* To properly absorb email, read stdin to completion */
	char linebuf[MAXLINEBUF];
	taking_action(text);
	while (fgets(linebuf, sizeof(linebuf), stdin))
		;
	exit(excode);	/* if --excode used, indicates filtering happened */
}


/*
	Called whenever taking some kind of filtering action (the type: configuration.action)
	Responsible for counting filtering, logging

	NOTE that this function resets configuration.action -- call it AFTER handling
*/
void taking_action(const char* caughtfile)
{
	if (verbose)
		fprintf(stderr, "%s \"%s\" in %s", act2str(configuration.action), caughtfile, messageid);
	if (configuration.use_syslog == OPTION_ENABLED)
	{
		#ifdef HAVE_SYSLOG_H
		openlog("renattach", LOG_PID, LOG_MAIL);
		syslog(LOG_WARNING, "%s \"%s\" in %s", act2str(configuration.action), caughtfile, messageid);
		closelog();
		#endif
	}
	/* Update filter counters (but not in case of ACTION_SKIP) */
	if (configuration.action != ACTION_SKIP)
		filtcount++;
	if (configuration.action == ACTION_DELETE)
		del_count++;
	else if (configuration.action == ACTION_RENAME)
		ren_count++;
	/* Reset to default action */
	configuration.action = configuration.def_act;
}


/*
	Called when NOT acting upon filename; logs warning
	As with taking_action, this resets configuration.action
*/
void not_taking_action(const char* reason, const char* missedfile)
{
	if (verbose)
		fprintf(stderr, TXT_INFO_NOTACTING " - %s: %s\n", reason, missedfile);
	if (configuration.use_syslog == OPTION_ENABLED)
	{
		#ifdef HAVE_SYSLOG_H
		openlog("renattach", LOG_PID, LOG_MAIL);
		syslog(LOG_WARNING, TXT_INFO_NOTACTING " - %s: %s", reason, missedfile);
		closelog();
		#endif
	}
	/* Reset to default action */
	configuration.action = configuration.def_act;
}


/*
	Explain filtering by writing a new Subject field to dest
	oldsubject may be NULL (if adding an entirely new Subject field)

	Returns 0 if nothing written ('#' rule suppressed output)
	Returns nonzero if new subject has been written
*/
int explain_subject(FILE* dest, char* oldsubject)
{
	char* therest = "\n";
	if (oldsubject)
		therest = oldsubject;

	if (ban_count && *configuration.subj_banned)
	{
		if (strcmp(configuration.subj_banned, "#") == 0)
			return 0;
		else
			fprintf(dest, HEAD_SUBJECT " %s%s", configuration.subj_banned, therest);
	}
	else if (exe_count && *configuration.subj_exec)
	{
		if (strcmp(configuration.subj_exec, "#") == 0)
			return 0;
		else
			fprintf(dest, HEAD_SUBJECT " %s%s", configuration.subj_exec, therest);
	}
	else if (del_count && *configuration.subj_deleted)
	{
		if (strcmp(configuration.subj_deleted, "#") == 0)
			return 0;
		else
			fprintf(dest, HEAD_SUBJECT " %s%s", configuration.subj_deleted, therest);
	}
	else if (ren_count && *configuration.subj_renamed)
	{
		if (strcmp(configuration.subj_renamed, "#") == 0)
			return 0;
		else
			fprintf(dest, HEAD_SUBJECT " %s%s", configuration.subj_renamed, therest);
	}
	else if (filtcount && *configuration.add_subject)
	{
		if (strcmp(configuration.add_subject, "#") == 0)
			return 0;
		else
			fprintf(dest, HEAD_SUBJECT " %s%s", configuration.add_subject, therest);
	}
	else
		return 0;

	return 1;
}


/*
	SIGPIPE signal handler; clean up tempfile, exit
*/
void abnormalproc(int sig)
{
	if (tempfile)
		fclose(tempfile);
	fprintf(stderr, TXT_INFO_SIGPIPE "\n");
	exit(CODE_TEMPFAIL);	/* temporary failure */
}


/*
	Opens a write-pipe to indicated command, where args[0] is program
	childpid receives the child's process ID, required for closepipe()	
	
	Returns FILE* for writing to on success
	Returns NULL if pipe/fork/exec fails, or program terminates prematurely
*/
FILE* openpipe(char** args, pid_t* childpid)
{
	FILE* command;
	int cmdpipe[2];

	if (pipe(cmdpipe) != 0)
		return NULL;
	switch (*childpid = fork())
	{
		case -1:	/* error */
			close(cmdpipe[0]);
			close(cmdpipe[1]);
			return NULL;

		case 0:		/* child */
			if (dup2(cmdpipe[0], STDIN_FILENO) == -1)
				_exit(-1);
			close(cmdpipe[0]);
			close(cmdpipe[1]);
			execv(args[0], args);
			fprintf(stderr, TXT_ERR_EXEC ": %s\n", args[0]);
			_exit(-1);
	
		default:	/* parent */
			close(cmdpipe[0]);			/* close unused pipe end */
			command = fdopen(cmdpipe[1], "w");	/* open write-side of pipe */
			if (command)
				return command;
			else
			{
				close(cmdpipe[1]);
				return NULL;
			}
	}
}


/*
	Close the write-pipe opened earlier with openpipe()
	
	Returns 0 on normal termination, exitcode filled in
	Otherwise returns nonzero value, exitcode unmodified
*/
int closepipe(FILE* command, pid_t childpid, int* exitcode)
{
	int status;
	if (fclose(command) != 0)
	{
		waitpid(childpid, &status, 0);	/* cleanup any way */
		return -1;		/* error closing stream/pipe */
	}
	if (waitpid(childpid, &status, 0) == -1)
		return -1;		/* error waiting for child */
	if (WIFEXITED(status))
	{
		*exitcode = WEXITSTATUS(status);
		return 0;		/* terminated normally */
	}
	else
		return -1;		/* abnormal termination or nonzero exit code */
}
