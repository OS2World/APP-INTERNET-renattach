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

*/

#include "config.h"
#include "renattach.h"
#include "strings-en.h"
#include "utility.h"
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef HAVE_STRCASECMP
#define strcasecmp stricmp
#endif

#ifndef HAVE_STRNCASECMP
#define strncasecmp strnicmp
#endif



/*
	Iterates through the list and returns the number of entries within.
	If verbose is specified, each entry is displayed.
*/
int tokenize_list(const char* thelist, int verbose)
{
	char *tmpbuf, *token;
	int count=0;

	tmpbuf = malloc(strlen(thelist) + 1);
	strcpy(tmpbuf, thelist);
	token = strtok(tmpbuf, LIST_TOKENS);
	while (token)
	{
		if (verbose)
		{
			if (count)
				fprintf(stderr, "|%s", token);
			else
				fprintf(stderr, "%s", token);
		}
		count++;
		token = strtok(NULL, LIST_TOKENS);
	}
	free(tmpbuf);
	return count;
}


/*
	Parse the configuration file and fill in the options structure
	Any errors found are written to stderr. Returns CODE_OK or CODE_FAILURE
*/
int parse_conf(struct config_opts* options)
{
	int linecount = 0;
	char confline[MAXFIELD];
	char directive[MAXFIELD], parameter[MAXFIELD];
	FILE* config = NULL;

	if (options->config_file == NULL)
	{
		fprintf(stderr, TXT_INFO_CONFIGFILE TXT_INFO_UNDEF "\n");
		return CODE_FAILURE;
	}

	config = fopen(options->config_file, "r");
	if (!config)
	{
		perror(options->config_file);
		return CODE_FAILURE;
	}

	while (fgets(confline, sizeof(confline), config))
	{
		int syntax_error = 0;
		linecount++;

		trim_leading(confline);
		trim_trailing(confline);

		if ((*confline == '#') || (*confline == '\r') || (*confline == '\n'))
			continue;
		if (sscanf(confline, "%[^\t =]%*[\t =]%[^\r\n]", directive, parameter) == 2)
		{
			int state = OPTION_UNSET;
			if ( (strncasecmp(parameter, TAG_OPTENABLED, strlen(TAG_OPTENABLED))==0)
				|| (*parameter=='1'))	/* 'yes' or 1 */
				state = OPTION_ENABLED;
			else if ( (strncasecmp(parameter, TAG_OPTDISABLED, strlen(TAG_OPTDISABLED))==0)
				|| (*parameter=='0'))	/* 'no' or 0 */
				state = OPTION_DISABLED;

			if ((strcasecmp(directive, TAG_DELETE_EXE)==0) && state)
				options->delete_exe = state;
			else if ((strcasecmp(directive, TAG_KILL_EXE)==0) && state)
				options->kill_exe = state;
			else if ((strcasecmp(directive, TAG_SEARCH_ZIP)==0) && state)
				options->search_zip = state;
			else if ((strcasecmp(directive, TAG_PASS_CONTENTID)==0) && state)
				options->pass_contentid = state;
			else if ((strcasecmp(directive, TAG_FULL_RENAME)==0) && state)
				options->full_rename = state;
			else if ((strcasecmp(directive, TAG_USE_SYSLOG)==0) && state)
				options->use_syslog = state;
			else if (strcasecmp(directive, TAG_GENERIC_NAME) == 0)
				strcpy(options->generic_name, parameter);
			else if (strcasecmp(directive, TAG_NEW_EXTENSION) == 0)
				strcpy(options->new_extension, parameter);
			else if (strcasecmp(directive, TAG_NEW_MIME_TYPE) == 0)
				strcpy(options->new_mime_type, parameter);
			else if (strcasecmp(directive, TAG_SUBJ_BANNED) == 0)
				strcpy(options->subj_banned, parameter);
			else if (strcasecmp(directive, TAG_SUBJ_EXEC) == 0)
				strcpy(options->subj_exec, parameter);
			else if (strcasecmp(directive, TAG_SUBJ_DELETED) == 0)
				strcpy(options->subj_deleted, parameter);
			else if (strcasecmp(directive, TAG_SUBJ_RENAMED) == 0)
				strcpy(options->subj_renamed, parameter);
			else if (strcasecmp(directive, TAG_ADD_SUBJECT) == 0)
				strcpy(options->add_subject, parameter);
			else if (strcasecmp(directive, TAG_HTMLWARN_POS) == 0)
				strcpy(options->htmlwarn_pos, parameter);
			else if (strcasecmp(directive, TAG_WARNING_TEXT) == 0)
				expand_list(&options->warning_text, parameter, "\n");
			else if (strcasecmp(directive, TAG_WARNING_HTML) == 0)
				expand_list(&options->warning_html, parameter, "\n");
			else if (strcasecmp(directive, TAG_ADD_HEADER) == 0)
				expand_list(&options->add_header, parameter, "\n");
			else if (strcasecmp(directive, TAG_BANNED_FILES) == 0)
				expand_list(&options->banned_files, parameter, "\n");
			else if (strcasecmp(directive, TAG_BADLIST) == 0)
				expand_list(&options->badlist, parameter, "\n");
			else if (strcasecmp(directive, TAG_GOODLIST) == 0)
				expand_list(&options->goodlist, parameter, "\n");
			else
				syntax_error = 1;
		}
		else
			syntax_error = 1;

		if (syntax_error)
		{
			fprintf(stderr, TXT_ERR_CONFSYNTAX "%d: %s\n", linecount, confline);
			fclose(config);
			return CODE_FAILURE;
		}
	}

	fclose(config);
	return CODE_OK;
}


/*
	Check all configuration directives
	If any are unset, use defaults
*/
void directive_defaults(struct config_opts* options)
{
	if (options->delete_exe == OPTION_UNSET)
		options->delete_exe = DEF_DELETE_EXE;
	if (options->kill_exe == OPTION_UNSET)
		options->kill_exe = DEF_KILL_EXE;
	if (options->search_zip == OPTION_UNSET)
		options->search_zip = DEF_SEARCH_ZIP;
	if (options->pass_contentid == OPTION_UNSET)
		options->pass_contentid = DEF_PASS_CONTENTID;
	if (options->full_rename == OPTION_UNSET)
		options->full_rename = DEF_FULL_RENAME;
	if (options->use_syslog == OPTION_UNSET)
		options->use_syslog = DEF_USE_SYSLOG;
	if (options->generic_name[0] == '\0')
		strcpy(options->generic_name, DEF_GENERIC_NAME);
	if (options->new_extension[0] == '\0')
		strcpy(options->new_extension, DEF_NEW_EXTENSION);
	if (options->new_mime_type[0] == '\0')
		strcpy(options->new_mime_type, DEF_NEW_MIME_TYPE);
	if (options->subj_banned[0] == '\0')
		strcpy(options->subj_banned, DEF_SUBJ_BANNED);
	if (options->subj_exec[0] == '\0')
		strcpy(options->subj_exec, DEF_SUBJ_EXEC);
	if (options->subj_deleted[0] == '\0')
		strcpy(options->subj_deleted, DEF_SUBJ_DELETED);
	if (options->subj_renamed[0] == '\0')
		strcpy(options->subj_renamed, DEF_SUBJ_RENAMED);
	if (options->add_subject[0] == '\0')
		strcpy(options->add_subject, DEF_ADD_SUBJECT);
	if (options->htmlwarn_pos[0] == '\0')
		strcpy(options->htmlwarn_pos, DEF_HTMLWARN_POS);
	if ( (options->warning_text == NULL) || (options->warning_text[0] == '\0'))
		expand_list(&options->warning_text, DEF_WARNING_TEXT, "\n");
	if ( (options->warning_html == NULL) || (options->warning_html[0] == '\0'))
		expand_list(&options->warning_html, DEF_WARNING_HTML, "\n");
	if ( (options->add_header == NULL) || (options->add_header[0] == '\0'))
		expand_list(&options->add_header, DEF_ADD_HEADER, "\n");
	if ( (options->banned_files == NULL) || (tokenize_list(options->banned_files, 0)==0) )
		expand_list(&options->banned_files, DEF_BANNED_FILES, "\n");
	if ( (options->goodlist == NULL) || (tokenize_list(options->goodlist, 0)==0) )
		expand_list(&options->goodlist, DEF_GOODLIST, "\n");
	if ( (options->badlist == NULL) || (tokenize_list(options->badlist, 0)==0) )
		expand_list(&options->badlist, DEF_BADLIST, "\n");
}


/*
	Show current configuration; used in verbose mode
*/
void show_configuration(struct config_opts* options)
{
	/* Where is our configuration file? */
	fprintf(stderr, TXT_INFO_CONFIGFILE "\n  ");
	if (options->config_file)
		fprintf(stderr, "%s\n", options->config_file);
	else
		fprintf(stderr, TXT_INFO_UNDEF "\n");

	fprintf(stderr, TXT_INFO_OUTPUT "\n  ");
	if (options->pipe_cmd)
	{
		int arg = 0;
		fprintf(stderr, "{");
		while (options->pipe_cmd[arg])
		{
			if (arg) fprintf(stderr, "|");
			fprintf(stderr, "%s", options->pipe_cmd[arg++]);
		}
		fprintf(stderr, "}\n");
	}
	else
		fprintf(stderr, TXT_INFO_STDOUT "\n");

	/* What filtering mode are we using? */
	fprintf(stderr, TXT_INFO_MODE "\n  %s\n", mode2str(options->mode));

	/* What filtering action are we using? */
	fprintf(stderr, TXT_INFO_ACTION "\n  %s\n", act2str(options->action));

	fprintf(stderr, TXT_INFO_CONFOPTS "\n");
	/* Show status of all settings from renattach.conf */
	fprintf(stderr, "  "TAG_DELETE_EXE ": %s\n", opt2str(options->delete_exe));
	fprintf(stderr, "  "TAG_KILL_EXE ": %s\n", opt2str(options->kill_exe));
	fprintf(stderr, "  "TAG_SEARCH_ZIP ": %s\n", opt2str(options->search_zip));
	fprintf(stderr, "  "TAG_PASS_CONTENTID ": %s\n", opt2str(options->pass_contentid));
	fprintf(stderr, "  "TAG_FULL_RENAME ": %s\n", opt2str(options->full_rename));
	fprintf(stderr, "  "TAG_USE_SYSLOG ": %s\n", opt2str(options->use_syslog));
	fprintf(stderr, "  "TAG_GENERIC_NAME ": \"%s\"\n", options->generic_name);
	fprintf(stderr, "  "TAG_NEW_EXTENSION ": \"%s\"\n", options->new_extension);
	fprintf(stderr, "  "TAG_NEW_MIME_TYPE ": \"%s\"\n", options->new_mime_type);
	fprintf(stderr, "  "TAG_SUBJ_BANNED ": \"%s\"\n", options->subj_banned);
	fprintf(stderr, "  "TAG_SUBJ_EXEC ": \"%s\"\n", options->subj_exec);
	fprintf(stderr, "  "TAG_SUBJ_DELETED ": \"%s\"\n", options->subj_deleted);
	fprintf(stderr, "  "TAG_SUBJ_RENAMED ": \"%s\"\n", options->subj_renamed);
	fprintf(stderr, "  "TAG_ADD_SUBJECT ": \"%s\"\n", options->add_subject);
	fprintf(stderr, "  "TAG_HTMLWARN_POS ": \"%s\"\n", options->htmlwarn_pos);
	fprintf(stderr, "  "TAG_WARNING_TEXT ": %d bytes,\n%s\n",
		(int)strlen(options->warning_text), options->warning_text);
	fprintf(stderr, "  "TAG_WARNING_HTML ": %d bytes,\n%s\n",
		(int)strlen(options->warning_html), options->warning_html);
	fprintf(stderr, "  "TAG_ADD_HEADER ": %d bytes,\n%s\n",
		(int)strlen(options->add_header), options->add_header);
	fprintf(stderr, "  "TAG_BANNED_FILES ": {");
	tokenize_list(options->banned_files, 1);
	fprintf(stderr, "}\n  "TAG_BADLIST ": {");
	tokenize_list(options->badlist, 1);
	fprintf(stderr, "}\n  " TAG_GOODLIST ": {");
	tokenize_list(options->goodlist, 1);
	fprintf(stderr, "}\n");
}


/*
	Return description of directive state
*/
const char* opt2str(int state)
{
	if (state == OPTION_DISABLED)
		return TAG_OPTDISABLED;
	else if (state == OPTION_ENABLED)
		return TAG_OPTENABLED;
	else
		return TXT_INFO_UNDEF;
}


/*
	Return description of filter mode
*/
const char* mode2str(int mode)
{
	if (mode == MODE_ALL)
		return TXT_INFO_MODEALL;
	else if (mode == MODE_BAD)
		return TXT_INFO_MODEBAD;
	else if (mode == MODE_GOOD)
		return TXT_INFO_MODEGOOD;
	else
		return TXT_INFO_UNDEF;
}


/*
	Return description of action type
*/
const char* act2str(int action)
{
	switch (action)
	{
		case ACTION_RENAME:
			return TAG_ACTRENAME;

		case ACTION_DELETE:
			return TAG_ACTDELETE;

		case ACTION_KILL:
			return TAG_ACTKILL;

		case ACTION_SKIP:
			return TAG_ACTSKIP;

		default:
			return TXT_INFO_UNDEF;
	}
}
