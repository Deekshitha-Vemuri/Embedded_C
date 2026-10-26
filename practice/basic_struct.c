#include<stdio.h>
struct myStructure{
    int myNum;
    char myLetter;
};
int main(){
    struct myStructure s1;
    printf("enter number");
    scanf("%d",&s1.myNum);
    printf("enter a letter");
    scanf(" %c",&s1.myLetter);
    printf("struct details\n");
    printf("%d\n",s1.myNum);
    printf("%c\n",s1.myLetter);
    return 0;
}