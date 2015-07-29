#ifndef	bg_xmlparser_h_included
#define	bg_xmlparser_h_included

// from q3mme

#include "../qcommon/q_shared.h"  // fileHandle_t


typedef struct {
	fileHandle_t fileHandle;
	int line;
	int fileSize, filePos;
	int depth;
} BG_XMLParse_t;

typedef struct BG_XMLParseBlock_s {
	char *tagName;
	qboolean (*openHandler)(BG_XMLParse_t *,const struct BG_XMLParseBlock_s *, void *);
	qboolean (*textHandler)(BG_XMLParse_t *,const char *, void *);
} BG_XMLParseBlock_t;

qboolean BG_XMLError(BG_XMLParse_t *parse, const char *msg, ... );
void BG_XMLWarning(BG_XMLParse_t *parse, const char *msg, ... );
qboolean BG_XMLOpen( BG_XMLParse_t *parse, const char *fileName );
qboolean BG_XMLParse( BG_XMLParse_t *parse, const BG_XMLParseBlock_t *fromBlock, const BG_XMLParseBlock_t *parseBlock, void *data );

#endif  // bg_xmlparser_h_included

