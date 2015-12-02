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

#if defined(HAVE_LIB_ROXML)
#include <errno.h>
#include <roxml.h>
#include "xmlapi.h"

/* dummy stuff ; really a node_t */
struct clish_xmldoc_s {
	int dummy;
};

/* dummy stuff ; really a node_t */
struct clish_xmlnode_s {
	int dummy;
};

static inline node_t *xmldoc_to_node(clish_xmldoc_t *doc)
{
	return (node_t*)doc;
}

static inline node_t *xmlnode_to_node(clish_xmlnode_t *node)
{
	return (node_t*)node;
}

static inline clish_xmldoc_t *node_to_xmldoc(node_t *node)
{
	return (clish_xmldoc_t*)node;
}

static inline clish_xmlnode_t *node_to_xmlnode(node_t *node)
{
	return (clish_xmlnode_t*)node;
}

/*
 * public interface
 */

int clish_xmldoc_start(void)
{
	return 0;
}

int clish_xmldoc_stop(void)
{
	return 0;
}

clish_xmldoc_t *clish_xmldoc_read(const char *filename)
{
	node_t *doc = roxml_load_doc((char*)filename);
	return node_to_xmldoc(doc);
}

void clish_xmldoc_release(clish_xmldoc_t *doc)
{
	if (doc) {
		node_t *node = xmldoc_to_node(doc);
		roxml_release(RELEASE_ALL);
		roxml_close(node);
	}
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
		int type = roxml_get_type(xmlnode_to_node(node));
		switch (type) {
		case ROXML_ELM_NODE: 
			return CLISH_XMLNODE_ELM;
		case ROXML_TXT_NODE: 
			return CLISH_XMLNODE_TEXT;
		case ROXML_CMT_NODE: 
			return CLISH_XMLNODE_COMMENT;
		case ROXML_PI_NODE: 
			return CLISH_XMLNODE_PI;
		case ROXML_ATTR_NODE: 
			return CLISH_XMLNODE_ATTR;
		default:
			break;
		}
	}

	return CLISH_XMLNODE_UNKNOWN;
}

clish_xmlnode_t *clish_xmldoc_get_root(clish_xmldoc_t *doc)
{
	node_t *root;
	char *name = NULL;

	if (!doc)
		return NULL;
	root = roxml_get_root(xmldoc_to_node(doc));
	if (!root)
		return NULL;
	/* The root node is always documentRoot since libroxml-2.2.2. */
	/* It's good but not compatible with another XML parsers. */
	name = roxml_get_name(root, NULL, 0);
	if (0 == strcmp(name, "documentRoot"))
		root = roxml_get_chld(root, NULL, 0);
	roxml_release(name);

	return node_to_xmlnode(root);
}

clish_xmlnode_t *clish_xmlnode_parent(clish_xmlnode_t *node)
{
	if (node) {
		node_t *roxn = xmlnode_to_node(node);
		node_t *root = roxml_get_root(roxn);
		if (roxn != root)
			return node_to_xmlnode(roxml_get_parent(roxn));
	}
	return NULL;
}

clish_xmlnode_t *clish_xmlnode_next_child(clish_xmlnode_t *parent, 
					  clish_xmlnode_t *curchild)
{
	node_t *roxc;

	if (!parent)
		return NULL;

	roxc = xmlnode_to_node(curchild);

	if (roxc) {
		return node_to_xmlnode(roxml_get_next_sibling(roxc));
	} else {
		node_t *roxp = xmlnode_to_node(parent);
		node_t *child = NULL;
		int count;

		count = roxml_get_chld_nb(roxp);
		if (count)
			child = roxml_get_chld(roxp, NULL, 0);

		return node_to_xmlnode(child);
	}

	return NULL;
}

static int i_is_needle(char *src, const char *needle)
{
	int nlen = strlen(needle);
	int slen = strlen(src);

	if (slen >= nlen) {
		if (strncmp(src, needle, nlen) == 0)
			return 1;
	}
	return 0;
}

/* warning: dst == src is valid */
static void i_decode_and_copy(char *dst, char *src)
{
	while (*src) {
		if (*src == '&') {
			if (i_is_needle(src, "&lt;")) {
				*dst++ = '<';
				src += 4;
			} else if (i_is_needle(src, "&gt;")) {
				*dst++ = '>';
				src += 4;
			} else if (i_is_needle(src, "&amp;")) {
				*dst++ = '&';
				src += 5;
			} else {
				*dst++ = *src++;
			}
		} else {
			*dst++ = *src++;
		}
	}
	*dst++ = 0;
}

char *clish_xmlnode_fetch_attr(clish_xmlnode_t *node,
			       const char *attrname)
{
	node_t *roxn;
	node_t *attr;
	char *content;

	if (!node || !attrname)
		return NULL;

	roxn = xmlnode_to_node(node);
	attr = roxml_get_attr(roxn, (char*)attrname, 0);

	content = roxml_get_content(attr, NULL, 0, NULL);
	if (content) {
		i_decode_and_copy(content, content);
	}
	return content;
}

static int i_get_content(node_t *n, char *v, unsigned int *vl)
{
	char *c;
	int len;

	c = roxml_get_content(n, NULL, 0, NULL);
	if (c) {
		len = strlen(c) + 1;
		if (len <= *vl) {
			i_decode_and_copy(v, c);
			roxml_release(c);
			return 0;
		} else {
			*vl = len;
			roxml_release(c);
			return -E2BIG;
		}
	}
	*vl = (unsigned int)-1;
	return -ENOMEM;
}

int clish_xmlnode_get_content(clish_xmlnode_t *node, char *content, 
			      unsigned int *contentlen)
{
	if (content && contentlen && *contentlen)
		*content = 0;

	if (!node || !content || !contentlen)
		return -EINVAL;

	if (*contentlen <= 1)
		return -EINVAL;

	*content = 0;

	return i_get_content(xmlnode_to_node(node), content, contentlen);
}

static int i_get_name(node_t *n, char *v, unsigned int *vl)
{
	char *c;
	int len;

	c = roxml_get_name(n, NULL, 0);
	if (c) {
		len = strlen(c) + 1;
		if (len <= *vl) {
			snprintf(v, *vl, "%s", c);
			v[*vl - 1] = '\0';
			roxml_release(c);
			return 0;
		} else {
			*vl = len;
			roxml_release(c);
			return -E2BIG;
		}
	}
	*vl = (unsigned int)-1;
	return -ENOMEM;
}

int clish_xmlnode_get_name(clish_xmlnode_t *node, char *name, 
			    unsigned int *namelen)
{
	if (name && namelen && *namelen)
		*name = 0;

	if (!node || !name || !namelen)
		return -EINVAL;

	if (*namelen <= 1)
		return -EINVAL;

	*name = 0;

	return i_get_name(xmlnode_to_node(node), name, namelen);
}

void clish_xmlnode_print(clish_xmlnode_t *node, FILE *out)
{
	node_t *roxn;
	char *name;

	roxn = xmlnode_to_node(node);
	name = roxml_get_name(roxn, NULL, 0);
	if (name) {
		fprintf(out, "<%s", name);
		roxml_release(name);
		if (roxml_get_attr_nb(roxn)) {
			int attr_count = roxml_get_attr_nb(roxn);
			int attr_pos;
			for (attr_pos = 0; attr_pos < attr_count; ++attr_pos) {
				node_t *attr = roxml_get_attr(roxn, NULL, attr_pos);
				char *n = roxml_get_name(attr, NULL, 0);
				char *v = roxml_get_content(attr, NULL, 0, NULL);
				if (n && v) {
					fprintf(out, " %s='%s'", n, v);
				}
				if (v) 
					roxml_release(v);
				if (n) 
					roxml_release(n);
			}
		}
		fprintf(out, ">");
	}
}

void clish_xml_release(void *p)
{
	if (p) {
		roxml_release(p);
	}
}

#endif /* HAVE_LIB_ROXML */

