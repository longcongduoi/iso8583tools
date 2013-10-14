#ifndef _PARSE_H_
#define _PARSE_H_

#define FRM_UNKNOWN 0    //unknown format    U
#define FRM_ISOBITMAP 1  //primary and secondary bitmaps
#define FRM_BITMAP 2     //fixed length bitmap
#define FRM_SUBFIELDS 3  //the contents is split to subfields     SF
#define FRM_TLV1 4       //TLV subfields, 1 byte tag format      TLV1
#define FRM_TLV2 5       //TLV subfields, 2 bytes tag format     TLV2
#define FRM_TLV3 6       //TLV subfields, 3 bytes tag format     TLV3
#define FRM_TLV4 7       //TLV subfields, 4 bytes tag format     TLV4
#define FRM_TLVEMV 8     //TLV subfields, EMV tag format         TLVE
#define FRM_EBCDIC 9    // EBCDIC EEEEE
#define FRM_BCD 10   //  0x012345  CCCCC  BCD
#define FRM_BIN 11   //  12345     BBBBB  BIN
#define FRM_ASCII 12  // "12345"   LLLLL  ASC
#define FRM_FIXED 13     // Fixed length   F12345
#define FRM_HEX	14	// 0x0123FD -> "0123FD"
#define FRM_BCDSF 15	// The field is first converted from BCD to ASCII, and then is split into subfields
#define FRM_BITSTR 16	//Same as BITMAP but does not define subfields
#define FRM_EMVL 17     // Length format for EMV tags
#define FRM_TLVDS 18

typedef struct field
{
	char* data;
	char* tag;
	unsigned int length;
	unsigned int fields;
	struct field **fld;
} field;

typedef struct fldformat
{
	//unsigned int number;
	unsigned int lengthFormat;
	unsigned int lengthLength;
	unsigned short lengthInclusive;
	unsigned int maxLength;
	unsigned int dataFormat;
	unsigned int tagFormat;
	char *description;
	unsigned int maxFields;
	unsigned int fields;
	struct fldformat **fld;
} fldformat;

fldformat *load_format(char*);
void freeFormat(fldformat*);
field *parse_message(char*, unsigned int, fldformat*);
unsigned int get_field_length(char*, unsigned int, fldformat*);
void print_message(field*, fldformat*);
#endif
