#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>

int citireFifo(int fd, int fdPipe){
    while(1){
        int t1 = 0,t2 = 0;
        char op = 0;
        int rez = 0;
        //Citim din supervisor tripletul de termen, operatie, termen
        int bytes_count = read(fd,&t1,sizeof(int));

        if(bytes_count == -1){
            perror("Eroare la citire!");
            exit(4);
        }
        if(bytes_count != sizeof(int)){
            return 3;
        }

        bytes_count = read(fd,&op,sizeof(char));
        if(bytes_count == -1){
            perror("Eroare la citire!");
            exit(4);
        }
        if(bytes_count != sizeof(char)){
            return 3;
        }

        bytes_count = read(fd,&t2,sizeof(int));
        if(bytes_count == -1){
            perror("Eroare la citire!");
            exit(4);
        }
        if(bytes_count != sizeof(int)){
            return 3;
        }
        //printf("%d %c %d",t1,op,t2);
        switch(op){
            case '+':   rez = t1+t2;    break;
            case '-':   rez = t1-t2;    break;
            case '*':   rez = t1*t2;    break;
            case '/':   rez = t1/t2;    break;
        }
        //printf(" = %d\n",rez);
        //Scriem apoi rezultatul in pipe-ul catre w2
        bytes_count = write(fdPipe,&rez,sizeof(int));
        if(-1 == bytes_count){
            perror("Eroare la scriere!");
            exit(5);
        }
        if(0 == bytes_count){
            printf("Am ajuns la final!");
            return 0;
        }
        
    }
    return 0;
}

int main(int argc, char* argv[]){

    //Initializam pipe-ul dintre w1 si w2
    int w1_to_w2[2];
    if(-1==pipe(w1_to_w2)){
        perror("Eroare la crearea pipe-ului");
        exit(1);
    }
    //Cream fiul
    pid_t pid_fiu=fork();
    if(pid_fiu==-1){
        perror("Eroare la crearea fiului");
        exit(2);
    }
    if(pid_fiu==0){
        //ZONA EXECUTATA DE FIU
        //inchidem capatul de scriere, deoarece nu avem nevoie de el
        close(w1_to_w2[1]);
        if(-1==dup2(w1_to_w2[0], STDIN_FILENO)){//stdin-ul fiului va fi capatul de citire din pipe
            perror("Eroare la redirectare");
            exit(3);
        }
        //acum inchidem si capatul de citire
        close(w1_to_w2[0]);
        //executam worker2
        execl("subordinates/worker2", "worker2", NULL);
        //urmatoarele se executa doar daca execl esueaza
        perror("Execl a esuat");
        exit(4);
    }else{
        //ZONA EXECUTATA DE TATA
        //initializam un canal fifo
        if(-1==mkfifo("sup_to_w1", 0600)){
            if(errno!=EEXIST){
                perror("Canalul fifo exista deja");
                exit(5);        
            }
        }
        //deschidem canalul fifo in mod WRITE ONLY
        int fdRF;
        if((fdRF=open("sup_to_w1", O_RDONLY))==-1){
            perror("Eroare la deschiderea canalului fifo");
            exit(7);
        }
        citireFifo(fdRF, w1_to_w2[1]);
        //inchidem fifo-ul
        if(-1==close(fdRF)){
            perror("Eroare la inchiderea fifo-ului");
            exit(8);
        }
        close(w1_to_w2[1]);
        close(w1_to_w2[0]);
    }
    return 0;
}