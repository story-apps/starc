EXTRA_DIST=libavoid.pc.in

lib_LTLIBRARIES = libavoid.la
libavoid_la_CPPFLAGS = -I$(top_srcdir) -I$(includedir)/libavoid -fPIC
libavoid_la_LDFLAGS = -no-undefined

libavoid_la_SOURCES = connectionpin.cpp \
			connector.cpp \
			connend.cpp \
			geometry.cpp \
			geomtypes.cpp \
			graph.cpp \
			junction.cpp \
			makepath.cpp \
			obstacle.cpp \
			orthogonal.cpp \
			router.cpp \
			shape.cpp \
			timer.cpp \
			vertices.cpp \
			viscluster.cpp \
			visibility.cpp \
			vpsc.cpp \
			hyperedge.cpp \
			hyperedgeimprover.cpp \
			mtst.cpp \
			hyperedgetree.cpp \
			scanline.cpp \
			actioninfo.cpp \
			assertions.h \
			connector.h \
			connectionpin.h \
			connend.h \
			debug.h \
			geometry.h \
			geomtypes.h \
			graph.h \
			junction.h \
			libavoid.h \
			makepath.h \
			obstacle.h \
			orthogonal.h \
			router.h \
			shape.h \
			timer.h \
			vertices.h \
			viscluster.h \
			visibility.h \
			hyperedge.h \
			mtst.h \
			hyperedgetree.h \
			scanline.h \
			actioninfo.h \
			vpsc.h \
			debughandler.h

libavoidincludedir = $(includedir)/libavoid
libavoidinclude_HEADERS = assertions.h \
			connector.h \
			connectionpin.h \
			connend.h \
			debug.h \
                        dllexport.h \
			geometry.h \
			geomtypes.h \
			graph.h \
			junction.h \
			libavoid.h \
			makepath.h \
			obstacle.h \
			orthogonal.h \
			router.h \
			shape.h \
			timer.h \
			vertices.h \
			viscluster.h \
			visibility.h \
			hyperedge.h \
			hyperedgeimprover.h \
			mtst.h \
			hyperedgetree.h \
			scanline.h \
			actioninfo.h \
			vpsc.h \
			debughandler.h

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libavoid.pc

SUBDIRS = . tests

