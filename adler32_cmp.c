/*
 * file:   adler32_cmp.c
 * author: david.rebatto@mi.infn.it
 * date:   19 Jan 2012
 *
 * description:
 *  use zlib to compute adler32 checksum of a file and
 *  compare it to the one stored by StoRM.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <attr/xattr.h>
#include <fcntl.h>
#include <errno.h>
#include <getopt.h>
#include <zlib.h>
#include <string.h>
#include <regex.h>

/* A GPFS block in our Tier2's file system */
#define BUFSIZE 262144
/* Hex representation of an uLong plus trailing 0 */
#define ADLSIZE 17

/* Differentiate exit codes to help shell scripting */
#define ERROR_MISSING_FILE 2
#define ERROR_IOERR        3
#define ERROR_BADCHKSUM    4

int
get_adler32(const int in_file, uLong *computed)
{
    char buffer[BUFSIZE];
    ssize_t length;
    uLong adler = adler32(0L, Z_NULL, 0);

    while ((length = read(in_file, buffer, BUFSIZE)) > 0) {
        adler = adler32(adler, buffer, length);
    }

    if (length == -1) {
        return(-1);
    }
    *computed = adler;

    return(0);
}

int
main(int argc, char **argv)
{
    char opt;
    int verbose = 0;
    int interactive = 0;
    int setchecksum = 0;
    int deletebad = 0;
    char *attrname = "user.storm.checksum.adler32";
    char *filename;
    int exitcode = EXIT_SUCCESS;
    char answ;
    int in_file;
    char saved[ADLSIZE];
    ssize_t attrlen;
    uLong computed;
    uLong saved_int;
    regex_t splitpath;
    regmatch_t matchptr[3];
    char *i;

    if (regcomp(&splitpath, "\\([^/]*\\)/\\([^/]*\\)$", 0)) {
        perror("Cannot compile regex");
        exit(EXIT_FAILURE);
    }

    while((opt = getopt(argc, argv, "vicdn:")) != -1) {
        switch (opt) {
        case 'v':
            /* Produce more output (namely, both checksums are printed) */
            verbose = 1;
            break;
        case 'i':
            /* Interactively prompt for file deletion */
            interactive = 1;
            verbose = 1;
            break;
        case 'c':
            /* Set/replace the stored checksum with the computed one */
            setchecksum = 1;
            break;
        case 'd':
            /* Delete unreadable files */
            deletebad = 1;
            break;
        case 'n':
            /* Use a different checksum attribute name */
            attrname = optarg;
            break;
        case '?':
            optind = argc;
        }
    }

    if (optind == argc) {
        fprintf(stderr, "Usage: %s [-v | -i] [-c] [-d] [-n <attribute_name>] <file> ...\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    for (filename = argv[optind]; optind < argc; filename = argv[++optind]) {
        if (verbose) printf("Examining %s\n", filename);

        /* Open the file */
        if ((in_file = open(filename, O_RDONLY)) == -1) {
            exitcode = (errno == ENOENT) ? ERROR_MISSING_FILE : EXIT_FAILURE ;
            if (verbose) {
                printf("Error opening file: %s\n", strerror(errno));
            } else {
                fprintf(stderr, "Error opening file %s: ", filename);
                perror("");
            }
            continue;
        }

        /* Compute the adler32 for the file on disk */
        if (get_adler32(in_file, &computed)) {
            exitcode = (errno == EIO) ? ERROR_IOERR : EXIT_FAILURE;
            if (errno == EIO) {
                if (interactive) {
                    answ = 'n';
                    fprintf(stderr, "I/O error: remove the file? [y/N] "); 
                    answ = getchar();
                }
                if (deletebad || answ == 'y' || answ == 'Y') {
                    unlink(filename);
                }
            } else if ((errno == EISDIR) && verbose) {
                printf("It's a directory, skipping\n");
            } else {
                fprintf(stderr, "Error reading file %s: ", filename);
                perror("");
            }
            close(in_file);
            continue;
        } else {
            if (verbose) printf("Computed checksum: %x\n", computed);
        } 

        /* Read the adler32 written by StoRM when the file was replicated */
        if ((attrlen = fgetxattr(in_file, attrname, (void *)saved, ADLSIZE)) == -1) {
            if ((errno == ENOATTR) && (interactive || setchecksum)) {
                if (interactive) {
                    printf("No saved checksum: "); 
                    if (regexec(&splitpath, filename, 3, matchptr, 0) == 0) {
                        printf("you might want to try the following command from a UI:\n"); 
                        printf("dq2-list-files ");
                        for (i = filename+matchptr[1].rm_so; i < filename+matchptr[1].rm_eo; i++)
                            printf("%c", *i);
                        printf(" | grep %s\n", filename+matchptr[2].rm_so);
                    }
                    printf("set the checksum to the computed value (%x)? [y/N] ", computed); 
                    answ = getchar();
                }
                if ((answ == 'y') || setchecksum) {
                    snprintf(saved, ADLSIZE, "%x", computed);
                    if (fsetxattr(in_file, attrname, (const void *)saved, strlen(saved), 0) != 0) {
                      perror("fsetxattr()");
                    }
                }
            } else if (verbose) {
                printf("error getting saved checksum: %s\n", strerror(errno));
            } else {
                fprintf(stderr, "Error getting saved checksum for file %s: ", filename);
                perror("");
            }
            close(in_file);
            exitcode = EXIT_FAILURE;
            continue;
        } else {
            saved[attrlen] = '\000';
            if (verbose) printf("Saved checksum: %s\n", saved);
        }

        /* Compare the adler32 values */
        if (!verbose) printf("%s - ", filename);
        sscanf(saved, "%x", &saved_int);
        if (computed ==  saved_int) {
            printf("Checksum verified\n");
        } else {
            printf("Checksum mismatch!\n");
            exitcode = ERROR_BADCHKSUM;
        }
        close(in_file);
    }
    exit(exitcode);
}

