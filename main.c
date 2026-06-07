/*
Copyright (c) 2026 etar125 Admanse

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED “AS IS” AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <e1l.h>
#include <elist.h>
#define ESTRNDUPL
#include "estrdup.h"
#include "ssgc.h"

#define _e goto error
#define __e(x) perror(x); goto error
#define __f(x) fprintf(stderr, x); goto error
#define _f(x) fprintf(stderr, x)

int usage(char *progname) {
    printf("ssgc v"VERSION"\n"
           "Usage: %s /path/to/src\n",
           progname);
    return 0;
}

int main(int argc, char **argv) {
    ssg_cfg c;
    char *path = NULL, *buf = NULL, *src = NULL,
         *t = NULL;
    FILE *f = NULL;
    size_t fz, sl = 0, pl, i, j, k, tl, crpll = 0, crpli = 0;
    elist el;
    elist_obj eo;
    elist_pair ep;
    int ret = 1;
    ssg_rp crp, *crpl = NULL, *ncrpl = NULL;

    c.sitename = NULL;
    c.mdhandler = NULL;
    c.replaces = NULL;
    c.template = NULL;
    c.ignore = NULL;
    crp.new = NULL;
    crp.old = NULL;

    if (argc < 2) { return usage(argv[0]); }
    src = argv[1];
    sl = strlen(src);

    path = join(src, sl, "ssgc.elist", 10,
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
    
    el = elist_read(buf, fz);
    free(buf);
    buf = NULL;
    if (el.count == 0) { __f("ssgc.elist is empty"); }
    for (i = 0; i < el.count; i++) {
        eo = el.objs[i];
        for (j = 0; j < eo.count; j++) {
            ep = eo.pairs[j];
            if (ep.values.count == 0) {
                _f("found key without value");
            } else {
                #define pcmp(x) strcmp(ep.key, x) == 0
                if (pcmp("sitename")) {
                    if (ep.values.count > 1) {
                        _f("sitename requires only one value, first will be taken");
                        t = sarr_getstr(&ep.values, 0, &tl);
                        c.sitename = estrndupl(t, tl, NULL);
                        if (!c.sitename) { __e("estrndupl"); }
                        free(ep.values.strs);
                    } else {
                        c.sitename = ep.values.strs;
                    }
                } else if (pcmp("mdhandler")) {
                    if (ep.values.count > 1) {
                        _f("mdhandler requires only one value, first will be taken");
                        t = sarr_getstr(&ep.values, 0, &tl);
                        c.mdhandler = estrndupl(t, tl, &c.mdhandlerlen);
                        if (!c.mdhandler) { __e("estrndupl"); }
                        free(ep.values.strs);
                    } else {
                        c.mdhandler = ep.values.strs;
                        c.mdhandlerlen = ep.values.size;
                    }
                } else if (pcmp("old")) {
                    if (ep.values.count > 1) {
                        _f("old requires only one value, first will be taken");
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
                } else if (pcmp("new")) {
                    if (ep.values.count > 1) {
                        _f("new requires only one value, first will be taken");
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
                } else if (pcmp("ignore")) {
                    c.ignore = malloc(sizeof(char*) * (ep.values.count + 1));
                    if (!c.ignore) { __e("malloc"); }
                    for (k = 0; k < ep.values.count; k++) {
                        c.ignore[k] = NULL;
                        t = sarr_getstr(&ep.values, k, &tl);
                        c.ignore[k] = estrndupl(t, tl, NULL);
                        if (!c.ignore[k]) { __e("estrndupl"); }
                    }
                    c.ignore[k] = NULL;
                } else if (pcmp("template")) {
                    if (ep.values.count > 1) {
                        _f("template requires only one value, first will be taken");
                        t = sarr_getstr(&ep.values, 0, NULL);
                    } else { t = ep.values.strs; }

                    f = fopen(t, "r");
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

                    c.template = buf;
                    c.templatelen = fz;
                    buf = NULL;
                }
                free(ep.values.offsets);
                free(ep.key);
                ep.values.offsets = NULL;
                ep.values.strs = NULL;
                ep.key = NULL;
            }
        }
        free(eo.pairs);
        eo.pairs = NULL;
    }
    free(el.objs);
    el.objs = NULL;
    if (remove(path) != 0) { __e("remove"); }

    c.replaces = crpl;
    c.replaceslen = crpll;

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
    }
    if (crp.new) { free(crp.new); }
    if (crp.old) { free(crp.old); }

    return ret;
}
