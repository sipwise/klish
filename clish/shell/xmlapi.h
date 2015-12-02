/*
 * xmlapi.h
 *
 * private klish file: internal XML API
 */

#ifndef clish_xmlapi_included_h
#define clish_xmlapi_included_h

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <errno.h>
#include <stdio.h> /* need for FILE */

/* 
 * XML document (opaque type) 
 * The real type is defined by the selected external API
 */
typedef struct clish_xmldoc_s clish_xmldoc_t;

/* 
 * XML node (opaque type) 
 * The real type is defined by the selected external API
 */
typedef struct clish_xmlnode_s clish_xmlnode_t;

/*
 * Start and Stop XML parser engine.
 * Some parsers need a global cleanup at the end of the programm.
 */
int clish_xmldoc_start(void);
int clish_xmldoc_stop(void);

/*
 * read an XML document
 */
clish_xmldoc_t *clish_xmldoc_read(const char *filename);

/*
 * release a previously opened XML document
 */
void clish_xmldoc_release(clish_xmldoc_t *doc);

/*
 * check if a doc is valid (i.e. it loaded successfully)
 */
int clish_xmldoc_is_valid(clish_xmldoc_t *doc);

/*
 * XML implementation error capabilitiess
 * The real capabilities is or'ed using the following
 * constants
 */
typedef enum {
	CLISH_XMLERR_NOCAPS 	= 0,
	CLISH_XMLERR_LINE 	= 0x10,
	CLISH_XMLERR_COL 	= 0x20,
	CLISH_XMLERR_DESC 	= 0x40
} clish_xmlerrcaps_e;

/*
 * does this specific implementation define any error?
 * -> get the capabilities
 */
int clish_xmldoc_error_caps(clish_xmldoc_t *doc);

typedef enum {
	CLISH_XMLNODE_DOC,
	CLISH_XMLNODE_ELM,
	CLISH_XMLNODE_TEXT,
	CLISH_XMLNODE_ATTR,
	CLISH_XMLNODE_COMMENT,
	CLISH_XMLNODE_PI,
	CLISH_XMLNODE_DECL,
	CLISH_XMLNODE_UNKNOWN,
} clish_xmlnodetype_e;

/*
 * get error description, when available
 */
int clish_xmldoc_get_err_line(clish_xmldoc_t *doc);
int clish_xmldoc_get_err_col(clish_xmldoc_t *doc);
const char *clish_xmldoc_get_err_msg(clish_xmldoc_t *doc);

/*
 * get the node type
 */
int clish_xmlnode_get_type(clish_xmlnode_t *node);

/*
 * get the document root
 */
clish_xmlnode_t *clish_xmldoc_get_root(clish_xmldoc_t *doc);

/*
 * get the next child or NULL. If curchild is NULL, 
 * then the function returns the first child. 
 */
clish_xmlnode_t *clish_xmlnode_next_child(
	clish_xmlnode_t *parent, 
	clish_xmlnode_t *curchild);

/*
 * get the parent node.
 * returns NULL if node is the document root node.
 */
clish_xmlnode_t *clish_xmlnode_parent(clish_xmlnode_t *node);

/*
 * get the node name.
 * neither name not namelen shall be NULL. *namelen is the length of the
 * name buffer. If it's too small, we return -E2BIG and set *namelen to
 * the minimum length value.
 * returns < 0 on error. On error, name shall not be modified.
 */
int clish_xmlnode_get_name(
	clish_xmlnode_t *node,
	char *name, 
	unsigned int *namelen);

/*
 * get the node name
 * dynamically allocate the buffer (it must be freed once you don't need it
 * anymore) that will contain all the content of the node.
 * return NULL on error.
 */
static inline char* clish_xmlnode_get_all_name(clish_xmlnode_t *node)
{
	char *name = NULL; 
	unsigned int nlen = 2048; 
	int result;

	do { 
		name = (char*)realloc(name, nlen);
		result = clish_xmlnode_get_name(node, name, &nlen);
	} while (result == -E2BIG);

	if (result < 0) {
		free(name);
		return NULL;
	}

	return name;
}

/*
 * get the node content.
 * neither content not contentlen shall be NULL. *contentlen is the length
 * of the content buffer. If it's too small, we return -E2BIG and set
 * *contentlen to the minimum length value (including space for the \0 
 * character) so that two subsequent calls to this functions are going
 * to succeed if the forst one failed because of a too small buffer.
 * returns < 0 on error. On error, content shall not be modified.
 */
int clish_xmlnode_get_content(
	clish_xmlnode_t *node,
	char *content, 
	unsigned int *contentlen);

/*
 * get the node content
 * dynamically allocate the buffer (it must be freed once you don't need it
 * anymore) that will contain all the content of the node.
 * return NULL on error.
 */
static inline char* clish_xmlnode_get_all_content(clish_xmlnode_t *node)
{
	char *content = NULL; 
	unsigned int clen = 2048; 
	int result;

	do { 
		content = (char*)realloc(content, clen);
		result = clish_xmlnode_get_content(node, content, &clen);
	} while (result == -E2BIG);

	if (result < 0) {
		free(content);
		return NULL;
	}

	return content;
}

/*
 * get an attribute by name. May return NULL if the
 * attribute is not found
 * Special: allocate memory (to free with clish_xml_release())
 */
char *clish_xmlnode_fetch_attr(
	clish_xmlnode_t *node,
	const char *attrname);

/*
 * Free a pointer allocated by the XML backend
 */
void clish_xml_release(void *p);

/*
 * print an XML node to the out file
 */
void clish_xmlnode_print(clish_xmlnode_t *node, FILE *out);

#ifdef HAVE_LIB_LIBXSLT

/*
 * XSLT stylesheet (opaque type)
 * The real type is defined by the selected external API
 */
typedef struct clish_xslt_s clish_xslt_t;

/*
 * Load an XSLT stylesheet
 */
clish_xslt_t *clish_xslt_read(const char *filename);

/*
 * Load an embedded XSLT stylesheet from already
 * loaded XML document.
 */
clish_xslt_t *clish_xslt_read_embedded(clish_xmldoc_t *xmldoc);

/* Apply XSLT stylesheet */
clish_xmldoc_t *clish_xslt_apply(clish_xmldoc_t *xmldoc, clish_xslt_t *stylesheet);

/*
 * Release a previously opened XSLT stylesheet
 */
void clish_xslt_release(clish_xslt_t *stylesheet);

/*
 * Check if a stylesheet is valid (i.e. it loaded successfully)
 */
int clish_xslt_is_valid(clish_xslt_t *stylesheet);

#endif /* HAVE_LIB_LIBXSLT */

#endif /* clish_xmlapi_included_h */

