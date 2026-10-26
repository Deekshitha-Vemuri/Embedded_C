#include<stdio.h>
#include<stdlib.h>
int main(){
    char input[100];
    char *ptr;
    double c,f;
    while(1){
        printf("Enter Celcius temperature: ");
        if(fgets(input,sizeof(input),stdin)==NULL){
            printf("No input recieved and input stream closed\n");
            return 0;
        }
        c=strtod(input,&ptr);
        if(ptr!=input&&(*ptr=='\n'||*ptr=='\0')){
            break; //found the valid input
        }
        printf("Invalid input.Please try again.\n");
    }
    f=((9.0/5.0)*c)+32.0;
    printf("Celcius=%.2lf Fahrenheit=%.2lf F",c,f);
    return 0;
}