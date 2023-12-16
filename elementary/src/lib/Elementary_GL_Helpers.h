/*
 * Elementary EGL/OpenGL-ES helper macros and functions
 *
 * Author: Jean-Philippe ANDRE <jpeg@videolan.org>
 */

#ifndef _ELEMENTARY_GL_HELPERS_H
#define _ELEMENTARY_GL_HELPERS_H

#include <Elementary.h>
#include <Evas_GL_GLES1_Helpers.h>
#include <Evas_GL_GLES2_Helpers.h>

/**
 * @file Elementary_GL_Helpers.h
 * @brief Convenience functions and definitions to use OpenGL with EFL.
 *
 * Applications can combine OpenGL and Elementary using the @ref GLView widget.
 * Because of the design and specific features of EFL's OpenGL support, porting
 * an application from OpenGL-ES+EGL to EFL can be a bit tedious. In order to
 * ease the transition, this file provides a set of convenience functions and
 * macros.
 *
 * Please also refer to @ref elm_opengl_page and the various guides about
 * @ref Evas_GL "Evas GL".
 *
 * Users can include this file instead of any other GL or EGL header file.
 * This will redefine all the standard GL functions into macros using the
 * @ref Evas_GL_API structure.
 */

/**
 * @name Convenience functions for OpenGL
 * @{
 */

/**
 * @brief Convenience macro to insert at the beginning of every function calling OpenGL with @ref GLView.
 * @param[in]  glview   Elementary GLView object in use
 *
 * Here's a very simple code example:
 * @code
static void _draw_gl(Evas_Object *obj)
{
   ELEMENTARY_GLVIEW_USE(obj);

   glClearColor(0.2, 0.2, 0.2, 1.0);
   glClear(GL_COLOR_BUFFER_BIT);
}
 * @endcode
 *
 * This is the equivalent of:
 * @code
static void _draw_gl(Evas_Object *obj)
{
   Evas_GL_API *api = elm_glview_gl_api_get(obj);

   api->glClearColor(0.2, 0.2, 0.2, 1.0);
   api->glClear(GL_COLOR_BUFFER_BIT);
}
 * @endcode
 *
 * @note This macro should be used in every function that calls OpenGL from the
 *       Elementary. This indeed means that each function must have access to
 *       the @ref GLView widget. Although this might require some changes in
 *       existing GL codebases, this is the recommended way to use the GL API.
 *
 * @since_tizen 2.3
 */
#define ELEMENTARY_GLVIEW_USE(glview) \
   Evas_GL_API *__evas_gl_glapi = elm_glview_gl_api_get(glview);

/**
 * @brief Convenience macro to insert at the beginning of every function calling OpenGL with @ref GLView.
 *
 * @param[in]  glview   Elementary @ref GLView object in use
 * @param[in]  retval   A value to return in case of failure (GL API was not found), can be empty
 *
 * This is similar to @ref ELEMENTARY_GLVIEW_USE except that it will return from
 * the function if the GL API can not be used.
 *
 * @since_tizen 2.3
 */
#define ELEMENTARY_GLVIEW_USE_OR_RETURN(glview, retval) \
   Evas_GL_API *__evas_gl_glapi = elm_glview_gl_api_get(glview); \
   if (!__evas_gl_glapi) return retval;

/**
 * @brief Convenience function returning the GL view's rotation when using direct rendering
 *
 * @param[in]  glview   The Elementary @ref GLView object in use
 *
 * @return A screen rotation in degrees (0, 90, 180, 270).
 *
 * @note This rotation can be different from the device orientation. This
 *       rotation value must be used in case of direct rendering and should be
 *       taken into account by the application when setting the internal rotation
 *       matrix for the view.
 *
 * @since_tizen 2.3
 */
#define elm_glview_rotation_get(glview) \
   (evas_gl_rotation_get(elm_glview_evas_gl_get(glview))

// End of the convenience functions
/** @} */


// Global convenience functions and big warnings on their limited usage

/**
 * @name Convenience functions for OpenGL (global definitions)
 * @{
 *
 * This second set of helper macros can be used in simple applications
 * that use OpenGL with a single target surface.
 *
 * @warning Be very careful when using these macros! The only recommended solution
 *          is to use @ref ELEMENTARY_GLVIEW_USE in every client function. Here
 *          are some situations where you should <b>not</b> use the global helpers:
 * @li If you are using more than one Evas canvas at a time (eg. multiple windows).
 *     The GL API will be different if you are using different rendering engines
 *     (software and GL for instance), and this can happen as soon as you have
 *     multiple canvases.
 * @li If you are using both OpenGL-ES 1.1 and OpenGL-ES 2.0 APIs.
 * @li If you are writing or porting a library that may be used by other
 *     applications.
 *
 * This set of macros should be used only in the following situation:
 * @li Only one surface is used for GL rendering,
 * @li Only one API set (GLES 1.1 or GLES 2.0) is used
 */

/**
 * @brief Convenience macro to use the GL helpers in simple applications: declare
 *
 * @c ELEMENTARY_GLVIEW_GLOBAL_DECLARE should be used in a global header for the
 * application. For example, in a platform-specific compatibility header file.
 *
 * Example of a global header file @c main.h:
 * @code
#include <Elementary_GL_Helpers.h>
// other includes...

ELEMENTARY_GLVIEW_GLOBAL_DECLARE()

// ...
 * @endcode
 *
 * @see @ref ELEMENTARY_GLVIEW_GLOBAL_DEFINE
 * @see @ref ELEMENTARY_GLVIEW_GLOBAL_USE
 *
 * @since_tizen 2.3
 */
#define ELEMENTARY_GLVIEW_GLOBAL_DECLARE() \
   extern Evas_GL_API *__evas_gl_glapi;

/**
 * @brief Convenience macro to use the GL helpers in simple applications: define
 *
 * @c ELEMENTARY_GLVIEW_GLOBAL_DEFINE should be used at the top of a file
 * creating the @ref GLView widget.
 *
 * Example of a file @c glview.c:
 *
 * @code
#include "main.h"
ELEMENTARY_GLVIEW_GLOBAL_DEFINE()

// ...

static Evas_Object *
glview_create(Evas_Object *parent)
{
   Evas_Object *glview;

   glview = elm_glview_version_add(parent, EVAS_GL_GLES_2_X);
   ELEMENTARY_GLVIEW_GLOBAL_USE(glview);

   elm_glview_mode_set(glview, ELM_GLVIEW_ALPHA | ELM_GLVIEW_DEPTH | ELM_GLVIEW_STENCIL);
   elm_glview_resize_policy_set(glview, ELM_GLVIEW_RESIZE_POLICY_RECREATE);
   elm_glview_render_policy_set(glview, ELM_GLVIEW_RENDER_POLICY_ON_DEMAND);

   elm_glview_init_func_set(glview, _init_gl);
   elm_glview_del_func_set(glview, _del_gl);
   elm_glview_resize_func_set(glview, _resize_gl);
   elm_glview_render_func_set(glview, _draw_gl);

   return glview;
}

// ...
 * @endcode
 *
 * @see @ref ELEMENTARY_GLVIEW_GLOBAL_DECLARE
 * @see @ref ELEMENTARY_GLVIEW_GLOBAL_USE
 *
 * @since_tizen 2.3
 */
#define ELEMENTARY_GLVIEW_GLOBAL_DEFINE() \
   Evas_GL_API *__evas_gl_glapi = NULL;

/**
 * @brief Convenience macro to use the GL helpers in simple applications: use
 *
 * This macro will set the global variable holding the GL API so that it's
 * available to the application.
 *
 * It should be used right after setting up the @ref GLView object.
 *
 * @see @ref ELEMENTARY_GLVIEW_GLOBAL_DECLARE
 * @see @ref ELEMENTARY_GLVIEW_GLOBAL_DEFINE
 *
 * @since_tizen 2.3
 */
#define ELEMENTARY_GLVIEW_GLOBAL_USE(glview) \
   do { __evas_gl_glapi = elm_glview_gl_api_get(glview); } while (0)

/**
 * @brief Macro to check that the GL APIs are properly set (GLES 1.1)
 */
#define ELEMENTARY_GLVIEW_GLES1_API_CHECK() EVAS_GL_GLES1_API_CHECK()

/**
 * @brief Macro to check that the GL APIs are properly set (GLES 2.0)
 */
#define ELEMENTARY_GLVIEW_GLES2_API_CHECK() EVAS_GL_GLES2_API_CHECK()


// End of the convenience functions
/** @} */

/**
 * @page elm_opengl_page OpenGL with Elementary
 *
 *
 * @section elm_opengl_egl2evasgl Porting a native EGL+OpenGL-ES2 application to EFL
 *
 * Contents of this section:
 * @li @ref elm_opengl_foreword
 * @li @ref elm_opengl_why
 * @li @ref elm_opengl_evasglexample
 * @li @ref elm_opengl_direct_rendering
 * @li @ref elm_opengl_gles1
 * @li @ref elm_opengl_otheregl
 *
 *
 * @subsection elm_opengl_foreword Foreword
 *
 * While Evas and Ecore provide all the required functions to build a whole
 * application based on EFL and using OpenGL, it is recommended to use
 * @ref GLView instead. Elementary @ref GLView will create a drawable GL
 * surface for the application, and set up all the required callbacks so that
 * the complexity of Evas GL is hidden.
 *
 *
 * @subsection elm_opengl_helpers Convenience functions for OpenGL with EFL
 *
 * The file @ref Elementary_GL_Helpers.h provides some convenience functions
 * that ease the use of OpenGL within an Elementary application.
 *
 *
 * @subsection elm_opengl_why Why all the trouble?
 *
 * Evas GL is an abstraction layer on top of EGL, GLX or WGL that should provide
 * the necessary features for most applications, in a platform-independent way.
 * Since the goal of Evas GL is to abstract the underlying platform, only a
 * subset of the features can be used by applications.
 *
 * On top of this, an Evas GL surface can be stacked within a GUI layout just
 * like any other widget, and will support transparency, direct rendering, and
 * various other features. For these reasons, it is not possible to directly
 * expose lower level APIs like OpenGL or EGL and interact with Evas as the
 * same time.
 *
 * The following sections should provide developers guides on how to use
 * OpenGL-ES in an EFL application.
 *
 *
 * @subsection elm_opengl_evasglexample Evas GL initialiation with GLView
 *
 * When using @ref GLView, EFL will take care of the tedious creation of all
 * the surfaces and contexts. Also, EFL hides the underlying display system
 * so there is no way to get a direct handle to
 *
 * Here is a demo using EFL with OpenGL:
 *
 * @code
// gcc `pkg-config --cflags --libs elementary` glview.c -o glview
#include <Elementary_GL_Helpers.h>

static void _draw_gl(Evas_Object *obj)
{
   ELEMENTARY_GLVIEW_USE(obj);

   glClearColor(0.2, 0.2, 0.2, 1.0);
   glClear(GL_COLOR_BUFFER_BIT);
}

static void _resize_gl(Evas_Object *obj)
{
   ELEMENTARY_GLVIEW_USE(obj);
   int w, h;

   elm_glview_size_get(obj, &w, &h);
   glViewport(0, 0, w, h);
}

// This is the GL initialization function
Evas_Object* glview_create(Evas_Object *win)
{
   Evas_Object *glview;

   glview = elm_glview_add(win);
   elm_win_resize_object_add(win, glview);
   elm_glview_mode_set(glview, ELM_GLVIEW_ALPHA | ELM_GLVIEW_DEPTH | ELM_GLVIEW_STENCIL);
   elm_glview_resize_policy_set(glview, ELM_GLVIEW_RESIZE_POLICY_RECREATE);
   elm_glview_render_policy_set(glview, ELM_GLVIEW_RENDER_POLICY_ON_DEMAND);
   //elm_glview_init_func_set(glview, _init_gl);
   //elm_glview_del_func_set(glview, _del_gl);
   elm_glview_render_func_set(glview, _draw_gl);
   elm_glview_resize_func_set(glview, _resize_gl);
   evas_object_size_hint_min_set(glview, 250, 250);
   evas_object_show(glview);

   return glview;
}

EAPI int elm_main(int argc EINA_UNUSED, char **argv EINA_UNUSED)
{
   Evas_Object *win;

   // Make sure that the engine supports OpenGL
   elm_config_engine_set("opengl_x11");

   // Create a window
   win = elm_win_util_standard_add("glview", "GLView");
   evas_object_show(win);

   // Setup our GLView
   glview_create(win);

   elm_run();
   elm_shutdown();
   return 0;
}
ELM_MAIN()
 * @endcode
 *
 *
 *
 * @subsection elm_opengl_direct_rendering Direct rendering with Evas GL
 *
 * Evas GL can be used to render directly to the back buffer of the Evas
 * window. It is important to note that this is an optimization path and this
 * will present a few limitations. Normally, Evas GL will create an FBO and use
 * it as target surface for all the GL draw operations, and finally blend this
 * FBO's contents to the Evas canvas.
 *
 * Evas GL will automatically fallback to indirect rendering unless the
 * conditions are right. The flag @ref EVAS_GL_OPTIONS_DIRECT must be set on
 * the target surface in order to enable direct rendering. When using the
 * Elementary GLView widget, the flag @c ELM_GLVIEW_DIRECT should be set.
 *
 * Some limitations of direct rendering include the following:
 * @li If @c glClear() is called after @c glClearColor(0,0,0,0), then clear will be
 *     skipped for the color buffer. Otherwise this would erase the back buffer
 *     in evas, instead of preparing a transparent surface for GL rendering.
 *     Opaque colors will be cleared as expected (eg. @c (0,0,0,1)).
 * @li Evas GL will fallback to indirect rendering if an Evas Map is applied,
 *     or if other conditions force Evas to fallback.
 * @li The user application is responsible for handling screen rotation when
 *     using direct rendering, which is why applications should set the
 *     flag @ref EVAS_GL_OPTIONS_CLIENT_SIDE_ROTATION on the GL surface (or
 *     @ref ELM_GLVIEW_CLIENT_SIDE_ROTATION for @ref GLView). Please also see
 *     @ref evas_gl_rotation_get.
 *
 * @note Direct rendering is an option that can drastically improve the
 *       performance of OpenGL applications, but it can exhibit some side effects.
 *
 *
 *
 * @subsection elm_opengl_gles1 OpenGL-ES 1.1 support in EFL
 *
 * Since Tizen 2.3, Evas GL supports the OpenGL-ES 1.1 set of rendering APIs on
 * top of the normal OpenGL-ES 2.0 APIs, if the drivers supports it.
 *
 * With @ref GLView, it is easy to create a 1.1 capable surface:
 * @code
Evas_Object *glview;
glview = elm_glview_version_add(win, EVAS_GL_GLES_1_X);
 * @endcode
 *
 * As usual, the GL API is available using @ref elm_glview_gl_api_get, which
 * can be abstracted with @ref ELEMENTARY_GLVIEW_USE.
 *
 * When using Evas GL directly, developers must be careful to use
 * @ref evas_gl_context_api_get with the current context in order to get
 * the proper API (1.1 or 2.0). Indeed, @ref evas_gl_api_get will always return
 * a GLES 2.0 API, and the two APIs are not compatible. Also, the application
 * will then be responsible for calling @c evas_gl_make_current.
 *
 * @remarks Always use @ref GLView unless there is a very good reason not to.
 *
 *
 * @subsection elm_opengl_otheregl Other uses of EGL and their Evas GL equivalents
 *
 * Of course, Evas GL is not limited to creating fullscreen OpenGL target surfaces,
 * and some developers might want to exploit some of the more advanced features
 * of Evas GL.
 *
 * @li @ref elm_opengl_evasglvsegl "Evas GL vs. EGL"
 * @li @ref elm_opengl_surfquery "Query surfaces for their properties"
 * @li @ref elm_opengl_pbuffer "PBuffer surfaces for multithread rendering"
 *
 * These features usually require to use @ref Evas_GL directly rather than just
 * @ref GLView.
 *
 *
 * @anchor elm_opengl_evasglvsegl
 * <h2> Evas GL vs. EGL </h2>
 *
 * As explained above, Evas GL is an abstraction layer on top of EGL, but not
 * limited to EGL. While trying to look similar to EGL, Evas GL should also
 * support GLX and other backends (WGL, ...).
 *
 * As a consequence, only a subset of the EGL features are supported by Evas GL.
 *
 * Manual work is required in order to transform all calls to EGL into Evas GL
 * calls, but the final code should be much lighter when using EFL. Here is
 * a simple table comparison between EGL and Evas GL calls:
 *
 * <table border="1">
 *    <tr> <th> EGL </th> <th> Evas GL </th> <th> Comment </th> </tr>
 *    <tr> <td> @c eglGetDisplay </td> <td> N/A </td> <td> Not required </td> </tr>
 *    <tr> <td> @c eglInitialize </td> <td> @ref evas_gl_new </td> <td> - </td> </tr>
 *    <tr> <td> @c eglTerminate </td> <td> @ref evas_gl_free </td> <td> - </td> </tr>
 *    <tr> <td> @c eglQueryString </td> <td> @ref evas_gl_string_query </td> <td> For extensions: if (glapi->evasglExtFunc) { ... } </td> </tr>
 *    <tr> <td> @c eglReleaseThread </td> <td> N/A </td> <td> - </td> </tr>
 *    <tr> <td> @c eglGetConfigs </td> <td> N/A </td> <td> Not required </td> </tr>
 *    <tr> <td> @c eglGetConfigAttrib </td> <td> N/A </td> <td> - </td> </tr>
 *    <tr> <td> @c eglChooseConfig </td> <td> @ref evas_gl_config_new and @ref Evas_GL_Config </td> <td> - </td> </tr>
 *    <tr> <td> @c eglCreateWindowSurface </td> <td> @ref GLView or @ref evas_gl_surface_create </td> <td> @ref GLView provides @ref elm_glview_add </td> </tr>
 *    <tr> <td> @c eglCreatePixmapSurface </td> <td> N/A </td> <td> Not available because it is platform dependent </td> </tr>
 *    <tr> <td> @c eglCreatePbufferSurface </td> <td> @ref evas_gl_pbuffer_surface_create </td> <td> - </td> </tr>
 *    <tr> <td> @c eglCreatePbufferFromClientBuffer </td> <td> N/A </td> <td> - </td> </tr>
 *    <tr> <td> @c eglDestroySurface </td> <td> @ref evas_gl_surface_destroy </td> <td> - </td> </tr>
 *    <tr> <td> @c eglSurfaceAttrib </td> <td> N/A </td> <td> Surfaces can't be changed </td> </tr>
 *    <tr> <td> @c eglQuerySurface </td> <td> @ref evas_gl_surface_query </td> <td> Subset of features only </td> </tr>
 *    <tr> <td> @c eglCreateContext </td> <td> @ref evas_gl_context_create and @ref evas_gl_context_version_create </td> <td> @ref GLView provides @ref elm_glview_add </td> </tr>
 *    <tr> <td> @c eglDestroyContext </td> <td> @ref evas_gl_context_destroy </td> <td> - </td> </tr>
 *    <tr> <td> @c eglMakeCurrent </td> <td> @ref evas_gl_make_current </td> <td> - </td> </tr>
 *    <tr> <td> @c eglWaitGL </td> <td> N/A </td> <td> Use a fence sync if available </td> </tr>
 *    <tr> <td> @c eglWaitNative </td> <td> N/A </td> <td> Use a fence sync if available </td> </tr>
 *    <tr> <td> @c eglWaitClient </td> <td> N/A </td> <td> Use a fence sync if available </td> </tr>
 *    <tr> <td> @c eglSwapBuffers </td> <td> N/A </td> <td> Transparently done by Evas </td> </tr>
 *    <tr> <td> @c eglCopyBuffers </td> <td> N/A </td> <td> Not available because it is platform dependent </td> </tr>
 *    <tr> <td> @c eglSwapInterval </td> <td> N/A </td> <td> Transparently done by Ecore and Evas </td> </tr>
 *    <tr> <td> @c eglBindTexImage </td> <td> N/A </td> <td> Not available, use FBOs </td> </tr>
 *    <tr> <td> @c eglReleaseTexImage </td> <td> N/A </td> <td> Not available, use FBOs </td> </tr>
 *    <tr> <td> @c eglGetProcAddress </td> <td> @ref Evas_GL_API: @ref evas_gl_proc_address_get </td> <td> Provides extra Evas GL extensions (not EGL) </td> </tr>
 *    <tr> <td> @c eglCreateImageKHR </td> <td> @ref Evas_GL_API: @ref evasglCreateImageForContext </td> <td> Extension </td> </tr>
 *    <tr> <td> @c eglDestroyImageKHR </td> <td> @ref Evas_GL_API: @ref evasglDestroyImage </td> <td> Extension </td> </tr>
 *    <tr> <td> @c eglCreateSyncKHR </td> <td> @ref Evas_GL_API: @ref evasglCreateSync </td> <td> Extension </td> </tr>
 *    <tr> <td> @c eglDestroySyncKHR </td> <td> @ref Evas_GL_API: @ref evasglDestroySync </td> <td> Extension </td> </tr>
 *    <tr> <td> @c eglClientWaitSyncKHR </td> <td> @ref Evas_GL_API: @ref evasglClientWaitSync </td> <td> Extension </td> </tr>
 *    <tr> <td> @c eglSignalSyncKHR </td> <td> @ref Evas_GL_API: @ref evasglSignalSync </td> <td> Extension </td> </tr>
 *    <tr> <td> @c eglGetSyncAttribKHR </td> <td> @ref Evas_GL_API: @ref evasglGetSyncAttrib </td> <td> Extension </td> </tr>
 *    <tr> <td> @c eglWaitSyncKHR </td> <td> @ref Evas_GL_API: @ref evasglWaitSync </td> <td> Extension </td> </tr>
 * </table>
 *
 * The extensions above may or may not be available depending on the OpenGL
 * driver and the backend used.
 *
 * Some EGL definitions have also been imported and transformed for Evas GL. In
 * particular, the EVAS_GL error codes returned by @ref evas_gl_error_get don't
 * start from @c 0x3000 like EGL but from 0. Also, attribute lists can be
 * terminated by 0 instead of @c EGL_NONE.
 *
 *
 * @anchor elm_opengl_surfquery
 * <h2> Query surfaces for their properties </h2>
 *
 * When using EGL, it is common to query a surface for its properties. Evas GL
 * supports only a subset of the surface properties:
 * @li @ref EVAS_GL_WIDTH,
 * @li @ref EVAS_GL_HEIGHT,
 * @li @ref EVAS_GL_TEXTURE_FORMAT,
 * @li @ref EVAS_GL_TEXTURE_TARGET
 *
 * Refer to @ref evas_gl_surface_query for more information.
 *
 *
 * @anchor elm_opengl_pbuffer
 * <h2> PBuffer surfaces for multithread rendering </h2>
 *
 * If an application wants to render offscreen using OpenGL, it can use FBOs.
 * But if the application wants to render in a separate render thread, a surface
 * must be created for that thread in order to call @ref evas_gl_make_current.
 *
 * In the EGL world, it is common to create a PBuffer surface with
 * @c eglCreatePBufferSurface() and set its size to 1x1. With @ref Evas_GL this
 * is possible using @ref evas_gl_pbuffer_surface_create.
 *
 * Here is how an application could setup a render context in a separate thread:
 *
 * @code
// In the init function:
Evas_GL_Surface *sfc;
Evas_GL_Config *cfg;
Evas_GL_Context *ctx;
Evas_GL *evasgl;

evasgl = elm_glview_evas_gl_get(glview);

cfg = evas_gl_config_new();
cfg->color_format = EVAS_GL_RGBA_8888;
cfg->depth_bits = EVAS_GL_DEPTH_NONE;
cfg->stencil_bits = EVAS_GL_STENCIL_NONE;
cfg->options_bits = EVAS_GL_OPTIONS_NONE;

sfc = evas_gl_pbuffer_surface_create(evasgl, cfg, WIDTH, HEIGHT, NULL);
ctx = evas_gl_context_create(elm_glview_evas_gl_get(glview), NULL);
evas_gl_config_free(cfg);
// ...

// In the render function:
evas_gl_make_current(evasgl, sfc, ctx);

// Render to a FBO, bind it to a native image and pass it to the main thread
// using an EvasGLImage.
 * @endcode
 *
 * Multithread OpenGL rendering with Evas GL is the topic of another guide.
 *
 *
 */

#endif // _ELEMENTARY_GL_HELPERS_H
