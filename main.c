/*
Copyright (c) 2026 etar125 Admanse

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED “AS IS” AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <e1l.h>
#include <elist.h>
#define ESTRNDUPL
#include "estrdup.h"
#include "ssgc.h"

#define _P "(main)"

#define _e goto error
#define __e(x) perror(_P" "x); goto error
#define __f(x) fprintf(stderr, _P" "x); goto error
#define _f(x) fprintf(stderr, _P" "x)

/*#define system(x) mysys(x)

int mysys(char *s) {
    puts(s);
    return 0;
}*/

int usage(char *progname) {
    printf("ssgc v"VERSION"\n"
           "Usage: %s /path/to/src [/path/to/dst]\n",
           progname);
    return 0;
}

int main(int argc, char **argv) {
    ssg_cfg c;
    char *path = NULL, *buf = NULL, *src = NULL,
        *t = NULL, *dst = NULL, *copycmd = NULL, *cpath = NULL, *tpath = NULL;
    FILE *f = NULL;
    size_t fz, sl = 0, dl = 0, pl, i, j, k, tl, crpll = 0, crpli = 0, copycmdlen = 0,
           acrpll = 0, acrpli = 0;
    elist el;
    elist_obj eo;
    elist_pair ep;
    int ret = 1;
    ssg_rp crp, *crpl = NULL, *ncrpl = NULL,
           acrp, *acrpl = NULL, *ancrpl = NULL;

    c.sitename = NULL;
    c.mdhandler = NULL;
    c.replaces = NULL;
    c.aliases = NULL;
    c.template = NULL;
    c.ignore = NULL;
    crp.new = NULL;
    crp.old = NULL;
    el.objs = NULL;

    if (argc < 2 || argc > 3) { return usage(argv[0]); }
    src = argv[1];
    sl = strlen(src);
    if (argc == 3) {
        dst = argv[2];
        dl = strlen(dst);
    } else {
        path = join(src, sl, "ssgc.lock", 9,
                    (sl == 0 || src[sl - 1] == '/' ? NULL : "/"), 1,
                    &pl);
        if (!path) { __e("join"); }
        if (access(path, F_OK) == 0) { __f("ssgc.lock exists, aborting\n"); }
        free(path);
        path = NULL;
    }

    path = join(src, sl, "ssgc.elist", 10,
                (sl == 0 || src[sl - 1] == '/' ? NULL : "/"), 1,
                &pl);
    if (!path) { __e("join"); }

    f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, _P" unable to open cfg '%s'\n", path);
        __e("fopen");
    }
    fseek(f, 0, SEEK_END);
    fz = ftell(f);
    rewind(f);

    buf = malloc(fz + 1);
    if (!buf) { __e("malloc"); }
    fread(buf, 1, fz, f);
    fclose(f);
    f = NULL;
    buf[fz] = '\0';

    cpath = path;
    path = NULL;
    
    el = elist_read(buf, fz);
    free(buf);
    buf = NULL;
    if (el.count == 0) { __f("ssgc.elist is empty"); }
    for (i = 0; i < el.count; i++) {
        eo = el.objs[i];
        for (j = 0; j < eo.count; j++) {
            ep = eo.pairs[j];
            if (ep.values.count == 0) {
                _f("found key without value\n");
            } else {
                #define pcmp(x) strcmp(ep.key, x) == 0
                if (pcmp("sitename")) {
                    if (c.sitename) {
                        free(c.sitename);
                        c.sitename = NULL;
                    }
                    if (ep.values.count > 1) {
                        _f("sitename requires only one value, first will be taken\n");
                        t = sarr_getstr(&ep.values, 0, &tl);
                        c.sitename = estrndupl(t, tl, NULL);
                        if (!c.sitename) { __e("estrndupl"); }
                        free(ep.values.strs);
                    } else {
                        c.sitename = ep.values.strs;
                    }
                } else if (pcmp("mdhandler")) {
                    if (c.mdhandler) {
                        free(c.mdhandler);
                        c.mdhandler = NULL;
                    }
                    if (ep.values.count > 1) {
                        _f("mdhandler requires only one value, first will be taken\n");
                        t = sarr_getstr(&ep.values, 0, &tl);
                        c.mdhandler = estrndupl(t, tl, &c.mdhandlerlen);
                        if (!c.mdhandler) { __e("estrndupl"); }
                        free(ep.values.strs);
                    } else {
                        c.mdhandler = ep.values.strs;
                        c.mdhandlerlen = ep.values.size;
                    }
                } else if (pcmp("oldr")) {
                    if (crp.old) {
                        free(crp.old);
                        crp.old = NULL;
                    }
                    if (ep.values.count > 1) {
                        _f("old requires only one value, first will be taken\n");
                        t = sarr_getstr(&ep.values, 0, &tl);
                        crp.old = estrndupl(t, tl, &crp.oldlen);
                        if (!crp.old) { __e("estrndupl"); }
                        free(ep.values.strs);
                    } else {
                        crp.old = ep.values.strs;
                        crp.oldlen = ep.values.size;
                    }
                    if (crp.new) {
                        if (crpli == crpll) {
                            crpll += 1;
                            ncrpl = realloc(crpl, sizeof(ssg_rp) * crpll);
                            if (!ncrpl) { __e("realloc"); }
                            crpl = ncrpl;
                        }
                        crpl[crpli++] = crp;
                        crp.new = NULL;
                        crp.old = NULL;
                    }
                } else if (pcmp("newr")) {
                    if (crp.new) {
                        free(crp.new);
                        crp.new = NULL;
                    }
                    if (ep.values.count > 1) {
                        _f("new requires only one value, first will be taken\n");
                        t = sarr_getstr(&ep.values, 0, &tl);
                        crp.new = estrndupl(t, tl, NULL);
                        if (!crp.new) { __e("estrndupl"); }
                        free(ep.values.strs);
                    } else {
                        crp.new = ep.values.strs;
                    }
                    if (crp.old) {
                        if (crpli == crpll) {
                            crpll += 1;
                            ncrpl = realloc(crpl, sizeof(ssg_rp) * crpll);
                            if (!ncrpl) { __e("realloc"); }
                            crpl = ncrpl;
                        }
                        crpl[crpli++] = crp;
                        crp.new = NULL;
                        crp.old = NULL;
                    }
                } else if (pcmp("olda")) {
                    if (acrp.old) {
                        free(acrp.old);
                        acrp.old = NULL;
                    }
                    if (ep.values.count > 1) {
                        _f("old requires only one value, first will be taken\n");
                        t = sarr_getstr(&ep.values, 0, &tl);
                        acrp.old = estrndupl(t, tl, &acrp.oldlen);
                        if (!acrp.old) { __e("estrndupl"); }
                        free(ep.values.strs);
                    } else {
                        acrp.old = ep.values.strs;
                        acrp.oldlen = ep.values.size;
                    }
                    if (acrp.new) {
                        if (acrpli == acrpll) {
                            acrpll += 1;
                            ancrpl = realloc(acrpl, sizeof(ssg_rp) * acrpll);
                            if (!ancrpl) { __e("realloc"); }
                            acrpl = ancrpl;
                        }
                        acrpl[acrpli++] = acrp;
                        acrp.new = NULL;
                        acrp.old = NULL;
                    }
                } else if (pcmp("newa")) {
                    if (acrp.new) {
                        free(acrp.new);
                        acrp.new = NULL;
                    }
                    if (ep.values.count > 1) {
                        _f("new requires only one value, first will be taken\n");
                        t = sarr_getstr(&ep.values, 0, &tl);
                        acrp.new = estrndupl(t, tl, NULL);
                        if (!acrp.new) { __e("estrndupl"); }
                        free(ep.values.strs);
                    } else {
                        acrp.new = ep.values.strs;
                    }
                    if (acrp.old) {
                        if (acrpli == acrpll) {
                            acrpll += 1;
                            ancrpl = realloc(acrpl, sizeof(ssg_rp) * acrpll);
                            if (!ancrpl) { __e("realloc"); }
                            acrpl = ancrpl;
                        }
                        acrpl[acrpli++] = acrp;
                        acrp.new = NULL;
                        acrp.old = NULL;
                    }
                } else if (pcmp("ignore")) {
                    if (c.ignore) {
                        free(c.ignore);
                        c.ignore = NULL;
                    }
                    c.ignore = malloc(sizeof(char*) * (ep.values.count + 1));
                    if (!c.ignore) { __e("malloc"); }
                    for (k = 0; k < ep.values.count; k++) {
                        c.ignore[k] = NULL;
                        t = sarr_getstr(&ep.values, k, &tl);
                        c.ignore[k] = estrndupl(t, tl, NULL);
                        if (!c.ignore[k]) { __e("estrndupl"); }
                    }
                    c.ignore[k] = NULL;
                    free(ep.values.strs);
                } else if (pcmp("template")) {
                    if (c.template) {
                        free(c.template);
                        c.template = NULL;
                    }
                    if (ep.values.count > 1) {
                        _f("template requires only one value, first will be taken\n");
                        t = sarr_getstr(&ep.values, 0, NULL);
                    } else { t = ep.values.strs; }

                    path = join(src, sl, t, strlen(t),
                                (sl == 0 || src[sl - 1] == '/' ? NULL : "/"), 1,
                                &pl);
                    if (!path) { __e("join"); }
                    
                    f = fopen(path, "r");
                    if (!f) { __e("fopen"); }
                    fseek(f, 0, SEEK_END);
                    fz = ftell(f);
                    rewind(f);

                    buf = malloc(fz + 1);
                    if (!buf) { __e("malloc"); }
                    fread(buf, 1, fz, f);
                    fclose(f);
                    f = NULL;
                    buf[fz] = '\0';

                    /* if (remove(path) != 0) { __e("remove"); } */
                    tpath = path;

                    c.template = buf;
                    c.templatelen = fz;
                    buf = NULL;

                    free(ep.values.strs);
                } else if (pcmp("copycmd")) {
                    if (copycmd) {
                        free(copycmd);
                        copycmd = NULL;
                    }
                    if (ep.values.count > 1) {
                        _f("copycmd requires only one value, first will be taken\n");
                        t = sarr_getstr(&ep.values, 0, &tl);
                        copycmd = estrndupl(t, tl, &copycmdlen);
                        if (!copycmd) { __e("estrndupl"); }
                        free(ep.values.strs);
                    } else {
                        copycmd = ep.values.strs;
                        copycmdlen = ep.values.size;
                    }
                }
            }
            free(ep.values.offsets);
            free(ep.key);
            eo.pairs[j].values.offsets = NULL;
            eo.pairs[j].values.strs = NULL;
            eo.pairs[j].key = NULL;
        }
        free(eo.pairs);
        el.objs[i].pairs = NULL;
    }
    free(el.objs);
    el.objs = NULL;

    c.replaces = crpl;
    c.replaceslen = crpll;
    c.aliases = acrpl;
    c.aliaseslen = acrpll;

    if (dst) {
        if (!copycmd) {
            path = join("rm -rfv ", 8, dst, dl, NULL, 0, &pl);
            if (!path) { __f("join\n"); }
            if (system(path) != 0) { __f("remove destination failed\n"); }
            free(path);
            path = NULL;

            /* buf is / for dst */
            buf = dl == 0 || dst[dl - 1] == '/' ? NULL : "/";

            path = join("cp -rfv ", 8, src, sl, NULL, 0, &pl);
            if (!path) { __f("join\n"); }
            t = NULL;
            t = join(path, pl, dst, dl, " ", 1, NULL);
            if (!t) { __f("join\n"); }
        
            if (system(t) != 0) { __f("copy source to destination failed\n"); }
            free(t);
            free(path);
            t = NULL;
            path = NULL;

            path = join("rm -fv ", 7, dst, dl, NULL, 0, &pl);
            if (!path) { __f("join\n"); }
            t = join(path, pl, "ssgc.elist ", 11, buf, 1, &tl);
            if (!t) { __f("join\n"); }
            free(path);
            path = NULL;
            path = join(t, tl, dst, dl, NULL, 0, &pl);
            if (!path) { __f("join\n"); }
            free(t);
            t = NULL;
            t = join(path, pl, "template.html ", 14, buf, 1, &tl);
            if (!t) { __f("join\n"); }
            free(path);
            path = NULL;
            path = join(t, tl, dst, dl, NULL, 0, &pl);
            if (!path) { __f("join\n"); }
            free(t);
            t = NULL;
            t = join(path, pl, "ssgc.lock", 9, buf, 1, &tl);
            if (!t) { __f("join\n"); }
            free(path);
            path = NULL;

            if (system(t) != 0) { __f("remove ssgc.elist, template.html, ssgc.lock failed\n"); }
            free(t);
            t = NULL;
            buf = NULL;
        } else {
            path = join(copycmd, copycmdlen, src, sl, " ", 1, &pl);
            if (!path) { __f("join\n"); }
            t = NULL;
            t = join(path, pl, dst, dl, " ", 1, NULL);
            if (!t) { __f("join\n"); }
        
            if (system(t) != 0) { __f("copy command failed\n"); }
            free(t);
            t = NULL;
        }

        src = dst;
    } else {
        if (copycmd) { _f("copy command present but no destination, ignoring it\n"); }
        if (remove(cpath) != 0) { __e("remove"); }
        if (remove(tpath) != 0) { __e("remove"); }
    }

    ret = ssg_main(&c, src);
error:
    if (c.sitename) { free(c.sitename); }
    if (c.mdhandler) { free(c.mdhandler); }
    /* if (c.replaces) { free(c.replaces); } */
    if (c.template) { free(c.template); }
    if (c.ignore) {
        for (k = 0; c.ignore[k] != NULL; k++) {
            free(c.ignore[k]);
        }
        free(c.ignore);
    }
    if (copycmd) { free(copycmd); }
    if (path) { free(path); }
    if (f) { fclose(f); }
    if (buf) { free(buf); }
    if (el.objs) {
        for (i = 0; i < el.count; i++) {
            eo = el.objs[i];
            if (eo.pairs) {
                for (j = 0; j < eo.count; j++) {
                    ep = eo.pairs[j];
                    if (ep.key) { free(ep.key); }
                    if (ep.values.strs) { free(ep.values.strs); }
                    if (ep.values.offsets) { free(ep.values.offsets); }
                }
                free(eo.pairs);
            }
        }
        free(el.objs);
    }
    if (crpl) {
        for (i = 0; i < crpll; i++) {
            if (crpl[i].old) { free(crpl[i].old); }
            if (crpl[i].new) { free(crpl[i].new); }
        }
        free(crpl);
    }
    if (crp.new) { free(crp.new); }
    if (crp.old) { free(crp.old); }
    if (acrpl) {
        for (i = 0; i < acrpll; i++) {
            if (acrpl[i].old) { free(acrpl[i].old); }
            if (acrpl[i].new) { free(acrpl[i].new); }
        }
        free(acrpl);
    }
    if (acrp.new) { free(acrp.new); }
    if (acrp.old) { free(acrp.old); }
    if (cpath) { free(cpath); }
    if (tpath) { free(tpath); }

    return ret;
}
