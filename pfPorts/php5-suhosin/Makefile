# New ports collection makefile for:	suhosin
# Date created:			4 Sep 2006
# Whom:				ale
#
# $FreeBSD: ports/security/php-suhosin/Makefile,v 1.24 2012/02/03 09:04:56 ale Exp $
#

PORTNAME=	suhosin
PORTVERSION=	0.9.33
CATEGORIES=	security
MASTER_SITES=	http://download.suhosin.org/
PKGNAMEPREFIX=	php-
EXTRACT_SUFX=	.tgz
NO_STAGE=	yes

MAINTAINER=	ale@FreeBSD.org
COMMENT=	A PHP extension that implements high-level protections

USE_PHP=	yes
USE_PHPEXT=	yes
IGNORE_WITH_PHP=4

CONFIGURE_ARGS=	--enable-suhosin

post-install:
	${MKDIR} ${EXAMPLESDIR}
	${INSTALL_DATA} ${WRKSRC}/suhosin.ini ${EXAMPLESDIR}

.include <bsd.port.mk>
