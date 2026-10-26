#include<stdio.h>
#include<string.h>
struct Owner{
 char first_name[30];
 char last_name[30];
};
struct car{
    char brand[30];
    int year;
    struct Owner owner;
};
int main(){
  struct Owner o1;
  printf("enter owner first name: ");
  fgets(o1.first_name,sizeof(o1.first_name),stdin);
  o1.first_name[strcspn(o1.first_name, "\n")] = '\0';
  printf("enter owner last name: ");
  fgets(o1.last_name,sizeof(o1.last_name),stdin);
  o1.last_name[strcspn(o1.last_name, "\n")] = '\0';
  struct car c1;
  printf("enter his car details\n");
  printf("enter the car brand:");
  fgets(c1.brand,sizeof(c1.brand),stdin);
  c1.brand[strcspn(c1.brand, "\n")] = '\0';
  printf("enter the year you bought the car: ");
  scanf("%d",&c1.year);
  c1.owner=o1;
  printf("Details\n");
  printf("The %s car was bought by %s %s in the year %d",c1.brand,c1.owner.first_name,c1.owner.last_name,c1.year);
  return 0;  
}
