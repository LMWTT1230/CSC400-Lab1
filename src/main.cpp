/*
 * Program 4 example with diffuse and spline camera PRESS 'g'
 * CPE 471 Cal Poly Z. Wood + S. Sueda + I. Dunn (spline D. McGirr)
 */

#include <iostream>
#include <glad/glad.h>

#include "GLSL.h"
#include "Program.h"
#include "Shape.h"
#include "MatrixStack.h"
#include "WindowManager.h"
#include "Texture.h"
#include "stb_image.h"
#include "Bezier.h"
#include "Spline.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader/tiny_obj_loader.h>
#define PI 3.1415927

// value_ptr for glm
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>

using namespace std;
using namespace glm;

class Application : public EventCallbacks
{

public:

	WindowManager * windowManager = nullptr;

	// Our shader program - use this one for Blinn-Phong has diffuse
	std::shared_ptr<Program> prog;

	//Our shader program for textures
	std::shared_ptr<Program> texProg;

	//our geometry
	shared_ptr<Shape> dog;
	shared_ptr<Shape> sphere;
	shared_ptr<Shape> bone;
	shared_ptr<Shape> bone1;
	shared_ptr<Shape> bone2;
	shared_ptr<Shape> bone3;
	shared_ptr<Shape> bones[4]={bone, bone1, bone2, bone3};
	

	//global data for ground plane - direct load constant defined CPU data to GPU (not obj)
	GLuint GrndBuffObj, GrndNorBuffObj, GrndTexBuffObj, GIndxBuffObj;
	int g_GiboLen;
	//ground VAO
	GLuint GroundVertexArrayID;

	//the image to use as a texture (ground)
	shared_ptr<Texture> texture0;
	shared_ptr<Texture> texture1;	

	//animation data
	float lightTrans = 0;

	//camera
	double g_theta;
	double g_phi = atan(-2.0f / 8.0f);
	vec3 view = vec3(0, 0, 1);
	vec3 strafe = vec3(1, 0, 0);
	vec3 g_eye = vec3(0, 2, 4);
	vec3 g_lookAt = vec3(0, 0, -4);


	Spline splinepath[2];
	bool goCamera = false;

	void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		{
			glfwSetWindowShouldClose(window, GL_TRUE);
		}
		if (key == GLFW_KEY_Q && action == GLFW_PRESS){
			lightTrans += 0.5;
		}
		if (key == GLFW_KEY_E && action == GLFW_PRESS){
			lightTrans -= 0.5;
		}
		if (key == GLFW_KEY_Z && action == GLFW_PRESS) {
			glPolygonMode( GL_FRONT_AND_BACK, GL_LINE );
		}
		if (key == GLFW_KEY_G && action == GLFW_RELEASE) {
			goCamera = !goCamera;
		}

		const float cameraSpeed = 0.1f;
		vec3 strafeVec = cameraSpeed * vec3(1, 0, 0);
		vec3 dollyVec = cameraSpeed * vec3(0, 0, 1);
		    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        {
			dog->pos -= dollyVec;
            g_eye -= dollyVec;
            g_lookAt -= dollyVec;
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        {
			dog->pos += dollyVec;
            g_eye += dollyVec;
            g_lookAt += dollyVec;
        }
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        {
			dog->pos -= strafeVec;
            g_eye -= strafeVec;
            g_lookAt -= strafeVec;
        }
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        {
			dog->pos += strafeVec;
            g_eye += strafeVec;
            g_lookAt += strafeVec;
        }
    }
	}

	void mouseCallback(GLFWwindow *window, int button, int action, int mods)
	{
		double posX, posY;

		if (action == GLFW_PRESS)
		{
			 glfwGetCursorPos(window, &posX, &posY);
			 cout << "Pos X " << posX <<  " Pos Y " << posY << endl;
		}
	}

	void scrollCallback(GLFWwindow* window, double deltaX, double deltaY) {
		cout << "xDel + yDel " << deltaX << " " << deltaY << endl;
		cout << "g_phi + g_theta " << g_phi << " " << g_theta << endl;

		g_theta += PI * deltaX / 200;

		g_phi += PI * deltaY/ 100;

		if (g_phi < -PI * 30 / 180) {
			g_phi = -PI * 30 / 180;
		}
		else if (g_phi > atan(-2.0f / 8.0f) + PI * 10 / 180) {
			g_phi = atan(-2.0f / 8.0f) + PI * 10 / 180;
		}

		view = -normalize(vec3(cos(g_phi) * cos(g_theta), sin(g_phi), cos(g_phi) * cos((PI / 2.0) - g_theta)));
		g_lookAt = g_eye + vec3(cos(g_phi) * cos(g_theta), sin(g_phi), cos(g_phi) * cos((PI / 2.0) - g_theta));
	}


	void resizeCallback(GLFWwindow *window, int width, int height)
	{
		glViewport(0, 0, width, height);
	}

	void init(const std::string& resourceDirectory)
	{
		GLSL::checkVersion();

		// Set background color.
		glClearColor(.72f, .84f, 1.06f, 1.0f);
		// Enable z-buffer test.
		glEnable(GL_DEPTH_TEST);

		g_theta = -PI / 2;

		// Initialize the GLSL program that we will use for local shading
		prog = make_shared<Program>();
		prog->setVerbose(true);
		prog->setShaderNames(resourceDirectory + "/simple_vert.glsl", resourceDirectory + "/simple_frag.glsl");
		prog->init();
		prog->addUniform("P");
		prog->addUniform("V");
		prog->addUniform("M");
		prog->addUniform("MatAmb");
		prog->addUniform("MatDif");
		prog->addUniform("MatSpec");
		prog->addUniform("MatShine");
		prog->addUniform("lightPos");
		prog->addAttribute("vertPos");
		prog->addAttribute("vertNor");
		prog->addAttribute("vertTex"); //silence error


		// Initialize the GLSL program that we will use for texture mapping
		texProg = make_shared<Program>();
		texProg->setVerbose(true);
		texProg->setShaderNames(resourceDirectory + "/tex_vert.glsl", resourceDirectory + "/tex_frag0.glsl");
		texProg->init();
		texProg->addUniform("P");
		texProg->addUniform("V");
		texProg->addUniform("M");
		texProg->addUniform("flip");
		texProg->addUniform("Texture0");
		texProg->addUniform("MatShine");
		texProg->addUniform("lightPos");
		texProg->addAttribute("vertPos");
		texProg->addAttribute("vertNor");
		texProg->addAttribute("vertTex");

		//read in a load the texture
		texture0 = make_shared<Texture>();
  		texture0->setFilename(resourceDirectory + "/flowers.jpg");
  		texture0->init();
  		texture0->setUnit(0);
  		texture0->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

  		texture1 = make_shared<Texture>();
  		texture1->setFilename(resourceDirectory + "/skyBox/top.jpg");
  		texture1->init();
  		texture1->setUnit(1);
  		texture1->setWrapModes(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);

  		// init splines up and down
       splinepath[0] = Spline(glm::vec3(-6,0,5), glm::vec3(-1,-5,5), glm::vec3(1, 5, 5), glm::vec3(2,0,5), 5);
       splinepath[1] = Spline(glm::vec3(2,0,5), glm::vec3(3,-2,5), glm::vec3(-0.25, 0.25, 5), glm::vec3(0,0,5), 5);
    
	}

	void initGeom(const std::string& resourceDirectory)
	{
		string errStr;

		//load in another mesh and make the shape(s)
		vector<tinyobj::shape_t> TOshapes1;
		vector<tinyobj::material_t> objMaterials1;
		bool rc = tinyobj::LoadObj(TOshapes1, objMaterials1, errStr, (resourceDirectory + "/dog.obj").c_str());

		if (!rc) {
			cerr << errStr << endl;
		}
		else {
			//for now all our shapes will not have textures - change in later labs
			dog = make_shared<Shape>();
			dog->createShape(TOshapes1[0]);
			dog->measure();
			dog->init();
			dog->pos = vec3(-0.2, -0.4, -0.5);
			dog->scale = vec3(0.3, 0.3, 0.3);
		}

		vector<tinyobj::shape_t> TOshapes2;
		vector<tinyobj::material_t> objMaterials2;
		//load in the mesh and make the shape(s)
		rc = tinyobj::LoadObj(TOshapes2, objMaterials2, errStr, (resourceDirectory + "/sphereWTex.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		}
		else {
			sphere = make_shared<Shape>();
			sphere->createShape(TOshapes2[0]);
			sphere->measure();
			sphere->init();
			sphere->scale = vec3(10.0);
		}

		vector<tinyobj::shape_t> TOshapes3;
		vector<tinyobj::material_t> objMaterials3;
		//load in the mesh and make the shape(s)
		rc = tinyobj::LoadObj(TOshapes3, objMaterials3, errStr, (resourceDirectory + "/bone.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		}
		else {
			bones[0] = make_shared<Shape>();
			bones[0]->createShape(TOshapes3[0]);
			bones[0]->measure();
			bones[0]->init();
			bones[0]->pos = vec3(4, 0, 3);
			bones[0]->scale = vec3(4, 3, 3);
		}

		vector<tinyobj::shape_t> TOshapes4;
		vector<tinyobj::material_t> objMaterials4;
		//load in the mesh and make the shape(s)
		rc = tinyobj::LoadObj(TOshapes4, objMaterials4, errStr, (resourceDirectory + "/bone.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		}
		else {
			bones[1] = make_shared<Shape>();
			bones[1]->createShape(TOshapes4[0]);
			bones[1]->measure();
			bones[1]->init();
			bones[1]->pos = vec3(-3, 0, 4);
			bones[1]->scale = vec3(4, 3, 3);
		}

		vector<tinyobj::shape_t> TOshapes5;
		vector<tinyobj::material_t> objMaterials5;
		//load in the mesh and make the shape(s)
		rc = tinyobj::LoadObj(TOshapes5, objMaterials5, errStr, (resourceDirectory + "/bone.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		}
		else {
			bones[2] = make_shared<Shape>();
			bones[2]->createShape(TOshapes5[0]);
			bones[2]->measure();
			bones[2]->init();
			bones[2]->pos = vec3(2, 0, -4);
			bones[2]->scale = vec3(4, 3, 3);
		}

		vector<tinyobj::shape_t> TOshapes6;
		vector<tinyobj::material_t> objMaterials6;
		//load in the mesh and make the shape(s)
		rc = tinyobj::LoadObj(TOshapes6, objMaterials6, errStr, (resourceDirectory + "/bone.obj").c_str());
		if (!rc) {
			cerr << errStr << endl;
		}
		else {
			bones[3] = make_shared<Shape>();
			bones[3]->createShape(TOshapes6[0]);
			bones[3]->measure();
			bones[3]->init();
			bones[3]->pos = vec3(-4, 0, -3);
			bones[3]->scale = vec3(4, 3, 3);
		}

		//code to load in the ground plane (CPU defined data passed to GPU)
		initGround();


	}

	//directly pass quad for the ground to the GPU
	void initGround() {

		float g_groundSize = 20;
		float g_groundY = -0.25;

  		// A x-z plane at y = g_groundY of dimension [-g_groundSize, g_groundSize]^2
		float GrndPos[] = {
			-g_groundSize, g_groundY, -g_groundSize,
			-g_groundSize, g_groundY,  g_groundSize,
			g_groundSize, g_groundY,  g_groundSize,
			g_groundSize, g_groundY, -g_groundSize
		};

		float GrndNorm[] = {
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0,
			0, 1, 0
		};

		static GLfloat GrndTex[] = {
      		0, 0, // back
      		0, 1,
      		1, 1,
      		1, 0 };

      	unsigned short idx[] = {0, 1, 2, 0, 2, 3};

		//generate the ground VAO
      	glGenVertexArrays(1, &GroundVertexArrayID);
      	glBindVertexArray(GroundVertexArrayID);

      	g_GiboLen = 6;
      	glGenBuffers(1, &GrndBuffObj);
      	glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
      	glBufferData(GL_ARRAY_BUFFER, sizeof(GrndPos), GrndPos, GL_STATIC_DRAW);

      	glGenBuffers(1, &GrndNorBuffObj);
      	glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
      	glBufferData(GL_ARRAY_BUFFER, sizeof(GrndNorm), GrndNorm, GL_STATIC_DRAW);

      	glGenBuffers(1, &GrndTexBuffObj);
      	glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
      	glBufferData(GL_ARRAY_BUFFER, sizeof(GrndTex), GrndTex, GL_STATIC_DRAW);

      	glGenBuffers(1, &GIndxBuffObj);
     	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
      	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
      }

      //code to draw the ground plane
	void drawGround(shared_ptr<Program> curS) {
		curS->bind();
		glBindVertexArray(GroundVertexArrayID);
		texture0->bind(curS->getUniform("Texture0"));
		//draw the ground plane 
		SetModel(vec3(0, -1, 0), 0, 0, 0.8, curS);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, GrndBuffObj);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(1);
		glBindBuffer(GL_ARRAY_BUFFER, GrndNorBuffObj);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

		glEnableVertexAttribArray(2);
		glBindBuffer(GL_ARRAY_BUFFER, GrndTexBuffObj);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);

		// draw!
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, GIndxBuffObj);
		glDrawElements(GL_TRIANGLES, g_GiboLen, GL_UNSIGNED_SHORT, 0);

		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
		glDisableVertexAttribArray(2);
		curS->unbind();
	}

     //helper function to pass material data to the GPU
	void SetMaterial(shared_ptr<Program> curS, int i) {

		switch (i) {
		case 0: //
			glUniform3f(curS->getUniform("MatAmb"), 0.01, 0.01, 0.01);
			glUniform3f(curS->getUniform("MatDif"), 0.3, 0.3, 0.3);
			glUniform3f(curS->getUniform("MatSpec"), 0.45, 0.23, 0.45);
			glUniform1f(curS->getUniform("MatShine"), 120.0);
			break;
		case 1: // 
			glUniform3f(curS->getUniform("MatAmb"), 0.1, 0.1, 0.1);
			glUniform3f(curS->getUniform("MatDif"), 0.5, 0.5, 0.5);
			glUniform3f(curS->getUniform("MatSpec"), 0.3, 0.2, 0.5);
			glUniform1f(curS->getUniform("MatShine"), 4.0);
			break;
		case 2: //
			glUniform3f(curS->getUniform("MatAmb"), 0.004, 0.09, 0.01);
			glUniform3f(curS->getUniform("MatDif"), 0.4, 0.9, 0.1);
			glUniform3f(curS->getUniform("MatSpec"), 0.02, 0.25, 0.45);
			glUniform1f(curS->getUniform("MatShine"), 27.9);
			break;
		case 3: //
			glUniform3f(curS->getUniform("MatAmb"), 0.01, 0.01, 0.01);
			glUniform3f(curS->getUniform("MatDif"), 0.9, 0.1, 0.1);
			glUniform3f(curS->getUniform("MatSpec"), 0.45, 0.23, 0.45);
			glUniform1f(curS->getUniform("MatShine"), 120.0);
			break;
		case 4: //
			glUniform3f(curS->getUniform("MatAmb"), 0.01, 0.01, 0.01);
			glUniform3f(curS->getUniform("MatDif"), 1.0, 0.5, 0.0);
			glUniform3f(curS->getUniform("MatSpec"), 0.45, 0.23, 0.45);
			glUniform1f(curS->getUniform("MatShine"), 120.0);
			break;
		}
	}

	/* helper function to set model trasnforms */
  	void SetModel(vec3 trans, float rotY, float rotX, float sc, shared_ptr<Program> curS) {
  		mat4 Trans = glm::translate( glm::mat4(1.0f), trans);
  		mat4 RotX = glm::rotate( glm::mat4(1.0f), rotX, vec3(1, 0, 0));
  		mat4 RotY = glm::rotate( glm::mat4(1.0f), rotY, vec3(0, 1, 0));
  		mat4 ScaleS = glm::scale(glm::mat4(1.0f), vec3(sc));
  		mat4 ctm = Trans*RotX*RotY*ScaleS;
  		glUniformMatrix4fv(curS->getUniform("M"), 1, GL_FALSE, value_ptr(ctm));
  	}

	void setModel(std::shared_ptr<Program> prog, std::shared_ptr<MatrixStack>M) {
		glUniformMatrix4fv(prog->getUniform("M"), 1, GL_FALSE, value_ptr(M->topMatrix()));
   	}

   	/* camera controls - do not change */
	void SetView(shared_ptr<Program>  shader) {
  		glm::mat4 Cam = glm::lookAt(g_eye, g_lookAt, vec3(0, 1, 0));
  		glUniformMatrix4fv(shader->getUniform("V"), 1, GL_FALSE, value_ptr(Cam));
	}

   	/* code to draw waving hierarchical model */
  // 	void drawHierModel(shared_ptr<MatrixStack> Model, shared_ptr<Program> prog) {
  // 		// simplified for releaes code
		//Model->pushMatrix();
		//	Model->loadIdentity();
		//	Model->translate(vec3(0, 0, -6));
		//	Model->scale(vec3(2.3));
		//	setModel(prog, Model);
		//	sphere->draw(prog);
		//Model->popMatrix();
  // 	}


   	void updateUsingCameraPath(float frametime)  {

   	  if (goCamera) {
       if(!splinepath[0].isDone()){
       		splinepath[0].update(frametime);
            g_eye = splinepath[0].getPosition();
        } else {
            splinepath[1].update(frametime);
            g_eye = splinepath[1].getPosition();
        }
      }
   	}

	bool CheckCollision(shared_ptr<Shape> one, shared_ptr<Shape> two) {
		float expansionX = 3.0;
		bool collisionX = one->pos.x + one->max.x * one->scale.x * expansionX >= two->pos.x + two->min.x * two->scale.x &&
			two->pos.x + two->max.x * two->scale.x * expansionX >= one->pos.x + one->min.x * one->scale.x;
		bool collisionY = one->pos.y + one->max.y * one->scale.y >= two->pos.y + two->min.y * two->scale.y &&
			two->pos.y + two->max.y * two->scale.y >= one->pos.y + one->min.y * one->scale.y;
		bool collisionZ = one->pos.z + one->max.z * one->scale.z >= two->pos.z + two->min.z * two->scale.z &&
			two->pos.z + two->max.z * two->scale.z >= one->pos.z + one->min.z * one->scale.z;

		return collisionX && collisionY && collisionZ;
	}


	bool CheckCollisionSkyBox(shared_ptr<Shape> obj, shared_ptr<Shape> skybox) {
		bool collisionX = obj->pos.x + obj->max.x * obj->scale.x >= skybox->pos.x + skybox->max.x * skybox->scale.x ||
			obj->pos.x + obj->min.x * obj->scale.x <= skybox->pos.x + skybox->min.x * skybox->scale.x;
		bool collisionY = obj->pos.y + obj->max.y * obj->scale.y >= skybox->pos.y + skybox->max.y * skybox->scale.y ||
			obj->pos.y + obj->min.y * obj->scale.y <= skybox->pos.y + skybox->min.y * skybox->scale.y;
		bool collisionZ = obj->pos.z + obj->max.z * obj->scale.z >= skybox->pos.z + skybox->max.z * skybox->scale.z ||
			obj->pos.z + obj->min.z * obj->scale.z <= skybox->pos.z + skybox->min.z * skybox->scale.z;

		return collisionX || collisionY || collisionZ;
	}

	void render(float frametime) {
		// Get current frame buffer size.
		int width, height;
		glfwGetFramebufferSize(windowManager->getHandle(), &width, &height);
		glViewport(0, 0, width, height);

		// Clear framebuffer.
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		//Use the matrix stack for Lab 6
		float aspect = width/(float)height;

		// Create the matrix stacks - please leave these alone for now
		auto Projection = make_shared<MatrixStack>();
		auto Model = make_shared<MatrixStack>();

		//update the camera position
		updateUsingCameraPath(frametime);

		// Apply perspective projection.
		Projection->pushMatrix();
		Projection->perspective(45.0f, aspect, 0.01f, 100.0f);

		// Draw the doggos
		texProg->bind();
		glUniformMatrix4fv(texProg->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		SetView(texProg);

		//draw big background sphere
		glUniform1i(texProg->getUniform("flip"), 0);
		texture1->bind(texProg->getUniform("Texture0"));
		Model->pushMatrix();
			Model->loadIdentity();
			Model->scale(sphere->scale);
			setModel(texProg, Model);
			sphere->draw(texProg);
		Model->popMatrix();

		glUniform1i(texProg->getUniform("flip"), 1);
		drawGround(texProg);

		texProg->unbind();

		//use the material shader
		prog->bind();
		glUniformMatrix4fv(prog->getUniform("P"), 1, GL_FALSE, value_ptr(Projection->topMatrix()));
		SetView(prog);
		glUniform3f(prog->getUniform("lightPos"), lightTrans, 2.0, 2.0);

		//use helper function that uses glm to create some transform matrices

		if (CheckCollisionSkyBox(dog, sphere)) {
			//cout << "colliding" << endl;
			dog->pos = vec3(-0.2, -0.4, -0.5);
			g_eye = vec3(0, 2, 4);
			g_lookAt = vec3(0, 0, -4);
			g_theta = -PI / 2;
			g_phi = atan(-2.0f / 8.0f);
		}

		Model->pushMatrix();
			Model->translate(dog->pos);
			Model->rotate(PI, vec3(0, 1, 0));
			Model->scale(dog->scale);
			setModel(prog, Model);
			SetMaterial(prog, 2);
			dog->draw(prog);
		Model->popMatrix();

		for (auto bone : bones) {
			if (CheckCollision(dog, bone)) {
				bone->Destroyed = true;
			}
			Model->pushMatrix();
				Model->translate(bone->pos);
				Model->rotate(PI / 2, vec3(0, 0, 1));
				Model->scale(bone->scale);
				setModel(prog, Model);
				SetMaterial(prog, 1);
				if (!bone->Destroyed) {
					bone->draw(prog);
				}
			Model->popMatrix();
		}
		/*if (CheckCollision(dog, bones[0])) {
			bones[0]->Destroyed = true;
		}

		if (CheckCollision(dog, bones[1])) {
			bones[1]->Destroyed = true;
		}

		if (CheckCollision(dog, bones[2])) {
			bones[2]->Destroyed = true;
		}

		if (CheckCollision(dog, bones[3])) {
			bones[3]->Destroyed = true;
		}*/
		
		Model->pushMatrix();
			Model->translate(bones[0]->pos);
			Model->rotate(PI / 2, vec3(0, 0, 1));
			Model->scale(bones[0]->scale);
			setModel(prog, Model);
			SetMaterial(prog, 1);
			if (!bones[0]->Destroyed) {
				bones[0]->draw(prog);
			}
		Model->popMatrix();

		//Model->pushMatrix();
		//	Model->translate(bones[1]->pos);
		//	Model->rotate(PI / 2, vec3(0, 0, 1));
		//	Model->scale(bones[1]->scale);
		//	setModel(prog, Model);
		//	SetMaterial(prog, 1);
		//	if (!bones[1]->Destroyed) {
		//		bones[1]->draw(prog);
		//	}
		//Model->popMatrix();

		//Model->pushMatrix();
		//	Model->translate(bones[2]->pos);
		//	Model->rotate(PI / 2, vec3(0, 0, 1));
		//	Model->scale(bones[2]->scale);
		//	setModel(prog, Model);
		//	SetMaterial(prog, 1);
		//	if (!bones[2]->Destroyed) {
		//		bones[2]->draw(prog);
		//	}
		//Model->popMatrix();

		//Model->pushMatrix();
		//	Model->translate(bones[3]->pos);
		//	Model->rotate(PI / 2, vec3(0, 0, 1));
		//	Model->scale(bones[3]->scale);
		//	setModel(prog, Model);
		//	SetMaterial(prog, 1);
		//	if (!bones[3]->Destroyed) {
		//		bones[3]->draw(prog);
		//	}
		//Model->popMatrix();
		

		prog->unbind();


		// Pop matrix stacks.
		Projection->popMatrix();

	}
};

int main(int argc, char *argv[])
{
	// Where the resources are loaded from
	std::string resourceDir = "../resources";

	if (argc >= 2)
	{
		resourceDir = argv[1];
	}

	Application *application = new Application();

	// Your main will always include a similar set up to establish your window
	// and GL context, etc.

	WindowManager *windowManager = new WindowManager();
	windowManager->init(640, 480);
	windowManager->setEventCallbacks(application);
	application->windowManager = windowManager;

	// This is the code that will likely change program to program as you
	// may need to initialize or set up different data and state

	application->init(resourceDir);
	application->initGeom(resourceDir);

	auto lastTime = chrono::high_resolution_clock::now();
	// Loop until the user closes the window.
	while (! glfwWindowShouldClose(windowManager->getHandle()))
	{
		// save current time for next frame
		auto nextLastTime = chrono::high_resolution_clock::now();

		// get time since last frame
		float deltaTime =
			chrono::duration_cast<std::chrono::microseconds>(
				chrono::high_resolution_clock::now() - lastTime)
				.count();
		// convert microseconds (weird) to seconds (less weird)
		deltaTime *= 0.000001;

		// reset lastTime so that we can calculate the deltaTime
		// on the next frame
		lastTime = nextLastTime;

		// Render scene.
		application->render(deltaTime);
		// Swap front and back buffers.
		glfwSwapBuffers(windowManager->getHandle());
		// Poll for and process events.
		glfwPollEvents();
	}

	// Quit program.
	windowManager->shutdown();
	return 0;
}
