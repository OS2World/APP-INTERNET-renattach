
What is this?
=============

This is the OS/2 port of renattach version 1.2.4, a utility that renames
or removes attachments in email based on rules, file extensions, and
such. The tool can be incorporated nicely into a procmail rule to strip
out all the MS virus stuff floating around on the net before it even
hits a spam filter such as bogofilter. This works even inside zip files,
if so configured in the configuration file. For more information, see
the file README, the included man page, and the home page of renattach.


How was it built?
==================

The port was built on ecomstation ECS 1.2 (based on IBM OS/2 Warp 4.52)
after some minor OS/2 specific patches to the source code and Makefiles.
The patches are recorded in the *.diff files found in the subdirectories
src/ and conf/; the original source files have been renamed to *.orig.

The most important modification is that support for the syslog facility
has not been compiled in, as I do not have it running on this system,
nor do I have the needed header files. If you need that you will have to
recompile. For a description of the changes, please consult the file
build_notes_os2_1.2.4.txt

The build was made with the OS/2 port of gcc 2.8.1 (emx0.9d + emxfix4)
and will need the corresponding emxrt.dll. The executable is stripped to
save space (gcc -s -O2).


How to install it
=================

1. Go to the subdirectory src/ 
2. Copy the executable renattach.exe to a directory in your path. If you
   like, you can compress it with LXLITE and save another 32 kB or so on
   disk (I did that).
3. In case you have a working man setup to read UNIXish manual pages,
   copy the file renattach.1 to a subdirectory man1 in your MANPATH.
   Then read it. (In case you have a compressed copy of a previous
   version in your cat1 directory, remove that one. Otherwise, man keeps
   displaying that one.)
4. Go to the subdirectory conf/
5. Copy the file renattach.conf.ex to a location you see fit, such as
   %ETC%, ~/.renattachrc (or wherever you keep your configuration files)
   and edit it to fit your needs.
   You will tell renattach with the -c flag to use it. The standard
   location /usr/local/etc is fine on UNIX and relatives, but has no
   clear equivalent on OS/2.
6. Now you can play with it, adapt your procmail rules, and keep the
   flood of attachments at gate.


What else
=========

This port works here and is in productive use. No guaranty nor warranty
of any kind is given. If it kills your cat, blows up your fridge, or
reformats your toaster you've been warned.


Have fun!  Cheers, Stefan   (sad@mailaps.org)             28 October 2006
