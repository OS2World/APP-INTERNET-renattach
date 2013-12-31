 renattach 1.2.4 - Filter that renames/deletes dangerous email attachments
 Copyright (C) 2003-2006  Jem E. Berkes

		Web site:	http://www.sysdesign.ca/
		Program page:	http://www.pc-tools.net/unix/renattach/
		Email contact:	support@pc-tools.net

	As per the GNU GPL, there is no warranty for this software.
	The author makes no guarantees as to software performance or
	effectiveness. renattach is NOT a virus scanner. Filtering
	is based on MIME headers and detectable filenames; as such,
	the software tries to handle both correct structures and
	incorrectly formatted messages. This filter will not catch
	all dangerous emails, particularly attachments embedded inside
	attachments.


************************************************************************
WARNING:
	THIS SOFTWARE HAS BEEN DISCONTINUED. IT IS NO LONGER MAINTAINED.
************************************************************************

	The author recommends that you do not depend upon renattach to
	filter emails for dangerous content. As of 2006, renattach used
	on its own is not enough to filter potentially harmful emails.
	Dangerous attachments, or other attacks, may pass through the
	filter undetected. Please switch from renattach to some other
	actively developed security system.
						Jem E. Berkes
						2006-03-19

************************************************************************


renattach is a fast and efficient UNIX stream filter that can rename or
delete potentially dangerous e-mail attachments. The filter is invoked
as a simple pipe for use in a wide variety of systems. The 'kill' feature
(which eliminates entire messages) can also help sites deal with resource
strains caused by modern virus floods.

renattach is written in pure C and can quickly process mail with little
overhead. Unlike a conventional virus scanner, there are no specific virus
or worm definitions. Instead, renattach identifies potentially dangerous
attachments based on file extension and executable encoded body content.
The software is even capable of reading filenames from inside ZIP archives
on the fly, without requiring any external software. The self-contained
MIME code parses, fully interprets, then rewrites the header of every
attached file. During this process it checks the file's extension against
a list, and further checks to make sure the filename is not on a banned
list. Only after passing through these steps is the MIME header written
fresh using a predetermined, known format.

The program's operation is simple: a single mail message is read from
stdin, filtered, then written to stdout (or piped to an external command).

Tested under Linux, FreeBSD, Solaris, Mac OS X, and Cygwin. This software
should compile on any UNIX-like system that has standard C libraries.


FEATURES
--------
* Fast, lightweight, little overhead
* Recognizes both MIME and uuencoded attachments
* Compliant with RFC2047 and RFC2231, handles encoded filenames
* Capable of reading filenames inside ZIP archives, on the fly
* Can rename or delete attachments, or kill entire messages
* Can detect executables that carry DOS/Windows signature
* Supports list of banned filenames (great for handling floods)
* Simple pipe/stream operation; can be used within many filtering systems
* Can be installed directly as a content_filter for Postfix MTA
* Can be installed as a local delivery agent for Sendmail MTA


renattach looks for its configuration file (renattach.conf) in the path
specified at compile time. Alternatively, you can specify the location of
renattach.conf by using the -c command-line options. For example:
renattach -c renattach.conf


COMMAND USAGE
-------------
Note that the filter's default behaviour is to rename dangerous attachments
that match the badlist {mode=badlist, action=rename}. If searching inside
ZIP archives for filenames (see the search_zip configuration option), the
only actions that modify the ZIP files are delete and kill but NOT rename.
Therefore the default rename action has no effect on ZIP files; instead, use
the --delete or --kill options. Alternatively, append the /d and /k switches
to badlist extensions in the .conf file to selectively delete or kill some
file types while just renaming the rest.

(See man page for more detail on some of these command-line options)

Usage: renattach [OPTIONS]

  -a, --all
        Filter mode: Match all attachments.

  -b, --badlist
        Filter mode: Only match filenames that have extensions listed on the
        bad-list. This will match only attachments with known dangerous file
        extensions (default).

  -c, --config filename
        Use the specified configuration file. Run renattach with --settings
        to verify current settings.

  -d, --delete
        Filter action: Delete attachment body after renaming headers.

  -e, --excode
        Extend exitcodes: 77=filtering occurred. This is in addition to the
        default codes: 0=success, 75=temporary failure, 255=critical failure

  -g, --goodlist
        Filter mode: Match all attachments except those that have extensions
        listed on the goodlist.

  -h, --help
        Show help, explain options.

  -k, --kill
        Filter action: Kill (absorb) entire email.

  -l, --loop
        Remove Delivered-To headers to prevent malicious mail forwarding loop.

  -p, --pipe command [args]
        Instead of writing output to stdout, open pipe to command (with args)
        and send output there. This program must return with exit code 0.
        This must be the last option on the command line.

  -r, --rename
        Filter action: Rename matching attachments (default).

  -s, --settings
        Show current settings/configuration and terminate.

  -v, --verbose
        Write verbose output (including settings) to stderr.

  -V, --version
        Display software version and terminate.


CONF FILE
---------
renattach reads its configuration options from renattach.conf, in your
$sysconfdir. There are defaults for all options but you will probably want
to tweak the configuration for your needs. The example configuration file
renattach.conf.ex fully describes all supported configuration directives
(in conf/ and copied to $sysconfdir by install).

Configuration directives are also described in the man page.


# Drop mail carrying executable attachments (DOS/Windows exec signature)
delete_exe = no
kill_exe = yes

# Search for filenames inside ZIP files
search_zip = yes

# Log filtered mail (delete, kill) to syslog mail facility
use_syslog = yes

# Delete winmail (MS proprietary) attachments without modifying Subject, 
# also drop emails containing annoying scanner-generated warning bounces
banned_files = /winmail/d, /warn.txt/k, DELETED0.TXT/k
subj_banned = #

subj_deleted = [deleted attachment]
subj_renamed = [renamed attachment]

# When these file types are encountered, rename the attachment (assuming
# filter is invoked with default action=rename). However, kill mail containing
# any BAT, COM, etc. attachments even if they are inside ZIP files. There is
# risk of collateral damage. EML//d means delete ZIPs that contain EML.
badlist = ADE, ADP, BAS, BAT/k, CHM, CMD/k, COM/k, CPL/k, CRT, EML//d, EXE/k
badlist = HLP, HTA/k, HTM, HTML, INF, INS, ISP, JS, JSE, LNK, MDB
badlist = MDE, MSC, MSH, MSI, MSP, MST, NWS, OCX, PCD, PIF/k, REG/k
badlist = SCR/k, SCT, SHB, SHS, URL, VB, VBE, VBS/k, WSC, WSF, WSH
