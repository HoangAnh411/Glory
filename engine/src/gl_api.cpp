#include "glory/engine/gl_api.hpp"

#include <SDL3/SDL_video.h>

#include <stdexcept>
#include <string>

namespace glory::engine {
namespace {

template <typename Procedure>
Procedure loadProcedure(const char* name) {
    const SDL_FunctionPointer address = SDL_GL_GetProcAddress(name);
    if (address == nullptr) {
        throw std::runtime_error(std::string("OpenGL procedure unavailable: ") + name);
    }
    return reinterpret_cast<Procedure>(address);
}

} // namespace

GlApi::GlApi() {
    createShader = loadProcedure<PFNGLCREATESHADERPROC>("glCreateShader");
    shaderSource = loadProcedure<PFNGLSHADERSOURCEPROC>("glShaderSource");
    compileShader = loadProcedure<PFNGLCOMPILESHADERPROC>("glCompileShader");
    getShaderiv = loadProcedure<PFNGLGETSHADERIVPROC>("glGetShaderiv");
    getShaderInfoLog = loadProcedure<PFNGLGETSHADERINFOLOGPROC>("glGetShaderInfoLog");
    deleteShader = loadProcedure<PFNGLDELETESHADERPROC>("glDeleteShader");
    createProgram = loadProcedure<PFNGLCREATEPROGRAMPROC>("glCreateProgram");
    attachShader = loadProcedure<PFNGLATTACHSHADERPROC>("glAttachShader");
    linkProgram = loadProcedure<PFNGLLINKPROGRAMPROC>("glLinkProgram");
    getProgramiv = loadProcedure<PFNGLGETPROGRAMIVPROC>("glGetProgramiv");
    getProgramInfoLog = loadProcedure<PFNGLGETPROGRAMINFOLOGPROC>("glGetProgramInfoLog");
    deleteProgram = loadProcedure<PFNGLDELETEPROGRAMPROC>("glDeleteProgram");
    useProgram = loadProcedure<PFNGLUSEPROGRAMPROC>("glUseProgram");
    getUniformLocation = loadProcedure<PFNGLGETUNIFORMLOCATIONPROC>("glGetUniformLocation");
    uniformMatrix4fv = loadProcedure<PFNGLUNIFORMMATRIX4FVPROC>("glUniformMatrix4fv");
    uniform1f = loadProcedure<GloryGlUniform1fProc>("glUniform1f");
    uniform3f = loadProcedure<GloryGlUniform3fProc>("glUniform3f");
    uniform4f = loadProcedure<GloryGlUniform4fProc>("glUniform4f");
    genVertexArrays = loadProcedure<PFNGLGENVERTEXARRAYSPROC>("glGenVertexArrays");
    bindVertexArray = loadProcedure<PFNGLBINDVERTEXARRAYPROC>("glBindVertexArray");
    deleteVertexArrays = loadProcedure<PFNGLDELETEVERTEXARRAYSPROC>("glDeleteVertexArrays");
    genBuffers = loadProcedure<PFNGLGENBUFFERSPROC>("glGenBuffers");
    bindBuffer = loadProcedure<PFNGLBINDBUFFERPROC>("glBindBuffer");
    bufferData = loadProcedure<PFNGLBUFFERDATAPROC>("glBufferData");
    deleteBuffers = loadProcedure<PFNGLDELETEBUFFERSPROC>("glDeleteBuffers");
    enableVertexAttribArray =
        loadProcedure<PFNGLENABLEVERTEXATTRIBARRAYPROC>("glEnableVertexAttribArray");
    vertexAttribPointer = loadProcedure<PFNGLVERTEXATTRIBPOINTERPROC>("glVertexAttribPointer");
    vertexAttribIPointer = loadProcedure<GloryGlVertexAttribIPointerProc>("glVertexAttribIPointer");
    drawArrays = loadProcedure<GloryGlDrawArraysProc>("glDrawArrays");
    drawElements = loadProcedure<GloryGlDrawElementsProc>("glDrawElements");
    genTextures = loadProcedure<GloryGlGenTexturesProc>("glGenTextures");
    bindTexture = loadProcedure<GloryGlBindTextureProc>("glBindTexture");
    texImage2D = loadProcedure<GloryGlTexImage2DProc>("glTexImage2D");
    texParameteri = loadProcedure<GloryGlTexParameteriProc>("glTexParameteri");
    generateMipmap = loadProcedure<GloryGlGenerateMipmapProc>("glGenerateMipmap");
    deleteTextures = loadProcedure<GloryGlDeleteTexturesProc>("glDeleteTextures");
    texParameterfv = loadProcedure<GloryGlTexParameterfvProc>("glTexParameterfv");
    genFramebuffers = loadProcedure<GloryGlGenFramebuffersProc>("glGenFramebuffers");
    bindFramebuffer = loadProcedure<GloryGlBindFramebufferProc>("glBindFramebuffer");
    framebufferTexture2D = loadProcedure<GloryGlFramebufferTexture2DProc>("glFramebufferTexture2D");
    deleteFramebuffers = loadProcedure<GloryGlDeleteFramebuffersProc>("glDeleteFramebuffers");
    checkFramebufferStatus = loadProcedure<GloryGlCheckFramebufferStatusProc>("glCheckFramebufferStatus");
    drawBuffer = loadProcedure<GloryGlDrawBufferProc>("glDrawBuffer");
    readBuffer = loadProcedure<GloryGlReadBufferProc>("glReadBuffer");
    activeTexture = loadProcedure<GloryGlActiveTextureProc>("glActiveTexture");
    uniform1i = loadProcedure<PFNGLUNIFORM1IPROC>("glUniform1i");
    viewport = loadProcedure<GloryGlViewportProc>("glViewport");
    clearColor = loadProcedure<GloryGlClearColorProc>("glClearColor");
    clear = loadProcedure<GloryGlClearProc>("glClear");
    enable = loadProcedure<GloryGlEnableProc>("glEnable");
    depthFunc = loadProcedure<GloryGlDepthFuncProc>("glDepthFunc");
    blendFunc = loadProcedure<GloryGlBlendFuncProc>("glBlendFunc");
    depthMask = loadProcedure<GloryGlDepthMaskProc>("glDepthMask");
    disable = loadProcedure<GloryGlEnableProc>("glDisable");
    genQueries = reinterpret_cast<GloryGlGenQueriesProc>(SDL_GL_GetProcAddress("glGenQueries"));
    deleteQueries = reinterpret_cast<GloryGlDeleteQueriesProc>(SDL_GL_GetProcAddress("glDeleteQueries"));
    beginQuery = reinterpret_cast<GloryGlBeginQueryProc>(SDL_GL_GetProcAddress("glBeginQuery"));
    endQuery = reinterpret_cast<GloryGlEndQueryProc>(SDL_GL_GetProcAddress("glEndQuery"));
    queryCounter = reinterpret_cast<GloryGlQueryCounterProc>(SDL_GL_GetProcAddress("glQueryCounter"));
    getQueryObjectiv = reinterpret_cast<GloryGlGetQueryObjectivProc>(SDL_GL_GetProcAddress("glGetQueryObjectiv"));
    getQueryObjectui64v = reinterpret_cast<GloryGlGetQueryObjectui64vProc>(SDL_GL_GetProcAddress("glGetQueryObjectui64v"));
    timerQueriesAvailable = genQueries && deleteQueries && beginQuery && endQuery &&
        queryCounter && getQueryObjectiv && getQueryObjectui64v;
    getString = loadProcedure<GloryGlGetStringProc>("glGetString");
}

} // namespace glory::engine
