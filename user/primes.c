#include "kernel/types.h"
#include "user/user.h"

void 
process(int pd[])
{
    int p=0,arr[34];//用于记录新的数组
    close(pd[1]);
    int pd_child[2];
    pipe(pd_child);

    int first,next;
    read(pd[0],&first,sizeof(first));
    printf("prime %d\n",first);//第一个数为质数

    while(read(pd[0],&next,sizeof(next))){//读到没有数据为止
        if(next%first!=0){//不是倍数的传递到下一个管道
            arr[p]=next;
            p++;
        }
    }

    if(p==0)//如果已经没有数据要处理了，则表示判断结束
        {exit(0);}

    int pid=fork();//创建下一个管道
    if(pid==0){
        close(pd[0]);
        process(pd_child);
    }
    else{
        int cur;
        close(pd_child[0]);
        for(int i = 0; i < p; i++){
            cur = arr[i];
            write(pd_child[1],&cur,sizeof(cur));
        }
        close(pd_child[1]);
        wait(0);
    }

}


int 
main(int argc, char *argv[]){
    int pd[2]; 
    pipe(pd);//创建管道
    int pid=fork();

    if(pid==0){//子进程
        process(pd);
    }
    else{//父进程
        int nums[34];
        for(int i=2;i<36;i++){
            nums[i-2]=i;
        }
        int cur;
        close(pd[0]);
        for(int i = 0; i < 34; i++){
            cur = nums[i];
            write(pd[1],&cur,sizeof(cur));
        }
        close(pd[1]);
        wait(0);
    }
    exit(0);
}