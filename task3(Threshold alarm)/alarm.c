#include<stdio.h>
int main(){
    double t;
    int alarm=0;
    double h,l;
    printf("enter higher thresholdvalue: ");
    scanf("%lf",&h);
    printf("enter lower threshold value: ");
    scanf("%lf",&l);
    printf("Enter temperatures\n");
    while(scanf("%lf",&t)==1){
        if(alarm==0&&t>=h){
            alarm=1;
        }
        // else if(alarm==0&&t<h){
        //     alarm=0;
        // }
        // else if(alarm==1&&t>l){
        //     alarm=1;
        // }
        else if(alarm==1&&t<=l){
            alarm=0;
        }
        printf("%.1f -> %s\n", t, alarm ? "Alarm Ringing" : "Alarm resting");
    }
}