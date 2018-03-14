#include "YUV420P_Player.h"

#if defined(USE_GLFW_LIB)

YUV420P_Player::YUV420P_Player()
  :vid_w(0)
  ,vid_h(0)
  ,win_w(0)
  ,win_h(0)
  ,vao(0)
  ,y_tex(0)
  ,u_tex(0)
  ,v_tex(0)
  ,vert(0)
  ,frag(0)
  ,prog(0)
  ,u_pos(-1)
  ,textures_created(false)
  ,shader_created(false)
  ,y_pixels(NULL)
  ,u_pixels(NULL)
  ,v_pixels(NULL)
{
}

bool YUV420P_Player::setup(int vidW, int vidH) {

  vid_w = vidW;
  vid_h = vidH;

  if(!vid_w || !vid_h) {
    printf("Invalid texture size.\n");
    return false;
  }

  if(y_pixels || u_pixels || v_pixels) {
    printf("Already setup the YUV420P_Player.\n");
    return false;
  }

  y_pixels = new uint8_t[vid_w * vid_h];
  u_pixels = new uint8_t[int((vid_w * 0.5) * (vid_h * 0.5))];
  v_pixels = new uint8_t[int((vid_w * 0.5) * (vid_h * 0.5))];

  if(!setupTextures()) {
    return false;
  }

  if(!setupShader()) {
    return false;
  }

  glGenVertexArrays(1, &vao);

  return true;
}

bool YUV420P_Player::setupShader() {

  if(shader_created) {
    printf("Already creatd the shader.\n");
    return false;
  }

  vert = rx_create_shader(GL_VERTEX_SHADER, YUV420P_VS);
  frag = rx_create_shader(GL_FRAGMENT_SHADER, YUV420P_FS);
  prog = rx_create_program(vert, frag);

  glLinkProgram(prog);
  rx_print_shader_link_info(prog);

  glUseProgram(prog);
  glUniform1i(glGetUniformLocation(prog, "y_tex"), 0);
  glUniform1i(glGetUniformLocation(prog, "u_tex"), 1);
  glUniform1i(glGetUniformLocation(prog, "v_tex"), 2);

  u_pos = glGetUniformLocation(prog, "draw_pos");

  GLint viewport[4];
  glGetIntegerv(GL_VIEWPORT, viewport);
  resize(viewport[2], viewport[3]);

  return true;
}

bool YUV420P_Player::setupTextures() {

  if(textures_created) {
    printf("Textures already created.\n");
    return false;
  }

  glGenTextures(1, &y_tex);
  glBindTexture(GL_TEXTURE_2D, y_tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, vid_w, vid_h, 0, GL_RED, GL_UNSIGNED_BYTE, NULL); // y_pixels);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glGenTextures(1, &u_tex);
  glBindTexture(GL_TEXTURE_2D, u_tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, vid_w/2, vid_h/2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glGenTextures(1, &v_tex);
  glBindTexture(GL_TEXTURE_2D, v_tex);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, vid_w/2, vid_h/2, 0, GL_RED, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  textures_created = true;
  return true;
}

void YUV420P_Player::draw(int x, int y, int w, int h) {
  // assert(textures_created == true);

  if(w == 0) {
    w = vid_w;
  }

  if(h == 0) {
    h = vid_h;
  }

  glBindVertexArray(vao);
  glUseProgram(prog);

  glUniform4f(u_pos, x, y, w, h);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, y_tex);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, u_tex);

  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, v_tex);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void YUV420P_Player::resize(int winW, int winH) {
  assert(winW > 0 && winH > 0);

  win_w = winW;
  win_h = winH;

  pm.identity();
  pm.ortho(0, win_w, win_h, 0, 0.0, 100.0f);

  glUseProgram(prog);
  glUniformMatrix4fv(glGetUniformLocation(prog, "u_pm"), 1, GL_FALSE, pm.ptr());
}

void YUV420P_Player::setYPixels(uint8_t* pixels, int stride) {
  // assert(textures_created == true);

  glBindTexture(GL_TEXTURE_2D, y_tex);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, vid_w, vid_h, GL_RED, GL_UNSIGNED_BYTE, pixels);
}

void YUV420P_Player::setUPixels(uint8_t* pixels, int stride) {
  // assert(textures_created == true);

  glBindTexture(GL_TEXTURE_2D, u_tex);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, vid_w/2, vid_h/2, GL_RED, GL_UNSIGNED_BYTE, pixels);
}

void YUV420P_Player::setVPixels(uint8_t* pixels, int stride) {
  // assert(textures_created == true);

  glBindTexture(GL_TEXTURE_2D, v_tex);
  glPixelStorei(GL_UNPACK_ROW_LENGTH, stride);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, vid_w/2, vid_h/2, GL_RED, GL_UNSIGNED_BYTE, pixels);
}

#endif // USE_GLFW_LIB