#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <linux/swab.h>
#include "gsctoolrecre.c"

/* Had to go through multiple files to find that! */
int VENDOR_CC_RMA_CHALLENGE_RESPONSE = 30;

uint32_t send_vendor_command(struct transfer_descriptor *td,
			     uint16_t subcommand,
			     const void *command_body,
			     size_t command_body_size,
			     void *response,
			     size_t *response_size)
{
	int32_t rv;
    rv = tpm_send_pkt(td, 0, 0,
            command_body, command_body_size,
            response, response_size, subcommand);
    if (rv == -1) {
        fprintf(stderr,
            "Error: Failed to send vendor command %d\n",
            subcommand);
	}
	return rv; /* This will be converted into uint32_t */
}

static int process_rma(struct transfer_descriptor *td, const char *authcode)
{
    char rma_response[81];
	size_t response_size = sizeof(rma_response);
	size_t i;
	size_t auth_size = 0;
    printf("Processing response...\n");
	auth_size = strlen(authcode);
	response_size = sizeof(rma_response);
	int32_t temp = send_vendor_command(td, VENDOR_CC_RMA_CHALLENGE_RESPONSE,
			    authcode, auth_size,
			    rma_response, &response_size);
    if (temp == -1){
        return -1;
    }
	if (response_size == 1) {
		printf("\nrma unlock failed");
        return -1;
	}
	printf("RMA unlock succeeded.\n");
    return 1;
    
}
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
void process(void * td){
    while(1){
        char string[8];
        generate(string);
        if (process_rma(&td, string) == 1){
            printf("Auth code found, sleeping for 10 seconds before setting GBB flags to ignore FWMP");
            sleep(10);
            unenroll();

            execlp("/bin/bash", "bash", (char *)NULL);
        }
    }
}
int main(int argc,char* argv[]){
    int threads;
    printf("How many threads: \n");

    scanf("%d", &threads);
    if (threads < 1){
        printf("Because you input a value below 1 it was set to 1\n");
        threads = 1;
        sleep(3);
    }
    struct transfer_descriptor td;
    td.ep_type = ts_xfer;
    pthread_t thid;
    for (int i = 0; i < threads-1; i++){
        pthread_create(&thid,NULL,(void *)process,(void *)&td);
    }
    void *ret;
    pthread_join(thid,&ret);
    srand(getpid());
    return 0;
}