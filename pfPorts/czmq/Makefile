# Created by: Andrew Thompson <thompsa@FreeBSD.org>
# $FreeBSD$

PORTNAME=	czmq
PORTVERSION=	1.3.2
CATEGORIES=	devel net
MASTER_SITES=	http://download.zeromq.org/
NO_STAGE=	yes

MAINTAINER=	bapt@FreeBSD.org
COMMENT=	High-level C Binding for zmq

LICENSE=	LGPL3

LIB_DEPENDS=	zmq:${PORTSDIR}/devel/zmq

USE_PERL5_BUILD=	yes
USE_GMAKE=	yes
GNU_CONFIGURE=	yes
CONFIGURE_ARGS=	--with-libzmq=${LOCALBASE}
USE_LDCONFIG=	yes

MAN7=		czmq.7 \
		zclock.7 \
		zctx.7 \
		zfile.7 \
		zframe.7 \
		zhash.7 \
		zlist.7 \
		zloop.7 \
		zmsg.7 \
		zmutex.7 \
		zsocket.7 \
		zsockopt.7 \
		zstr.7 \
		zthread.7

post-configure:
	@${REINPLACE_CMD} -e 's|$$(libdir)/pkgconfig|$$(prefix)/libdata/pkgconfig|' ${WRKSRC}/src/Makefile
	@${REINPLACE_CMD} -e 's|/usr/bin/perl|${PERL}|' ${WRKSRC}/doc/mkman

.include <bsd.port.mk>
