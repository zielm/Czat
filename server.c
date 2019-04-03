
/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   server.c
 * Author: zielm
 *
 * Created on 19 stycznia 2018, 21:32
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>


typedef struct msgbuf {
    long mType;
    int typ;
    int podtyp;
    char odbiorca[16];      // także haslo
    char nadawca[16];       // także login
    char wiadomosc[256];
    long mTypeNadawcy;
} msg;

int rozmiarMsg = sizeof(msg) - sizeof(long);

typedef struct user {
    long id;
    char login[16];
    char haslo[16];
    int aktywny;
} klient;

typedef struct group {
    char nazwa[16];
    klient *skladGrupy[64];
    int liczbaOsob;
} grupa;

/*
 * Wypełnianie treści wiadomości / komunikatu
 * @m wypełniana wiadomość
 * @mType do kogo wysyłamy
 * @typ typ komunikatu
 * @podtyp podtyp komunikatu
 * @mTypeNadawcy kto wysyła
 * @nadawca nazwa nadawcy
 * @odbiorca nazwa odbiorcy
 * @wiadomosc tresc wiadomosci
 */
void setMsg(msg *m, long mType, int typ, int podtyp, long mTypeNadawcy, char nadawca[16], char odbiorca[16], char wiadomosc[128]) {
    m->mType = mType;
    m->typ = typ;
    m->podtyp = podtyp;
    m->mTypeNadawcy = mTypeNadawcy;
    strcpy(m->nadawca, nadawca);
    strcpy(m->odbiorca, odbiorca);
    strcpy(m->wiadomosc, wiadomosc);
}

int wczytajUzytkownikow(klient *uzytkownicy) {
    int fd = open("users.txt", O_RDONLY);
    int nrUzytkownika = 0;
    char c;
    while(read(fd, &c, 1) > 0) {
        char buf[16];
        // odczytaj login
        int nrZnaku = 0;
        do {
            buf[nrZnaku] = c;    
            nrZnaku++;
            read(fd, &c, 1);
        } while(c != ' ');
        strcpy(uzytkownicy[nrUzytkownika].login, buf);
        
        int i;
        for(i = 0; i < 16; i++) {
            buf[i] = 0;
        }
        nrZnaku = 0;
        
        //odczytanie znaku za spacją
        read(fd, &c, 1);
        
        // odczytaj hasło
        while (c != '\n') {
            buf[nrZnaku] = c;
            nrZnaku++;
            read(fd, &c, 1);
        }
        strcpy(uzytkownicy[nrUzytkownika].haslo, buf);
        uzytkownicy[nrUzytkownika].aktywny = 0;
        uzytkownicy[nrUzytkownika].id = 0;
        nrUzytkownika++;
        
    }
    //printf("***** Wczytano użytkowników *****\n");
    close(fd);
    return nrUzytkownika;
}


int wczytajGrupy(grupa *grupy) {
    int fd = open("groups.txt", O_RDONLY);
    int nrGrupy = 0;
    char c;
    while(read(fd, &c, 1) > 0) {
        char buf[16];
        // odczytaj nazwy
        int nrZnaku = 0;
        do {
            buf[nrZnaku] = c;    
            nrZnaku++;
            read(fd, &c, 1);
        } while(c != '\n');
        strcpy(grupy[nrGrupy].nazwa, buf);  
        grupy[nrGrupy].liczbaOsob = 0;
        nrGrupy++;   
    }
    //printf("***** Wczytano grupy *****\n");
    close(fd);
    return nrGrupy;
}


void logowanie();
void podgladList();
void obslugaGrupy();
void wysylanieWiadomosciInd();
void wysylanieWiadomosciGrupowej();


msg komunikat;
klient uzytkownicy[100];
grupa grupy[30];
int liczbaUzytkownikow;
int liczbaGrup;

int idKolejkaWiadomosci;
int idKolejkaPriorytet;
int idKolejkaKomunikat;

int idSerwera;
int idKlienta;

int main() {
    
    liczbaUzytkownikow = wczytajUzytkownikow(uzytkownicy);
    liczbaGrup = wczytajGrupy(grupy);

    
    idSerwera = 1;
    idKolejkaWiadomosci = msgget(88888, IPC_CREAT|0644);
    idKolejkaPriorytet = msgget(77777, IPC_CREAT|0644);
    idKolejkaKomunikat = msgget(11111, IPC_CREAT|0644);
    
    printf("\n******************************** \n");
    printf("          Serwer czatu             \n");
    while (1 == 1){
        
        idKlienta = -1;
        msgrcv(idKolejkaWiadomosci, &komunikat, rozmiarMsg, idSerwera, 0);
        idKlienta = komunikat.mTypeNadawcy;
        
        switch(komunikat.typ) {
            // obsługa logowania
            case 0:
                printf("*** \nWłączono obsługę logowania\n");
                logowanie();
                printf("%s\n", komunikat.wiadomosc);
                printf("Zakończono obsługę logowania\n*** \n");
                break;
            
            // podgląd list
            case 1:
                printf("*** \nWłączono podgląd listy\n");
                podgladList();
                printf("Zakończono podgląd list\n*** \n");
                break;
            
            // wysyłanie wiadomości prywatnej
            case 2:
                printf("*** \nWysłano wiadomość indywidualną \n");
                wysylanieWiadomosciInd();
                printf("%s\n", komunikat.wiadomosc);
                printf("Przesłano wiadomość indywidualną\n*** \n");
                break;
                
            // wysyłanie wiadomości grupowej
            case 3:
                printf("*** \nWysłano wiadomość do grupy\n");
                wysylanieWiadomosciGrupowej();
                printf("%s\n", komunikat.wiadomosc);
                printf("przesłano wiadomość do grupy\n*** \n");
                break;

                
            // obsługa grupy
            case 5:
                printf("*** \nWłączono obsługę grupy\n");
                obslugaGrupy();
                printf("%s\n", komunikat.wiadomosc);
                printf("Zakończono obsługę grupy\n*** \n");
                break;

        }

    }

}

void logowanie() {
    int i;
    int nrUzytkownika = -1;
    
    //zaloguj
    if(komunikat.podtyp == 0) {
        printf("Użytkownik próbuje się zalogować... \n");
        for(i = 0; i<liczbaUzytkownikow; i++) {
            if(strcmp(uzytkownicy[i].login, komunikat.nadawca) == 0) {
                nrUzytkownika = i;
                if(strcmp(uzytkownicy[i].haslo, komunikat.odbiorca) == 0) {
                    if(uzytkownicy[i].aktywny == 0) {
                        uzytkownicy[i].id = idKlienta;
                        uzytkownicy[i].aktywny = 1;
                        setMsg(&komunikat, idKlienta, 9, 1, idSerwera, "","", "Jesteś zalogowany!");
                    }
                    else {
                        setMsg(&komunikat, idKlienta, 9, -1, idSerwera, "","", "Użytkownik o takim loginie jest już zalogowany!");
                    }    
                }
                else {
                    setMsg(&komunikat, idKlienta, 9, -1, idSerwera, "","", "Podałeś niepoprawne hasło!");
                }
                break;
            }
        }
        
        if(nrUzytkownika == -1) {
            setMsg(&komunikat, idKlienta, 9, -1, idSerwera, "","", "Użytkownik o takim loginie nie istnieje!");
        }
        
        printf("Zakończono procedurę logowania.\n");    
        msgsnd(idKolejkaKomunikat, &komunikat, rozmiarMsg, 0);
        return;
    }
    
    //wyloguj
    else if(komunikat.podtyp == 1) {
        printf("Użytkownik chce się wylogować... \n");
        
        for(i = 0; i < liczbaUzytkownikow; i++) {
            if(uzytkownicy[i].id == idKlienta) {
                uzytkownicy[i].aktywny = 0;
                uzytkownicy[i].id = 0;
                setMsg(&komunikat, idKlienta, 9, 1, idSerwera, "","", "Zostałeś poprawnie wylogowany");
                printf("Zakończono procedurę wylogowywania.\n"); 
                msgsnd(idKolejkaKomunikat, &komunikat, rozmiarMsg, 0);
                return;
            } 
        }
    }
    
    setMsg(&komunikat, idKlienta, 9, -1, idSerwera, "","", "Przepraszamy, wystąpił nieznany błąd.");
    printf("Wystąpił nieznany błąd.\n"); 
    msgsnd(idKolejkaKomunikat, &komunikat, rozmiarMsg, 0);
    return;
    
}

void podgladList() {
    
    int i, nrGrupy;
    char lista[256] = "";
    switch(komunikat.podtyp) {

        // lista zalogowanych użytkowników
        case 0:
            printf("Wyświetlam listę użytkowników\n");
            for(i = 0; i < liczbaUzytkownikow; i++)
            {
                if(uzytkownicy[i].aktywny == 1) {
                    strcat(lista, uzytkownicy[i].login);
                    strcat(lista,"\n");
                }
            }
            setMsg(&komunikat, idKlienta, 9, 1, idSerwera, "","", lista);
            break;
            
        // lista grup    
        case 1:
            printf("Wyświetlam listę grup\n");
            for(i = 0; i < liczbaGrup; i++)
            {
                strcat(lista, grupy[i].nazwa);
                strcat(lista,"\n");
            }
            setMsg(&komunikat, idKlienta, 9, 1, idSerwera, "","", lista);
            break;
        
        // lista użytkowników grupy    
        case 2:
            nrGrupy = -1;
            printf("Wyświetlam listę użytkowników grupy\n");
            for(i = 0; i < liczbaGrup; i++)
            {
                if(strcmp(komunikat.odbiorca, grupy[i].nazwa) == 0) {
                    nrGrupy = i;
                    int j;
                    
                    // czy ktoś jest zapisany do grupy
                    if(grupy[i].liczbaOsob == 0) {
                        setMsg(&komunikat, idKlienta, 9, 1, idSerwera, "","", "Grupa jest pusta");
                        break;
                    }
                    
                    // wypisz kto jest w grupie
                    for(j = 0; j < grupy[i].liczbaOsob; j++) {
                        strcat(lista, grupy[i].skladGrupy[j]->login);
                        strcat(lista,"\n");  
                    }
                    setMsg(&komunikat, idKlienta, 9, 1, idSerwera, "","", lista);
                    break;
                }
            }
            if(nrGrupy == -1) {
                setMsg(&komunikat, idKlienta, 9, -1, idSerwera, "","", "Grupa o podanej nazwie nie istnieje.");
            }
            break;
    }
    
    if(komunikat.typ != 9) {
        setMsg(&komunikat, idKlienta, 9, -1, idSerwera, "","", "Przepraszamy, wystąpił nieznany błąd");
    }
    
    msgsnd(idKolejkaKomunikat, &komunikat, rozmiarMsg, 0);
    return;
}

void wysylanieWiadomosciInd() {
    int i, idOdbiorcy = -1;

    // id odbiorcy
    for(i = 0; i < liczbaUzytkownikow; i++) {
       if(strcmp(komunikat.odbiorca, uzytkownicy[i].login) == 0) {
           idOdbiorcy = uzytkownicy[i].id;
           break;
       }
    }
        
    if(idOdbiorcy == -1) {
        setMsg(&komunikat, idKlienta, 9, -1, idSerwera, "", "", "Odbiorca nie istnieje");
        msgsnd(idKolejkaKomunikat, &komunikat, rozmiarMsg, 0);
        return;
    }
    
    if(uzytkownicy[i].aktywny == 0) {
        setMsg(&komunikat, idKlienta, 9, -1, idSerwera, "", "", "Użytkownik nie jest obecnie zalogowany");
        msgsnd(idKolejkaKomunikat, &komunikat, rozmiarMsg, 0);
        return;
    }
    
    switch(komunikat.podtyp) {
        // priorytet
        case 0:
            printf("Wysyłanie wiadomości priorytetowej\n");
            setMsg(&komunikat, idOdbiorcy, 4, 0, idKlienta, komunikat.nadawca, komunikat.odbiorca, komunikat.wiadomosc);
            msgsnd(idKolejkaPriorytet, &komunikat, rozmiarMsg, 0);
            setMsg(&komunikat, idKlienta, 9, 1, idSerwera, "", "", "Wiadomość priorytetowa została wysłana");
            msgsnd(idKolejkaKomunikat, &komunikat, rozmiarMsg, 0);
            break;
            
        // zwykla   
        case 1:
            printf("Wysyłanie zwykłej wiadomości\n");
            setMsg(&komunikat, idOdbiorcy, 4, 1, idKlienta, komunikat.nadawca, komunikat.odbiorca, komunikat.wiadomosc);
            msgsnd(idKolejkaWiadomosci, &komunikat, rozmiarMsg, 0);
            setMsg(&komunikat, idKlienta, 9, 1, idSerwera, "", "", "Wiadomość została wysłana");
            msgsnd(idKolejkaKomunikat, &komunikat, rozmiarMsg, 0);
            break;
            
        default:
            break;
    }
            
    
    
}

void wysylanieWiadomosciGrupowej() {
    int i, nrGrupy = -1, ile = 0;
    // id grupy
    for(i = 0; i < liczbaGrup; i++) {
       if(strcmp(komunikat.odbiorca, grupy[i].nazwa) == 0) {
           nrGrupy = i;
           break;
       }
    }
    
    if(nrGrupy == -1) {
        setMsg(&komunikat, idKlienta, 9, -1, idSerwera, "", "", "Grupa nie istnieje");
        msgsnd(idKolejkaKomunikat, &komunikat, rozmiarMsg, 0);
        return;
    }
    
    if(grupy[nrGrupy].liczbaOsob == 0) {
        setMsg(&komunikat, idKlienta, 9, -1, idSerwera, "", "", "Nikt nie należy do tej grupy");
        msgsnd(idKolejkaKomunikat, &komunikat, rozmiarMsg, 0);
        return;
    }
    
    switch(komunikat.podtyp) {

        // priorytet
        case 0:
            printf("Wysyłanie wiadomości priorytetowej\n");
            for(i = 0; i < grupy[nrGrupy].liczbaOsob; i++) {
                if(grupy[nrGrupy].skladGrupy[i]->aktywny == 1) {
                    setMsg(&komunikat, grupy[nrGrupy].skladGrupy[i]->id, 4, 0, idKlienta, komunikat.nadawca, komunikat.odbiorca, komunikat.wiadomosc);
                    msgsnd(idKolejkaPriorytet, &komunikat, rozmiarMsg, 0);
                    ile++;
                }
            }
            if(ile == 0) {
                setMsg(&komunikat, idKlienta, 9, -1, idSerwera, "", "", "Żadna osoba należąca do grupy nie jest obecnie zalogowana");
            }
            else {
                setMsg(&komunikat, idKlienta, 9, 1, idSerwera, "", "", "Wiadomość priorytetowa do grupy została wysłana");
            }
            msgsnd(idKolejkaKomunikat, &komunikat, rozmiarMsg, 0);
            return;
            
        // zwykla   
        case 1:
            printf("Wysyłanie zwykłej wiadomości\n");
            for(i = 0; i < grupy[nrGrupy].liczbaOsob; i++) {
                if(grupy[nrGrupy].skladGrupy[i]->aktywny == 1) {
                    setMsg(&komunikat, grupy[nrGrupy].skladGrupy[i]->id, 4, 1, idKlienta, komunikat.nadawca, komunikat.odbiorca, komunikat.wiadomosc);
                    msgsnd(idKolejkaWiadomosci, &komunikat, rozmiarMsg, 0);
                    ile++;
                }
            }
            if(ile == 0) {
                setMsg(&komunikat, idKlienta, 9, -1, idSerwera, "", "", "Żadna osoba należąca do grupy nie jest obecnie zalogowana");
            }
            else {
                setMsg(&komunikat, idKlienta, 9, 1, idSerwera, "", "", "Wiadomość do grupy została wysłana");
            }
            msgsnd(idKolejkaKomunikat, &komunikat, rozmiarMsg, 0);
            return;
            
        default:
            break;
    }
    
}

void obslugaGrupy() {
    
   int i, nrGrupy = -1, nrKlienta;
    
   for(i = 0; i < liczbaUzytkownikow; i++) {
       if(idKlienta == uzytkownicy[i].id) {
            nrKlienta = i;
            break;
        }
    }
    
    switch(komunikat.podtyp) {

        // zapis do grupy
        case 0:
            printf("Próbuję zapisać do grupy\n");
            for(i = 0; i < liczbaGrup; i++)
            {
                if(strcmp(komunikat.odbiorca, grupy[i].nazwa) == 0) {
                    nrGrupy = i;
                    int j;
                    // czy nie jest zapisany do tej grupy
                    for(j = 0; j < grupy[i].liczbaOsob; j++) {
                        if(idKlienta == grupy[i].skladGrupy[j]->id) {
                            setMsg(&komunikat, idKlienta, 9, -1, idSerwera, "", "", "Już należysz do tej grupy!");
                            msgsnd(idKolejkaKomunikat, &komunikat, rozmiarMsg, 0);
                            return;
                        }                       
                    }
                    printf("Zapisuje do grupy\n");
                    grupy[i].skladGrupy[grupy[i].liczbaOsob] = &uzytkownicy[nrKlienta];
                    grupy[i].liczbaOsob++;
                    setMsg(&komunikat, idKlienta, 9, 1, idSerwera, "", "", "Zostałeś dodany do grupy.");
                    break;
                }
            }
            if(nrGrupy == -1) {
                setMsg(&komunikat, idKlienta, 9, -1, idSerwera, "","", "Grupa o podanej nazwie nie istnieje.");
            }
            break;
            
        // wypisz z grupy   
        case 1:
            printf("Próbuję wypisać z grupy\n");
            for(i = 0; i < liczbaGrup; i++)
            {
                if(strcmp(komunikat.odbiorca, grupy[i].nazwa) == 0) {
                    nrGrupy = i;
                    int j;
                    
                    // czy jest zapisany do tej grupy
                    for(j = 0; j < grupy[i].liczbaOsob; j++) {
                        if(idKlienta == grupy[i].skladGrupy[j]->id) {
                            break;
                        }
                    }
                    
                    if (j == grupy[i].liczbaOsob) {
                        setMsg(&komunikat, idKlienta, 9, -1, idSerwera, "", "", "Nie należysz do tej grupy.");
                        break;
                    }
                    
                    for(j = j; j < grupy[i].liczbaOsob - 1; j++) {
                        grupy[i].skladGrupy[j] = grupy[i].skladGrupy[j+1];
                    }
                    
                    grupy[i].skladGrupy[j] = NULL;
                        

                    printf("Wypisuje z grupy\n");
                    grupy[i].liczbaOsob--;
                    setMsg(&komunikat, idKlienta, 9, 1, idSerwera, "", "", "Zostałeś wypisany z grupy.");
                    break;
                }
            }
            if(nrGrupy == -1) {
                setMsg(&komunikat, idKlienta, 9, -1, idSerwera, "","", "Grupa o podanej nazwie nie istnieje.");
            }
            break;
          
    }
    
    if(komunikat.typ != 9) {
        setMsg(&komunikat, idKlienta, 9, -1, idSerwera, "","", "Przepraszamy, wystąpił nieznany błąd");
    }
    
    msgsnd(idKolejkaKomunikat, &komunikat, rozmiarMsg, 0);
    return;
    
}