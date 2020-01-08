#include <GL/glew.h>
#include <GL/glut.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stdexcept>
#include <iostream>
#include <string>
#include <vector>

#include "Shader.h"
#include "Mesh.h"
#include "Renderer.h"
#include "MassSpringSolver.h"
#include "UserInteraction.h"

// G L O B A L S ///////////////////////////////////////////////////////////////////

// Window
static int g_windowWidth = 640, g_windowHeight = 640;
static bool g_mouseClickDown = false;
static bool g_mouseLClickButton, g_mouseRClickButton, g_mouseMClickButton;
static int g_mouseClickX;
static int g_mouseClickY;

// User Interaction
static UserInteraction* UI;
static Renderer* g_pickRenderer;

// Constants
static const float PI = glm::pi<float>();

// Shader Handles
static PhongShader* g_phongShader; // linked phong shader
static PickShader* g_pickShader; // linked pick shader

// Shader parameters
static const glm::vec3 g_albedo(0.0f, 0.3f, 0.7f);
static const glm::vec3 g_ambient(0.01f, 0.01f, 0.01f);
static const glm::vec3 g_light(1.0f, 1.0f, -1.0f);

// Mesh
static Mesh* g_clothMesh; // halfedge data structure

// Render Target
static ProgramInput* g_render_target; // vertex, normal, texutre, index

// Animation
static const int g_fps = 60; // frames per second  | 60
static const int g_iter = 5; // iterations per time step | 10
static const int g_frame_time = 15; // approximate time for frame calculations | 15
static const int g_animation_timer = (int) ((1.0f / g_fps) * 1000 - g_frame_time);

// Mass Spring System
static mass_spring_system* g_system;
static MassSpringSolver* g_solver;

// System parameters
namespace SystemParam {
	static const int n = 61; // must be odd, n * n = n_vertices | 61
	static const float w = 2.0f; // width | 2.0f
	static const float h = 0.008f; // time step, smaller for better results | 0.008f = 0.016f/2
	static const float r = w / (n - 1) * 1.05f; // spring rest legnth
	static const float k = 1.0f; // spring stiffness | 1.0f;
	static const float m = 0.25f / (n * n); // point mass | 0.25f
	static const float a = 0.993f; // damping, close to 1.0 | 0.993f
	static const float g = 9.8f * m; // gravitational force | 9.8f
}

// Constraint Graph
static CgRootNode* g_cgRootNode;

// Scene parameters
static const float g_camera_distance = 4.2f;

// Scene matrices
static glm::mat4 g_ModelViewMatrix;
static glm::mat4 g_ProjectionMatrix;

// F U N C T I O N S //////////////////////////////////////////////////////////////
// state initialization
static void initGlutState(int, char**);
static void initGLState();

static void initShaders(); // Read, compile and link shaders
static void initCloth(); // Generate cloth mesh
static void initScene(); // Generate scene matrices

// demos
static void demo_hang(); // curtain hanging from top corners
static void demo_drop(); // curtain dropping on sphere
static void(*g_demo)() = demo_drop;

// glut callbacks
static void display();
static void reshape(int, int);
static void mouse(int, int, int, int);
static void motion(int, int);

// draw cloth function
static void drawCloth();
static void animateCloth(int value);

// scene update
static void updateProjection();
static void updateRenderTarget();

// cleaning
static void cleanUp();

// error checks
void checkGlErrors();



// M A I N //////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
	try {
		initGlutState(argc, argv);
		glewInit();
		initGLState();

		initShaders();
		initCloth();
		initScene();

		glutTimerFunc(g_animation_timer, animateCloth, 0);
		glutMainLoop();

		cleanUp();
		return 0;
	}
	catch (const std::runtime_error& e) {
		std::cout << "Exception caught: " << e.what() << std::endl;
		return -1;
	}
}


// S T A T E  I N I T I A L I Z A T O N /////////////////////////////////////////////
static void initGlutState(int argc, char** argv) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(g_windowWidth, g_windowHeight);
	glutCreateWindow("Cloth App");

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
}

static void initGLState() {
	glClearColor(0.25f, 0.25f, 0.25f, 0);
	glClearDepth(1.);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glReadBuffer(GL_BACK);
	glEnable(GL_FRAMEBUFFER_SRGB);

	checkGlErrors();
}

static void initShaders() {
	GLShader basic_vert(GL_VERTEX_SHADER);
	GLShader phong_frag(GL_FRAGMENT_SHADER);
	GLShader pick_frag(GL_FRAGMENT_SHADER);

	basic_vert.compile(std::ifstream("./shaders/basic.vshader"));
	phong_frag.compile(std::ifstream("./shaders/phong.fshader"));
	pick_frag.compile(std::ifstream("./shaders/pick.fshader"));

	g_phongShader = new PhongShader;
	g_pickShader = new PickShader;
	g_phongShader->link(basic_vert, phong_frag);
	g_pickShader->link(basic_vert, pick_frag);

	checkGlErrors();
}

static void initCloth() {
	// short hand
	const int n = SystemParam::n;
	const float w = SystemParam::w;

	// generate mesh
	MeshBuilder meshBuilder;
	meshBuilder.uniformGrid(w, n);
	g_clothMesh = meshBuilder.getResult();

	// fill program input
	g_render_target = new ProgramInput;
	g_render_target->setPositionData(g_clothMesh->vbuff(), g_clothMesh->vbuffLen());
	g_render_target->setNormalData(g_clothMesh->nbuff(), g_clothMesh->nbuffLen());
	g_render_target->setTextureData(g_clothMesh->tbuff(), g_clothMesh->tbuffLen());
	g_render_target->setIndexData(g_clothMesh->ibuff(), g_clothMesh->ibuffLen());

	// check errors
	checkGlErrors();

	// build demo system
	g_demo();
}

static void initScene() {
	g_ModelViewMatrix = glm::lookAt(
		glm::vec3(0.618, -0.786, 0.3f) * g_camera_distance,
		glm::vec3(0.0f, 0.0f, -1.0f),
		glm::vec3(0.0f, 0.0f, 1.0f)
	) * glm::translate(glm::mat4(1), glm::vec3(0.0f, 0.0f, SystemParam::w / 4));
	updateProjection();
}

static void demo_hang() {
	// short hand
	const int n = SystemParam::n;

	// initialize mass spring system
	MassSpringBuilder massSpringBuilder;
	massSpringBuilder.uniformGrid(
		SystemParam::n,
		SystemParam::h,
		SystemParam::r,
		SystemParam::k,
		SystemParam::m,
		SystemParam::a,
		SystemParam::g
	);
	g_system = massSpringBuilder.getResult();

	// initialize mass spring solver
	g_solver = new MassSpringSolver(g_system, g_clothMesh->vbuff());

	// deformation constraint parameters
	const float tauc = 0.4f; // critical spring deformation | 0.4f
	const unsigned int deformIter = 15; // number of iterations | 15

	// initialize constraints
	// spring deformation constraint
	CgSpringDeformationNode* deformationNode =
		new CgSpringDeformationNode(g_system, g_clothMesh->vbuff(), tauc, deformIter);
	deformationNode->addSprings(massSpringBuilder.getShearIndex());
	deformationNode->addSprings(massSpringBuilder.getStructIndex());

	// fix top corners
	CgPointFixNode* cornerFixer = new CgPointFixNode(g_system, g_clothMesh->vbuff());
	cornerFixer->fixPoint(0);
	cornerFixer->fixPoint(n - 1);

	// initialize user interaction
	g_pickRenderer = new Renderer();
	g_pickRenderer->setProgram(g_pickShader);
	g_pickRenderer->setProgramInput(g_render_target);
	g_pickRenderer->setElementCount(g_clothMesh->ibuffLen());
	g_pickShader->setTessFact(SystemParam::n);
	CgPointFixNode* mouseFixer = new CgPointFixNode(g_system, g_clothMesh->vbuff());
	UI = new GridMeshUI(g_pickRenderer, mouseFixer, g_clothMesh->vbuff(), n);

	// build constraint graph
	g_cgRootNode = new CgRootNode(g_system, g_clothMesh->vbuff());

	// first layer
	g_cgRootNode->addChild(deformationNode);

	// second layer
	deformationNode->addChild(cornerFixer);
	deformationNode->addChild(mouseFixer);
}

static void demo_drop() {
	// short hand
	const int n = SystemParam::n;

	// initialize mass spring system
	MassSpringBuilder massSpringBuilder;
	massSpringBuilder.uniformGrid(
		SystemParam::n,
		SystemParam::h,
		SystemParam::r,
		SystemParam::k,
		SystemParam::m,
		SystemParam::a,
		SystemParam::g
	);
	g_system = massSpringBuilder.getResult();

	// initialize mass spring solver
	g_solver = new MassSpringSolver(g_system, g_clothMesh->vbuff());

	// sphere collision constraint parameters
	const float radius = 0.64f; // sphere radius | 0.64f
	const Eigen::Vector3f center(0, 0, -1);// sphere center | (0, 0, -1)

	// deformation constraint parameters
	const float tauc = 0.12f; // critical spring deformation | 0.12f
	const unsigned int deformIter = 15; // number of iterations | 15

	// initialize constraints
	// sphere collision constraint
	CgSphereCollisionNode* sphereCollisionNode =
		new CgSphereCollisionNode(g_system, g_clothMesh->vbuff(), radius, center);

	// spring deformation constraint
	CgSpringDeformationNode* deformationNode =
		new CgSpringDeformationNode(g_system, g_clothMesh->vbuff(), tauc, deformIter);
	deformationNode->addSprings(massSpringBuilder.getShearIndex());
	deformationNode->addSprings(massSpringBuilder.getStructIndex());

	// initialize user interaction
	g_pickRenderer = new Renderer();
	g_pickRenderer->setProgram(g_pickShader);
	g_pickRenderer->setProgramInput(g_render_target);
	g_pickRenderer->setElementCount(g_clothMesh->ibuffLen());
	g_pickShader->setTessFact(SystemParam::n);
	CgPointFixNode* mouseFixer = new CgPointFixNode(g_system, g_clothMesh->vbuff());
	UI = new GridMeshUI(g_pickRenderer, mouseFixer, g_clothMesh->vbuff(), n);

	// build constraint graph
	g_cgRootNode = new CgRootNode(g_system, g_clothMesh->vbuff());

	// first layer
	g_cgRootNode->addChild(deformationNode);
	g_cgRootNode->addChild(sphereCollisionNode);

	// second layer
	deformationNode->addChild(mouseFixer);
}

// G L U T  C A L L B A C K S //////////////////////////////////////////////////////
static void display() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawCloth();
	glutSwapBuffers();

	checkGlErrors();
}

static void reshape(int w, int h) {
	g_windowWidth = w;
	g_windowHeight = h;
	glViewport(0, 0, w, h);
	updateProjection();
	glutPostRedisplay();
}

static void mouse(const int button, const int state, const int x, const int y) {
	g_mouseClickX = x;
	g_mouseClickY = g_windowHeight - y - 1;

	g_mouseLClickButton |= (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN);
	g_mouseRClickButton |= (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN);
	g_mouseMClickButton |= (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN);

	g_mouseLClickButton &= !(button == GLUT_LEFT_BUTTON && state == GLUT_UP);
	g_mouseRClickButton &= !(button == GLUT_RIGHT_BUTTON && state == GLUT_UP);
	g_mouseMClickButton &= !(button == GLUT_MIDDLE_BUTTON && state == GLUT_UP);

	g_mouseClickDown = g_mouseLClickButton || g_mouseRClickButton || g_mouseMClickButton;

	// TODO: move to UserInteraction class: add renderer member variable
	// pick point
	if (g_mouseLClickButton) {
		UI->setModelview(g_ModelViewMatrix);
		UI->setProjection(g_ProjectionMatrix);
		UI->grabPoint(g_mouseClickX, g_mouseClickY);
	}
	else UI->releasePoint();
}

static void motion(const int x, const int y) {
	const float dx = float(x - g_mouseClickX);
	const float dy = float (-(g_windowHeight - y - 1 - g_mouseClickY));

	if (g_mouseLClickButton) {
		//glm::vec3 ux(g_ModelViewMatrix * glm::vec4(1, 0, 0, 0));
		//glm::vec3 uy(g_ModelViewMatrix * glm::vec4(0, 1, 0, 0));
		glm::vec3 ux(0, 1, 0);
		glm::vec3 uy(0, 0, -1);
		UI->movePoint(0.01f * (dx * ux + dy * uy));
	}

	g_mouseClickX = x;
	g_mouseClickY = g_windowHeight - y - 1;
}

// C L O T H ///////////////////////////////////////////////////////////////////////
static void drawCloth() {
	Renderer renderer;
	renderer.setProgram(g_phongShader);
	renderer.setModelview(g_ModelViewMatrix);
	renderer.setProjection(g_ProjectionMatrix);
	g_phongShader->setAlbedo(g_albedo);
	g_phongShader->setAmbient(g_ambient);
	g_phongShader->setLight(g_light);
	renderer.setProgramInput(g_render_target);
	renderer.setElementCount(g_clothMesh->ibuffLen());
	renderer.draw();
}

static void animateCloth(int value) {

	// solve two time-steps
	g_solver->solve(g_iter);
	g_solver->solve(g_iter);

	// fix points
	CgSatisfyVisitor visitor;
	visitor.satisfy(*g_cgRootNode);

	// update normals
	g_clothMesh->request_face_normals();
	g_clothMesh->update_normals();
	g_clothMesh->release_face_normals();

	// update target
	updateRenderTarget();

	// redisplay
	glutPostRedisplay();

	// reset timer
	glutTimerFunc(g_animation_timer, animateCloth, 0);
}

// S C E N E  U P D A T E ///////////////////////////////////////////////////////////
static void updateProjection() {
	g_ProjectionMatrix = glm::perspective(PI / 4.0f,
		g_windowWidth * 1.0f / g_windowHeight, 0.01f, 1000.0f);
}

static void updateRenderTarget() {
	// update vertex positions
	g_render_target->setPositionData(g_clothMesh->vbuff(), g_clothMesh->vbuffLen());

	// update vertex normals
	g_render_target->setNormalData(g_clothMesh->nbuff(), g_clothMesh->vbuffLen());

}

// C L E A N  U P //////////////////////////////////////////////////////////////////
static void cleanUp() {
	// delete mesh
	delete g_clothMesh;

	// delete UI
	delete g_pickRenderer;
	delete UI;

	// delete render target
	delete g_render_target;

	// delete mass-spring system
	delete g_system;
	delete g_solver;

	// delete constraint graph
	// TODO
}

// E R R O R S /////////////////////////////////////////////////////////////////////
void checkGlErrors() {
	const GLenum errCode = glGetError();

	if (errCode != GL_NO_ERROR) {
		std::string error("GL Error: ");
		error += reinterpret_cast<const char*>(gluErrorString(errCode));
		std::cerr << error << std::endl;
		throw std::runtime_error(error);
	}
}