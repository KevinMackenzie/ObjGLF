// TestProject.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#define USING_ASSIMP
#define GLUF_DEBUG
#define SUPPRESS_RADIAN_ERROR
#define SUPPRESS_UTF8_ERROR
#include "../ObjGLUF/GLUFGui.h"
#include <stdlib.h>
#include <stdio.h>
#include <fstream>
#include <iostream>
#include <vector>

using namespace GLUF;

GLFWwindow* window;

DialogResourceManagerPtr resMan;
DialogPtr dlg;

//extern FontPtr g_ArielDefault;

static void error_callback(int error, const char* description)
{
	fputs(description, stderr);
}
void MyErrorMethod(const std::string& message, const char* func, const char* file, unsigned int line)
{
    std::cout << "(" << func << " | " << line << "): " << message << std::endl;
	//pause
    //char c;
    //std::cin >> c;
}

bool MsgProc(_GUI_CALLBACK_PARAM)
{
	if (msg == KEY)
	{
		if (param1 == GLFW_KEY_ESCAPE && param3 == GLFW_RELEASE)
			//DialogOpened = !DialogOpened;
			dlg->SetMinimized(!dlg->GetMinimized());
		//return true;
	}

    /*if (msg != MessageType::CURSOR_ENTER || msg != MessageType::CURSOR_POS || msg != MessageType::FOCUS)
    {
        
    }*/

	resMan->MsgProc(_PASS_CALLBACK_PARAM);
	dlg->MsgProc(_PASS_CALLBACK_PARAM);

	return false;
}

void ControlEventCallback(Event evt, ControlPtr&, const EventCallbackReceivablePtr&) noexcept
{
	if (evt == EVENT_BUTTON_CLICKED)
	{
		printf("HORRAY\n");
	}
}

struct JustPositions : VertexStruct
{
    glm::vec3 pos;

    JustPositions(){};

    virtual void* operator&() const override
    {
		char* copy = new char[sizeof(glm::vec3)];
		memcpy(copy, &pos[0], sizeof(glm::vec3));
		return (void*)&copy[0];
    }

    virtual size_t size() const override
    {
        return 12;
    }

    virtual size_t n_elem_size(size_t element)
    {
        return 12;
    }

    virtual void buffer_element(void* data, size_t element) override
    {
        pos = *static_cast<glm::vec3*>(data);
    }

    static GLVector<JustPositions> MakeMany(size_t howMany)
    {
        GLVector<JustPositions> ret;
        ret.reserve(howMany);

        for (size_t i = 0; i < howMany; ++i)
        {
            ret.push_back(JustPositions());
        }

        return ret;
    }

    JustPositions& operator=(const glm::vec3& other)
    {
        pos = other;
        return *this;
    }

    JustPositions& operator=(const JustPositions& other)
    {
        pos = other.pos;
        return *this;
    }
    JustPositions& operator=(JustPositions&& other)
    {
        pos = other.pos;
        return *this;
    }
};

void myunexpected()
{
    int i = 0;
}

std::string RemoveChar(std::string str, char ch)
{
	std::stringstream ss;
	for (auto s : str)
	{
		if (s == ch)
			continue;
		ss << s;
	}

	return ss.str();
}

GLUF::GLVector<JustPositions> csvToArray(std::string text)
{
	auto lines = GLUF::SplitStr(text, '\n', false, true);
	auto ret = JustPositions::MakeMany(lines.size());
	int j = 0;
	for (auto it : lines)
	{
		auto columns = GLUF::SplitStr(it, ',');
		glm::vec3 row;
		for (int i = 0; i < columns.size(); ++i)
		{
			columns[i] = RemoveChar(columns[i], ' ');
			row[i] = (atof(columns[i].c_str()));
		}
		ret[j] = row;
		j++;
	}
	return ret;
}

//#define USE_SEPARATE

int main(void)
{

    std::set_unexpected(myunexpected);

    RegisterErrorMethod(MyErrorMethod);
	Init();


	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 1);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

	window = glfwCreateWindow(800, 800, "Simple example", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwMakeContextCurrent(window);

	InitOpenGLExtensions();
    
    //for testing purposes
    /*gGLVersionMajor = 2;
    gGLVersionMinor = 0;
    gGLVersion2Digit = 20;*/

	

#ifdef USE_SEPARATE
    ProgramPtrList Progs;

    //ShaderPathList paths;
    //paths.insert(std::pair<ShaderType, std::wstring>(SH_VERTEX_SHADER, L"Shaders/BasicLighting120.vert.glsl"));
    //paths.insert(std::pair<ShaderType, std::wstring>(SH_FRAGMENT_SHADER, L"Shaders/BasicLighting120.frag.glsl"));

    ShaderSourceList VertSources, FragSources;
    unsigned long len = 0;

    std::string text;
    std::vector<char> rawMem;
    LoadFileIntoMemory(L"Shaders/BasicLighting120.vert.glsl", rawMem);
    LoadBinaryArrayIntoString(rawMem, text);

    text += '\n';
    VertSources.insert(std::pair<ShaderType, const char*>(SH_VERTEX_SHADER, text.c_str()));

    ProgramPtr Vert;
    SHADERMANAGER.CreateProgram(Vert, VertSources, true);

    std::string text1;
    LoadFileIntoMemory(L"Shaders/BasicLighting120.frag.glsl", rawMem);
    LoadBinaryArrayIntoString(rawMem, text1);
    text1 += '\n';
    FragSources.insert(std::pair<ShaderType, const char*>(SH_FRAGMENT_SHADER, text1.c_str()));

    ProgramPtr Frag;
    SHADERMANAGER.CreateProgram(Frag, FragSources, true);

    ProgramPtrList programs;
    programs.push_back(Vert);
    programs.push_back(Frag);

    SepProgramPtr Prog;

    SHADERMANAGER.CreateSeparateProgram(Prog, programs);

#else
    //ShaderPathList paths;
    //paths.insert(std::pair<ShaderType, std::wstring>(SH_VERTEX_SHADER, L"Shaders/BasicLighting120.vert.glsl"));
    //paths.insert(std::pair<ShaderType, std::wstring>(SH_FRAGMENT_SHADER, L"Shaders/BasicLighting120.frag.glsl"));

    ProgramPtr Prog;

    ShaderSourceList Sources;
    unsigned long len = 0;

    std::string text;
    std::vector<char> rawMem;
    LoadFileIntoMemory(L"Shaders/Lines.vert.glsl", rawMem);
    LoadBinaryArrayIntoString(rawMem, text);

    text += '\n';
    Sources.insert(std::pair<ShaderType, const char*>(SH_VERTEX_SHADER, text.c_str()));

    std::string text1;
    LoadFileIntoMemory(L"Shaders/Lines.frag.glsl", rawMem);
    LoadBinaryArrayIntoString(rawMem, text1);
    text1 += '\n';
    Sources.insert(std::pair<ShaderType, const char*>(SH_FRAGMENT_SHADER, text1.c_str()));

    SHADERMANAGER.CreateProgram(Prog, Sources);
#endif


	VariableLocMap attribs, uniforms;
	attribs = SHADERMANAGER.GetShaderAttribLocations(Prog);
	uniforms = SHADERMANAGER.GetShaderUniformLocations(Prog);

	// Get a handle for our "MVP" uniform
	GLuint MatrixID = uniforms["MVP"];
	GLuint TimeID = uniforms["Time"];
	GLuint ColorID = uniforms["Color"];
	GLuint TimeRangeID = uniforms["TimeRange"];

	// Get a handle for our buffers
	GLuint vertexPosition_modelspaceID = attribs["vertexPosition_modelspace"];

	GLUF::VertexArrayAoS lines(GL_LINES, GL_DYNAMIC_DRAW);
	lines.AddVertexAttrib(VertAttrib(vertexPosition_modelspaceID, 4, 3, GL_FLOAT));
	
	std::string csvText;
	try
	{
		GLUF::LoadFileIntoMemory("lines.csv", csvText);
	}
	catch (...) {}
	auto data = csvToArray(csvText);

	std::vector<GLuint> indices;
	for (int i = 1; i < data.size(); ++i)
	{
		indices.push_back(i - 1);
		indices.push_back(i);
	}

	lines.BufferData(data);
	lines.BufferIndices(indices);

	//load up the locations

	/*std::shared_ptr<VertexArray> vertexData = LoadVertexArrayFromScene(scene, attributes);
	if (!vertexData)
		EXIT_FAILURE;*/


	float ellapsedTime = 0.0f;
	float prevTime = 0.0f;
	float currTime = 0.0f;


	printf("%i.%i", GetGLVersionMajor(), GetGLVersionMinor);

	// Cull triangles which normal is not towards the camera
	//glEnable(GL_CULL_FACE);

	//::TextHelper *textHelper = new ::TextHelper(resMan);
	//textHelper->Init(20);


	/*GLuint skycubemap = ::LoadTextureFromFile(L"afternoon_sky.cubemap.dds", TTF_DDS_CUBEMAP);

    sources.clear();
    LoadFileIntoMemory(L"Shaders/BasicLighting120.vert.glsl", rawMem);
    LoadBinaryArrayIntoString(rawMem, text);
	text += '\n';
	sources.insert(std::pair<ShaderType, const char*>(SH_VERTEX_SHADER, text.c_str()));

    LoadFileIntoMemory(L"Shaders/BasicLighting120.frag.glsl", rawMem);
    LoadBinaryArrayIntoString(rawMem, text1);
	text1 += '\n';
	sources.insert(std::pair<ShaderType, const char*>(SH_FRAGMENT_SHADER, text1.c_str()));

    ProgramPtr sky;
    SHADERMANAGER.CreateProgram(sky, sources);

	VariableLocMap attribLocations = SHADERMANAGER.GetShaderAttribLocations(sky);
	VariableLocMap unifLocations = SHADERMANAGER.GetShaderUniformLocations(sky);

	GLuint sampLoc = unifLocations["m_tex0"];
	GLuint mvpLoc = unifLocations["_mvp"];

	VariableLocMap::iterator it;

	VertexArray m_pVertexArray;
	GLuint mPositionLoc = 0;

	it = attribLocations.find("_position");
	if (it != attribLocations.end())
	{
		m_pVertexArray.AddVertexAttrib(VertAttrib(it->second, 4, 3, GL_FLOAT));
		mPositionLoc = it->second;
	}

    GLVector<JustPositions> verts = JustPositions::MakeMany(8);
	float depth;
	float val = depth = 10.0f;
    verts[0] = glm::vec3(val, -val, -depth);
    verts[1] = glm::vec3(-val, -val, -depth);
    verts[2] = glm::vec3(-val, val, -depth);
    verts[3] = glm::vec3(val, val, -depth);

	//front
    verts[4] = glm::vec3(-val, -val, depth);
    verts[5] = glm::vec3(val, -val, depth);
    verts[6] = glm::vec3(val, val, depth);
    verts[7] = glm::vec3(-val, val, depth);

	std::vector<glm::u32vec3> mTriangles;

	//north
	mTriangles.push_back(glm::u32vec3(4, 5, 6));
	mTriangles.push_back(glm::u32vec3(6, 7, 4));

	//south
	mTriangles.push_back(glm::u32vec3(0, 1, 2));
	mTriangles.push_back(glm::u32vec3(2, 3, 0));

	//east
	mTriangles.push_back(glm::u32vec3(1, 4, 7));
	mTriangles.push_back(glm::u32vec3(7, 2, 1));

	//west
	mTriangles.push_back(glm::u32vec3(5, 0, 3));
	mTriangles.push_back(glm::u32vec3(3, 6, 5));

	//top
	mTriangles.push_back(glm::u32vec3(7, 6, 3));
	mTriangles.push_back(glm::u32vec3(3, 2, 7));

	//bottom
	mTriangles.push_back(glm::u32vec3(1, 0, 5));
	mTriangles.push_back(glm::u32vec3(5, 4, 1));

	m_pVertexArray.BufferData(verts);
	m_pVertexArray.BufferIndices(mTriangles);*/

	float timeStart = -1;
	do{
		if (timeStart == -1)
			timeStart = (float)glfwGetTime();

		currTime = (float)glfwGetTime();
		ellapsedTime = currTime - prevTime;

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		float ratio;
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		ratio = width / (float)height;
		glViewport(0, 0, width, height);
		static const GLfloat black[] = { 0.0f, 0.0f, 0.0f, 1.0f };
		static const GLfloat one = 1.0f;

		glClearBufferfv(GL_COLOR, 0, black);
		
		
		// Enable depth test
		//glEnable(GL_DEPTH_TEST);
		// Accept fragment if it closer to the camera than the former one
		//glDepthFunc(GL_LESS);

		// Cull triangles which normal is not towards the camera
		//glEnable(GL_CULL_FACE);

		//SHADERMANAGER.UseProgram(Prog); 
		float dataSpaceHeight = height / 5;
		glm::vec3 pos(0, 0, 80);
		glm::mat4 ProjectionMatrix = glm::ortho<float>(0, dataSpaceHeight *ratio, 0, dataSpaceHeight);
		glm::mat4 MVP = ProjectionMatrix;

		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		SHADERMANAGER.UseProgram(Prog);

#ifndef USE_SEPARATE
		
		// Send our transformation to the currently bound shader, 
		// in the "MVP" uniform
		SHADERMANAGER.GLUniformMatrix4f(MatrixID, MVP);
		SHADERMANAGER.GLUniform1f(TimeRangeID, 10);
		SHADERMANAGER.GLUniform3f(ColorID, glm::vec3(1,1,0));
		SHADERMANAGER.GLUniform1f(TimeID, currTime - timeStart);
		
		lines.Draw();

		//vertexData2->Draw();

#else

        SHADERMANAGER.GLActiveShaderProgram(Prog, SH_FRAGMENT_SHADER);

        // Bind our texture in Texture Unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        // Set our "myTextureSampler" sampler to user Texture Unit 0
        SHADERMANAGER.GLProgramUniform1i(Prog, TextureID, 0);

        ModelMatrix = glm::translate(glm::mat4(), glm::vec3(1.5f, 0.0f, -5.0f)) * glm::toMat4(glm::quat(glm::vec3(0.0f, 2.0f * currTime, 0.0f)));
        MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

        SHADERMANAGER.GLActiveShaderProgram(Prog, SH_VERTEX_SHADER);

        // Send our transformation to the currently bound shader, 
        // in the "MVP" uniform
        SHADERMANAGER.GLProgramUniformMatrix4f(Prog, MatrixID, MVP);
        SHADERMANAGER.GLProgramUniformMatrix4f(Prog, ModelMatrixID, ModelMatrix);
        SHADERMANAGER.GLProgramUniformMatrix4f(Prog, ViewMatrixID, ViewMatrix);

        glm::vec3 lightPos = glm::vec3(4, 4, 4);
        SHADERMANAGER.GLProgramUniform3f(Prog, LightID, lightPos);

        vertexData2->Draw();

#endif

		//render dialog last(overlay)
		//if ((int)currTime % 2)
			//dlg->OnRender(ellapsedTime);
			//dlg->DrawRect(rc, ::Color(255, 0, 0, 255));

            /*textHelper->Begin(0, 20, 15);
            textHelper->DrawFormattedTextLineBase({ { 500 }, 300, 700, { 280 } }, GT_TOP | GT_LEFT, L"Val1 = %; Val2 = %; Val3 = %", 36, 29.334f, "String");
            textHelper->End();*/

        //sky box rendering stuff
		/*SHADERMANAGER.UseProgram(sky);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, skycubemap);
		glUniform1i(sampLoc, 0);
		glUniformMatrix4fv(mvpLoc, 1, GL_FALSE, &MVP[0][0]);
		glDisable(GL_DEPTH_CLAMP);
		glDepthMask(GL_FALSE);
		glDisable(GL_CULL_FACE);
		m_pVertexArray.Draw();
		glEnable(GL_CULL_FACE);
		glDepthMask(GL_TRUE);
		glEnable(GL_DEPTH_CLAMP);*/

		//glEnable(GL_BLEND);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		/*textHelper->Begin(0);
		textHelper->SetInsertionPos(Point(100, 100));
		textHelper->DrawTextLine(L"TESTING");

		SHADERMANAGER.UseProgram(linesprog);
		glUniformMatrix4fv(0, 1, GL_FALSE, &MVP[0][0]);
		m_Squares.Draw();*/

		// Swap buffers
        glfwSwapBuffers(window);
		glfwPollEvents();

		//prevTime = currTime;

	} // Check if the ESC key was pressed or the window was closed
	while (glfwWindowShouldClose(window) == 0);


	glfwDestroyWindow(window);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	Terminate();

	return 0;
}