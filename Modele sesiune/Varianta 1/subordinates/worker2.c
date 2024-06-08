#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <string.h>

int citesteRezultate(int* shm_obj){
    int rez = 0;
    int minImp = __INT_MAX__;
    int maxPar = -9999;
    while(read(STDIN_FILENO,&rez,sizeof(int)) == 4){
            if(abs(rez)%2 == 0 && rez > maxPar){
                maxPar = rez;
            }
            if(abs(rez)%2 == 1 && rez < minImp){
                minImp = rez;
            }
    }
    if(maxPar == -9999) maxPar = 0;
    if(minImp == __INT_MAX__) minImp = 0;


    shm_obj[0]=maxPar;
    shm_obj[1]=minImp;
    return 0;
}

int main(int argc, char* argv[]){

     //initializam o mapare nepersistenta cu nume
    char* shm_name="/w2_to_sup";
    int shm_fd=shm_open(shm_name, O_CREAT|O_RDWR, 0600);
    if(shm_fd==-1){
        perror("Eroare la deschiderea maparii");
        exit(1);
    }
    //Setam size-ul la 2*sizeof(int), intrucat maparea va contine doar 2 numere intregi
    if(-1==ftruncate(shm_fd, 2*sizeof(int))){
        perror("Eroare la setarea marimii maparii");
        exit(2);
    }
    //Mapam fisierul
    int* shm_obj=mmap(NULL, 2*sizeof(int), PROT_READ|PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(shm_obj==MAP_FAILED){
        perror("Eroare la mapare");
        exit(3);
    }
    //dupa ce mapam, putem inchide deja shm_fd
    if(-1==close(shm_fd)){
        perror("Eroare la inchiderea fisierului de mapare");
        exit(4);
    }

    citesteRezultate(shm_obj);

    //stergem maparea
    if(-1==munmap(shm_obj, 2*sizeof(int))){
        perror("Eroare la stergerea maparii");
        exit(14);
    }
    close(STDIN_FILENO);
    return 0;
}