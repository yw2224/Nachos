#include "syscall.h"

int main()
{
    SpaceId newProc;
    OpenFileId input = ConsoleInput;
    OpenFileId output = ConsoleOutput;
    char prompt[2], ch, buffer[60];
    int i;

    prompt[0] = '-';
    prompt[1] = '-';

    while( 1 )
    {
	       Write(prompt, 2, output);

	       i = 0;
	       do {
	          Read(&buffer[i], 1, input);
	       } while( buffer[i++] != '\n' );

	       buffer[--i] = '\0';

         if(buffer[0] == 'x' && buffer[1] == ' ') {
            newProc = Exec(buffer + 2);
            Join(newProc);
         }
         else if(buffer[0] == 'p' && buffer[1] == 'w' && buffer[2] == 'd' && buffer[3] == '\0') {
            Pwd(); // unsolved bug here; fxxxxxxxxxxk;
         }
/*
	       if( i > 0 ) {
		        newProc = Exec(buffer);
		        Join(newProc);
	       }*/
    }
}
