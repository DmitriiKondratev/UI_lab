#ifndef LIBRARY_H
#define LIBRARY_H

#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  define LIBRARY_EXPORT __declspec(dllexport)
#  define LIBRARY_IMPORT __declspec(dllimport)
#else
#  define LIBRARY_EXPORT __attribute__((visibility("default")))
#  define LIBRARY_IMPORT __attribute__((visibility("default")))
#endif

#endif // LIBRARY_H
