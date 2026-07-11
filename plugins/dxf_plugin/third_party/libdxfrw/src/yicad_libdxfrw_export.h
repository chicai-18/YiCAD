/******************************************************************************
**  YiCAD local modification, 2026-07-11: Windows DLL export declarations.  **
******************************************************************************/

#ifndef YICAD_LIBDXFRW_EXPORT_H
#define YICAD_LIBDXFRW_EXPORT_H

#if defined(_WIN32)
#  if defined(YICAD_LIBDXFRW_BUILD)
#    define YICAD_LIBDXFRW_API __declspec(dllexport)
#  else
#    define YICAD_LIBDXFRW_API __declspec(dllimport)
#  endif
#else
#  define YICAD_LIBDXFRW_API
#endif

#endif // YICAD_LIBDXFRW_EXPORT_H
