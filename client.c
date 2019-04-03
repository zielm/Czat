/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   client.c
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


void wyswietlMenu();
void podgladList();
void wyslijWiadomoscInd();
void wyslijWiadomoscGrupowa();
void odbierzWiadomosc();
void zarzadzanieGrupami();
void wyloguj();

msg komunikat;
int idSerwera;
int idKlienta;
char login[16];
int idKolejkaWiadomosci;
int idKolejkaPriorytet;
int idKolejkaKomunikat;
int dzialaj, zalogowany;


int main() {
    
    idKlienta = getpid();
    idSerwera = 1;
    idKolejkaKomunikat = msgget(11111, IPC_CREAT|0644);
    idKolejkaPriorytet = msgget(77777, IPC_CREAT|0644);
    idKolejkaWiadomosci = msgget(88888, IPC_CREAT|0644);
    dzialaj = 1;
    zalogowany = 0;
    int glownaOpcja;
    

    while(dzialaj == 1) {
        
        // menu początkowe
        while(zalogowany == 0) {
            printf("\n******************************** \n");
            printf("               Czat             \n");
            printf("%s\n", komunikat.wiadomosc);
            printf("Wybierz opcje: \n 1 - zaloguj się\n 0 - wyłącz czat\n");
            scanf("%d", &glownaOpcja);
            switch(glownaOpcja) {

                // logowanie
                case 1:
                    printf("Zaloguj się:\n");
                    printf("Login: ");
                    scanf("%s", login);
                    printf("Hasło: ");
                    scanf("%s", komunikat.odbiorca);
                    setMsg(&komunikat, idSerwera, 0, 0, idKlienta, login, komunikat.odbiorca, "");
                    msgsnd(idKolejkaWiadomosci, &komunikat, rozmiarMsg, 0);
                    printf("Oczekuję na odpowiedź serwera...\n");
                    msgrcv(idKolejkaKomunikat, &komunikat, rozmiarMsg, idKlienta, 0);
                    if(komunikat.podtyp == 1) {
                        printf("Zostałeś zalogowany!");
                        zalogowany = 1;
                    }
                    break;

                // wyłączanie czatu
                case 0:
                    printf("Do zobaczenia!\n");
                    return (EXIT_SUCCESS);

                default:
                    printf("\n******************************** \n");
                    printf("Niepoprawny wybór\n");

            }
        }
        
        //menu
        printf("\n******************************** \n");
        wyswietlMenu();
        scanf("%d", &glownaOpcja);
        switch(glownaOpcja) {
            // wylogowywanie
            case 0:
                wyloguj();
                break;
            
            // podgląd listy    
            case 1:
                podgladList();
                break;    
                
            // wyślij wiadomość indywidualną   
            case 2:
                wyslijWiadomoscInd();
                break;
                
            // wyślij wiadomość grupową   
            case 3:
                wyslijWiadomoscGrupowa();
                break;    
                
            // odbierz wiadomość
            case 4:
                odbierzWiadomosc();
                break;
            
            // zarządzaj grupami    
            case 5:
                zarzadzanieGrupami();
                break;
                
            default:
                break;
        }
        

    } 
        
    
    return (EXIT_SUCCESS);
    
}

void wyswietlMenu() {
    printf("Witaj %s!\n", login);
    printf("Wybierz opcje: \n 1 - wyświetl listy \n 2 - wyślij wiadomość indywidualną\n");
    printf(" 3 - wyślij wiadomość do grupy \n 4 - odbierz wiadomość \n");
    printf(" 5 - zarządzaj grupami \n 0 - wyloguj \n -> ");
}

void wyloguj() {
    
    setMsg(&komunikat, idSerwera, 0, 1, idKlienta, "", "", "");
    msgsnd(idKolejkaWiadomosci, &komunikat, rozmiarMsg, 0);
    printf("\nOczekuję na odpowiedź serwera...\n");
    msgrcv(idKolejkaKomunikat, &komunikat, rozmiarMsg, idKlienta, 0);
    printf("%s\n", komunikat.wiadomosc);
    if(komunikat.podtyp == 1) {
        zalogowany = 0;                
    }
}

void wyslijWiadomoscInd() {
    int opcja;
    printf("***\nJaką wiadomość chcesz wysłać: \n 1 - zwykłą \n 2 - priorytetową \n 0 - powrót \n -> ");
    scanf("%d", &opcja);
    
    switch(opcja) {
        // powrót
        case 0:
            printf("Wracam do głównego menu\n");
            break;
        
        // wysylanie wiadomości zwykłej
        case 1:
            printf("Podaj do kogo chcesz wysłać wiadomość: ");
            scanf("%s", komunikat.odbiorca);     
            printf("Treść wiadomości: \n");
            scanf("%s", komunikat.wiadomosc);
            setMsg(&komunikat, idSerwera, 2, 1, idKlienta, login, komunikat.odbiorca, komunikat.wiadomosc);
            msgsnd(idKolejkaWiadomosci, &komunikat, rozmiarMsg, 0);
            printf("\nOczekuję na odpowiedź serwera...\n");
            msgrcv(idKolejkaKomunikat, &komunikat, rozmiarMsg, idKlienta, 0);
           
            printf("%s\n", komunikat.wiadomosc);
            sleep(1);
            break;
            
        // wysylanie wiadomości priorytetowej
        case 2:
            printf("Podaj do kogo chcesz wysłać wiadomość: ");
            scanf("%s", komunikat.odbiorca);     
            printf("Treść wiadomości: \n");
            scanf("%s", komunikat.wiadomosc);
            setMsg(&komunikat, idSerwera, 2, 0, idKlienta, login, komunikat.odbiorca, komunikat.wiadomosc);
            msgsnd(idKolejkaWiadomosci, &komunikat, rozmiarMsg, 0);
            printf("\nOczekuję na odpowiedź serwera...\n");
            msgrcv(idKolejkaKomunikat, &komunikat, rozmiarMsg, idKlienta, 0);
            printf("%s\n", komunikat.wiadomosc);
            sleep(1);
            break;
    }
}

void wyslijWiadomoscGrupowa() {
    int opcja;
    printf("***\nJaką wiadomość chcesz wysłać: \n 1 - zwykłą \n 2 - priorytetową \n 0 - powrót \n -> ");
    scanf("%d", &opcja);
    
    switch(opcja) {
        // powrót
        case 0:
            printf("Wracam do głównego menu\n");
            break;
        
        // zwykła do grupy
        case 1:
            printf("Podaj grupę do jakiej chcesz wysłać wiadomość: ");
            scanf("%s", komunikat.odbiorca);     
            printf("Treść wiadomości: \n");
            scanf("%s", komunikat.wiadomosc);
            setMsg(&komunikat, idSerwera, 3, 1, idKlienta, login, komunikat.odbiorca, komunikat.wiadomosc);
            msgsnd(idKolejkaWiadomosci, &komunikat, rozmiarMsg, 0);
            printf("\nOczekuję na odpowiedź serwera...\n");
            msgrcv(idKolejkaKomunikat, &komunikat, rozmiarMsg, idKlienta, 0);
           
            printf("%s\n", komunikat.wiadomosc);
            sleep(1);
            break;
            
        // priorytetowa do grupy
        case 2:
            printf("Podaj grupę do jakiej chcesz wysłać wiadomość: ");
            scanf("%s", komunikat.odbiorca);     
            printf("Treść wiadomości: \n");
            scanf("%s", komunikat.wiadomosc);
            setMsg(&komunikat, idSerwera, 3, 0, idKlienta, login, komunikat.odbiorca, komunikat.wiadomosc);
            msgsnd(idKolejkaWiadomosci, &komunikat, rozmiarMsg, 0);
            printf("\nOczekuję na odpowiedź serwera...\n");
            msgrcv(idKolejkaKomunikat, &komunikat, rozmiarMsg, idKlienta, 0);
            printf("%s\n", komunikat.wiadomosc);
            sleep(1);
            break;
    }
}

void odbierzWiadomosc() {
    int opcja = 1;
    while(opcja == 1) {
        if(msgrcv(idKolejkaPriorytet, &komunikat, rozmiarMsg, idKlienta, IPC_NOWAIT) != -1)
        {
            printf("Wiadomość priorytetowa od %s\n", komunikat.nadawca);
            printf("%s\n", komunikat.wiadomosc);
            sleep(1);
        }
    
        else if(msgrcv(idKolejkaWiadomosci, &komunikat, rozmiarMsg, idKlienta, IPC_NOWAIT) != -1)
        {
            printf("Wiadomość od %s\n", komunikat.nadawca);
            printf("%s\n", komunikat.wiadomosc);
            sleep(1);
        }
        
        else {
            printf("Brak nowych wiadomości\n");
            sleep(1);
            return;            
        }
        
        printf("***\n 1 - odbierz kolejną wiadomość \n 0 - wróć do menu głównego \n -> ");
        do {
            scanf("%d", &opcja);
            if(opcja == 0) return;
        } while(opcja != 1);
    }
    
    
}

void podgladList() {
    int opcja;
    char nazwaGrupy[16];
    printf("***\nWybierz listę, którą chcesz zobaczyć: \n 1 - zalogowani użytkownicy \n 2 - dostępne grupy \n 3 - użytkownicy grupy \n 0 - powrót \n -> ");
    scanf("%d", &opcja);
    
    switch(opcja) {
        // powrót
        case 0:
            printf("Wracam do głównego menu\n");
            break;
        
        // zalogowani użytkownicy
        case 1:
            setMsg(&komunikat, idSerwera, 1, 0, idKlienta, "", "", "");
            msgsnd(idKolejkaWiadomosci, &komunikat, rozmiarMsg, 0);
            printf("\nOczekuję na odpowiedź serwera...\n");
            msgrcv(idKolejkaKomunikat, &komunikat, rozmiarMsg, idKlienta, 0);
            if(komunikat.podtyp == 1) {
                printf("Lista zalogowanych użytkowników: \n");
            }
            printf("%s", komunikat.wiadomosc);
            printf("0 - powrót do głównego menu\n -> ");
            while(opcja != 0) {
                scanf("%d", &opcja);
            }
            break;
        
        // dostępne grupy
        case 2:
            setMsg(&komunikat, idSerwera, 1, 1, idKlienta, "", "", "");
            msgsnd(idKolejkaWiadomosci, &komunikat, rozmiarMsg, 0);
            printf("\nOczekuję na odpowiedź serwera...\n");
            msgrcv(idKolejkaKomunikat, &komunikat, rozmiarMsg, idKlienta, 0);
            if(komunikat.podtyp == 1) {
                printf("Lista grup: \n");
            }
            printf("%s\n", komunikat.wiadomosc);
            printf("0 - powrót do głównego menu\n -> ");
            while(opcja != 0) {
                scanf("%d", &opcja);
            }
            break;
        
        // uzytkownicy grupy    
        case 3:
            printf("Podaj nazwę grupy której członków chcesz zobaczyć: ");
            scanf("%s", nazwaGrupy); 
            setMsg(&komunikat, idSerwera, 1, 2, idKlienta, "", nazwaGrupy, "");
            msgsnd(idKolejkaWiadomosci, &komunikat, rozmiarMsg, 0);
            printf("\nOczekuję na odpowiedź serwera...\n");
            msgrcv(idKolejkaKomunikat, &komunikat, rozmiarMsg, idKlienta, 0);
            if(komunikat.podtyp == 1) {
                printf("Członkowie grupy: \n");
            }
            printf("%s\n", komunikat.wiadomosc);
            printf("0 - powrót do głównego menu\n -> ");
            while(opcja != 0) {
                scanf("%d", &opcja);
            }
            break;
            break;
            
    }
    
}

void zarzadzanieGrupami() {
    int opcja;
    char nazwaGrupy[16];
    printf("***\nWybierz co chcesz zrobić: \n 1 - zapisz się do grupy \n 2 - wypisz się z grupy \n 0 - powrót \n -> ");
    scanf("%d", &opcja);
    
    switch(opcja) {
        // powrót
        case 0:
            printf("Wracam do głównego menu\n");
            break;
        
        // zapis do grupy
        case 1:
            printf("Podaj nazwę grupy do jakiej chcesz się zapisać: ");
            scanf("%s", nazwaGrupy);     
            setMsg(&komunikat, idSerwera, 5, 0, idKlienta, "", nazwaGrupy, "");
            msgsnd(idKolejkaWiadomosci, &komunikat, rozmiarMsg, 0);
            printf("\nOczekuję na odpowiedź serwera...\n");
            msgrcv(idKolejkaKomunikat, &komunikat, rozmiarMsg, idKlienta, 0);
           
            printf("%s\n", komunikat.wiadomosc);
            sleep(1);
            break;
            
        // wypis z grupy
        case 2:
            printf("Podaj nazwę grupy z której chcesz się wypisać: ");
            scanf("%s", nazwaGrupy);     
            setMsg(&komunikat, idSerwera, 5, 1, idKlienta, "", nazwaGrupy, "");
            msgsnd(idKolejkaWiadomosci, &komunikat, rozmiarMsg, 0);
            printf("\nOczekuję na odpowiedź serwera...\n");
            msgrcv(idKolejkaKomunikat, &komunikat, rozmiarMsg, idKlienta, 0);
            printf("%s\n", komunikat.wiadomosc);
            sleep(1);
            break;
    }
    
    
}