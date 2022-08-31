//modified by:
//date:
//
//author: Gordon Griesel
//date: Spring 2022
//purpose: get openGL working on your personal computer
//
#include <iostream>
using namespace std;
#include <stdio.h>
#include <unistd.h>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cmath>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/glx.h>


//some structures

const int MAX_PARTICLES = 1000;

class Global {
public:
	int xres, yres;
        int mouse_x, mouse_y;
        int pressed;
        int frame_count = 0;
	Global();
} g;

void make_boxes(int h, int w, int x, int y);


class Box {
    public:
	float vel[2];
    	float w;
	float h;
	float x;
	float y;
    	float dir;
    	float pos[2];
    	Box() {
       		h = 15.0f;
		w = 80.0f;
       		dir = 25.0f;
       		pos[0] = x = g.xres / 6.0;
       		pos[1] = y = g.yres / 1.2f;
		vel[0] = vel[1] = 0.0;
		make_boxes(h, w, x, y);
    	}
	Box(float width, float d, float p0, float p1) {
	    	w = width;
		dir = d;
		pos[0] = p0;
		pos[1] = p1;
	}
} box, particle(5.0, 1.0, g.xres/2.0, g.yres/4.0 * 3.0) ;

Box particles[MAX_PARTICLES];
Box boxes[5];
int n = 0;

void make_boxes(int h, int w, int x, int y)
{
	for (int i = 0; i < 5; i++) {
	    boxes[i].h = h;
	    boxes[i].w = w;
	    boxes[i].pos[0] = x;
	    boxes[i].pos[1] = y;
	    x+=120;
	    y-=80;
	}
}

class X11_wrapper {
private:
	Display *dpy;
	Window win;
	GLXContext glc;
public:
	~X11_wrapper();
	X11_wrapper();
	void set_title();
	bool getXPending();
	XEvent getXNextEvent();
	void swapBuffers();
	void reshape_window(int width, int height);
	void check_resize(XEvent *e);
	void check_mouse(XEvent *e);
	int check_keys(XEvent *e);
} x11;

//Function prototypes
void init_opengl(void);
void physics(void);
void render(void);


//=====================================
// MAIN FUNCTION IS HERE
//=====================================
int main()
{
	init_opengl();
	//Main loop
	int done = 0;
	while (!done) {
		//Process external events.
		while (x11.getXPending()) {
			XEvent e = x11.getXNextEvent();
			x11.check_resize(&e);
			x11.check_mouse(&e);
			done = x11.check_keys(&e);
		}
		physics();
		render();
		x11.swapBuffers();
		usleep(200);
	}
	return 0;
}

Global::Global()
{
	xres = 800;
	yres = 600;
        pressed = 0;
        mouse_x = 0;
        mouse_y = 0;
}

X11_wrapper::~X11_wrapper()
{
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
}

X11_wrapper::X11_wrapper()
{
	GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	int w = g.xres, h = g.yres;
	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		cout << "\n\tcannot connect to X server\n" << endl;
		exit(EXIT_FAILURE);
	}
	Window root = DefaultRootWindow(dpy);
	XVisualInfo *vi = glXChooseVisual(dpy, 0, att);
	if (vi == NULL) {
		cout << "\n\tno appropriate visual found\n" << endl;
		exit(EXIT_FAILURE);
	} 
	Colormap cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
	XSetWindowAttributes swa;
	swa.colormap = cmap;
	swa.event_mask =
		ExposureMask | KeyPressMask | KeyReleaseMask |
		ButtonPress | ButtonReleaseMask |
		PointerMotionMask |
		StructureNotifyMask | SubstructureNotifyMask;
	win = XCreateWindow(dpy, root, 0, 0, w, h, 0, vi->depth,
		InputOutput, vi->visual, CWColormap | CWEventMask, &swa);
	set_title();
	glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
	glXMakeCurrent(dpy, win, glc);
}

void X11_wrapper::set_title()
{
	//Set the window title bar.
	XMapWindow(dpy, win);
	XStoreName(dpy, win, "3350 Lab1");
}

bool X11_wrapper::getXPending()
{
	//See if there are pending events.
	return XPending(dpy);
}

XEvent X11_wrapper::getXNextEvent()
{
	//Get a pending event.
	XEvent e;
	XNextEvent(dpy, &e);
	return e;
}

void X11_wrapper::swapBuffers()
{
	glXSwapBuffers(dpy, win);
}

void X11_wrapper::reshape_window(int width, int height)
{
	//window has been resized.
	g.xres = width;
	g.yres = height;
	//
	glViewport(0, 0, (GLint)width, (GLint)height);
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	glOrtho(0, g.xres, 0, g.yres, -1, 1);
}

void X11_wrapper::check_resize(XEvent *e)
{
	//The ConfigureNotify is sent by the
	//server if the window is resized.
	if (e->type != ConfigureNotify)
		return;
	XConfigureEvent xce = e->xconfigure;
	if (xce.width != g.xres || xce.height != g.yres) {
		//Window size did change.
		reshape_window(xce.width, xce.height);
	}
}
//-----------------------------------------------------------------------------

void make_particle(int p1, int p2)
{
    if (n >= MAX_PARTICLES)
	return;

    particles[n].w = 4.0;
    particles[n].pos[0] = p1;
    particles[n].pos[1] = p2;
    particles[n].vel[0] = particles[n].vel[1] = 0;
    n++;
}


void X11_wrapper::check_mouse(XEvent *e)
{
	static int savex = 0;
	static int savey = 0;

	//Weed out non-mouse events
	if (e->type != ButtonRelease &&
		e->type != ButtonPress &&
		e->type != MotionNotify) {
		//This is not a mouse event that we care about.
		return;
	}
	//
	if (e->type == ButtonRelease) {
		return;
	}
	if (e->type == ButtonPress) {
		while (e->xbutton.button==1) {
			//Left button was pressed.
			g.mouse_y = g.yres - e->xbutton.y;
			g.mouse_x = e->xbutton.x;
                        g.pressed = 1;
                        make_particle(g.mouse_x, g.mouse_y);
			return;
		}
		if (e->xbutton.button==3) {
			//Right button was pressed.
			return;
		}
	}
	if (e->type == MotionNotify) {
		//The mouse moved!
		if (savex != e->xbutton.x || savey != e->xbutton.y) {
			savex = e->xbutton.x;
			savey = e->xbutton.y;
			//Code placed here will execute whenever the mouse moves.


		}
	}
}

int X11_wrapper::check_keys(XEvent *e)
{
	if (e->type != KeyPress && e->type != KeyRelease)
		return 0;
	int key = XLookupKeysym(&e->xkey, 0);
	if (e->type == KeyPress) {
		switch (key) {
			case XK_1:
				//Key 1 was pressed
				break;
			case XK_Escape:
				//Escape key was pressed
				return 1;
		}
	}
	return 0;
}

void init_opengl(void)
{
	//OpenGL initialization
	glViewport(0, 0, g.xres, g.yres);
	//Initialize matrices
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	//Set 2D mode (no perspective)
	glOrtho(0, g.xres, 0, g.yres, -1, 1);
	//Set the screen background color
	glClearColor(0.1, 0.1, 0.1, 1.0);
}

void physics()
{
    	particle.vel[1] -= 0.01;
	particle.pos[0] += particle.vel[0];
	particle.pos[1] += particle.vel[1];
	
	if ((particle.pos[1] - particle.w) < (box.pos[1] + box.h) &&
	    particle.pos[1] > (box.pos[1] - box.h) &&
	    particle.pos[0] > (box.pos[0] - box.w) &&
	    particle.pos[0] < (box.pos[0] + box.w)) {
	    	particle.vel[1] = 0.0;
		particle.vel[0] += 0.01;
	}

	for (int i = 0; i < n; i++) {

    		particles[i].vel[1] -= 0.01;
		particles[i].pos[0] += particles[i].vel[0];
		particles[i].pos[1] += particles[i].vel[1];

                for (int j = 0; j < 5; j++) {
		        if ((particles[i].pos[1] - particles[i].w) < (boxes[j].pos[1] + boxes[j].h) &&
	    	        particles[i].pos[1] > (boxes[j].pos[1] - boxes[j].h) &&
	    	        particles[i].pos[0] > (boxes[j].pos[0] - boxes[j].w) &&
	    	        particles[i].pos[0] < (boxes[j].pos[0] + boxes[j].w)) {
	    		        particles[i].vel[1] = 0.0;
			        particles[i].vel[0] += 0.005;
                                if (particles[i].vel[0] > 2)
                                        particles[i].vel[0] = 2;
		        }
                }
                for (int k = 0; k < n; k++) {
                        if (k != i) {
		        if ((particles[i].pos[1] - particles[i].w) < (particles[k].pos[1] + particles[k].h) &&
	    	        particles[i].pos[1] > (particles[k].pos[1] - particles[k].h) &&
	    	        particles[i].pos[0] > (particles[k].pos[0] - particles[k].w) &&
	    	        particles[i].pos[0] < (particles[k].pos[0] + particles[k].w)) {
			        particles[i].vel[0] += 0.001;
                                if (particles[i].vel[0] > 0.1)
                                        particles[i].vel[0] = 0.1;
		        }

		        else if ((particles[i].pos[1] - particles[i].w) < (particles[k].pos[1] + particles[k].h) &&
	    	        particles[i].pos[1] > (particles[k].pos[1] - particles[k].h) &&
	    	        particles[i].pos[0] > (particles[k].pos[0] - particles[k].w) &&
	    	        particles[i].pos[0] < (particles[k].pos[0] + particles[k].w)) {
			        particles[i].vel[0] -= 0.01;
                        
                        }
                        }
                }

		if (particles[i].pos[1] < 0.0) {
		    particles[i] = particles[n-1];
		    n--;
		}
	}
}


void render()
{
        if (g.frame_count >= 10 || g.pressed)
                make_particle(g.mouse_x, g.mouse_y);
        else {
                g.frame_count++;
                if (g.frame_count > 10)
                        g.frame_count = 0;
        }

	glClear(GL_COLOR_BUFFER_BIT);
	//Draw box.
	glPushMatrix();
	glColor3ub(100, 255, 100);
	glTranslatef(box.pos[0], box.pos[1], 0.0f);
	glBegin(GL_QUADS);
		glVertex2f(-box.w, -box.h);
		glVertex2f(-box.w,  box.h);
		glVertex2f(box.w, box.h);
		glVertex2f(box.w, -box.h);
	glEnd();
	glPopMatrix();
	// Draw all boxes
	for (int i = 0; i < 5; i++) {
		glPushMatrix();
		glColor3ub(100, 255, 100);
		glTranslatef(boxes[i].pos[0], boxes[i].pos[1], 0.0f);
		glBegin(GL_QUADS);
			glVertex2f(-boxes[i].w, -boxes[i].h);
			glVertex2f(-boxes[i].w,  boxes[i].h);
			glVertex2f(boxes[i].w, boxes[i].h);
			glVertex2f(boxes[i].w, -boxes[i].h);
		glEnd();
		glPopMatrix();
	}
	// Draw particle
	glPushMatrix();
	glColor3ub(150, 160, 220);
	glTranslatef(particle.pos[0], particle.pos[1], 0.0f);
	glBegin(GL_QUADS);
		glVertex2f(-particle.w, -particle.w);
		glVertex2f(-particle.w, particle.w);
		glVertex2f(particle.w, particle.w);
		glVertex2f(particle.w, -particle.w);
	glEnd();
	glPopMatrix();
	// Draw all particles
	for (int i = 0; i < n; i++) {
		glPushMatrix();
		glColor3ub(150, 160, 220);
		glTranslatef(particles[i].pos[0], particles[i].pos[1], 0.0f);
		glBegin(GL_QUADS);
			glVertex2f(-particles[i].w, -particles[i].w);
			glVertex2f(-particles[i].w, particles[i].w);
			glVertex2f(particles[i].w, particles[i].w);
			glVertex2f(particles[i].w, -particles[i].w);
		glEnd();
		glPopMatrix();
	}
}






