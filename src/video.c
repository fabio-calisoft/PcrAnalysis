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
#include <time.h>
#include <inttypes.h>
#include <sys/time.h>
#include "functions.h"
#include "video.h"


char msglog[255];


int printPtsPcrDelta(char *filename, int pcr_pid, int MAX_PCR_PTS_DELTA,int exit_on_error, int bitrate) {
	int pid,time_start=0,time_end,bytes_read=0;
	double PCR_seconds;
	FILE *fp;
	unsigned char PES_HEADER[3] = {0x00,0x00,0x01};

	unsigned char *ts_packet,*adaptation_field;
	unsigned char *pes_packet;
	fp = fopen(filename, "rb");
	sprintf(msglog,                
			"extractVideoPid Error: There was an Error opening for R the file %s",
			filename);

	if (fp == NULL) {
		Log(ERROR,msglog);
		return -1;
	}

	if (bitrate == 0) {
		Log(ERROR,"stream bitrate is 0");
		return -1;
	}
	double byterate = bitrate/8;
	sprintf(msglog,"stream BYTErate is %f",byterate);
	Log(DEBUG,msglog);

	Log(DEBUG,"-----printPtsPcrDelta----");

	ts_packet = malloc(TS_PACKET_LEN * sizeof(char));

	int index =0;
	int last_PCR_packet_number=-1;
	double max_delta_pcr_pts = 0;

	while (!feof(fp) ) {
		sprintf ( msglog, "----->packet n. %d",index);
		Log( VERBOSE, msglog );
		index++;
		bytes_read = fread(ts_packet, sizeof(char), TS_PACKET_LEN, fp);


		// 1) check we have the sync byte
		if (bytes_read == TS_PACKET_LEN && ts_packet[0] == 0x47) {
			// 2) check it's a video packet
			pid = ts_packet[2] + ((ts_packet[+1] & 0x1F) << 8);

			if (pid == pcr_pid) {

				int AFC_len = 0;

				// 4a) Check the TS packet has Adaptation Field B[3]&0x20 == 1.
				if ((ts_packet[3] & 0x20) == 0x20) {
					Log(DEBUG,"Found AFC");
					adaptation_field = (ts_packet+4);

					// 4b) Se ha AFC, calcolo la sua lunghezza
					// AFC_len=B[4] altrimenti AFC_len=0
					AFC_len = *(adaptation_field);
					sprintf(msglog,"AFC len=%d", AFC_len);
					Log(DEBUG,msglog);
					// check if adaptation field contains a PCR field
					if ( AFC_len > 0 &&  ( *(adaptation_field+1) & 0x10) == 0x10) {
						sprintf(msglog,"Found PCR. packet number %d",index);
						Log(DEBUG,msglog);
						unsigned char *pcr_pointer = (adaptation_field+2);
						//hex_print(pcr_pointer, 6); // 33+6+9
						double previous_PCR_seconds = PCR_seconds;
						PCR_seconds = get_pcr_time(pcr_pointer);
						last_PCR_packet_number = index;
						double PCR_jitter_ns = (PCR_seconds-previous_PCR_seconds) *  1000;
						sprintf(msglog,"PCR time=%f  jitter=%fns",PCR_seconds,PCR_jitter_ns );
						Log(DEBUG,msglog);
					}
					else
						Log(DEBUG,"AFC not present or no PCR present ");
					if ( (*(adaptation_field+1) & 0x08) == 0x08) {
						Log(DEBUG,"Found (O)PCR");
					}

				} else {
					Log(DEBUG,"AFC not Found");
				}


				// 4b) Check the TS packet has PAYLOAD B[3]&0x10 == 1.
				if ((ts_packet[3] & 0x10) == 0x10) {
					Log(DEBUG,"Found payload:");


					pes_packet = (ts_packet+4) + AFC_len;
					// hex_print( pes_packet ,15);
					if ( memcmp(pes_packet, PES_HEADER, 3)==0 ) {
						// see http://dvd.sourceforge.net/dvdinfo/pes-hdr.html
						Log(DEBUG,"Found PES");
						unsigned char stream_id = *(pes_packet+3);
						if ( (stream_id & 0xE0) == 0xE0 ) {
							unsigned char pes_flags = *(pes_packet+7);
							sprintf(msglog,"pes flags=0x%02x",pes_flags);
							Log(DEBUG,msglog);
							unsigned char *pts_dts = (pes_packet+9);
							double pts_time = 0;
							if ( (pes_flags & 0xC0) == 0x80 ) {
								// PTS/DTS present
								Log(DEBUG,"PTS/DTS present [10]:");
								//hex_print( pts_dts ,5);
								if ( ( (*pts_dts) & 0xF0) != 0x20 ) {
									Log(ERROR,"---ERROR:: expected PTS !!!");
									fclose(fp);
									return -1;
								}
								pts_time = get_pts_time(pts_dts);
								sprintf(msglog,"PTS=%f",pts_time);
								Log(DEBUG,msglog);
							}
							else if ( (pes_flags & 0xC0) == 0xC0 ) {
								// PTS/DTS present
								Log(DEBUG,"PTS/DTS present [11]:");
								//hex_print(  pts_dts ,5);
								if ( ( (*pts_dts) & 0xF0) == 0x30 ) {
									Log(DEBUG,"found PTS");

									pts_time = get_pts_time(pts_dts);
									sprintf(msglog,"PTS=%f",pts_time);
									Log(DEBUG,msglog);
								}
								else if ( ( (*pts_dts) & 0x30) != 0x10 ) {
									Log(DEBUG,"found DTS");
								}
							}

							int delta_packets = index - last_PCR_packet_number;
							double pcr_time_shifting = delta_packets * TS_PACKET_LEN /  byterate;
							// old double delta_pcr_pts = (PCR_seconds-pts_time)*1000;
							double delta_pcr_pts = (PCR_seconds + pcr_time_shifting - pts_time)*1000;
							if ( delta_pcr_pts < 0 ) 
								delta_pcr_pts *= -1; // make it always positive without using Math libraries
							sprintf(msglog,"delta pcr-pts (ms)=%f [ pcr_time_shifting=%f]",delta_pcr_pts,pcr_time_shifting);
							Log(DEBUG,msglog );
							if (delta_pcr_pts > max_delta_pcr_pts)
								max_delta_pcr_pts = delta_pcr_pts;
							if ( (delta_pcr_pts > MAX_PCR_PTS_DELTA) && exit_on_error)
							{
								sprintf ( msglog, "Test Failed at packet n. %d - delta pcr-pts(ms)=%f  delta TS packets=%d",index,delta_pcr_pts,delta_packets);
								Log(ERROR,msglog );
								return -1;
							}

						}
						if ( (stream_id & 0xE0) == 0xE0 ) {
							Log(DEBUG,"Found audio PES");
							// hex_print(pes_packet,4);
						}
					}
				} else {
					Log(DEBUG,"payload not Found");
				}
			} else {
				sprintf(msglog,"-----packet n. %d ",index);
				Log(DEBUG,msglog);
				Log(DEBUG,"Not a Video Pid");
			}

		}

		if (ts_packet[0] != 0x47) {
			Log(ERROR,"ERROR: missed a sync byte. not a TS stream");
			fclose(fp);
			return -1;
		}
	}  // while
	time_end = time(0);
	fclose(fp);
	if ( max_delta_pcr_pts > MAX_PCR_PTS_DELTA )
	{
		sprintf ( msglog, "Test Failed - max delta pcr-pts(ms)=%f",max_delta_pcr_pts);
		Log(ERROR,msglog );
	}
	else {
		sprintf ( msglog, "Test Passed - max delta pcr-pts(ms)=%f",max_delta_pcr_pts);
		Log(INFO,msglog );
	}
	return (time_end - time_start);
}

double get_pts_time(unsigned char *pts_dts) {
    unsigned int pts_14 = ( (*(pts_dts+3)<<8) + *(pts_dts+4) ) >> 1;
    unsigned int pts_29 = ( (*(pts_dts+1)<<8) + *(pts_dts+2) ) >> 1;
    unsigned long long pts_32 = ( ( (*pts_dts)&0x0F)>>1);
    pts_32 = pts_32<<32;

    unsigned long long pts = pts_14 + (pts_29<<15) + (pts_32<<30);
    unsigned long long pts_27MHz = pts * 300;
    double pts_time = (double)pts_27MHz /27000000;
    //LOG("\n PTS=%llu   [pts_14=%u pts_29=%u pts_32=%llu]",pts,pts_14,pts_29,pts_32);
    //LOG("\n PTS=%llu   27MHz=%llu time=%f",pts,pts_27MHz,pts_time);
    return pts_time;
}

double get_pcr_time(unsigned char *pcr_pointer) {
    //LOG("\nPCR memory:");
    //hex_print(pcr_pointer,8);

    unsigned long long high_byte = ((unsigned long long)((*pcr_pointer)&0xFF))<<24;
    //unsigned long long high_byte = UINT64_C( (*pcr_pointer)&0xFF ) <<32;
    sprintf(msglog,"high_byte = %llu",high_byte);
    Log(DEBUG,msglog);
    unsigned long long program_clock_reference_base = high_byte + (*(pcr_pointer+1)<<16) + (*(pcr_pointer+2)<<8) + (*(pcr_pointer+3)&0xFE);
    program_clock_reference_base = (program_clock_reference_base<<1) + ((*(pcr_pointer+4)&0x80)>>7);
    sprintf(msglog,"program_clock_reference_base=%llu",program_clock_reference_base);
    Log(DEBUG,msglog);

    unsigned long long program_clock_reference_extension = pcr_pointer[5] + ((pcr_pointer[4] & 0x01)<<8);
    sprintf(msglog,"program_clock_reference_extension=%llu",program_clock_reference_extension);
    Log(DEBUG,msglog);




    // Time = (PCR@90KHz x 300 + PCR@27MHz) / 27MHz
    double PCR_seconds = ((program_clock_reference_base * 300) + program_clock_reference_extension ); 
    PCR_seconds = PCR_seconds / 27000000; 
    return PCR_seconds;
}




/**
 * returns a stream_info wi the following informations:
 *   int PCR_pid;
 *   int video_pid;
 *   int audio_pid;
 *   int file_bitrate_kbps;
 *   int duration;
 * @params filename: pointer to the ts stream
 * @return the video pid.
 **/
int extractStreamInfo(char *filename,stream_info_struct *stream_info) {

	int found = 0;
	int pid;
	int PMT_pid = -1;
	int pcr_pid = -1;
	int stream_pid,stream_type,video_pid,audio_pid;
	double first_PCR=-1, last_PCR=-1,PCR_seconds;
	unsigned char* ts_packet;

	FILE *fp;
	fp = fopen(filename, "rb");
	if (fp == NULL) {
		sprintf(msglog,
				"extractPcrPid Error: There was an Error opening the file %s",
				filename);
		Log(ERROR,msglog);
		return -1;
	}


	fseek(fp, 0L, SEEK_END);
	unsigned long int file_size = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	ts_packet = malloc(TS_PACKET_LEN * sizeof(char));

	int index=0, hasVideo=0,hasAudio=0;

	while (!feof(fp) && !found && (index*(TS_PACKET_LEN+1) )<file_size ) {
		index++;
		int bytes_read = fread(ts_packet, sizeof(char), TS_PACKET_LEN, fp);
		if (bytes_read != TS_PACKET_LEN) {
			sprintf(msglog,"ERROR in extractStreamInfo at index=%d",index);
			Log(ERROR,msglog);
			fclose(fp);
			return -1;
		}

		// 1) check we have the sync byte
		if (ts_packet[0] == 0x47) {

			// 2) check it's a PAT (pid=0x00)
			pid = ts_packet[2] + ((ts_packet[+1] & 0x1F) << 8);
			if (pid == 0x00) {
				// found PAT, get the PMT pid
				PMT_pid = ((ts_packet[15] & 0x1F) << 8) + ts_packet[16];
				sprintf(msglog,"found PAT !!! PMT pid = 0x%02x", PMT_pid);
				Log(DEBUG,msglog);
			} else if (pid == PMT_pid) {
				// found PMT, extract the programs
				//				hex_print(ts_packet, TS_PACKET_LEN);

				pcr_pid = (ts_packet[13] & 0x07) <<8;
				pcr_pid |= ts_packet[14];
				int section_length = ((ts_packet[6] & 0x0F) << 8)
					+ ts_packet[7];
				int offset = 17;

				while ((offset - 4) < section_length) {
					stream_type = ts_packet[offset];
					stream_pid = ((ts_packet[offset + 1] & 0x1F) << 8)
						+ ts_packet[offset + 2];
					sprintf(msglog,
							"offset(%d) :: stream:: type=0x%02x , pid=0x%02x",
							offset, stream_type, stream_pid);
					Log(DEBUG,msglog);
					// check if it's h264 video stream
					sprintf(msglog,"stream type=%d",stream_type );
					Log(DEBUG,msglog);

					if (!hasVideo && isVideoStreamType(stream_type)==0 ) {
						hasVideo=1;
						video_pid = stream_pid;
						printf("\nfound video pid=0x%02x type=0x%02x", video_pid,stream_type);
					}
					else if (!hasAudio && isAudioStreamType(stream_type)==0 ) {
						hasAudio = 1;
						audio_pid = stream_pid;
						printf("\nfound audio pid=0x%02x type=0x%02x", audio_pid,stream_type);
					}

					offset += (5 + ((ts_packet[offset + 3] & 0x0F) << 8)
							+ ts_packet[offset + 4]);
					sprintf(msglog,"new offset=%d", offset);
					Log(DEBUG,msglog);
				}

			}
			else if (pid == pcr_pid) { 
				// I need the first and last PCR to know the content length and calculate ther bitrate

				// Check the TS packet has Adaptation Field B[3]&0x20 == 1.
				if ((ts_packet[3] & 0x20) == 0x20) {
					Log(DEBUG,"Found AFC");
					unsigned char *adaptation_field = (ts_packet+4);

					// 4b) Se ha AFC, calcolo la sua lunghezza
					// AFC_len=B[4] altrimenti AFC_len=0
					unsigned char AFC_len = *(adaptation_field);
					sprintf(msglog,"AFC len=%d", AFC_len);
					Log(DEBUG,msglog);

					// check if adaptation field contains a PCR field
					if ( AFC_len > 0 &&  ( *(adaptation_field+1) & 0x10) == 0x10) {
						unsigned char *pcr_pointer = (adaptation_field+2);
						PCR_seconds = get_pcr_time(pcr_pointer);
						if (first_PCR == -1)
							first_PCR = PCR_seconds;
					}
				}
			} // if (pid == pcr_pid) 
			stream_info->PCR_pid = pcr_pid;
			stream_info->video_pid = video_pid;
			stream_info->audio_pid = audio_pid;

		} else {
			Log(ERROR,"no sync byte ????");
			fclose(fp);
			return -1;
		}
	} // while
	last_PCR = PCR_seconds;
	printf("\n firstPCR=%f  lastPCR=%f index=%d",first_PCR,last_PCR,index);
	double content_duration = last_PCR - first_PCR;
	double bitrate =  (index/ content_duration) * 8 * 188;
	stream_info->file_bitrate_kbps = bitrate;
	stream_info->duration = content_duration;
	fclose(fp);
	return 0;
}



/*
 * returns 0 if the stream_type is a video program
 * returns -1 else case
 */
int isVideoStreamType(int type) {
    if (
            (type == ISO_IEC_11172_2_VIDEO ) ||
            (type == ISO_IEC_13818_2_VIDEO ) ||
            (type == ISO_IEC_13818_1_PRIVATE_SECTION ) ||
            (type == ISO_IEC_13818_1_PES ) ||
            (type == ISO_IEC_13818_6_TYPE_A ) ||
            (type == ISO_IEC_13818_6_TYPE_B ) ||
            (type == ISO_IEC_13818_6_TYPE_C ) ||
            (type == ISO_IEC_13818_6_TYPE_D ) ||
            (type == ISO_IEC_14496_2_VISUAL ) ||
            (type == ITU_T_H264 ) ) 
        return 0;
    else
        return -1;
}

/*
 * returns 0 if the stream_type is a audio program
 * returns -1 else case
 */
int isAudioStreamType(int type) {
    if (
            (type == ISO_IEC_11172_3_AUDIO ) ||
            (type == ISO_IEC_13818_3_AUDIO ) ||
            (type == ISO_IEC_13818_7_AUDIO ) ||
            (type == ISO_IEC_14496_3_AUDIO ) ||
            (type == DOLBY_AC3_AUDIO ) )
        return 0;
    else
        return -1;
}




