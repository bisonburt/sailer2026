/*
   op  nin nout cur
   |   |   |   |   |
   |_|_|_|_|_|_|_|_|
*/



#define NONE 0x0
#define RIGHT 0x1
#define LEFT 0x2
#define BOTH 0x3

#define FALSE   0
#define TRUE    1

main()
{
unsigned int i;
unsigned int cur,newin,newout,op;
int point;

printf("static char runtable[128] = \n{\n");

    for (i=0;i<256;i++)
    {
        cur = i & 0x3;
        newout = (i>>2) & 0x3;
        newin = (i>>4) & 0x3;
        op = (i>>6) & 0x3;

        point = FALSE;

        switch(op)
        {
            case 0:
                if ((cur == LEFT && newout == LEFT) ||
                    (cur == RIGHT && newout == RIGHT))
                    point = FALSE;
                else if (cur != NONE)
                    point = TRUE;
            break;
            case 1:
                if (newout != NONE)
                    point = FALSE;
                else if (cur == BOTH)
                    point = TRUE;
            break;
            case 2:
//                if (cur == LEFT) /* bug */
                  if (cur == LEFT && newout != LEFT)
                    point = TRUE;
                else if (cur == BOTH && newout == RIGHT)
                    point = TRUE;
//  removing code below seems to fix the bug with objects
//  casting shadows on themselfs
//                else if (cur == BOTH && newin == RIGHT)
//                    point = TRUE;
            break;
        }
        if (point)
            printf("1,");
        else
            printf("0,");
        if (i % 32 == 31) printf("\n");
    }
    printf("};\n");
}