/*
 * lisa_test.c
 *
 *  Created on: Sep 20, 2017
 *      Author: carlos
 */

#include "my_lisa.h"

void main() {
    time_t t;
    srand((unsigned) time(&t));  // seed for repeatable results
    //srand(97);  // seed for repeatable results
    clock_t start, end;

    int corruption_pct, match_confidence, payload_idx_output, payload_idx_input;
    unsigned char lisa_sync_buffer [LISA_SYNC_LEN] = {0};
    unsigned char output_buffer [BUFFER_LEN];
    unsigned char input_buffer [BUFFER_LEN];
    char * payload = "Hello LISA!";

    // Generate output
    pct_prompt(&corruption_pct, "%% LISA Sync Field Corruption? [0-100]: ");
    generate_lisa_sync(corruption_pct, lisa_sync_buffer);
    payload_idx_output = gen_output_buffer(output_buffer,
            lisa_sync_buffer, payload);
    write_file_buffer(output_buffer, "lisa_output.txt");

    // Read input buffer
    pct_prompt(&match_confidence, "%% Confidence level to apply [0-100]: ");
    read_file_buffer(input_buffer, "lisa_output.txt");

    start = clock();
    payload_idx_input = lisa_find_payload_vanilla(match_confidence, input_buffer);
    //payload_idx_input = lisa_find_payload_prob(match_confidence, input_buffer);
    end = clock();

    if(payload_idx_output == payload_idx_input) {
        printf("[SUCCESS] Payload found at index %d: %s",
        		payload_idx_input, &output_buffer[payload_idx_input]);
    } else
        printf("[ERROR] Failed to find payload with confidence >= %d%%."
        		" [(E)%d , (R)%d]", match_confidence, payload_idx_output,
				payload_idx_input);

    printf("\nTotal time taken: %f\n", ((double) (end - start)) / CLOCKS_PER_SEC);
}
