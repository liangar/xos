#include <iostream.h>
#include "des.h"

void t_des(void);

int main(void)
{
    try{
        t_des();
    }
    catch(...){
        cerr << "some error occured." << endl;
    }
    return 0;
}

void t_des(void)
{
    int	ret, l;
    char * b = new char[2048], *p;
    deshandle h;
    h = 0;

    cout << "please enter a command or '|' to exit and '?' for help: ";
    while (1){
        cin.getline(b, 512);
        if (b[0] == '|')  break;
	if (b[0] == '?'){	// show usage
		cout << "usage :\n\t'|'           : exit the program"
	           "\n\t'o:<key string>'          : init des with the sp. key"
		   "\n\t'c:'                      : close the des"
		   "\n\t'd:<string for encode>'   : "
		   "\n\t'<string for encode>'     : encode a string with the sp. key"
		   "\n\t's:                       : show the decode result"
		   "\n\t'D:                       : show the decode result len" << endl;
	}else{
		if (b[1] != ':'){
			l = strlen(b);
			memmove(b+2, b, l+1);  b[0] = 'd';  b[1] = ':';
		}

                switch (b[0]){
                case 'o':  h = des_open(b+2);            
			   break;
                case 'c':  ret = des_close(h);
			   break;
                case 'd':  des_encode(h, b+1024, b+2);  
			   break;
		case 's':  des_decode(h, b+2, b+1024);
			   cerr << b+1024 << endl;
			   cerr << b+2 << endl;
			   break;
		case 'D':  ret = des_encode(h, b+1024, b+2, strlen(b+2)+1);
			   des_decode(h, b+2, b+1024, ret);
			   cerr << "the result string [len = " << ret << "] : "
			        << b+2 << endl;
			   break;
                default :  cerr << "unknow command" << endl;
                }
	}
    }
    if (h != 0){
	des_close(h);
    }
}
