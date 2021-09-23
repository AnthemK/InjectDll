#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#define N 1
#include <stdio.h>
#include <conio.h>
#include <stdlib.h>
#define N 1
int main(int argc,char *argv[])
{
 FILE *fp;
 FILE *out;
 char name[]="hahaa.exe";
 if((fp=fopen(argv[0],"rb"))==NULL) 
 {
  printf("hehe error");
  getch();
 }

   for(int i=0;i<N;++i)
   {
    name[4]++;
    rewind(fp);
    if((out=fopen(name,"wb"))==NULL) 
     continue;
    //while(!feof(fp))
     putc(getc(fp),out); 
    fclose(out);
   }

 fclose(fp);
 printf("enjoy");
   return 0;
}
