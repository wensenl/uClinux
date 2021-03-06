/*
 *  Copyright (C) 2007-2009 Sourcefire, Inc.
 *
 *  Authors: Tomasz Kojm, aCaB
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301, USA.
 */

#if HAVE_CONFIG_H
#include "clamav-config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <signal.h>

#include "shared/output.h"
#include "shared/misc.h"
#include "shared/optparser.h"
#include "shared/actions.h"

#include "client.h"

void help(void);

extern int printinfected;

static void print_server_version(const struct optstruct *opt)
{
    if(get_clamd_version(opt)) {
	/* can't get version from server, fallback */
	printf("ClamAV %s\n", get_version());
    }
}

int main(int argc, char **argv)
{
	int ds, dms, ret, infected;
	struct timeval t1, t2;
	struct timezone tz;
	time_t starttime;
        struct optstruct *opts;
        const struct optstruct *opt;
	struct sigaction sigact;


    if((opts = optparse(NULL, argc, argv, 1, OPT_CLAMDSCAN, OPT_CLAMSCAN, NULL)) == NULL) {
	mprintf("!Can't parse command line options\n");
	return 2;
    }

    if(optget(opts, "verbose")->enabled) {
	mprintf_verbose = 1;
	logg_verbose = 1;
    }

    if(optget(opts, "quiet")->enabled)
	mprintf_quiet = 1;

    if(optget(opts, "stdout")->enabled)
	mprintf_stdout = 1;

    if(optget(opts, "version")->enabled) {
	print_server_version(opts);
	optfree(opts);
	exit(0);
    }

    if(optget(opts, "help")->enabled) {
	optfree(opts);
    	help();
    }

    if(optget(opts, "infected")->enabled)
	printinfected = 1;

    /* initialize logger */

    if((opt = optget(opts, "log"))->enabled) {
	logg_file = opt->strarg;
	if(logg("--------------------------------------\n")) {
	    mprintf("!Problem with internal logger.\n");
	    optfree(opts);
	    exit(2);
	}
    } else 
	logg_file = NULL;


    if(optget(opts, "reload")->enabled) {
	ret = reload_clamd_database(opts);
	optfree(opts);
	logg_close();
	exit(ret);
    }

    if(actsetup(opts)) {
	optfree(opts);
	logg_close();
	exit(2);
    }

    memset(&sigact, 0, sizeof(struct sigaction));
    sigact.sa_handler = SIG_IGN;
    sigemptyset(&sigact.sa_mask);
    sigaddset(&sigact.sa_mask, SIGPIPE);
    sigaction(SIGPIPE, &sigact, NULL);

    time(&starttime);
    /* ctime() does \n, but I need it once more */

    gettimeofday(&t1, &tz);

    ret = client(opts, &infected);

    /* TODO: Implement STATUS in clamd */
    if((infected || ret != 2) && !optget(opts, "no-summary")->enabled) {
	gettimeofday(&t2, &tz);
	ds = t2.tv_sec - t1.tv_sec;
	dms = t2.tv_usec - t1.tv_usec;
	ds -= (dms < 0) ? (1):(0);
	dms += (dms < 0) ? (1000000):(0);
	logg("\n----------- SCAN SUMMARY -----------\n");
	logg("Infected files: %d\n", infected);
	if(notremoved) {
	    logg("Not removed: %d\n", notremoved);
	}
	if(notmoved) {
	    logg("Not moved: %d\n", notmoved);
	}
	logg("Time: %d.%3.3d sec (%d m %d s)\n", ds, dms/1000, ds/60, ds%60);
    }

    logg_close();
    optfree(opts);
    exit(ret);
}

void help(void)
{

    mprintf_stdout = 1;

    mprintf("\n");
    mprintf("                       ClamAV Daemon Client %s\n", get_version());
    printf("           By The ClamAV Team: http://www.clamav.net/team\n");
    printf("           (C) 2007-2009 Sourcefire, Inc.\n\n");

    mprintf("    --help              -h             Show help\n");
    mprintf("    --version           -V             Print version number and exit\n");
    mprintf("    --verbose           -v             Be verbose\n");
    mprintf("    --quiet                            Be quiet, only output error messages\n");
    mprintf("    --stdout                           Write to stdout instead of stderr\n");
    mprintf("                                       (this help is always written to stdout)\n");
    mprintf("    --log=FILE          -l FILE        Save scan report in FILE\n");
    mprintf("    --file-list=FILE    -f FILE        Scan files from FILE\n");
    mprintf("    --remove                           Remove infected files. Be careful!\n");
    mprintf("    --move=DIRECTORY                   Move infected files into DIRECTORY\n");
    mprintf("    --copy=DIRECTORY                   Copy infected files into DIRECTORY\n");
    mprintf("    --config-file=FILE                 Read configuration from FILE.\n");
    mprintf("    --multiscan           -m           Force MULTISCAN mode\n");
    mprintf("    --infected            -i           Only print infected files\n");
    mprintf("    --no-summary                       Disable summary at end of scanning\n");
    mprintf("    --reload                           Request clamd to reload virus database\n");
    mprintf("    --fdpass                           Pass filedescriptor to clamd (useful if clamd is running as a different user)\n");
    mprintf("    --stream                           Force streaming files to clamd (for debugging and unit testing)\n");
    mprintf("\n");

    exit(0);
}
