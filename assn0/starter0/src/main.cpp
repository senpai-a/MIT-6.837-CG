#include "gl.h"
#include <GLFW/glfw3.h>

#include <cmath>
#include <vector>
#include <cassert>
#include <iostream>
#include <sstream>
#include <cstdio>
#include <vecmath.h>
#include "starter0_util.h"
#include "recorder.h"
#include "teapot.h"

using namespace std;

// Globals
uint32_t program;
// This is the list of points (3D vectors)
vector<Vector3f> vecv;
// This is the list of normals (also 3D vectors)
vector<Vector3f> vecn;
// This is the list of faces (indices into vecv and vecn)
vector<vector<unsigned>> vecf;//[[v1,n1,v2,n2,v3,n3],..]

// You will need more global variables to implement color and position changes
uint32_t colorIdx = 0;
GLfloat currentColor[4] = {0.5f, 0.5f, 0.9f, 1.0f};
GLfloat lightPos[] = { 2.0f, 3.0f, 5.0f, 1.0f };

bool rotating = false;
float rotationy = 0,rotationx = 0;
float currentRotationy = 0,currentRotationx = 0;
float scale = 1;
float currentScale = 1;

float transfromSpeed = .2;
float colorSpeed = .02;

bool dragging = false;
double dragFromX,dragFromY;

void mousePosCallback(GLFWwindow *wnd, double x,double y){
    printf("\rx: %lf, y: %lf",x,y);
    if (dragging){
        double dx = x-dragFromX, dy = y-dragFromY;
        rotationx+= dy;
        rotationy+= dx;
        glfwGetCursorPos(wnd,&dragFromX,&dragFromY);
    }
};
void scrollCallback(GLFWwindow *wnd, double x,double y){
    //printf("\nscroll x: %lf, y: %lf\n",x,y);
    if(y<0) scale-=0.05;
    else if (y>0) scale+=.05;
    if (scale<=0) scale = .01;
};
void mouseBtnCallback(GLFWwindow *wnd,int btn, int action, int mods){
    //printf("\nmouse btn:%d action:%d mod:%d\n",btn,action,mods);
    if (btn==GLFW_MOUSE_BUTTON_1){
        if (action==GLFW_PRESS){
            dragging = true;
            glfwGetCursorPos(wnd,&dragFromX,&dragFromY);
        }            
        else if (action==GLFW_RELEASE){
            dragging = false;
        }
    }
    else if (btn == GLFW_MOUSE_BUTTON_2){
        if (action==GLFW_PRESS){
            rotationx = 0;
            rotationy = 0;
        }
    }
    else if (btn == GLFW_MOUSE_BUTTON_3){
        if (action==GLFW_PRESS){
            scale = 1;
        }
    }
    
}

void keyCallback(GLFWwindow* window, int key,
    int scancode, int action, int mods)
{
    if (action == GLFW_RELEASE) { // only handle PRESS and REPEAT
        return;
    }

    // Special keys (arrows, CTRL, ...) are documented
    // here: http://www.glfw.org/docs/latest/group__keys.html
    if (key == GLFW_KEY_ESCAPE) {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    } else if (key == 'A') {
        printf("Key A pressed\n");
    } else if (key == 'C') {
        colorIdx += 1 ;  
    } else if (key == 263) { //←
        lightPos[0] -= .5f ;
    } else if (key == 262) { //→
        lightPos[0] += .5f ;
    } else if (key == 265) {   //↑
        lightPos[1] += .5f ;
    } else if (key == 264) {   //↓
        lightPos[1] -= .5f ;
    } else if (key == 'W') {
        lightPos[2] -= .5f ;
    } else if (key == 'S') {
        lightPos[2] += .5f ;
    } else if (key == 'Z') {
        lightPos[0] = 2.0f ;
        lightPos[1] = 3.0f ;
        lightPos[2] = 5.0f ;
    } else if (key == 'R') {
        rotating = !rotating;
    }
    else {
        printf("Unhandled key press %d\n", key);
    }
}

void drawTriangle()
{
    // set a reasonable upper limit for the buffer size
    GeometryRecorder rec(1024);
    rec.record(Vector3f(0.0, 0.0, 0.0), // Position
        Vector3f(0.0, 0.0, 1.0));// Normal

    rec.record(Vector3f(3.0, 0.0, 0.0),
        Vector3f(0.0, 0.0, 1.0));

    rec.record(Vector3f(3.0, 3.0, 0.0),
        Vector3f(0.0, 0.0, 1.0));
    rec.draw();
}

void drawTeapot()
{
    // set the required buffer size exactly.
    GeometryRecorder rec(teapot_num_faces * 3);
    for (int idx : teapot_indices) {
        Vector3f position(teapot_positions[idx * 3 + 0],
            teapot_positions[idx * 3 + 1],
            teapot_positions[idx * 3 + 2]);

        Vector3f normal(teapot_normals[idx * 3 + 0],
            teapot_normals[idx * 3 + 1],
            teapot_normals[idx * 3 + 2]);

        rec.record(position, normal);
    }
    rec.draw();
}

void drawObjMesh() {
    // draw obj mesh here
    // read vertices and face indices from vecv, vecn, vecf
    int nFaces = vecf.size();
    
    GeometryRecorder rec(nFaces*3);
    for (auto it=vecf.begin();it!=vecf.end();it++) {//vecf[i]:v1,n1,v2,n2,v3,n3
        auto face = *it;
        rec.record(vecv[face[0]-1], vecn[face[1]-1]);
        rec.record(vecv[face[2]-1], vecn[face[3]-1]);
        rec.record(vecv[face[4]-1], vecn[face[5]-1]);
    }
    rec.draw();
}

// This function is responsible for displaying the object.
void drawScene()
{
    drawObjMesh();
    //drawTeapot();
}

void setViewport(GLFWwindow* window)
{
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    // make sure the viewport is square-shaped.
    if (width > height) {
        int offsetx = (width - height) / 2;
        glViewport(offsetx, 0, height, height);
    } else {
        int offsety = (height - width) / 2;
        glViewport(0, offsety, width, width);
    }
}

void updateCameraUniforms()
{
    // Set up a perspective view, with square aspect ratio
    float fovy_radians = deg2rad(50.0f);
    float nearz = 1.0f;
    float farz = 100.0f;
    float aspect = 1.0f;
    Matrix4f P = Matrix4f::perspectiveProjection(
        fovy_radians, aspect, nearz, farz);

    // See https://www.opengl.org/sdk/docs/man/html/glUniform.xhtml
    // for the many version of glUniformXYZ()
    // Returns -1 if uniform not found.
    int loc = glGetUniformLocation(program, "P");
    glUniformMatrix4fv(loc, 1, false, P);

    Vector3f eye(0.0, 0.0, 7.0f);
    Vector3f center(0.0, 0.0, 0.0);
    Vector3f up(0.0, 1.0f, -0.2f);
    Matrix4f V = Matrix4f::lookAt(eye, center, up);
    loc = glGetUniformLocation(program, "V");
    glUniformMatrix4fv(loc, 1, false, V);
    loc = glGetUniformLocation(program, "camPos");
    glUniform3fv(loc, 1, eye);

    // Make sure the model is centered in the viewport
    // We translate the model using the "Model" matrix
    Matrix4f M = Matrix4f::translation(0, -2.0, 0);
    if (rotating){
        rotationy += 1.0;
    }
    currentRotationy = currentRotationy*(1-transfromSpeed)+rotationy*transfromSpeed;
    currentRotationx = currentRotationx*(1-transfromSpeed)+rotationx*transfromSpeed;
    currentScale = currentScale*(1-transfromSpeed)+scale*transfromSpeed;
    Matrix4f Ry = Matrix4f::rotateY(deg2rad(currentRotationy));
    Matrix4f Rx = Matrix4f::rotateX(deg2rad(currentRotationx));
    Matrix4f S = Matrix4f::scaling(currentScale,currentScale,currentScale);
    M = M*S*Rx*Ry;
    loc = glGetUniformLocation(program, "M");
    glUniformMatrix4fv(loc, 1, false, M);
    
    // Transformation matrices act differently
    // on vectors than on points.
    // The inverse-transpose is what we want.
    Matrix4f N = M.inverse().transposed();
    loc = glGetUniformLocation(program, "N");
    glUniformMatrix4fv(loc, 1, false, N);
}

void updateMaterialUniforms()
{
    #define nColors (4)
    // Here are some colors you might use - feel free to add more
    GLfloat diffColors[nColors][4] = { 
    { 0.5f, 0.5f, 0.9f, 1.0f },
    { 0.9f, 0.5f, 0.5f, 1.0f },
    { 0.5f, 0.9f, 0.3f, 1.0f },
    { 0.3f, 0.8f, 0.9f, 1.0f } };

    // Here we use the first color entry as the diffuse color
    int loc = glGetUniformLocation(program, "diffColor");
    for(int i=0;i<4;i++){
        GLfloat t = diffColors[colorIdx % nColors][i];
        currentColor[i] += (t - currentColor[i])*colorSpeed;//expotentially change color
    }
    glUniform4fv(loc, 1, currentColor);
    #undef nColors
    // Define specular color and shininess
    GLfloat specColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat shininess[] = { 10.0f };

    // Note that the specular color and shininess can stay constant
    loc = glGetUniformLocation(program, "specColor");
    glUniform4fv(loc, 1, specColor);
    loc = glGetUniformLocation(program, "shininess");
    glUniform1f(loc, shininess[0]);
}

void updateLightUniforms()
{
    // Light Position
    //GLfloat lightPos[] = { 2.0f, 3.0f, 5.0f, 1.0f };
    int loc = glGetUniformLocation(program, "lightPos");
    glUniform4fv(loc, 1, lightPos);

    // Light Color
    GLfloat lightDiff[] = { 120.0, 120.0, 120.0, 1.0 };
    loc = glGetUniformLocation(program, "lightDiff");
    glUniform4fv(loc, 1, lightDiff);
}

void loadInput()
{
    // load the OBJ file here
    const int MAX_BUFFER_SIZE = 4096;
    char buffer[MAX_BUFFER_SIZE];
    int linecount = 0;
    while(cin.getline(buffer, MAX_BUFFER_SIZE)){
        stringstream ss(buffer);
        Vector3f v;
        int id[3];
        string eleCode;
        ss>>eleCode;
        if(eleCode == "v"){
            ss>>v[0]>>v[1]>>v[2];
            vecv.push_back(v);
        } 
        else if(eleCode == "vn"){
            ss>>v[0]>>v[1]>>v[2];
            vecn.push_back(v);
        }
        else if(eleCode == "f"){
            vector<unsigned> fv;//[v1,n1,v2,n2,v3,n2]
            for(int i=0;i<3;i++){
                char c;
                ss>>id[0]>>c>>id[1]>>c>>id[2];    
                fv.push_back(id[0]); fv.push_back(id[2]);
            }
            vecf.push_back(fv);
        }
        linecount+=1;
    }
    cout<<"file loaded: "<<linecount<<endl;
    printf("nFaces:%d nV:%d nN:%d\n",vecf.size(),vecv.size(),vecn.size());
}

// Main routine.
// Set up OpenGL, define the callbacks and start the main loop
int main(int argc, char** argv)
{
    loadInput();

    GLFWwindow* window = createOpenGLWindow(640, 480, "a0");
    
    // setup the keyboard event handler
    glfwSetKeyCallback(window, keyCallback);

    glfwSetCursorPosCallback(window,mousePosCallback);
    glfwSetScrollCallback(window, scrollCallback);
    glfwSetMouseButtonCallback(window, mouseBtnCallback);
    // glEnable() and glDisable() control parts of OpenGL's
    // fixed-function pipeline, such as rasterization, or
    // depth-buffering. What happens if you remove the next line?
    glEnable(GL_DEPTH_TEST);

    // The program object controls the programmable parts
    // of OpenGL. All OpenGL programs define a vertex shader
    // and a fragment shader.
    program = compileProgram(c_vertexshader, c_fragmentshader);
    if (!program) {
        printf("Cannot compile program\n");
        return -1;
    }

    glUseProgram(program);

    // Main Loop
    while (!glfwWindowShouldClose(window)) {
        // Clear the rendering window
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        setViewport(window);

        updateCameraUniforms();
        updateLightUniforms();
        updateMaterialUniforms();

        // Draw to back buffer
        drawScene();

        // Make back buffer visible
        glfwSwapBuffers(window);

        // Check if any input happened during the last frame
        glfwPollEvents();
    }

    // All OpenGL resource that are created with
    // glGen* or glCreate* must be freed.
    glDeleteProgram(program);

    glfwTerminate(); // destroy the window
    return 0;
}
