#include<stdio.h>
int main(){
    int n;
    printf("Enter size");
     if(scanf("%d", &n) != 1) {
        printf("Invalid input.Enter an integer size.\n");
        return 1;
    }
    if(n<=0){
        printf("s3ize must be greater than 0");
        return 1;
    }
    int a[n];
    printf("Array created enter values");
    for(int i=0;i<n;i++){
        if (scanf("%d", &a[i]) != 1) {
        printf("Invalid input. Enter integers only.\n");
        return 1;
    }}
    if(n>=1){
        printf("%.1f ",(float)a[0]);
    }
    if(n>=2){
        float c1=((float)a[0]+a[1])/2.0;
        printf("%.1f ",c1);
    }
    if(n>=3){
        float sum=((float)a[0]+a[1]+a[2]);
        float c=sum/3.0;
        printf("%.1f ",c);
        for(int i=3;i<n;i++){
            sum=sum-a[i-3]+a[i];
            printf("%.1f ",sum/3.0);
        }
    }
    return 0;
}