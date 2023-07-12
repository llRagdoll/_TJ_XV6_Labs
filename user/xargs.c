#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#define MAXLEN 32

int 
main(int argc, char *argv[])
{
    char* args[MAXLEN];
    for (int i = 0; i < argc-1; ++i) {
        args[i] = argv[i+1];
    }
    if(argc<2){
        printf("Parameters are not enough!\n");
        exit(0);
    }
    else{
        char buf[MAXLEN];
        char* p = buf;
        while ((read(0, p, 1)) > 0) {
            if (*p != '\n') {
                ++p;//向后读取
            } else {      
                *p = '\0';//将末尾换行符换为0
                int pid=fork();
                if (pid== 0) {//子进程
                    args[argc-1] = buf;
                    exec(args[0], args);
                    exit(0);
                } else {
                    wait(0);
                }
                p = buf;
            }
        }
    }
    exit(0);
}