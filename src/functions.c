/*
 * functions.c
 *
 *  Created on: June 8, 2013
 *      Author: Secure Digital Ddelivery
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "functions.h"

char msglog[200];
int verbosity_level;

void print_menu(char *this_program_name) {
	printf("\nUsage:");
	printf("%s -i [Input File Name] -l [log verbosity] -m [max pcr-pts delta(millseconds)] [-e 1 exit on error]", this_program_name);
	printf("\nlog verbosity: 0.DEBUG,1.VERBOSE,2.INFO,3.WARN,4.ERROR");
	printf("\n");
}

void convert_Hex_string_to_uchar(char *myarg, unsigned char **uchar) {

	char *str = malloc(2 * sizeof(char));
	int i = 0, j = 0, value;
	char *p;
	for (; i < 2 * (16); i++) {
		str[0] = myarg[0 + i];
		str[1] = myarg[1 + i];
		value = strtoul(str, &p, 16);
		(*uchar)[j++] = value;
//		printf("\n-->%d", value);
		i++;
	}
}

int convert_input_params_to_vars(int argc, char **argv, char **TS_FILE, int *max_pcr_pts_delta,int *exit_on_error) {
	int c;
    int found=0;
	while ((c = getopt(argc, argv, "i:l:m:e:x")) != -1)
		switch (c) {
		case 'i':
			*TS_FILE = strdup(optarg);
            found=1;
			break;
		case 'l':
            verbosity_level=atoi( optarg );
            sprintf(msglog,"verbosity=%d",verbosity_level);
            Log(DEBUG,msglog);
			break;
        case 'm': // MAX_PCR_PTS_DELTA
            *max_pcr_pts_delta=atoi( optarg );
            sprintf(msglog,"MAX_PCR_PTS_DELTA=%s",optarg);
            Log(DEBUG,msglog);
            break;
        case 'e': // exit on error
            *exit_on_error=1;
            break;
		case '?':
			print_menu(argv[0]);
			break;
		default:
			abort();
		}
    if (!found) {
        print_menu(argv[0]);
        return -1;
    }
    else {
        return 1;
    }

}

// a simple hex-print routine. could be modified to print 16 bytes-per-line
void hex_print(const unsigned char *pv, int len) {

	if (NULL == pv)
		printf("NULL");
	else {
		int i = 0;
		int x = 0;
		printf("\n.0..1..2..3  | ..4..5..6..7\n");
		for (; i < len; ++i) {
			if (x == 0) {
				printf("\n%d..", i);
			}
			printf("%02x ", pv[i]);
			if (x == 3)
				printf(" | ");
			if (x == 7) {
				x = 0;
//				printf("\n");
			} else
				x++;
		}
	}
	printf("\n");
}

int loadFile(char *path, unsigned char **buffer) {
	FILE *fp; /*filepointer*/
	int size; /*filesize*/


	// -------- Open the File
	fp = fopen(path, "rb");
	if (fp == NULL) { /*ERROR detection if file == empty*/
		printf("loadFile Error: There was an Error opening the file %s \n", path);
		exit(1);
	}
	fseek(fp, 0, SEEK_END);
	size = ftell(fp); /*calc the size needed*/
	*buffer = malloc(size * sizeof(char)); /*allocalte space on heap*/
	// -------- Open the File



	int bytes_read = fread(*buffer, sizeof(char), size, fp);
	if (bytes_read != size) { /* if count of read bytes != calculated size of .bin file -> ERROR*/
		printf("loadFile Error: There was an Error reading the file %s size=%d\n", path,
				size);
		return -1;
	}
	return size;
}

int saveFile(char *path, unsigned char *buffer, int size) {
	FILE *fp; /*filepointer*/

	fp = fopen(path, "wb"); /*open file*/
	fseek(fp, 0, SEEK_SET);
	if (fp == NULL) { /*ERROR detection if file == empty*/
		printf("1 Error: There was an Error opening for W the file %s \n",
				path);
		return -1;
	}
	int res = fwrite(buffer, size, sizeof(char), fp);
	return res;
}

int readData(FILE *fp, unsigned char **buffer, unsigned int size_to_read) {
	unsigned int bytes_read = fread(*buffer, sizeof(char), size_to_read, fp);
	return bytes_read;
}

void Log(LEVEL verbosity , char * msg) {
    if (verbosity >= verbosity_level) {
        printf("\n(%d): %s",verbosity, msg);
    }
}


