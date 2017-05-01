/*!
 * \file xmlparsermacros.h
 * \brief Macros to utilize XML parsing using QXmlStreamReader.
 * \sa For an example, see dbquery.cpp of the tageditor project.
 */

// ensure there are no conflicting macros defined
#include "./undefxmlparsermacros.h"

#define iftag(tagName) if (xmlReader.name() == QLatin1String(tagName))
#define eliftag(tagName) else if (xmlReader.name() == QLatin1String(tagName))
#define else_skip                                                                                                                                    \
    else                                                                                                                                             \
    {                                                                                                                                                \
        xmlReader.skipCurrentElement();                                                                                                              \
    }
#define children while (xmlReader.readNextStartElement())
#define text (xmlReader.readElementText(QXmlStreamReader::ErrorOnUnexpectedElement))
#define attribute(attributeName) (xmlReader.attributes().value(QLatin1String(attributeName)))
#define attributeFlag(attributeName)                                                                                                                 \
    (xmlReader.attributes().hasAttribute(QLatin1String(attributeName))                                                                               \
        && xmlReader.attributes().value(QLatin1String(attributeName)) != QLatin1String("false"))
