#pragma once

#include <SDL3/SDL_opengl.h>

typedef void (APIENTRYP GloryGlDrawArraysProc)(GLenum mode, GLint first, GLsizei count);
typedef void (APIENTRYP GloryGlViewportProc)(GLint x, GLint y, GLsizei width, GLsizei height);
typedef void (APIENTRYP GloryGlClearColorProc)(
    GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
typedef void (APIENTRYP GloryGlClearProc)(GLbitfield mask);
typedef void (APIENTRYP GloryGlEnableProc)(GLenum capability);
typedef void (APIENTRYP GloryGlActiveTextureProc)(GLenum texture);
typedef void (APIENTRYP GloryGlDrawElementsProc)(GLenum mode, GLsizei count, GLenum type, const void* indices);
typedef void (APIENTRYP GloryGlGenTexturesProc)(GLsizei count, GLuint* textures);
typedef void (APIENTRYP GloryGlBindTextureProc)(GLenum target, GLuint texture);
typedef void (APIENTRYP GloryGlTexImage2DProc)(GLenum target, GLint level, GLint internalFormat, GLsizei width, GLsizei height, GLint border, GLenum format, GLenum type, const void* pixels);
typedef void (APIENTRYP GloryGlTexParameteriProc)(GLenum target, GLenum name, GLint value);
typedef void (APIENTRYP GloryGlDeleteTexturesProc)(GLsizei count, const GLuint* textures);
typedef void (APIENTRYP GloryGlGenerateMipmapProc)(GLenum target);
typedef void (APIENTRYP GloryGlUniform1fProc)(GLint location, GLfloat value);
typedef void (APIENTRYP GloryGlUniform3fProc)(GLint location, GLfloat x, GLfloat y, GLfloat z);
typedef void (APIENTRYP GloryGlVertexAttribIPointerProc)(GLuint index, GLint size, GLenum type, GLsizei stride, const void* pointer);
typedef void (APIENTRYP GloryGlGenFramebuffersProc)(GLsizei count, GLuint* framebuffers);
typedef void (APIENTRYP GloryGlBindFramebufferProc)(GLenum target, GLuint framebuffer);
typedef void (APIENTRYP GloryGlFramebufferTexture2DProc)(GLenum target, GLenum attachment, GLenum textarget, GLuint texture, GLint level);
typedef void (APIENTRYP GloryGlDeleteFramebuffersProc)(GLsizei count, const GLuint* framebuffers);
typedef GLenum (APIENTRYP GloryGlCheckFramebufferStatusProc)(GLenum target);
typedef void (APIENTRYP GloryGlDrawBufferProc)(GLenum buffer);
typedef void (APIENTRYP GloryGlReadBufferProc)(GLenum buffer);
typedef void (APIENTRYP GloryGlTexParameterfvProc)(GLenum target, GLenum name, const GLfloat* values);
typedef void (APIENTRYP GloryGlDepthFuncProc)(GLenum function);
typedef void (APIENTRYP GloryGlBlendFuncProc)(GLenum source, GLenum destination);
typedef void (APIENTRYP GloryGlDepthMaskProc)(GLboolean enabled);
typedef void (APIENTRYP GloryGlGenQueriesProc)(GLsizei count, GLuint* queries);
typedef void (APIENTRYP GloryGlDeleteQueriesProc)(GLsizei count, const GLuint* queries);
typedef void (APIENTRYP GloryGlBeginQueryProc)(GLenum target, GLuint query);
typedef void (APIENTRYP GloryGlEndQueryProc)(GLenum target);
typedef void (APIENTRYP GloryGlQueryCounterProc)(GLuint query, GLenum target);
typedef void (APIENTRYP GloryGlGetQueryObjectivProc)(GLuint query, GLenum name, GLint* value);
typedef void (APIENTRYP GloryGlGetQueryObjectui64vProc)(GLuint query, GLenum name, GLuint64* value);
typedef const GLubyte* (APIENTRYP GloryGlGetStringProc)(GLenum name);

namespace glory::engine {

class GlApi {
public:
    GlApi();

    PFNGLCREATESHADERPROC createShader = nullptr;
    PFNGLSHADERSOURCEPROC shaderSource = nullptr;
    PFNGLCOMPILESHADERPROC compileShader = nullptr;
    PFNGLGETSHADERIVPROC getShaderiv = nullptr;
    PFNGLGETSHADERINFOLOGPROC getShaderInfoLog = nullptr;
    PFNGLDELETESHADERPROC deleteShader = nullptr;
    PFNGLCREATEPROGRAMPROC createProgram = nullptr;
    PFNGLATTACHSHADERPROC attachShader = nullptr;
    PFNGLLINKPROGRAMPROC linkProgram = nullptr;
    PFNGLGETPROGRAMIVPROC getProgramiv = nullptr;
    PFNGLGETPROGRAMINFOLOGPROC getProgramInfoLog = nullptr;
    PFNGLDELETEPROGRAMPROC deleteProgram = nullptr;
    PFNGLUSEPROGRAMPROC useProgram = nullptr;
    PFNGLGETUNIFORMLOCATIONPROC getUniformLocation = nullptr;
    PFNGLUNIFORMMATRIX4FVPROC uniformMatrix4fv = nullptr;
    PFNGLGENVERTEXARRAYSPROC genVertexArrays = nullptr;
    PFNGLBINDVERTEXARRAYPROC bindVertexArray = nullptr;
    PFNGLDELETEVERTEXARRAYSPROC deleteVertexArrays = nullptr;
    PFNGLGENBUFFERSPROC genBuffers = nullptr;
    PFNGLBINDBUFFERPROC bindBuffer = nullptr;
    PFNGLBUFFERDATAPROC bufferData = nullptr;
    PFNGLDELETEBUFFERSPROC deleteBuffers = nullptr;
    PFNGLENABLEVERTEXATTRIBARRAYPROC enableVertexAttribArray = nullptr;
    PFNGLVERTEXATTRIBPOINTERPROC vertexAttribPointer = nullptr;
    GloryGlVertexAttribIPointerProc vertexAttribIPointer = nullptr;
    GloryGlDrawArraysProc drawArrays = nullptr;
    GloryGlDrawElementsProc drawElements = nullptr;
    GloryGlGenTexturesProc genTextures = nullptr;
    GloryGlBindTextureProc bindTexture = nullptr;
    GloryGlTexImage2DProc texImage2D = nullptr;
    GloryGlTexParameteriProc texParameteri = nullptr;
    GloryGlGenerateMipmapProc generateMipmap = nullptr;
    GloryGlDeleteTexturesProc deleteTextures = nullptr;
    GloryGlTexParameterfvProc texParameterfv = nullptr;
    GloryGlGenFramebuffersProc genFramebuffers = nullptr;
    GloryGlBindFramebufferProc bindFramebuffer = nullptr;
    GloryGlFramebufferTexture2DProc framebufferTexture2D = nullptr;
    GloryGlDeleteFramebuffersProc deleteFramebuffers = nullptr;
    GloryGlCheckFramebufferStatusProc checkFramebufferStatus = nullptr;
    GloryGlDrawBufferProc drawBuffer = nullptr;
    GloryGlReadBufferProc readBuffer = nullptr;
    GloryGlActiveTextureProc activeTexture = nullptr;
    PFNGLUNIFORM1IPROC uniform1i = nullptr;
    GloryGlUniform1fProc uniform1f = nullptr;
    GloryGlUniform3fProc uniform3f = nullptr;
    GloryGlViewportProc viewport = nullptr;
    GloryGlClearColorProc clearColor = nullptr;
    GloryGlClearProc clear = nullptr;
    GloryGlEnableProc enable = nullptr;
    GloryGlDepthFuncProc depthFunc = nullptr;
    GloryGlBlendFuncProc blendFunc = nullptr;
    GloryGlDepthMaskProc depthMask = nullptr;
    GloryGlEnableProc disable = nullptr;
    GloryGlGenQueriesProc genQueries = nullptr;
    GloryGlDeleteQueriesProc deleteQueries = nullptr;
    GloryGlBeginQueryProc beginQuery = nullptr;
    GloryGlEndQueryProc endQuery = nullptr;
    GloryGlQueryCounterProc queryCounter = nullptr;
    GloryGlGetQueryObjectivProc getQueryObjectiv = nullptr;
    GloryGlGetQueryObjectui64vProc getQueryObjectui64v = nullptr;
    GloryGlGetStringProc getString = nullptr;
    bool timerQueriesAvailable = false;
};

} // namespace glory::engine
