#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

#define MAX_FUNCS 32768
#define MAX_FILES 4096

typedef struct Edge {
    int to;
    struct Edge *next;
} Edge;

typedef struct {
    char name[128];
    char file[256];
    int file_idx;
    char *body;

    Edge *callees;

    int score;
    int visiting;
} Func;

typedef struct {
    char name[256];
    int score;
} File;

static Func funcs[MAX_FUNCS];
static int nfuncs = 0;

static File files[MAX_FILES];
static int nfiles = 0;

/* ---------- ignore dirs ---------- */

static const char *IGNORE_DIRS[] = {
    "meta",
    "usage",
    NULL
};

static int is_ignored(const char *name) {
    for (int i = 0; IGNORE_DIRS[i]; i++) {
        if (strcmp(name, IGNORE_DIRS[i]) == 0)
            return 1;
    }
    return 0;
}

/* ---------- utils ---------- */

static int is_ident(char c) {
    return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||(c>='0'&&c<='9')||c=='_';
}

static const char *rel(const char *p) {
    if (!strncmp(p,"../",3)) return p+3;
    if (!strncmp(p,"./",2)) return p+2;
    return p;
}

static int func_index(const char *name) {
    for (int i=0;i<nfuncs;i++)
        if (!strcmp(funcs[i].name,name)) return i;
    return -1;
}

static int file_id(const char *name) {
    for (int i=0;i<nfiles;i++)
        if (!strcmp(files[i].name,name)) return i;

    strcpy(files[nfiles].name,name);
    files[nfiles].score = 0;
    return nfiles++;
}

/* ---------- file read ---------- */

static char *read_file(const char *path) {
    FILE *f = fopen(path,"rb");
    if (!f) return NULL;

    fseek(f,0,SEEK_END);
    long sz = ftell(f);
    rewind(f);

    char *buf = malloc(sz+1);
    if (!buf) { fclose(f); return NULL; }

    if (fread(buf,1,(size_t)sz,f)!=(size_t)sz) {
        fclose(f);
        free(buf);
        return NULL;
    }

    buf[sz]=0;
    fclose(f);
    return buf;
}

/* ---------- scan ---------- */

static void add_func(const char *name, const char *file, char *body) {
    strcpy(funcs[nfuncs].name,name);
    strcpy(funcs[nfuncs].file,file);
    funcs[nfuncs].file_idx = file_id(file);
    funcs[nfuncs].body=body;
    funcs[nfuncs].callees=NULL;
    funcs[nfuncs].score=0;
    funcs[nfuncs].visiting=0;
    nfuncs++;
}

static void scan_file(const char *path) {
    char *text = read_file(path);
    if (!text) return;

    for (char *p=text;*p;p++) {
        if (*p!='(') continue;

        char *q=p-1;
        while(q>=text && !is_ident(*q)) q--;
        char *end=q;
        while(q>=text && is_ident(*q)) q--;
        char *start=q+1;

        int len=end-start+1;
        if (len<=0||len>=120) continue;

        char name[128];
        strncpy(name,start,len);
        name[len]=0;

        if (!strcmp(name,"if")||!strcmp(name,"for")||
            !strcmp(name,"while")||!strcmp(name,"switch")||
            !strcmp(name,"return"))
            continue;

        char *r = strchr(p,')');
        if (!r) continue;

        char *b = strchr(r,'{');
        if (!b) continue;

        int depth=1;
        char *s=b+1;

        while (*s && depth>0) {
            if (*s=='{') depth++;
            else if (*s=='}') depth--;
            s++;
        }
        if (depth!=0) continue;

        int blen = s-b;
        char *body = malloc(blen+1);
        memcpy(body,b,blen);
        body[blen]=0;

        add_func(name,path,body);

        p=s;
    }

    free(text);
}

/* ---------- walk ---------- */

static void walk(const char *dir) {
    DIR *d=opendir(dir);
    if(!d) return;

    struct dirent *ent;
    while((ent=readdir(d))){
        if(!strcmp(ent->d_name,".")||!strcmp(ent->d_name,".."))
            continue;

        if (is_ignored(ent->d_name))
            continue;

        char path[512];
        snprintf(path,sizeof(path),"%s/%s",dir,ent->d_name);

        struct stat st;
        if(stat(path,&st)!=0) continue;

        if(S_ISDIR(st.st_mode))
            walk(path);
        else if(strstr(ent->d_name,".c"))
            scan_file(path);
    }

    closedir(d);
}

/* ---------- edges ---------- */

static void add_edge(int from, int to) {
    if (from == to) return;

    for (Edge *e=funcs[from].callees;e;e=e->next)
        if (e->to==to) return;

    Edge *e = malloc(sizeof(*e));
    e->to = to;
    e->next = funcs[from].callees;
    funcs[from].callees = e;
}

static void build_edges() {
    for (int i=0;i<nfuncs;i++) {
        char *text = funcs[i].body;
        if (!text) continue;

        for (char *p=text;*p;p++) {
            if (*p!='(') continue;

            char *q=p-1;
            while(q>=text && !is_ident(*q)) q--;
            char *end=q;
            while(q>=text && is_ident(*q)) q--;
            char *start=q+1;

            int len=end-start+1;
            if (len<=0||len>=120) continue;

            char name[128];
            strncpy(name,start,len);
            name[len]=0;

            int fi = func_index(name);
            if (fi>=0)
                add_edge(i,fi);
        }
    }
}

/* ---------- scoring ---------- */

static int compute_score(int i) {
    if (funcs[i].score) return funcs[i].score;

    if (funcs[i].visiting)
        return 1;

    funcs[i].visiting = 1;

    int sum = 1;
    for (Edge *e=funcs[i].callees;e;e=e->next)
        sum += compute_score(e->to);

    funcs[i].visiting = 0;
    funcs[i].score = sum;
    return sum;
}

/* ---------- sorting ---------- */

static int cmp_file(const void *a, const void *b) {
    int fa = *(int*)a;
    int fb = *(int*)b;

    if (files[fa].score != files[fb].score)
        return files[fa].score - files[fb].score;

    return strcmp(files[fa].name, files[fb].name);
}

static int cmp_func_local(const void *a, const void *b) {
    int ia = *(int*)a;
    int ib = *(int*)b;

    if (funcs[ia].score != funcs[ib].score)
        return funcs[ia].score - funcs[ib].score;

    return strcmp(funcs[ia].name, funcs[ib].name);
}

/* ---------- main ---------- */

int main(int argc,char **argv){
    const char *root = (argc>1)?argv[1]:"..";

    walk(root);

    printf("Found %d functions\n", nfuncs);

    build_edges();

    for (int i=0;i<nfuncs;i++)
        compute_score(i);

    for (int i=0;i<nfuncs;i++) {
        files[funcs[i].file_idx].score += funcs[i].score;
    }

    int *file_counts = calloc(nfiles, sizeof(int));
    int **file_funcs = malloc(nfiles * sizeof(int*));

    for (int i=0;i<nfiles;i++)
        file_funcs[i] = malloc(nfuncs * sizeof(int));

    for (int i=0;i<nfuncs;i++) {
        int f = funcs[i].file_idx;
        file_funcs[f][file_counts[f]++] = i;
    }

    int *file_order = malloc(nfiles * sizeof(int));
    for (int i=0;i<nfiles;i++) file_order[i]=i;

    qsort(file_order, nfiles, sizeof(int), cmp_file);

    for (int k=0;k<nfiles;k++) {
        int f = file_order[k];
        if (file_counts[f]==0) continue;

        printf("\n--- %s (%d) ---\n",
            rel(files[f].name),
            files[f].score);

        qsort(file_funcs[f], file_counts[f], sizeof(int), cmp_func_local);

        for (int i=0;i<file_counts[f];i++) {
            int idx = file_funcs[f][i];

            printf("%s (%d)\n",
                funcs[idx].name,
                funcs[idx].score);

            for (Edge *e=funcs[idx].callees;e;e=e->next) {
                int c = e->to;
                printf("    %s %s\n",
                    rel(funcs[c].file),
                    funcs[c].name);
            }
        }
    }

    return 0;
}
