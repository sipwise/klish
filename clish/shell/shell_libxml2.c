/*
 * ------------------------------------------------------
 * shell_roxml.c
 *
 * This file implements the means to read an XML encoded file 
 * and populate the CLI tree based on the contents. It implements
 * the clish_xml API using roxml
 * ------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(HAVE_LIB_LIBXML2)
#include <errno.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "xmlapi.h"

#ifdef HAVE_LIB_LIBXSLT
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
extern int xmlLoadExtDtdDefaultValue;

/* dummy stuff ; really a xsltStylesheet */
struct clish_xslt_s {
	int dummy;
};
#endif

/* dummy stuff ; really a xmlDoc */
struct clish_xmldoc_s {
	int dummy;
};

/* dummy stuff ; really a xmlNode */
struct clish_xmlnode_s {
	int dummy;
};

static inline xmlDoc *xmldoc_to_doc(clish_xmldoc_t *doc)
{
	return (xmlDoc*)doc;
}

static inline xmlNode *xmlnode_to_node(clish_xmlnode_t *node)
{
	return (xmlNode*)node;
}

static inline clish_xmldoc_t *doc_to_xmldoc(xmlDoc *node)
{
	return (clish_xmldoc_t*)node;
}

static inline clish_xmlnode_t *node_to_xmlnode(xmlNode *node)
{
	return (clish_xmlnode_t*)node;
}

/*
 * public interface
 */

int clish_xmldoc_start(void)
{
#ifdef HAVE_LIB_LIBXSLT
	/* The XSLT example contain these settings but I doubt 
	 * it's really necessary.
	 */
/*	xmlSubstituteEntitiesDefault(1);
	xmlLoadExtDtdDefaultValue = 1;
*/
#endif
	return 0;
}

int clish_xmldoc_stop(void)
{
#ifdef HAVE_LIB_LIBXSLT
	xsltCleanupGlobals();
#endif
	xmlCleanupParser();
	return 0;
}

clish_xmldoc_t *clish_xmldoc_read(const char *filename)
{
	xmlDoc *doc;
	doc = xmlReadFile(filename, NULL, 0);
	return doc_to_xmldoc(doc);
}

void clish_xmldoc_release(clish_xmldoc_t *doc)
{
	if (doc)
		xmlFreeDoc(xmldoc_to_doc(doc));
}

int clish_xmldoc_is_valid(clish_xmldoc_t *doc)
{
	return doc != NULL;
}

int clish_xmldoc_error_caps(clish_xmldoc_t *doc)
{
	return CLISH_XMLERR_NOCAPS;
}

int clish_xmldoc_get_err_line(clish_xmldoc_t *doc)
{
	return -1;
}

int clish_xmldoc_get_err_col(clish_xmldoc_t *doc)
{
	return -1;
}

const char *clish_xmldoc_get_err_msg(clish_xmldoc_t *doc)
{
	return "";
}

int clish_xmlnode_get_type(clish_xmlnode_t *node)
{
	if (node) {
		xmlNode *n = xmlnode_to_node(node);
		switch (n->type) {
		case XML_ELEMENT_NODE: 
			return CLISH_XMLNODE_ELM;
		case XML_TEXT_NODE: 
			return CLISH_XMLNODE_TEXT;
		case XML_COMMENT_NODE: 
			return CLISH_XMLNODE_COMMENT;
		case XML_PI_NODE: 
			return CLISH_XMLNODE_PI;
		case XML_ATTRIBUTE_NODE: 
			return CLISH_XMLNODE_ATTR;
		default:
			break;
		}
	}

	return CLISH_XMLNODE_UNKNOWN;
}

clish_xmlnode_t *clish_xmldoc_get_root(clish_xmldoc_t *doc)
{
	if (doc) {
		xmlNode *root = xmlDocGetRootElement(xmldoc_to_doc(doc));
		return node_to_xmlnode(root);
	}
	return NULL;
}

clish_xmlnode_t *clish_xmlnode_parent(clish_xmlnode_t *node)
{
	if (node) {
		xmlNode *n = xmlnode_to_node(node);
		xmlNode *root = xmlDocGetRootElement(n->doc);
		if (n != root)
			return node_to_xmlnode(n->parent);
	}
	return NULL;
}

clish_xmlnode_t *clish_xmlnode_next_child(clish_xmlnode_t *parent, 
					  clish_xmlnode_t *curchild)
{
	xmlNode *child;

	if (!parent)
		return NULL;

	if (curchild) {
		child = xmlnode_to_node(curchild)->next;
	} else {
		child = xmlnode_to_node(parent)->children;
	}

	return node_to_xmlnode(child);
}

char *clish_xmlnode_fetch_attr(clish_xmlnode_t *node,
					  const char *attrname)
{
	xmlNode *n;

	if (!node || !attrname)
		return NULL;

	n = xmlnode_to_node(node);

	if (n->type == XML_ELEMENT_NODE) {
		xmlAttr *a = n->properties;
		while (a) {
			if (strcmp((char*)a->name, attrname) == 0) {
				if (a->children && a->children->content)
					return (char *)a->children->content;
				else
					return NULL;
			}
			a = a->next;
		}
	}
		
	return NULL;
}

int clish_xmlnode_get_content(clish_xmlnode_t *node, char *content, 
			      unsigned int *contentlen)
{
	xmlNode *n;
	xmlNode *c;
	int rlen = 0;

	if (content && contentlen && *contentlen)
		*content = 0;

	if (!node || !content || !contentlen)
		return -EINVAL;

	if (*contentlen <= 1)
		return -EINVAL;

	*content = 0;
	n = xmlnode_to_node(node);

	/* first, get the content length */
	c = n->children;
	while (c) {
		if ((c->type == XML_TEXT_NODE || c->type == XML_CDATA_SECTION_NODE)
			&& !xmlIsBlankNode(c)) {
			rlen += strlen((char*)c->content);
		}
		c = c->next;
	}
	++rlen;

	if (rlen <= *contentlen) {
		c = n->children;
		while (c) {
			if ((c->type == XML_TEXT_NODE || c->type == XML_CDATA_SECTION_NODE)
				 && !xmlIsBlankNode(c)) {
				strcat(content, (char*)c->content);
			}
			c = c->next;
		}
		return 0;
	} else {
		*contentlen = rlen;
		return -E2BIG;
	}
}

int clish_xmlnode_get_name(clish_xmlnode_t *node, char *name, 
			    unsigned int *namelen)
{
	int rlen;
	xmlNode *n;

	if (name && namelen && *namelen)
		*name = 0;

	if (!node || !name || !namelen)
		return -EINVAL;

	if (*namelen <= 1)
		return -EINVAL;

	*name = 0;
	n = xmlnode_to_node(node);
	rlen = strlen((char*)n->name) + 1;
	
	if (rlen <= *namelen) {
		snprintf(name, *namelen, "%s", (char*)n->name);
		name[*namelen - 1] = '\0';
		return 0;
	} else {
		*namelen = rlen;
		return -E2BIG;
	}
}

void clish_xmlnode_print(clish_xmlnode_t *node, FILE *out)
{
	xmlNode *n;
	xmlAttr *a;

	n = xmlnode_to_node(node);
	if (n && n->name) {
		fprintf(out, "<%s", (char*)n->name);
		a = n->properties;
		while (a) {
			char *av = "";
			if (a->children && a->children->content)
				av = (char*)a->children->content;
			fprintf(out, " %s='%s'", (char*)a->name, av);
			a = a->next;
		}
		fprintf(out, ">");
	}
}

void clish_xml_release(void *p)
{
	/* do we allocate memory? not yet. */
}

#ifdef HAVE_LIB_LIBXSLT

static inline xsltStylesheet *xslt_to_xsltStylesheet(clish_xslt_t *xslt)
{
	return (xsltStylesheet*)xslt;
}

static inline clish_xslt_t *xsltStylesheet_to_xslt(xsltStylesheet *xslt)
{
	return (clish_xslt_t*)xslt;
}

int clish_xslt_is_valid(clish_xslt_t *stylesheet)
{
	return stylesheet != NULL;
}

clish_xmldoc_t *clish_xslt_apply(clish_xmldoc_t *xmldoc, clish_xslt_t *stylesheet)
{
	xmlDoc *doc = xmldoc_to_doc(xmldoc);
	xsltStylesheetPtr cur = xslt_to_xsltStylesheet(stylesheet);
	xmlDoc *res;

	if (!doc || !cur)
		return doc_to_xmldoc(NULL);
	res = xsltApplyStylesheet(cur, doc, NULL);

	return doc_to_xmldoc(res);
}

clish_xslt_t *clish_xslt_read(const char *filename)
{
	xsltStylesheet* cur = NULL;

	cur = xsltParseStylesheetFile((const xmlChar *)filename);

	return xsltStylesheet_to_xslt(cur);
}

clish_xslt_t *clish_xslt_read_embedded(clish_xmldoc_t *xmldoc)
{
	xsltStylesheet* cur = NULL;
	xmlDoc *doc = xmldoc_to_doc(xmldoc);

	cur = xsltLoadStylesheetPI(doc);

	return xsltStylesheet_to_xslt(cur);
}

void clish_xslt_release(clish_xslt_t *stylesheet)
{
	xsltStylesheet* cur = xslt_to_xsltStylesheet(stylesheet);

	if (!cur)
		return;
	xsltFreeStylesheet(cur);
}

#endif /* HAVE_LIB_LIBXSLT */

#endif /* HAVE_LIB_LIBXML2 */

