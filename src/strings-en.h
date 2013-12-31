#define TXT_ERR_USAGE		"Usage: renattach [OPTIONS]\n\n"
#define TXT_HELP_HINT		"Try `renattach --help' for more options\n"
#define TXT_WARN_DEFAULTS	"Warning: using defaults in absence of "
#define TXT_ERR_CONFSYNTAX	"Configuration file syntax error, line "
#define TXT_ERR_MULTMODES	"You cannot specify multiple filter modes\n"
#define TXT_ERR_MULTACTIONS	"You cannot specify multiple filter actions\n"
#define TXT_ERR_EXEC		"Error executing pipe command"
#define TXT_ERR_OPENPIPE	"Error opening pipe command"
#define TXT_ERR_CLOSEPIPE	"Error closing pipe command"

#define TXT_INFO_UNDEF		"undefined"
#define TXT_INFO_MODE		"Filter mode: "
#define TXT_INFO_MODEALL	"all"
#define TXT_INFO_MODEBAD	"badlist"
#define TXT_INFO_MODEGOOD	"goodlist"
#define TXT_INFO_ACTION		"Filter action: "
#define TXT_INFO_CONFIGFILE	"Configuration file: "
#define TXT_INFO_OUTPUT		"Writing output to: "
#define TXT_INFO_STDOUT		"stdout"
#define TXT_INFO_PIPEXITCODE	"pipe command exitcode"
#define TXT_INFO_SIGPIPE	"Caught SIGPIPE; pipe stopped accepting data"
#define TXT_INFO_CONFOPTS	"Configuration directives:"
#define TXT_INFO_NOTACTING	"WARNING: Not acting on file"
#define TXT_INFO_CANTRENZIP	"Can't rename inside ZIP"


#define TXT_HEAD_MODE		"mode"
#define TXT_HEAD_ACTION		"action"
#define TXT_HEAD_FILTCOUNT	"count"
#define TXT_HEAD_FILTERED	"filtered"
#define TXT_ENCODEDBODY		"[encoded attachment body]"
#define TXT_DELIVEREDTO		"[forged Delivered-To header]"

#define TXT_HELP_A	"  -a, --all\n" \
			"	Filter mode: Match all attachments.\n\n"
#define TXT_HELP_B	"  -b, --badlist\n" \
			"	Filter mode: Only match filenames that have extensions listed on the\n" \
			"	bad-list. This will match only attachments with known dangerous file\n" \
			"	extensions (default).\n\n"
#define TXT_HELP_C	"  -c, --config filename\n" \
			"	Use the specified configuration file. Run renattach with --settings\n" \
			"	to verify current settings.\n\n"
#define TXT_HELP_D	"  -d, --delete\n" \
			"	Filter action: Delete attachment body after renaming headers.\n\n"
#define TXT_HELP_E	"  -e, --excode\n" \
			"	Extend exitcodes: 77=filtering occurred. This is in addition to the\n" \
			"	default codes: 0=success, 75=temporary failure, 255=critical failure\n\n"
#define TXT_HELP_G	"  -g, --goodlist\n" \
			"	Filter mode: Match all attachments except those that have extensions\n" \
			"	listed on the goodlist.\n\n"
#define TXT_HELP_H	"  -h, --help\n" \
			"	Show help, explain options.\n\n"
#define TXT_HELP_K	"  -k, --kill\n" \
			"	Filter action: Kill (absorb) entire email.\n\n"
#define TXT_HELP_L	"  -l, --loop\n" \
			"	Remove Delivered-To headers to prevent malicious mail forwarding loop.\n\n"
#define TXT_HELP_P	"  -p, --pipe command [args]\n" \
			"	Instead of writing output to stdout, open pipe to command (with args)\n" \
			"	and send output there. This program must return with exit code 0.\n" \
			"	This must be the last option on the command line.\n\n"
#define TXT_HELP_R	"  -r, --rename\n" \
			"	Filter action: Rename matching attachments (default).\n\n"
#define TXT_HELP_S	"  -s, --settings\n" \
			"	Show current settings/configuration and terminate.\n\n"
#define TXT_HELP_V1	"  -v, --verbose\n" \
			"	Write verbose output (including settings) to stderr.\n\n"
#define TXT_HELP_V2	"  -V, --version\n" \
			"	Display software version and terminate.\n\n"
