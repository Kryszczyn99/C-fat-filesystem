#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "fat.h"



int main(void)
{
    //Tutaj mozna zdefiniowac nazwe obrazu fat12
    FILE * obraz_fat=fopen("floppy.img","rb");
    if(obraz_fat==NULL)
    {
        printf("Obraz_fat nie zostal poprawnie wczytany\n");
        return 1;
    }
    
    //Jezeli udalo sie wczytac obraz_fat to mozna wczytac sektor startowy
    sektor_startowy s_s;
    int wczytane_bajty=fread(&s_s,1,sizeof(sektor_startowy),obraz_fat);
    if(wczytane_bajty!=sizeof(sektor_startowy))
    {
        printf("Sektor startowy nie zostal poprawnie wczytany\n");
        fclose(obraz_fat);
        return 1;
    }

    //Teraz mozna wczytac fat, lub faty(jezeli istnieja kopie)
    //rozmiar_fat podany jest w sektorach (mnozenie nizej przez s_s.bajty_na_sektor)
    //s_s.liczba_blokow_fat jest albo 1 albo 2 (zaleznie  czy jest kopia zapasowa)
    uint32_t rozmiar_obszaru_fat=s_s.bajty_na_sektor*(s_s.liczba_blokow_fat*s_s.rozmiar_fat);
    uint8_t * obszar_fat=(uint8_t *)malloc(rozmiar_obszaru_fat);
    if(obszar_fat==NULL)
    {
        printf("Alokacja obszaru_fat nie powiodla sie\n");
        return 1;
    }
    wczytane_bajty=fread(obszar_fat,1,rozmiar_obszaru_fat,obraz_fat);
    if(wczytane_bajty!=rozmiar_obszaru_fat)
    {
        printf("Obszar fat nie zostal poprawnie wczytany\n");
        return 1;
    }

    //Wczytywanie obszaru root
    //Z regu�y s_s.liczba_wpisow_root powinna wynosic 512
    uint32_t rozmiar_obszaru_root=s_s.liczba_wpisow_root*sizeof(wpis);
    wpis * obszar_root=(wpis*)malloc(rozmiar_obszaru_root);
    if(obszar_root==NULL)
    {
        printf("Alokacja obszaru_root nie powiodla sie\n");
        return 1;
    }
    wczytane_bajty=fread(obszar_root,1,rozmiar_obszaru_root,obraz_fat);
    if(wczytane_bajty!=rozmiar_obszaru_root)
    {
        printf("Obszar root nie zostal poprawnie wczytany\n");
        return 1;
    }

    //Na koniec wczytujemy pozostaly obszar gdzie fat trzyma dane
    
    //Dla fat12/16 sektory sa zapisane w s_s.liczba_sektorow_16
    //If ponizej jest dla bezpieczenstwa (tylko dla fat32)
    uint32_t liczba_wszystkich_sektorow=s_s.liczba_sektorow_16;
    if (liczba_wszystkich_sektorow==0) liczba_wszystkich_sektorow=s_s.liczba_sektorow_32;
    uint32_t rozmiar_calego_obrazu=liczba_wszystkich_sektorow*s_s.bajty_na_sektor; //rozmiar_fat w bajtach
    uint32_t rozmiar_obszaru_danych=rozmiar_calego_obrazu-rozmiar_obszaru_root-rozmiar_obszaru_fat-sizeof(s_s);
    uint8_t * obszar_danych=(uint8_t *)malloc(rozmiar_obszaru_danych);
    if(obszar_danych==NULL)
    {
        printf("Alokacja obszaru danych nie powiodla sie\n");
        return 1;
    }
    wczytane_bajty=fread(obszar_danych,1,rozmiar_obszaru_danych,obraz_fat);
    if(wczytane_bajty!=rozmiar_obszaru_danych)
    {
        printf("Obszar danych nie zostal poprawnie wczytany\n");
        return 1;
    }

    printf("Lista opcji:\n");
    printf("exit - kończy działanie programu\n");
    printf("dir - wyświetla listę plików oraz katalogów w bieżącym katalogu obrazu wraz z informacją o ich wielkości oraz dacie modyfikacji.\n");
    printf("cd <nazwa> - zmienia bieżący katalog obrazu nanazwalub wyświetla komunikat o błędzie.\n");
    printf("pwd - wyświetla nazwę aktualnego katalogu (ang. Print Working Directory)obrazu.\n");
    printf("cat <nazwa> - wyświetla zawartość pliku o nazwaz bieżącego katalogu obrazu.\n");
    printf("get <nazwa> - zapisuje (kopiuje) plik o nazwie nazwaz bieżącego katalogu obrazu do bieżącego katalogu systemu operacyjnego–polecenie „wyciąga” plik nazwa z obrazu woluminu i zapisuje go do „prawdziwego” katalogu.\n");
    printf("zip <nazwa1> <nazwa2> <nazwa3> - otwiera pliki nazwa1 oraz nazwa2 a następnie wczytuje z nich kolejne linie i zapisujeje naprzemiennie do pliku nazwa3 w bieżącym katalogu systemu operacyjnego.\n");
    printf("rootinfo - wyświetla informacje o katalogu głównym woluminu\n");
    printf("spaceinfo - wyświetla informacje o klastrach zajetych, wolnych, uszkodzonych, konczacych lancuchy klastrow\n");
    printf("fileinfo <nazwa> - wyświetla informacje o pliku nazwa z bieżącego katalogu obrazu\n");

    struct list_t * lista = ll_create();
    if(lista==NULL)
    {
        printf("Alokacja listy sie nie powiodla\n");
        free(obszar_fat);
        free(obszar_root);
        free(obszar_danych);
        fclose(obraz_fat);
        return 1;
    }

    char komenda[100];
    while (1)
    {
        char * test = fgets(komenda, 100, stdin);
        if(test==NULL)
        {
            printf("Wystapil problem przy wczytywaniu komendy\n");
            break;
        }
        char * pierwsza_czesc = strtok(test, " \n");
        if(!strcmp(pierwsza_czesc, "cd"))
        {
            char * druga_czesc = strtok(NULL, " \n");
            int error = 0;
            if(!strcmp(druga_czesc, "..")) 
            {
                ll_pop_back(lista, &error);
                if(error!=0)printf("Nie udalo sie zdjac ostatniego elementu z listy\n");
                continue;
            }
            int rozmiar = 0;
            //sprawdzam czy folder o nazwie "druga_czesc" znajduje sie w folderze w ktorym aktualnie jestesmy
            wpis * folder = zwroc_szukany_wpis(druga_czesc, obszar_fat, &rozmiar, obszar_danych, obszar_root, lista, &s_s);
            if(folder != NULL) ll_push_back(lista, folder);    
        }
        if(!strcmp(pierwsza_czesc, "cat"))
        {
            char * druga_czesc = strtok(NULL, " \n");
            int rozmiar_1 = 0;
            wpis * plik = zwroc_szukany_wpis(druga_czesc, obszar_fat, &rozmiar_1, obszar_danych, obszar_root, lista, &s_s);
            int rozmiar_2 = 0;
            void * tekst = zwroc_dane_spod_adresu(plik->adres_pierwszego_bloku_fat, &rozmiar_2, obszar_fat, obszar_danych, &s_s);
            printf("%s\n", (char *)tekst);
            free(tekst);
        }
        //"kopia cat tylko na koncu jest zapis do pliku zamiast wyswietlenia"
        if(!strcmp(pierwsza_czesc, "get"))
        {
            char * druga_czesc = strtok(NULL, " \n");
            int rozmiar_1 = 0;
            wpis * plik = zwroc_szukany_wpis(druga_czesc, obszar_fat, &rozmiar_1, obszar_danych, obszar_root, lista, &s_s);
            int rozmiar_2 = 0;
            void * tekst = zwroc_dane_spod_adresu(plik->adres_pierwszego_bloku_fat, &rozmiar_2, obszar_fat, obszar_danych, &s_s);
            FILE * f = fopen("get.txt", "w");
            if(f==NULL)
            {
                free(tekst);
                printf("Nie udalo otworzyc sie pliku\n");
                break;
            }
            fwrite(tekst, 1, plik->rozmiar_wpisu, f);
            fclose(f);
            free(tekst);
        }
        //"podwojna kopia funkcji get"
        if(!strcmp(pierwsza_czesc, "zip"))
        {
            char * druga_czesc = strtok(NULL, " ");
            char * trzecia_czesc = strtok(NULL, " ");
            char * czwarta_czesc = strtok(NULL, " \n");
            if(druga_czesc==NULL || trzecia_czesc == NULL || czwarta_czesc==NULL)
            {
                printf("Blad polecenia");
                continue;
            }
            int rozmiar_1 = 0;
            wpis * plik = zwroc_szukany_wpis(druga_czesc, obszar_fat, &rozmiar_1, obszar_danych, obszar_root, lista, &s_s);
            int rozmiar_2 = 0;
            void * tekst = zwroc_dane_spod_adresu(plik->adres_pierwszego_bloku_fat, &rozmiar_2, obszar_fat, obszar_danych, &s_s);
            FILE * f = fopen(czwarta_czesc, "w");
            if(f==NULL)
            {
                free(tekst);
                printf("Nie udalo otworzyc sie pliku\n");
                break;
            }
            fwrite(tekst, 1, plik->rozmiar_wpisu, f);
            fclose(f);
            free(tekst);
            rozmiar_1 = 0;
            plik = zwroc_szukany_wpis(trzecia_czesc, obszar_fat, &rozmiar_1, obszar_danych, obszar_root, lista, &s_s);
            rozmiar_2 = 0;
            tekst = zwroc_dane_spod_adresu(plik->adres_pierwszego_bloku_fat, &rozmiar_2, obszar_fat, obszar_danych, &s_s);
            FILE * f2 = fopen(czwarta_czesc, "a+");
            if(f2==NULL)
            {
                free(tekst);
                printf("Nie udalo otworzyc sie pliku\n");
                break;
            }
            fwrite(tekst, 1, plik->rozmiar_wpisu, f2);
            fclose(f2);
            free(tekst);
        }
        if(!strcmp(pierwsza_czesc, "fileinfo"))
        {
            char * druga_czesc = strtok(NULL, " \n");
            printf("R:");
            struct node_t* wsk=lista->head;
            while(wsk!=NULL)
            {
                printf("/%s", strtok(wsk->data->nazwa_pliku, " \n"));
                wsk=wsk->next;
            }
            printf("\n");
            int rozmiar = 0;
            wpis * plik = zwroc_szukany_wpis(druga_czesc, obszar_fat, &rozmiar, obszar_danych, obszar_root, lista, &s_s);      
            printf("Lista atrybutow:\n");
            printf("Archive: ");
            if(plik->atrybuty_wpisu&ARCHIWUM) printf("Tak\n");
            else printf("Nie\n");
            printf("Subfolder: ");
            if(plik->atrybuty_wpisu&FOLDER) printf("Tak\n");
            else printf("Nie\n");
            printf("Hidden Subfolder: ");
            if(plik->atrybuty_wpisu&UKRYTY_FOLDER) printf("Tak\n");
            else printf("Nie\n");
            printf("Volume label: ");
            if(plik->atrybuty_wpisu&ETYKIETA_WOLUMINU) printf("Tak\n");
            else printf("Nie\n");
            printf("Plik systemowy: ");
            if(plik->atrybuty_wpisu&PLIK_SYSTEMOWY) printf("Tak\n");
            else printf("Nie\n");
            printf("Tylko do odczytu: ");
            if(plik->atrybuty_wpisu&ODCZYT) printf("Tak\n");
            else printf("Nie\n");

            wypisz_klastry(plik->adres_pierwszego_bloku_fat, obszar_fat, &s_s);
            printf("Wielkosc: %u",plik->rozmiar_wpisu);
            printf("Utworzono: ");
            wyswietl_date(plik,1);
            wyswietl_godzine(plik,1);
            printf("\n");
            printf("Modyfikowano: ");
            wyswietl_date(plik,2);
            wyswietl_godzine(plik,2);
            printf("\n");
            printf("\n");
        }
        if(!strcmp(pierwsza_czesc, "exit"))
        {
            printf("Wybrano zamkniecie programu\n");
            break;
        }
        if(!strcmp(pierwsza_czesc, "pwd"))
        {
            printf("R:");
            struct node_t* wsk=lista->head;
            while(wsk!=NULL)
            {
                printf("/%s", strtok(wsk->data->nazwa_pliku, " \n"));
                wsk=wsk->next;
            }
            printf("\n");
        }
        
    	//kopia funkcji zwroc_szukany_wpis przerobiona na to aby printowac 
        if(!strcmp(pierwsza_czesc, "dir"))
        {
            //Aby wypisac wszystkie pliki w danym folderze nalezy pobrac folder w ktorym jestesmy
            int liczba_wpisow = 0;
            wpis * folder = zwroc_ostatni_folder(lista, obszar_root);
            if(folder==obszar_root) liczba_wpisow = s_s.liczba_wpisow_root;
            else
            {
                int rozmiar_obszaru_folderu = 0;
                //dostaniemy rozmiar w bajtach wiec nizej bedziemy chcieli to zamienic na liczbe wpisow i castowac na wpis*
                void * obszar_folderu = zwroc_dane_spod_adresu(folder->adres_pierwszego_bloku_fat, &rozmiar_obszaru_folderu, obszar_fat, obszar_danych, &s_s);
                liczba_wpisow = rozmiar_obszaru_folderu / sizeof(wpis);
                folder = (wpis *)(obszar_folderu);//castowanie na wpis*
            }
            int i = 0;
            while(i < liczba_wpisow)
            {
                if(!(folder[i].atrybuty_wpisu & UKRYTY_FOLDER||
                     folder[i].status_alokacji==USUNIETE||
                     folder[i].status_alokacji==NIE_ZAALOKOWANE)) {
                    printf("%s", strtok(folder[i].nazwa_pliku, " \n"));
                    char * reszta = strtok(NULL, " \n");
                    if(reszta != NULL && strlen(reszta)>=0 && isalpha(reszta[0])) printf(".%s\t", reszta);
                    else printf("\t\t");
                    printf("%u\t", folder[i].rozmiar_wpisu);
                    wyswietl_date(&folder[i], 1);
                    wyswietl_godzine(&folder[i], 1);
                    printf("\n");
                }
                i++;
            }
        }
        if(!strcmp(pierwsza_czesc, "spaceinfo"))
        {
            uint32_t klastry_zajete=0;
            uint32_t klastry_wolne=0;
            uint32_t klastry_uszkodzone=0;
            uint32_t klasty_konczace=0;
            uint32_t i = 0;
            uint16_t obecny_klaster = 0;
            //dzielimy przez 2/3 bo jest to odwrotnosc 1,5 (czyli fat 12, kt�ry zajmuje 1,5bajta)
            //s_s.liczba_blokow_fata zwraca ilosc 1 lub 2
            //s_s.rozmiar_fat zwraca ilosc sektorow
            //s_s.bajty_na_sektor zwraca w bajtach wartosc kazdego sektora
            while (i < (2*s_s.liczba_blokow_fat*s_s.bajty_na_sektor*s_s.rozmiar_fat)/3)//Calkowita liczba klastrow w obszarze Fat
            {
                obecny_klaster = nastepny_klaster_fat(obszar_fat, i, &s_s);
                if(obecny_klaster==wolny_klaster)klastry_wolne+=1;       
                else if(obecny_klaster==uszkodzony_klaster)klastry_uszkodzone+=1;
                else if(obecny_klaster>=konczacy_klaster)klasty_konczace+=1;
                else klastry_zajete+=1;     
                i++;    
            }
            //Statystki przestrzeni fat
            printf("Status klastrow obszaru fat:\n");
            printf("Liczba klastrow zajetych: %u\n", klastry_zajete);
            printf("Liczba klastrow wolne: %u\n", klastry_wolne);
            printf("Liczba klastrow uszkodzone: %u\n", klastry_uszkodzone);
            printf("Liczba klastrow konczace: %u\n", klasty_konczace);
            printf("Koniec spaceinfo\n");
        }
        if(!strcmp(pierwsza_czesc, "rootinfo"))
        {
            uint16_t uzyteczne_wpisy = 0;
            uint16_t i=0;
            double wykorzystywana_przestrzen = 0;
            //Przechodzimy po kazdym wpisie w przestrzeni root i sprawdzamy czy jest poprawna
            while(i < s_s.liczba_wpisow_root)
            {
                if(!(obszar_root[i].status_alokacji==DLUGA_NAZWA||
                    obszar_root[i].status_alokacji==NIE_ZAALOKOWANE||
                    obszar_root[i].status_alokacji==USUNIETE)) uzyteczne_wpisy+=1;
                i++;
            }
            wykorzystywana_przestrzen = (double)uzyteczne_wpisy / s_s.liczba_wpisow_root * 100.00;
            //Statystyki wpisow
            printf("Kompletna liczba wszystkich wpisow w roocie: %d\n", s_s.liczba_wpisow_root);
            printf("Wpisy w naszym roocie: %u\n", uzyteczne_wpisy);
            printf("Przestrzen naszego roota w uzyciu (wpisy/maksimum wpisow): %.1lf%%\n", wykorzystywana_przestrzen);
        }
    }
    
    free(obszar_fat);
    free(obszar_root);
    free(obszar_danych);
    fclose(obraz_fat);
    ll_clear(lista);
    return 0;
}
