/*
 * my_lisa.c
 *
 *  Created on: Sep 16, 2017
 *      Author: Carlos R. Davila
 *
 *      Status: Working version. Tests and robustness still WIP.
 */

#include "my_lisa.h"

/**
 * Function will combine lisa_sync + payload and place it in the
 * buffer provided.
 *
 * @param buffer    Output buffer to place sync + payload.
 * @param lisa_sync Sync field to place in the buffer
 * @param payload   The payload to place in the output buffer
 * @param gen_file  Whether to generate an output file [1] or not [0]. The
 *                  name of the output file will be lisa_output.txt
 */
int gen_output_buffer(unsigned char * buffer, unsigned char * lisa_sync,
		char * payload) {

    int lisa_idx, payload_idx;
    int payload_len = strlen(payload);

    // Generate random offset for sync + payload
    // between 0 and BUFFER_LEN - LISA_SYNC_LEN - payload_len
    lisa_idx = rand()%(BUFFER_LEN - LISA_SYNC_LEN - payload_len);
    payload_idx = lisa_idx + LISA_SYNC_LEN;

    // Insert LISA and payload fields byte by byte
    for(int i = lisa_idx; i < lisa_idx + LISA_SYNC_LEN; i++) {
            buffer[i] = lisa_sync[i - lisa_idx];
            buffer[i + LISA_SYNC_LEN] = payload[i - lisa_idx];
    }

    if(DEBUG == 1) {
        for(int i = lisa_idx; i < lisa_idx + LISA_SYNC_LEN; i++)
            printf("output_buffer[%d] = %x\n", i, buffer[i] & 0xff);

        for(int i = payload_idx; i < payload_idx + payload_len; i++)
                printf("output_buffer[%d] = %c\n", i, buffer[i]);
    }

    return payload_idx;
}

/**
 * Generate bit mask with a single 1 at 'position' and all other bits zeros
 *
 * @param position  Location of enabled bit in the sequence
 */
uint gen_mask(uint position) {
    return 1 << position;
}

/**
 * Generate a LISA sync buffer with specified corruption
 *
 * @param corrupt_pct       Percentage of bits to corrupt in LISA buffer (0-100)
 * @param lisa_sync_buffer  Buffer to save sync field to
 */
void generate_lisa_sync(int corrupt_pct, unsigned char * lisa_sync_buffer) {
    char lisa_prefixes [2] = {0xA0, 0x50};
    int idx;

    // Generate standard LISA
    for(int i = 0; i < 2; i++) {
        for(int j = 0; j < 16; j++) {
            idx = i * 16 + j;
            lisa_sync_buffer[idx] = lisa_prefixes[i] + j;

            if(DEBUG > 1)
                printf("gen_lisa_buffer[%d] = %x\n", idx,
                                  lisa_sync_buffer[idx] & 0xff);
        }
    }

    // Corrupt LISA (non-uniform distribution)
    int num_corrupt_bits = (LISA_SYNC_LEN * 8 * corrupt_pct) / 100;
    int rand_bit_buffer [num_corrupt_bits]; // history of corrupted bits
    int global_byte_offset = 0; // byte offset where bit will be corrupted
    uint bit_mask = 0;          // bit mask to invert bit
    int rand_bit = 0;           // corruption bit offset

    for(int i = 0; i < num_corrupt_bits; i++) {
        while(1) {
            rand_bit = rand()%(LISA_SYNC_LEN * 8);  // generate offset location

            // check if location has already been corrupted
            for(int j = 0; j < num_corrupt_bits; j++)
                if(rand_bit_buffer[j] == rand_bit)
                    continue;

            rand_bit_buffer[i] = rand_bit;
            break;
        }

        // go to offset byte and invert the selected bit offset
        global_byte_offset = rand_bit / 8;
        bit_mask = gen_mask(rand_bit - global_byte_offset * 8);
        lisa_sync_buffer[global_byte_offset] ^= bit_mask;

        if(DEBUG >= 1)
            printf("[%d] Byte after applying mask [%d] at %d: %x\n", i,
                    bit_mask, global_byte_offset,
                    lisa_sync_buffer[global_byte_offset]);
    }
}

/**
 * Prompt user for input
 *
 * @param input_pct Variable to save integer value (0-100) input by user
 * @param prompt    Prompt to display to the user
 */
void pct_prompt(int * input_pct, char * prompt) {
    int failed_attempts = 0;
    int possible_attempts = 3;

    while(1) {
        printf(prompt);
        scanf("%d", input_pct);

        if(*input_pct >= 0 && *input_pct <= 100) {
            printf("[INFO] Value Set: %d%%\n", *input_pct);
            break;
        } else {
            printf("(%d of %d attempts) Corruption value needs "
                    "to be between 0-100. You entered: %d\n",
                    ++failed_attempts, possible_attempts, *input_pct);

            if(failed_attempts >= possible_attempts) {
                printf("[ERROR] Too many failed attempts.\n\n");
                exit(-1);
            }
        }
    }
}

/**
 * Standard LISA algorithm discussed in class. Will implement custom ones for
 * fun to test speed and other reliability improvements.
 *
 * @param confidence_lvl	confidence level for the search algorithm to test
 * @param input_buffer		buffer to search for the LISA sync and payload
 */
int lisa_find_payload_vanilla(int confidence_lvl, unsigned char * input_buffer) {
    int num_matched, max_match_idx = 0;
    double max_matched_pct = 0.0;
    int window_match_pcts[BUFFER_LEN - LISA_SYNC_LEN] = {0};
    unsigned char lisa_buffer [LISA_SYNC_LEN];   // Gen and hold good LISA field

    generate_lisa_sync(0, lisa_buffer);

    // Window method
    for(int i = 0; i < BUFFER_LEN - LISA_SYNC_LEN; i++) {
        for(int j = 0; j < LISA_SYNC_LEN; j++) {
            if(input_buffer[i + j] == lisa_buffer[j])
                num_matched++;
        }
        window_match_pcts[i] = num_matched;
        if(window_match_pcts[max_match_idx] < num_matched)
            max_match_idx = i;
        num_matched = 0;
    }

    max_matched_pct = (double)window_match_pcts[max_match_idx] / LISA_SYNC_LEN;
    printf("[INFO] Match at (%d) with confidence: %.2f%%\n",
    		max_match_idx + LISA_SYNC_LEN, max_matched_pct * 100);
    if(max_matched_pct >= confidence_lvl) {
        return max_match_idx + LISA_SYNC_LEN;
    } else
        return 0;
}

int prob_match(int offset, unsigned char * buffer,
		unsigned char * lisa_buffer) {
	int matched [2] = {0};
	int idx = 0;

	for(int i = 0; i < LISA_SYNC_LEN; i++){
		if(buffer[offset + i] == lisa_buffer[i]) {
			matched[idx]++;
			if(matched[0] >= 3 && idx == 0) {
				i += 0xAF - lisa_buffer[i];
				idx++;
			} else if(matched[1] >= 3) break;
		}
	}

	if(matched[1] < 3)
		return -1;
	else
		return offset + 32;
}

/**
 * New LISA algorithm. With two key improvements:
 * 	1) Searches with non-overlapping windows
 * 	2) Limited comparisons upon match with high confidence
 *
 * @param confidence_lvl	confidence level for the search algorithm to test
 * @param input_buffer		buffer to search for the LISA sync and payload
 */
int lisa_find_payload_prob(int confidence_lvl, unsigned char * input_buffer) {
    int matched = 0;
    int sub = 0;
    unsigned char lisa_buffer [LISA_SYNC_LEN];   // Gen and hold good LISA field

    generate_lisa_sync(0, lisa_buffer);

    // Window method
    for(int i = 0; i < BUFFER_LEN - LISA_SYNC_LEN; i++) {
        for(int j = 0; j < LISA_SYNC_LEN; j++) {
            if(input_buffer[i + j] == lisa_buffer[j]) {
                //launch search alg
            	if(input_buffer[i + j] >> 4 == 0xA)
            		sub = 0xA0;
            	else
            		sub = 0x50;

            	int lisa_start_loc = (i + j) - (input_buffer[i + j] - sub);
            	matched = prob_match(lisa_start_loc, input_buffer, lisa_buffer);
            	if(matched != -1) {
            		i = j = BUFFER_LEN;
            		break;
            	}
            }
        }
    }

    printf("[INFO] Matched using prob alg!\n");
    return matched;
}

/**
 *  Read buffer from file
 *  
 *  @param payload_idx_in
 */
void read_file_buffer(unsigned char * buffer, char * file_location) {
//    char buffer[BUFFER_LEN];
    char line_buffer[1] = {0};
    FILE *fp;

    fp = fopen(file_location, "r");
    if(fp) {
        for(int i = 0; i < BUFFER_LEN; i++) {
            if(fgets(line_buffer, 2, (FILE*)fp) != NULL);
            	buffer[i] = line_buffer[0];
            if(DEBUG >= 1 && i > 370 && i < 400)
            	printf("RLine[%d]: %.2x\n", i, (unsigned char)buffer[i]);
        }
        fclose(fp);
    }
}

/**
 * Write buffer to a file. Each character will be in a single
 * line with '\n' at the end.
 *
 * @param buffer    buffer to write to a file
 * @param filename  filename to write to
 */
void write_file_buffer(unsigned char * buffer, char * file_location) {
    // Read file and stuff
    FILE *fp;
    fp = fopen(file_location, "w");

    if(fp) {
        for(int i = 0; i < BUFFER_LEN; i++) {
            fprintf(fp, "%c", (unsigned char)buffer[i]);
            if(DEBUG >= 1 && i > 370 && i < 400)
            	printf("WLine[%d]:%.2x\n", i, (unsigned char)buffer[i]);
        }
        fclose(fp);
    }
}

//void main() {
//    time_t t;
//    srand((unsigned) time(&t));  // seed for repeatable results
//    srand(97);  // seed for repeatable results
//
//    int corruption_pct, match_confidence, payload_idx_output, payload_idx_input;
//    unsigned char lisa_sync_buffer [LISA_SYNC_LEN] = {0};
//    unsigned char output_buffer [BUFFER_LEN];
//    unsigned char input_buffer [BUFFER_LEN];
//    char * payload = "Hello LISA!";
//
//    // Generate output
//    pct_prompt(&corruption_pct, "%% LISA Sync Field Corruption? [0-100]: ");
//    generate_lisa_sync(corruption_pct, lisa_sync_buffer);
//    payload_idx_output = gen_output_buffer(output_buffer,
//            lisa_sync_buffer, payload);
//    write_file_buffer(output_buffer, "lisa_output.txt");
//
//    // Read input buffer
//    pct_prompt(&match_confidence, "%% Confidence level to apply [0-100]: ");
//    read_file_buffer(input_buffer, "lisa_output.txt");
//    payload_idx_input = lisa_find_payload_vanilla(match_confidence,
//    		input_buffer);
//
//    if(payload_idx_output == payload_idx_input) {
//        printf("[SUCCESS] Payload found at index %d: %s",
//        		payload_idx_input, &output_buffer[payload_idx_input]);
//    } else
//        printf("[ERROR] Failed to find payload with confidence >= %d%%."
//        		" [(E)%d , (R)%d]", match_confidence, payload_idx_output,
//				payload_idx_input);
//}
