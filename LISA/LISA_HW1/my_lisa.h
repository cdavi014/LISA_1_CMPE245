/*
 * my_lisa.h
 *
 *  Created on: Sep 20, 2017
 *      Author: carlos
 */

#ifndef MY_LISA_H_
#define MY_LISA_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define LISA_SYNC_LEN 32
#define BUFFER_LEN 1100000
#define DEBUG 0

/* Util Functions */
int gen_output_buffer(unsigned char * buffer, unsigned char * lisa_sync,
		char * payload);
uint gen_mask(uint position);
void generate_lisa_sync(int corrupt_pct, unsigned char * lisa_sync_buffer);
void pct_prompt(int * input_pct, char * prompt);
void read_file_buffer(unsigned char * buffer, char * file_location);
void write_file_buffer(unsigned char * buffer, char * file_location);

/* LISA Algorithms */
int lisa_find_payload_vanilla(int confidence_lvl, unsigned char * input_buffer);
int lisa_find_payload_prob(int confidence_lvl, unsigned char * input_buffer);

#endif /* MY_LISA_H_ */
