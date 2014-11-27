adler32_cmp
===========

Compute the adler32 checksum of the given files and compare it to the value
stored as extended attribute (using StoRM's name `user.storm.checksum.adler32`
by default).

When run interactively (with the `-i` switch), it prompts the user for actions
to take when the stored value is missing or differ from the computed one, or
if the file is unreadable (I/O error).

When not in interactive mode, the command line switch `-c` turns on or off the
automatic correction of the stored checksum. No action is taken on unreadable
files, they are just listed on stderr.
