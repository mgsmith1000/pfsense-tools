# New ports collection makefile for:	php5-extensions
# Date created:			7 Jul 2004
# Whom:				Alex Dupre <ale@FreeBSD.org>
#
# $FreeBSD: ports/lang/php5-extensions/Makefile,v 1.24 2011/11/25 15:50:34 ale Exp $
#

PORTNAME=	php5
PORTVERSION=	1.6
CATEGORIES=	lang
MASTER_SITES=	# none
PKGNAMESUFFIX=	-extensions
DISTFILES=	# none
EXTRACT_ONLY=	# none
NO_STAGE=	yes

MAINTAINER=	ale@FreeBSD.org
COMMENT=	A "meta-port" to install PHP extensions

USE_PHP=	yes
DEFAULT_PHP_VER=5
IGNORE_WITH_PHP=4
USE_PHP_BUILD=	yes

NO_BUILD=	yes

LATEST_LINK=	${PORTNAME}${PKGNAMESUFFIX}

WITH_BCMATH=	yes
WITH_CTYPE=	yes
WITH_DOM=	yes
WITH_FILTER=	yes
WITH_HASH=	yes
WITH_ICONV=	yes
WITH_JSON=	yes
WITH_PDO=	yes
WITH_PDO_SQLITE=yes
WITH_PHAR=	yes
WITH_POSIX=	yes
WITH_SESSION=	yes
WITH_SIMPLEXML=	yes
WITH_SQLITE=	yes
WITH_SQLITE3=	yes
WITH_TOKENIZER=	yes
WITH_XML=	yes
WITH_XMLREADER=	yes
WITH_XMLWRITER=	yes

OPTIONS=	BCMATH          "bc style precision math functions" on \
		BZ2             "bzip2 library support" off \
		CALENDAR        "calendar conversion support" off \
		CTYPE           "ctype functions" on \
		CURL            "CURL support" on \
		DBA             "dba support" off \
		DOM             "DOM support" on \
		EXIF            "EXIF support" off \
		FILEINFO        "fileinfo support" off \
		FILTER          "input filter support" on \
		FRIBIDI         "FriBidi support" off \
		FTP             "FTP support" off \
		GD              "GD library support" off \
		GETTEXT         "gettext library support" off \
		GMP             "GNU MP support" off \
		HASH            "HASH Message Digest Framework" on \
		ICONV           "iconv support" on \
		IMAP            "IMAP support" off \
		INTERBASE       "Interbase 6 database support (Firebird)" off \
		JSON            "JavaScript Object Serialization support" on \
		LDAP            "OpenLDAP support" off \
		MBSTRING        "multibyte string support" off \
		MCRYPT          "Encryption support" on \
		MSSQL           "MS-SQL database support" off \
		MYSQL           "MySQL database support" off \
		MYSQLI          "MySQLi database support" off \
		ODBC            "ODBC support" off \
		OPENSSL         "OpenSSL support" off \
		PCNTL           "pcntl support (CLI only)" off \
		PDF             "PDFlib support (implies GD)" off \
		PDO             "PHP Data Objects Interface (PDO)" on \
		PDO_SQLITE      "PDO sqlite driver" on \
		PGSQL           "PostgreSQL database support" off \
		PHAR            "phar support" on \
		POSIX           "POSIX-like functions" on \
		PSPELL          "pspell support" off \
		READLINE        "readline support (CLI only)" off \
		RECODE          "recode support" off \
		SESSION         "session support" on \
		SHMOP           "shmop support" off \
		SIMPLEXML       "simplexml support" on \
		SNMP            "SNMP support" off \
		SOAP            "SOAP support" off \
		SOCKETS         "sockets support" off \
		SQLITE          "sqlite support" on \
		SQLITE3         "sqlite3 support" on \
		SYBASE_CT       "Sybase database support" off \
		SYSVMSG         "System V message support" off \
		SYSVSEM         "System V semaphore support" off \
		SYSVSHM         "System V shared memory support" off \
		TIDY            "TIDY support" off \
		TOKENIZER       "tokenizer support" on \
		WDDX            "WDDX support (implies XML)" off \
		XML             "XML support" on \
		XMLREADER       "XMLReader support" on \
		XMLRPC          "XMLRPC-EPI support" off \
		XMLWRITER       "XMLWriter support" on \
		XSL             "XSL support (Implies DOM)" off \
		YAZ             "YAZ support (ANSI/NISO Z39.50)" off \
		ZIP             "ZIP support" off \
		ZLIB            "ZLIB support" on

ALL_OPTIONS=	BCMATH BZ2 CALENDAR CTYPE CURL DBA \
		DOM EXIF FILEINFO FILTER FRIBIDI FTP GD GETTEXT \
		GMP HASH ICONV IMAP INTERBASE JSON LDAP MBSTRING MCRYPT \
		MSSQL MYSQL MYSQLI \
		ODBC OPENSSL PCNTL PDF PDO PDO_SQLITE PGSQL PHAR POSIX \
		PSPELL READLINE RECODE SESSION SHMOP SIMPLEXML SNMP SOAP \
		SOCKETS SQLITE SQLITE3 SYBASE_CT SYSVMSG SYSVSEM SYSVSHM \
		TIDY TOKENIZER WDDX XML XMLREADER XMLRPC XMLWRITER XSL \
		YAZ ZIP ZLIB

.include <bsd.port.options.mk>

.for opt in ${ALL_OPTIONS}
.	if defined(WITH_${opt}) && !defined(WITHOUT_${opt})
USE_PHP+=	${opt:L}
.	endif
.endfor

do-install: build
	@${DO_NADA}

.include <bsd.port.mk>
