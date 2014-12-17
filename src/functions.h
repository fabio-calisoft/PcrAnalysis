/*
 * functions.h
 *
 *  Created on: June 8, 2013
 *      Author: Secure Digital Ddelivery
 */

#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

int loadFile(char *path, unsigned char **buffer);
int readData(FILE *fp, unsigned char **buffer, unsigned int size_to_read);
int saveFile(char *path, unsigned char *buffer, int size);

typedef enum LEVEL_enum {
    DEBUG,
    VERBOSE,
    INFO,
    WARN,
    ERROR
} LEVEL;
void Log(LEVEL verbosity , char * msg);

void print_menu(char *this_program_name);
void hex_print(const unsigned char *pv, int len);
void convert_Hex_string_to_uchar(char *myarg, unsigned char **uchar);
int convert_input_params_to_vars(int argc, char **argv, char **TS_FILE, int* max_pcr_pts_delta,int* exit_on_error);


#endif /* FUNCTIONS_H_ */
