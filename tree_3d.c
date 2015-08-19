//cs371 Fall 2014
//
//program: tree_3d.c
//author:  Jasjot Sumal
//
// possible required packages:
// libx11-dev
// libglew1.6
// libglew1.6-dev
//
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include "vecFxns.h"
#include "importObj.h"
#include "log.h"      // --- ERROR LOGGING / DEBUGGING ---
#include "fonts.h"

#define FAR_FIELD -800.0f       // Max visible z distance (3D mode)
#define NEAR_FIELD 1.0f         // Min visible x distance
#define ROT_STEP 10.0f          // For user controlled rotation
#define TRANS_STEP 5.0f         // For user controlled translation
#define ZOOM_STEP 5.0f          // For user controlled scaling
#define PI 3.14159265358979323846264338327950
#define epsilon 1e-12f

#define TEX_NUM 2               // Max number of textures

#define MAX_TAGE 50             // Max tree age
#define MAX_BRANCHES 5793       // Max possible branches = 2^12.5

// Macro Functions
#define blvl(age) age*0.25      // Number of branch levels
#define rnd() (Flt)rand() / (Flt)RAND_MAX

//X Windows variables
Display *dpy;
Window win;
GLXContext glc;

enum {
  ANM_DEFAULT,
  ANM_TREE
};

typedef struct t_mesh {   // triangle mesh
  unsigned int idx0;      // start of mesh in tri[] array
  unsigned int size;      // number of triangles in mesh
} Mesh;

struct Spring {
  struct Mass * mptr0, * mptr1;
  Flt length;
  Flt stiffness;
} springs[MAX_BRANCHES * 7];
unsigned int nsprings = 0;

struct Mass {
  Flt mass;
  Flt oomass;         // 1/mass
  Vec pos;            // position coordinates
  Vec vel;            // velocity
  Vec force;
} masses[MAX_BRANCHES * 7];
unsigned int nmasses = 0;

struct Branch {
  Flt age;
  Vec p;              // end point of branch in absolute coord axis
  Vec rot_offset;     // rotation offset from the previous branch
  Flt length;
  Flt rMatrix[16];    // store rotation & translation to end of branch
  Vec color;
  struct Mass * m;    // mass of current branch plus all children
};

struct Tree {
  Flt age;
  Flt trunk_len;
  Vec startpt;        // starting point: base of tree
  unsigned int branch_levels;
  unsigned int branch_num;
  Flt rotMax;
  Flt rotMin;
  struct Branch branches[MAX_BRANCHES];
} t1;

// ---- FUNCTION PROTOTYPES ----
void  initXWindows      (void);
void  init_opengl       (void);
void  cleanupXWindows   (void);
void  check_resize      (XEvent *);
void  check_mouse       (XEvent *);
void  check_keys        (XEvent *);
void  drawTexSquare     (Vec, Vec, Vec, Vec);
void  debugNorm         (Vec);
void  render            (void);

// ---- TREE FUNCTIONS ----
void  zeroTree          (void);
void  DrawTree          (void);
void  initTree          (void);
void  genBranches       (void);
void  genBranches_r     (const Flt, const unsigned int, const Vec);
void  drawTreeGuideLines (void);
void  drawTreeAbsPts_r  (const unsigned int);
void  drawBranches      (void);

// ---- PHYSICS FUNCTIONS ----
unsigned int fastlog2d  (unsigned int);

// ---- GLOBAL VARIABLES ----
int anm_num = ANM_DEFAULT;
int done = 0;
int xres = 640,
    yres = 480;

// ---- Rotation Angle ----
Flt angle, angle_inc;  // rotating around circle
Flt rotx = 0.0f;
Flt roty = 0.0f;
Flt rotz = 0.0f;
Flt vertTrans = 0.0f;
Flt horzTrans = 0.0f;
Flt zoom = 0.0f;

// ---- Lighting ----
GLfloat LightAmbient[]  = {   0.2f,  0.2f,  0.2f, 1.0f };
GLfloat LightDiffuse[]  = {   1.0f,  1.0f,  1.0f, 1.0f };
GLfloat LightSpecular[] = {   0.5f,  0.5f,  0.5f, 1.0f };
GLfloat LightPosition[] = { 100.0f, 40.0f, 40.0f, 1.0f };

// ---- Triangle Meshes ----
Mesh branchMesh;
Mesh leafMesh;


int main (void)
{
  srand(time(NULL));
  initXWindows();
  init_opengl();

  while(!done) {
    while(XPending(dpy)) {
      XEvent e;
      XNextEvent(dpy, &e);
      check_resize(&e);
      check_mouse(&e);
      check_keys(&e);
    }
    render();
    glXSwapBuffers(dpy, win);
  }

  cleanupXWindows();
  cleanup_fonts();

  return 0;
}

void cleanupXWindows (void)
{
  XDestroyWindow(dpy, win);
  XCloseDisplay(dpy);
}

void set_title (void)
{
  //Set the window title bar.
  XMapWindow(dpy, win);
  XStoreName(dpy, win, "CS371 - Lab7");
}

void setup_screen_res (const int w, const int h)
{
  xres = w;
  yres = h;
}

void initXWindows (void)
{
  Window root;
  GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
  XVisualInfo *vi;
  Colormap cmap;
  XSetWindowAttributes swa;

  setup_screen_res(640, 480);
  dpy = XOpenDisplay(NULL);
  if(dpy == NULL) {
    exit(EXIT_FAILURE);
  }
  root = DefaultRootWindow(dpy);
  vi = glXChooseVisual(dpy, 0, att);
  if(vi == NULL) {
    exit(EXIT_FAILURE);
  }
  cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
  swa.colormap = cmap;
  swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
            StructureNotifyMask | SubstructureNotifyMask;
  win = XCreateWindow(dpy, root, 0, 0, xres, yres, 0,
              vi->depth, InputOutput, vi->visual,
              CWColormap | CWEventMask, &swa);
  set_title();
  glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
  glXMakeCurrent(dpy, win, glc);
}

void reshape_window (int width, int height)
{
  //window has been resized.
  setup_screen_res(width, height);
  //
  glViewport(0, 0, (GLint)width, (GLint)height);
  glMatrixMode(GL_PROJECTION); glLoadIdentity();
  glMatrixMode(GL_MODELVIEW); glLoadIdentity();
  glOrtho(0, xres, 0, yres, -1, 1);
  set_title();
}

void init_opengl (void)
{
  Vec rgb;
  rotx=0.0f;
  roty=0.0f;
  rotz=0.0f;

  switch(anm_num) {
    case ANM_DEFAULT:
			glDisable(GL_LIGHTING);
			glDisable(GL_LIGHT0);

      glViewport(0, 0, xres, yres);
      //Initialize matrices
      glMatrixMode(GL_PROJECTION); glLoadIdentity();
      glMatrixMode(GL_MODELVIEW); glLoadIdentity();
      //This sets 2D mode (no perspective)
      glOrtho(0, xres, 0, yres, -1, 1);
      //Clear the screen
      glClearColor(1.0, 1.0, 1.0, 1.0);
      //glClear(GL_COLOR_BUFFER_BIT);
      //Do this to allow fonts
      glEnable(GL_TEXTURE_2D);
      initialize_fonts();
      break;

    case ANM_TREE:
      glClearColor(0.0f, 0.0f, 0.0f, 0.0f); // Set background to black
      glClearDepth(1.0);        // Enables Clearing Of The Depth Buffer
      glDepthFunc(GL_LESS);     // The Type Of Depth Test To Do
      glEnable(GL_DEPTH_TEST);  // Enables Depth Testing
      glShadeModel(GL_SMOOTH);  // Enables Smooth Color Shading

      glMatrixMode(GL_PROJECTION); glLoadIdentity();
      gluPerspective(45.0f, (GLfloat)xres/(GLfloat)yres, NEAR_FIELD, FAR_FIELD);
      // 3D setup - Calculate The Aspect Ratio Of The Window
      glMatrixMode(GL_MODELVIEW); glLoadIdentity();

			glEnable(GL_COLOR_MATERIAL);
			glEnable( GL_LIGHTING );
			glLightfv(GL_LIGHT0, GL_AMBIENT, LightAmbient);
			glLightfv(GL_LIGHT0, GL_DIFFUSE, LightDiffuse);
			glLightfv(GL_LIGHT0, GL_SPECULAR, LightSpecular);
			glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);
			glEnable(GL_LIGHT0);

      initImportTri();
      rgb[0] = rnd()*0.2f + 0.6f;
      rgb[1] = rnd()*0.2f + 0.35f;
      rgb[2] = rnd()*0.2f + 0.2f;
      branchMesh.idx0 = ntri;
      branchMesh.size = importTriObj("finalbranch.obj",rgb,1);
      break;
  }
}

void check_resize (XEvent *e)
{
  //The ConfigureNotify is sent by the
  //server if the window is resized.
  if (e->type != ConfigureNotify)
    return;
  XConfigureEvent xce = e->xconfigure;
  if (xce.width != xres || xce.height != yres) {
    //Window size did change.
    reshape_window(xce.width, xce.height);
  }
}

// check if mouse moved or was clicked
void check_mouse (XEvent *e)
{
  static int savex = 0;
  static int savey = 0;
  //
  if (e->type == ButtonRelease) {
    return;
  }
  if (e->type == ButtonPress) {
    if (e->xbutton.button==1) {
      //Left button is down
    }
    if (e->xbutton.button==3) {
      //Right button is down
    }
  }
  if (savex != e->xbutton.x || savey != e->xbutton.y) {
    //Mouse moved
    savex = e->xbutton.x;
    savey = e->xbutton.y;
  }
}

void check_keys (XEvent *e)
{
  if (e->type == KeyPress) {
    int key = XLookupKeysym(&e->xkey, 0);

    switch (key) {
      case XK_0:
        anm_num = ANM_DEFAULT;
        init_opengl();
        break;
      case XK_1:
        anm_num = ANM_TREE;
        init_opengl();
        initTree();
        vertTrans = -50.0f;
        horzTrans = 0.0f;
        zoom = -350.0f;
        break;
      case XK_x:
        rotx+=ROT_STEP;
        break;
      case XK_y:
        roty+=ROT_STEP;
        break;
      case XK_z:
        rotz+=ROT_STEP;
        break;
      case XK_w:
        vertTrans += TRANS_STEP;
        break;
      case XK_a:
        horzTrans -= TRANS_STEP;
        break;
      case XK_s:
        vertTrans -= TRANS_STEP;
        break;
      case XK_d:
        horzTrans += TRANS_STEP;
        break;
      case XK_9:
        if (zoom < -ZOOM_STEP+NEAR_FIELD) {
          zoom -= ZOOM_STEP;
        }
        break;
      case XK_8:
        if (zoom > FAR_FIELD) {
          zoom += ZOOM_STEP;
        }
        break;
      case XK_Escape:
        done=1;
        return;
    }
  }
}

void debugNorm (Vec n)
{
  glBegin(GL_LINES);
    glVertex3fv(n);
    glVertex3f(n[0]*3, n[1]*3, n[2]*3);
  glEnd();
}

void render (void)
{
  Rect r;
    // Clear The Screen And The Depth Buffer
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

  switch (anm_num) {
    case ANM_DEFAULT:
      r.bot = yres - 20;
      r.left = 10;
      r.center = 0;
      ggprint8b(&r, 16, 0x00ff0000, "Lab7: OpenGL animation (with lighting)");
      ggprint8b(&r, 16, 0x00ff0000, "0 - Show Menu");
      ggprint8b(&r, 16, 0x00ff0000, "1 - Generate Tree");
      ggprint8b(&r, 16, 0x00ff0000, "x, y, z - rotate objects around axis");
      ggprint8b(&r, 16, 0x00ff0000, "w, a, s, d - translate objects up, left, down, right");
      ggprint8b(&r, 16, 0x00ff0000, "8, 9 - zoom in/out");
      break;
    case ANM_TREE:
      DrawTree();
      break;
  }
}


// ------------- TREE FUNCTIONS -------------
void zeroTree ()
{
  unsigned int i;
  struct Branch * bptr;

  t1.age = t1.trunk_len =
  t1.rotMax = t1.rotMin = 0.0f;
  t1.startpt[0]=t1.startpt[1]=t1.startpt[2]=0.0f;
  t1.branch_levels = 0;

  for (i = 0; i < t1.branch_num; i++) {
    bptr = &t1.branches[i];
    bptr->age = bptr->length = 0.0f;
    bptr->p[0] = bptr->p[1] = bptr->p[2] = 0.0f;
    bptr->rot_offset[0]=
    bptr->rot_offset[1]=
    bptr->rot_offset[2]= 0.0f;
    bptr->color[0] =
    bptr->color[1] =
    bptr->color[2] = 0.0f;
  }
  t1.branch_num = 0;
}

void DrawTree (void)
{
  glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
  // draw tree
  glLoadIdentity();
  glTranslatef(horzTrans, vertTrans, zoom);
  glRotatef(rotx,1.0f,0.0f,0.0f);
  glRotatef(roty,0.0f,1.0f,0.0f);
  glRotatef(rotz,0.0f,0.0f,1.0f);
  //drawTreeGuideLines(); // for debugging
  drawBranches();
}

void initTree (void)
{
  zeroTree();

  // determine number of branches
  t1.age = rnd()*MAX_TAGE*0.35 + MAX_TAGE*0.65;
  t1.trunk_len = (Flt)t1.age*0.7;
  t1.branch_levels = blvl(t1.age);
  t1.startpt[0] = 0.0f;
  t1.startpt[1] = 0.0f;
  t1.startpt[2] = 0.0f;
    // assuming 2 branches per branch point:
  t1.branch_num = pow(2, t1.branch_levels);

  t1.rotMin = 7.0f;   // min & max z rotation offsets in degress
  t1.rotMax = 60.0f;  // range: max to min & -min to -max
  genBranches();
}

// -------------------------------------------------------

void genBranches (void)
{
  //Log("genBranches\n");
  Vec next_rot;
  Flt next_age;

  t1.branches[0].length = t1.trunk_len;
  t1.branches[0].rot_offset[0] = rnd()*25.0f * (rand()%2? -1 : 1);
  t1.branches[0].rot_offset[1] = rnd()*90.0f * (rand()%2? -1 : 1);
  t1.branches[0].rot_offset[2] = rnd()*25.0f * (rand()%2? -1 : 1);

  // Set Trunk Absolute Coordinates
  glLoadIdentity();
  // adjust modelview matrix
  glTranslatef(t1.startpt[0],t1.startpt[1],t1.startpt[2]);
  glRotatef(t1.branches[0].rot_offset[0],1.0f,0.0f,0.0f);
  glRotatef(t1.branches[0].rot_offset[1],0.0f,1.0f,0.0f);
  glRotatef(t1.branches[0].rot_offset[2],0.0f,0.0f,1.0f);
  glTranslatef(0.0f,t1.branches[0].length,0.0f);
  // copy modelview into rMatrix
  glGetFloatv(GL_MODELVIEW_MATRIX, t1.branches[0].rMatrix);
  // 1st resolve rotation:
  t1.branches[0].p[0] =
    t1.branches[0].rMatrix[0]  *  t1.branches[0].p[0] +
    t1.branches[0].rMatrix[4]  *  t1.branches[0].p[1] +
    t1.branches[0].rMatrix[8]  *  t1.branches[0].p[2];
  t1.branches[0].p[1] =
    t1.branches[0].rMatrix[1]  *  t1.branches[0].p[0] +
    t1.branches[0].rMatrix[5]  *  t1.branches[0].p[1] +
    t1.branches[0].rMatrix[9]  *  t1.branches[0].p[2];
  t1.branches[0].p[2] =
    t1.branches[0].rMatrix[2]  *  t1.branches[0].p[0] +
    t1.branches[0].rMatrix[6]  *  t1.branches[0].p[1] +
    t1.branches[0].rMatrix[10] *  t1.branches[0].p[2];
  // 2nd resolve translation:
  t1.branches[0].p[0] = t1.branches[0].rMatrix[12];
  t1.branches[0].p[1] = t1.branches[0].rMatrix[13];
  t1.branches[0].p[2] = t1.branches[0].rMatrix[14];

  // Set 1st Branch Attributes, then Recurse
  next_age = rnd()*(t1.age-1.8f)*0.5f + (t1.age-1.8f)*0.5f + 0.9f;
  t1.branches[0].age = next_age;
  next_rot[0] = 0.0f;
  next_rot[1] = rnd()*90.0f * (rand()%2? -1 : 1);
  next_rot[2] = (1.0f - (Flt)next_age/(Flt)t1.age) *
                (t1.rotMax-t1.rotMin) + t1.rotMin;
  genBranches_r (next_age, 1, next_rot);

  // Set 2nd Branch Attributes, then Recurse
  next_age = rnd()*(t1.age-1.8f)*0.5f + (t1.age-1.8f)*0.5f + 0.9f;
  t1.branches[0].age += next_age*0.2f;
  next_rot[0] = 0.0f;
  next_rot[1] = rnd()*90.0f * (rand()%2? -1 : 1);
  next_rot[2] = (1.0f - (Flt)next_age/(Flt)t1.age) *
                (t1.rotMax-t1.rotMin) + t1.rotMin;
  genBranches_r (next_age, 2, next_rot);
}

// -------------------------------------------------------

void genBranches_r (const Flt cur_age, const unsigned int pos, const Vec cur_rot)
{
// Store branches in a heap structure
  Vec next_rot;
  Flt next_age;
  unsigned int child1, child2;

  // Want branch age to be at least 1, but less than previous age,
  // which must be at least 2 (recursion stops if age == 1)
  t1.branches[pos].age = cur_age;
  t1.branches[pos].rot_offset[0] = cur_rot[0];
  t1.branches[pos].rot_offset[1] = cur_rot[1];
  t1.branches[pos].rot_offset[2] = cur_rot[2];

  child1 = (2*pos) + 1;
  child2 = child1 + 1;
  // base case: reached absolute top branch level
  if (child1 >= t1.branch_num || child2 >= t1.branch_num) {
    t1.branches[pos].length = 1.0f;
    t1.branches[pos].age = 0.9f;
  }
  // base case: top branch level relative to age of previous branches
  else if (t1.branches[pos].age <= 1) {
    t1.branches[pos].length = 1.0f;
  }
  else
    t1.branches[pos].length = rnd()*(Flt)t1.branches[pos].age*0.3f + (Flt)t1.branches[pos].age*0.7f;

  // Set Branch Absolute Coordinates
  // save caller's state
  glPushMatrix();
  // adjust modelview matrix
  glRotatef(t1.branches[pos].rot_offset[0],1.0f,0.0f,0.0f);
  glRotatef(t1.branches[pos].rot_offset[1],0.0f,1.0f,0.0f);
  glRotatef(t1.branches[pos].rot_offset[2],0.0f,0.0f,1.0f);
  glTranslatef(0.0f,t1.branches[pos].length,0.0f);
  // copy modelview into rMatrix
  glGetFloatv(GL_MODELVIEW_MATRIX, t1.branches[pos].rMatrix);
  // 1st resolve rotation:
  t1.branches[pos].p[0] =
    t1.branches[pos].rMatrix[0]  *  t1.branches[pos].p[0] +
    t1.branches[pos].rMatrix[4]  *  t1.branches[pos].p[1] +
    t1.branches[pos].rMatrix[8]  *  t1.branches[pos].p[2];
  t1.branches[pos].p[1] =
    t1.branches[pos].rMatrix[1]  *  t1.branches[pos].p[0] +
    t1.branches[pos].rMatrix[5]  *  t1.branches[pos].p[1] +
    t1.branches[pos].rMatrix[9]  *  t1.branches[pos].p[2];
  t1.branches[pos].p[2] =
    t1.branches[pos].rMatrix[2]  *  t1.branches[pos].p[0] +
    t1.branches[pos].rMatrix[6]  *  t1.branches[pos].p[1] +
    t1.branches[pos].rMatrix[10] *  t1.branches[pos].p[2];
  // 2nd resolve translation:
  t1.branches[pos].p[0] = t1.branches[pos].rMatrix[12];
  t1.branches[pos].p[1] = t1.branches[pos].rMatrix[13];
  t1.branches[pos].p[2] = t1.branches[pos].rMatrix[14];

  // recursive case
  if (t1.branches[pos].age > 1) {
    // ----- 1st Child -----
    next_age = rnd()*(cur_age-1.8f)*0.5f + (cur_age-1.8f)*0.5f + 0.9f;
    // X Rotation (Angle from parent branch front/back)
    next_rot[0] = 0.0f;
    // Y Rotation (Twist around parent branch)
    next_rot[1] = rnd()*90.0f * (rand()%2? -1 : 1);
    // Z Rotation (Angle from parent branch left/right)
    next_rot[2] = (1.0f - (Flt)next_age/(Flt)t1.age) *
                  (t1.rotMax-t1.rotMin) + t1.rotMin;
    genBranches_r(next_age, child1, next_rot);

    // ----- 2nd Child -----
    next_age = rnd()*(cur_age-1.8f)*0.5f + (cur_age-1.8f)*0.5f + 0.9f;
    // X Rotation:
    next_rot[0] = 0.0f;
    // Y Rotation:
    if (next_rot[1] < 0)
      next_rot[1] -= 180.0f;
    else
      next_rot[1] += 180.0f;
    // Z Rotation:
    next_rot[2] = (1.0f - (Flt)next_age/(Flt)t1.age) *
                  (t1.rotMax-t1.rotMin) + t1.rotMin;
    genBranches_r(next_age, child2, next_rot);
  }
  else {
    // green rgb:   0.2, 0.4, 0
    // yellow rgb:  0.8156, 0.6275, 0
    // difference:  0.6156, 0.2275, 0
    t1.branches[pos].color[0] = rnd()*0.6156f + 0.2f;
    t1.branches[pos].color[1] = rnd()*0.2275f + 0.4f;
    t1.branches[pos].color[2] = 0.0f;
  }
  // restore caller's state
  glPopMatrix();
}

// -------------------------------------------------------

void drawTreeGuideLines (void) // for debugging
{
// wrapper for recrusive draw function
  Vec dir;

  // draw first branch
  dir[0] = t1.branches[0].p[0] - t1.startpt[0];
  dir[1] = t1.branches[0].p[1] - t1.startpt[1];
  dir[2] = t1.branches[0].p[2] - t1.startpt[2];
  vecNormalize(dir);

  glBegin(GL_LINES);
    glColor3f(1.0f,1.0f,1.0f);
		//glNormal3f( 1.0f, 0.0f, 0.0f); -- sample: setting normal
    glVertex3f(t1.startpt[0],t1.startpt[1],t1.startpt[2]);
    glVertex3f(t1.startpt[0] + dir[0]*t1.branches[0].length*0.25,
               t1.startpt[1] + dir[1]*t1.branches[0].length*0.25,
               t1.startpt[2] + dir[2]*t1.branches[0].length*0.25);

    glColor3f(0.0f,1.0f,1.0f);
    glVertex3f(t1.startpt[0] + dir[0]*t1.branches[0].length*0.25,
               t1.startpt[1] + dir[1]*t1.branches[0].length*0.25,
               t1.startpt[2] + dir[2]*t1.branches[0].length*0.25);
    glVertex3f(t1.startpt[0] + dir[0]*t1.branches[0].length*0.5,
               t1.startpt[1] + dir[1]*t1.branches[0].length*0.5,
               t1.startpt[2] + dir[2]*t1.branches[0].length*0.5);

    glColor3f(1.0f,1.0f,0.0f);
    glVertex3f(t1.startpt[0] + dir[0]*t1.branches[0].length*0.5,
               t1.startpt[1] + dir[1]*t1.branches[0].length*0.5,
               t1.startpt[2] + dir[2]*t1.branches[0].length*0.5);
    glVertex3f(t1.startpt[0] + dir[0]*t1.branches[0].length*0.75,
               t1.startpt[1] + dir[1]*t1.branches[0].length*0.75,
               t1.startpt[2] + dir[2]*t1.branches[0].length*0.75);

    glColor3f(1.0f,0.0f,1.0f);
    glVertex3f(t1.startpt[0] + dir[0]*t1.branches[0].length*0.75,
               t1.startpt[1] + dir[1]*t1.branches[0].length*0.75,
               t1.startpt[2] + dir[2]*t1.branches[0].length*0.75);
    glVertex3f(t1.startpt[0] + dir[0]*t1.branches[0].length,
               t1.startpt[1] + dir[1]*t1.branches[0].length,
               t1.startpt[2] + dir[2]*t1.branches[0].length);
  glEnd();

  // draw remaining branches
  drawTreeAbsPts_r(1);
  drawTreeAbsPts_r(2);
}

void drawTreeAbsPts_r (const unsigned int i)
{
  Vec dir;
  Vec prev;
  unsigned int parent = (unsigned int)floor((i-1)*0.5f);

  prev[0] = t1.branches[parent].p[0];
  prev[1] = t1.branches[parent].p[1];
  prev[2] = t1.branches[parent].p[2];
  dir[0] = t1.branches[i].p[0] - prev[0];
  dir[1] = t1.branches[i].p[1] - prev[1];
  dir[2] = t1.branches[i].p[2] - prev[2];
  vecNormalize(dir);

  glBegin(GL_LINES);
    glColor3f(1.0f,1.0f,1.0f);
    glVertex3f(prev[0],prev[1],prev[2]);
    glVertex3f(prev[0] + dir[0]*t1.branches[i].length*0.25,
               prev[1] + dir[1]*t1.branches[i].length*0.25,
               prev[2] + dir[2]*t1.branches[i].length*0.25);

    glColor3f(0.0f,1.0f,1.0f);
    glVertex3f(prev[0] + dir[0]*t1.branches[i].length*0.25,
               prev[1] + dir[1]*t1.branches[i].length*0.25,
               prev[2] + dir[2]*t1.branches[i].length*0.25);
    glVertex3f(prev[0] + dir[0]*t1.branches[i].length*0.5,
               prev[1] + dir[1]*t1.branches[i].length*0.5,
               prev[2] + dir[2]*t1.branches[i].length*0.5);

    glColor3f(1.0f,1.0f,0.0f);
    glVertex3f(prev[0] + dir[0]*t1.branches[i].length*0.5,
               prev[1] + dir[1]*t1.branches[i].length*0.5,
               prev[2] + dir[2]*t1.branches[i].length*0.5);
    glVertex3f(prev[0] + dir[0]*t1.branches[i].length*0.75,
               prev[1] + dir[1]*t1.branches[i].length*0.75,
               prev[2] + dir[2]*t1.branches[i].length*0.75);

    glColor3f(1.0f,0.0f,1.0f);
    glVertex3f(prev[0] + dir[0]*t1.branches[i].length*0.75,
               prev[1] + dir[1]*t1.branches[i].length*0.75,
               prev[2] + dir[2]*t1.branches[i].length*0.75);
    glVertex3f(prev[0] + dir[0]*t1.branches[i].length,
               prev[1] + dir[1]*t1.branches[i].length,
               prev[2] + dir[2]*t1.branches[i].length);
  glEnd();

  // recursive case
  if (t1.branches[i].age > 1) {
    drawTreeAbsPts_r(2*i + 1);
    drawTreeAbsPts_r(2*i + 2);
  }
}

void drawBranches (void)
{
  Vec normal, v0, v1, v2;
  Flt leafscl = 6.0f;
  unsigned int i, j;

  for (i = 0; i < t1.branch_num; i++) {
    if(t1.branches[i].age < epsilon) continue;
    glPushMatrix();

    if (t1.branches[i].age > 1.0f) {
      glMultMatrixf(t1.branches[i].rMatrix);
      glScalef(t1.branches[i].age*0.8,
               t1.branches[i].length*1.1,
               t1.branches[i].age*0.8);
      for (j = branchMesh.idx0; j < ntri; j++) {
        glColor3fv(tri[j].color);
        glBegin(GL_TRIANGLES);
          glNormal3fv(tri[j].norm);
          glVertex3fv(tri[j].vertex[0]);
          glVertex3fv(tri[j].vertex[1]);
          glVertex3fv(tri[j].vertex[2]);
        glEnd();
      }
    }
    else {
      glMultMatrixf(t1.branches[i].rMatrix);
      glColor3fv(t1.branches[i].color);
      v0[0] = -(t1.branches[i].age*0.25f*leafscl);
      v0[1] = t1.branches[i].length*leafscl*0.5f;
      v0[2] = 0.0f;
      v1[0] = 0.0f;
      v1[1] = t1.branches[i].length*leafscl;
      v1[2] = 0.0f;
      v2[0] = t1.branches[i].age*0.25f*leafscl;
      v2[1] = t1.branches[i].length*leafscl*0.5f;
      v2[2] = 0.0f;
      getNormal(v0,v1,v2,normal);
      glBegin(GL_QUADS);
        glNormal3fv(normal);
        glVertex3f(0.0f,0.0f,0.0f);
        glVertex3fv(v2);
        glVertex3fv(v0);
        glVertex3fv(v1);
      glEnd();
    }

    glPopMatrix();
  }
}

unsigned int fastlog2d (unsigned int val) {
  unsigned int r = -1;
  while (val >>= 1) {
    r++;
  }
  return r;
}
