#include <stdio.h>
#include <stdlib.h>

int main()
{
   FILE *in = fopen("input.txt", "r");
   char c;

   while((c = fgetc(in)) != EOF)
    {
        printf("%c",c);

    }

   fclose(in);
   return 42;
}
