#ifndef __FAT_H_
#define __FAT_H_

#include <stdint.h>

//https://wiki.osdev.org/FAT

typedef struct __attribute__((packed)) 
{
	//BIOS blok parametrów
    uint8_t  instrukcje_skoku[3];
    uint8_t  nazwa_OEM[8];
    uint16_t bajty_na_sektor;
    uint8_t  sektory_na_kluster;
    uint16_t zarezerwowane;
    uint8_t  liczba_blokow_fat;
    uint16_t liczba_wpisow_root;
    uint16_t liczba_sektorow_16;
    uint8_t  rodzaj_media;
    uint16_t rozmiar_fat; 
    uint16_t sektory_na_sciezke;                      
    uint16_t liczba_glowic;
    uint32_t liczba_sektorow_partycji_startowej;
    uint32_t liczba_sektorow_32;
    
    //FAT12 - reszta
    uint8_t  numer_napedu;
    uint8_t  pusta_przestrzen;
    uint8_t  rozszerzony_podpis_boot;
    uint32_t serial_woluminu;
    uint8_t  etykieta_woluminu[11];
    uint8_t  rodzaj_systemu[8];
    uint8_t  pusta_przestrzen_2[448];
    uint16_t podpis;
}sektor_startowy;


//Standard 8.3 format
typedef struct __attribute__((packed))
{
    union 
    {
        uint8_t status_alokacji;
        struct 
        {
            uint8_t nazwa_pliku[8];
            uint8_t rozszerzenie_pliku[3];
        };
    };
    uint8_t  atrybuty_wpisu;
    uint8_t  zarezerwowane;
    uint8_t  czas_utworzenia_ms;
    uint16_t czas_utworzenia;
    uint16_t data_utworzenia;
    uint16_t data_dostepu;
    uint16_t address_fat_32;
    uint16_t czas_ostatniej_modyfikacji;
    uint16_t data_ostatniej_modyfikacji;
    uint16_t adres_pierwszego_bloku_fat;
    uint32_t rozmiar_wpisu;
} wpis;

//zaimplementowanie listy dwukierunkowej
struct list_t
{
  struct node_t *head;
  struct node_t *tail;
};

struct node_t
{
  wpis * data;
  struct node_t *next;
  struct node_t *prev;
};

#define ARCHIWUM            0x20
#define USUNIETE            0xE5
#define DLUGA_NAZWA         0x0f
#define NIE_ZAALOKOWANE     0x00
#define FOLDER              0x10
#define PLIK_SYSTEMOWY      0x04
#define ETYKIETA_WOLUMINU   0x08
#define ODCZYT              0x01
#define UKRYTY_FOLDER       0x02
#define WARTOSC_EOC         0xFF7

#define wolny_klaster       0x000
#define uszkodzony_klaster  0xFF7
#define konczacy_klaster    0xFF8
//Poprawne klastry sa pomiedzy wolnymi i uszkodzonymi klastrami

uint16_t nastepny_klaster_fat(uint8_t * obszar_fat, uint16_t numer_klastra, sektor_startowy * s_s);
wpis * zwroc_ostatni_folder(struct list_t * lista, wpis * obszar_root);
wpis * zwroc_szukany_wpis(char * nazwa_pliku, uint8_t * obszar_fat, uint32_t * rozmiar_pliku, uint8_t * obszar_danych, wpis * obszar_root, struct list_t * lista, sektor_startowy * s_s);
void * zwroc_dane_spod_adresu(uint16_t adres_klastra_fat, uint32_t * rozmiar_folderu, uint8_t * obszar_fat, uint8_t * obszar_danych, sektor_startowy * s_s);
uint32_t zwroc_liczbe_klastrow_folderu(uint16_t adres_klastra_fat, uint8_t * obszar_fat, sektor_startowy * s_s);
uint8_t * pobierz_dane_spod_adresu(uint16_t adres_klastra_fat, uint8_t * obszar_danych, uint32_t rozmiar_klastra_danych);
uint32_t zapisz_dane_do_bloku(uint8_t * blok_danych, uint32_t iterator_bloku_danych, uint8_t * dane_klastra, uint32_t rozmiar_klastra_danych);
void wyswietl_date(wpis * folder, uint8_t wybor);
void wyswietl_godzine( wpis * folder, uint8_t wybor);
void wypisz_klastry(uint16_t adres, uint8_t * obszar_fat, sektor_startowy * s_s);
//funkcje dotycz¹ce listy dwukierunkowej
struct list_t* ll_create();
int ll_push_back(struct list_t* ll, wpis * value);
int ll_pop_back(struct list_t* ll, int *err_code);
wpis * ll_back(const struct list_t* ll, int *err_code);
void ll_clear(struct list_t* ll);
int ll_size(const struct list_t* ll);

#endif
