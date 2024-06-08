#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>

void parseazaFisier(char* filename, int fd_fifo){
    //deschidem fisierul cu operatii
    int fd_file=open(filename, O_RDONLY);
    if(fd_file==-1){
        perror("Eroare la deschiderea fisierului cu operatii");
        exit(8);
    }
    //citim din fisier
    while(1){
        int line_length=0;
        char buffer[128]={0};

        int bytes_count=0;
        while((bytes_count=read(fd_file, &buffer[line_length], sizeof(char)))){
            if(bytes_count==-1 || bytes_count>1){
                perror("Eroare la citirea din fisierul cu opratii");
                exit(9);
            }
            //cand gasim un newline, il inlocuim cu NULL si oprim while-ul curent
            //pentru a procesa numerele si operatorul de pe linia respectiva
            if(buffer[line_length]=='\n'){
                buffer[line_length]='\0';
                break;
            }
            line_length++;
        }
        //Daca am terminat de citim tot din fisier, ne oprim
        if(bytes_count==0){
            break;
        }
        //Tokenizam sirul, impartind-ul in termen1, operatie, termen2
        char* t1=NULL, *op=NULL, *t2=NULL;
        char* p=strtok(buffer, " ");
        while(p!=NULL){
            if(t1==NULL) t1=p;
            else if((op=NULL)) op=p;
            else if(t2==NULL) t2=p;
            p=strtok(NULL, " ");
        }
        int T1, T2;
        char operator=op[0];
        T1=atoi(t1);
        T2=atoi(t2);
        printf("Am parsat %d , %c, %d\n", T1, operator, T2);
        //scriem datele in canalul fifo
        if(-1==write(fd_fifo, &T1, sizeof(int))){
            perror("Eroare la scrierea T1 in fifo");
            exit(10);
        }
        if(-1==write(fd_fifo, &operator, sizeof(char))){
            perror("Eroare la scrierea operatorului in fifo");
            exit(11);
        }
        if(-1==write(fd_fifo, &T2, sizeof(int))){
            perror("Eroare la scrierea T2 in fifo");
            exit(12);
        }
    }
}

void primesteDate(int* shm_obj){
    int sum=shm_obj[0]+shm_obj[1];
    printf("%d + %d = %d\n", shm_obj[0], shm_obj[1], sum);
}

int main(int argc, char* argv[]){
    //testam existenta unui argument primit in linia de comanda
    if(argc!=2){
        printf("Usage: ./supervisor [path]");
        exit(1);    
    }
    char* path_to_filename=argv[1];

    //initializam un canal fifo
    if(-1==mkfifo("sup_to_w1", 0600)){
        if(errno!=EEXIST){
            perror("Canalul fifo exista deja");
            exit(2);        
        }
    }
    //deschidem canalul fifo in mod WRITE ONLY
    int fdWF;
    if((fdWF=open("sup_to_w1", O_WRONLY))==-1){
        perror("Eroare la deschiderea canalului fifo");
        exit(4);
    }
    
    //initializam o mapare nepersistenta cu nume
    char* shm_name="/w2_to_sup";
    int shm_fd=shm_open(shm_name, O_RDWR|O_CREAT, 0600);
    if(shm_fd==-1){
        perror("Eroare la deschiderea maparii");
        exit(5);
    }
    //Setam size-ul la 2*sizeof(int), intrucat maparea va contine doar 2 numere intregi
    if(-1==ftruncate(shm_fd, 2*sizeof(int))){
        perror("Eroare la setarea marimii maparii");
        exit(6);
    }
    //Mapam fisierul
    int* shm_obj=mmap(NULL, 2*sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(shm_obj==MAP_FAILED){
        perror("Eroare la mapare");
        exit(7);
    }
    //dupa ce mapam, putem inchide deja shm_fd
    if(-1==close(shm_fd)){
        perror("Eroare la inchiderea fisierului de mapare");
        exit(15);
    }
    //parsam fisierul primit ca argument
    parseazaFisier(path_to_filename, fdWF);
    //Inchidem fifo-ul, deoarece nu mai avem nevoie de el, deja am transmis datele
    if(-1==close(fdWF)){
        perror("Eroare la inchiderea fisierului fifo");
        exit(13);
    }
    //lasam putin timp pentru ca worker2 sa scrie in mapare
    sleep(1);
    //Primim si lucram cu datele primite din worker2
    primesteDate(shm_obj);
    //stergem maparea
    if(-1==munmap(shm_obj, 2*sizeof(int))){
        perror("Eroare la stergerea maparii");
        exit(14);
    }


    return 0;
}
