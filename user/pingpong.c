#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]){
	int pipe_p_c[2], pipe_c_p[2];
	pipe(pipe_p_c); 
	pipe(pipe_c_p); 
	
	if(fork() != 0) {//父进程的代码 	
        char buf;
		write(pipe_p_c[1], "*", 1); // 父进程发出该字节
		read(pipe_c_p[0], &buf, 1); // 父进程等待读取子进程
		printf("%d: received pong\n", getpid()); //子进程读到数据
		wait(0);//前面进程都完成了才继续
	} else { //子进程的代码
		char buf;
		read(pipe_p_c[0], &buf, 1); // 子进程读取管道
		printf("%d: received ping\n", getpid());
		write(pipe_c_p[1], &buf, 1); //子进程将字节写回父进程
	}
    close(pipe_p_c[0]);
    close(pipe_p_c[1]);
    close(pipe_c_p[0]);
    close(pipe_c_p[1]);
	exit(0);
}