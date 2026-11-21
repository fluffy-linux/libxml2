/*
 * testcatalog.c: C program to run libxml2 catalog.c unit tests
 *
 * To compile on Unixes:
 * cc -o testcatalog `xml2-config --cflags` testcatalog.c `xml2-config --libs` -lpthread
 *
 * See Copyright for the status of this software.
 *
 * Author: Daniel Garcia <dani@danigm.net>
 */


#include "libxml.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_DECL_GLOB && !defined(_WIN32)
#include <unistd.h>
#endif

#ifdef LIBXML_CATALOG_ENABLED
#include <libxml/catalog.h>

/* Test catalog resolve uri with recursive catalog */
static int
testRecursiveDelegateUri(void) {
    int ret = 0;
    const char *cat = "test/catalogs/catalog-recursive.xml";
    const char *entity = "/foo.ent";
    xmlChar *resolved = NULL;

    xmlInitParser();
    xmlLoadCatalog(cat);

    /* This should trigger recursive error */
    resolved = xmlCatalogResolveURI(BAD_CAST entity);
    if (resolved != NULL) {
        fprintf(stderr, "CATALOG-FAILURE: Catalog %s entity should fail to resolve\n", entity);
        ret = 1;
    }
    xmlCatalogCleanup();

    return ret;
}

/* Test parsing repeated NextCatalog */
static int
testRepeatedNextCatalog(void) {
    int ret = 0;
    int i = 0;
    const char *cat = "test/catalogs/repeated-next-catalog.xml";
    const char *entity = "/foo.ent";
    xmlDocPtr doc = NULL;
    xmlNodePtr node = NULL;

    xmlInitParser();

    xmlLoadCatalog(cat);
    /* To force the complete recursive load */
    xmlCatalogResolveURI(BAD_CAST entity);
    /**
     * Ensure that the doc doesn't contain the same nextCatalog
     */
    doc = xmlCatalogDumpDoc();
    xmlCatalogCleanup();

    if (doc == NULL) {
        fprintf(stderr, "CATALOG-FAILURE: Failed to dump the catalog\n");
        return 1;
    }

    /* Just the root "catalog" node with a series of nextCatalog */
    node = xmlDocGetRootElement(doc);
    node = node->children;
    for (i=0; node != NULL; node=node->next, i++) {}
    if (i > 1) {
        fprintf(stderr, "CATALOG-FAILURE: Found %d nextCatalog entries and should be 1\n", i);
        ret = 1;
    }

    xmlFreeDoc(doc);

    return ret;
}

#if HAVE_DECL_GLOB && !defined(_WIN32)
/*
 * Test wildcards in the default catalog list: each matching catalog is
 * used, in sorted order, then the next entry; a pattern matching nothing
 * is skipped.
 */
static int
checkResolved(const char *pubID, const char *suffix) {
    xmlChar *resolved = xmlCatalogResolvePublic(BAD_CAST pubID);
    size_t len, slen = strlen(suffix);
    int ret = 0;

    if ((resolved == NULL) ||
        ((len = strlen((const char *) resolved)) < slen) ||
        (strcmp((const char *) resolved + len - slen, suffix) != 0)) {
        fprintf(stderr, "CATALOG-FAILURE: %s resolved to %s, expected *%s\n",
                pubID, resolved ? (const char *) resolved : "(null)", suffix);
        ret = 1;
    }
    xmlFree(resolved);
    return ret;
}

static int
testWildcardDefaultCatalogs(void) {
    char cwd[4096];
    char files[3 * 4096 + 256];
    int ret = 0;

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        fprintf(stderr, "CATALOG-FAILURE: getcwd failed\n");
        return 1;
    }
    snprintf(files, sizeof(files),
             "file://%s/test/catalogs/wildcard/nomatch/*/catalog.xml "
             "file://%s/test/catalogs/wildcard/pkgs/*/catalog.xml "
             "%s/test/catalogs/wildcard/etc.xml",
             cwd, cwd, cwd);
    setenv("XML_CATALOG_FILES", files, 1);

    xmlInitParser();
    xmlInitializeCatalog();

    ret |= checkResolved("-//FLUFFY//DTD A//EN", "/pkgs/a/a.dtd");
    ret |= checkResolved("-//FLUFFY//DTD B//EN", "/pkgs/b/b.dtd");
    ret |= checkResolved("-//FLUFFY//DTD ETC//EN", "/wildcard/etc.dtd");
    ret |= checkResolved("-//FLUFFY//DTD SHARED//EN", "/pkgs/a/from-a.dtd");

    xmlCatalogCleanup();
    unsetenv("XML_CATALOG_FILES");

    return ret;
}
#endif

int
main(void) {
    int err = 0;

#if HAVE_DECL_GLOB && !defined(_WIN32)
    err |= testWildcardDefaultCatalogs();
#endif
    err |= testRecursiveDelegateUri();
    err |= testRepeatedNextCatalog();

    return err;
}
#else
/* No catalog, so everything okay */
int
main(void) {
    return 0;
}
#endif
