#undef TARGET_OS_CPP_BUILTINS
#define TARGET_OS_CPP_BUILTINS() \
do { \
    builtin_define( "__yaosp__" ); \
} while (0)

#undef TARGET_VERSION
#define TARGET_VERSION fprintf( stderr, " (i386 yaosp)" );
