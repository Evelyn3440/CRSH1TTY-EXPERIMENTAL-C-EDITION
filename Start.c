#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>    // time()

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
int main(int argc,char* argv[]){
    srand(time(NULL));
    char string[8];
    generate(string);
    printf("%s\n\n",string);

    
    
    return 0;
}