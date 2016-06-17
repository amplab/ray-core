# This file is read into the GN build.

# Files are relative to third_party/skia.
{
  'skia_library_sources': [
    '<(skia_src_path)/ports/SkImageGenerator_none.cpp',

    '<(skia_include_path)/images/SkMovie.h',
    '<(skia_src_path)/fonts/SkFontMgr_indirect.cpp',
    '<(skia_src_path)/fonts/SkRemotableFontMgr.cpp',
    '<(skia_src_path)/ports/SkFontHost_FreeType_common.cpp',
    '<(skia_src_path)/ports/SkFontHost_FreeType_common.h',
    '<(skia_src_path)/ports/SkFontHost_FreeType.cpp',
    '<(skia_src_path)/ports/SkFontHost_mac.cpp',
    '<(skia_src_path)/ports/SkFontMgr_android.cpp',
    '<(skia_src_path)/ports/SkFontMgr_android_factory.cpp',
    '<(skia_src_path)/ports/SkFontMgr_android_parser.cpp',
    '<(skia_src_path)/ports/SkGlobalInitialization_default.cpp',
    '<(skia_src_path)/ports/SkImageEncoder_none.cpp',
    '<(skia_src_path)/ports/SkOSFile_posix.cpp',
    '<(skia_src_path)/ports/SkOSFile_stdio.cpp',
    '<(skia_src_path)/ports/SkTLS_pthread.cpp',
    '<(skia_src_path)/sfnt/SkOTTable_name.cpp',
    '<(skia_src_path)/sfnt/SkOTTable_name.h',
    '<(skia_src_path)/sfnt/SkOTUtils.cpp',
    '<(skia_src_path)/sfnt/SkOTUtils.h',

    #mac
    '<(skia_src_path)/utils/mac/SkStream_mac.cpp',

    #windows

    #testing
    '<(skia_src_path)/fonts/SkGScalerContext.cpp',
    '<(skia_src_path)/fonts/SkGScalerContext.h',
  ],
}
