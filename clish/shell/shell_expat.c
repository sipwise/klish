/*
 * ------------------------------------------------------
 * shell_expat.c
 *
 * This file implements the means to read an XML encoded file
 * and populate the CLI tree based on the contents. It implements
 * the clish_xml API using the expat XML parser
 *
 * expat is not your typicall XML parser. It does not work
 * by creating a full in-memory XML tree, but by calling specific
 * callbacks (element handlers) regularly while parsing. It's up
 * to the user to create the corresponding XML tree if needed
 * (obviously, this is what we're doing, as we really need the XML
 * tree in klish).
 *
 * The code below do that. It transforms the output of expat
 * to a DOM representation of the underlying XML file. This is
 * a bit overkill, and maybe a later implementation will help to
 * cut the work to something simpler, but the current klish
 * implementation requires this.
 * ------------------------------------------------------
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#if defined(HAVE_LIB_EXPAT)
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>

/* FreeBSD have verbatim version of expat named bsdxml */
#ifdef HAVE_LIB_BSDXML
#include <bsdxml.h>
#else
#include <expat.h>
#endif

#include "xmlapi.h"

/** DOM_like XML node
 *
 * @struct clish_xmlnode_s
 */
struct clish_xmlnode_s {
	char *name;
	clish_xmlnode_t *parent; /**< parent node */
	clish_xmlnode_t *children; /**< list of children */
	clish_xmlnode_t *next; /**< next sibling */
	clish_xmlnode_t *attributes; /**< attributes are nodes too */
	char *content; /**< !NULL for text and attributes nodes */
	clish_xmlnodetype_e type; /**< node type */
	int depth; /**< node depth */
	clish_xmldoc_t *doc;
};

/** DOM-like XML document
 *
 * @struct clish_xmldoc_s
 */
struct clish_xmldoc_s {
	clish_xmlnode_t *root; /**< list of root elements */
	clish_xmlnode_t *current; /**< current element */
	char *filename; /**< current filename */
};

/*
 * Expat need these functions to be able to build a DOM-like tree that
 * will be usable by klish.
 */
/** Put a element at the and of an element list
 *
 * @param first first element of the list
 * @param node element to add
 * @return new first element of the list
 */
static clish_xmlnode_t *clish_expat_list_push_back(clish_xmlnode_t *first, clish_xmlnode_t *node)
{
	clish_xmlnode_t *cur = first;
	clish_xmlnode_t *prev = NULL;

	while (cur) {
		prev = cur;
		cur = cur->next;
	}
	if (prev) {
		prev->next = node;
		return first;
	}
	return node;
}

/** Generic add_attr() function
 *
 * @param first first attribute in the attribute list
 * @param n attribute name
 * @param v attribute value
 * @return the new first attribute in the attribute list
 */
static clish_xmlnode_t *clish_expat_add_attr(clish_xmlnode_t *first, const char *n, const char *v)
{
	clish_xmlnode_t *node;

	node = malloc(sizeof(clish_xmlnode_t));
	if (!node)
		return first;

	node->name = strdup(n);
	node->content = strdup(v);
	node->children = NULL;
	node->attributes = NULL;
	node->next = NULL;
	node->type = CLISH_XMLNODE_ATTR;
	node->depth = 0;

	return clish_expat_list_push_back(first, node);
}

/** Run through an expat attribute list, and create a DOM-like attribute list
 *
 * @param node parent node
 * @param attr NULL-terminated attribute liste
 *
 * Each attribute uses two slots in the expat attribute list. The first one is
 * used to store the name, the second one is used to store the value.
 */
static void clish_expat_add_attrlist(clish_xmlnode_t *node, const char **attr)
{
	int i;

	for (i = 0; attr[i]; i += 2) {
		node->attributes = clish_expat_add_attr(node->attributes,
			attr[i], attr[i+1]);
	}
}

/** Generic make_node() function
 *
 * @param parent XML parent node
 * @param type XML node type
 * @param n node name (can be NULL, strdup'ed)
 * @param v node content (can be NULL, strdup'ed)
 * @param attr attribute list
 * @return a new node or NULL on error
 */
static clish_xmlnode_t *clish_expat_make_node(clish_xmlnode_t *parent,
					      clish_xmlnodetype_e type,
					      const char *n,
					      const char *v,
					      const char **attr)
{
	clish_xmlnode_t *node;

	node = malloc(sizeof(clish_xmlnode_t));
	if (!node)
		return NULL;
	node->name = n ? strdup(n) : NULL;
	node->content = v ? strdup(v) : NULL;
	node->children = NULL;
	node->attributes = NULL;
	node->next = NULL;
	node->parent = parent;
	node->doc = parent ? parent->doc : NULL;
	node->depth = parent ? parent->depth + 1 : 0;
	node->type = type;

	if (attr)
		clish_expat_add_attrlist(node, attr);

	if (parent)
		parent->children = clish_expat_list_push_back(parent->children, node);

	return node;
}

/** Add a new XML root
 *
 * @param doc XML document
 * @param el root node name
 * @param attr expat attribute list
 * @return a new root element
 */
static clish_xmlnode_t *clish_expat_add_root(clish_xmldoc_t *doc, const char *el, const char **attr)
{
	clish_xmlnode_t *node;

	node = clish_expat_make_node(NULL, CLISH_XMLNODE_ELM, el, NULL, attr);
	if (!node)
		return doc->root;

	doc->root = clish_expat_list_push_back(doc->root, node);

	return node;
}

/** Add a new XML element as a child
 *
 * @param cur parent XML element
 * @param el element name
 * @param attr expat attribute list
 * @return a new XMl element
 */
static clish_xmlnode_t *clish_expat_add_child(clish_xmlnode_t *cur, const char *el, const char **attr)
{
	clish_xmlnode_t *node;

	node = clish_expat_make_node(cur, CLISH_XMLNODE_ELM, el, NULL, attr);
	if (!node)
		return cur;

	return node;
}

/** Expat handler: element content
 *
 * @param data user data
 * @param s content (not nul-termainated)
 * @param len content length
 */
static void clish_expat_chardata_handler(void *data, const char *s, int len)
{
	clish_xmldoc_t *doc = data;

	if (doc->current) {
		char *content = malloc(len + 1);
		strncpy(content, s, len);
		content[len] = '\0';

		clish_expat_make_node(doc->current, CLISH_XMLNODE_TEXT, NULL, content, NULL);
		/*
		 * the previous call is a bit too generic, and strdup() content
		 * so we need to free out own version of content.
		 */
		free(content);
	}
}

/** Expat handler: start XML element
 *
 * @param data user data
 * @param el element name (nul-terminated)
 * @param attr expat attribute list
 */
static void clish_expat_element_start(void *data, const char *el, const char **attr)
{
	clish_xmldoc_t *doc = data;

	if (!doc->current) {
		doc->current = clish_expat_add_root(doc, el, attr);
	} else {
		doc->current = clish_expat_add_child(doc->current, el, attr);
	}
}

/** Expat handler: end XML element
 *
 * @param data user data
 * @param el element name
 */
static void clish_expat_element_end(void *data, const char *el)
{
	clish_xmldoc_t *doc = data;

	if (doc->current) {
		doc->current = doc->current->parent;
	}

	el = el; /* Happy compiler */
}

/** Free a node, its children and its attributes
 *
 * @param node node to free
 */
static void clish_expat_free_node(clish_xmlnode_t *cur)
{
	clish_xmlnode_t *node;
	clish_xmlnode_t *first;

	if (cur->attributes) {
		first = cur->attributes;
		while (first) {
			node = first;
			first = first->next;
			clish_expat_free_node(node);
		}
	}
	if (cur->children) {
		first = cur->children;
		while (first) {
			node = first;
			first = first->next;
			clish_expat_free_node(node);
		}
	}
	if (cur->name)
		free(cur->name);
	if (cur->content)
		free(cur->content);
	free(cur);
}

/*
 * Public interface
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
	clish_xmldoc_t *doc;
	struct stat sb;
	int fd;
	char *buffer;
	XML_Parser parser;
	int rb;

	doc = malloc(sizeof(clish_xmldoc_t));
	if (!doc)
		return NULL;
	memset(doc, 0, sizeof(clish_xmldoc_t));
	doc->filename = strdup(filename);
	parser = XML_ParserCreate(NULL);
	if (!parser)
		goto error_parser_create;
	XML_SetUserData(parser, doc);
	XML_SetCharacterDataHandler(parser, clish_expat_chardata_handler);
	XML_SetElementHandler(parser,
		clish_expat_element_start,
		clish_expat_element_end);

	fd = open(filename, O_RDONLY);
	if (fd < 0)
		goto error_open;
	fstat(fd, &sb);
	buffer = malloc(sb.st_size+1);
	rb = read(fd, buffer, sb.st_size);
	if (rb < 0) {
		close(fd);
		goto error_parse;
	}
	buffer[sb.st_size] = 0;
	close(fd);

	if (!XML_Parse(parser, buffer, sb.st_size, 1))
		goto error_parse;

	XML_ParserFree(parser);
	free(buffer);

	return doc;

error_parse:
	free(buffer);

error_open:
	XML_ParserFree(parser);

error_parser_create:
	clish_xmldoc_release(doc);

	return NULL;
}

void clish_xmldoc_release(clish_xmldoc_t *doc)
{
	if (doc) {
		clish_xmlnode_t *node;
		while (doc->root) {
			node = doc->root;
			doc->root = node->next;
			clish_expat_free_node(node);
		}
		if (doc->filename)
			free(doc->filename);
		free(doc);
	}
}

int clish_xmldoc_is_valid(clish_xmldoc_t *doc)
{
	return doc && doc->root;
}

int clish_xmldoc_error_caps(clish_xmldoc_t *doc)
{
	doc = doc; /* Happy compiler */

	return CLISH_XMLERR_NOCAPS;
}

int clish_xmldoc_get_err_line(clish_xmldoc_t *doc)
{
	doc = doc; /* Happy compiler */

	return -1;
}

int clish_xmldoc_get_err_col(clish_xmldoc_t *doc)
{
	doc = doc; /* Happy compiler */

	return -1;
}

const char *clish_xmldoc_get_err_msg(clish_xmldoc_t *doc)
{
	doc = doc; /* Happy compiler */

	return "";
}

int clish_xmlnode_get_type(clish_xmlnode_t *node)
{
	if (node)
		return node->type;
	return CLISH_XMLNODE_UNKNOWN;
}

clish_xmlnode_t *clish_xmldoc_get_root(clish_xmldoc_t *doc)
{
	if (doc)
		return doc->root;
	return NULL;
}

clish_xmlnode_t *clish_xmlnode_parent(clish_xmlnode_t *node)
{
	if (node)
		return node->parent;
	return NULL;
}

clish_xmlnode_t *clish_xmlnode_next_child(clish_xmlnode_t *parent,
					  clish_xmlnode_t *curchild)
{
	if (curchild)
		return curchild->next;
	if (parent)
		return parent->children;
	return NULL;
}

char *clish_xmlnode_fetch_attr(clish_xmlnode_t *node,
			       const char *attrname)
{
	if (node) {
		clish_xmlnode_t *n = node->attributes;
		while (n) {
			if (strcmp(n->name, attrname) == 0)
				return n->content;
			n = n->next;
		}
	}
	return NULL;
}

int clish_xmlnode_get_content(clish_xmlnode_t *node, char *content,
			      unsigned int *contentlen)
{
	unsigned int minlen = 1;

	if (node && content && contentlen) {
		clish_xmlnode_t *children = node->children;
		while (children) {
			if (children->type == CLISH_XMLNODE_TEXT && children->content)
				minlen += strlen(children->content);
			children = children->next;
		}
		if (minlen >= *contentlen) {
			*contentlen = minlen + 1;
			return -E2BIG;
		}
		children = node->children;
		*content = 0;
		while (children) {
			if (children->type == CLISH_XMLNODE_TEXT && children->content)
				strcat(content, children->content);
			children = children->next;
		}
		return 0;
	}

	return -EINVAL;
}

int clish_xmlnode_get_name(clish_xmlnode_t *node, char *name,
	unsigned int *namelen)
{
	if (node && name && namelen) {
		if (strlen(node->name) >= *namelen) {
			*namelen = strlen(node->name) + 1;
			return -E2BIG;
		}
		snprintf(name, *namelen, "%s", node->name);
		name[*namelen - 1] = '\0';
		return 0;
	}
	return -EINVAL;
}

void clish_xmlnode_print(clish_xmlnode_t *node, FILE *out)
{
	if (node) {
		int i;
		clish_xmlnode_t *a;
		for (i=0; i<node->depth; ++i) {
			fprintf(out, "  ");
		}
		fprintf(out, "<%s", node->name);
		a = node->attributes;
		while (a) {
			fprintf(out, " %s='%s'", a->name, a->content);
			a = a->next;
		}
		fprintf(out, ">...");
	}
}

void clish_xml_release(void *p)
{
	p = p; /* Happy compiler */
	/* nothing to release */
}

#endif /* HAVE_LIB_EXPAT */

