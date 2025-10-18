#ifndef PERSIST_USERSTATS_TXT_H
#define PERSIST_USERSTATS_TXT_H

/* Stats por usuario y canción (opcional)
   Formato TXT (una línea por canción):
   song_id plays segundos_escucha
*/

typedef struct {
    int  song_id;
    int  plays;
    long segs;      /* segundos acumulados */
} tUserSongStat;

typedef struct {
    tUserSongStat* v;  /* array dinámico */
    int n;             /* usados */
    int cap;           /* capacidad */
} tUserStats;

/* Contenedor en RAM */
void ustats_init(tUserStats* S);
void ustats_free(tUserStats* S);

/* Agrega una reproducción:
   - si existe song_id: plays++ y segs += dur_seg
   - si no: inserta nueva entrada
   Retorna 1 ok, 0 sin memoria. */
int  ustats_add_play(tUserStats* S, int song_id, int dur_seg);

/* Persistencia TXT:
   Archivo por usuario, líneas: "song_id plays segs\n" */
int  ustats_load_txt(const char* path, tUserStats* S);
int  ustats_save_txt(const char* path, const tUserStats* S);

#endif /* PERSIST_USERSTATS_TXT_H */
