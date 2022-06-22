#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
#include "fat.h"

//wziete z linku ponizej
//https://wiki.osdev.org/FAT#FAT_12_2
uint16_t nastepny_klaster_fat(uint8_t * obszar_fat, uint16_t numer_klastra, sektor_startowy * s_s)
{
    if(obszar_fat==NULL || s_s==NULL || numer_klastra>=WARTOSC_EOC) return numer_klastra;

    uint16_t kopia_numeru_klastra = numer_klastra;
    //Pobieranie nastepnego numeru klastra
    uint32_t nastepny_klaster = numer_klastra + (numer_klastra/2);
    numer_klastra = *(uint16_t *)(&obszar_fat[nastepny_klaster]);

    //operacje bitowe fat 12 (aby poprawnie odczytac i przechodzic po fat12)
    if(kopia_numeru_klastra & 0x0001) numer_klastra = numer_klastra >> 4;
    else numer_klastra = numer_klastra & 0x0FFF;

    return numer_klastra;
}

//odczytujemy z listy ostatni folder jesli folder jest rootem to zwracamy root
wpis * zwroc_ostatni_folder(struct list_t * lista, wpis * obszar_root)
{
    if(lista==NULL) return NULL;
    int error = 0;
    wpis * folder = ll_back(lista, &error);
    if(folder==NULL) return obszar_root;
    return folder;
}

//funckja zwracajaca dane na temat konkretnego pliku (wpis 1 konkretnego pliku)
wpis * zwroc_szukany_wpis(char * nazwa_pliku, uint8_t * obszar_fat, uint32_t * rozmiar_pliku, uint8_t * obszar_danych, wpis * obszar_root, struct list_t * lista, sektor_startowy * s_s)
{
    int error = 0;
    
    //warunek na wpisanie cd .. (cofniecie sie o katalog nizej)
    if(!strcmp(nazwa_pliku, "..")) 
    {
        ll_pop_back(lista, &error);
        if(error!=0)printf("Nie udalo sie zdjac ostatniego elementu z listy\n");
        return NULL;
    }
    
    //alokowanie pamieci na nazwe pliku oraz zmiana wielkosci liter na male
    char * nazwa_pliku_tolower = (char *)calloc(strlen(nazwa_pliku), 1);
    if(nazwa_pliku_tolower==NULL)
    {
        printf("Alokacja pamieci w CD nie powiodla sie\n");
        return NULL;
    }
    for(unsigned int i = 0; i < strlen(nazwa_pliku); i++)
    {
        nazwa_pliku_tolower[i] = tolower(nazwa_pliku[i]);
    }
    
    uint32_t liczba_wpisow = 0;
    //zwracamy dane na temat ostatniego folderu
    wpis * folder = zwroc_ostatni_folder(lista, obszar_root);
    if(folder==obszar_root) liczba_wpisow = s_s->liczba_wpisow_root;
    else
    {
        int rozmiar_obszaru_folderu = 0;
        //odpalamy funkcje ktora zwroci nam obszar danych jednego folderu wraz z wielkoscia w bajtach
        //pozniej bedziemy iterowac sie po wpisach tego folderu
        void * obszar_folderu = zwroc_dane_spod_adresu(folder->adres_pierwszego_bloku_fat, &rozmiar_obszaru_folderu, obszar_fat, obszar_danych, s_s);
        liczba_wpisow = rozmiar_obszaru_folderu / sizeof(wpis);//tutaj zamieniamy bajtowy rozmiar na wpisowy
        folder = (wpis *)(obszar_folderu);
    }
    int i = 0;
    while(i < liczba_wpisow)
    {
    	//jesli folder jest poprawny 
        if(!(folder[i].atrybuty_wpisu & UKRYTY_FOLDER||
            folder[i].status_alokacji==USUNIETE||
            folder[i].status_alokacji==NIE_ZAALOKOWANE)) {
            	
            //wyodrebnienie nazwy pliku, alokowanie miejsca na nie oraz zmiana nazwy na male litery
            char * nazwa_p = strtok(folder[i].nazwa_pliku, " \n");
            char * nazwa_p_tolower = (char *)calloc(strlen(nazwa_p), 1);
            if(nazwa_p_tolower==NULL)
            {
                printf("Alokacja pamieci w CD nie powiodla sie\n");
                return NULL;
            }
            for(unsigned int i = 0; i < strlen(nazwa_pliku); i++)
            {
                nazwa_p_tolower[i] = tolower(nazwa_p[i]);
            }
            
            //dopoki nasz plik na ktorym iterator jest teraz nie jest rowny
            //temu co go szukamy nie wejdziemy w warunek if
            if(!strcmp(nazwa_pliku_tolower, nazwa_p_tolower))
            {
                wpis * plik = &folder[i];
                return plik;
            }
            free(nazwa_p_tolower);
        }
        i++;
    }
    free(nazwa_pliku_tolower);   
    return NULL;
}

//funkcja przechodzi po FAT area, odnosi sie do data area i zwraca zlepiony blok danych konkretnego pliku/folderu
void * zwroc_dane_spod_adresu(uint16_t adres_klastra_fat, uint32_t * rozmiar_folderu, uint8_t * obszar_fat, uint8_t * obszar_danych, sektor_startowy * s_s)
{
    if(rozmiar_folderu==NULL) return NULL;
    
    //Liczymy rozmiar jednego klastra (wynik w bajtach)
    uint32_t rozmiar_klastra_danych = s_s->bajty_na_sektor * s_s->sektory_na_kluster;
    //liczymy ilosc klastrow i mnozymy to przez rozmiar
    uint8_t * blok_danych = (uint8_t *)calloc(zwroc_liczbe_klastrow_folderu(adres_klastra_fat, obszar_fat, s_s) * rozmiar_klastra_danych, 1);
    if(blok_danych==NULL) return NULL;
    uint32_t iterator_bloku_danych = 0;
    while(1)
    {
        if(adres_klastra_fat>=konczacy_klaster) break;
        uint8_t * dane_klastra = pobierz_dane_spod_adresu(adres_klastra_fat, obszar_danych, rozmiar_klastra_danych);
        iterator_bloku_danych = zapisz_dane_do_bloku(blok_danych, iterator_bloku_danych, dane_klastra, rozmiar_klastra_danych);
        *rozmiar_folderu = iterator_bloku_danych;
        adres_klastra_fat = nastepny_klaster_fat(obszar_fat, adres_klastra_fat, s_s);
    }

    return (void *)blok_danych;
}


//funkcja zwracajaca wartosc ilosci klastrow dla danego folderu/pliku 
uint32_t zwroc_liczbe_klastrow_folderu(uint16_t adres_klastra_fat, uint8_t * obszar_fat, sektor_startowy * s_s)
{
    uint32_t liczba_klastrow = 0;
    while(1)
    {
        if(adres_klastra_fat>=konczacy_klaster)break;
        else
        {
            liczba_klastrow+=1;
            adres_klastra_fat = nastepny_klaster_fat(obszar_fat, adres_klastra_fat, s_s);
        }
    }
    return liczba_klastrow;
}


//zapisuje do zaalokowanego bloku dane znajdujace sie pod konkretnym adresem przekazanym w parametrze
uint8_t * pobierz_dane_spod_adresu(uint16_t adres_klastra_fat, uint8_t * obszar_danych, uint32_t rozmiar_klastra_danych)
{
    uint8_t * dane = (uint8_t *)calloc(rozmiar_klastra_danych, 1);
    if(dane == NULL) return NULL;

	//zwrocona nizej wartosc to NUMER KLASTRA (np:50)
    uint32_t adres_klastra_danych = adres_klastra_fat - 2; //magiczna liczba 2, bo 2 pierwsze klastry sa zarezerwowane (w data area)
    //trzeba pomnozyc to przykladowe 50(wyzej) przez rozmiar klastra by trafic w adres (bajty)
    adres_klastra_danych = adres_klastra_danych * rozmiar_klastra_danych;
	
    memcpy(dane, obszar_danych + adres_klastra_danych, rozmiar_klastra_danych);

    return dane;
}

//funkcja zapisujaca (dopisujaca) tekst na koniec bloku zaalokowanego wczesniej
uint32_t zapisz_dane_do_bloku(uint8_t * blok_danych, uint32_t iterator_bloku_danych, uint8_t * dane_klastra, uint32_t rozmiar_klastra_danych)
{
    memcpy(blok_danych + iterator_bloku_danych, dane_klastra, rozmiar_klastra_danych);
    iterator_bloku_danych+=rozmiar_klastra_danych;
    return iterator_bloku_danych;
}


//wyswietlanie daty w zaleznosci od wyboru
//utworzenie/ostatnia modyfikacja/dostep
//roz³ozenie bitow oraz magiczne mnozenie przez 2 zostalo wziete ze strony fat
void wyswietl_date(wpis * folder, uint8_t wybor)
{
    if(wybor==1)
    {
        uint32_t dzien    = (folder->data_utworzenia  & 0b0000000000011111);
        uint32_t miesiac  = ((folder->data_utworzenia & 0b0000000111100000) >> 5);
        uint32_t rok      = ((folder->data_utworzenia & 0b1111111000000000) >> 9) + 1980;
        printf("%u:%u:%u\t", rok, miesiac, dzien);
    }
    if(wybor==2)
    {
        uint32_t dzien    = (folder->data_ostatniej_modyfikacji  & 0b0000000000011111);
        uint32_t miesiac  = ((folder->data_ostatniej_modyfikacji & 0b0000000111100000) >> 5);
        uint32_t rok      = ((folder->data_ostatniej_modyfikacji & 0b1111111000000000) >> 9) + 1980;
        printf("%u:%u:%u\t", rok, miesiac, dzien);
    }
    if(wybor==3)
    {
        uint32_t dzien    = (folder->data_dostepu  & 0b0000000000011111);
        uint32_t miesiac  = ((folder->data_dostepu & 0b0000000111100000) >> 5);
        uint32_t rok      = ((folder->data_dostepu & 0b1111111000000000) >> 9) + 1980;
        printf("%u:%u:%u\t", rok, miesiac, dzien);
    }
}

//wyswietlenie godziny w zaleznosci od wyboru
//utworzenie/ostatnia modyfikacja
//roz³ozenie bitow oraz magiczne mnozenie przez 2 zostalo wziete ze strony fat
void wyswietl_godzine(wpis * folder, uint8_t wybor)
{
    if(wybor==1)
    {
        uint32_t sekundy = (folder->czas_utworzenia  & 0b0000000000011111) * 2;
        uint32_t minuty  = ((folder->czas_utworzenia & 0b0000011111100000) >> 5);
        uint32_t godziny = ((folder->czas_utworzenia & 0b1111100000000000) >> 11);
        printf("%u:%u:%u\t", godziny, minuty, sekundy);
    }
    if(wybor==2)
    {
        uint32_t sekundy = (folder->czas_ostatniej_modyfikacji  & 0b0000000000011111) * 2;
        uint32_t minuty  = ((folder->czas_ostatniej_modyfikacji & 0b0000011111100000) >> 5);
        uint32_t godziny = ((folder->czas_ostatniej_modyfikacji & 0b1111100000000000) >> 11);
        printf("%u:%u:%u\t", godziny, minuty, sekundy);
    }
}


//funkcja wypisujaca wszystkie klastry 
void wypisz_klastry(uint16_t adres, uint8_t * obszar_fat, sektor_startowy * s_s)
{
	int i=0;
    while(1)
    {
        printf("%hu ", adres);
        if(adres>=konczacy_klaster) break;
        adres=nastepny_klaster_fat(obszar_fat, adres, s_s);
        i+=1;
    }
    printf("Liczba klastrów: %d\n",i);
}


//wszystkie funkcje dot. listy dwukierunkowej (oparte o dante)
struct list_t* ll_create()
{
    struct list_t* wsk = (struct list_t*)malloc(sizeof(struct list_t));
    if (wsk == NULL) return NULL;
    wsk->head = NULL;
    wsk->tail = NULL;
    return wsk;
}

int ll_push_back(struct list_t* ll, wpis * value)
{
    if (ll == NULL) return 1;
    if (ll->head == NULL && ll->tail != NULL) return 1;
    if (ll->head != NULL && ll->tail == NULL) return 1;
    struct node_t* element = (struct node_t*)malloc(sizeof(struct node_t));
    if (element == NULL) return 2;
    element->data = value;
    element->next = NULL;
    element->prev = NULL;
    if (ll->head == NULL && ll->tail == NULL)
    {
        ll->head = element;
        ll->tail = element;
    }
    else
    {
        ll->tail->next = element;
        element->prev = ll->tail;
        ll->tail = element;
    }
    return 0;
}

int ll_pop_back(struct list_t* ll, int *err_code)
{
    if (ll == NULL)
    {
        if (err_code != NULL) *err_code = 1;
        return 0;
    }
    if (ll->head == NULL && ll->tail != NULL)
    {
        if (err_code != NULL) *err_code = 1;
        return 0;
    }
    if (ll->head != NULL && ll->tail == NULL)
    {
        if (err_code != NULL) *err_code = 1;
        return 0;
    }
    if (ll->head == NULL && ll->tail == NULL)
    {
        if (err_code != NULL) *err_code = 1;
        return 0;
    }
    int size = ll_size(ll);
    if (size == 1)
    {
        wpis * value = ll->head->data;
        free(ll->head);
        ll->head = NULL;
        ll->tail = NULL;
        if (err_code != NULL) *err_code = 0;
        return 0;
    }
    else
    {
        wpis * value = ll->tail->data;
        struct node_t* ptr = ll->tail->prev;
        ptr->next = NULL;
        free(ll->tail);
        ll->tail = ptr;
        if (err_code != NULL) *err_code = 0;
        return 0;
    }
    return 0;//dla spokoju kompilatora
}

wpis * ll_back(const struct list_t* ll, int *err_code)
{
    if (ll == NULL)
    {
        if (err_code != NULL) *err_code = 1;
        return NULL;
    }
    if (ll->head == NULL && ll->tail != NULL)
    {
        if (err_code != NULL) *err_code = 1;
        return NULL;
    }
    if (ll->head != NULL && ll->tail == NULL)
    {
        if (err_code != NULL) *err_code = 1;
        return NULL;
    }
    if (ll->head == NULL && ll->tail == NULL)
    {
        if (err_code != NULL) *err_code = 1;
        return NULL;
    }
    wpis * value = ll->tail->data;
    if (err_code != NULL) *err_code = 0;
    return value;
}

void ll_clear(struct list_t* ll)
{
    if (ll == NULL) return;
    if (ll->head != NULL && ll->tail != NULL)
    {
        struct node_t* wsk = ll->head;
        while (wsk != NULL)
        {
            struct node_t* temp = wsk->next;
            free(wsk);
            wsk = temp;
        }
    }
    ll->head=NULL;
    ll->tail=NULL;
}

int ll_size(const struct list_t* ll)
{
    if (ll == NULL) return -1;
    if (ll->head == NULL && ll->tail != NULL) return -1;
    if (ll->head != NULL && ll->tail == NULL) return -1;
    if (ll->head == NULL && ll->tail == NULL) return 0;
    struct node_t* wsk = ll->head;
    int size = 0;
    while (wsk != NULL)
    {
        size += 1;
        wsk = wsk->next;
    }
    return size;
}
