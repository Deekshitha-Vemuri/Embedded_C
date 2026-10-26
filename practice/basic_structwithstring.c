#include<stdio.h>
struct st{
int num;
char name[30];
char gender;
};

int main(){
    struct st s1;
    printf("enter number: ");
    scanf("%d",&s1.num);
    getchar(); 
    printf("enter your name: ");
    fgets(s1.name,sizeof(s1.name),stdin);
    printf("enter your gender M/F: ");
    scanf(" %c",&s1.gender);
    printf("Details\n");
    printf("%d\n%s%c\n",s1.num,s1.name,s1.gender);
    return 0;
}
