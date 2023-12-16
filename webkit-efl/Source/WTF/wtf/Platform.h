/*
 * Copyright (C) 2006, 2007, 2008, 2009, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
 * Copyright (C) 2010, 2011 Research In Motion Limited. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WTF_Platform_h
#define WTF_Platform_h

/* Include compiler specific macros */
#include <wtf/Compiler.h>

/* ==== PLATFORM handles OS, operating environment, graphics API, and
   CPU. This macro will be phased out in favor of platform adaptation
   macros, policy decision macros, and top-level port definitions. ==== */
#define PLATFORM(WTF_FEATURE) (defined WTF_PLATFORM_##WTF_FEATURE  && WTF_PLATFORM_##WTF_FEATURE)


/* ==== Platform adaptation macros: these describe properties of the target environment. ==== */

/* CPU() - the target CPU architecture */
#define CPU(WTF_FEATURE) (defined WTF_CPU_##WTF_FEATURE  && WTF_CPU_##WTF_FEATURE)
/* HAVE() - specific system features (headers, functions or similar) that are present or not */
#define HAVE(WTF_FEATURE) (defined HAVE_##WTF_FEATURE  && HAVE_##WTF_FEATURE)
/* OS() - underlying operating system; only to be used for mandated low-level services like 
   virtual memory, not to choose a GUI toolkit */
#define OS(WTF_FEATURE) (defined WTF_OS_##WTF_FEATURE  && WTF_OS_##WTF_FEATURE)


/* ==== Policy decision macros: these define policy choices for a particular port. ==== */

/* USE() - use a particular third-party library or optional OS service */
#define USE(WTF_FEATURE) (defined WTF_USE_##WTF_FEATURE  && WTF_USE_##WTF_FEATURE)
/* ENABLE() - turn on a specific feature of WebKit */
#define ENABLE(WTF_FEATURE) (defined ENABLE_##WTF_FEATURE  && ENABLE_##WTF_FEATURE)


/* ==== CPU() - the target CPU architecture ==== */

/* This also defines CPU(BIG_ENDIAN) or CPU(MIDDLE_ENDIAN) or neither, as appropriate. */

/* CPU(ALPHA) - DEC Alpha */
#if defined(__alpha__)
#define WTF_CPU_ALPHA 1
#endif

/* CPU(HPPA) - HP PA-RISC */
#if defined(__hppa__) || defined(__hppa64__)
#define WTF_CPU_HPPA 1
#endif

/* CPU(IA64) - Itanium / IA-64 */
#if defined(__ia64__)
#define WTF_CPU_IA64 1
/* 32-bit mode on Itanium */
#if !defined(__LP64__)
#define WTF_CPU_IA64_32 1
#endif
#endif

/* CPU(MIPS) - MIPS 32-bit */
/* Note: Only O32 ABI is tested, so we enable it for O32 ABI for now.  */
#if (defined(mips) || defined(__mips__) || defined(MIPS) || defined(_MIPS_)) \
    && defined(_ABIO32)
#define WTF_CPU_MIPS 1
#if defined(__MIPSEB__)
#define WTF_CPU_BIG_ENDIAN 1
#endif
#define WTF_MIPS_PIC (defined __PIC__)
#define WTF_MIPS_ARCH __mips
#define WTF_MIPS_ISA(v) (defined WTF_MIPS_ARCH && WTF_MIPS_ARCH == v)
#define WTF_MIPS_ISA_AT_LEAST(v) (defined WTF_MIPS_ARCH && WTF_MIPS_ARCH >= v)
#define WTF_MIPS_ARCH_REV __mips_isa_rev
#define WTF_MIPS_ISA_REV(v) (defined WTF_MIPS_ARCH_REV && WTF_MIPS_ARCH_REV == v)
#define WTF_MIPS_DOUBLE_FLOAT (defined __mips_hard_float && !defined __mips_single_float)
#define WTF_MIPS_FP64 (defined __mips_fpr && __mips_fpr == 64)
/* MIPS requires allocators to use aligned memory */
#define WTF_USE_ARENA_ALLOC_ALIGNMENT_INTEGER 1
#endif /* MIPS */

/* CPU(PPC) - PowerPC 32-bit */
#if   defined(__ppc__)     \
    || defined(__PPC__)     \
    || defined(__powerpc__) \
    || defined(__powerpc)   \
    || defined(__POWERPC__) \
    || defined(_M_PPC)      \
    || defined(__PPC)
#define WTF_CPU_PPC 1
#define WTF_CPU_BIG_ENDIAN 1
#endif

/* CPU(PPC64) - PowerPC 64-bit */
#if   defined(__ppc64__) \
    || defined(__PPC64__)
#define WTF_CPU_PPC64 1
#define WTF_CPU_BIG_ENDIAN 1
#endif

/* CPU(SH4) - SuperH SH-4 */
#if defined(__SH4__)
#define WTF_CPU_SH4 1
#endif

/* CPU(SPARC32) - SPARC 32-bit */
#if defined(__sparc) && !defined(__arch64__) || defined(__sparcv8)
#define WTF_CPU_SPARC32 1
#define WTF_CPU_BIG_ENDIAN 1
#endif

/* CPU(SPARC64) - SPARC 64-bit */
#if defined(__sparc__) && defined(__arch64__) || defined (__sparcv9)
#define WTF_CPU_SPARC64 1
#define WTF_CPU_BIG_ENDIAN 1
#endif

/* CPU(SPARC) - any SPARC, true for CPU(SPARC32) and CPU(SPARC64) */
#if CPU(SPARC32) || CPU(SPARC64)
#define WTF_CPU_SPARC 1
#endif

/* CPU(S390X) - S390 64-bit */
#if defined(__s390x__)
#define WTF_CPU_S390X 1
#define WTF_CPU_BIG_ENDIAN 1
#endif

/* CPU(S390) - S390 32-bit */
#if defined(__s390__)
#define WTF_CPU_S390 1
#define WTF_CPU_BIG_ENDIAN 1
#endif

/* CPU(X86) - i386 / x86 32-bit */
#if   defined(__i386__) \
    || defined(i386)     \
    || defined(_M_IX86)  \
    || defined(_X86_)    \
    || defined(__THW_INTEL)
#define WTF_CPU_X86 1
#endif

/* CPU(X86_64) - AMD64 / Intel64 / x86_64 64-bit */
#if   defined(__x86_64__) \
    || defined(_M_X64)
#define WTF_CPU_X86_64 1
#endif

/* CPU(ARM) - ARM, any version*/
#define WTF_ARM_ARCH_AT_LEAST(N) (CPU(ARM) && WTF_ARM_ARCH_VERSION >= N)

#if   defined(arm) \
    || defined(__arm__) \
    || defined(ARM) \
    || defined(_ARM_)
#define WTF_CPU_ARM 1

#if defined(__ARM_PCS_VFP)
#define WTF_CPU_ARM_HARDFP 1
#endif

#if defined(__ARMEB__) || (COMPILER(RVCT) && defined(__BIG_ENDIAN))
#define WTF_CPU_BIG_ENDIAN 1

#elif !defined(__ARM_EABI__) \
    && !defined(__EABI__) \
    && !defined(__VFP_FP__) \
    && !defined(_WIN32_WCE)
#define WTF_CPU_MIDDLE_ENDIAN 1

#endif

/* Set WTF_ARM_ARCH_VERSION */
#if   defined(__ARM_ARCH_4__) \
    || defined(__ARM_ARCH_4T__) \
    || defined(__MARM_ARMV4__)
#define WTF_ARM_ARCH_VERSION 4

#elif defined(__ARM_ARCH_5__) \
    || defined(__ARM_ARCH_5T__) \
    || defined(__MARM_ARMV5__)
#define WTF_ARM_ARCH_VERSION 5

#elif defined(__ARM_ARCH_5E__) \
    || defined(__ARM_ARCH_5TE__) \
    || defined(__ARM_ARCH_5TEJ__)
#define WTF_ARM_ARCH_VERSION 5
/*ARMv5TE requires allocators to use aligned memory*/
#define WTF_USE_ARENA_ALLOC_ALIGNMENT_INTEGER 1

#elif defined(__ARM_ARCH_6__) \
    || defined(__ARM_ARCH_6J__) \
    || defined(__ARM_ARCH_6K__) \
    || defined(__ARM_ARCH_6Z__) \
    || defined(__ARM_ARCH_6ZK__) \
    || defined(__ARM_ARCH_6T2__) \
    || defined(__ARMV6__)
#define WTF_ARM_ARCH_VERSION 6

#elif defined(__ARM_ARCH_7A__) \
    || defined(__ARM_ARCH_7R__) \
    || defined(__ARM_ARCH_7S__)
#define WTF_ARM_ARCH_VERSION 7

/* MSVC sets _M_ARM */
#elif defined(_M_ARM)
#define WTF_ARM_ARCH_VERSION _M_ARM

/* RVCT sets _TARGET_ARCH_ARM */
#elif defined(__TARGET_ARCH_ARM)
#define WTF_ARM_ARCH_VERSION __TARGET_ARCH_ARM

#if defined(__TARGET_ARCH_5E) \
    || defined(__TARGET_ARCH_5TE) \
    || defined(__TARGET_ARCH_5TEJ)
/*ARMv5TE requires allocators to use aligned memory*/
#define WTF_USE_ARENA_ALLOC_ALIGNMENT_INTEGER 1
#endif

#else
#define WTF_ARM_ARCH_VERSION 0

#endif

/* Set WTF_THUMB_ARCH_VERSION */
#if   defined(__ARM_ARCH_4T__)
#define WTF_THUMB_ARCH_VERSION 1

#elif defined(__ARM_ARCH_5T__) \
    || defined(__ARM_ARCH_5TE__) \
    || defined(__ARM_ARCH_5TEJ__)
#define WTF_THUMB_ARCH_VERSION 2

#elif defined(__ARM_ARCH_6J__) \
    || defined(__ARM_ARCH_6K__) \
    || defined(__ARM_ARCH_6Z__) \
    || defined(__ARM_ARCH_6ZK__) \
    || defined(__ARM_ARCH_6M__)
#define WTF_THUMB_ARCH_VERSION 3

#elif defined(__ARM_ARCH_6T2__) \
    || defined(__ARM_ARCH_7__) \
    || defined(__ARM_ARCH_7A__) \
    || defined(__ARM_ARCH_7M__) \
    || defined(__ARM_ARCH_7R__) \
    || defined(__ARM_ARCH_7S__)
#define WTF_THUMB_ARCH_VERSION 4

/* RVCT sets __TARGET_ARCH_THUMB */
#elif defined(__TARGET_ARCH_THUMB)
#define WTF_THUMB_ARCH_VERSION __TARGET_ARCH_THUMB

#else
#define WTF_THUMB_ARCH_VERSION 0
#endif


/* CPU(ARMV5_OR_LOWER) - ARM instruction set v5 or earlier */
/* On ARMv5 and below the natural alignment is required. 
   And there are some other differences for v5 or earlier. */
#if !defined(ARMV5_OR_LOWER) && !WTF_ARM_ARCH_AT_LEAST(6)
#define WTF_CPU_ARMV5_OR_LOWER 1
#endif


/* CPU(ARM_TRADITIONAL) - Thumb2 is not available, only traditional ARM (v4 or greater) */
/* CPU(ARM_THUMB2) - Thumb2 instruction set is available */
/* Only one of these will be defined. */
#if !defined(WTF_CPU_ARM_TRADITIONAL) && !defined(WTF_CPU_ARM_THUMB2)
#  if defined(thumb2) || defined(__thumb2__) \
    || ((defined(__thumb) || defined(__thumb__)) && WTF_THUMB_ARCH_VERSION == 4)
#    define WTF_CPU_ARM_TRADITIONAL 0
#    define WTF_CPU_ARM_THUMB2 1
#  elif WTF_ARM_ARCH_AT_LEAST(4)
#    define WTF_CPU_ARM_TRADITIONAL 1
#    define WTF_CPU_ARM_THUMB2 0
#  else
#    error "Not supported ARM architecture"
#  endif
#elif CPU(ARM_TRADITIONAL) && CPU(ARM_THUMB2) /* Sanity Check */
#  error "Cannot use both of WTF_CPU_ARM_TRADITIONAL and WTF_CPU_ARM_THUMB2 platforms"
#endif /* !defined(WTF_CPU_ARM_TRADITIONAL) && !defined(WTF_CPU_ARM_THUMB2) */

#if defined(__ARM_NEON__) && !defined(WTF_CPU_ARM_NEON)
#define WTF_CPU_ARM_NEON 1
#endif

#if defined(__ARM_NEON__) && PLATFORM(TIZEN)

#if defined (WTF_CPU_ARM_NEON)
// All NEON intrinsics usage enabled for Tizen by this macro.
#define HAVE_ARM_NEON_INTRINSICS 1
#endif

#else // defined(__ARM_NEON__) && PLATFORM(TIZEN)

#if CPU(ARM_NEON) && (!COMPILER(GCC) || GCC_VERSION_AT_LEAST(4, 7, 0))
// All NEON intrinsics usage can be disabled by this macro.
#define HAVE_ARM_NEON_INTRINSICS 1
#endif

#endif // defined(__ARM_NEON__) && PLATFORM(TIZEN)

#if (defined(__VFP_FP__) && !defined(__SOFTFP__))
#define WTF_CPU_ARM_VFP 1
#endif

#if defined(__ARM_ARCH_7S__)
#define WTF_CPU_APPLE_ARMV7S 1
#endif

#endif /* ARM */

#if CPU(ARM) || CPU(MIPS) || CPU(SH4) || CPU(SPARC)
#define WTF_CPU_NEEDS_ALIGNED_ACCESS 1
#endif

/* Samsung Platforms */
#if PLATFORM(TIZEN)

#define ENABLE_TIZEN_PROCESS_PERMISSION_CONTROL 1 /* Yunchan Cho(yunchan.cho@samsung.com), Ryuan Choi(ryuan.choi@samsung.com) : Change smack label of launched webkit processes */

#if USE(ACCELERATED_COMPOSITING)
#if USE(TIZEN_PLATFORM_SURFACE)
#define ENABLE_TIZEN_CANVAS_GRAPHICS_SURFACE 1 /* Heejin Chung(heejin.r.chung@samsung.com) : WebGL Based on GraphicsSurface */
#define ENABLE_TIZEN_ACCELERATED_2D_CANVAS_EFL 1 /* Kyungjin Kim(gen.kim@samsung.com), Hyunki Baik(hyunki.baik@samsung.com) : Accelerated 2D Canvas */
#define ENABLE_TIZEN_WEBGL_MULTIBUFFER 1 /* Yonggeol Jung(yg48.jung@samsung.com) : Use multiple buffers for webgl rendering to improve performance */
#define ENABLE_TIZEN_RUNTIME_BACKEND_SELECTION 1 /* Hyowon Kim(hw1008.kim@samsung.com), Yonggeol Jung(yg48.jung@samsung.com) : Allow selecting the backend for Texture Mapper at run-time */
// FIXME : rebase
// Direct image layer does not working properly with TIZEN_PLATFORM_SURFACE macro.
#define ENABLE_TIZEN_DISABLE_DIRECTLY_COMPOSITED_IMAGE 0 /* Hurnjoo Lee(hurnjoo.lee@samsung.com) : Disable directly composited image */
#define ENABLE_TIZEN_WEBKIT2_DEBUG_BORDERS 0 /* Hyowon Kim(hw1008.kim@samsung.com) : Renders a border around composited Render Layers to help debug and study layer compositing. */
#if ENABLE(TIZEN_ACCELERATED_2D_CANVAS_EFL)
#define ENABLE_TIZEN_CANVAS_CAIRO_GLES_RENDERING 1 /* Kyungjin Kim(gen.kim@samsung.com), Hyunki Baik(hyunki.baik@samsung.com) : canvas cairo/GLES rendering */
#define ENABLE_TIZEN_CANVAS_CAIRO_GLES_OFFSCREEN_BUFFER 0 /* Kyungjin Kim(gen.kim@samsung.com) : Use
 offscreen buffer for rendering */
#if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
#define ENABLE_TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE 1 /* Byeongha Cho(byeongha.cho@samsung.com) : Use pixmap surface to decode image and share the buffer between CPU and GPU */
#define ENABLE_TIZEN_CAIROGLES_MULTISAMPLE_WORKAROUND 1
#endif // #if ENABLE(TIZEN_CANVAS_CAIRO_GLES_RENDERING)
#endif // #if ENABLE(TIZEN_ACCELERATED_2D_CANVAS_EFL)
#endif // #if USE(TIZEN_PLATFORM_SURFACE)
#endif // #if USE(ACCELERATED_COMPOSITING)
#define ENABLE_TIZEN_VIEW_MODE_CSS_MEDIA 1 //karthik.sh@samsung.com  support added for view-mode css property
#define ENABLE_TIZEN_CORE_THEME_SCALE 1 /* Ryuan Choi(ryuan.choi@samsung.com) Use scaled theme for better qurality */
#define ENABLE_TIZEN_WEBKIT2_ABOUT_MEMORY 0 /* Jinwoo Song(jinwoo7.song@samsung.com) : Implement 'About:Memory' feature */
#define ENABLE_TIZEN_WEBKIT2_PROXY 1 /* Ryuan Choi(ryuan.choi@samsung.com) : Implement network proxy */
#define ENABLE_TIZEN_ENLARGE_CARET_WIDTH 1 /* Jinwoo Song(jinwoo7.song@samsung.com): enlarge default caret width(size) - Email team is demanding this function. When composing, it is difficult to know where caret is.*/
#define ENABLE_TIZEN_TEXT_CARET_HANDLING_WK2 1 /* Piotr Roguski(p.roguski@samsung.com) : enables two methods for getting and setting text caret position in editable fields */
#define ENABLE_TIZEN_TEXT_ZOOM 1 /* Ryuan Choi(ryuan.choi@samsung.com) : API for text zoom, it should be upstreamed. */
#define ENABLE_TIZEN_FIND_STRING 1 /* Jinwoo Song(jinwoo7.song@samsung.com) : Fix the bug to enable searching the web page which has the 'webkit-user-select: none' CSS property and value. */
#define ENABLE_TIZEN_SYSTEM_FONT 1 /* Ryuan Choi(ryuan.choi@samsung.com) : Bug fix, default width of input field is too big. */
#define ENABLE_TIZEN_FALLBACK_THEME 1 /* Ryuan Choi(ryuan.choi@samsung.com) : TIZEN applications didn't set theme so we locally used it */
#define ENABLE_TIZEN_CERTIFICATE_HANDLING 1 /* Krzysztof Czech(k.czech@samsung.com) : support for certificate callback function set by UI */ /* DongJae KIM : Enable Patch*/
#define ENABLE_TIZEN_BACKGROUND_DISK_CACHE 1 /* Byeongha.cho(byeongha.cho@samsung.com), Sungman Kim(ssungmai.kim@samsung.com) : Cache encoded image data to disk on background state. */
#define ENABLE_TIZEN_POPUP_INTERNAL 1 /*Byungjun Kim(bj1987.kim@samsung.com) : Popup feature */
#define ENABLE_TIZEN_MEDIA_NULL_CHECK_WORKAROUND 1 /* Sanghyup Lee(sh53.lee@samsung.com) : Add null check to fix crash issue. */
#define ENABLE_TIZEN_INPUT_DATE_TIME 1 /*Byungjun Kim(bj1987.kim@samsung.com) : Input Date and Time feature */
#define ENABLE_TIZEN_LOAD_NONEMPTY_LAYOUT_FINISHED_HANDLING 1 /* Joonghun Park(jh718.park@samsung.com) : Add "load,nonemptylayout,finished" signal and the handling function for it. */
#define ENABLE_TIZEN_ADJUST_MEDIA_CONTROL 1 /*Byungjun Kim(bj1987.kim@samsung.com) : support for adjusting media control */
#define ENABLE_TIZEN_CALL_SIGNAL_FOR_WRT 1 /*Byungjun Kim(bj1987.kim@samsung.com) : add signal(ime, mediaControl, contextMenu) for WRT */

#define ENABLE_TIZEN_ROOT_INLINE_BOX_SELECTION_TOP 1 /* Kamil Blank(k.blank@samsung.com) Jinwoo Song(jinwoo7.song@samsung.com) : Fix for too high selection on bbc.co.uk, https://bugs.webkit.org/show_bug.cgi?id=65307 */
#define ENABLE_TIZEN_PREVENT_CRASH_OF_TOUCH_EVENTS 1 /* Jinwoo Song(jinwoo7.song@samsung.com) : https://bugs.webkit.org/show_bug.cgi?id=40163 */

/* Editor patches */
#define ENABLE_TIZEN_PARAGRAPH_SEPARATOR_FIX 1 /* Michal Pakula (m.pakula@samsung.com) : This is a quick fix for a bug where new paragraph was not created on contenteditable area with image and text nodes inside span tag */
#define ENABLE_TIZEN_CONTENT_EDITABLE_BACKSPACE 1 /* Wojciech Bielawski(w.bielawski.com) : This is a quick fix for a bug where after pressing backspace image was moved not correctly */

#define ENABLE_TIZEN_RELOAD_CACHE_POLICY_PATCH 1 /*Sungman Kim(ssungmai.kim@samsung.com) : Set cache policy of initialRequest to ReloadIgnoringCacheData from browser reload*/
#define ENABLE_TIZEN_ON_REDIRECTION_REQUESTED 1 /* Sungman Kim(ssungmai.kim@samsung.com) : Feature that offer the redirection header by callback set function */
#define ENABLE_TIZEN_CHECK_DID_PERFORM_FIRST_NAVIGATION 1 /* Sungman Kim(ssungmai.kim@samsung.com) : To skip sendSync message from WebProcess to UIProcess, doesn't use itemAtIndex() on BackForwardController */
#define ENABLE_TIZEN_CONSIDER_COOKIE_DISABLED_IF_SOUP_COOKIE_JAR_ACCEPT_NEVER_IS_SET 1 /* Raveendra Karu(r.karu@samsung.com) : Gets cookies enabled/disabled status from browser settings and returns the same to the Navigator Object */
#define ENABLE_TIZEN_CLEAR_MEMORY_CACHE_BUG_FIX 1 /* Keunyong Lee(ky07.lee@samsung.com) : Fix decoded data contolling problem after memory cache clearing */
#define ENABLE_TIZEN_LOAD_HTML_STRING_AS_UTF8 1 /* KwangYong Choi (ky0.choi@samsung.com) : Use UTF-8 instead of UTF-16 when the page is loaded by WebPage::loadHTMLString() */
#define ENABLE_TIZEN_POLICY_DECISION_HTTP_METHOD_GET 1/* Sungman Kim(ssungmai.kim@samsung.com) : Add function to get http method from navigation policy decision*/

#define ENABLE_TIZEN_WEBKIT2_PRE_RENDERING_WITH_DIRECTIVITY 1 /*JungJik Lee(jungjik.lee@samsung.com : Calculates cover-rect with trajectory vector scalar value to consider directivity. */
#define ENABLE_TIZEN_INJECTED_BUNDLE_EXPOSE 1 /* Ryuan Choi(ryuan.choi@samsung.com) : Expose Injectedbundle to application side */
#define ENABLE_TIZEN_SCALE_SET 1 /* Sanghyup Lee(sh53.lee@samsung.com) : Fix that offset of ewk_view_scale_set doesn't applied when fixed layout is used. */

#if USE(FREETYPE)
#define ENABLE_TIZEN_FT_EMBOLDEN_FOR_SYNTHETIC_BOLD 1 /*Younghwan Cho(yhwan.cho@samsung.com) : Use freetype's 'embolden' instead of drawing twice for synthetic bold*/
#endif
#define ENABLE_TIZEN_CHECK_SPACE_FOR_AVG_CHAR_WIDTH 1 /* Himanshu Joshi (h.joshi@samsung.com : Added check to consider Space as possible way to calculate Average Char Width if '0' and 'x' are not present*/
#define ENABLE_TIZEN_FIX_SHOULD_AA_LINES_CONDITION 1 /*Younghwan Cho(yhwan.cho@samsung.com) : Add some conditions for AA to draw box-lines */
#define ENABLE_TIZEN_USER_SETTING_FONT 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : If we don't find glyphs for characters, we can use SLP fallback font that is selected by user in Setting */
#define ENABLE_TIZEN_SET_PRIORITY_TO_USER_SETTING_LANGUAGE 1 /* Byeongha Cho(byeongha.cho@samsung.com) : Basically, Japanese font take priority over Chinese font in fontConfig. We should give priority to font that is selected by user in Setting */
#define ENABLE_TIZEN_FALLBACK_FONTDATA 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : Add the fallback fontData to FontFallbackList*/
#define ENABLE_TIZEN_NAVIGATOR_LANGUAGE_STR 1 /* Jaehun Lim(ljaehun.lim@samsung.com) : Modify return value from like a "ko-KR.UTF8" to "ko-KR" for "navigator.language". */
#define ENABLE_TIZEN_DBSPACE_PATH 1 /* Jaehun Lim(ljaehun.lim@samsung.com) : Set the TIZEN's default database directory */
#define ENABLE_TIZEN_REMOVE_IMG_SRC_ATTRIBUTE 1 /* Jaehun Lim(ljaehun.lim@samsung.com) : Don't paint when <img>'s src attribute is removed */
#define ENABLE_TIZEN_SEARCH_FIELD_STYLE 1 /* Jaehun Lim(ljaehun.lim@samsung.com) : making search fields style-able */
#define ENABLE_TIZEN_DOWNLOAD_LINK_FILTER 1 /* Jaehun Lim(ljaehun.lim@samsung.com) : show download context menus for only http(s) and ftp(s) link */
#define ENABLE_TIZEN_CACHE_RESOURCE_FILTER 1 /* Jaehun Lim(ljaehun.lim@samsung.com) : don't cache wrt's raw main resource file because its url will be modified */

#define ENABLE_TIZEN_DOM_TIMER_MIN_INTERVAL_SET 1 /* Gyuyoung Kim(gyuyoung.kim@samsung.com) : Change minimum interval for DOMTimer on Tizen */ 
#define ENABLE_TIZEN_CSS_FILTER_OPTIMIZATION 1 /* Hurnjoo Lee(hurnjoo.lee@samsung.com : Optimize performance of css filters */

#define ENABLE_TIZEN_SKIP_DRAWING_SHADOW_BLUR_SCRATCH_BUFFER 1 /* Yonggeol Jung(yg48.jung@samsung.com) */
#define ENABLE_TIZEN_PERFORMANCE_CANVAS_SCALED_SHADOW_BLUR 1 /* Yonggeol Jung(yg48.jung@samsung.com) : Performance improvement for canvas shadow blur. */
#define ENABLE_TIZEN_FIX_FOR_PIXMAN_SIZE_LIMITATION 1 /* Yonggeol Jung(yg48.jung@samsung.com) : Fixed for size limitation of pixman */
#define ENABLE_TIZEN_FIX_FIXED_LAYER_SHAKING_ISSUE 1 /* Yonggeol Jung(yg48.jung@samsung.com) */
#define ENABLE_TIZEN_FIX_MASK_LAYER_CRASH_ISSUE 1 /* Yonggeol Jung(yg48.jung@samsung.com) */
#define ENABLE_TIZEN_FIX_MASK_LAYER_NOT_SHOWING_ISSUE 1 /* Yonggeol Jung(yg48.jung@samsung.com) */
#define ENABLE_TIZEN_FIX_PRESERVE3D_LAYER_ORDER_ISSUE 1 /* Yonggeol Jung(yg48.jung@samsung.com) */
#define ENABLE_TIZEN_FIX_WORK_QUEUE_CRASH_ISSUE 1 /* Yonggeol Jung(yg48.jung@samsung.com), Joonghun Park(jh718.park@samsung.com) */
#define ENABLE_TIZEN_NOTIFY_FIRST_LAYOUT_COMPLETE 1 /* Yonggeol Jung(yg48.jung@samsung.com) : Notify the completion of first layout while loading the page. */
#define ENABLE_TIZEN_FIX_INVALID_DIRTY_AFTER_SCROLL_ISSUE 1 /* Yonggeol Jung(yg48.jung@samsung.com) */
#define ENABLE_TIZEN_WEBKIT2_FOCUS_RING 1 /* Yuni Jeong(yhnet.jung@samsung.com) : Focus ring implementation for WK2 */
#define ENABLE_TIZEN_WEBGL_SUPPORT 1 /* Yonggeol Jung(yg48.jung@samsung.com) */
#define ENABLE_TIZEN_MINIMIZE_AUDIO_STREAM_BUFFERING 1 /* Yonggeol Jung(yg48.jung@samsung.com) */
#define ENABLE_TIZEN_CANVAS_2D_LINE_JOIN 1 /* Rashmi Shyamasundar(rashmi.s2@samsung.com) Save the value of lineJoin in GraphicsContextState */
#define ENABLE_TIZEN_SESSION_REQUEST_CANCEL 1 /* Who is owner ? : */
#define ENABLE_TIZEN_ADD_AA_CONDITIONS_FOR_NINE_PATCH 1 /*Younghwan Cho(yhwan.cho@samsung.com) : Add conditions of antialias for fixing 9patch-problem */
#define ENABLE_TIZEN_WEBSOCKET_TLS_SUPPORT  1 /* Basavaraj P S : Handle TLS connection for secure websocket requests */
#define ENABLE_TIZEN_USE_FIXED_SCALE_ANIMATION 1 /* JungJik Lee(jungjik.lee@samsung.com) : use fixed scale if the layer is animating */
#define ENABLE_TIZEN_DRAW_SCALED_PATTERN 1 /* Kyungjin Kim(gen.kim@samsung.com) : Scale image prior to draw it's pattern to enhance performance */
#define ENABLE_TIZEN_PAINT_SELECTION_ANTIALIAS_NONE 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : Remove the line between the preceding text block and the next text block which are selected */
#define ENABLE_TIZEN_FIX_CLIP_PATTERN_RECT 1 /* Kyungjin Kim(gen.kim@samsung.com) : adjust clip rect to the min of pattern and clip for repeatX, repeatY */
#define ENABLE_TIZEN_NETWORK_INFO 1 /* Gyuyoung Kim(gyuyoung.kim@samsung.com) : Tizen local patch to support Tizen platform */
#if ENABLE(CONTEXT_MENUS)
#define ENABLE_TIZEN_CONTEXT_MENU 1 /* Gyuyoung Kim(gyuyoung.kim@samsung.com) : Support context menu for Tizen platform */
#define ENABLE_TIZEN_CONTEXT_MENU_TEMPORARY_FIX 1 /* Michal Pakula(m.pakula@samsung.com) : Temporary hack to prevent from crash when calling context menu on editable fiedld */
#define ENABLE_TIZEN_CONTEXT_MENU_SELECT 1 /* Michal Pakula(m.pakula@samsung.com) : Adds Select All and Select options to context menu */
#endif
#define ENABLE_TIZEN_FALLBACK_FONT_WEIGHT 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : Check fontDescription's weight to find the bold fallbackfont when font-weight is "bold" */

#define ENABLE_TIZEN_PREVENT_INCREMENTAL_IMAGE_RENDERING_WORKAROUND 1 /* Ryuan Choi(ryuan.choi@samsung.com), Byeongha Cho(byeongha.cho@samsung.com) : Prevent incremental image rendering. */
#define ENABLE_TIZEN_JPEG_IMAGE_SCALE_DECODING 1 /* Keunyong Lee(ky07.lee@samsung.com) : Scaled decoding Feature for Jpeg Image. Becuase this Feature replace Down Sampling, IMAGE_DECODER_DOWN_SAMPLING should be false when you want to use it. */
#define ENABLE_TIZEN_FIX_TIMEZONE_ISSUE 1 /* Hurnjoo Lee(hurnjoo.lee@samsung.com) : Local time information should be re-initialized after changing timezone. */
#if ENABLE(TIZEN_JPEG_IMAGE_SCALE_DECODING)
#define ENABLE_IMAGE_DECODER_DOWN_SAMPLING 0
#endif

#if ENABLE(WORKERS)
#define ENABLE_TIZEN_WORKERS 1 /* Jihye Kang(jye.kang@samsung.com) : Use allowUniversalAccessFromFileURLs setting for workers */
#endif

#define ENABLE_TIZEN_PAGE_VISIBILITY_API 1 /* Gyuyoung Kim(gyuyoung.kim@samsung.com) : Add ewk API to change page visibility */

/* Plugin Patches */
#define ENABLE_TIZEN_SUPPORT_PLUGINS 1 /* Mariusz Grzegorczyk(mariusz.g@samsung.com) : */

#if ENABLE(TOUCH_EVENTS)
#define ENABLE_TIZEN_TOUCH_EVENT_PERFORMANCE 1 /* Sanghyup Lee(sh53.lee@samsung.com) : Tizen Local patch to enhance touch event performance */
#define ENABLE_TIZEN_GESTURE_PERFORMANCE_WORKAROUND 1 /* Sanghyup Lee(sh53.lee@samsung.com) : Workaround patch to enhance pan and flick gesture performance */
#define ENABLE_TIZEN_UPDATE_DISPLAY_WITH_ANIMATOR 1 /* Eunmi Lee(eunmi15.lee): Update viewport using Ecore_Animator to prevent to request updating too much times during panning. */
#define ENABLE_TIZEN_USE_RAW_MOUSE_EVENT_FOR_HISTORY 1 /* Eunmi Lee(eunmi15.lee): Use raw mouse event for history to collect data without waiting result of touch events. */
#define ENABLE_TIZEN_FLICK_FASTER 1 /* Sanghyup Lee(sh53.lee@samsung.com) : Change easing equations in flick animation. */
#define ENABLE_TIZEN_GESTURE_WITH_PREVENT_DEFAULT 1 /* Sanghyup Lee(sh53.lee@samsung.com) : Support proper gesture behavior in web apps using preventDefault */
#endif
#define ENABLE_TIZEN_WEBKIT2_HIT_TEST 1 /* Yuni Jeong(yhnet.jung@samsung.com) : Hit Test API implementation for WK2 */
#define ENABLE_TIZEN_WEBKIT2_GET_TEXT_STYLE_FOR_SELECTION 0 /* Yuni Jeong(yhnet.jung@samsung.com), ryuan.choi@samsung.com : Patchs to get text style for supporting email requirement. Need to consider to find another way not to have many APIs */
#define ENABLE_TIZEN_TOUCH_CANCEL 1 /* Sanghyup Lee(sh53.lee) : To support touch cancel event. */
#if !ENABLE(TIZEN_TOUCH_CANCEL)
#define ENABLE_TIZEN_SUSPEND_TOUCH_CANCEL 1 /* Sanghyup Lee(sh53.lee) : Send touchCancel Event to clear :active style when Web App is suspended.*/
#endif

#define ENABLE_TIZEN_CACHE_MEMORY_OPTIMIZATION 1 /* Keunyong Lee(ky07.lee@samsung.com) : The Patches that can reduce memory usage */
/*
 * Extend standard npapi NPNVariable.
 * Replace XEvents with XTNPEvents for tizen platform needs.
 */
#define ENABLE_TIZEN_NPAPI 1 /* Mariusz Grzegorczyk(mariusz.g@samsung.com) : */
#if ENABLE(TIZEN_NPAPI)
#define ENABLE_TIZEN_PLUGIN_KEYBOARD_EVENT 1 /* KwangYong Choi(ky0.choi@samsung.com) : Support TNP soft key event */
#define ENABLE_TIZEN_PLUGIN_SUSPEND_RESUME 1 /* KwangYong Choi(ky0.choi@samsung.com) : Suspend/Resume notification to the plugin */
#define ENABLE_TIZEN_PLUGIN_TOUCH_EVENT ENABLE_TOUCH_EVENTS /* KwangYong Choi(ky0.choi@samsung.com) : Support TNP touch event */
#endif

#define ENABLE_TIZEN_BASE64 1 /* Sanghyun Park(sh919.park@samsung.com) : Fix the bugs about atob(Base64) */

/* JavaScriptCore */
#define ENABLE_TIZEN_USE_STACKPOINTER_AT_WEBPROCESS_STARTUP 1

#define ENABLE_TIZEN_BACKFORWARD_LIST_ERRORPAGE 1 /* Basavaraj P S(basavaraj.ps@samsung.com) */
#define ENABLE_TIZEN_ELEMENT_CREATED_BY_PARSER_INFO_STORE 1 /* Who is owner ? : support storing info about element's creation: from parser or JS */
#define ENABLE_TIZEN_HTTP_REQUEST_HEADER_APPEND 1 /* Sungman Kim(ssungmai.kim@samsung.com : Append request headers for  accept-charset. This is refered to other browser's header information. */
#define ENABLE_TIZEN_CSS_INSET_BORDER_BUG 1 /* Byeonga Cho(byeongha.cho@samsung.com : Fix CSS inset border bug in x86. This is workaround. FIX ME */
#define ENABLE_TIZEN_DFG_STORE_CELL_TAG_FOR_ARGUMENTS 1 /* Hojong Han(hojong.han@samsung.com) : Store cell tag for arguments */
#define ENABLE_TIZEN_DFG_UPDATE_TAG_FOR_REGEXP_TEST 1 /* Hojong Han(hojong.han@samsung.com) : Fix crash while using Naver map. Working on <https://bugs.webkit.org/show_bug.cgi?id=105756> */
#define ENABLE_TIZEN_SET_PROPERTY_ATTRIBUTE_CORRECTLY 1 /* Hojong Han(hojong.han@samsung.com) : Fix overwritting Read-only property, set from WRT */
#define ENABLE_TIZEN_DO_NOT_APPLY_SCROLLOFFSET_FOR_DELEGATESSCROLLING 1 /* Eunmi Lee(eunmi15.lee@samsung.com) : Fix the wrong position of hitTest result when we do hit test in the subFrame (It should be contributed to the opensource) */
#define ENABLE_TIZEN_FONT_HINT_NONE 1 /* Jaesick Chang(jaesik.chang@samsung.com) : */
/* Fix for continues layout when element having width given in percent is nested in flattened frame
   https://bugs.webkit.org/show_bug.cgi?id=54284
*/
#define ENABLE_TIZEN_ELEMENTS_NESTED_IN_FLATTENED_FRAME_FIX 1 /* Piotr Roguski(p.roguski@samsung.com) : */
#define ENABLE_TIZEN_GET_EXTERNAL_RESOURCES_IN_MHTML_FROM_NETWORK 1 /* Praveen(praveen.ks@samsung.com) : Allow external resources in MHTML file to be fetched from network rather than failing them */
#define ENABLE_TIZEN_MAIN_THREAD_SCHEDULE_DISCARD_DUPLICATE_REQUEST 1 /* Jihye Kang(jye.kang@samsung.com) : Fix lockup while doing stress test for filewriter */
#define ENABLE_TIZEN_META_CHARSET_PARSER 1 /* Kangil Han(kangil.han@samsung.com) : Parser fix for <http://www.onnuritour.com>. Working on <https://bugs.webkit.org/show_bug.cgi?id=102677>. */
#define ENABLE_TIZEN_MHTML_CSS_IMPORT_URL_RELATIVE_TO_PARENT 1 /*Santosh Mahto(santosh.ma@samsung.com) : URL of stylesheet specified in @import should be relative to parent Stylesheet in page serializing */
#define ENABLE_TIZEN_MHTML_CSS_MEDIA_RULE_RESOURCE 1 /* Santosh Mahto(santosh.ma@samsung.com) : Collect the subresource specified in css media rule in mhtml PageSerializing */
#define ENABLE_TIZEN_PAINT_MISSING_PLUGIN_RECT 1 /* KwangYong Choi(ky0.choi@samsung.com) : Paint missing plugin rect with gray */
#define ENABLE_TIZEN_PLUGIN_PREVENT_AUTOMATIC_FLASH_DOWNLOAD 1 /* KwangYong Choi(ky0.choi@samsung.com) : Prevent plugin auto-download */
#define ENABLE_TIZEN_PLUGIN_SCALE_FACTOR_FIX 1 /* KwangYong Choi(ky0.choi@samsung.com) : Use fixed device scale factor to fix drawing */
#define ENABLE_TIZEN_PLUGIN_SHOW_MISSING_PLUGIN_STATEMENT_EVEN_IF_PLUGIN_IS_DISABLED_ON_PREFERENCE 1 /* Keunsoon Lee(keunsoon.lee@samsung.com): show "missing plugin" even if preference setting for plugin is disabled. */
#define ENABLE_TIZEN_NOT_USE_TRANSFORM_INFO_WHEN_GETTING_CARET_RECT 1 /* Youngtaeck Song(youngtaeck.song@samsung.com) : Patch to do not use transform infomation when getting caret rect */
#define ENABLE_TIZEN_RESTORE_SCALE_FACTOR_AND_SCROLL_POSITION_BACKFORWARD 1 /* Gyuyoung Kim (gyuyoung.kim@samsung.com): Fix wrong scroll position after forwarding/backing - https://bugs.webkit.org/show_bug.cgi?id=126022 */
#define ENABLE_TIZEN_SOUP_HANDLE_STATUS_CODE_5XX_AFTER_RECEIVING_HTTP_BODY 1 /* Changhyup Jwa(ch.jwa@samsung.com) : handle status code 5xx after receiving HTTP body */
#define ENABLE_TIZEN_SOUP_NOT_ALLOW_BROWSE_LOCAL_DIRECTORY 1 /* Jongseok Yang(js45.yang@samsung.com) */
#define ENABLE_TIZEN_SOUP_AVOID_CERTIFICATION_POPUP_WORKAROUND 1 /* Jinwoo Jeong(jw00.jeong@samsung.com */
#define ENABLE_TIZEN_SOUP_DATA_DIRECTORY_UNDER_EACH_APP 1 /* Jinwoo Jeong(jw00.jeong@samsung.com) : Create a soup data directory under eache application */
#define ENABLE_TIZEN_TIMEOUT_FIX 1 /* Łukasz Ślachciak(l.slachciak@samsung.com) : fix for connection timeout */
#define ENABLE_TIZEN_WEBKIT2_FIX_INVAlID_SCROLL_FOR_NEW_PAGE 1 /* Jongseok Yang (js45.yang@samsung.com) : Patch to fix the invalid scroll position by open source patch */
#define ENABLE_TIZEN_WEBKIT2_PATCH_FOR_TC 1 /* Changhyup Jwa(ch.jwa@samsung.com) : Patchs to pass TC */
#define ENABLE_TIZEN_PREVENT_CRASH_OF_UI_SIDE_ANIMATION 1 /* Hurnjoo Lee(hurnjoo.lee@samsung.com) : Prevent crashes of UI side animation */
#define ENABLE_TIZEN_GENERIC_FONT_FAMILY 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : Tizen Generic font families */
#define ENABLE_TIZEN_APPLY_COMPLEX_TEXT_ELLIPSIS 1 /* Byeongha Cho(byeongha.cho@samsung.com : Apply ellipsis for complex text */
#if USE(ACCELERATED_COMPOSITING) && USE(TEXTURE_MAPPER)
#define ENABLE_TIZEN_UI_SIDE_ANIMATION_SYNC 1 /* Hyowon Kim(hw1008.kim@samsung.com) : keep last frame of an animation until the animated layer is removed */
#define ENABLE_TIZEN_LAYER_FLUSH_THROTTLING 0 /* JungJik Lee(jungjik.lee@samsung.com) : Give 0.5 sec delay to layer flush scheduler while loading the page */
#define ENABLE_TIZEN_SET_INITIAL_COLOR_OF_WEBVIEW_EVAS_IMAGE 1 /* Youngtaeck Song(youngtaeck.song@samsung.com) : Set initial color of webview evas image */
#endif
#define ENABLE_TIZEN_FIX_TCT_FOR_CANVAS 1 /* Tanvir Rizvi(tanvir.rizvi): Modified the code to fix the tct test of canvas. WebKit didn't make the exception of InvalidStateError */

#if ENABLE(TIZEN_USE_XPIXMAP_DECODED_IMAGESOURCE)
#define ENABLE_TIZEN_SHADOW_CONTEXT_RUNTIME_BACKEND 1 /* Shanmuga Pandi(shanmuga.m@samsung.com) : Shadow context backend should be based on destination context's backend */
#endif

#define ENABLE_TIZEN_TEXTURE_MAPPER_TEXTURE_POOL_MAKE_CONTEXT_CURRENT 1 /* Shanmuga Pandi(shanmuga.m@samsung.com) : TextureMapper texture pool does not set make context currently on shrink, and leads to OOM finally */

/* Debugging Patches */
#define ENABLE_TIZEN_DISPLAY_MESSAGE_TO_CONSOLE 1 /* Seokju Kwon(seokju.kwon@samsung.com) : Display console-messages for WebApps, Browser ... */

/* Network Patches*/
/* macro which solve network delay when some of contents was blocked for a while
 - example) m.daum.net, m.paran.com
 - When one of request was blocked, progress looks stopped at 50~60%(but almost was loaded) */
#define ENABLE_TIZEN_NETWORK_DELAYED_RESOURCE_SOLVE 1 /* Łukasz Ślachciak(l.slachciak@samsung.com) : */

/* change MAX_CONNECTIONS_PER_HOST value 6 -> 12 */
/* enhanced network loading speed */
/* apply tunning value */
#define ENABLE_TIZEN_SESSION_CONNECTION_PER_HOST_TUNNING 1 /* Who is owner ? : */

/* When displaying menu list using menu icon, a additional scrollbar is displayed in the screen center
   So, this patch is to remove the logic for a additional scrollbar */
#define ENABLE_TIZEN_FIX_DRAWING_ADDITIONAL_SCROLLBAR 1 /* Jongseok Yang(js45.yang@samsung.com) */
/* Problem : An additional scrollbar is displayed in the screen center when loading is finished as auto fitting.
   Solution : Remove the Widget::frameRectsChanged for EFL port. */
#define ENABLE_TIZEN_FIX_SCROLLBAR_FOR_AUTO_FITTING 1 /* Jongseok Yang(js45.yang@samsung.com) */

#define ENABLE_TIZEN_PREFERENCE 1 /* Eunmi Lee(eunmi15.lee@samsung.com) : TIZEN specific preferences */

#define ENABLE_TIZEN_WEBKIT2_TILED_AC_DONT_ADJUST_COVER_RECT 1 /* Eunsol Park(eunsol47.park@samsung.com) : Patch to do not adjust cover rect as fixed pixel size*/

#if USE(ACCELERATED_COMPOSITING)
#define ENABLE_TIZEN_FIX_DEPTH_TEST_BUG_OF_WEBGL 1 /* YongGeol Jung(yg48.jung@samsung.com) : Fix webgl bug related to depth-test */
#endif //USE(ACCELERATED_COMPOSITING)

#define ENABLE_TIZEN_FASTPATH_FOR_KNOWN_REGEXP 1 /*KyungTae Kim(ktf.kim@samsung.com) : Make a fast path for common regular expression patterns used in common js libraries*/

#define ENABLE_TIZEN_CACHE_CONTROL 1 /*Sungman Kim(ssungmai.kim@samsung.com) : Control cache enable or disable mode*/
#define ENABLE_TIZEN_CSP 1 /* Seonae Kim(sunaeluv.kim@samsung.com) : Support CSP to provide EWK API to web applicatoin */
#define ENABLE_TIZEN_LOAD_REMOTE_IMAGES 1 /* Dongjae Kim(dongjae1.kim@samsung.com) : for tizen remode image load setting */
#define ENABLE_TIZEN_WEBSOCKET_ADD_CANCELLABLE 1 /* Praveen (praveen.ks@samsung.com) : Add cancellable to input stream and cancel the cancellable when we close the platform */// 
#define ENABLE_TIZEN_REDUCE_KEY_LAGGING 1    /* Soon-Young Lee(sy5002.lee@samsung.com) : Temporary solution for a keylagging problem. FIXME */
#define ENABLE_TIZEN_USE_SETTINGS_FONT 1 /* Hyeonji Kim(hyeonji.kim@samsung.com) : When font-family is "Tizen", use system's setting font as default font-family */
#define ENABLE_TIZEN_WEBKIT2_SEPERATE_LOAD_PROGRESS 1 /* Yuni Jeong(yhnet.jung@samsung.com) : Patchs to seperate load progress callback for supporting OSP requirement  - "load,progress,started", "load,progress,finished" */
#define ENABLE_TIZEN_WEBKIT2_LOCAL_IMPLEMETATION_FOR_NAVIGATION_POLICY 1 /* Jongseok Yang(js45.yang@samsung.com) : temporary pathes to maintain local implementation for navigation policy operation */
#define ENABLE_TIZEN_WEBKIT2_LOCAL_IMPLEMENTATION_FOR_ERROR 1 /* Jongseok Yang(js45.yang@samsung.com) : temporary pathes to maintain local implementation for error operation */
#define ENABLE_TIZEN_WEBKIT2_TEXT_TRANSLATION 1 /* Bunam Jeon(bunam.jeon@samsung.com) : Support the text translation of webkit2 */
#define ENABLE_TIZEN_DOWNLOAD 1 /* Keunsoon Lee(keunsoon.lee@samsung.com) : Download related patches */
#define ENABLE_TIZEN_PASTEBOARD 1 /* Michal Pakula(m.pakula@samsung.com), Eunmi Lee(eunmi15.lee@samsung.com) : Pasteboard implementation for WK1 and WK2. */
#define ENABLE_TIZEN_JAVASCRIPT_AND_RESOURCE_SUSPEND_RESUME 1 /* Joonghun Park(jh718.park@samsung.com) : patches to suspend and resume JavaScript operation and resource loading */
#define ENABLE_TIZEN_WEBKIT2_IMF_CARET_FIX 1 /* Robert Plociennik (r.plociennik@samsung.com) : Caret fix for both WEB-4874 and WEB-5367. */
#define ENABLE_TIZEN_WEBKIT2_IMF_WITHOUT_INITIAL_CONTEXT 1 /* Jongseok Yang(js45.yang) : Remove the logic to create ecore imf context when creating webview */
#define ENABLE_TIZEN_WEBKIT2_IMF_REVEAL_SELECTION 1 /* Hoyong Jung(hoyong.jung@samsung.com) : Reveal selection when IMF input panel shown */
#define ENABLE_TIZEN_WEBKIT2_IMF_CONTEXT_INPUT_PANEL 1 /* Hoyong Jung(hoyong.jung@samsung.com) : bug fix for not showing the input panel  */
#define ENABLE_TIZEN_WEBKIT2_IMF_ADDITIONAL_CALLBACK 1 /* Sanghyup Lee(sh53.lee@samsung.com) : Support onIMFInputPanelSelectionSet, onIMFInputPanelDeleteSurrounding callback.  */
#define ENABLE_TIZEN_EDITING_PROHIBITED_CHILDREN_FIX 1 /* Robert Plociennik (r.plociennik@samsung.com) : Fix for moving prohibited child nodes during execution of editing commands (WEB-6188). */
#define ENABLE_TIZEN_ENCODING_VERIFICATION 1 /* Grzegorz Czajkowski (g.czajkowski@samsung.com) : new Ewk's API to check whether the encoding is supported by WebKit. This could be done while settings the encoding (in ewk_settings_default_encoding_set). However, the WRT Team prefers the separated API. */

#if USE(TIZEN_PLATFORM_SURFACE)
#if ENABLE(REQUEST_ANIMATION_FRAME)
#if USE(REQUEST_ANIMATION_FRAME_TIMER)
#define ENABLE_TIZEN_SYNC_REQUEST_ANIMATION_FRAME 1 /* YongGeol Jung(yg48.jung@samsung.com) : Synchronize callback function of ScriptedAnimationController between UI Process and Web Process to 1 : 1 */
#endif
#endif
#endif

#define ENABLE_TIZEN_REMOTE_INSPECTOR 1 /* Sanghyup Lee(sh53.lee@samsung.com) : Support Tizen Remote Inspector API */
#define ENABLE_TIZEN_PREVENT_CRASH_OF_APPLICATION_UNINSTALLATION 1 /* Sanghyup Lee(sh53.lee@samsung.com) : Fix crash issue of application uninstallation while inspector is running. */
#define ENABLE_TIZEN_WEBKIT2_NOTIFY_SUSPEND_BY_REMOTE_WEB_INSPECTOR 1 /* Bunam Jeon(bunam.jeon@samsung.com) : Notify to UI process that the content has been suspended by inspector */
#define ENABLE_TIZEN_OFFLINE_PAGE_SAVE 1 /* Nikhil Bansal (n.bansal@samsung.com) : Tizen feature for offline page save */

#if ENABLE(NOTIFICATIONS)
#define ENABLE_TIZEN_NOTIFICATIONS 1 /* Kihong Kwon(kihong.kwon@samsung.com) : Temp patch for notification score in the html5test.com */
#endif

#define ENABLE_TIZEN_APPLICATION_CACHE 1 /* Seonae Kim(sunaeluv.kim@samsung.com) : Support Application Cache */
#define ENABLE_TIZEN_WEB_STORAGE 1 /* Seonae Kim(sunaeluv.kim@samsung.com) : Support Web Storage */
#define ENABLE_TIZEN_FULLSCREEN_API 1 /* Jongseok Yang(js45.yang@samsung.com) : Implement the smart function for fullscreen API */

#define ENABLE_TIZEN_RUNLOOP_WAKEUP_ERROR_WORKAROUND 1 /* Seokju Kwon : Fix S1-6807 issue temporarily */
#define ENABLE_TIZEN_WRT_LAUNCHING_PERFORMANCE 1 /* Byungwoo Lee(bw80.lee@samsung.com) : Local patches to enhance web app launching performance */
#define ENABLE_TIZEN_WRT_APP_URI_SCHEME_EXTENSION 1 /* Jinwoo Jeong (jw00.jeong@samsung.com) : Local patches to support WRT data encryption scheme */
#define ENABLE_TIZEN_WRT_APP_CONTROL_SCHEME 1 /* Jinwoo Jeong (jw00.jeong@samsung.com) : The web application can have a custom scheme, and the scheme could be called via location on http header, we should support it.*/
#define ENABLE_TIZEN_WEBKIT2_NOTIFY_POPUP_REPLY_STATUS 1 /* Byungwoo Lee(bw80.lee@samsung.com) : Notify popup reply status through the smart callback */

#define ENABLE_TIZEN_DIRECT_RENDERING 1 /* Hyowon Kim(hw1008.kim@samsung.com) */
#define ENABLE_TIZEN_DONT_USE_TEXTURE_MAPPER_TEXTURE_POOL 0 /* Seojin Kim(seojin.kim@samsung.com) : TextureMapper texture pool has no shrink mechanism currently, and leads to OOM finally */

#define ENABLE_TIZEN_EXCLUDE_HEIGHT_FROM_MINIMUM_SCALE_CALC_WORKAROUND 1 /* Gyuyoung Kim(gyuyoung.kim@samsung.com) : Fix the issue that page is scaled when IME dissapear. It was caused by calculating height of viewport size / content size. */
#define ENABLE_TIZEN_SURROUNDING_TEXT_CALLBACK 1 /* Byeongha Cho(byeongha.cho@samsung.com) : Fix the issue that Cannot input Thai word that combined with upper and lower vowel. Support IMFRetrieveSurrounding callback*/

/* Debugging Patch */
#define ENABLE_TIZEN_LOG 1 /* Gyuyoung Kim(gyuyoung.kim@samsung.com) : */
#if ENABLE(TIZEN_LOG)
#define ERROR_DISABLED 0
#define LOG_DISABLED 0
#define ENABLE_TIZEN_DISPLAY_MESSAGE_TO_CONSOLE 1 /* Seokju Kwon(seokju.kwon@samsung.com) : Display console-messages for WebApps, Browser ... */
#define ENABLE_TIZEN_RENDERING_LOG 0 /* Hunseop Jeong(hs85.jeong@samsung.com) : Enable the log about TiledAC and Accelerated Compositing */
#endif

/* API extensions */
#define ENABLE_TIZEN_JS_EXT_API 1 /* Gyuyoung Kim(gyuyoung.kim@samsung.com): Add new APIs for JS */

#define ENABLE_TIZEN_SUPPORT_GESTURE_SMART_EVENTS 1 /* Eunmi Lee(eunmi15.lee): Support smart events for gesture. */
#define ENABLE_TIZEN_DISABLE_DEFAULT_PLUGIN_PATH 1 /* Ryuan Choi(ryuan.choi): Disable default plugin path */

#define ENABLE_TIZEN_CHOOSE_TO_USE_KEYPAD_OR_NOT_WITHOUT_USER_ACTION 1 /* Eunmi Lee(eunmi15.lee): Choose to use keypad or not without user action. Without this macro, keypad can be always shown without user action. Example of use is that Internet application can prevent to show keypad by javascript in the desktop naver.com. */

#define ENABLE_TIZEN_SET_FOCUS_TO_EWK_VIEW 1 /* Eunmi Lee(eunmi15.lee): The ewk_view(Evas_Object, not Elementary Widget) can not get focus correctly, so we have to set focus to ewk_view(Evas_Object) when ewk_view is touched. */

#define ENABLE_TIZEN_IMF_SETTING 1 /* Eunmi Lee(eunmi15.lee): Setting IMF for Tizen. */

#define ENABLE_TIZEN_ENABLE_PASSWORD_ECHO 1 /* Eunmi Lee(eunmi15.lee): Enable password echo as a default for Tizen. */

#define ENABLE_TIZEN_SCREEN_CAPTURE 1 /* Hyunsoo Shim(hyunsoo.shim): Add API that capturing images of screen for TIZEN. */

#if ENABLE(TIZEN_CIRCLE_DISPLAY)
#define ENABLE_TIZEN_SCROLLBAR 0 /* Łukasz Marek (l.marek@samsung.com), Michał Pakuła vel Rutka (m.pakula@samsung.com) : Patch enabling scrollbar in UIProcess */
#else
#define ENABLE_TIZEN_SCROLLBAR 1 /* Łukasz Marek (l.marek@samsung.com), Michał Pakuła vel Rutka (m.pakula@samsung.com) : Patch enabling scrollbar in UIProcess */
#endif

/* Patches for Tizen Micro */
#if ENABLE(TIZEN_WEARABLE_PROFILE)
#define ENABLE_TIZEN_MIRRORED_BLUR_EFFECT_SUPPORT 1 /* Tanvir Rizvi(tanvir.rizvi) Support for the Mirrored Blur Effect for the webapps of size 320 on WC1 */
#endif

#if ENABLE(TIZEN_WEARABLE_PROFILE)
#if ENABLE(IMAGE_DECODER_DOWN_SAMPLING) || ENABLE(TIZEN_JPEG_IMAGE_SCALE_DECODING)
#define ENABLE_TIZEN_MICRO_FORTIFIED_SCALED_DECODING 0 /* Byeongha Cho (byeongha.cho@samsung.com : Set strong threshold for scaled decoding and down sampling */
#endif

#if ENABLE(TOUCH_EVENTS)
#define ENABLE_TIZEN_CHANGE_HOVER_ACTIVE_STATE_ONLY_FOR_TOUCH_EVENTS 1 /* Eunmi Lee(eunmi15.lee): Change hover active state only for touch events. It is requested by WebUIFW to fix a twinkling issue at buttons with hover style. */
#if ENABLE(TIZEN_CHANGE_HOVER_ACTIVE_STATE_ONLY_FOR_TOUCH_EVENTS)
#define ENABLE_TIZEN_PROCESS_CLICK_ONLY_IF_NODE_IS_SAME_AS_TOUCHED_NODE 1 /* Eunmi Lee(eunmi15.lee): Process click event only if node is same as touched node. This macro should be enabled only if all touch events are sent to the web process. */
#endif
#define ENABLE_TIZEN_MULTI_TOUCH_TAP 1 /* Sanghyup Lee(sh53.lee@samsung.com) : Support single tap when we press multi points. */
#define ENABLE_TIZEN_SCROLL_SNAP 1 /* Gyuyoung Kim(gyuyoung.kim@samsung.com), Joonghun Park(jh718.park@samsung.com), Sanghyup Lee(sh53.lee@samsung.com) : Support css scroll snap */
#endif

#if ENABLE(TIZEN_CIRCLE_DISPLAY)
#define ENABLE_TIZEN_CIRCLE_DISPLAY_SCROLLBAR 1 /* Sanghyup Lee(sh53.lee@samsung.com) : Support overflow scrollbar in circle display device. */
#define ENABLE_TIZEN_CIRCLE_DISPLAY_SELECT 1 /* ByungJun Kim(bj1987.kim@samsung.com) : Support Select Popup List in circle display device. */
#define ENABLE_TIZEN_CIRCLE_DISPLAY_INPUT 1 /* Taeyun An(ty.an@samsung.com) : Support Input picker in circle display device. */
#define ENABLE_TIZEN_CIRCLE_DISPLAY_JS_POPUP 1 /* Gurpreet Kaur (k.gurpreet@samsung.com) : Support JS Popup in circle display device. */
#endif

#if ENABLE(TIZEN_IMPROVE_GESTURE_PERFORMANCE)
#define ENABLE_TIZEN_USE_ESTIMATED_POSITION_FOR_GESTURE 1 /* Eunmi Lee(eunmi15.lee): Get estimated current position using history. */
#define ENABLE_TIZEN_ENABLE_2_CPU 1 /* Eunmi Lee(eunmi15.lee): Enable 2 cpu. */
#endif

#define ENABLE_TIZEN_EDGE_EFFECT 1 /* Eunmi Lee(eunmi15.lee) : Support edge effect. */
#define ENABLE_TIZEN_CACHE_DISABLE 1 /* Jinwoo Jeong(jw00.jeong): Set cache disable*/
#if ENABLE(TIZEN_EMULATOR)
#define ENABLE_TIZEN_SUPPORT_IDLECLOCK_AC 0 /* Hunseop Jeong(hs85.jeong), Yonggeol Jung(yg48.jung@samsung.com) */
#else
#define ENABLE_TIZEN_SUPPORT_IDLECLOCK_AC 1 /* Hunseop Jeong(hs85.jeong), Yonggeol Jung(yg48.jung@samsung.com) */
#endif

/* Patches for Tizen TV */
#elif ENABLE(TIZEN_TV_PROFILE)
#define ENABLE_TIZEN_TV_ASM_READY 1 /* Marcin Konarski(m.konarski@partner.samsung.com) : Enable AudioSessionManager related code. */
#define ENABLE_TIZEN_ACCELERATED_ROTATE_TRANSFORM 1 /* Hurnjoo Lee(hurnjoo.lee@samsung.com) : Accelerated rotate transform */
#define ENABLE_TIZEN_HTTP_HEADER_CHECKING 1 /* Maciej Florek(m.florek@samsung.com) : Check for multiplied headers being passed from SOUP library to Webkit ResponseRequest */
#define ENABLE_TIZEN_VD_WHEEL_FLICK 1 /* Sanghyup Lee(sh53.lee@samsung.com) : Implement wheel flick feature on remote control */
#define ENABLE_TIZEN_SPATIAL_NAVIGATION 1 /* Sangyong Park(sy302.park@samsung.com), Sanghyup Lee(sh53.lee@samsung.com) : Support spatial navigation. */
#define ENABLE_TIZEN_SUPPORT_IDLECLOCK_AC 0 /* Hunseop Jeong(hs85.jeong), Yonggeol Jung(yg48.jung@samsung.com) */
#define ENABLE_TIZEN_PAGE_ZOOM_ON_FIXED_LAYOUT 1 /* Gyuyoung Kim(gyuyoung.kim@samsung.com) : page zoom doesn't work correctly when fixed layout is enabled. */
#define ENABLE_TIZEN_CONSTRUCT_TAG_NODE_LIST_WITH_SNAPSHOT 1 /* Dong-Gwan Kim (donggwan.kim@samsung.com) : create snapshot with TagNodeList in order to speed up Browsermark2 DOM Search benchmark */

/* Patches for Tizen Mobile */
#elif ENABLE(TIZEN_MOBILE_PROFILE)
#define ENABLE_TIZEN_MOBILE_WEB_PRINT 1 /* Hyunsoo Shim(hyunsoo.shim@samsung.com) : Mobile Web Print for AC layer */
#define ENABLE_TIZEN_WEBKIT2_SAME_PAGE_GROUP_FOR_CREATE_WINDOW_OPERATION 1 /* Jongseok Yang (js45.yang@samsung.com), Ryuan Choi (ryuan.choi) : The page from create window operation has same page group as the caller page. This is for OSP */
#define ENABLE_TIZEN_WEBKIT2_SUPPORT_JAPANESE_IME 1 /* Jongseok Yang(js45.yang@samsung.com), Karthik Gopalan(karthikg.g@samsung.com), Vaibhav Sharma(vb.sharma@samsung.com) : Support Japanese IME */
#define ENABLE_TIZEN_BROWSER_DASH_MODE 0 /* Kyungjin Kim (gen.kim@samsung.com) : Enable dash mode interface */
#define ENABLE_TIZEN_CONSTRUCT_TAG_NODE_LIST_WITH_SNAPSHOT 1 /* Dong-Gwan Kim (donggwan.kim@samsung.com) : create snapshot with TagNodeList in order to speed up Browsermark2 DOM Search benchmark */

#if ENABLE(TIZEN_IMPROVE_GESTURE_PERFORMANCE)
#define ENABLE_TIZEN_USE_ESTIMATED_POSITION_FOR_GESTURE 1 /* Eunmi Lee(eunmi15.lee): Get estimated current position using history. */
#define ENABLE_TIZEN_WAKEUP_GPU 1 /* Eunmi Lee(eunmi15.lee): Wake up GPU. */
#define ENABLE_TIZEN_ENABLE_2_CPU 1 /* Eunmi Lee(eunmi15.lee): Enable 2 CPU. */
#endif

#endif /* The end of profile macros */

#define ENABLE_TIZEN_FIX_VISIBLE_CONTENTS_RECT_PROBLEMS 0 /* Eunmi Lee(eunmi15.lee): Fix visibleContentsRect problems and it should be uploaded to opensource webkit. */
#define ENABLE_TIZEN_PAGE_SUSPEND_RESUME 1 /* Hunseop Jeong(hs85.jeong): Add function that suspending(resuming) the page. */
#define ENABLE_TIZEN_SCROLL_SCROLLABLE_AREA 1 /* Eunmi Lee(eunmi15.lee): Scroll scrollable area - sub frame, overflow node, overflow input text, etc. */
#if ENABLE(TIZEN_SCROLL_SCROLLABLE_AREA)
#define ENABLE_TIZEN_OVERFLOW_SCROLLBAR 1 /* Jinwoo Jeong(jw00.jeong) : Paint scrollbars for overflow areas. */
#if ENABLE(ACCELERATED_OVERFLOW_SCROLLING) && ENABLE(TIZEN_OVERFLOW_SCROLLBAR)
#define ENABLE_TIZEN_UI_SIDE_OVERFLOW_SCROLLING 0 /* Hyowon Kim(hw1008.kim) */
#endif
#endif
#define ENABLE_TIZEN_IMPROVE_ZOOM_PERFORMANCE 1 /* Hunseop Jeong(hs85.jeong): Improve zoom performance. When using pinch gesture, suspened the view until zoom is over. */

#define ENABLE_TIZEN_OPTIMIZE_CONTENT_WIDTH_CALCULATION 1 /* Jinwoo Song(jinwoo7.song@samsung.com) : Optimize performance of content width calculation. */

#define ENABLE_TIZEN_JSC_REGEXP_TEST_CACHE 1 /* Hojong Han(hojong.han@samsung.com) : Cache results of regexp test */
#define ENABLE_TIZEN_JSC_DONT_USE_JSSTRINGWITHCACHE 1 /* SangGyu Lee(sg5.lee@samsung.com) : Turn off jsStringWithCache */

#define ENABLE_TIZEN_HANDLE_MOUSE_WHEEL_IN_THE_UI_SIDE 1 /* Eunmi Lee(eunmi15.lee) : Handle mouse wheel in the ui side for performance */

#define ENABLE_TIZEN_WEBKIT2_CONTEXT_X_WINDOW 1 /* Changhyup Jwa(ch.jwa@samsung.com) : WebProcess cannot access to evas, it needs to obtain window id in WebProcess*/

#if ENABLE(TIZEN_GSTREAMER_VIDEO) && !ENABLE(TIZEN_EMULATOR)
#define ENABLE_TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN 1 /* Eojin ham(eojin.ham@samsung.com) : Use HW video overlay when play video in fullscreen. */
#if ENABLE(TIZEN_USE_HW_VIDEO_OVERLAY_IN_FULLSCREEN)
#define ENABLE_TIZEN_SKIP_COMPOSITING_FOR_FULL_SCREEN_VIDEO 1 /* YongGeol Jung(yg48.jung@samsung.com) : Composite full screen layer only for native full screen video play-back */
#endif
#endif

#define ENABLE_TIZEN_FONT_CONFIG_CHECK 1 /* Michal Pakula (m.pakula@samsung.com): After a patch in Evas: http://slp-info.sec.samsung.net/gerrit/#/c/414511/, WebKitTestRunner is removing application fonts from font config, which causes crashes.
This patch reinitializes font config if fonts were removed. It should be removed after Evas will be fixed.*/

#define ENABLE_TIZEN_MYANMAR_FONT_SUPPORT 1 /* Divakar (divakar.a@samsung.com) : For supporting Myanmar font when locale is my-ZG */

#define ENABLE_TIZEN_CLEAR_BACKINGSTORE 1 /* Hunseop Jeong(hs85.jeong) : Clear the backingStore when screen isn't visible. */

#define ENABLE_TIZEN_SUPPORT_SCROLLING_APIS 1 /* Eunmi Lee(eunmi15.lee) : Support scrolling APIs which are provided in the Tizen public opensource */

// FIXME : This macro will be merged to TIZEN_RUNTIME_BACKEND_SELECTION after TIZEN_PLATFORM_SURFACE was turned on at EMULATOR.
#define ENABLE_TIZEN_FORCE_CREATION_TEXTUREMAPPER 1 /* Hyowon Kim(hw1008.kim@samsung.com) : Force early creation of TextureMapper */
#define ENABLE_TIZEN_CHECK_MODAL_POPUP 1 /* Hunseop Jeong(hs85.jeong) : Check if modal popup is shown. Because we need to suspend UIProcess behavior when a foreground application with a modal popup hides background. */
#define ENABLE_TIZEN_WEBKIT2_LANGUAGE_OPERATION 1   /* Yuni Jeong(yhnet.jung@samsung.com) : To support "Accept-Language" and navigator.language */

#endif /* PLATFORM(TIZEN) */

/* ==== OS() - underlying operating system; only to be used for mandated low-level services like 
   virtual memory, not to choose a GUI toolkit ==== */

/* OS(AIX) - AIX */
#ifdef _AIX
#define WTF_OS_AIX 1
#endif

/* OS(DARWIN) - Any Darwin-based OS, including Mac OS X and iPhone OS */
#ifdef __APPLE__
#define WTF_OS_DARWIN 1

#include <Availability.h>
#include <AvailabilityMacros.h>
#include <TargetConditionals.h>
#endif

/* OS(IOS) - iOS */
/* OS(MAC_OS_X) - Mac OS X (not including iOS) */
#if OS(DARWIN) && ((defined(TARGET_OS_EMBEDDED) && TARGET_OS_EMBEDDED) \
    || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)                 \
    || (defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR))
#define WTF_OS_IOS 1
#elif OS(DARWIN) && defined(TARGET_OS_MAC) && TARGET_OS_MAC
#define WTF_OS_MAC_OS_X 1

/* FIXME: These can be removed after sufficient time has passed since the removal of BUILDING_ON / TARGETING macros. */

#define ERROR_PLEASE_COMPARE_WITH_MAC_OS_X_VERSION_MIN_REQUIRED 0 / 0
#define ERROR_PLEASE_COMPARE_WITH_MAC_OS_X_VERSION_MAX_ALLOWED 0 / 0

#define BUILDING_ON_LEOPARD ERROR_PLEASE_COMPARE_WITH_MAC_OS_X_VERSION_MIN_REQUIRED
#define BUILDING_ON_SNOW_LEOPARD ERROR_PLEASE_COMPARE_WITH_MAC_OS_X_VERSION_MIN_REQUIRED
#define BUILDING_ON_LION ERROR_PLEASE_COMPARE_WITH_MAC_OS_X_VERSION_MIN_REQUIRED

#define TARGETING_LEOPARD ERROR_PLEASE_COMPARE_WITH_MAC_OS_X_VERSION_MAX_ALLOWED
#define TARGETING_SNOW_LEOPARD ERROR_PLEASE_COMPARE_WITH_MAC_OS_X_VERSION_MAX_ALLOWED
#define TARGETING_LION ERROR_PLEASE_COMPARE_WITH_MAC_OS_X_VERSION_MAX_ALLOWED
#endif

/* OS(FREEBSD) - FreeBSD */
#if defined(__FreeBSD__) || defined(__DragonFly__) || defined(__FreeBSD_kernel__)
#define WTF_OS_FREEBSD 1
#endif

/* OS(HURD) - GNU/Hurd */
#ifdef __GNU__
#define WTF_OS_HURD 1
#endif

/* OS(LINUX) - Linux */
#ifdef __linux__
#define WTF_OS_LINUX 1
#endif

/* OS(NETBSD) - NetBSD */
#if defined(__NetBSD__)
#define WTF_OS_NETBSD 1
#endif

/* OS(OPENBSD) - OpenBSD */
#ifdef __OpenBSD__
#define WTF_OS_OPENBSD 1
#endif

/* OS(QNX) - QNX */
#if defined(__QNXNTO__)
#define WTF_OS_QNX 1
#endif

/* OS(SOLARIS) - Solaris */
#if defined(sun) || defined(__sun)
#define WTF_OS_SOLARIS 1
#endif

/* OS(WINCE) - Windows CE; note that for this platform OS(WINDOWS) is also defined */
#if defined(_WIN32_WCE)
#define WTF_OS_WINCE 1
#endif

/* OS(WINDOWS) - Any version of Windows */
#if defined(WIN32) || defined(_WIN32)
#define WTF_OS_WINDOWS 1
#endif

#define WTF_OS_WIN ERROR "USE WINDOWS WITH OS NOT WIN"
#define WTF_OS_MAC ERROR "USE MAC_OS_X WITH OS NOT MAC"

/* OS(UNIX) - Any Unix-like system */
#if    OS(AIX)              \
    || OS(DARWIN)           \
    || OS(FREEBSD)          \
    || OS(HURD)             \
    || OS(LINUX)            \
    || OS(NETBSD)           \
    || OS(OPENBSD)          \
    || OS(QNX)              \
    || OS(SOLARIS)          \
    || defined(unix)        \
    || defined(__unix)      \
    || defined(__unix__)
#define WTF_OS_UNIX 1
#endif

/* Operating environments */

/* FIXME: these are all mixes of OS, operating environment and policy choices. */
/* PLATFORM(QT) */
/* PLATFORM(EFL) */
/* PLATFORM(GTK) */
/* PLATFORM(BLACKBERRY) */
/* PLATFORM(MAC) */
/* PLATFORM(WIN) */
#if defined(BUILDING_QT__)
#define WTF_PLATFORM_QT 1
#elif defined(BUILDING_EFL__)
#define WTF_PLATFORM_EFL 1
#elif defined(BUILDING_GTK__)
#define WTF_PLATFORM_GTK 1
#elif defined(BUILDING_BLACKBERRY__)
#define WTF_PLATFORM_BLACKBERRY 1
#elif OS(DARWIN)
#define WTF_PLATFORM_MAC 1
#elif OS(WINDOWS)
#define WTF_PLATFORM_WIN 1
#endif

/* PLATFORM(IOS) */
/* FIXME: this is sometimes used as an OS switch and sometimes for higher-level things */
#if (defined(TARGET_OS_EMBEDDED) && TARGET_OS_EMBEDDED) || (defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
#define WTF_PLATFORM_IOS 1
#endif

/* PLATFORM(IOS_SIMULATOR) */
#if defined(TARGET_IPHONE_SIMULATOR) && TARGET_IPHONE_SIMULATOR
#define WTF_PLATFORM_IOS 1
#define WTF_PLATFORM_IOS_SIMULATOR 1
#endif

/* Graphics engines */

/* USE(CG) and PLATFORM(CI) */
#if PLATFORM(MAC) || PLATFORM(IOS) || (PLATFORM(WIN) && !USE(WINGDI) && !PLATFORM(WIN_CAIRO))
#define WTF_USE_CG 1
#endif
#if PLATFORM(MAC) || PLATFORM(IOS) || (PLATFORM(WIN) && USE(CG))
#define WTF_USE_CA 1
#endif

#if PLATFORM(BLACKBERRY)
#define WTF_USE_LOW_QUALITY_IMAGE_INTERPOLATION 1
#define WTF_USE_LOW_QUALITY_IMAGE_NO_JPEG_DITHERING 1
#define WTF_USE_LOW_QUALITY_IMAGE_NO_JPEG_FANCY_UPSAMPLING 1
#endif

#if PLATFORM(GTK)
#define WTF_USE_CAIRO 1
#define WTF_USE_GLIB 1
#define WTF_USE_FREETYPE 1
#define WTF_USE_HARFBUZZ 1
#define WTF_USE_SOUP 1
#define WTF_USE_WEBP 1
#define ENABLE_GLOBAL_FASTMALLOC_NEW 0
#define GST_API_VERSION_1 1
#endif

/* On Windows, use QueryPerformanceCounter by default */
#if OS(WINDOWS)
#define WTF_USE_QUERY_PERFORMANCE_COUNTER  1
#endif

#if OS(WINCE) && !PLATFORM(QT)
#define NOSHLWAPI      /* shlwapi.h not available on WinCe */

/* MSDN documentation says these functions are provided with uspce.lib.  But we cannot find this file. */
#define __usp10__      /* disable "usp10.h" */

#define _INC_ASSERT    /* disable "assert.h" */
#define assert(x)

#endif  /* OS(WINCE) && !PLATFORM(QT) */

#if !USE(WCHAR_UNICODE)
#define WTF_USE_ICU_UNICODE 1
#endif

#if PLATFORM(MAC) && !PLATFORM(IOS)
#if CPU(X86_64)
#define WTF_USE_PLUGIN_HOST_PROCESS 1
#endif
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
#define WTF_USE_SCROLLBAR_PAINTER 1
#define HAVE_XPC 1
#endif
#define WTF_USE_CF 1
#define HAVE_READLINE 1
#define HAVE_RUNLOOP_TIMER 1
#if __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
#define HAVE_LAYER_HOSTING_IN_WINDOW_SERVER 1
#endif
#define WTF_USE_APPKIT 1
#define WTF_USE_SECURITY_FRAMEWORK 1
#endif /* PLATFORM(MAC) && !PLATFORM(IOS) */

#if PLATFORM(IOS)
#define DONT_FINALIZE_ON_MAIN_THREAD 1
#endif

#if PLATFORM(QT) && OS(DARWIN)
#define WTF_USE_CF 1
#endif

#if OS(DARWIN) && !PLATFORM(GTK) && !PLATFORM(QT)
#define ENABLE_PURGEABLE_MEMORY 1
#endif

#if PLATFORM(IOS)
#define HAVE_READLINE 1
#define WTF_USE_APPKIT 0
#define WTF_USE_CF 1
#define WTF_USE_CFNETWORK 1
#define WTF_USE_NETWORK_CFDATA_ARRAY_CALLBACK 1
#define WTF_USE_SECURITY_FRAMEWORK 0
#define WTF_USE_WEB_THREAD 1
#endif /* PLATFORM(IOS) */

#if PLATFORM(WIN) && !USE(WINGDI)
#define WTF_USE_CF 1
#endif

#if PLATFORM(WIN) && !USE(WINGDI) && !PLATFORM(WIN_CAIRO)
#define WTF_USE_CFNETWORK 1
#endif

#if USE(CFNETWORK) || PLATFORM(MAC) || PLATFORM(IOS)
#define WTF_USE_CFURLCACHE 1
#endif

#if !defined(HAVE_ACCESSIBILITY)
#if PLATFORM(IOS) || PLATFORM(MAC) || PLATFORM(WIN) || PLATFORM(GTK) || PLATFORM(EFL)
#define HAVE_ACCESSIBILITY 1
#endif
#endif /* !defined(HAVE_ACCESSIBILITY) */

#if OS(UNIX)
#define HAVE_ERRNO_H 1
#define HAVE_MMAP 1   
#define HAVE_SIGNAL_H 1
#define HAVE_STRINGS_H 1
#define HAVE_SYS_PARAM_H 1
#define HAVE_SYS_TIME_H 1 
#define WTF_USE_PTHREADS 1
#endif /* OS(UNIX) */

#if OS(UNIX) && !OS(QNX)
#define HAVE_LANGINFO_H 1
#endif

#if (OS(FREEBSD) || OS(OPENBSD)) && !defined(__GLIBC__)
#define HAVE_PTHREAD_NP_H 1
#endif

#if !defined(HAVE_VASPRINTF)
#if !COMPILER(MSVC) && !COMPILER(RVCT) && !COMPILER(MINGW) && !(COMPILER(GCC) && OS(QNX))
#define HAVE_VASPRINTF 1
#endif
#endif

#if !defined(HAVE_STRNSTR)
#if OS(DARWIN) || (OS(FREEBSD) && !defined(__GLIBC__))
#define HAVE_STRNSTR 1
#endif
#endif

#if !OS(WINDOWS) && !OS(SOLARIS)
#define HAVE_TM_GMTOFF 1
#define HAVE_TM_ZONE 1
#define HAVE_TIMEGM 1
#endif

#if OS(DARWIN)

#define HAVE_DISPATCH_H 1
#define HAVE_MADV_FREE 1
#define HAVE_MADV_FREE_REUSE 1
#define HAVE_MERGESORT 1
#define HAVE_PTHREAD_SETNAME_NP 1
#define HAVE_SYS_TIMEB_H 1
#define WTF_USE_ACCELERATE 1

#if !PLATFORM(IOS)
#define HAVE_HOSTED_CORE_ANIMATION 1
#endif /* !PLATFORM(IOS) */

#endif /* OS(DARWIN) */

#if OS(WINDOWS) && !OS(WINCE)
#define HAVE_SYS_TIMEB_H 1
#define HAVE_ALIGNED_MALLOC 1
#define HAVE_ISDEBUGGERPRESENT 1

#if !PLATFORM(QT)
#include <WTF/WTFHeaderDetection.h>
#endif

#endif

#if OS(WINDOWS)
#define HAVE_VIRTUALALLOC 1
#endif

#if OS(QNX)
#define HAVE_MADV_FREE_REUSE 1
#define HAVE_MADV_FREE 1
#endif

/* ENABLE macro defaults */

/* FIXME: move out all ENABLE() defines from here to FeatureDefines.h */

/* Include feature macros */
#include <wtf/FeatureDefines.h>

#if PLATFORM(QT)
/* We must not customize the global operator new and delete for the Qt port. */
#define ENABLE_GLOBAL_FASTMALLOC_NEW 0
#if !OS(UNIX)
#define USE_SYSTEM_MALLOC 1
#endif
#endif

#if PLATFORM(EFL)
#define ENABLE_GLOBAL_FASTMALLOC_NEW 0
#endif

#if OS(WINDOWS)
#define ENABLE_GLOBAL_FASTMALLOC_NEW 0
#endif

#if !defined(ENABLE_GLOBAL_FASTMALLOC_NEW)
#define ENABLE_GLOBAL_FASTMALLOC_NEW 1
#endif

#define ENABLE_DEBUG_WITH_BREAKPOINT 0
#define ENABLE_SAMPLING_COUNTERS 0
#define ENABLE_SAMPLING_FLAGS 0
#define ENABLE_SAMPLING_REGIONS 0
#define ENABLE_OPCODE_SAMPLING 0
#define ENABLE_CODEBLOCK_SAMPLING 0
#if ENABLE(CODEBLOCK_SAMPLING) && !ENABLE(OPCODE_SAMPLING)
#error "CODEBLOCK_SAMPLING requires OPCODE_SAMPLING"
#endif
#if ENABLE(OPCODE_SAMPLING) || ENABLE(SAMPLING_FLAGS) || ENABLE(SAMPLING_REGIONS)
#define ENABLE_SAMPLING_THREAD 1
#endif

#if !defined(WTF_USE_JSVALUE64) && !defined(WTF_USE_JSVALUE32_64)
#if (CPU(X86_64) && (OS(UNIX) || OS(WINDOWS))) \
    || (CPU(IA64) && !CPU(IA64_32)) \
    || CPU(ALPHA) \
    || CPU(SPARC64) \
    || CPU(S390X) \
    || CPU(PPC64)
#define WTF_USE_JSVALUE64 1
#else
#define WTF_USE_JSVALUE32_64 1
#endif
#endif /* !defined(WTF_USE_JSVALUE64) && !defined(WTF_USE_JSVALUE32_64) */

/* Disable the JIT on versions of GCC prior to 4.1 */
#if !defined(ENABLE_JIT) && COMPILER(GCC) && !GCC_VERSION_AT_LEAST(4, 1, 0)
#define ENABLE_JIT 0
#endif

#if !defined(ENABLE_JIT) && CPU(SH4) && PLATFORM(QT)
#define ENABLE_JIT 1
#endif

/* The JIT is enabled by default on all x86, x86-64, ARM & MIPS platforms. */
#if !defined(ENABLE_JIT) \
    && (CPU(X86) || CPU(X86_64) || CPU(ARM) || CPU(MIPS)) \
    && (OS(DARWIN) || !COMPILER(GCC) || GCC_VERSION_AT_LEAST(4, 1, 0)) \
    && !OS(WINCE) \
    && !(OS(QNX) && !PLATFORM(QT)) /* We use JIT in QNX Qt */
#define ENABLE_JIT 1
#endif

/* If possible, try to enable a disassembler. This is optional. We proceed in two
   steps: first we try to find some disassembler that we can use, and then we
   decide if the high-level disassembler API can be enabled. */
#if !defined(WTF_USE_UDIS86) && ENABLE(JIT) && (PLATFORM(MAC) || (PLATFORM(QT) && OS(LINUX))) \
    && (CPU(X86) || CPU(X86_64))
#define WTF_USE_UDIS86 1
#endif

#if !defined(WTF_USE_ARMV7_DISASSEMBLER) && ENABLE(JIT) && PLATFORM(IOS) && CPU(ARM_THUMB2)
#define WTF_USE_ARMV7_DISASSEMBLER 1
#endif

#if !defined(ENABLE_DISASSEMBLER) && (USE(UDIS86) || USE(ARMV7_DISASSEMBLER))
#define ENABLE_DISASSEMBLER 1
#endif

/* On some of the platforms where we have a JIT, we want to also have the 
   low-level interpreter. */
#if !defined(ENABLE_LLINT) \
    && ENABLE(JIT) \
    && (OS(DARWIN) || OS(LINUX)) \
    && (PLATFORM(MAC) || PLATFORM(IOS) || PLATFORM(GTK) || PLATFORM(QT)) \
    && (CPU(X86) || CPU(X86_64) || CPU(ARM_THUMB2) || CPU(ARM_TRADITIONAL) || CPU(MIPS) || CPU(SH4))
#define ENABLE_LLINT 1
#endif

#if !defined(ENABLE_DFG_JIT) && ENABLE(JIT) && !COMPILER(MSVC)
/* Enable the DFG JIT on X86 and X86_64.  Only tested on Mac and GNU/Linux. */
#if (CPU(X86) || CPU(X86_64)) && (OS(DARWIN) || OS(LINUX))
#define ENABLE_DFG_JIT 1
#endif
/* Enable the DFG JIT on ARMv7.  Only tested on iOS and Qt Linux. */
#if CPU(ARM_THUMB2) && (PLATFORM(IOS) || PLATFORM(BLACKBERRY) || PLATFORM(QT) || PLATFORM(TIZEN))
#define ENABLE_DFG_JIT 1
#endif
/* Enable the DFG JIT on ARM. */
#if CPU(ARM_TRADITIONAL)
#define ENABLE_DFG_JIT 1
#endif
/* Enable the DFG JIT on MIPS. */
#if CPU(MIPS)
#define ENABLE_DFG_JIT 1
#endif
#endif

/* If the jit is not available, enable the LLInt C Loop: */
#if !ENABLE(JIT)
#undef ENABLE_LLINT        /* Undef so that we can redefine it. */
#undef ENABLE_LLINT_C_LOOP /* Undef so that we can redefine it. */
#undef ENABLE_DFG_JIT      /* Undef so that we can redefine it. */
#define ENABLE_LLINT 1
#define ENABLE_LLINT_C_LOOP 1
#define ENABLE_DFG_JIT 0
#endif

/* Do a sanity check to make sure that we at least have one execution engine in
   use: */
#if !(ENABLE(JIT) || ENABLE(LLINT))
#error You have to have at least one execution model enabled to build JSC
#endif

/* Profiling of types and values used by JIT code. DFG_JIT depends on it, but you
   can enable it manually with DFG turned off if you want to use it as a standalone
   profiler. In that case, you probably want to also enable VERBOSE_VALUE_PROFILE
   below. */
#if !defined(ENABLE_VALUE_PROFILER) && ENABLE(DFG_JIT)
#define ENABLE_VALUE_PROFILER 1
#endif

#if !defined(ENABLE_VERBOSE_VALUE_PROFILE) && ENABLE(VALUE_PROFILER)
#define ENABLE_VERBOSE_VALUE_PROFILE 0
#endif

#if !defined(ENABLE_SIMPLE_HEAP_PROFILING)
#define ENABLE_SIMPLE_HEAP_PROFILING 0
#endif

/* Counts uses of write barriers using sampling counters. Be sure to also
   set ENABLE_SAMPLING_COUNTERS to 1. */
#if !defined(ENABLE_WRITE_BARRIER_PROFILING)
#define ENABLE_WRITE_BARRIER_PROFILING 0
#endif

/* Enable verification that that register allocations are not made within generated control flow.
   Turned on for debug builds. */
#if !defined(ENABLE_DFG_REGISTER_ALLOCATION_VALIDATION) && ENABLE(DFG_JIT)
#if !defined(NDEBUG)
#define ENABLE_DFG_REGISTER_ALLOCATION_VALIDATION 1
#else
#define ENABLE_DFG_REGISTER_ALLOCATION_VALIDATION 0
#endif
#endif

/* Configure the JIT */
#if CPU(X86) && COMPILER(MSVC)
#define JSC_HOST_CALL __fastcall
#elif CPU(X86) && COMPILER(GCC)
#define JSC_HOST_CALL __attribute__ ((fastcall))
#else
#define JSC_HOST_CALL
#endif

/* Configure the interpreter */
#if COMPILER(GCC) || (COMPILER(RVCT) && defined(__GNUC__))
#define HAVE_COMPUTED_GOTO 1
#endif

/* Determine if we need to enable Computed Goto Opcodes or not: */
#if HAVE(COMPUTED_GOTO) && ENABLE(LLINT)
#define ENABLE_COMPUTED_GOTO_OPCODES 1
#endif

/* Regular Expression Tracing - Set to 1 to trace RegExp's in jsc.  Results dumped at exit */
#define ENABLE_REGEXP_TRACING 0

/* Yet Another Regex Runtime - turned on by default for JIT enabled ports. */
#if !defined(ENABLE_YARR_JIT) && (ENABLE(JIT) || ENABLE(LLINT_C_LOOP)) && !(OS(QNX) && PLATFORM(QT))
#define ENABLE_YARR_JIT 1

/* Setting this flag compares JIT results with interpreter results. */
#define ENABLE_YARR_JIT_DEBUG 0
#endif

/* If either the JIT or the RegExp JIT is enabled, then the Assembler must be
   enabled as well: */
#if ENABLE(JIT) || ENABLE(YARR_JIT)
#if defined(ENABLE_ASSEMBLER) && !ENABLE_ASSEMBLER
#error "Cannot enable the JIT or RegExp JIT without enabling the Assembler"
#else
#undef ENABLE_ASSEMBLER
#define ENABLE_ASSEMBLER 1
#endif
#endif

/* Pick which allocator to use; we only need an executable allocator if the assembler is compiled in.
   On x86-64 we use a single fixed mmap, on other platforms we mmap on demand. */
#if ENABLE(ASSEMBLER)
#if CPU(X86_64) && !OS(WINDOWS) || PLATFORM(IOS)
#define ENABLE_EXECUTABLE_ALLOCATOR_FIXED 1
#else
#define ENABLE_EXECUTABLE_ALLOCATOR_DEMAND 1
#endif
#endif

/* Use the QXmlStreamReader implementation for XMLDocumentParser */
/* Use the QXmlQuery implementation for XSLTProcessor */
#if PLATFORM(QT)
#if !USE(LIBXML2)
#define WTF_USE_QXMLSTREAM 1
#define WTF_USE_QXMLQUERY 1
#endif
#endif

/* Accelerated compositing */
#if PLATFORM(MAC) || PLATFORM(IOS) || PLATFORM(QT) || (PLATFORM(WIN) && !USE(WINGDI) && !PLATFORM(WIN_CAIRO))
#define WTF_USE_ACCELERATED_COMPOSITING 1
#endif

#if ENABLE(WEBGL) && !defined(WTF_USE_3D_GRAPHICS)
#define WTF_USE_3D_GRAPHICS 1
#endif

/* Qt always uses Texture Mapper */
#if PLATFORM(QT)
#define WTF_USE_TEXTURE_MAPPER 1
#endif

#if USE(TEXTURE_MAPPER) && USE(3D_GRAPHICS) && !defined(WTF_USE_TEXTURE_MAPPER_GL)
#define WTF_USE_TEXTURE_MAPPER_GL 1
#endif

/* Compositing on the UI-process in WebKit2 */
#if USE(3D_GRAPHICS) && PLATFORM(QT)
#define WTF_USE_COORDINATED_GRAPHICS 1
#endif

#if PLATFORM(MAC) || PLATFORM(IOS)
#define WTF_USE_PROTECTION_SPACE_AUTH_CALLBACK 1
#endif

/* Set up a define for a common error that is intended to cause a build error -- thus the space after Error. */
#define WTF_PLATFORM_CFNETWORK Error USE_macro_should_be_used_with_CFNETWORK

#if PLATFORM(WIN)
#define WTF_USE_CROSS_PLATFORM_CONTEXT_MENUS 1
#endif

#if PLATFORM(MAC) && HAVE(ACCESSIBILITY)
#define WTF_USE_ACCESSIBILITY_CONTEXT_MENUS 1
#endif

#if CPU(ARM_THUMB2)
#define ENABLE_BRANCH_COMPACTION 1
#endif

#if !defined(ENABLE_THREADING_LIBDISPATCH) && HAVE(DISPATCH_H)
#define ENABLE_THREADING_LIBDISPATCH 1
#elif !defined(ENABLE_THREADING_OPENMP) && defined(_OPENMP)
#define ENABLE_THREADING_OPENMP 1
#elif !defined(THREADING_GENERIC)
#define ENABLE_THREADING_GENERIC 1
#endif

#if USE(GLIB)
#include <wtf/gobject/GTypedefs.h>
#endif

/* FIXME: This define won't be needed once #27551 is fully landed. However, 
   since most ports try to support sub-project independence, adding new headers
   to WTF causes many ports to break, and so this way we can address the build
   breakages one port at a time. */
#if !defined(WTF_USE_EXPORT_MACROS) && (PLATFORM(MAC) || PLATFORM(QT) || (PLATFORM(WIN) && (defined(_MSC_VER) && _MSC_VER >= 1600)))
#define WTF_USE_EXPORT_MACROS 1
#endif

#if !defined(WTF_USE_EXPORT_MACROS_FOR_TESTING) && (PLATFORM(GTK) || PLATFORM(WIN))
#define WTF_USE_EXPORT_MACROS_FOR_TESTING 1
#endif

#if (PLATFORM(QT) && !OS(DARWIN) && !OS(WINDOWS)) || PLATFORM(GTK) || PLATFORM(EFL)
#define WTF_USE_UNIX_DOMAIN_SOCKETS 1
#endif

#if !defined(WTF_USE_IMLANG_FONT_LINK2) && !OS(WINCE)
#define WTF_USE_IMLANG_FONT_LINK2 1
#endif

#if !defined(ENABLE_COMPARE_AND_SWAP) && (OS(WINDOWS) || (COMPILER(GCC) && (CPU(X86) || CPU(X86_64) || CPU(ARM_THUMB2))))
#define ENABLE_COMPARE_AND_SWAP 1
#endif

#define ENABLE_OBJECT_MARK_LOGGING 0

#if !defined(ENABLE_PARALLEL_GC) && !ENABLE(OBJECT_MARK_LOGGING) && (PLATFORM(MAC) || PLATFORM(IOS) || PLATFORM(QT) || PLATFORM(BLACKBERRY) || PLATFORM(GTK) || PLATFORM(EFL)) && ENABLE(COMPARE_AND_SWAP)
#define ENABLE_PARALLEL_GC 1
#endif

#if !defined(ENABLE_GC_VALIDATION) && !defined(NDEBUG)
#define ENABLE_GC_VALIDATION 1
#endif

#if !defined(ENABLE_BINDING_INTEGRITY) && !OS(WINDOWS)
#define ENABLE_BINDING_INTEGRITY 1
#endif

#if PLATFORM(MAC) && !PLATFORM(IOS) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070
#define WTF_USE_AVFOUNDATION 1
#endif

#if (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 60000) || (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080)
#define WTF_USE_COREMEDIA 1
#endif

#if (PLATFORM(MAC) || (OS(WINDOWS) && USE(CG))) && !PLATFORM(IOS) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
#define HAVE_AVFOUNDATION_MEDIA_SELECTION_GROUP 1
#endif

#if (PLATFORM(MAC) || (OS(WINDOWS) && USE(CG))) && !PLATFORM(IOS) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
#define HAVE_AVFOUNDATION_LEGIBLE_OUTPUT_SUPPORT 1
#endif

#if (PLATFORM(MAC) || (OS(WINDOWS) && USE(CG))) && !PLATFORM(IOS) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090
#define HAVE_MEDIA_ACCESSIBILITY_FRAMEWORK 1
#endif

#if PLATFORM(MAC) || PLATFORM(GTK) || (PLATFORM(WIN) && !USE(WINGDI) && !PLATFORM(WIN_CAIRO))
#define WTF_USE_REQUEST_ANIMATION_FRAME_TIMER 1
#endif

#if PLATFORM(MAC)
#define WTF_USE_REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR 1
#endif

#if PLATFORM(MAC) && (PLATFORM(IOS) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070)
#define HAVE_INVERTED_WHEEL_EVENTS 1
#endif

#if PLATFORM(MAC)
#define WTF_USE_COREAUDIO 1
#endif

#if !defined(WTF_USE_ZLIB) && !PLATFORM(QT)
#define WTF_USE_ZLIB 1
#endif

#if PLATFORM(QT)
#include <qglobal.h>
#if defined(QT_OPENGL_ES_2) && !defined(WTF_USE_OPENGL_ES_2)
#define WTF_USE_OPENGL_ES_2 1
#endif
#endif

#if !PLATFORM(IOS) && PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1080
#define WTF_USE_CONTENT_FILTERING 1
#endif


#define WTF_USE_GRAMMAR_CHECKING 1

#if PLATFORM(MAC) || PLATFORM(BLACKBERRY) || PLATFORM(EFL)
#define WTF_USE_UNIFIED_TEXT_CHECKING 1
#endif
#if PLATFORM(MAC)
#define WTF_USE_AUTOMATIC_TEXT_REPLACEMENT 1
#endif

#if PLATFORM(MAC) && (PLATFORM(IOS) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070)
/* Some platforms provide UI for suggesting autocorrection. */
#define WTF_USE_AUTOCORRECTION_PANEL 1
/* Some platforms use spelling and autocorrection markers to provide visual cue. On such platform, if word with marker is edited, we need to remove the marker. */
#define WTF_USE_MARKER_REMOVAL_UPON_EDITING 1
#endif /* #if PLATFORM(MAC) && (PLATFORM(IOS) || __MAC_OS_X_VERSION_MIN_REQUIRED >= 1070) */

#if PLATFORM(MAC) || PLATFORM(IOS)
#define WTF_USE_AUDIO_SESSION 1
#endif

#endif /* WTF_Platform_h */
