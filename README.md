adler32_cmp
===========

Compute the adler32 checksum of the given files and compare it to the value stored as extended attribute
(using StoRM's name by default).
It can be run interactively, in this case it prompts the user for actions to take when the stored value
is missing or differ from the computed one, or if the file is unreadable (I/O error).
When not in interactive mode, another switch turn on or off the automatic correction of the stored checksum
and the deletion of corrupted (unreadable) files.
