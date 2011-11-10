/*
 * (C) Copyright David Gibson <dwg@au1.ibm.com>, IBM Corporation.  2005.
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307
 *                                                                   USA
 */

#include "dtc.h"
#include "srcpos.h"
#include <getopt.h>

#include "version_gen.h"

#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif

/*
 * Command line options
 */
int quiet;		/* Level of quietness */
int reservenum;		/* Number of memory reservation slots */
int minsize;		/* Minimum blob size */
int padsize;		/* Additional padding to blob */

char *join_path(const char *path, const char *name)
{
	int lenp = strlen(path);
	int lenn = strlen(name);
	int len;
	int needslash = 1;
	char *str;

	len = lenp + lenn + 2;
	if ((lenp > 0) && (path[lenp-1] == '/')) {
		needslash = 0;
		len--;
	}

	str = xmalloc(len);
	memcpy(str, path, lenp);
	if (needslash) {
		str[lenp] = '/';
		lenp++;
	}
	memcpy(str+lenp, name, lenn+1);
	return str;
}

static void fill_fullpaths(struct node *tree, const char *prefix)
{
	struct node *child;
	const char *unit;

	tree->fullpath = join_path(prefix, tree->name);

	unit = strchr(tree->name, '@');
	if (unit)
		tree->basenamelen = unit - tree->name;
	else
		tree->basenamelen = strlen(tree->name);

	for_each_child(tree, child)
		fill_fullpaths(child, tree->fullpath);
}

static void  __attribute__ ((noreturn)) usage(int error)
{
        FILE *f = error ? stderr : stdout;
	fprintf(f, "Usage:\n");
	fprintf(f, "\tdtc [options] <input file>\n");
	fprintf(f, "\nOptions:\n");
	fprintf(f, "\t-h\n");
	fprintf(f, "\t\tThis help text\n");
	fprintf(f, "\t-q\n");
	fprintf(f, "\t\tQuiet: -q suppress warnings, -qq errors, -qqq all\n");
	fprintf(f, "\t-I <input format>\n");
	fprintf(f, "\t\tInput formats are:\n");
	fprintf(f, "\t\t\tdts - device tree source text\n");
	fprintf(f, "\t\t\tdtb - device tree blob\n");
	fprintf(f, "\t\t\tfs - /proc/device-tree style directory\n");
	fprintf(f, "\t-o <output file>\n");
	fprintf(f, "\t-O <output format>\n");
	fprintf(f, "\t\tOutput formats are:\n");
	fprintf(f, "\t\t\tdts - device tree source text\n");
	fprintf(f, "\t\t\tdtb - device tree blob\n");
	fprintf(f, "\t\t\tasm - assembler source\n");
	fprintf(f, "\t-V <output version>\n");
	fprintf(f, "\t\tBlob version to produce, defaults to %d (relevant for dtb\n\t\tand asm output only)\n", DEFAULT_FDT_VERSION);
	fprintf(f, "\t-R <number>\n");
	fprintf(f, "\t\tMake space for <number> reserve map entries (relevant for \n\t\tdtb and asm output only)\n");
	fprintf(f, "\t-S <bytes>\n");
	fprintf(f, "\t\tMake the blob at least <bytes> long (extra space)\n");
	fprintf(f, "\t-p <bytes>\n");
	fprintf(f, "\t\tAdd padding to the blob of <bytes> long (extra space)\n");
	fprintf(f, "\t-b <number>\n");
	fprintf(f, "\t\tSet the physical boot cpu\n");
	fprintf(f, "\t-f\n");
	fprintf(f, "\t\tForce - try to produce output even if the input tree has errors\n");
	fprintf(f, "\t-v\n");
	fprintf(f, "\t\tPrint DTC version and exit\n");
	fprintf(f, "\n");
	fprintf(f, "Report bugs to %s\n", BUG_URL);
	exit(error ? 3 : 0);
}

int main(int argc, char *argv[])
{
	struct boot_info *bi;
	const char *inform = "dts";
	const char *outform = "dts";
	const char *outname = "-";
	int force = 0, check = 0;
	const char *arg;
	int opt;
	FILE *outf = NULL;
	int outversion = DEFAULT_FDT_VERSION;
	long long cmdline_boot_cpuid = -1;
        static const struct option longopts[] = {
            {"help", no_argument, NULL, 'h'},
            {"version", no_argument, NULL, 'v'},
            {NULL, 0, NULL, 0}
        };

	quiet      = 0;
	reservenum = 0;
	minsize    = 0;
	padsize    = 0;

	while ((opt = getopt_long(argc, argv, "hI:O:o:V:R:S:p:fcqb:v",
                longopts, NULL)) != EOF) {
		switch (opt) {
		case 'I':
			inform = optarg;
			break;
		case 'O':
			outform = optarg;
			break;
		case 'o':
			outname = optarg;
			break;
		case 'V':
			outversion = strtol(optarg, NULL, 0);
			break;
		case 'R':
			reservenum = strtol(optarg, NULL, 0);
			break;
		case 'S':
			minsize = strtol(optarg, NULL, 0);
			break;
		case 'p':
			padsize = strtol(optarg, NULL, 0);
			break;
		case 'f':
			force = 1;
			break;
		case 'c':
			check = 1;
			break;
		case 'q':
			quiet++;
			break;
		case 'b':
			cmdline_boot_cpuid = strtoll(optarg, NULL, 0);
			break;
		case 'v':
			printf("Version: %s\n", DTC_VERSION);
			exit(0);
		case 'h':
			usage(0);
		default:
                        usage(1);
		}
	}

	if (argc > (optind+1))
		usage(1);
	else if (argc < (optind+1))
		arg = "-";
	else
		arg = argv[optind];

	/* minsize and padsize are mutually exclusive */
	if (minsize && padsize)
		die("Can't set both -p and -S\n");

	fprintf(stderr, "DTC: %s->%s  on file \"%s\"\n",
		inform, outform, arg);

	if (streq(inform, "dts"))
		bi = dt_from_source(arg);
	else if (streq(inform, "fs"))
		bi = dt_from_fs(arg);
	else if(streq(inform, "dtb"))
		bi = dt_from_blob(arg);
	else
		die("Unknown input format \"%s\"\n", inform);

	if (cmdline_boot_cpuid != -1)
		bi->boot_cpuid_phys = cmdline_boot_cpuid;

	fill_fullpaths(bi->dt, "");
	process_checks(force, bi);


	if (streq(outname, "-")) {
		outf = stdout;
	} else {
		outf = fopen(outname, "w");
		if (! outf)
			die("Couldn't open output file %s: %s\n",
			    outname, strerror(errno));
	}

	if (streq(outform, "dts")) {
		dt_to_source(outf, bi);
	} else if (streq(outform, "dtb")) {
#ifdef _WIN32
                _setmode(_fileno(outf), _O_BINARY);
#endif
		dt_to_blob(outf, bi, outversion);
	} else if (streq(outform, "asm")) {
		dt_to_asm(outf, bi, outversion);
	} else if (streq(outform, "null")) {
		/* do nothing */
	} else {
		die("Unknown output format \"%s\"\n", outform);
	}

	exit(0);
}
