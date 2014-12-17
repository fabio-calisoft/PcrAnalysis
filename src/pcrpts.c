/*
 * pcrpts.c
 *
 *  Created on: June 8, 2013
 *      Author: Secure Digital Ddelivery
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "functions.h"
#include "video.h"

FILE *fp;

// main entry point
int main(int argc, char **argv) {

	char *INPUT_FILE_NAME;
    stream_info_struct video_info;
	/* initialization */
    int MAX_PCR_PTS_DELTA = 2;
    int exit_on_error=0;

	INPUT_FILE_NAME = malloc(255 * sizeof(char));



	if ( convert_input_params_to_vars(argc, argv, &INPUT_FILE_NAME, &MAX_PCR_PTS_DELTA,&exit_on_error) < 1)
    {
        printf("\n Missing information");
        exit(-1);
    }
    printf("\n Input file:%s", INPUT_FILE_NAME);

    video_info.video_pid = -1;
    video_info.PCR_pid = -1;
    video_info.file_bitrate_kbps = -1;

    extractStreamInfo(INPUT_FILE_NAME,&video_info);

    if (video_info.video_pid < 0) {
        printf("\n ERROR: no video pid found. Quit\n");
        return -1;
    }
    if (video_info.PCR_pid < 0) {
        printf("\n ERROR: no PCR pid found\n");
        return -1;
    }

    printf("\n video_pid = 0x%02x", video_info.video_pid);
    printf("\n audio_pid = 0x%02x", video_info.audio_pid);
    printf("\n pcr_pid = 0x%02x", video_info.PCR_pid);
    printf("\n file bitrate = %f bps", video_info.file_bitrate_kbps);
    printf("\n content duration = %d seconds", video_info.duration);
    printf("\n Max Pcr/Pts delta allowed = %d millseconds", MAX_PCR_PTS_DELTA);

    printPtsPcrDelta( INPUT_FILE_NAME, video_info.PCR_pid, MAX_PCR_PTS_DELTA,exit_on_error,video_info.file_bitrate_kbps);
    return 0;
}
