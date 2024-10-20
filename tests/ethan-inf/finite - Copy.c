#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int i, n = 0;
    char buf[512];
    for (i = 0; i < 10; i++)
    {
        if (n == 0)
            read(0, buf, 512);
        printf("%c\n", buf[i]);
        n--;
    }
    return 0;
}