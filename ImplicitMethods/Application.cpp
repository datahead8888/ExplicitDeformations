//Cloth simulation using implicit methods to implement springs for a grid of particles
//Features:
//	*Implements springs between particles defined in an edge list using hooke's law with a damping component
//	*Has an earth gravity component to drag the cloth downwards
//	*Uses implicit methods, allowing for a higher spring constant (variable ks in class ParticleSystem)
//	*Uses a conjugate gradient solver to implement implicit methods
//	*Gourad shading for lighting
//Controls:
//Keyboard:
//	SPACE: Begin animating
//	A: Decrease rest length between particles
//	D: Increase rest length between particles
//	W: Increase earth gravity (starts at 9.8 m/sec)
//	S: Decrease earth gravity
//	Z: toggle wireframe mode
//	X: toggle informational text display
//  E: run an explicit implementation of the simulation (useful for comparison; most obvious if you turn automatic implicit animation off with space bar)
//  R: reset the simulation
//  I: render to a series of numbered images so that they can be combined into a video
//	O: toggle logging of positions/velocities of constrained particles (only if DEBUGGING macro is #defined in Logger.h)
//	P: toggle complete logging (only if DEBUGGING macro is #defined in Logger.h)
//Mouse:
//	Left button - hold this whie dragging the mouse to change the rotation angle of the piece of cloth shown
//	Middle button - zoom in
//	Right button - zoom out
//Written by Chris Jacobsen with advisement from Professor Huamin Wang

//Based on:
//Deformation Papers:
//http://www.math.ucla.edu/~jteran/papers/TSNF03.pdf � Finite Volume Methods for the Simulation of Skeletal Muscle
//By R. Fedkiw et. al
//Deformation Method #1 - Class Stanford System
//http://graphics.berkeley.edu/papers/Obrien-GMA-1999-08/Obrien-GMA-1999-08.pdf � Graphical Modeling and Animation of Brittle Fracture
//By James O'Brien and Jessica Hodgkins
//Deformation Method #2 - Class GeorgiaInstituteSystem
//http://www-ljk.imag.fr/Publications/Basilic/com.lmc.publi.PUBLI_Article@11f6a0378d9_18c74/tensile.pdf � Simple, yet Accurate Nonlinear Tensile Stiffness
//Pascal Volino et. al
//Deformation Method #3 - Class NonlinearMethodSystem

#include <gl/glut.h>
#include <gl/GLU.H>

#include <string>
#include <sstream>
#include <iostream>
#include "StanfordSystem.h"
#include "GeorgiaInstituteSystem.h"
#include "NonlinearMethodSystem.h"
#include "ViewManager.h"
#include "Keyboard.h"
#include "TetraMeshReader.h"

using namespace std;

//Note: the reason these were declared globally is to accommodate Glut's function calling system
ParticleSystem * particleSystem;	//The main particle system
ViewManager viewManager;			//Instance of the view manager to allow user view control
Keyboard * keyboard;				//Instance of the Keyboard class to process key presses
Logger * logger;					//Instance of Logger class to perform all logging
const int whichMethod = 1;			//1 for stanford method.  2 for georgia Institute Method.  3 for NonLinear Paper method.

//This function is called for rendering by GLUT
void render()
{
	//Timing mechanism for performance evaluation
	#ifdef DEBUGGING
	int startTime, endTime;
	if (logger -> isLogging)
	{
		startTime = glutGet(GLUT_ELAPSED_TIME);
	}
	#endif

	//Update Logic
	double timeElapsed;

	switch (whichMethod)
	{
	case 1:					//Stanford Method
		timeElapsed = 0.005;
		break;
	case 2:					//Georgia Institute Method
		timeElapsed = 0.005;
		break;
	case 3:					//Non Linear Paper Method
		timeElapsed = 0.0005;
		break;
	default:
		timeElapsed = 0.005;
		break;
	}

	//for (int i = 0; i < 1; i++)
	for (int i = 0; i < 10; i++)
	{				
		particleSystem -> doUpdate(timeElapsed);
	}
	
	//startTime = glutGet(GLUT_ELAPSED_TIME);
		
	particleSystem -> calculateNormals();

	//Render Logic
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLoadIdentity();

	viewManager.doTransform();
	particleSystem -> doRender(timeElapsed * 4);
	//particleSystem -> doRender(0.001);

	glFlush();

	glutSwapBuffers();

	#ifdef DEBUGGING
	if (logger -> isLogging)
	{
		//Calculate time this frame took to compute and render
		endTime = glutGet(GLUT_ELAPSED_TIME);
		if (logger -> loggingLevel >= logger -> LIGHT)
		{
			cout << "Total time for frame was: " << (double)(endTime - startTime) / 1000 << " seconds" << endl;
		}
	}

	#endif

}

//This function processes window resize events, ensuring the aspect ratio and anything else dependent on window dimensions is correct
//Parameters width and height are the dimensions of the window
void resize(int width, int height)
{
	//Minimizing the window results in sizes of 0, which causes problems with video generation.
	//Do not allow the width or height to be set to 0.
	if (width == 0 || height == 0)
	{
		return;
	}

	//Calculate the aspect ratio using the x and y dimensions
	double ar = (double) width / height;

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	//Set the Open GL viewport to the window's dimensions
	glViewport(0, 0, width, height);
	//Set up perspective.
	//45 degree viewing angle, use the above calculated aspect ratio,
	//set up near clipping plane to 0.1, the far clipping pane to 50.0
	//(Clipping plane values must be positive)
	gluPerspective(45.0, ar, 0.1, 50.0);

	//Go back to MODEL VIEW matrix mode
	glMatrixMode(GL_MODELVIEW);
	particleSystem -> setWindowDimensions(width,height);
}

//This function processes mouse clicks (when the button is pressed)
//Parameters:
//button - which button was pressed
//state - was it up or down?
//x - x coordinate of click
//y - y coordinte of click
void mouseClick(int button, int state, int x, int y)
{
	viewManager.mouseClick(button,state,x,y);
}

//This function processes mouse motion.
//It only takes action if a button is being held
//Parameters:
//x - x coordinate of movement
//y - y coordinate of movement
void mouseMove(int x, int y)
{
	viewManager.mouseMove(x,y);
}

//This function processes keyPress events (a keyboard button going down)
//Parameter key - the button being pressed
//The other parameters are not used
void keyPressed (unsigned char key, int mystery, int mystery2)
{
	keyboard -> keyPressed(key);
}

//This function processes keyRelease events (a keyboard button going up)
//Parameter key - the button being released
//The other parameters are not used
void keyReleased (unsigned char key, int mystery, int mystery2)
{
	keyboard -> keyReleased(key);
}

//Main function
int main(int argCount, char **argValue)
{
	logger = new Logger();

	int vertexCount = 0;
	int tetraCount = 0;
	Vertex * vertexList = NULL;
	int * tetraList = NULL;
	TetraMeshReader theReader;
	//if (theReader.openFile("hack.node", "hack.ele"))
	//if (theReader.openFile("house2.node", "house2.ele"))
	//if (theReader.openFile("dragon.node", "dragon.ele"))
	// (theReader.openFile("chris.node", "chris.ele"))
	//if (theReader.openFile("chris2.node", "chris2.ele"))
	//if (theReader.openFile("chris4.node", "chris4.ele"))
	if (theReader.openFile("chrisSimpler.node", "chrisSimpler.ele"))
	{
		bool loadSucceeded = theReader.loadData(vertexList, vertexCount, tetraList, tetraCount, logger);
		
		theReader.closeFile();

		if (loadSucceeded && vertexList != NULL && tetraList != NULL)
		{
			switch(whichMethod)
			{
			case 1:
				particleSystem = new StanfordSystem(vertexList, vertexCount, tetraList, tetraCount, logger);
				break;
			case 2:
				particleSystem = new GeorgiaInstituteSystem(vertexList, vertexCount, tetraList, tetraCount, logger);
				break;
			case 3:
				particleSystem = new NonlinearMethodSystem(vertexList, vertexCount, tetraList, tetraCount, logger);
				break;
			default:
				cerr << "Incorrect system identifier -- defaulting to stanford system" << endl;
				particleSystem = new StanfordSystem(vertexList, vertexCount, tetraList, tetraCount, logger);
				break;
			}


			//particleSystem -> loadSpecialState();
			
			keyboard = new Keyboard(particleSystem, logger);

			glutInit(&argCount,argValue);
			glutInitDisplayMode(GLUT_DEPTH|GLUT_DOUBLE | GLUT_RGBA);

			glutInitWindowPosition(0,0);
			int windowWidth = 1000;
			int windowHeight = 700;
			glutInitWindowSize(windowWidth, windowHeight);
			particleSystem -> setWindowDimensions(windowWidth, windowHeight);
			glutCreateWindow("Implicit Methods");

			//Note: this must be AFTER the create window function call to work properly
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_LEQUAL);
			glShadeModel(GL_SMOOTH);

			//Set up lighting attributes
			GLfloat lightPosition[] = {0.0, 0,0, 1.0, 0.0};
			GLfloat lightAmbient[] = {1.0f, 1.0f, 1.0f, 1.0f};
			GLfloat lightDiffuse[] = {1.0f, 1.0f, 1.0f, 1.0f};
			GLfloat lightSpecular[] = {1.0f, 1.0f, 1.0f, 1.0f};
			glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
			glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
			glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
			glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);

			GLfloat dummy[2];  //The glLightModelfv requires a pointer.  Since it's not zero here, this will result in two sided lighting
			//glLightModelfv(GL_LIGHT_MODEL_TWO_SIDE, dummy);

			GLfloat materialAmbientGreen[] = {0.1, 0.1, 0.1, 1.0}; //Just a little ambient color
			GLfloat materialDiffuseGreen[] = {0.0, 0.7, 0.0, 1.0};
			//GLfloat materialAmbientBlue[] = {0.0, 0.0, 0.0, 1.0};  //No ambient color
			GLfloat materialAmbientBlue[] = {0.1, 0.1, 0.1, 1.0};  //Just a little ambient color
			GLfloat materialDiffuseBlue[] = {0.0, 0.0, 0.7, 1.0};
			glMaterialfv(GL_BACK, GL_AMBIENT, materialAmbientBlue);
			glMaterialfv(GL_FRONT, GL_AMBIENT, materialAmbientGreen);
			glMaterialfv(GL_BACK, GL_DIFFUSE, materialDiffuseBlue);
			glMaterialfv(GL_FRONT, GL_DIFFUSE, materialDiffuseGreen);

			glEnable(GL_LIGHTING);
			glEnable(GL_LIGHT0);
			glClearColor(0.0f,0.0f,0.0f,0.0f);
	
			glutDisplayFunc(render);
			glutIdleFunc(render);
			glutReshapeFunc(resize);
			glutMouseFunc(mouseClick);
			glutMotionFunc(mouseMove);
			glutKeyboardFunc(keyPressed);
			glutKeyboardUpFunc(keyReleased);
			
			glutMainLoop();

			delete keyboard;
			delete particleSystem;
		}
		else
		{
			cerr << "Program cannot run with error in loading input data contents" << endl;
			system("pause");
		}
	}
	else
	{
		cerr << "Unable to execute program without input data" << endl;
		system("pause");
	}

	delete logger;

}