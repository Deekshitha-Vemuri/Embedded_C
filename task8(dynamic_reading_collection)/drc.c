#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<limits.h>
int main(){
    int size=4,c=0;
    int *ptr;
    ptr=malloc(size*(sizeof(*ptr)));
    if(!ptr){
        return 1;
    }
    printf("Enter readings");
    int r;
    while(scanf("%d",&r)==1){
        if(c==size){
            if(size>INT_MAX/2/(int)sizeof(*ptr)){
                free(ptr);
                return 1;
            }
            int next_size=size*2;
            int *tmp=realloc(ptr,next_size*sizeof(*ptr));
            if(!tmp){
                free(ptr);
                return 1;
            }
            ptr=tmp;
            size=next_size;
        }
        ptr[c++]=r;
        printf("enter next reading to continue or q to view: ");
    }
   printf("Readings:\n");
    if(c>0){
        long long sum=0;
        for(int i=0;i<c;i++){
            printf("%d ",ptr[i]);
            sum+=ptr[i];
        }
        printf("\n");
        double average=(double)sum/c;
        printf("Average of readings:%.2f\n",average);
    }
    else{
        printf("No readings were entered\n");
    }
    free(ptr);
    return 0;

}