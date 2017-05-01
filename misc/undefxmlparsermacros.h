/*!
 * \file undefxmlparsermacros.h
 * \brief Undefines macros to utilize XML parsing using QXmlStreamReader.
 * \sa For an example, see dbquery.cpp of the tageditor project.
 */

#ifdef iftag
#undef iftag
#endif
#ifdef eliftag
#undef eliftag
#endif
#ifdef else_skip
#undef else_skip
#endif
#ifdef children
#undef children
#endif
#ifdef text
#undef text
#endif
#ifdef attribute
#undef attribute
#endif
