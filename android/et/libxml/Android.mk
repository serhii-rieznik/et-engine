LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libxml

LOCAL_SRC_FILES := c14n.c catalog.c chvalid.c debugXML.c dict.c DOCBparser.c encoding.c entities.c error.c \
	globals.c hash.c HTMLparser.c HTMLtree.c legacy.c list.c nanoftp.c nanohttp.c parser.c parserInternals.c \
	pattern.c relaxng.c SAX.c SAX2.c schematron.c threads.c tree.c trionan.c triostr.c uri.c valid.c \
	xinclude.c xlink.c xmlcatalog.c xmlIO.c xmlmemory.c xmlmodule.c xmlreader.c xmlregexp.c xmlsave.c \
	xmlschemas.c xmlschemastypes.c xmlstring.c xmlunicode.c xmlwriter.c xpath.c xpointer.c xzlib.c

include $(BUILD_STATIC_LIBRARY)
