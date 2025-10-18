#ifndef PERSIST_USERSTATS_TXT_H
#define PERSIST_USERSTATS_TXT_H

/* Interfaz muy simple:
   - add_play: acumula una reproducción de song_id en memoria (vector/tabla temporal).
   - load/save: leer/escribir archivo de stats del usuario.
   Para simplificar el TAD, usaremos un vector de pares (song_id, plays, segs). */

typedef struct {
    int  song_id;
    int  plays;
    long segs;
} tUserSongStat;

typedef struct {
    tUserSongStat* v;
    int n;      /* usados */
    int cap;    /* capacidad */
} tUserStats;

/* CRUD del contenedor en RAM */
void ustats_init(tUserStats* S);
void ustats_free(tUserStats* S);
int  ustats_add_play(tUserStats* S, int song_id, int dur_seg);

/* Persistencia a TXT por usuario */
int  ustats_load_txt(const char* path, tUserStats* S);
int  ustats_save_txt(const char* path, const tUserStats* S);

#endif /* PERSIST_USERSTATS_TXT_H */
