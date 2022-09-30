/*
 * John Salame
 * CSCI 5229 Computer Graphics
 * Homework 3 - 3D Scene
 * Due 9/22/22
 * Work log and version log:
 *   Version 1 (untracked) 10/21/22 10:00 PM - 10/22/22 1:30 AM = 3.5 hr
 *     Got the shaft of the candy cane, but missing the last quad; not sure why
 *       Answer: The quad was not missing; the cylinder wall was complete. The missing part was one pie slice of the base circle.
 *     Attempted to do the hook but realized I still don't know exactly how coordinates and transforms work
 *     This is as far as I get without looking at examples
 *   Version 2 (tracked) 10/22/22 3:30 PM - 4:24 PM = 0.9 hr
 *     Remembered to enable Z-Buffer, fixed the weird rendering of the cylinder when I should have seen the base but could not.
 *     Remembered to do the triangle fan at <=360 degrees (I had <360, which looked like Pac-Man instead of a circle).
 *     Figured out which axis I actually need to rotate the hook around (y-axis, not z-axis).
 *     I still need to get the candy cane hook in the correct location. Right now it is on top of the hook's axis of rotation instead of on top of the straight part of the cane. I need to maybe use homogeneous coordinates in order to rotate the (-hookRad, 0, 0) vector around the origin.
 *   Version 3 (tracked) 10/22/22 4:24 PM - 5:00 PM = 0.6 hr
 *     Got the candy cane hook working (but the top of the hook curve is disjoint because the cylinders are too short. I may make the hook mathematically later, with variable section lengths depending on the distance of the point from the center of the hook sweep curve). 
 *     I still do not specify the location and rotation of a candy cane as parameters. If I do this, it should make positioning things in the scene easier.
 *    Version 4 (tracked) 10/22/22 8:30 PM - 11:00 PM = 2.5 hr
 *      I have one smoooooth candy cane!
 *      I made a proof on a whiteboard for how long to make the secant line and in what direction, for perfectly connecting hook segments.
 *      The proof is hookProof.JPG.
 *      I still do not have parameters in the candy cane for physical placement, but I am happy with what I have.
 *      I think my object is sufficiently complicated, if you look at the equation I came up with to generate the hook.
 *    Version 5 (tracked)
 *      Add README.md
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
// math defines https://stackoverflow.com/questions/1727881/how-to-use-the-pi-constant-in-c
#define _USE_MATH_DEFINES 
#include <math.h>
// begin including OpenGL stuff
#ifdef USEGLEW
#include <GL/glew.h>
#endif
#define GL_GLEXT_PROTOTYPES
#ifdef __APPLE__
#include <GLUT/glut.h>
// Tell Xcode IDE to not gripe about OpenGL deprecation
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#else
#include <GL/glut.h>
#endif
//  Default resolution
//  For Retina displays compile with -DRES=2
#ifndef RES
#define RES 1
#endif


// Begin global variables
double th = 0; // angle around y-axis
double ph = 0; // angle around x-axis
int dim = 10; // width and height of the orthographic projection



// BEGIN UTILITY FUNCTIONS

/* sin function with degrees as input */
float Sin(float angle) {
  float newAngle = M_PI * angle / 180.0;
  return sin(newAngle);
}

/* cos function with degrees as input */
float Cos(float angle) {
  float newAngle = M_PI * angle / 180.0;
  return cos(newAngle);
}

/*
 * Convenience function for text taken from example 4
 */
#define LEN 8192  //  Maximum amount of text
void Print(const char* format , ...)
{
   char    buf[LEN]; // Text storage
   char*   ch=buf;   // Text pointer
   //  Create text to be display
   va_list args;
   va_start(args,format);
   vsnprintf(buf,LEN,format,args);
   va_end(args);
   //  Display text string
   while (*ch)
      glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,*ch++);
}

/*
 *  Print message to stderr and exit
 */
void Fatal(const char* format , ...) {
   va_list args;
   va_start(args,format);
   vfprintf(stderr,format,args);
   va_end(args);
   exit(1);
}

/*
 *  Function to print any errors encountered
 */
void ErrCheck(char* where)
{
   int err = glGetError();
   if (err) fprintf(stderr,"ERROR: %s [%s]\n",gluErrorString(err),where);
}

/* Show the rotated view; this is called at the start of display */
void rotateView() {
  glLoadIdentity();
  // Rotate on x-axis, then model y-axis
  glRotatef(ph, 1.0, 0, 0);
  glRotatef(th, 0, 1.0, 0);
  ErrCheck("rotate view");
}

/* Helper function to display the coordinate axes */
void displayAxes() {
  /* To-Do: Improve by pushing and popping the current transform matrix */
  // x-axis
  glColor3f(1.0, 1.0, 1.0);
  glBegin(GL_LINES);
  glVertex3f(0, 0, 0);
  glVertex3f(0.5*dim, 0, 0); 
  glEnd();
  glRasterPos3f(0.5*dim, 0, 0);
  Print("X");
  //y-axis
  glBegin(GL_LINES);
  glVertex3f(0, 0, 0);
  glVertex3f(0, 0.5*dim, 0);
  glEnd();
  glRasterPos3f(0, 0.5*dim, 0);
  Print("Y");
  // z-axis
  glBegin(GL_LINES);
  glVertex3f(0, 0, 0);
  glVertex3f(0, 0, 0.5*dim);
  glEnd();
  glRasterPos3f(0, 0, 0.5*dim);
  Print("Z");
  ErrCheck("display axes");
}


// BEGIN OBJECT DEFINITIONS

/* Draw a circle with intervals of circlePrecision degrees and radius r at origin (ox, oy, oz) */
void Circle(float circlePrecision, float r, float ox, float oy, float oz) {
  glBegin(GL_TRIANGLE_FAN);
  glVertex3f(ox, oy, oz); // center of triangle fan
  for(int i=0; i<=360; i+=circlePrecision) {
    glVertex3f(ox+r*Cos(i), oy+r*Sin(i), oz);
  }
  glEnd();
  ErrCheck("circle");
}

/* Helper function for candy cane, makes a cylinder wall with radius crossRad and height straightHeight */
void RedStripedCylinderWall(int circlePrecision, float crossRad, float straightHeight) {
  float nonRed = 1.0; // 1.0 when white stripe, 0.0 when red stripe
  glBegin(GL_QUAD_STRIP);
  for(int i=0; i<=360; i+=circlePrecision) {
    glColor3f(1.0, nonRed, nonRed); // I need an even number of rectangles for the color to start and end on red
    nonRed = !nonRed; // binary flip from 0 to 1 or 1 to 0
    glVertex3f(crossRad*Cos(i), crossRad*Sin(i), 0);
    glVertex3f(crossRad*Cos(i), crossRad*Sin(i), straightHeight);
  }
  glEnd();
}

/* 
 * Create a pseudo-cylinder segment of the hook part of the candy cane
 * @param int circlePrecision - how many degrees a quad takes up, and also how many degrees of the curve a hook segment spans
 * @param float crossRad - the radius of the circular cross section
 * @param float hookRad - the radius of the hook curve (distance from the center of the curve to the center of the circular cross section)
 */
void RedStripedHookSegment(int circlePrecision, float crossRad, float hookRad) {
  float nonRed = 1.0;
  int hookDeg = circlePrecision;
  float secantAngle = (float) hookDeg/2; // x in the image hookProof.JPG
  glBegin(GL_QUAD_STRIP);
  // Draw the quads, which have variable height based on how far out they are
  for(int i=0; i <=360; i+=hookDeg) {
    glColor3f(1.0, nonRed, nonRed); // I need an even number of rectangles for the color to start and end on red
    nonRed = !nonRed; // binary flip from 0 to 1 or 1 to 0
    float x = crossRad*Cos(i);
    float radius = hookRad-x;
    // The x value of the vertex is x-hookRad, which is the same as -1*radius
    glVertex3f(-radius, crossRad*Sin(i), 0);
    // Here we explain what is shown in my proof, hookProof.JPG
    // The "top" of the "cylinder" is found by following the secant line from the current hook segment to the start of the next hook segment.
    // The equation to do this is 2*hookRad*sin(hookDeg/2) and the direction of the secant line is the tangent line rotated by (hookDeg/2).
    // We rotate (0, 0, secantLength) counter-clockwise around the y-axis (positive) by (hookDeg/2).
    // From the 3D rotation matrix, sin(hookDeg/2) is (z*sin(hookDeg/2), 0, z*cos(hookDeg/2))
    //   = (secantLength*sin(hookDeg/2), 0, secantLength*cos(hookDeg/2)).
    //   We call hookDeg/2 the secantAngle.
    float secantLength = 2*radius*Sin(secantAngle);
    glVertex3f(-radius+secantLength*Sin(secantAngle), crossRad*Sin(i), secantLength*Cos(secantAngle));
  }
  glEnd();
}

/*
 * Create a candy cane with the bottom at the origin of the model matrix
 * @param float crossRad - radius of the cross section
 * @param float straightHeight - height of the tall straight part before the hook
 * @param float hookRad - the radius of the curve that makes the hook
 * @param float hookDeg - the length of the hook's sweep path in degrees (180 would make a half-circle)
 */
void CandyCane(float crossRad, float straightHeight, float hookRad, int hookDeg) {
  glPushMatrix();
  int circlePrecision = 15; // degrees per rectangle making up a cylinder
  // First, make a circle at the base of the candy cane
  Circle(circlePrecision, crossRad, 0, 0, 0);
  // Now, make the tall straight cylinder
  RedStripedCylinderWall(circlePrecision, crossRad, straightHeight);
  // Now, make the hook
  glTranslatef(hookRad, 0, straightHeight);
  for(int i=0; i<hookDeg; i+=circlePrecision) {
    RedStripedHookSegment(circlePrecision, crossRad, hookRad);
    glRotatef(circlePrecision, 0, 1.0, 0); // rotate the curve origin so the next segment starts at the end of the previous segment
  }
  Circle(circlePrecision, crossRad, -hookRad, 0, 0);
  glPopMatrix();
  ErrCheck("candy cane");
}



// BEGIN CALLBACK FUNCTIONS


void display() {
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  // TO-DO: Push the model matrix (not necessary since rotateView resets the transform to the identity matrix)
  rotateView();
  displayAxes();
  glRotatef(-90, 1.0, 0, 0); // make it so y is up for the candy cane instead of z
  CandyCane(0.8, 4.0, 1.5, 180);
  glTranslatef(2.0, -2.0, -1.0);
  glRotatef(60, 0, 0, 1.0); // rotate about the z-axis (which is the axis of the candy cane at this point since transforms happen from bottom-up)
  CandyCane(1.0, 7.0, 1.3, 160);
  // TO-DO: Pop the model matrix
  // render the scene
  glFlush();
  glutSwapBuffers();
  ErrCheck("display");
}

/* React to key presses */
void special(int key, int x, int y) {
  int dir = 0; // increase a mode or parameter up or down
  int dir2 = 0; //same as dir, but for up and down arrow keys; increases param at a higher rate.
  int thRate = 5; // degrees of rotation per arrow key press
  int phRate = 5;
  if(key == GLUT_KEY_RIGHT)
    dir = 1;
  else if(key == GLUT_KEY_LEFT)
    dir = -1;
  else if(key == GLUT_KEY_UP)
    dir2 = 1;
  else if(key == GLUT_KEY_DOWN)
    dir2 = -1;

  th += dir * thRate;
  ph += dir2 * phRate;
  // re-render the screen
  glutPostRedisplay();
}

void reshape(int width, int height) {
  //  Set viewport as entire window
  glViewport(0, 0, RES*width, RES*height);
  // Select projection matrix
  glMatrixMode(GL_PROJECTION);
  // Set projection to identity
  glLoadIdentity();
  //  Orthogonal projection:  unit cube adjusted for aspect ratio
  double asp = (height>0) ? (double)width/height : 1;
  glOrtho(-dim*asp,+dim*asp, -dim,+dim, -dim,+dim);
 // Select model view of matrix
 glMatrixMode(GL_MODELVIEW);
 // Set the model view to identity
 glLoadIdentity();
}


int main(int argc, char** argv){
  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
  glutCreateWindow("John Salame Homework 3: 3D Scene");
  // Register the callback functions
  glutDisplayFunc(display);
  glutSpecialFunc(special);
  glutReshapeFunc(reshape);
  // Enable Z-buffer depth test
  glEnable(GL_DEPTH_TEST);
  // Finally, allow the window to draw
  glutMainLoop();
  return 0;
}

