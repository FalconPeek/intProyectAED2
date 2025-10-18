#include "persist_userstats_txt.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void ustats_init(tUserStats* S){
    S->v = NULL; S->n = 0; S->cap = 0;
}

void ustats_free(tUserStats* S){
    free(S->v);
    S->v = NULL; S->n = 0; S->cap = 0;
}

static int ensure_cap(tUserStats* S, int need){
    if(need <= S->cap) return 1;
    int ncap = (S->cap == 0 ? 8 : S->cap * 2);
    if(ncap < need) ncap = need;
    tUserSongStat* nv = (tUserSongStat*)realloc(S->v, ncap * sizeof(*nv));
    if(!nv) return 0;
    S->v = nv; S->cap = ncap;
    return 1;
}

int ustats_add_play(tUserStats* S, int song_id, int dur_seg){
    /* buscar si ya existe */
    for(int i=0; i<S->n; ++i){
        if(S->v[i].song_id == song_id){
            S->v[i].plays += 1;
            S->v[i].segs  += dur_seg;
            return 1;
        }
    }
    /* nuevo */
    if(!ensure_cap(S, S->n + 1)) return 0;
    S->v[S->n].song_id = song_id;
    S->v[S->n].plays   = 1;
    S->v[S->n].segs    = dur_seg;
    S->n++;
    return 1;
}

/* TXT por usuario: "song_id plays segs\n" */
int ustats_load_txt(const char* path, tUserStats* S){
    FILE* f = fopen(path, "r");
    if(!f){
        return (errno == ENOENT) ? 1 : 0;
    }
    int id, plays; long segs;
    int r = fscanf(f, " %d %d %ld", &id, &plays, &segs);
    while(!feof(f)){
        if(r == 3){
            if(!ensure_cap(S, S->n + 1)){ fclose(f); return 0; }
            S->v[S->n].song_id = id;
            S->v[S->n].plays   = plays;
            S->v[S->n].segs    = segs;
            S->n++;
        }
        r = fscanf(f, " %d %d %ld", &id, &plays, &segs);
    }
    fclose(f);
    return 1;
}

int ustats_save_txt(const char* path, const tUserStats* S){
    FILE* f = fopen(path, "w");
    if(!f) return 0;
    for(int i=0; i<S->n; ++i){
        if(fprintf(f, "%d %d %ld\n", S->v[i].song_id, S->v[i].plays, S->v[i].segs) <= 0){
            fclose(f); return 0;
        }
    }
    fclose(f);
    return 1;
}
