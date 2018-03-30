/*******************************************************************************
*  Copyright (c) 1998 MFC Forum
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* Module Name:
*
* Revision History:
*
* Date        Ver Name                    Description
* ----------  --- --------------------- -----------------------------------------
* 07-Jun-2016 0.1 Youngho Kim             Created
* ----------  --- --------------------- -----------------------------------------
*
* DESCRIPTION:
*
*  $Author:
*  $LastChangedBy:
*  $Date:
*  $Revision: 2949 $
*  $Id:
*  $HeadURL:
*******************************************************************************/
#ifndef ROXLU_YUV420P_PLAYER_H
#define ROXLU_YUV420P_PLAYER_H

#if defined(USE_GLFW_LIB)

 // reference
 // https://medium.com/@Plimsky/how-to-install-a-opengl-environment-on-ubuntu-e3918cf5ab6c
 // https://nervous.io/ffmpeg/opengl/2017/01/31/ffmpeg-opengl/
 #ifdef __APPLE__
#include <OpenGL/gl3.h>
#else
#include <GL/glew.h>
#endif

#include <tinylib.h>

static const char* YUV420P_VS = ""
  "#version 330\n"
  ""
  "uniform mat4 u_pm;"
  "uniform vec4 draw_pos;"
  ""
  "const vec2 verts[4] = vec2[] ("
  "  vec2(-0.5,  0.5), "
  "  vec2(-0.5, -0.5), "
  "  vec2( 0.5,  0.5), "
  "  vec2( 0.5, -0.5)  "
  ");"
  ""
  "const vec2 texcoords[4] = vec2[] ("
  "  vec2(0.0, 1.0), "
  "  vec2(0.0, 0.0), "
  "  vec2(1.0, 1.0), "
  "  vec2(1.0, 0.0)  "
  "); "
  ""
  "out vec2 v_coord; "
  ""
  "void main() {"
  "   vec2 vert = verts[gl_VertexID];"
  "   vec4 p = vec4((0.5 * draw_pos.z) + draw_pos.x + (vert.x * draw_pos.z), "
  "                 (0.5 * draw_pos.w) + draw_pos.y + (vert.y * draw_pos.w), "
  "                 0, 1);"
  "   gl_Position = u_pm * p;"
  "   v_coord = texcoords[gl_VertexID];"
  "}"
  "";

static const char* YUV420P_FS = ""
 "#version 330\n"
  "uniform sampler2D y_tex;"
  "uniform sampler2D u_tex;"
  "uniform sampler2D v_tex;"
  "in vec2 v_coord;"
  "layout( location = 0 ) out vec4 fragcolor;"
  ""
  "const vec3 R_cf = vec3(1.164383,  0.000000,  1.596027);"
  "const vec3 G_cf = vec3(1.164383, -0.391762, -0.812968);"
  "const vec3 B_cf = vec3(1.164383,  2.017232,  0.000000);"
  "const vec3 offset = vec3(-0.0625, -0.5, -0.5);"
  ""
  "void main() {"
  "  float y = texture(y_tex, v_coord).r;"
  "  float u = texture(u_tex, v_coord).r;"
  "  float v = texture(v_tex, v_coord).r;"
  "  vec3 yuv = vec3(y,u,v);"
  "  yuv += offset;"
  "  fragcolor = vec4(0.0, 0.0, 0.0, 1.0);"
  "  fragcolor.r = dot(yuv, R_cf);"
  "  fragcolor.g = dot(yuv, G_cf);"
  "  fragcolor.b = dot(yuv, B_cf);"
  "}"
  "";

class YUV420P_Player {

 public:
  YUV420P_Player();
  bool setup(int w, int h);
  void setYPixels(uint8_t* pixels, int stride);
  void setUPixels(uint8_t* pixels, int stride);
  void setVPixels(uint8_t* pixels, int stride);
  void draw(int x, int y, int w = 0, int h = 0);
  void resize(int winW, int winH);

 private:
  bool setupTextures();
  bool setupShader();

 public:
  int vid_w;
  int vid_h;
  int win_w;
  int win_h;
  GLuint vao;
  GLuint y_tex;
  GLuint u_tex;
  GLuint v_tex;
  GLuint vert;
  GLuint frag;
  GLuint prog;
  GLint u_pos;
  bool textures_created;
  bool shader_created;
  uint8_t* y_pixels;
  uint8_t* u_pixels;
  uint8_t* v_pixels;
  mat4 pm;
};
#endif // USE_GLFW_LIB
#endif