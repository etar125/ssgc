/*
Copyright (c) 2026 etar125 Admanse

Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted, provided that the above copyright notice and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED “AS IS” AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#ifndef SSGC_H
#define SSGC_H

#include <stddef.h>

typedef struct ssg_rp {
    char *old;
    char *new;
    size_t oldlen;
} ssg_rp;

typedef struct ssg_cfg {
    char *sitename;
    char *mdhandler;
    size_t mdhandlerlen;
    ssg_rp *replaces;
    size_t replaceslen;
    ssg_rp *aliases;
    size_t aliaseslen;
    char *template;
    size_t templatelen;
    char **ignore;
} ssg_cfg;

int ssg_main(ssg_cfg *cfg, const char *dir);

#endif
