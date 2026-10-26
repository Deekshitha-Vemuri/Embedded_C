#include<stdio.h>
struct Student{
    char name[30];
    int marks;
};
int main(){
    struct Student students[4];
    for(int i=0;i<4;i++){
        printf("enter student name");
        scanf("%s",&students[i].name);
        printf("enter student marks");
        scanf("%d",&students[i].marks);
    }
    int best=0;
    for(int i=1;i<4;i++){
        if(students[i].marks>students[best].marks){
            best=i;
        }
    }
    printf("Top studentis %s who secured %d marks",students[best].name,students[best].marks);
    return 0;
}
