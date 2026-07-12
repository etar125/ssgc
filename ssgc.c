/*
Copyright (c) 2026 etar125 Admanse

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED “AS IS” AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
/* #include <sys/wait.h> */
#include <e1l.h>

#define ESTRDUPL
#include "estrdup.h"

#define _P "(prefix unset)"

#define llen(x) sizeof(x)/sizeof(x[0])
#define _e goto error
#define __e(x) perror(_P" "x); goto error
#define __f(x) fprintf(stderr, _P" "x); goto error
#define _f(x) fprintf(stderr, _P" "x)

#include "ssgc.h"

#define EXECC_BUFSIZE 4096
#define COPY_BUFSIZE 4096

typedef struct {
    char *name;
    size_t namelen;
    char *vname;
    size_t vnamelen;
    char *parent;
    size_t parentlen;
} sdir;

static const char *rootdir;
static size_t rootlen;

static ssg_rp breplaces[] = {
    { "\\{", "{", 2 },
    { "{{up}}", NULL, 6 },
    { "{{home}}", "/", 8 },
    { "{{sitename}}", NULL, 12 },
    { "{{dirname}}", NULL, 11 },
    { "{{content}}", NULL, 11 },
    { "{{alias}}", NULL, 9 },
};

static char* execc(const char *cmd, size_t *outlen) {
    int pipefd[2];
    pid_t pid;
    char *ds = NULL, *ret = NULL, t[EXECC_BUFSIZE];
    size_t asize = 0, bsize = 0;
    int bread;

    if (pipe(pipefd) == -1) { return NULL; }

    pid = fork();

    if (pid == -1) {
        close(pipefd[0]);
        close(pipefd[1]);
        return NULL;
    }

    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);

        execl("/bin/sh", "sh", "-c", cmd, NULL);
        _exit(127);
    }

    close(pipefd[1]);

    while (1) {
        bread = read(pipefd[0], t, EXECC_BUFSIZE);
        if (bread <= 0) { break; }

        if (d_append(&ds, &asize, &bsize, t, (size_t)bread) != 0) {
            if (ds) { free(ds); }
            close(pipefd[0]);
            return NULL;
        }
    }

    close(pipefd[0]);

    ret = d_shrink(ds, asize, bsize);
    free(ds);
    if (!ret) { return NULL; }
    if (outlen) { *outlen = asize; }
    return ret;
}

static char *ssub(ssg_cfg *cfg, const char *s, size_t l, size_t *outlen) {
    char *ds = NULL, *ret = NULL;
    size_t asize = 0, bsize = 0, i, j, affected;

    if (!s) { return NULL; }
    
    for (i = 0; i < l;) {
        affected = 0;
        for (j = 0; j < llen(breplaces) && affected == 0; j++) {
            if (strncmp(s + i, breplaces[j].old, breplaces[j].oldlen) == 0) {
                affected = breplaces[j].oldlen;
                if (breplaces[j].new && d_append(&ds, &asize, &bsize, breplaces[j].new,
                             strlen(breplaces[j].new)) != 0) {
                    if (ds) { free(ds); }
                    return NULL;
                }
            }
        }
        if (cfg->replaces) {
            for (j = 0; j < cfg->replaceslen && affected == 0; j++) {
                if (strncmp(s + i, cfg->replaces[j].old, cfg->replaces[j].oldlen) == 0) {
                    affected = cfg->replaces[j].oldlen;
                    if (d_append(&ds, &asize, &bsize, cfg->replaces[j].new,
                                 strlen(cfg->replaces[j].new)) != 0) {
                        if (ds) { free(ds); }
                        return NULL;
                    }
                }
            }
        }
        if (affected != 0) { i += affected; }
        else if (d_append(&ds, &asize, &bsize, &s[i++], 1) != 0) {
            if (ds) { free(ds); }
            return NULL;
        }
    }

    ret = d_shrink(ds, asize, bsize);
    free(ds);
    if (!ret) { return NULL; }
    if (outlen) { *outlen = asize; }
    return ret;
}

static int inignore(ssg_cfg *cfg, const char *dir, size_t dl, const char *name, size_t nl) {
    size_t i;
    struct stat st1, st2;
    char *path = NULL, *ipath = NULL;
    if (!dir || !name || !cfg->ignore) { return 0; } 
    path = join(dir, dl, name, nl, dir[dl - 1] == '/' ? NULL : "/", 1, NULL);
    if (!path) { return -1; }
    if (stat(path, &st1) != 0) { free(path); return -1; }
    free(path);
    for (i = 0; cfg->ignore[i] != NULL; i++) {
        ipath = join(rootdir, rootlen, cfg->ignore[i], strlen(cfg->ignore[i]),
                     rootlen > 0 ? (rootdir[rootlen - 1] == '/' ? NULL : "/") : NULL, 1, NULL);
        if (!ipath) { continue; }
        if (stat(ipath, &st2) != 0) { free(ipath); continue; }
        free(ipath);
        if (st1.st_dev == st2.st_dev && st1.st_ino == st2.st_ino) { return 1; }
    }
    return 0;
}

static int convert(ssg_cfg *cfg, const char *dir, size_t l) {
#undef _P
#define _P "(convert)"
    int ret = 1;
    DIR *d = NULL;
    struct dirent *e = NULL;
    struct stat st;
    char *path = NULL, *dname, *cpath = NULL,
         *nwext = ".html", *st1 = NULL, *st2 = NULL,
         *cmd = NULL;
    size_t pl = 0, dl = 0, i, ext = 0, st1l, st2l;
    FILE *cf = NULL;

    if (!dir) { return 1; }

    path = join(dir, l, ".preconvert", 11, dir[l - 1] == '/' ? NULL : "/", 1, NULL);
    if (!path) { __f("join"); }
    if (access(path, F_OK) == 0) {
        if (access(path, X_OK) != 0) {
            fprintf(stderr, _P" .preconvert in %s is not executable\n", dir);
        } else if (system(path) != 0) {
            fprintf(stderr, _P" .preconvert in %s failed\n", dir);
            goto error;
        }
    }
    free(path);
    path = NULL;

    path = join(dir, l, ".convert", 8, dir[l - 1] == '/' ? NULL : "/", 1, NULL);
    if (!path) { __f("join"); }
    if (access(path, F_OK) == 0) {
        if (access(path, X_OK) != 0) {
            fprintf(stderr, _P" .convert in %s is not executable\n", dir);
        } else if (system(path) != 0) {
            fprintf(stderr, _P" .convert in %s failed\n", dir);
            goto error;
        } else {
            free(path);
            path = NULL;
            goto skip;
        }
    }
    free(path);
    path = NULL;

    d = opendir(dir);
    if (!d) { __e("opendir"); }

    while ((e = readdir(d)) != NULL) {
        dname = e->d_name;
        dl = strlen(dname);
        if (dname[0] == '.' || inignore(cfg, dir, l, dname, dl)) { continue; }
        
        for (i = 0; i < dl; i++) {
            if (dname[i] == '.') {
                ext = i;
            }
        }

        if (strcmp(&dname[ext], ".md") != 0) { continue; }

        path = join(dir, l, dname, dl, dir[l - 1] == '/' ? NULL : "/", 1, &pl);
        if (!path) { __f("join"); }

        for (i = 0; i < pl; i++) {
            if (path[i] == '.') {
                ext = i;
            }
        }

        if (stat(path, &st) == 0 && !S_ISDIR(st.st_mode)) {
            cmd = join(cfg->mdhandler, cfg->mdhandlerlen, path, pl, " ", 1, NULL);

            st1 = execc(cmd, &st1l);
            if (!st1) { __f("execc"); }
            if (remove(path) != 0) { __e("remove"); }
            st2 = ssub(cfg, st1, st1l, &st2l);
            if (!st2) { __f("ssub"); }
            free(st1);
            free(cmd);
            st1 = NULL;
            cmd = NULL;

            cpath = malloc(ext + 6);
            if (!cpath) { __e("malloc"); }
            memcpy(cpath, path, ext);
            memcpy(&cpath[ext], nwext, 5);
            cpath[ext + 5] = '\0';
            
            cf = fopen(cpath, "w");
            if (!cf) { __e("fopen"); }
            fwrite(st2, 1, st2l, cf);
            fclose(cf);
            cf = NULL;
            free(st2);
            st2 = NULL;

            free(cpath);
            cpath = NULL;
        }

        free(path);
        path = NULL;
    }

skip:
    path = join(dir, l, ".postconvert", 12, dir[l - 1] == '/' ? NULL : "/", 1, NULL);
    if (!path) { __f("join"); }
    if (access(path, F_OK) == 0) {
        if (access(path, X_OK) != 0) {
            fprintf(stderr, _P" .postconvert in %s is not executable\n", dir);
        } else if (system(path) != 0) {
            fprintf(stderr, _P" .postconvert in %s failed\n", dir);
            goto error;
        }
    }
    free(path);
    path = NULL;
    
    ret = 0;
error:
    if (d) { closedir(d); }
    if (cmd) { free(cmd); }
    if (st1) { free(st1); }
    if (st2) { free(st2); }
    if (cpath) { free(cpath); }
    if (cf) { free(cf); }
    if (path) { free(path); }

    return ret;
}

static int process(ssg_cfg *cfg, const char *dir, size_t l) {
#undef _P
#define _P "(process)"
    int ret = 1;
    DIR *d = NULL;
    struct dirent *e = NULL;
    struct stat st;
    char *path = NULL, *dname = NULL, *fc = NULL, *nfc = NULL;
    size_t pl = 0, dl = 0, i, ext = 0, fz, nfz;
    FILE *f = NULL;

    if (!dir) { return 1; }

    path = join(dir, l, ".preprocess", 11, dir[l - 1] == '/' ? NULL : "/", 1, NULL);
    if (!path) { __f("join"); }
    if (access(path, F_OK) == 0) {
        if (access(path, X_OK) != 0) {
            fprintf(stderr, _P" .preprocess in %s is not executable\n", dir);
        } else if (system(path) != 0) {
            fprintf(stderr, _P" .preprocess in %s failed\n", dir);
            goto error;
        }
    }
    free(path);
    path = NULL;

    path = join(dir, l, ".process", 8, dir[l - 1] == '/' ? NULL : "/", 1, NULL);
    if (!path) { __f("join"); }
    if (access(path, F_OK) == 0) {
        if (access(path, X_OK) != 0) {
            fprintf(stderr, _P" .process in %s is not executable\n", dir);
        } else if (system(path) != 0) {
            fprintf(stderr, _P" .process in %s failed\n", dir);
            goto error;
        } else {
            free(path);
            path = NULL;
            goto skip;
        }
    }
    free(path);
    path = NULL;
    
    d = opendir(dir);
    if (!d) { __e("opendir"); }

    while ((e = readdir(d)) != NULL) {
        dname = e->d_name;
        dl = strlen(dname);
        if (dname[0] == '.' || inignore(cfg, dir, l, dname, dl)) { continue; }
        
        for (ext = 0, i = 0; i < dl; i++) {
            if (dname[i] == '.') {
                ext = i;
            }
        }

        if (strcmp(&dname[ext], ".html") != 0) { continue; }
        
        path = join(dir, l, dname, dl, dir[l - 1] == '/' ? NULL : "/", 1, &pl);
        if (!path) { __f("join"); }

        if (stat(path, &st) == 0 && !S_ISDIR(st.st_mode)) {
            breplaces[6].new = path + rootlen + (rootlen != 0 ? (rootdir[rootlen - 1] == '/' ? 0 : 1) : 0);
            if (cfg->aliases) {
                for (i = 0; i < cfg->aliaseslen; i++) {
                    if (strcmp(cfg->aliases[i].old, path + rootlen + 1) == 0) {
                        breplaces[6].new = cfg->aliases[i].new;
                        break;
                    }
                }
            }
            
            f = fopen(path, "r");
            if (!f) { __e("fopen"); }
            fseek(f, 0, SEEK_END);
            fz = ftell(f);
            rewind(f);
            fc = malloc(fz + 1);
            if (!fc) { __e("malloc"); }
            fread(fc, 1, fz, f);
            fc[fz] = '\0';
            fclose(f);
            
            breplaces[5].new = fc;
            nfc = ssub(cfg, cfg->template, cfg->templatelen, &nfz);
            breplaces[5].new = NULL;
            breplaces[6].new = NULL;
            if (!nfc) { __f("nfc"); }
            free(fc);
            fc = NULL;
            f = NULL;

            f = fopen(path, "w");
            if (!f) { __e("fopen"); }
            fwrite(nfc, 1, nfz, f);
            fclose(f);
            f = NULL;
            free(nfc);
            nfc = NULL;
        }
        
        free(path);
        path = NULL;
    }
skip:
    path = join(dir, l, ".postprocess", 12, dir[l - 1] == '/' ? NULL : "/", 1, NULL);
    if (!path) { __f("join"); }
    if (access(path, F_OK) == 0) {
        if (access(path, X_OK) != 0) {
            fprintf(stderr, _P" .postprocess in %s is not executable\n", dir);
        } else if (system(path) != 0) {
            fprintf(stderr, _P" .postprocess in %s failed\n", dir);
            goto error;
        }
    }
    free(path);
    path = NULL;

    ret = 0;
error:
    if (d) { closedir(d); }
    if (f) { fclose(f); }
    if (path) { free(path); }
    if (nfc) { free(nfc); }
    if (fc) { free(fc); }

    return ret;
}

int ssg_main(ssg_cfg *cfg, const char *dir) {
#undef _P
#define _P "(ssg_main)"
    int ret = 1;
    DIR *d = NULL;
    struct dirent *e = NULL;
    struct stat st;
    char *cur = NULL, *path = NULL;
    size_t i = 0, cl = 0, dl = 0, pl = 0, dirlen = 0;

    sdir *sdl = NULL, *nsdl = NULL;
    size_t cd = 0, asz = 8, od = 0;

    if (!cfg) { __f("config is null\n"); }
    if (!cfg->sitename) { __f("sitename not set\n"); }
    if (!cfg->mdhandler) { __f("mdhandler not set\n"); }
    if (!cfg->template) { __f("template not set\n"); }
    if (!dir) { __f("dir is null\n"); }

    dirlen = strlen(dir);

    path = join(dir, dirlen, "ssgc.lock", 9, dirlen > 0 && dir[dirlen - 1] == '/' ? NULL : "/", 1, &pl);
    if (!path) { __e("join"); }
    if (access(path, F_OK) == 0) { __f("ssgc.lock exists, aborting\n"); }
    free(path);
    path = NULL;

    breplaces[3].new = cfg->sitename;
    rootdir = dir;
    rootlen = dirlen;

    sdl = malloc(sizeof(sdir) * 8);
    if (!sdl) { __e("malloc"); }
    sdl[0].vname = NULL;
    sdl[0].name = NULL;
    sdl[0].name = estrdupl(dir, &sdl[0].namelen);
    if (!sdl[0].name) { __e("estrdupl"); }
    sdl[0].vname = estrdupl("/", NULL);
    sdl[0].vnamelen = 1;
    if (!sdl[0].vname) { __e("estrdupl"); }
    sdl[0].parent = sdl[0].vname;
    sdl[0].parentlen = sdl[0].vnamelen;
    cd += 1;

    for (i = 0; i < cd; i++) {
        cur = sdl[i].name, cl = sdl[i].namelen;
        if (!cur) { __f("sdir.name is null\n"); }
        if (cl == 0 || inignore(cfg, NULL, 0, cur, cl)) { continue; }

        d = opendir(cur);
        if (!d) { __e("opendir"); }

        while ((e = readdir(d)) != NULL) {
            if (e->d_name[0] == '.') { continue; }
            dl = strlen(e->d_name);
            path = join(cur, cl, e->d_name, dl, cur[cl - 1] == '/' ? NULL : "/", 1, &pl);
            if (!path) { __e("join"); }
            if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
                if (cd == 0) { free(path); __f("impossible error\n"); /* ??? */ }
                if (cd == asz) {
                    asz += asz / 2;
                    nsdl = realloc(sdl, sizeof(sdir) * asz);
                    if (!nsdl) { free(path); __e("realloc"); }
                    sdl = nsdl;
                }
                od = cd - 1;
                sdl[cd].name = path;
                sdl[cd].namelen = pl;
                sdl[cd].vname = join(sdl[od].vname, sdl[od].vnamelen,
                                  e->d_name, dl,
                                  sdl[od].vname[sdl[od].vnamelen - 1] == '/' ? NULL : "/", 1,
                                  &sdl[cd].vnamelen);
                sdl[cd].parent = sdl[od].vname;
                sdl[cd].parentlen = sdl[od].vnamelen;
                if (!sdl[cd].vname) { __e("join"); }
                cd += 1;
            } else { free(path); path = NULL; }
        }

        closedir(d);
        d = NULL;

        breplaces[1].new = sdl[i].parent;
        breplaces[4].new = sdl[i].vname;

        if (convert(cfg, cur, cl)) { __f("convert failed\n"); }
        if (process(cfg, cur, cl)) { __f("process failed\n"); }
    }

    ret = 0;
error:
    if (sdl) {
        for (i = 0; i < cd; i++) {
            if (sdl[i].name) { free(sdl[i].name); }
            if (sdl[i].vname) { free(sdl[i].vname); }
        }
        free(sdl);
    }
    if (d) { closedir(d); }

    return ret;
}
