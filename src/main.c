#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <GL/glu.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <float.h>
#include <time.h>
#include "game.h"
#include "physics.h"
#include "texture.h"
#include "font.h"

static const double PI = 3.14159265;
#define RADS(angle) (((angle) * PI) / 180.0)

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

static const double MOUSE_SENS = 0.1;
static double lastx = 0.0;
static double lasty = 0.0;

static const double MOVE_VEL = 2.0;
static const double FRICTION = 0.2;

static Font *font;

static int fps = 0;

static bool selecting = false;
static Vector3d trace;

static void error(int error, const char *description)
{
#ifdef DEBUG_GRAPHICS
  fprintf(stderr, "[INIT ] glfw error: %s\n", description);
#endif
}

static void display(GLFWwindow *window)
{
  Game *game = glfwGetWindowUserPointer(window);
  World *world = game->worlds[0];
  Entity *player = world->entities[0];
  
  glClearColor(0x87/255.0, 0xce/255.0, 0xfa/255.0, 1.0);
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glRotated(player->apos.x, 1.0, 0.0, 0.0);
  glRotated(player->apos.y, 0.0, 1.0, 0.0);
  glRotated(player->apos.z, 0.0, 0.0, 1.0);
  glTranslated(-player->pos.x, -player->pos.y-0.5, -player->pos.z);
  
  world_draw(world);

  if(selecting) {
    glTranslated(floor(trace.x), floor(trace.y), floor(trace.z));
    glDisable(GL_TEXTURE_2D);
    glColor4d(0.0, 0.0, 1.0, 1.0);
    glLineWidth(10.0);
    glBegin(GL_LINE_LOOP);
    glVertex3d(0.0, 0.0, 0.0);
    glVertex3d(0.0, 0.0, 1.0);
    glVertex3d(0.0, 1.0, 1.0);
    glVertex3d(0.0, 1.0, 0.0);
    glVertex3d(1.0, 1.0, 0.0);
    glVertex3d(1.0, 1.0, 1.0);
    glVertex3d(1.0, 0.0, 1.0);
    glVertex3d(1.0, 0.0, 0.0);
    glEnd();
    glBegin(GL_LINES);
    glVertex3d(0.0, 1.0, 1.0);
    glVertex3d(1.0, 1.0, 1.0);
    glVertex3d(0.0, 0.0, 1.0);
    glVertex3d(1.0, 0.0, 1.0);
    glVertex3d(0.0, 0.0, 0.0);
    glVertex3d(0.0, 1.0, 0.0);
    glVertex3d(1.0, 0.0, 0.0);
    glVertex3d(1.0, 1.0, 0.0);
    glEnd();
  }
  
  glMatrixMode(GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  glOrtho(0, width, 0, height, -0.1, 0.1);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glTranslated(10.0, 10.0, 0.0);
  glColor4d(1.0, 1.0, 1.0, 1.0);
  char tmp[64];
  snprintf(tmp, 64, "FPS: %d", fps);
  font_draw_text(font, tmp);

  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  
  glfwSwapBuffers(window);
}

static void reshape(GLFWwindow *window, int width, int height)
{
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  gluPerspective(90.0, ((double) width)/height, 0.1, 100.0);
  glViewport(0, 0, width, height);
#ifdef DEBUG_GRAPHICS
  fprintf(stderr, "[RESHP] width %d height %d\n", width, height);
#endif
}

static void mouse(GLFWwindow *window, double x, double y)
{
  Game *game = glfwGetWindowUserPointer(window);
  Vector3d *rot = &game->worlds[0]->entities[0]->apos;
  rot->x += (y - lasty) * MOUSE_SENS;
  rot->y += (x - lastx) * MOUSE_SENS;
  rot->x = rot->x > 90.0 ? 90.0 : (rot->x < -90.0 ? -90.0 : rot->x);
  lastx = x;
  lasty = y;
#ifdef DEBUG
  fprintf(stderr, "[MOUSE] x %.2f, y %.2f, xrot %.2f, yrot %.2f\n", x, y, rot->x, rot->y);
#endif
}

static void click(GLFWwindow *window, int button, int action, int modifiers)
{
  Game *game = glfwGetWindowUserPointer(window);
  if(selecting && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
    world_block_delete3d(game->worlds[0], &trace);
  }
}

static void keyboard(GLFWwindow *window, int key, int scancode, int action, int modifiers)
{
  Game *game = glfwGetWindowUserPointer(window);
  Vector3d *vel = &game->worlds[0]->entities[0]->vel;
  switch(key) {
  case GLFW_KEY_SPACE:
    if(action == GLFW_PRESS && phys_is_grounded(game->worlds[0], game->worlds[0]->entities[0])) {
      vel->y = ((modifiers & GLFW_MOD_CONTROL) != 0) ? sqrt(2*9.81*100.0) : sqrt(2*9.81*1.1);
    }
    break;
  }
#ifdef DEBUG
  fprintf(stderr, "[KEYBD] key %d, scan %d, action %d, mod %d\n", key, scancode, action, modifiers);
#endif
}

static GLuint rock_texture = 0;
static GLuint grass_texture = 0;

static void cube_draw(void *user)
{
  GLuint *texture = user;
  glEnable(GL_TEXTURE_2D);
  glBindTexture(GL_TEXTURE_2D, *texture);
  glColor4d(1.0, 1.0, 1.0, 1.0);
  glEnable(GL_DEPTH_TEST);
  glBegin(GL_QUADS);

  // front
  glTexCoord2d(0.0, 0.0);
  glVertex3d(0.0, 0.0, 0.0);
  glTexCoord2d(1.0, 0.0);
  glVertex3d(1.0, 0.0, 0.0);
  glTexCoord2d(1.0, 1.0);
  glVertex3d(1.0, 1.0, 0.0);
  glTexCoord2d(0.0, 1.0);
  glVertex3d(0.0, 1.0, 0.0);
  
  // back
  glTexCoord2d(0.0, 0.0);
  glVertex3d(1.0, 0.0, 1.0);
  glTexCoord2d(1.0, 0.0);
  glVertex3d(0.0, 0.0, 1.0);
  glTexCoord2d(1.0, 1.0);
  glVertex3d(0.0, 1.0, 1.0);
  glTexCoord2d(0.0, 1.0);
  glVertex3d(1.0, 1.0, 1.0);

  // left
  glTexCoord2d(0.0, 0.0);
  glVertex3d(0.0, 0.0, 1.0);
  glTexCoord2d(1.0, 0.0);
  glVertex3d(0.0, 0.0, 0.0);
  glTexCoord2d(1.0, 1.0);
  glVertex3d(0.0, 1.0, 0.0);
  glTexCoord2d(0.0, 1.0);
  glVertex3d(0.0, 1.0, 1.0);
  
  // right
  glTexCoord2d(0.0, 0.0);
  glVertex3d(1.0, 0.0, 0.0);
  glTexCoord2d(1.0, 0.0);
  glVertex3d(1.0, 0.0, 1.0);
  glTexCoord2d(1.0, 1.0);
  glVertex3d(1.0, 1.0, 1.0);
  glTexCoord2d(0.0, 1.0);
  glVertex3d(1.0, 1.0, 0.0);

  // top
  glTexCoord2d(0.0, 0.0);
  glVertex3d(0.0, 1.0, 0.0);
  glTexCoord2d(1.0, 0.0);
  glVertex3d(1.0, 1.0, 0.0);
  glTexCoord2d(1.0, 1.0);
  glVertex3d(1.0, 1.0, 1.0);
  glTexCoord2d(0.0, 1.0);
  glVertex3d(0.0, 1.0, 1.0);

  // bottom
  glTexCoord2d(0.0, 0.0);
  glVertex3d(0.0, 0.0, 1.0);
  glTexCoord2d(1.0, 0.0);
  glVertex3d(1.0, 0.0, 1.0);
  glTexCoord2d(1.0, 1.0);
  glVertex3d(1.0, 0.0, 0.0);
  glTexCoord2d(0.0, 1.0);
  glVertex3d(0.0, 0.0, 0.0);

  glEnd();
}

static long trender_max = 0, tphysics_max = 0, ttotal_max = 0;

int main(void)
{
  glfwSetErrorCallback(error);
  
  if(!glfwInit()) {
#ifdef DEBUG
    fprintf(stderr, "[INIT ] glfw failed to initialize\n");
#endif
    return 1;
  }
  
  GLFWwindow *window = glfwCreateWindow(800, 800, "test", NULL, NULL);
  if(!window) {
#ifdef DEBUG
    fprintf(stderr, "[INIT ] glfw window creation failed\n");
#endif
    glfwTerminate();
    return 1;
  }
  glfwMakeContextCurrent(window);

  glfwSetFramebufferSizeCallback(window, reshape);
  glfwSetWindowRefreshCallback(window, display);
  glfwSetCursorPosCallback(window, mouse);
  glfwSetMouseButtonCallback(window, click);
  glfwSetKeyCallback(window, keyboard);

  int width, height;
  glfwGetFramebufferSize(window, &width, &height);
  reshape(window, width, height);
  
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

  font = malloc(sizeof(Font));
#ifdef __MINGW32__
  font_init(font, "C:\\Windows\\Fonts\\arial.ttf", 12);
#else
  font_init(font, "/usr/share/fonts/TTF/arial.ttf", 12);
#endif

  Game *game = malloc(sizeof(Game));
  game_init(game);
  game_alloc_worlds(game, 1);
  World *world = game->worlds[0];
  world_alloc_players(world, 1);
  Entity *player = world->entities[0];
  glfwSetWindowUserPointer(window, game);
  
  rock_texture = load_texture("res/rock.png");
  grass_texture = load_texture("res/tiled_grass.png");
  Model rock_model, grass_model;
  model_init(&rock_model, DISPLAY_LIST);
  model_init(&grass_model, DISPLAY_LIST);
  model_gen_list(&rock_model, cube_draw, (void *) &rock_texture);
  model_gen_list(&grass_model, cube_draw, (void *) &grass_texture);
  Block rock, grass;
  block_init(&rock, &rock_model, 1, "block_rock", "Rock");
  block_init(&grass, &grass_model, 2, "block_grass", "Grass");
  world_add_mapping(world, &rock);
  world_add_mapping(world, &grass);

  while(!glfwWindowShouldClose(window)) {
    struct timespec start, finish, begin, end;
    clock_gettime(CLOCK_REALTIME, &start);

    for(double x=-0.5;x<1.0;++x) {
      for(double y=-0.5;y<1.0;++y) {
	for(double z=-0.5;z<1.0;++z) {
	  Vector3d tmp;
	  vec_set3d(&tmp, x+floor(player->pos.x/16), y+floor(player->pos.y/16), z+floor(player->pos.z/16));
	  world_chunk_gen3d(world, &tmp, 0);
	}
      }
    }

    clock_gettime(CLOCK_REALTIME, &begin);
    display(window);
    clock_gettime(CLOCK_REALTIME, &end);
    end.tv_nsec += (end.tv_sec - begin.tv_sec) * 1000000000;
    long trender = (end.tv_nsec - begin.tv_nsec) / 1000;
    
    glfwPollEvents();

    selecting = phys_trace(world, player, 5.0, &trace);

    Vector3d *rot = &player->apos;
    if(phys_is_grounded(world, player)) {
      if(player->vel.x >= FRICTION) {
	player->vel.x -= FRICTION;
      } else if(player->vel.x > 0.0) {
	player->vel.x = 0.0;
      } else if(player->vel.x <= -FRICTION) {
	player->vel.x += FRICTION;
      } else {
	player->vel.x = 0.0;
      }
      if(player->vel.z >= FRICTION) {
	player->vel.z -= FRICTION;
      } else if(player->vel.z > 0.0) {
	player->vel.z = 0.0;
      } else if(player->vel.z <= -FRICTION) {
	player->vel.z += FRICTION;
      } else {
	player->vel.z = 0.0;
      }
    }
    if(glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
      player->vel.x = sin(RADS(rot->y)) * MOVE_VEL;
      player->vel.z = -cos(RADS(rot->y)) * MOVE_VEL;
    }
    if(glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
      player->vel.x = sin(RADS(rot->y-90)) * MOVE_VEL;
      player->vel.z = -cos(RADS(rot->y-90)) * MOVE_VEL;
    }
    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
      player->vel.x = -sin(RADS(rot->y)) * MOVE_VEL;
      player->vel.z = cos(RADS(rot->y)) * MOVE_VEL;
    }
    if(glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
      player->vel.x = sin(RADS(rot->y+90)) * MOVE_VEL;
      player->vel.z = -cos(RADS(rot->y+90)) * MOVE_VEL;
    }
    player->vel.y -= 9.81/30;

    clock_gettime(CLOCK_REALTIME, &begin);
#ifdef DEBUG
    int cycles = 0;
#endif
    double t = 0.0;
    while(t < (1/30.0)) {
      t += phys_update(world, (1/30.0) - t);
#ifdef DEBUG
      ++cycles;
#endif
    }
#ifdef DEBUG
    fprintf(stderr, "[INIT ] physics took %d cycles\n", cycles);
#endif
    clock_gettime(CLOCK_REALTIME, &end);
    end.tv_nsec += (end.tv_sec - begin.tv_sec) * 1000000000;
    long tphysics = (end.tv_nsec - begin.tv_nsec) / 1000;
    
#ifdef DEBUG_GRAPHICS
    GLenum error = glGetError();
    while(error != GL_NO_ERROR) {
      fprintf(stderr, "[GRAPH] gl error: %s\n", gluErrorString(error));
      error = glGetError();
    }
#endif
    
    clock_gettime(CLOCK_REALTIME, &finish);
    finish.tv_nsec += (finish.tv_sec - start.tv_sec) * 1000000000;
    long ttotal = (finish.tv_nsec - start.tv_nsec) / 1000;
    fps = 1000000 / ttotal;
    if(fps > 30) {fps = 30;}
#ifdef DEBUG
    fprintf(stderr, "[INIT ] graphics %d, physics %d, total %d (%d fps)\n",
	    (int) trender, (int) tphysics, (int) ttotal, fps);
#endif
    
    trender_max = MAX(trender_max, trender);
    tphysics_max = MAX(tphysics_max, tphysics);
    ttotal_max = MAX(ttotal_max, ttotal);
    
    usleep(ttotal > (1000000/30) ? 0 : (1000000/30) - ttotal);
  }

  fprintf(stderr, "[INIT ] exiting main loop, render max %d, physics max %d, total max %d (%d fps)\n",
	  (int) trender_max, (int) tphysics_max, (int) ttotal_max, (int) (1000000/ttotal_max));
  
  glfwTerminate();
  return 0;
}
