/*
 *  Copyright (C) 2010 Tieto Corporation.
 *  Copyright (C) 2011 Igalia S.L.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifndef OpenGLShims_h
#define OpenGLShims_h

#if PLATFORM(QT)
#include <qglobal.h>
#include <qopenglfunctions.h>
#include <QOpenGLContext>
#include <QSurface>
#elif PLATFORM(TIZEN)
#include <Evas_GL.h>
#else
#include <GL/gl.h>
#include <GL/glext.h>
#endif

#if defined(GL_ES_VERSION_2_0)
// Some openGL ES systems miss this typedef.
typedef char GLchar;
#endif

typedef struct _OpenGLFunctionTable OpenGLFunctionTable;

namespace WebCore {
bool initializeOpenGLShims();
OpenGLFunctionTable* openGLFunctionTable();

#if PLATFORM(TIZEN)
class EvasGlApiInterface {
public:
    static EvasGlApiInterface& shared()
    {
        static EvasGlApiInterface& evasGlApiInterface = *new EvasGlApiInterface;
        return evasGlApiInterface;
    }

    void initialize(Evas_GL_API* evasGlApi) { m_evasGlApi = evasGlApi; }
    Evas_GL_API* evasGlApi() { return m_evasGlApi; }

private:
    EvasGlApiInterface()
        : m_evasGlApi(0)
    {
    }

    Evas_GL_API* m_evasGlApi;
};
#endif
}

#if OS(WINDOWS)
#define GLAPIENTRY __stdcall
#else
#define GLAPIENTRY
#endif

#if PLATFORM(TIZEN)
#define LOOKUP_GL_FUNCTION(Function)                WebCore::EvasGlApiInterface::shared().evasGlApi()->Function
#define glActiveTexture                             LOOKUP_GL_FUNCTION(glActiveTexture)
#define glAttachShader                              LOOKUP_GL_FUNCTION(glAttachShader)
#define glBindAttribLocation                        LOOKUP_GL_FUNCTION(glBindAttribLocation)
#define glBindBuffer                                LOOKUP_GL_FUNCTION(glBindBuffer)
#define glBindFramebuffer                           LOOKUP_GL_FUNCTION(glBindFramebuffer)
#define glBindRenderbuffer                          LOOKUP_GL_FUNCTION(glBindRenderbuffer)
#define glBindTexture                               LOOKUP_GL_FUNCTION(glBindTexture)
#define glBlendColor                                LOOKUP_GL_FUNCTION(glBlendColor)
#define glBlendEquation                             LOOKUP_GL_FUNCTION(glBlendEquation)
#define glBlendEquationSeparate                     LOOKUP_GL_FUNCTION(glBlendEquationSeparate)
#define glBlendFunc                                 LOOKUP_GL_FUNCTION(glBlendFunc)
#define glBlendFuncSeparate                         LOOKUP_GL_FUNCTION(glBlendFuncSeparate)
#define glBufferData                                LOOKUP_GL_FUNCTION(glBufferData)
#define glBufferSubData                             LOOKUP_GL_FUNCTION(glBufferSubData)
#define glCheckFramebufferStatus                    LOOKUP_GL_FUNCTION(glCheckFramebufferStatus)
#define glClear                                     LOOKUP_GL_FUNCTION(glClear)
#define glClearColor                                LOOKUP_GL_FUNCTION(glClearColor)
#define glClearDepthf                               LOOKUP_GL_FUNCTION(glClearDepthf)
#define glClearStencil                              LOOKUP_GL_FUNCTION(glClearStencil)
#define glColorMask                                 LOOKUP_GL_FUNCTION(glColorMask)
#define glCompileShader                             LOOKUP_GL_FUNCTION(glCompileShader)
#define glCompressedTexImage2D                      LOOKUP_GL_FUNCTION(glCompressedTexImage2D)
#define glCompressedTexSubImage2D                   LOOKUP_GL_FUNCTION(glCompressedTexSubImage2D)
#define glCopyTexImage2D                            LOOKUP_GL_FUNCTION(glCopyTexImage2D)
#define glCopyTexSubImage2D                         LOOKUP_GL_FUNCTION(glCopyTexSubImage2D)
#define glCreateProgram                             LOOKUP_GL_FUNCTION(glCreateProgram)
#define glCreateShader                              LOOKUP_GL_FUNCTION(glCreateShader)
#define glCullFace                                  LOOKUP_GL_FUNCTION(glCullFace)
#define glDeleteBuffers                             LOOKUP_GL_FUNCTION(glDeleteBuffers)
#define glDeleteFramebuffers                        LOOKUP_GL_FUNCTION(glDeleteFramebuffers)
#define glDeleteProgram                             LOOKUP_GL_FUNCTION(glDeleteProgram)
#define glDeleteRenderbuffers                       LOOKUP_GL_FUNCTION(glDeleteRenderbuffers)
#define glDeleteShader                              LOOKUP_GL_FUNCTION(glDeleteShader)
#define glDeleteTextures                            LOOKUP_GL_FUNCTION(glDeleteTextures)
#define glDepthFunc                                 LOOKUP_GL_FUNCTION(glDepthFunc)
#define glDepthMask                                 LOOKUP_GL_FUNCTION(glDepthMask)
#define glDepthRangef                               LOOKUP_GL_FUNCTION(glDepthRangef)
#define glDetachShader                              LOOKUP_GL_FUNCTION(glDetachShader)
#define glDisable                                   LOOKUP_GL_FUNCTION(glDisable)
#define glDisableVertexAttribArray                  LOOKUP_GL_FUNCTION(glDisableVertexAttribArray)
#define glDrawArrays                                LOOKUP_GL_FUNCTION(glDrawArrays)
#define glDrawElements                              LOOKUP_GL_FUNCTION(glDrawElements)
#define glEnable                                    LOOKUP_GL_FUNCTION(glEnable)
#define glEnableVertexAttribArray                   LOOKUP_GL_FUNCTION(glEnableVertexAttribArray)
#define glFinish                                    LOOKUP_GL_FUNCTION(glFinish)
#define glFlush                                     LOOKUP_GL_FUNCTION(glFlush)
#define glFramebufferRenderbuffer                   LOOKUP_GL_FUNCTION(glFramebufferRenderbuffer)
#define glFramebufferTexture2D                      LOOKUP_GL_FUNCTION(glFramebufferTexture2D)
#define glFrontFace                                 LOOKUP_GL_FUNCTION(glFrontFace)
#define glGenBuffers                                LOOKUP_GL_FUNCTION(glGenBuffers)
#define glGenerateMipmap                            LOOKUP_GL_FUNCTION(glGenerateMipmap)
#define glGenFramebuffers                           LOOKUP_GL_FUNCTION(glGenFramebuffers)
#define glGenRenderbuffers                          LOOKUP_GL_FUNCTION(glGenRenderbuffers)
#define glGenTextures                               LOOKUP_GL_FUNCTION(glGenTextures)
#define glGetActiveAttrib                           LOOKUP_GL_FUNCTION(glGetActiveAttrib)
#define glGetActiveUniform                          LOOKUP_GL_FUNCTION(glGetActiveUniform)
#define glGetAttachedShaders                        LOOKUP_GL_FUNCTION(glGetAttachedShaders)
#define glGetAttribLocation                         LOOKUP_GL_FUNCTION(glGetAttribLocation)
#define glGetBooleanv                               LOOKUP_GL_FUNCTION(glGetBooleanv)
#define glGetBufferParameteriv                      LOOKUP_GL_FUNCTION(glGetBufferParameteriv)
#define glGetError                                  LOOKUP_GL_FUNCTION(glGetError)
#define glGetFloatv                                 LOOKUP_GL_FUNCTION(glGetFloatv)
#define glGetFramebufferAttachmentParameteriv       LOOKUP_GL_FUNCTION(glGetFramebufferAttachmentParameteriv)
#define glGetIntegerv                               LOOKUP_GL_FUNCTION(glGetIntegerv)
#define glGetProgramiv                              LOOKUP_GL_FUNCTION(glGetProgramiv)
#define glGetProgramInfoLog                         LOOKUP_GL_FUNCTION(glGetProgramInfoLog)
#define glGetRenderbufferParameteriv                LOOKUP_GL_FUNCTION(glGetRenderbufferParameteriv)
#define glGetShaderiv                               LOOKUP_GL_FUNCTION(glGetShaderiv)
#define glGetShaderInfoLog                          LOOKUP_GL_FUNCTION(glGetShaderInfoLog)
#define glGetShaderPrecisionFormat                  LOOKUP_GL_FUNCTION(glGetShaderPrecisionFormat)
#define glGetShaderSource                           LOOKUP_GL_FUNCTION(glGetShaderSource)
#define glGetString                                 LOOKUP_GL_FUNCTION(glGetString)
#define glGetTexParameterfv                         LOOKUP_GL_FUNCTION(glGetTexParameterfv)
#define glGetTexParameteriv                         LOOKUP_GL_FUNCTION(glGetTexParameteriv)
#define glGetUniformfv                              LOOKUP_GL_FUNCTION(glGetUniformfv)
#define glGetUniformiv                              LOOKUP_GL_FUNCTION(glGetUniformiv)
#define glGetUniformLocation                        LOOKUP_GL_FUNCTION(glGetUniformLocation)
#define glGetVertexAttribfv                         LOOKUP_GL_FUNCTION(glGetVertexAttribfv)
#define glGetVertexAttribiv                         LOOKUP_GL_FUNCTION(glGetVertexAttribiv)
#define glGetVertexAttribPointerv                   LOOKUP_GL_FUNCTION(glGetVertexAttribPointerv)
#define glHint                                      LOOKUP_GL_FUNCTION(glHint)
#define glIsBuffer                                  LOOKUP_GL_FUNCTION(glIsBuffer)
#define glIsEnabled                                 LOOKUP_GL_FUNCTION(glIsEnabled)
#define glIsFramebuffer                             LOOKUP_GL_FUNCTION(glIsFramebuffer)
#define glIsProgram                                 LOOKUP_GL_FUNCTION(glIsProgram)
#define glIsRenderbuffer                            LOOKUP_GL_FUNCTION(glIsRenderbuffer)
#define glIsShader                                  LOOKUP_GL_FUNCTION(glIsShader)
#define glIsTexture                                 LOOKUP_GL_FUNCTION(glIsTexture)
#define glLineWidth                                 LOOKUP_GL_FUNCTION(glLineWidth)
#define glLinkProgram                               LOOKUP_GL_FUNCTION(glLinkProgram)
#define glPixelStorei                               LOOKUP_GL_FUNCTION(glPixelStorei)
#define glPolygonOffset                             LOOKUP_GL_FUNCTION(glPolygonOffset)
#define glReadPixels                                LOOKUP_GL_FUNCTION(glReadPixels)
#define glReleaseShaderCompiler                     LOOKUP_GL_FUNCTION(glReleaseShaderCompiler)
#define glRenderbufferStorage                       LOOKUP_GL_FUNCTION(glRenderbufferStorage)
#define glSampleCoverage                            LOOKUP_GL_FUNCTION(glSampleCoverage)
#define glScissor                                   LOOKUP_GL_FUNCTION(glScissor)
#define glShaderBinary                              LOOKUP_GL_FUNCTION(glShaderBinary)
#define glShaderSource                              LOOKUP_GL_FUNCTION(glShaderSource)
#define glStencilFunc                               LOOKUP_GL_FUNCTION(glStencilFunc)
#define glStencilFuncSeparate                       LOOKUP_GL_FUNCTION(glStencilFuncSeparate)
#define glStencilMask                               LOOKUP_GL_FUNCTION(glStencilMask)
#define glStencilMaskSeparate                       LOOKUP_GL_FUNCTION(glStencilMaskSeparate)
#define glStencilOp                                 LOOKUP_GL_FUNCTION(glStencilOp)
#define glStencilOpSeparate                         LOOKUP_GL_FUNCTION(glStencilOpSeparate)
#define glTexImage2D                                LOOKUP_GL_FUNCTION(glTexImage2D)
#define glTexParameterf                             LOOKUP_GL_FUNCTION(glTexParameterf)
#define glTexParameterfv                            LOOKUP_GL_FUNCTION(glTexParameterfv)
#define glTexParameteri                             LOOKUP_GL_FUNCTION(glTexParameteri)
#define glTexParameteriv                            LOOKUP_GL_FUNCTION(glTexParameteriv)
#define glTexSubImage2D                             LOOKUP_GL_FUNCTION(glTexSubImage2D)
#define glUniform1f                                 LOOKUP_GL_FUNCTION(glUniform1f)
#define glUniform1fv                                LOOKUP_GL_FUNCTION(glUniform1fv)
#define glUniform1i                                 LOOKUP_GL_FUNCTION(glUniform1i)
#define glUniform1iv                                LOOKUP_GL_FUNCTION(glUniform1iv)
#define glUniform2f                                 LOOKUP_GL_FUNCTION(glUniform2f)
#define glUniform2fv                                LOOKUP_GL_FUNCTION(glUniform2fv)
#define glUniform2i                                 LOOKUP_GL_FUNCTION(glUniform2i)
#define glUniform2iv                                LOOKUP_GL_FUNCTION(glUniform2iv)
#define glUniform3f                                 LOOKUP_GL_FUNCTION(glUniform3f)
#define glUniform3fv                                LOOKUP_GL_FUNCTION(glUniform3fv)
#define glUniform3i                                 LOOKUP_GL_FUNCTION(glUniform3i)
#define glUniform3iv                                LOOKUP_GL_FUNCTION(glUniform3iv)
#define glUniform4f                                 LOOKUP_GL_FUNCTION(glUniform4f)
#define glUniform4fv                                LOOKUP_GL_FUNCTION(glUniform4fv)
#define glUniform4i                                 LOOKUP_GL_FUNCTION(glUniform4i)
#define glUniform4iv                                LOOKUP_GL_FUNCTION(glUniform4iv)
#define glUniformMatrix2fv                          LOOKUP_GL_FUNCTION(glUniformMatrix2fv)
#define glUniformMatrix3fv                          LOOKUP_GL_FUNCTION(glUniformMatrix3fv)
#define glUniformMatrix4fv                          LOOKUP_GL_FUNCTION(glUniformMatrix4fv)
#define glUseProgram                                LOOKUP_GL_FUNCTION(glUseProgram)
#define glValidateProgram                           LOOKUP_GL_FUNCTION(glValidateProgram)
#define glVertexAttrib1f                            LOOKUP_GL_FUNCTION(glVertexAttrib1f)
#define glVertexAttrib1fv                           LOOKUP_GL_FUNCTION(glVertexAttrib1fv)
#define glVertexAttrib2f                            LOOKUP_GL_FUNCTION(glVertexAttrib2f)
#define glVertexAttrib2fv                           LOOKUP_GL_FUNCTION(glVertexAttrib2fv)
#define glVertexAttrib3f                            LOOKUP_GL_FUNCTION(glVertexAttrib3f)
#define glVertexAttrib3fv                           LOOKUP_GL_FUNCTION(glVertexAttrib3fv)
#define glVertexAttrib4f                            LOOKUP_GL_FUNCTION(glVertexAttrib4f)
#define glVertexAttrib4fv                           LOOKUP_GL_FUNCTION(glVertexAttrib4fv)
#define glVertexAttribPointer                       LOOKUP_GL_FUNCTION(glVertexAttribPointer)
#define glViewport                                  LOOKUP_GL_FUNCTION(glViewport)
#define glEvasGLImageTargetTexture2DOES             LOOKUP_GL_FUNCTION(glEvasGLImageTargetTexture2DOES)
#define glEvasGLImageTargetRenderbufferStorageOES   LOOKUP_GL_FUNCTION(glEvasGLImageTargetRenderbufferStorageOES)
#define GetProgramBinaryOES                         LOOKUP_GL_FUNCTION(GetProgramBinaryOES)
#define ProgramBinaryOES                            LOOKUP_GL_FUNCTION(ProgramBinaryOES)
#define MapBufferOES                                LOOKUP_GL_FUNCTION(MapBufferOES)
#define glUnmapBufferOES                            LOOKUP_GL_FUNCTION(glUnmapBufferOES)
#define GetBufferPointervOES                        LOOKUP_GL_FUNCTION(GetBufferPointervOES)
#define TexImage3DOES                               LOOKUP_GL_FUNCTION(TexImage3DOES)
#define TexSubImage3DOES                            LOOKUP_GL_FUNCTION(TexSubImage3DOES)
#define CopyTexSubImage3DOES                        LOOKUP_GL_FUNCTION(CopyTexSubImage3DOES)
#define CompressedTexImage3DOES                     LOOKUP_GL_FUNCTION(CompressedTexImage3DOES)
#define CompressedTexSubImage3DOES                  LOOKUP_GL_FUNCTION(CompressedTexSubImage3DOES)
#define FramebufferTexture3DOES                     LOOKUP_GL_FUNCTION(FramebufferTexture3DOES)
#define GetPerfMonitorGroupsAMD                     LOOKUP_GL_FUNCTION(GetPerfMonitorGroupsAMD)
#define GetPerfMonitorCountersAMD                   LOOKUP_GL_FUNCTION(GetPerfMonitorCountersAMD)
#define GetPerfMonitorGroupStringAMD                LOOKUP_GL_FUNCTION(GetPerfMonitorGroupStringAMD)
#define GetPerfMonitorCounterStringAMD              LOOKUP_GL_FUNCTION(GetPerfMonitorCounterStringAMD)
#define GetPerfMonitorCounterInfoAMD                LOOKUP_GL_FUNCTION(GetPerfMonitorCounterInfoAMD)
#define GenPerfMonitorsAMD                          LOOKUP_GL_FUNCTION(GenPerfMonitorsAMD)
#define DeletePerfMonitorsAMD                       LOOKUP_GL_FUNCTION(DeletePerfMonitorsAMD)
#define SelectPerfMonitorCountersAMD                LOOKUP_GL_FUNCTION(SelectPerfMonitorCountersAMD)
#define BeginPerfMonitorAMD                         LOOKUP_GL_FUNCTION(BeginPerfMonitorAMD)
#define EndPerfMonitorAMD                           LOOKUP_GL_FUNCTION(EndPerfMonitorAMD)
#define GetPerfMonitorCounterDataAMD                LOOKUP_GL_FUNCTION(GetPerfMonitorCounterDataAMD)
#define DiscardFramebufferEXT                       LOOKUP_GL_FUNCTION(DiscardFramebufferEXT)
#define MultiDrawArraysEXT                          LOOKUP_GL_FUNCTION(MultiDrawArraysEXT)
#define MultiDrawElementsEXT                        LOOKUP_GL_FUNCTION(MultiDrawElementsEXT)
#define DeleteFencesNV                              LOOKUP_GL_FUNCTION(DeleteFencesNV)
#define GenFencesNV                                 LOOKUP_GL_FUNCTION(GenFencesNV)
#define glIsFenceNV                                 LOOKUP_GL_FUNCTION(glIsFenceNV)
#define glTestFenceNV                               LOOKUP_GL_FUNCTION(glTestFenceNV)
#define GetFenceivNV                                LOOKUP_GL_FUNCTION(GetFenceivNV)
#define FinishFenceNV                               LOOKUP_GL_FUNCTION(FinishFenceNV)
#define SetFenceNV                                  LOOKUP_GL_FUNCTION(SetFenceNV)
#define GetDriverControlsQCOM                       LOOKUP_GL_FUNCTION(GetDriverControlsQCOM)
#define GetDriverControlStringQCOM                  LOOKUP_GL_FUNCTION(GetDriverControlStringQCOM)
#define EnableDriverControlQCOM                     LOOKUP_GL_FUNCTION(EnableDriverControlQCOM)
#define DisableDriverControlQCOM                    LOOKUP_GL_FUNCTION(DisableDriverControlQCOM)
#define ExtGetTexturesQCOM                          LOOKUP_GL_FUNCTION(ExtGetTexturesQCOM)
#define ExtGetBuffersQCOM                           LOOKUP_GL_FUNCTION(ExtGetBuffersQCOM)
#define ExtGetRenderbuffersQCOM                     LOOKUP_GL_FUNCTION(ExtGetRenderbuffersQCOM)
#define ExtGetFramebuffersQCOM                      LOOKUP_GL_FUNCTION(ExtGetFramebuffersQCOM)
#define ExtGetTexLevelParameterivQCOM               LOOKUP_GL_FUNCTION(ExtGetTexLevelParameterivQCOM)
#define ExtTexObjectStateOverrideiQCOM              LOOKUP_GL_FUNCTION(ExtTexObjectStateOverrideiQCOM)
#define ExtGetTexSubImageQCOM                       LOOKUP_GL_FUNCTION(ExtGetTexSubImageQCOM)
#define ExtGetBufferPointervQCOM                    LOOKUP_GL_FUNCTION(ExtGetBufferPointervQCOM)
#define ExtGetShadersQCOM                           LOOKUP_GL_FUNCTION(ExtGetShadersQCOM)
#define ExtGetProgramsQCOM                          LOOKUP_GL_FUNCTION(ExtGetProgramsQCOM)
#define glExtIsProgramBinaryQCOM                    LOOKUP_GL_FUNCTION(glExtIsProgramBinaryQCOM)
#define ExtGetProgramBinarySourceQCOM               LOOKUP_GL_FUNCTION(ExtGetProgramBinarySourceQCOM)
#define evasglCreateImage                           LOOKUP_GL_FUNCTION(evasglCreateImage)
#define evasglDestroyImage                          LOOKUP_GL_FUNCTION(evasglDestroyImage)
#define glEvasGLImageTargetTexture2DOES             LOOKUP_GL_FUNCTION(glEvasGLImageTargetTexture2DOES)
#else
typedef void (GLAPIENTRY *glActiveTextureType) (GLenum);
typedef void (GLAPIENTRY *glAttachShaderType) (GLuint, GLuint);
typedef void (GLAPIENTRY *glBindAttribLocationType) (GLuint, GLuint, const char*);
typedef void (GLAPIENTRY *glBindBufferType) (GLenum, GLuint);
typedef void (GLAPIENTRY *glBindFramebufferType) (GLenum, GLuint);
typedef void (GLAPIENTRY *glBindRenderbufferType) (GLenum, GLuint);
typedef void (GLAPIENTRY *glBindVertexArrayType) (GLuint);
typedef void (GLAPIENTRY *glBlendColorType) (GLclampf, GLclampf, GLclampf, GLclampf);
typedef void (GLAPIENTRY *glBlendEquationType) (GLenum);
typedef void (GLAPIENTRY *glBlendEquationSeparateType)(GLenum, GLenum);
typedef void (GLAPIENTRY *glBlendFuncSeparateType)(GLenum srcRGB, GLenum dstRGB, GLenum srcAlpha, GLenum dstAlpha);
typedef void (GLAPIENTRY *glBlitFramebufferType) (GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLint, GLbitfield, GLenum);
typedef void (GLAPIENTRY *glBufferDataType) (GLenum, GLsizeiptr, const GLvoid*, GLenum);
typedef void (GLAPIENTRY *glBufferSubDataType) (GLenum, GLintptr, GLsizeiptr, const GLvoid*);
typedef GLenum (GLAPIENTRY *glCheckFramebufferStatusType) (GLenum);
typedef void (GLAPIENTRY *glCompileShaderType) (GLuint);
typedef void (GLAPIENTRY *glCompressedTexImage2DType) (GLenum, GLint, GLenum, GLsizei, GLsizei, GLint, GLsizei, const GLvoid*);
typedef void (GLAPIENTRY *glCompressedTexSubImage2DType) (GLenum, GLint, GLint, GLint, GLsizei, GLsizei, GLenum, GLsizei, const GLvoid*);
typedef GLuint (GLAPIENTRY *glCreateProgramType) ();
typedef GLuint (GLAPIENTRY *glCreateShaderType) (GLenum);
typedef void (GLAPIENTRY *glDeleteBuffersType) (GLsizei, const GLuint*);
typedef void (GLAPIENTRY *glDeleteFramebuffersType) (GLsizei n, const GLuint*);
typedef void (GLAPIENTRY *glDeleteProgramType) (GLuint);
typedef void (GLAPIENTRY *glDeleteRenderbuffersType) (GLsizei n, const GLuint*);
typedef void (GLAPIENTRY *glDeleteShaderType) (GLuint);
typedef void (GLAPIENTRY *glDeleteVertexArraysType) (GLsizei, const GLuint*);
typedef void (GLAPIENTRY *glDetachShaderType) (GLuint, GLuint);
typedef void (GLAPIENTRY *glDisableVertexAttribArrayType) (GLuint);
typedef void (GLAPIENTRY *glEnableVertexAttribArrayType) (GLuint);
typedef void (GLAPIENTRY *glFramebufferRenderbufferType) (GLenum, GLenum, GLenum, GLuint);
typedef void (GLAPIENTRY *glFramebufferTexture2DType) (GLenum, GLenum, GLenum, GLuint, GLint);
typedef void (GLAPIENTRY *glGenBuffersType) (GLsizei, GLuint*);
typedef void (GLAPIENTRY *glGenerateMipmapType) (GLenum target);
typedef void (GLAPIENTRY *glGenFramebuffersType) (GLsizei, GLuint*);
typedef void (GLAPIENTRY *glGenRenderbuffersType) (GLsizei, GLuint*);
typedef void (GLAPIENTRY *glGenVertexArraysType) (GLsizei, GLuint*);
typedef void (GLAPIENTRY *glGetActiveAttribType) (GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*);
typedef void (GLAPIENTRY *glGetActiveUniformType) (GLuint, GLuint, GLsizei, GLsizei*, GLint*, GLenum*, GLchar*);
typedef void (GLAPIENTRY *glGetAttachedShadersType) (GLuint, GLsizei, GLsizei*, GLuint*);
typedef GLint (GLAPIENTRY *glGetAttribLocationType) (GLuint, const char*);
typedef void (GLAPIENTRY *glGetBufferParameterivType) (GLenum, GLenum, GLint*);
typedef void (GLAPIENTRY *glGetFramebufferAttachmentParameterivType) (GLenum, GLenum, GLenum, GLint* params);
typedef void (GLAPIENTRY *glGetProgramInfoLogType) (GLuint, GLsizei, GLsizei*, char*);
typedef void (GLAPIENTRY *glGetProgramivType) (GLuint, GLenum, GLint*);
typedef void (GLAPIENTRY *glGetRenderbufferParameterivType) (GLenum, GLenum, GLint*);
typedef void (GLAPIENTRY *glGetShaderInfoLogType) (GLuint, GLsizei, GLsizei*, char*);
typedef void (GLAPIENTRY *glGetShaderivType) (GLuint, GLenum, GLint*);
typedef void (GLAPIENTRY *glGetShaderSourceType) (GLuint, GLsizei, GLsizei*, char*);
typedef GLint (GLAPIENTRY *glGetUniformLocationType) (GLuint, const char*);
typedef void (GLAPIENTRY *glGetUniformfvType) (GLuint, GLint, GLfloat*);
typedef void (GLAPIENTRY *glGetUniformivType) (GLuint, GLint, GLint*);
typedef void (GLAPIENTRY *glGetVertexAttribfvType) (GLuint, GLenum, GLfloat*);
typedef void (GLAPIENTRY *glGetVertexAttribivType) (GLuint, GLenum, GLint*);
typedef void (GLAPIENTRY *glGetVertexAttribPointervType) (GLuint, GLenum, GLvoid**);
typedef GLboolean (GLAPIENTRY *glIsBufferType) (GLuint);
typedef GLboolean (GLAPIENTRY *glIsFramebufferType) (GLuint);
typedef GLboolean (GLAPIENTRY *glIsProgramType) (GLuint);
typedef GLboolean (GLAPIENTRY *glIsRenderbufferType) (GLuint);
typedef GLboolean (GLAPIENTRY *glIsShaderType) (GLuint);
typedef GLboolean (GLAPIENTRY *glIsVertexArrayType) (GLuint);
typedef void (GLAPIENTRY *glLinkProgramType) (GLuint);
typedef void (GLAPIENTRY *glRenderbufferStorageType) (GLenum, GLenum, GLsizei, GLsizei);
typedef void (GLAPIENTRY *glRenderbufferStorageMultisampleType) (GLenum, GLsizei, GLenum, GLsizei, GLsizei);
typedef void (GLAPIENTRY *glSampleCoverageType) (GLclampf, GLboolean);
typedef void (GLAPIENTRY *glShaderSourceType) (GLuint, GLsizei, const char**, const GLint*);
typedef void (GLAPIENTRY *glStencilFuncSeparateType) (GLenum, GLenum, GLint, GLuint);
typedef void (GLAPIENTRY *glStencilMaskSeparateType) (GLenum, GLuint);
typedef void (GLAPIENTRY *glStencilOpSeparateType) (GLenum, GLenum, GLenum, GLenum);
typedef void (GLAPIENTRY *glUniform1fType) (GLint, GLfloat);
typedef void (GLAPIENTRY *glUniform1fvType) (GLint, GLsizei, const GLfloat*);
typedef void (GLAPIENTRY *glUniform1iType) (GLint, GLint);
typedef void (GLAPIENTRY *glUniform1ivType) (GLint, GLsizei, const GLint*);
typedef void (GLAPIENTRY *glUniform2fType) (GLint, GLfloat, GLfloat);
typedef void (GLAPIENTRY *glUniform2fvType) (GLint, GLsizei, const GLfloat*);
typedef void (GLAPIENTRY *glUniform2iType) (GLint, GLint, GLint);
typedef void (GLAPIENTRY *glUniform2ivType) (GLint, GLsizei, const GLint*);
typedef void (GLAPIENTRY *glUniform3fType) (GLint, GLfloat, GLfloat, GLfloat);
typedef void (GLAPIENTRY *glUniform3fvType) (GLint, GLsizei, const GLfloat*);
typedef void (GLAPIENTRY *glUniform3iType) (GLint, GLint, GLint, GLint);
typedef void (GLAPIENTRY *glUniform3ivType) (GLint, GLsizei, const GLint*);
typedef void (GLAPIENTRY *glUniform4fType) (GLint, GLfloat, GLfloat, GLfloat, GLfloat);
typedef void (GLAPIENTRY *glUniform4fvType) (GLint, GLsizei, const GLfloat*);
typedef void (GLAPIENTRY *glUniform4iType) (GLint, GLint, GLint, GLint, GLint);
typedef void (GLAPIENTRY *glUniform4ivType) (GLint, GLsizei, const GLint*);
typedef void (GLAPIENTRY *glUniformMatrix2fvType) (GLint, GLsizei, GLboolean, const GLfloat*);
typedef void (GLAPIENTRY *glUniformMatrix3fvType) (GLint, GLsizei, GLboolean, const GLfloat*);
typedef void (GLAPIENTRY *glUniformMatrix4fvType) (GLint, GLsizei, GLboolean, const GLfloat*);
typedef void (GLAPIENTRY *glUseProgramType) (GLuint);
typedef void (GLAPIENTRY *glValidateProgramType) (GLuint);
typedef void (GLAPIENTRY *glVertexAttrib1fType) (GLuint, const GLfloat);
typedef void (GLAPIENTRY *glVertexAttrib1fvType) (GLuint, const GLfloat*);
typedef void (GLAPIENTRY *glVertexAttrib2fType) (GLuint, const GLfloat, const GLfloat);
typedef void (GLAPIENTRY *glVertexAttrib2fvType) (GLuint, const GLfloat*);
typedef void (GLAPIENTRY *glVertexAttrib3fType) (GLuint, const GLfloat, const GLfloat, const GLfloat);
typedef void (GLAPIENTRY *glVertexAttrib3fvType) (GLuint, const GLfloat*);
typedef void (GLAPIENTRY *glVertexAttrib4fType) (GLuint, const GLfloat, const GLfloat, const GLfloat, const GLfloat);
typedef void (GLAPIENTRY *glVertexAttrib4fvType) (GLuint, const GLfloat*);
typedef void (GLAPIENTRY *glVertexAttribPointerType) (GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid*);

#define FUNCTION_TABLE_ENTRY(FunctionName) FunctionName##Type FunctionName

typedef struct _OpenGLFunctionTable {
    FUNCTION_TABLE_ENTRY(glActiveTexture);
    FUNCTION_TABLE_ENTRY(glAttachShader);
    FUNCTION_TABLE_ENTRY(glBindAttribLocation);
    FUNCTION_TABLE_ENTRY(glBindBuffer);
    FUNCTION_TABLE_ENTRY(glBindFramebuffer);
    FUNCTION_TABLE_ENTRY(glBindRenderbuffer);
    FUNCTION_TABLE_ENTRY(glBindVertexArray);
    FUNCTION_TABLE_ENTRY(glBlendColor);
    FUNCTION_TABLE_ENTRY(glBlendEquation);
    FUNCTION_TABLE_ENTRY(glBlendEquationSeparate);
    FUNCTION_TABLE_ENTRY(glBlendFuncSeparate);
    FUNCTION_TABLE_ENTRY(glBlitFramebuffer);
    FUNCTION_TABLE_ENTRY(glBufferData);
    FUNCTION_TABLE_ENTRY(glBufferSubData);
    FUNCTION_TABLE_ENTRY(glCheckFramebufferStatus);
    FUNCTION_TABLE_ENTRY(glCompileShader);
    FUNCTION_TABLE_ENTRY(glCompressedTexImage2D);
    FUNCTION_TABLE_ENTRY(glCompressedTexSubImage2D);
    FUNCTION_TABLE_ENTRY(glCreateProgram);
    FUNCTION_TABLE_ENTRY(glCreateShader);
    FUNCTION_TABLE_ENTRY(glDeleteBuffers);
    FUNCTION_TABLE_ENTRY(glDeleteFramebuffers);
    FUNCTION_TABLE_ENTRY(glDeleteProgram);
    FUNCTION_TABLE_ENTRY(glDeleteRenderbuffers);
    FUNCTION_TABLE_ENTRY(glDeleteShader);
    FUNCTION_TABLE_ENTRY(glDeleteVertexArrays);
    FUNCTION_TABLE_ENTRY(glDetachShader);
    FUNCTION_TABLE_ENTRY(glDisableVertexAttribArray);
    FUNCTION_TABLE_ENTRY(glEnableVertexAttribArray);
    FUNCTION_TABLE_ENTRY(glFramebufferRenderbuffer);
    FUNCTION_TABLE_ENTRY(glFramebufferTexture2D);
    FUNCTION_TABLE_ENTRY(glGenBuffers);
    FUNCTION_TABLE_ENTRY(glGenerateMipmap);
    FUNCTION_TABLE_ENTRY(glGenFramebuffers);
    FUNCTION_TABLE_ENTRY(glGenRenderbuffers);
    FUNCTION_TABLE_ENTRY(glGenVertexArrays);
    FUNCTION_TABLE_ENTRY(glGetActiveAttrib);
    FUNCTION_TABLE_ENTRY(glGetActiveUniform);
    FUNCTION_TABLE_ENTRY(glGetAttachedShaders);
    FUNCTION_TABLE_ENTRY(glGetAttribLocation);
    FUNCTION_TABLE_ENTRY(glGetBufferParameteriv);
    FUNCTION_TABLE_ENTRY(glGetFramebufferAttachmentParameteriv);
    FUNCTION_TABLE_ENTRY(glGetProgramInfoLog);
    FUNCTION_TABLE_ENTRY(glGetProgramiv);
    FUNCTION_TABLE_ENTRY(glGetRenderbufferParameteriv);
    FUNCTION_TABLE_ENTRY(glGetShaderInfoLog);
    FUNCTION_TABLE_ENTRY(glGetShaderiv);
    FUNCTION_TABLE_ENTRY(glGetShaderSource);
    FUNCTION_TABLE_ENTRY(glGetUniformfv);
    FUNCTION_TABLE_ENTRY(glGetUniformiv);
    FUNCTION_TABLE_ENTRY(glGetUniformLocation);
    FUNCTION_TABLE_ENTRY(glGetVertexAttribfv);
    FUNCTION_TABLE_ENTRY(glGetVertexAttribiv);
    FUNCTION_TABLE_ENTRY(glGetVertexAttribPointerv);
    FUNCTION_TABLE_ENTRY(glIsBuffer);
    FUNCTION_TABLE_ENTRY(glIsFramebuffer);
    FUNCTION_TABLE_ENTRY(glIsProgram);
    FUNCTION_TABLE_ENTRY(glIsRenderbuffer);
    FUNCTION_TABLE_ENTRY(glIsShader);
    FUNCTION_TABLE_ENTRY(glIsVertexArray);
    FUNCTION_TABLE_ENTRY(glLinkProgram);
    FUNCTION_TABLE_ENTRY(glRenderbufferStorage);
    FUNCTION_TABLE_ENTRY(glRenderbufferStorageMultisample);
    FUNCTION_TABLE_ENTRY(glSampleCoverage);
    FUNCTION_TABLE_ENTRY(glShaderSource);
    FUNCTION_TABLE_ENTRY(glStencilFuncSeparate);
    FUNCTION_TABLE_ENTRY(glStencilMaskSeparate);
    FUNCTION_TABLE_ENTRY(glStencilOpSeparate);
    FUNCTION_TABLE_ENTRY(glUniform1f);
    FUNCTION_TABLE_ENTRY(glUniform1fv);
    FUNCTION_TABLE_ENTRY(glUniform1i);
    FUNCTION_TABLE_ENTRY(glUniform1iv);
    FUNCTION_TABLE_ENTRY(glUniform2f);
    FUNCTION_TABLE_ENTRY(glUniform2fv);
    FUNCTION_TABLE_ENTRY(glUniform2i);
    FUNCTION_TABLE_ENTRY(glUniform2iv);
    FUNCTION_TABLE_ENTRY(glUniform3f);
    FUNCTION_TABLE_ENTRY(glUniform3fv);
    FUNCTION_TABLE_ENTRY(glUniform3i);
    FUNCTION_TABLE_ENTRY(glUniform3iv);
    FUNCTION_TABLE_ENTRY(glUniform4f);
    FUNCTION_TABLE_ENTRY(glUniform4fv);
    FUNCTION_TABLE_ENTRY(glUniform4i);
    FUNCTION_TABLE_ENTRY(glUniform4iv);
    FUNCTION_TABLE_ENTRY(glUniformMatrix2fv);
    FUNCTION_TABLE_ENTRY(glUniformMatrix3fv);
    FUNCTION_TABLE_ENTRY(glUniformMatrix4fv);
    FUNCTION_TABLE_ENTRY(glUseProgram);
    FUNCTION_TABLE_ENTRY(glValidateProgram);
    FUNCTION_TABLE_ENTRY(glVertexAttrib1f);
    FUNCTION_TABLE_ENTRY(glVertexAttrib1fv);
    FUNCTION_TABLE_ENTRY(glVertexAttrib2f);
    FUNCTION_TABLE_ENTRY(glVertexAttrib2fv);
    FUNCTION_TABLE_ENTRY(glVertexAttrib3f);
    FUNCTION_TABLE_ENTRY(glVertexAttrib3fv);
    FUNCTION_TABLE_ENTRY(glVertexAttrib4f);
    FUNCTION_TABLE_ENTRY(glVertexAttrib4fv);
    FUNCTION_TABLE_ENTRY(glVertexAttribPointer);
} OpenGLFunctionTable;

// We disable the shims for OpenGLShims.cpp, so that we can set them.
#ifndef DISABLE_SHIMS
#define LOOKUP_GL_FUNCTION(Function) WebCore::openGLFunctionTable()->Function
#define glActiveTexture                        LOOKUP_GL_FUNCTION(glActiveTexture)
#define glAttachShader                         LOOKUP_GL_FUNCTION(glAttachShader)
#define glBindAttribLocation                   LOOKUP_GL_FUNCTION(glBindAttribLocation)
#define glBindBuffer                           LOOKUP_GL_FUNCTION(glBindBuffer)
#define glBindFramebufferEXT                   glBindFramebuffer
#define glBindFramebuffer                      LOOKUP_GL_FUNCTION(glBindFramebuffer)
#define glBindRenderbufferEXT                  glBindRenderbuffer
#define glBindRenderbuffer                     LOOKUP_GL_FUNCTION(glBindRenderbuffer)
#define glBindVertexArrayOES                   glBindVertexArray
#define glBindVertexArray                      LOOKUP_GL_FUNCTION(glBindVertexArray)
#define glBlendColor                           LOOKUP_GL_FUNCTION(glBlendColor)
#define glBlendEquation                        LOOKUP_GL_FUNCTION(glBlendEquation)
#define glBlendEquationSeparate                LOOKUP_GL_FUNCTION(glBlendEquationSeparate)
#define glBlendFuncSeparate                    LOOKUP_GL_FUNCTION(glBlendFuncSeparate)
#define glBlitFramebufferEXT                   glBlitFramebuffer
#define glBlitFramebuffer                      LOOKUP_GL_FUNCTION(glBlitFramebuffer)
#define glBufferData                           LOOKUP_GL_FUNCTION(glBufferData)
#define glBufferSubData                        LOOKUP_GL_FUNCTION(glBufferSubData)
#define glCheckFramebufferStatusEXT            glCheckFramebufferStatus
#define glCheckFramebufferStatus               LOOKUP_GL_FUNCTION(glCheckFramebufferStatus)
#define glCompileShader                        LOOKUP_GL_FUNCTION(glCompileShader)
#define glCompressedTexImage2D                 LOOKUP_GL_FUNCTION(glCompressedTexImage2D)
#define glCompressedTexSubImage2D              LOOKUP_GL_FUNCTION(glCompressedTexSubImage2D)
#define glCreateProgram                        LOOKUP_GL_FUNCTION(glCreateProgram)
#define glCreateShader                         LOOKUP_GL_FUNCTION(glCreateShader)
#define glDeleteBuffers                        LOOKUP_GL_FUNCTION(glDeleteBuffers)
#define glDeleteFramebuffersEXT                glDeleteFramebuffers
#define glDeleteFramebuffers                   LOOKUP_GL_FUNCTION(glDeleteFramebuffers)
#define glDeleteProgram                        LOOKUP_GL_FUNCTION(glDeleteProgram)
#define glDeleteRenderbuffersEXT               glDeleteRenderbuffers
#define glDeleteRenderbuffers                  LOOKUP_GL_FUNCTION(glDeleteRenderbuffers)
#define glDeleteShader                         LOOKUP_GL_FUNCTION(glDeleteShader)
#define glDeleteVertexArraysOES                glDeleteVertexArrays
#define glDeleteVertexArrays                   LOOKUP_GL_FUNCTION(glDeleteVertexArrays)
#define glDetachShader                         LOOKUP_GL_FUNCTION(glDetachShader)
#define glDisableVertexAttribArray             LOOKUP_GL_FUNCTION(glDisableVertexAttribArray)
#define glEnableVertexAttribArray              LOOKUP_GL_FUNCTION(glEnableVertexAttribArray)
#define glFramebufferRenderbufferEXT           glFramebufferRenderbuffer
#define glFramebufferRenderbuffer              LOOKUP_GL_FUNCTION(glFramebufferRenderbuffer)
#define glFramebufferTexture2DEXT              glFramebufferTexture2D
#define glFramebufferTexture2D                 LOOKUP_GL_FUNCTION(glFramebufferTexture2D)
#define glGenBuffers                           LOOKUP_GL_FUNCTION(glGenBuffers)
#define glGenerateMipmapEXT                    glGenerateMipmap
#define glGenerateMipmap                       LOOKUP_GL_FUNCTION(glGenerateMipmap)
#define glGenFramebuffersEXT                   glGenFramebuffers
#define glGenFramebuffers                      LOOKUP_GL_FUNCTION(glGenFramebuffers)
#define glGenRenderbuffersEXT                  glGenRenderbuffers
#define glGenRenderbuffers                     LOOKUP_GL_FUNCTION(glGenRenderbuffers)
#define glGenVertexArraysOES                   glGenVertexArrays;
#define glGenVertexArrays                      LOOKUP_GL_FUNCTION(glGenVertexArrays)
#define glGetActiveAttrib                      LOOKUP_GL_FUNCTION(glGetActiveAttrib)
#define glGetActiveUniform                     LOOKUP_GL_FUNCTION(glGetActiveUniform)
#define glGetAttachedShaders                   LOOKUP_GL_FUNCTION(glGetAttachedShaders)
#define glGetAttribLocation                    LOOKUP_GL_FUNCTION(glGetAttribLocation)
#define glGetBufferParameterivEXT              glGetBufferParameteriv
#define glGetBufferParameteriv                 LOOKUP_GL_FUNCTION(glGetBufferParameteriv)
#define glGetFramebufferAttachmentParameterivEXT glGetFramebufferAttachmentParameteriv
#define glGetFramebufferAttachmentParameteriv  LOOKUP_GL_FUNCTION(glGetFramebufferAttachmentParameteriv)
#define glGetProgramInfoLog                    LOOKUP_GL_FUNCTION(glGetProgramInfoLog)
#define glGetProgramiv                         LOOKUP_GL_FUNCTION(glGetProgramiv)
#define glGetRenderbufferParameterivEXT        glGetRenderbufferParameteriv
#define glGetRenderbufferParameteriv           LOOKUP_GL_FUNCTION(glGetRenderbufferParameteriv)
#define glGetShaderInfoLog                     LOOKUP_GL_FUNCTION(glGetShaderInfoLog)
#define glGetShaderiv                          LOOKUP_GL_FUNCTION(glGetShaderiv)
#define glGetShaderSource                      LOOKUP_GL_FUNCTION(glGetShaderSource)
#define glGetUniformfv                         LOOKUP_GL_FUNCTION(glGetUniformfv)
#define glGetUniformiv                         LOOKUP_GL_FUNCTION(glGetUniformiv)
#define glGetUniformLocation                   LOOKUP_GL_FUNCTION(glGetUniformLocation)
#define glGetVertexAttribfv                    LOOKUP_GL_FUNCTION(glGetVertexAttribfv)
#define glGetVertexAttribiv                    LOOKUP_GL_FUNCTION(glGetVertexAttribiv)
#define glGetVertexAttribPointerv              LOOKUP_GL_FUNCTION(glGetVertexAttribPointerv)
#define glIsBuffer                             LOOKUP_GL_FUNCTION(glIsBuffer)
#define glIsFramebufferEXT                     glIsFramebuffer
#define glIsFramebuffer                        LOOKUP_GL_FUNCTION(glIsFramebuffer)
#define glIsProgram                            LOOKUP_GL_FUNCTION(glIsProgram)
#define glIsRenderbufferEXT                    glIsRenderbuffer
#define glIsRenderbuffer                       LOOKUP_GL_FUNCTION(glIsRenderbuffer)
#define glIsShader                             LOOKUP_GL_FUNCTION(glIsShader)
#define glIsVertexArrayOES                     glIsVertexArray;
#define glIsVertexArray                        LOOKUP_GL_FUNCTION(glIsVertexArray)
#define glLinkProgram                          LOOKUP_GL_FUNCTION(glLinkProgram)
#define glRenderbufferStorageEXT               glRenderbufferStorage
#define glRenderbufferStorage                  LOOKUP_GL_FUNCTION(glRenderbufferStorage)
#define glRenderbufferStorageMultisampleEXT    glRenderbufferStorageMultisample
#define glRenderbufferStorageMultisample       LOOKUP_GL_FUNCTION(glRenderbufferStorageMultisample)
#define glSampleCoverage                       LOOKUP_GL_FUNCTION(glSampleCoverage)
#define glShaderSource                         LOOKUP_GL_FUNCTION(glShaderSource)
#define glStencilFuncSeparate                  LOOKUP_GL_FUNCTION(glStencilFuncSeparate)
#define glStencilMaskSeparate                  LOOKUP_GL_FUNCTION(glStencilMaskSeparate)
#define glStencilOpSeparate                    LOOKUP_GL_FUNCTION(glStencilOpSeparate)
#define glUniform1f                            LOOKUP_GL_FUNCTION(glUniform1f)
#define glUniform1fv                           LOOKUP_GL_FUNCTION(glUniform1fv)
#define glUniform1i                            LOOKUP_GL_FUNCTION(glUniform1i)
#define glUniform1iv                           LOOKUP_GL_FUNCTION(glUniform1iv)
#define glUniform2f                            LOOKUP_GL_FUNCTION(glUniform2f)
#define glUniform2fv                           LOOKUP_GL_FUNCTION(glUniform2fv)
#define glUniform2i                            LOOKUP_GL_FUNCTION(glUniform2i)
#define glUniform2iv                           LOOKUP_GL_FUNCTION(glUniform2iv)
#define glUniform3f                            LOOKUP_GL_FUNCTION(glUniform3f)
#define glUniform3fv                           LOOKUP_GL_FUNCTION(glUniform3fv)
#define glUniform3i                            LOOKUP_GL_FUNCTION(glUniform3i)
#define glUniform3iv                           LOOKUP_GL_FUNCTION(glUniform3iv)
#define glUniform4f                            LOOKUP_GL_FUNCTION(glUniform4f)
#define glUniform4fv                           LOOKUP_GL_FUNCTION(glUniform4fv)
#define glUniform4i                            LOOKUP_GL_FUNCTION(glUniform4i)
#define glUniform4iv                           LOOKUP_GL_FUNCTION(glUniform4iv)
#define glUniformMatrix2fv                     LOOKUP_GL_FUNCTION(glUniformMatrix2fv)
#define glUniformMatrix3fv                     LOOKUP_GL_FUNCTION(glUniformMatrix3fv)
#define glUniformMatrix4fv                     LOOKUP_GL_FUNCTION(glUniformMatrix4fv)
#define glUseProgram                           LOOKUP_GL_FUNCTION(glUseProgram)
#define glValidateProgram                      LOOKUP_GL_FUNCTION(glValidateProgram)
#define glVertexAttrib1f                       LOOKUP_GL_FUNCTION(glVertexAttrib1f)
#define glVertexAttrib1fv                      LOOKUP_GL_FUNCTION(glVertexAttrib1fv)
#define glVertexAttrib2f                       LOOKUP_GL_FUNCTION(glVertexAttrib2f)
#define glVertexAttrib2fv                      LOOKUP_GL_FUNCTION(glVertexAttrib2fv)
#define glVertexAttrib3f                       LOOKUP_GL_FUNCTION(glVertexAttrib3f)
#define glVertexAttrib3fv                      LOOKUP_GL_FUNCTION(glVertexAttrib3fv)
#define glVertexAttrib4f                       LOOKUP_GL_FUNCTION(glVertexAttrib4f)
#define glVertexAttrib4fv                      LOOKUP_GL_FUNCTION(glVertexAttrib4fv)
#define glVertexAttribPointer                  LOOKUP_GL_FUNCTION(glVertexAttribPointer)
#endif
#endif // PLATFORM(TIZEN)

#endif
