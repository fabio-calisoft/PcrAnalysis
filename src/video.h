/*
 * functions.h
 *
 *  Created on: June 8, 2013
 *      Author: Secure Digital Ddelivery
 */

#ifndef VIDEO_H_
#define VIDEO_H_

#define TS_PACKET_LEN   188



typedef struct struct_stream_info_struct {
    int PCR_pid;
    int video_pid;
    int audio_pid;
    double file_bitrate_kbps;
    int duration;
} stream_info_struct ;


/*
 * see http://en.wikipedia.org/wiki/Category:ISO/IEC_13818
 * and http://en.wikipedia.org/wiki/Program-specific_information#Elementary_stream_types
 */
typedef enum MPEG2StreamType {
	BDA_UNITIALIZED_MPEG2STREAMTYPE = 1,
	Reserved1 = 0x00,
	ISO_IEC_11172_2_VIDEO = 0x01, // (MPEG-1 video)
	ISO_IEC_13818_2_VIDEO = 0x02, // ITU-T Rec. H.262 and ISO/IEC 13818-2 (MPEG-2 higher rate interlaced video)
	ISO_IEC_11172_3_AUDIO = 0x03, // ISO/IEC 11172-3 (MPEG-1 audio)
	ISO_IEC_13818_3_AUDIO = 0x04, // ISO/IEC 13818-3 (MPEG-2 halved sample rate audio)
	ISO_IEC_13818_1_PRIVATE_SECTION = 0x05, // ITU-T Rec. H.222 and ISO/IEC 13818-1 (MPEG-2 tabled data)
	ISO_IEC_13818_1_PES = 0x06, // ITU-T Rec. H.222 and ISO/IEC 13818-1 (MPEG-2 packetized data)
	ISO_IEC_13522_MHEG = 0x07,  
	ANNEX_A_DSM_CC = 0x08,
	ITU_T_REC_H_222_1 = 0x09,
	ISO_IEC_13818_6_TYPE_A = 0x0A, // MPEG-2    
	ISO_IEC_13818_6_TYPE_B = 0x0B, // MPEG-2
	ISO_IEC_13818_6_TYPE_C = 0x0C, // MPEG-2
	ISO_IEC_13818_6_TYPE_D = 0x0D, // mpeg-2
	ISO_IEC_13818_1_AUXILIARY = 0x0E,
	ISO_IEC_13818_7_AUDIO = 0x0F,   //  ISO/IEC 13818-7 ADTS AAC (MPEG-2 lower bit-rate audio)
	ISO_IEC_14496_2_VISUAL = 0x10,  // ISO/IEC 14496-2 (MPEG-4 H.263 based video) MPEG-4 Part 2
	ISO_IEC_14496_3_AUDIO = 0x11,   // ISO/IEC 14496-3 (MPEG-4 LOAS multi-format framed audio)
	ISO_IEC_14496_1_IN_PES = 0x12,
	ISO_IEC_14496_1_IN_SECTION = 0x13,
	ISO_IEC_13818_6_DOWNLOAD = 0x14,
	METADATA_IN_PES = 0x15,
	METADATA_IN_SECTION = 0x16,
	METADATA_IN_DATA_CAROUSEL = 0x17,
	METADATA_IN_OBJECT_CAROUSEL = 0x18,
	METADATA_IN_DOWNLOAD_PROTOCOL = 0x19,
	IRPM_STREAMM = 0x1A,
	ITU_T_H264 = 0x1B, // ITU-T Rec. H.264 and ISO/IEC 14496-10 (lower bit-rate video)
	ISO_IEC_13818_1_RESERVED = 0x1C,
	USER_PRIVATE = 0x10,
	ISO_IEC_USER_PRIVATE = 0x80,    // ITU-T Rec. H.262 and ISO/IEC 13818-2 for DigiCipher II or PCM audio for Blu-ray
	DOLBY_AC3_AUDIO = 0x81,         // Dolby Digital up to six channel audio for ATSC and Blu-ray
	DOLBY_DIGITAL_PLUS_AUDIO_ATSC = 0X87
} MPEG2StreamType;



int printPtsPcrDelta(char *filename, int pcr_pid,int MAX_PCR_PTS_DELTA,int exit_on_error,int bitrate);

/**
  * Extract the stream informations: PCR and video pids and the
  * bitare of the file (not video bitrate!)
  * @param: filename
  * @param: the struct stream_info to be returned
  **/
int extractStreamInfo(char *filename,stream_info_struct *stream_info);


/**
  * calculate the PCR time in seconds.
  * the Program Clock Reference is 42 bits long:
  * The first 33 bits are based on a 90 kHz clock and form the program_clock_reference_base.
  * The last 9 are based on a 27 MHz clock and called program_clock_reference_extension.
  * For the reference_base, we have to take the first 4 bytes and one bit and align the result.
  * Basically add all the bytes, scroll left of one bit and add the first bit from the 5th byte.
  * For the program_clock_reference_extension, use the 6th byte and add a 2^8 if the LSb of the 
  * 5th byte is on.
  * @param pcr_pointer: pointer to the PCR 6 bytes memory array
  * @return the time in seconds of the PCR
  *
  */
double get_pcr_time(unsigned char *pcr_pointer);
                                
double get_pts_time(unsigned char *pts_dts);





/**
 * return 0 when type is a video stream type
 * return -1 if it's not a video type
 * @param type: the ISO IEC stream type
 **/
int isVideoStreamType(int type);


/**
 * return 0 when type is a audio stream type
 * return -1 if it's not a audio type
 * @param type: the ISO IEC stream type
 **/
int isAudioStreamType(int type);


#endif /* VIDEO_H_ */
