#include <QXmlStreamReader>

/*!
 * \file xmlparsermacros.h
 * \brief Macros to utilize XML parsing using QXmlStreamReader.
 * \sa For an example, see dbquery.cpp of the tageditor project.
 */

#ifndef xmlReader
# error "xmlReader must be defined to use these macros."
#endif

#ifdef iftag
# undef iftag
#endif
#define iftag(tagName) if(xmlReader.name() == QLatin1String(tagName))

#ifdef eliftag
# undef eliftag
#endif
#define eliftag(tagName) else if(xmlReader.name() == QLatin1String(tagName))

#ifdef else_skip
# undef else_skip
#endif
#define else_skip else { xmlReader.skipCurrentElement(); }

#ifdef children
# undef children
#endif
#define children while(xmlReader.readNextStartElement())

#ifdef text
# undef text
#endif
#define text xmlReader.readElementText(QXmlStreamReader::ErrorOnUnexpectedElement)

#ifdef attribute
# undef attribute
#endif
#define attribute(attributeName) xmlReader.attributes().value(QLatin1String(attributeName))
