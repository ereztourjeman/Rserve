/*
 *  Rsrv.h : constants and macros for Rsrv client/server architecture
 *  Copyright (C) 2002 Simon Urbanek
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  $Id$
 */

#ifndef __RSRV_H__
#define __RSRV_H__

#define default_Rsrv_port 6311

/* Rserv communication is done over any reliable connection-oriented
   protocol (usually TCP/IP). After connection is established, server
   sends 32bytes of ID-string defining the capabilities of the server.
   each attribute of the ID-string is 4 bytes long and is meant to be user-
   readable (i.e. use no special characters), and it's a good idea to make
   "\r\n\r\n" the last attribute

   the ID string must be of the form:

   [0] "Rsrv" - R-server ID signature
   [4] "0100" - version of the R server
   [8] "QAP1" - protocol used for communication (here Quad Attributes Packets v1)
   [12] any additional attributes follow. \r\n<space> and '-' are ignored.

   optional attributes
   (in any order; it is legitimate to put dummy attributes, like "----" or
    "    " between attributes):

   "R151" - version of R (here 1.5.1)
   "ARpt" - authorization required (here "pt"=plain text, "uc"=unix crypt)
            connection will be closed
            if the first packet is not CMD_login.
	    if more AR.. methods are specified, then client is free to
	    use the one he supports (usually the most secure)
   "K***" - key if encoded authentification is challenged (*** is the key)
            for unix crypt the first two letters of the key are the salt
	    required by the server */

/* QAP1 transport protocol header structure

   all int and double entries throughout the transfer are in
   Intel-endianess format: int=0x12345678 -> char[4]=(0x78,0x56,x34,0x12)
   functions/macros for converting from native to protocol format 
   are available below */

struct phdr { /* always 16 bytes */
  int cmd; /* command */
  int len; /* length of the packet minus header (ergo -16) */
  int dof; /* data offset behind header (ergo usually 0) */
  int res; /* reserved - but must be sent so the minimal packet has 16 bytes */
};

/* each entry in the data section (aka parameter list) is preceded by 4 bytes:
   1 byte : parameter type
   3 bytes: length
   parameter list may be terminated by 0/0/0/0 but doesn't have to since "len"
   field specifies the packet length sufficiently (hint: best method for parsing is
   to allocate len+4 bytes, set the last 4 bytes to 0 and trverse list of parameters
   until (int)0 occurs */

/* macros for handling the first int - split/combine */
#define PAR_TYPE(X) ((X)&255)
#define PAR_LEN(X) ((X)>>8)
#define PAR_LENGTH PAR_LEN
#define SET_PAR(TY,LEN) ((((LEN)&0x7fffff)<<8)|((TY)&255))

#define CMD_STAT(X) (((X)>>24)&127) /* returns the stat code of the response */
#define SET_STAT(X,s) ((X)|(((s)&127)<<24)) /* sets the stat code */

#define CMD_RESP 0x10000  /* all responses have this flag set */

#define RESP_OK (CMD_RESP|0x0001) /* command succeeded; returned parameters depend
				     on the command issued */
#define RESP_ERR (CMD_RESP|0x0002) /* command failed, check stats code
				      attached string may describe the error */

/* stat codes; 0-0x3f are reserved for program specific codes - e.g. for R
   connection they correspond to the stat of Parse command.
   the following codes are returned by the Rserv itself

   codes <0 denote Rerror as provided by R_tryEval
 */
#define ERR_auth_failed      0x41 /* auth.failed or auth.reqeusted but no
				     login came. in case of authentification
				     failure due to name/pwd mismatch,
				     server may send CMD_accessDenied instead
				  */
#define ERR_conn_broken      0x42 /* connection closed or broken packet killed it */
#define ERR_inv_cmd          0x43 /* unsupported/invalid command */
#define ERR_inv_par          0x44 /* some pars are invalid */
#define ERR_Rerror           0x45 /* R-error occured, usually followed by
				     connection shutdown */
#define ERR_IOerror          0x46 /* I/O error */
#define ERR_notOpen          0x47 /* attempt to perform fileRead/Write 
				     on closed file */
#define ERR_accessDenied     0x48 /* this answer is also valid on
				     CMD_login; otherwise it's sent
				     if the server deosn;t allow the user
				     to issue the specified commend.
				     (e.g. some server admins may block
				     file I/O operations for some users) */
#define ERR_unsupportedCmd   0x49 /* unsupported command */
#define ERR_unknownCmd       0x4a /* unknown command - the difference
				     between unsupported and unknown is that
				     unsupported commands are known to the
				     server but for some reasons (e.g.
				     platform dependent) it's not supported.
				     unknown commands are simply not recognized
				     by the server at all. */

/* availiable commands */

#define CMD_login        0x001 /* "name\npwd" : - */
#define CMD_voidEval     0x002 /* string : - */
#define CMD_eval         0x003 /* string : encoded SEXP */
#define CMD_shutdown     0x004 /* [admin-pwd] : - */
/* file I/O routines. server may answe */
#define CMD_openFile     0x010 /* fn : - */
#define CMD_createFile   0x011 /* fn : - */
#define CMD_closeFile    0x012 /* - : - */
#define CMD_readFile     0x013 /* [int size] : data... ; if size not present,
				  server is free to choose any value - usually
				  it uses the size of its static buffer */
#define CMD_writeFile    0x014 /* data : - */

/* data types for the transport protocol (QAP1)
   do NOT confuse with XT_.. values. */

#define DT_INT        1  /* int */
#define DT_CHAR       2  /* char */
#define DT_DOUBLE     3  /* double */
#define DT_STRING     4  /* 0 terminted string */
#define DT_BYTESTREAM 5  /* stream of bytes (unlike DT_STRING may contain 0) */
#define DT_SEXP       10 /* encoded SEXP */
#define DT_ARRAY      11 /* array of objects (i.e. first 4 bytes specify how many
			    subsequent objects are part of the array; 0 is legitimate) */

/* XpressionTypes
   REXP - R expressions are packed in the same way as command parameters
   transport format of the encoded Xpressions:
   [0] int type/len (1 byte type, 3 bytes len - same as SET_PAR)
   [4] REXP attr (if bit 8 in type is set)
   [4/8] data .. */

#define XT_NULL          0 /* data: [0] */
#define XT_INT           1 /* data: [4]int */
#define XT_DOUBLE        2 /* data: [8]double */
#define XT_STR           3 /* data: [n]char null-term. strg. */
#define XT_LANG          4 /* data: ? lang XP */
#define XT_SYM           5 /* data: [n]char symbol name */
#define XT_BOOL          6 /* data: [1]byte boolean
			      (1=TRUE, 0=FALSE, 2=NA) */
#define XT_VECTOR        16 /* data: [?]REXP */
#define XT_LIST          17 /* X head, X vals */

#define XT_ARRAY_INT     32 /* data: [n*4]int,int,.. */
#define XT_ARRAY_DOUBLE  33 /* data: [n*8]double,double,.. */
#define XT_ARRAY_STR     34 /* data: [?]string,string,.. */
#define XT_ARRAY_BOOL    35 /* data: [n]byte,byte,.. */

#define XT_UNKNOWN       48 /* data: ? */

#define XT_HAS_ATTR      128 /* flag; if set, the following REXP is the
				attribute */
/* the use of attributes and vectors results in recursive storage of REXPs */

#define BOOL_TRUE  1
#define BOOL_FALSE 0
#define BOOL_NA    2

#define GET_XT(X) ((X)&127)
#define HAS_ATTR(X) (((X)&XT_HAS_ATTR)>0)

/* functions/macros to convert native endianess of int/double for transport
   currently ony PPC style and Intel style are supported */

#ifdef SWAPEND  /* swap endianness - for PPC and co. */
int itop(int i) { char b[4]; b[0]=((char*)&i)[3]; b[3]=((char*)&i)[0]; b[1]=((char*)&i)[2]; b[2]=((char*)&i)[1]; return *((int*)b); };
double dtop(double i) { char b[8]; b[0]=((char*)&i)[7]; b[1]=((char*)&i)[6]; b[2]=((char*)&i)[5]; b[3]=((char*)&i)[4]; b[7]=((char*)&i)[0]; b[6]=((char*)&i)[1]; b[5]=((char*)&i)[2]; b[4]=((char*)&i)[3]; return *((double*)b); };
#define ptoi(X) itop(X) /* itop*itop=id */
#define ptod(X) dtop(X)
#else
#define itop(X) (X)
#define ptoi(X) (X)
#define dtop(X) (X)
#define ptod(X) (X)
#endif

#ifndef HAVE_CONFIG_H
/* this tiny function can be used to make sure that the endianess
   is correct (it is not included in the package was configured with
   autoconf since then it should be fine anyway */
int isByteSexOk() {
  int i;
  i=itop(0x12345678);
  return (*((char*)&i)==0x78);
}
#else
#define isByteSexOk 1
#endif

#endif