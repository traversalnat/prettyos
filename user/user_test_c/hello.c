#include "userlib.h"



int main()
{
    char s[1000];
    settextcolor(11,0);
    puts("--------------------------------------------------------------------\n");
    puts("Bitte geben Sie ihren Namen ein:\n");
    gets(s);
    puts("This piece of software was loaded by PrettyOS from the floppy disk of ");
    puts(s);
    putch('\n');
    puts("--------------------------------------------------------------------\n");
    settextcolor(15,0);
    return 0;
}
