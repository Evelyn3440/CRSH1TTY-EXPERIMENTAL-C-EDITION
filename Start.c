#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <linux/swab.h>
#include "Start.h"
/* Had to go through multiple files to find that! */
// int VENDOR_CC_RMA_CHALLENGE_RESPONSE = 30;
// uint32_t send_vendor_command(struct transfer_descriptor *td,
// 			     uint16_t subcommand, const void *command_body,
// 			     size_t command_body_size, void *response,
// 			     size_t *response_size)
// {
// 	int32_t rv;

// 	rv = tpm_send_pkt(td, 0, 0, command_body, command_body_size,
// 				  response, response_size, subcommand);

// 		if (rv == -1) {
// 			fprintf(stderr,
// 				"Error: Failed to send vendor command %d\n",
// 				subcommand);
// 		}

// 	return rv; /* This will be converted into uint32_t */
// }

// static int process_rma(struct transfer_descriptor *td, const char *authcode)
// {
// 	char rma_response[81];
// 	size_t response_size = sizeof(rma_response);
// 	size_t i;
// 	size_t auth_size = 0;

// 	printf("Processing response...\n");
// 	auth_size = strlen(authcode);
// 	response_size = sizeof(rma_response);

//     int test = send_vendor_command(td, VENDOR_CC_RMA_CHALLENGE_RESPONSE, authcode,
// 			    auth_size, rma_response, &response_size);

// 	if (response_size == 1 || test == -1) {
// 		fprintf(stderr, "\nrma unlock failed, code %d\n", *rma_response);
//         return 0;
// 	}
// 	printf("RMA unlock succeeded.\n");
//     return 1;
// }
void unenroll(){
    system("flashrom --wp-disable");
    system("/usr/share/vboot/bin/set_gbb_flags.sh 0x80b0");
}
char characters[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};
int chars_length = sizeof(characters); //char is 1 iirc so no need to divide
char generate(char* str){
    for (int i = 0; i < 8; i++){
        str[i] = characters[rand() % chars_length];
    }
    str[8] = 0;
}
void* process(void * num){
    srand((int)num);
    while(1){
        char *test = malloc(8);
        char *test2 = malloc(24);
        strcpy(test2, "gsctool -t -r ");
        //printf("%s\n\n",test2);
        generate(test);
        //printf("old: %s\n\n",test2);
        strcat(test2,test);
        //printf("new: %s\n\n",test2);
        //printf("%li\n\n",sizeof(string));
        //system(cnd)
        if (system(test2) == 1){
            printf("Auth code found, sleeping for 10 seconds before setting GBB flags to ignore FWMP");
            fflush(stdout); //tired of this printing 6 minutes later
            sleep(10);
            unenroll();
            exit(1);
        }
    }
}
int main(int argc,char* argv[]){
    int threads;
    //printf("Warning! THIS DOESNT CLOSE WITHOUT YOU MURDERING YOUR TERMINAL! BE WARNED!\n");
    //printf("Please send the 'output.yoink' file to eve_344_test (Evelyn34402)\n");
    printf("How many threads: \n");

    scanf("%d", &threads);
    threads += 1;
    //threads = 3;
    if (threads < 2){
        printf("Because you input a value below 1 it was set to 1\n\n");
        threads = 2;
        sleep(3);
    }
    pthread_t thid;
    for (int i = 0; i < threads-1; i++){
        pthread_create(&thid,NULL,(void *)process,(void *)thid);
    }
    void *ret;
    pthread_join(thid,&ret);
    return 0;
}