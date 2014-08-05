#include "stdafx.h"
#include "GLUFGui.h"
#include "CBFG/BitmapFontClass.h"

#include <algorithm>
#include <stdio.h>

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//IMPORTANT///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//																						//////////////////////////////////////
//																						//////////////////////////////////////
//	I WILL NOT be using the api declared in ObjGLUF.h in this file UNTIL i get it		//////////////////////////////////////
//		to a more reliable state														//////////////////////////////////////
//																						//////////////////////////////////////
//																						//////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// Minimum scroll bar thumb size
#define SCROLLBAR_MINTHUMBSIZE 8

// Delay and repeat period when clicking on the scroll bar arrows
#define SCROLLBAR_ARROWCLICK_DELAY  0.33f
#define SCROLLBAR_ARROWCLICK_REPEAT 0.05f

#define GLUF_NEAR_BUTTON_DEPTH -0.6f
#define GLUF_FAR_BUTTON_DEPTH -0.8f

#define GLUF_MAX_GUI_SPRITES 500
#define WHEEL_DELTA 120//TODO:

// GLUF_MAX_EDITBOXLENGTH is the maximum string length allowed in edit boxes,
// including the nul terminator.
// 
// Uniscribe does not support strings having bigger-than-16-bits length.
// This means that the string must be less than 65536 characters long,
// including the nul terminator.
#define GLUF_MAX_EDITBOXLENGTH 0xFFFF

//this is just a constant to be a little bit less windows api dependent
unsigned int GetCaretBlinkTime()
{
	return 1000;
}

double                 GLUFDialog::s_fTimeRefresh = 0.0f;
GLUFControl*           GLUFDialog::s_pControlFocus = nullptr;        // The control which has focus
GLUFControl*           GLUFDialog::s_pControlPressed = nullptr;      // The control currently pressed



//======================================================================================
// GLFWCallback methods that ALL redirect to a universal callback
//======================================================================================

void MessageProcedure(GLUF_MESSAGE_TYPE, int, int, int, int);

//=====
//	GLFW Window Callback
//=====

void GLFWWindowPosCallback(GLFWwindow*, int x, int y)
{
	MessageProcedure(GM_POS, x, y, 0, 0);
}

void GLFWWindowSizeCallback(GLFWwindow*, int width, int height)
{
	MessageProcedure(GM_RESIZE, width, height, 0, 0);
}

void GLFWWindowCloseCallback(GLFWwindow*)
{
	MessageProcedure(GM_CLOSE, 0, 0, 0, 0);
}
/*
void GLFWWindowRefreshCallback(GLFWwindow*)
{
	MessageProcedure(GM_REFRESH, 0, 0, 0, 0);
}*/

void GLFWWindowFocusCallback(GLFWwindow*, int focused)
{
	MessageProcedure(GM_FOCUS, focused, 0, 0, 0);
}

void GLFWWindowIconifyCallback(GLFWwindow*, int iconified)
{
	MessageProcedure(GM_ICONIFY, iconified, 0, 0, 0);
}

void GLFWFrameBufferSizeCallback(GLFWwindow*, int width, int height)
{
	MessageProcedure(GM_FRAMEBUFFER_SIZE, width, height, 0, 0);
}

//=====
//	GLFW Input Callback
//=====

void GLFWMouseButtonCallback(GLFWwindow*, int button, int action, int mods)
{
	MessageProcedure(GM_MB, button, action, mods, 0);
}

void GLFWCursorPosCallback(GLFWwindow*, double xPos, double yPos)
{
	MessageProcedure(GM_CURSOR_POS, (int)xPos, (int)yPos, 0, 0);
}

void GLFWCursorEnterCallback(GLFWwindow*, int entered)
{
	MessageProcedure(GM_CURSOR_ENTER, entered, 0, 0, 0);
}

void GLFWScrollCallback(GLFWwindow*, double xoffset, double yoffset)
{
	MessageProcedure(GM_SCROLL, (int)(xoffset * 1000.0), (int)(yoffset * 1000.0), 0, 0);
}

void GLFWKeyCallback(GLFWwindow*, int key, int scancode, int action, int mods)
{
	MessageProcedure(GM_KEY, key, scancode, action, mods);
}

void GLFWCharCallback(GLFWwindow*, unsigned int codepoint)
{
	MessageProcedure(GM_UNICODE_CHAR, (int)codepoint, 0, 0, 0);
}


PGLUFCALLBACK g_pCallback;

void MessageProcedure(GLUF_MESSAGE_TYPE msg, int param1, int param2, int param3, int param4)
{
	if (g_pCallback(msg, param1, param2, param3, param4))
	{

	}

	//todo: anything else to do?
}



//======================================================================================
// Various Structs that are used exclusively for UI
//======================================================================================

struct GLUFScreenVertex
{
	glm::vec3 pos;
	Color     color;
	glm::vec2 uv;
};

struct GLUFScreenVertexUntex
{
	glm::vec3 pos;
	Color     color;
};

struct GLUFTextVertexArray
{
private:
	std::vector<glm::vec3> vPos;
	Color4f                vColor;
	std::vector<glm::vec2> vTex;
public:

	glm::vec3* data_pos()  { if (size() > 0) return &vPos[0];	}
	glm::vec2* data_tex()  { if (size() > 0) return &vTex[0];	}
	Color4f    get_color() { return vColor;						}

	void push_back(glm::vec3 pos, glm::vec2 tex)
	{
		vPos.push_back(/*GLUFScreenToClipspace(pos)*/pos);	vTex.push_back(tex);
	}

	void set_color(Color col)
	{
		vColor = GLUFColorToFloat(col);
	}

	void clear(){ vPos.clear(); vColor = Color4f(); vTex.clear(); }
	unsigned long size(){ return vPos.size(); }
};

//======================================================================================
// Initialization and globals
//======================================================================================


GLUFProgramPtr g_UIProgram;
GLUFProgramPtr g_UIProgramUntex;
GLUFProgramPtr g_TextProgram;
GLUFTextVertexArray g_TextVerticies;
GLuint g_TextVAO;
GLuint g_TextPos;
GLuint g_TextTexCoords;
GLuint g_TextIndices;

GLFWwindow* g_pGLFWWindow;
GLUFTexturePtr g_pControlTexturePtr;
int g_ControlTextureResourceManLocation = -1;

const char* g_UIShaderVert =
"#version 430 core \n"\
"layout(location = 0) in vec3 _Position; \n"\
"layout(location = 1) in vec4 _Color; \n"\
"layout(location = 2) in vec2 _UV; \n"\
"layout(location = 0) uniform mat4 _Ortho; \n"\
"out VS_OUT \n"\
"{ \n"\
"	vec4 Color; \n"\
"	vec2 uvCoord; \n" \
"} vs_out; \n"\
"void main(void) \n"\
"{ \n"\
"	gl_Position = vec4(_Position, 1.0f) * _Ortho; \n"\
"	vs_out.Color = _Color; \n"\
"   vs_out.uvCoord = abs(vec2(0.0f, 1.0f) - _UV); \n"  /*the V's are inverted because the texture is loaded bottom to top*/ \
"} \n";

const char* g_UIShaderFrag =
"#version 430 core \n"\
"in VS_OUT \n"\
"{"\
"	vec4 Color; \n"\
"	vec2 uvCoord; \n"\
"} fs_in; \n"\
"layout(location = 0) out vec4 Color; \n"\
"layout(location = 1) uniform sampler2D TextureSampler; \n"\
"void main(void) \n"\
"{ \n"\
"	//Color = vec4(1.0f, 0.0, 0.0f, 1.0f); \n"\
"	//Color = fs_in.Color; \n"\
"   Color = texture(TextureSampler, fs_in.uvCoord); \n" \
"	Color = vec4(mix(Color.rgb, fs_in.Color.rgb, fs_in.Color.a), Color.a); \n"\
"} \n";

const char* g_UIShaderFragUntex =
"#version 430 core \n"\
"in VS_OUT \n"\
"{ \n"\
"	vec4 Color; \n"\
"	vec2 uvCoord; \n"\
"} fs_in; \n"\
"layout(location = 0) out vec4 Color; \n"\
"void main(void) \n"\
"{ \n"\
"	Color = fs_in.Color; \n"\
"} \n";

const char* g_TextShaderVert =
"#version 430 core \n" \
"layout(location = 0) in vec3 _Position; \n"\
"layout(location = 1) in vec2 _UV; \n" \
"layout(location = 0) uniform mat4 _MV;\n"\
"out VS_OUT \n"\
"{\n"\
"	vec2 uvCoord;\n"\
"} vs_out;\n"\
"void main(void)\n"\
"{\n"\
"   vs_out.uvCoord = /*abs(vec2(0.0f, 1.0f) - */_UV/*)*/; \n"  /*the V's are inverted because the texture is loaded bottom to top*/ \
"	gl_Position = vec4(_Position, 1.0f) * _MV; \n"\
"} \n";

const char* g_TextShaderFrag =
"#version 430 core \n" \
"layout(location = 2) uniform vec4 _Color; \n"\
"layout(location = 1) uniform sampler2D TextureSampler; \n"\
"layout(location = 0) out vec4 Color; \n"\
"in VS_OUT \n"\
"{\n"\
"	vec2 uvCoord;\n"\
"} fs_in;\n"\
"void main(void)\n"\
"{\n"\
"	Color.a = texture(TextureSampler, fs_in.uvCoord).a;\n"\
"	Color.rgb = _Color.rgb; \n"\
"	//Color.a = 1.0f;\n"\
"	Color.a *= _Color.a;\n"\
"	//Color = vec4(1.0f, 0.0f, 0.0f, 1.0f); \n"\
"} \n";



void GLUFInitGui(GLFWwindow* pInitializedGLFWWindow, PGLUFCALLBACK callback, GLUFTexturePtr controltex)
{
	g_pGLFWWindow = pInitializedGLFWWindow;
	g_pCallback = callback;
	
	//register glfw event handlers
	glfwSetMouseButtonCallback(g_pGLFWWindow, GLFWMouseButtonCallback);
	glfwSetCursorPosCallback(g_pGLFWWindow, GLFWCursorPosCallback);
	glfwSetCursorEnterCallback(g_pGLFWWindow, GLFWCursorEnterCallback);
	glfwSetScrollCallback(g_pGLFWWindow, GLFWScrollCallback);
	glfwSetKeyCallback(g_pGLFWWindow, GLFWKeyCallback);
	glfwSetCharCallback(g_pGLFWWindow, GLFWCharCallback);

	glfwSetWindowPosCallback(g_pGLFWWindow, GLFWWindowPosCallback);
	glfwSetWindowSizeCallback(g_pGLFWWindow, GLFWWindowSizeCallback);
	glfwSetWindowCloseCallback(g_pGLFWWindow, GLFWWindowCloseCallback);
	//glfwSetWindowRefreshCallback(g_pGLFWWindow, GLFWWindowRefreshCallback); huge performace overhead
	glfwSetWindowFocusCallback(g_pGLFWWindow, GLFWWindowFocusCallback);
	glfwSetWindowIconifyCallback(g_pGLFWWindow, GLFWWindowIconifyCallback);
	glfwSetFramebufferSizeCallback(g_pGLFWWindow, GLFWFrameBufferSizeCallback);

	//load the ui shader
	GLUFProgramPtr programUIVert;
	GLUFProgramPtr programUIFrag;
	GLUFProgramPtr programUIFragUntex;

	GLUFShaderSourceList sources;
	sources.insert(std::pair<GLUFShaderType, const char*>(SH_VERTEX_SHADER, g_UIShaderVert));
	sources.insert(std::pair<GLUFShaderType, const char*>(SH_FRAGMENT_SHADER, g_UIShaderFrag));
	g_UIProgram = GLUFSHADERMANAGER.CreateProgram(sources);
	sources.clear();

	sources.insert(std::pair<GLUFShaderType, const char*>(SH_VERTEX_SHADER, g_UIShaderVert));
	sources.insert(std::pair<GLUFShaderType, const char*>(SH_FRAGMENT_SHADER, g_UIShaderFragUntex));
	g_UIProgramUntex = GLUFSHADERMANAGER.CreateProgram(sources);
	sources.clear();


	sources.insert(std::pair<GLUFShaderType, const char*>(SH_VERTEX_SHADER, g_TextShaderVert));
	sources.insert(std::pair<GLUFShaderType, const char*>(SH_FRAGMENT_SHADER, g_TextShaderFrag));
	g_TextProgram = GLUFSHADERMANAGER.CreateProgram(sources);


	//create the text arrrays
	glGenVertexArrays(1, &g_TextVAO);
	glBindVertexArray(g_TextVAO);
	glGenBuffers(1, &g_TextPos);
	glGenBuffers(1, &g_TextTexCoords);

	glBindBuffer(GL_ARRAY_BUFFER, g_TextPos);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, g_TextTexCoords);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindVertexArray(0);


	//load the texture for the controls
	g_pControlTexturePtr = controltex;
}


GLUFResult GLUFTrace(const char* file, const char* function, unsigned long lineNum, GLUFResult value, const char* message)
{
	//this might be more elaborate in the future
	GLUFGetErrorMethod()(message, function, file, lineNum);
	return value;
}


//======================================================================================
// GLUFFont
//======================================================================================

typedef struct
{
	unsigned char ID1, ID2;
	unsigned char BPP;
	int ImageWidth, ImageHeight, CellWidth, CellHeight;
	unsigned char StartPoint;
}GLUFFontFileHeader;

class GLUFFont
{	
public:
	int CellX, CellY, YOffset, RowPitch;
	char Base;
	char Width[256];
	GLuint TexID;
	float RowFactor, ColFactor;
	int RenderStyle;


	bool Init(char* data, uint64_t rawSize);

	//font properties

};

bool GLUFFont::Init(char* data, uint64_t rawSize)
{
	MemStreamBuf buf(data, rawSize);

	char *dat, *img;
	std::istream in(&buf);
	char bpp;
	int ImgX, ImgY;

	//in.open(fname, ios_base::binary | ios_base::in);

	if (in.fail())
		return false;


	// allocate space for file data
	dat = new char[rawSize];

	// Read filedata
	if (!dat)
		return false;

	in.read(dat, rawSize);

	if (in.fail())
	{
		delete[] dat;
		return false;
	}


	// Check ID is 'BFF2'
	if ((unsigned char)dat[0] != 0xBF || (unsigned char)dat[1] != 0xF2)
	{
		delete[] dat;
		return false;
	}

	// Grab the rest of the header
	memcpy(&ImgX, &dat[2], sizeof(int));
	memcpy(&ImgY, &dat[6], sizeof(int));
	memcpy(&CellX, &dat[10], sizeof(int));
	memcpy(&CellY, &dat[14], sizeof(int));
	bpp = dat[18];
	Base = dat[19];

	// Check filesize
	if (rawSize != ((MAP_DATA_OFFSET)+((ImgX*ImgY)*(bpp / 8))))
		return false;

	// Calculate font params
	RowPitch = ImgX / CellX;
	ColFactor = (float)CellX / (float)ImgX;
	RowFactor = (float)CellY / (float)ImgY;
	YOffset = CellY;

	// Determine blending options based on BPP
	switch (bpp)
	{
	case 8: // Greyscale
		RenderStyle = BFG_RS_ALPHA;
		break;

	case 24: // RGB
		RenderStyle = BFG_RS_RGB;
		break;

	case 32: // RGBA
		RenderStyle = BFG_RS_RGBA;
		break;

	default: // Unsupported BPP
		delete[] dat;
		return false;
		break;
	}

	// Allocate space for image
	img = new char[(ImgX*ImgY)*(bpp / 8)];

	if (!img)
	{
		delete[] dat;
		return false;
	}

	// Grab char widths
	memcpy(Width, &dat[WIDTH_DATA_OFFSET], 256);

	// Grab image data
	memcpy(img, &dat[MAP_DATA_OFFSET], (ImgX*ImgY)*(bpp / 8));

	// Create Texture
	glGenTextures(1, &TexID);
	glBindTexture(GL_TEXTURE_2D, TexID);
	// Fonts should be rendered at native resolution so no need for texture filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	// Stop chararcters from bleeding over edges
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	// Tex creation params are dependent on BPP
	switch (RenderStyle)
	{
	case BFG_RS_ALPHA:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, ImgX, ImgY, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, img);
		break;

	case BFG_RS_RGB:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, ImgX, ImgY, 0, GL_RGB, GL_UNSIGNED_BYTE, img);
		break;

	case BFG_RS_RGBA:
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ImgX, ImgY, 0, GL_RGBA, GL_UNSIGNED_BYTE, img);
		break;
	}

	// Clean up
	delete[] img;
	delete[] dat;

	return true;
}


GLUFFontPtr GLUFLoadFont(char* rawData, uint64_t rawSize)
{
	GLUFFontPtr ret(new GLUFFont());
	ret->Init(rawData, rawSize);
	return ret;
}
/*
GLUFFontPtr GLUFLoadFont(std::string fontFaceName)
{
	std::string path = "C:/Windows/Fonts/";
	path += fontFaceName.c_str();
	path += ".ttf";


	GLUFFontPtr ret(new GLUFFont());

	char* rawData = nullptr;
	unsigned long rawSize;
	if (GLUFLoadFileIntoMemory(path.c_str(), &rawSize, rawData))
	{
		if (!ret->Init(rawData, rawSize))
		{
			return nullptr;
		}
	}
	else
	{
		return nullptr;
	}
	
	return ret;
}*/

//======================================================================================
// GLUFBlendColor
//======================================================================================

//--------------------------------------------------------------------------------------
void GLUFBlendColor::Init(Color defaultColor, Color disabledColor, Color hiddenColor)
{
	for (int i = 0; i < MAX_CONTROL_STATES; i++)
	{
		States[i] = defaultColor;
	}

	States[GLUF_STATE_DISABLED] = disabledColor;
	States[GLUF_STATE_HIDDEN] = hiddenColor;
	Current = hiddenColor;//start hidden
}


//--------------------------------------------------------------------------------------
void GLUFBlendColor::Blend(GLUF_CONTROL_STATE iState, float fElapsedTime, float fRate)
{
	//this is quite condensed, this basically interpolates from the current state to the destination state based on the time
	Current = glm::mix(Current, States[iState], 1.0f - powf(fRate, 30 * fElapsedTime));
}

void GLUFBlendColor::SetCurrent(Color current)
{
	Current = current;
}

void GLUFBlendColor::SetCurrent(GLUF_CONTROL_STATE state)
{
	Current = States[state];
}

//======================================================================================
// GLUFElement
//======================================================================================

//--------------------------------------------------------------------------------------

void GLUFElement::SetTexture(unsigned int iTexture, GLUFRect* prcTexture, Color defaultTextureColor)
{
	this->iTexture = iTexture;

	if (prcTexture)
		rcTexture = *prcTexture;
	else
		GLUFSetRectEmpty(rcTexture);

	TextureColor.Init(defaultTextureColor);
}


//--------------------------------------------------------------------------------------

void GLUFElement::SetFont(unsigned int iFont, Color defaultFontColor, unsigned int dwTextFormat)
{
	this->iFont = iFont;
	this->dwTextFormat = dwTextFormat;

	FontColor.Init(defaultFontColor);
}


//--------------------------------------------------------------------------------------
void GLUFElement::Refresh()
{
	TextureColor.SetCurrent(GLUF_STATE_HIDDEN);
	FontColor.SetCurrent(GLUF_STATE_HIDDEN);
}



//======================================================================================
// GLUFDialog class
//======================================================================================

GLUFDialog::GLUFDialog() :
m_x(0.0f),
m_y(0.0f),
m_width(0.0f),
m_height(0.0f),
m_pManager(nullptr),
m_bVisible(true),
m_bCaption(false),
m_bMinimized(false),
m_bDrag(false),
m_nCaptionHeight(18),
m_colorTopLeft(0),
m_colorTopRight(0),
m_colorBottomLeft(0),
m_colorBottomRight(0),
m_pCallbackEvent(nullptr),
m_pCallbackEventUserContext(nullptr),
m_fTimeLastRefresh(0),
m_pControlMouseOver(nullptr),
m_nDefaultControlID(0xffff),
m_bNonUserEvents(false),
m_bKeyboardInput(false),
m_bMouseInput(true)
{
	m_wszCaption[0] = L'\0';

	m_pNextDialog = this;
	m_pPrevDialog = this;
}


//--------------------------------------------------------------------------------------
GLUFDialog::~GLUFDialog()
{
	RemoveAllControls();

	m_Fonts.clear();
	m_Textures.clear();

	for (auto it = m_DefaultElements.begin(); it != m_DefaultElements.end(); ++it)
	{
		GLUF_SAFE_DELETE(*it);
	}

	m_DefaultElements.clear();
}


//--------------------------------------------------------------------------------------

void GLUFDialog::Init(GLUFDialogResourceManager* pManager, bool bRegisterDialog)
{
	m_pManager = pManager;
	if (bRegisterDialog)
		pManager->RegisterDialog(this);

	if (g_ControlTextureResourceManLocation == -1)
	{
		g_ControlTextureResourceManLocation = m_pManager->AddTexture(g_pControlTexturePtr);
	}

	SetTexture(0, g_ControlTextureResourceManLocation);
	InitDefaultElements();
}


//--------------------------------------------------------------------------------------

void GLUFDialog::Init(GLUFDialogResourceManager* pManager, bool bRegisterDialog, unsigned int iTexture)
{
	m_pManager = pManager;
	if (bRegisterDialog)
		pManager->RegisterDialog(this);

	SetTexture(0, iTexture);//this will always be the first one in our buffer of indices
	InitDefaultElements();
}


//--------------------------------------------------------------------------------------

/*void GLUFDialog::Init(GLUFDialogResourceManager* pManager, bool bRegisterDialog, LPCWSTR szControlTextureResourceName, HMODULE hControlTextureResourceModule)
{
	m_pManager = pManager;
	if (bRegisterDialog)
		pManager->RegisterDialog(this);

	SetTexture(0, szControlTextureResourceName, hControlTextureResourceModule);
	InitDefaultElements();
}*/


//--------------------------------------------------------------------------------------

void GLUFDialog::SetCallback(PCALLBACKGLUFGUIEVENT pCallback, void* pUserContext)
{
	// If this assert triggers, you need to call GLUFDialog::Init() first.  This change
	// was made so that the GLUF's GUI could become seperate and optional from GLUF's core.  The 
	// creation and interfacing with GLUFDialogResourceManager is now the responsibility 
	// of the application if it wishes to use GLUF's GUI.
	GLUF_ASSERT(m_pManager && L"To fix call GLUFDialog::Init() first.  See comments for details.");

	m_pCallbackEvent = pCallback;
	m_pCallbackEventUserContext = pUserContext;
}


//--------------------------------------------------------------------------------------
void GLUFDialog::RemoveControl(int ID)
{
	for (auto it = m_Controls.begin(); it != m_Controls.end(); ++it)
	{
		if ((*it)->GetID() == ID)
		{
			// Clean focus first
			ClearFocus();

			// Clear references to this control
			if (s_pControlFocus == (*it))
				s_pControlFocus = nullptr;
			if (s_pControlPressed == (*it))
				s_pControlPressed = nullptr;
			if (m_pControlMouseOver == (*it))
				m_pControlMouseOver = nullptr;

			GLUF_SAFE_DELETE((*it));
			m_Controls.erase(it);

			return;
		}
	}
}


//--------------------------------------------------------------------------------------
void GLUFDialog::RemoveAllControls()
{
	if (s_pControlFocus && s_pControlFocus->m_pDialog == this)
		s_pControlFocus = nullptr;
	if (s_pControlPressed && s_pControlPressed->m_pDialog == this)
		s_pControlPressed = nullptr;
	m_pControlMouseOver = nullptr;

	for (auto it = m_Controls.begin(); it != m_Controls.end(); ++it)
	{
		GLUF_SAFE_DELETE(*it);
	}

	m_Controls.clear();
}


//--------------------------------------------------------------------------------------
void GLUFDialog::Refresh()
{
	if (s_pControlFocus)
		s_pControlFocus->OnFocusOut();

	if (m_pControlMouseOver)
		m_pControlMouseOver->OnMouseLeave();

	s_pControlFocus = nullptr;
	s_pControlPressed = nullptr;
	m_pControlMouseOver = nullptr;

	for (auto it = m_Controls.begin(); it != m_Controls.end(); ++it)
	{
		(*it)->Refresh();
	}

	if (m_bKeyboardInput)
		FocusDefaultControl();
}


//--------------------------------------------------------------------------------------
GLUFResult GLUFDialog::OnRender(float fElapsedTime)
{
	// If this assert triggers, you need to call GLUFDialogResourceManager::On*Device() from inside
	// the application's device callbacks.  See the SDK samples for an example of how to do this.
	//GLUF_ASSERT(m_pManager->GetD3D11Device() &&
	//	L"To fix hook up GLUFDialogResourceManager to device callbacks.  See comments for details");
	//no need for "devices", this is all handled by GLFW

	// See if the dialog needs to be refreshed
	if (m_fTimeLastRefresh < s_fTimeRefresh)
	{
		m_fTimeLastRefresh = GLUFGetTime();
		Refresh();
	}

	// For invisible dialog, out now.
	if (!m_bVisible ||
		(m_bMinimized && !m_bCaption))
		return GR_SUCCESS;

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//ID3D11Device* pd3dDevice = m_pManager->GetD3D11Device();
	//ID3D11DeviceContext* pd3dDeviceContext = m_pManager->GetD3D11DeviceContext();

	// Set up a state block here and restore it when finished drawing all the controls
	//m_pManager->StoreD3D11State(pd3dDeviceContext);

	//if any of them are visible, then draw
	bool bBackgroundIsVisible = (m_colorTopLeft.a > 0 || m_colorTopRight.a > 0 || m_colorBottomRight.a > 0 || m_colorBottomLeft.a > 0);
	if (!m_bMinimized && bBackgroundIsVisible)
	{
		// Convert the draw rectangle from screen coordinates to clip space coordinates.(where the origin is in the middle of the screen, and the edges are 1, or negative 1
		GLUFRect windowCoords = { m_x, m_y + m_height, m_x + m_width, m_y };
		windowCoords = GLUFScreenToClipspace(windowCoords);

		/*Left = m_x * 2.0f / m_pManager->m_nBackBufferWidth - 1.0f;
		Right = (m_x + m_width) * 2.0f / m_pManager->m_nBackBufferWidth - 1.0f;
		Top = 1.0f - m_y * 2.0f / m_pManager->m_nBackBufferHeight;
		Bottom = 1.0f - (m_y + m_height) * 2.0f / m_pManager->m_nBackBufferHeight;*/

		/*glm::vec3 vertices[4] =
		{
			glm::vec3(Left, Top, 0.5f), m_colorTopLeft, glm::vec2(0.0f, 0.0f),
			glm::vec3(Right, Top, 0.5f), m_colorTopRight, glm::vec2(1.0f, 0.0f),
			glm::vec3(Left, Bottom, 0.5f), m_colorBottomLeft, glm::vec2(0.0f, 1.0f),
			glm::vec3(Right, Bottom, 0.5f), m_colorBottomRight, glm::vec2(1.0f, 1.0f)
		};*/

		glm::vec3 positions[4] = 
		{ 
			glm::vec3(windowCoords.left, windowCoords.top, -0.99f),
			glm::vec3(windowCoords.right, windowCoords.top, -0.99f),
			glm::vec3(windowCoords.left, windowCoords.bottom, -0.99f),
			glm::vec3(windowCoords.right, windowCoords.bottom, -0.99f)
		};

		Color4f colors[4] = 
		{
			GLUFColorToFloat(m_colorBottomLeft),
			GLUFColorToFloat(m_colorTopRight),
			GLUFColorToFloat(m_colorBottomLeft),
			GLUFColorToFloat(m_colorBottomRight)
		};

		glm::vec2 texCoords[4] = 
		{
			glm::vec2(0.0f, 0.0f),
			glm::vec2(1.0f, 0.0f),
			glm::vec2(0.0f, 1.0f),
			glm::vec2(1.0f, 1.0f) 
		};

		//GLUF_SCREEN_VERTEX_10 *pVB;
		/*D3D11_MAPPED_SUBRESOURCE MappedData;
		if (SUCCEEDED(pd3dDeviceContext->Map(m_pManager->m_pVBScreenQuad11, 0, D3D11_MAP_WRITE_DISCARD,
			0, &MappedData)))
		{
			memcpy(MappedData.pData, vertices, sizeof(vertices));
			pd3dDeviceContext->Unmap(m_pManager->m_pVBScreenQuad11, 0);
		}*/

		glBindVertexArray(m_pManager->m_pVBScreenQuadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_pManager->m_pVBScreenQuadPositions);
		glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec3), positions, GL_STREAM_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, m_pManager->m_pVBScreenQuadColor);
		glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(Color4f), colors, GL_STREAM_DRAW);

		glBindBuffer(GL_ARRAY_BUFFER, m_pManager->m_pVBScreenQuadUVs);
		glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec2), texCoords, GL_STREAM_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pManager->m_pVBScreenQuadIndicies);
		

		// Set the quad VB as current
		/*UINT stride = sizeof(GLUF_SCREEN_VERTEX_10);
		UINT offset = 0;
		pd3dDeviceContext->IASetVertexBuffers(0, 1, &m_pManager->m_pVBScreenQuad11, &stride, &offset);
		pd3dDeviceContext->IASetInputLayout(m_pManager->m_pInputLayout11);
		pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);*/

		// Setup for rendering
		//m_pManager->ApplyRenderUIUntex11(pd3dDeviceContext);
		//pd3dDeviceContext->Draw(4, 0);


		//glEnableVertexAttribArray(0);//Positions
		//glEnableVertexAttribArray(1);//Colors
		//glEnableVertexAttribArray(2);//UVs

		//GLUFSHADERMANAGER.UseProgram(g_UIProgram);
		m_pManager->ApplyRenderUIUntex();



		//well we are drawing a square (would there be any reason to make this GL_TRIANGLES?)
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);
	}

	GLUFTextureNode* pTextureNode = GetTexture(0);
	//pd3dDeviceContext->PSSetShaderResources(0, 1, &pTextureNode->pTexResView11);
	
	/*glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pTextureNode->m_pTextureElement);
	glUniform1f(m_pManager->m_pSamplerLocation, 0);0 corresponds to GL_TEXTURE0
	*/

	// Sort depth back to front
	m_pManager->BeginSprites();
	BeginText(m_pManager->GetOrthoMatrix());

	m_pManager->ApplyRenderUI();

	GLUFBUFFERMANAGER.UseTexture(pTextureNode->m_pTextureElement, m_pManager->m_pSamplerLocation, GL_TEXTURE0);

	// Render the caption if it's enabled.
	/*if (m_bCaption)
	{
		// DrawSprite will offset the rect down by
		// m_nCaptionHeight, so adjust the rect higher
		// here to negate the effect.
		GLUFRect rc = { 0, m_nCaptionHeight, m_width, 0 };
		DrawSprite(&m_CapElement, rc, -0.99f);
		rc.left += 5; // Make a left margin
		if (m_bMinimized)
			DrawText("(Minimized)", &m_CapElement, rc, true);
		DrawText(m_wszCaption, &m_CapElement, rc, true);
	}*/

	// If the dialog is minimized, skip rendering
	// its controls.
	if (!m_bMinimized)
	{
		for (auto it = m_Controls.cbegin(); it != m_Controls.cend(); ++it)
		{
			// Focused control is drawn last
			if (*it == s_pControlFocus)
				continue;

			(*it)->Render(fElapsedTime);
		}

		if (s_pControlFocus && s_pControlFocus->m_pDialog == this)
			s_pControlFocus->Render(fElapsedTime);
	}

	// End sprites
	/*if (m_bCaption)
	{
		m_pManager->EndSprites();
		EndText();
	}*/
	//m_pManager->RestoreD3D11State(pd3dDeviceContext);

	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------

void GLUFDialog::SendEvent(GLUF_EVENT nEvent, bool bTriggeredByUser, GLUFControl* pControl)
{
	// If no callback has been registered there's nowhere to send the event to
	if (!m_pCallbackEvent)
		return;

	// Discard events triggered programatically if these types of events haven't been
	// enabled
	if (!bTriggeredByUser && !m_bNonUserEvents)
		return;

	m_pCallbackEvent(nEvent, pControl->GetID(), pControl);
}


//--------------------------------------------------------------------------------------

GLUFResult GLUFDialog::SetFont(GLUFFontIndex index, GLUFFontIndex resManFontIndex)
{
	// If this assert triggers, you need to call GLUFDialog::Init() first.  This change
	// was made so that the GLUF's GUI could become seperate and optional from GLUF's core.  The 
	// creation and interfacing with GLUFDialogResourceManager is now the responsibility 
	// of the application if it wishes to use GLUF's GUI.
	GLUF_ASSERT(m_pManager && L"To fix call GLUFDialog::Init() first.  See comments for details.");
	//_Analysis_assume_(m_pManager);


	for (size_t i = m_Fonts.size(); i <= index; i++)
	{
		m_Fonts.push_back(-1);
	}

	//int iFont = m_pManager->AddFont(font, height, weight);
	m_Fonts[index] = resManFontIndex;

	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------
GLUFFontNode* GLUFDialog::GetFont(unsigned int index) const
{
	if (!m_pManager)
		return nullptr;
	return m_pManager->GetFontNode(m_Fonts[index]);
}


//--------------------------------------------------------------------------------------

GLUFResult GLUFDialog::SetTexture(GLUFTextureIndex index, GLUFTextureIndex resManIndex)
{
	// If this assert triggers, you need to call GLUFDialog::Init() first.  This change
	// was made so that the GLUF's GUI could become seperate and optional from GLUF's core.  The 
	// creation and interfacing with GLUFDialogResourceManager is now the responsibility 
	// of the application if it wishes to use GLUF's GUI.
	GLUF_ASSERT(m_pManager && L"To fix this, call GLUFDialog::Init() first.  See comments for details.");
	//_Analysis_assume_(m_pManager);

	// Make sure the list is at least as large as the index being set
	for (size_t i = m_Textures.size(); i <= index; i++)
	{
		m_Textures.push_back(-1);
	}

	m_Textures[index] = resManIndex;
	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------
/* We will not support resources based on windows resource modules
GLUFResult GLUFDialog::SetTexture(UINT index, LPCWSTR strResourceName, HMODULE hResourceModule)
{
	// If this assert triggers, you need to call GLUFDialog::Init() first.  This change
	// was made so that the GLUF's GUI could become seperate and optional from GLUF's core.  The 
	// creation and interfacing with GLUFDialogResourceManager is now the responsibility 
	// of the application if it wishes to use GLUF's GUI.
	assert(m_pManager && L"To fix this, call GLUFDialog::Init() first.  See comments for details.");
	_Analysis_assume_(m_pManager);

	// Make sure the list is at least as large as the index being set
	for (size_t i = m_Textures.size(); i <= index; i++)
	{
		m_Textures.push_back(-1);
	}

	int iTexture = m_pManager->AddTexture(strResourceName, hResourceModule);

	m_Textures[index] = iTexture;
	return GR_SUCCESS;
}
*/

//--------------------------------------------------------------------------------------
GLUFTextureNode* GLUFDialog::GetTexture(unsigned int index) const
{
	if (!m_pManager)
		return nullptr;
	return m_pManager->GetTextureNode(m_Textures[index]);
}


//--------------------------------------------------------------------------------------

bool GLUFDialog::MsgProc(GLUF_MESSAGE_TYPE msg, int32_t param1, int32_t param2, int32_t param3, int32_t param4)
{
	//first, even if we are not going to use it, snatch up the cursor position just in case it moves in the time it takes to do this
	double x, y;
	glfwGetCursorPos(g_pGLFWWindow, &x, &y);
	m_MousePosition = GLUFPoint((float)x, (float)y);

	//reverse orthographic TODO:
	/*GLUFPoint orthoPt = m_pManager->GetOrthoPoint();
	GLUFNormPoint(mousePos, m_pManager->GetWindowSize());
	GLUFFlipPoint(mousePos);
	if (orthoPt.x == 1.0f && orthoPt.y == 1.0f)
	{ }
	else if (orthoPt.y == 1.0f)
	{
		//Offset = (orthoPt.x - 1.0f) / 2;
		mousePos.x -= ((orthoPt.x - 1.0f));
	}
	else if (orthoPt.x == 1.0f)
	{
		//Offset = (orthoPt.y - 1.0f) / 2;
		mousePos.y -= ((orthoPt.y - 1.0f) / 2.0f);
	}*/

	//TODO: make this more efficient
	GLUFPoint screenPt = m_pManager->GetWindowSize();
	//mousePos.x += screenPt.x / 2;
	m_MousePosition.y = (screenPt.y - m_MousePosition.y);// +screenPt.y / 2;

	if (screenPt.x >= screenPt.y)
	{
		//mousePos.y /= screenPt.y / 2;
		//mousePos.x /= screenPt.y / 2;
		float diff = (screenPt.x - screenPt.y);
		float diff2 = diff / 2.0f;
		m_MousePosition.x -= diff2;
		m_MousePosition.x /= screenPt.x - diff;
		m_MousePosition.y /= screenPt.y;
	}
	else
	{
		//mousePos.y /= screenPt.x / 2;
		//mousePos.x /= screenPt.x / 2;
		float diff = (screenPt.y - screenPt.x);
		float diff2 = diff / 2.0f;
		m_MousePosition.y -= diff2;
		m_MousePosition.y /= screenPt.y - diff;
		m_MousePosition.x /= screenPt.x;
	}


	m_MousePositionDialogSpace.x = m_MousePosition.x - m_x;
	m_MousePositionDialogSpace.y = m_MousePosition.y - m_y;

	//mousePos = GLUFMultPoints(mousePos, m_pManager->GetOrthoPoint());

	bool bHandled = false;

	// For invisible dialog, do not handle anything.
	if (!m_bVisible)
		return false;

	// If automation command-line switch is on, enable this dialog's keyboard input
	// upon any key press or mouse click. ????
	/*if (GLUFGetAutomation() &&
		(WM_LBUTTONDOWN == uMsg || WM_LBUTTONDBLCLK == uMsg || WM_KEYDOWN == uMsg))
	{
		m_pManager->EnableKeyboardInputForAllDialogs();
	}*/

	// If caption is enable, check for clicks in the caption area.
	if (m_bCaption)
	{
		if (((msg == GM_MB) == true) &&
			((param1 == GLFW_MOUSE_BUTTON_LEFT) == true) &&
			((param2 == GLFW_PRESS) == true) )
		{

			if (m_MousePosition.x >= m_x && m_MousePosition.x < m_x + m_width &&
				m_MousePosition.y >= m_y && m_MousePosition.y < m_y + m_nCaptionHeight)
			{
				m_bDrag = true;
				//SetCapture(GLUFGetHWND());
				return true;
			}
		}
		else if ((msg == GM_MB) == true &&
				(param1 == GLFW_MOUSE_BUTTON_LEFT) == true &&
				(param2 == GLFW_RELEASE) == true && 
				(m_bDrag))
		{
			if (m_MousePosition.x >= m_x && m_MousePosition.x < m_x + m_width &&
				m_MousePosition.y >= m_y && m_MousePosition.y < m_y + m_nCaptionHeight)
			{
				//ReleaseCapture();
				m_bDrag = false;
				m_bMinimized = !m_bMinimized;
				return true;
			}
		}
	}

	// If the dialog is minimized, don't send any messages to controls.
	if (m_bMinimized)
		return false;

	// If a control is in focus, it belongs to this dialog, and it's enabled, then give
	// it the first chance at handling the message.
	if (s_pControlFocus &&
		s_pControlFocus->m_pDialog == this &&
		s_pControlFocus->GetEnabled())
	{
		// If the control MsgProc handles it, then we don't.
		if (s_pControlFocus->MsgProc(msg, param1, param2, param3, param4))
			return true;
	}

	switch (msg)
	{
	case GM_RESIZE:
	case GM_POS:
	{
		// Handle sizing and moving messages so that in case the mouse cursor is moved out
		// of an UI control because of the window adjustment, we can properly
		// unhighlight the highlighted control.
		GLUFPoint pt =
		{
			-1, -1
		};
		OnMouseMove(pt);
		break;
	}

	case GM_FOCUS:
		// Call OnFocusIn()/OnFocusOut() of the control that currently has the focus
		// as the application is activated/deactivated.  This matches the Windows
		// behavior.
		if (s_pControlFocus &&
			s_pControlFocus->m_pDialog == this &&
			s_pControlFocus->GetEnabled())
		{
			if (param1 == GL_TRUE)
				s_pControlFocus->OnFocusIn();
			else
				s_pControlFocus->OnFocusOut();
		}
		break;

		// Keyboard messages
	case GM_KEY:
	//case WM_SYSKEYDOWN:
	//case WM_KEYUP:
	//case WM_SYSKEYUP:
	{
		// If a control is in focus, it belongs to this dialog, and it's enabled, then give
		// it the first chance at handling the message.
		if (s_pControlFocus &&
			s_pControlFocus->m_pDialog == this &&
			s_pControlFocus->GetEnabled())
			for (auto it = m_Controls.cbegin(); it != m_Controls.cend(); ++it)
			{
			if (s_pControlFocus->MsgProc(msg, param1, param2, param3, param4))
				return true;
			}

		// Not yet handled, see if this matches a control's hotkey
		// Activate the hotkey if the focus doesn't belong to an
		// edit box.
		if (param3 == GLFW_PRESS && (!s_pControlFocus ||
			(s_pControlFocus->GetType() != GLUF_CONTROL_EDITBOX
			&& s_pControlFocus->GetType() != GLUF_CONTROL_IMEEDITBOX)))
		{
			for (auto it = m_Controls.begin(); it != m_Controls.end(); ++it)
			{
				if ((*it)->GetHotkey() == param1)
				{
					(*it)->OnHotkey();
					return true;
				}
			}
		}

		// Not yet handled, check for focus messages
		if (param3 == GLFW_PRESS)
		{
			// If keyboard input is not enabled, this message should be ignored
			if (!m_bKeyboardInput)
				return false;

			switch (param1)
			{
			case GLFW_KEY_RIGHT:
			case GLFW_KEY_DOWN:
				if (s_pControlFocus)
				{
					return OnCycleFocus(true);
				}
				break;

			case GLFW_KEY_LEFT:
			case GLFW_KEY_UP:
				if (s_pControlFocus)
				{
					return OnCycleFocus(false);
				}
				break;

			case GLFW_KEY_TAB:
			{
				bool bShiftDown =(glfwGetKey(g_pGLFWWindow, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) ||
								(glfwGetKey(g_pGLFWWindow, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);
				return OnCycleFocus(!bShiftDown);
			}
			}
		}

		break;
	}


		// Mouse messages
	//case WM_MOUSEMOVE:
	//case WM_LBUTTONDOWN:
	//case WM_LBUTTONUP:
	//case WM_MBUTTONDOWN:
	//case WM_MBUTTONUP:
	//case WM_RBUTTONDOWN:
	//case WM_RBUTTONUP:
	//case WM_XBUTTONDOWN:
	//case WM_XBUTTONUP:
	//case WM_LBUTTONDBLCLK:
	//case WM_MBUTTONDBLCLK:
	//case WM_RBUTTONDBLCLK:
	//case WM_XBUTTONDBLCLK:
	//case WM_MOUSEWHEEL:
	case GM_MB:
	case GM_SCROLL:
	case GM_CURSOR_POS:
	{
		// If not accepting mouse input, return false to indicate the message should still 
		// be handled by the application (usually to move the camera).
		if (!m_bMouseInput)
			return false;


		// If caption is enabled, offset the Y coordinate by its height.
		//if (m_bCaption)
		//	m_MousePosition.y += m_nCaptionHeight;

		// If a control is in focus, it belongs to this dialog, and it's enabled, then give
		// it the first chance at handling the message.
		if (s_pControlFocus &&
			s_pControlFocus->m_pDialog == this &&
			s_pControlFocus->GetEnabled())
		{
			if (s_pControlFocus->MsgProc(msg, param1, param2, param3, param4))
				return true;
		}

		// Not yet handled, see if the mouse is over any controls
		GLUFControl* pControl = GetControlAtPoint(m_MousePositionDialogSpace);
		if (pControl && pControl->GetEnabled())
		{
			bHandled = pControl->MsgProc(msg, param1, param2, param3, param4);
			if (bHandled)
				return true;
		}
		else
		{
			// Mouse not over any controls in this dialog, if there was a control
			// which had focus it just lost it
			if (param1 == GLFW_MOUSE_BUTTON_LEFT &&
				param2 == GLFW_PRESS &&
				s_pControlFocus &&
				s_pControlFocus->m_pDialog == this)
			{
				s_pControlFocus->OnFocusOut();
				s_pControlFocus = nullptr;
			}
		}

		// Still not handled, hand this off to the dialog. Return false to indicate the
		// message should still be handled by the application (usually to move the camera).
		switch (msg)
		{
		case GM_CURSOR_POS:
			OnMouseMove(m_MousePositionDialogSpace);
			return false;
		}

		break;
	}

	case GM_CURSOR_ENTER:
	{
		// The application has lost mouse capture.
		// The dialog object may not have received
		// a WM_MOUSEUP when capture changed. Reset
		// m_bDrag so that the dialog does not mistakenly
		// think the mouse button is still held down.
		if (param1 == GL_FALSE)
			m_bDrag = false;
	}
	}

	return false;
}


//--------------------------------------------------------------------------------------
GLUFControl* GLUFDialog::GetControlAtPoint(GLUFPoint pt) 
{
	// Search through all child controls for the first one which
	// contains the mouse point
	for (auto it = m_Controls.cbegin(); it != m_Controls.cend(); ++it)
	{
		if (!*it)
		{
			continue;
		}

		// We only return the current control if it is visible
		// and enabled.  Because GetControlAtPoint() is used to do mouse
		// hittest, it makes sense to perform this filtering.
		if ((*it)->ContainsPoint(pt) && (*it)->GetEnabled() && (*it)->GetVisible())
		{
			return *it;
		}
	}

	return nullptr;
}


//--------------------------------------------------------------------------------------
bool GLUFDialog::GetControlEnabled(int ID)
{
	GLUFControl* pControl = GetControl(ID);
	if (!pControl)
		return false;

	return pControl->GetEnabled();
}



//--------------------------------------------------------------------------------------
void GLUFDialog::SetControlEnabled(int ID, bool bEnabled)
{
	GLUFControl* pControl = GetControl(ID);
	if (!pControl)
		return;

	pControl->SetEnabled(bEnabled);
}


//--------------------------------------------------------------------------------------
void GLUFDialog::OnMouseUp(GLUFPoint pt)
{
	GLUF_UNREFERENCED_PARAMETER(pt);
	s_pControlPressed = nullptr;
	m_pControlMouseOver = nullptr;
}


//--------------------------------------------------------------------------------------
void GLUFDialog::OnMouseMove(GLUFPoint pt)
{
	// Figure out which control the mouse is over now
	GLUFControl* pControl = GetControlAtPoint(pt);

	// If the mouse is still over the same control, nothing needs to be done
	if (pControl == m_pControlMouseOver)
		return;

	// Handle mouse leaving the old control
	if (m_pControlMouseOver)
		m_pControlMouseOver->OnMouseLeave();

	// Handle mouse entering the new control
	m_pControlMouseOver = pControl;
	if (pControl)
		m_pControlMouseOver->OnMouseEnter();
}


//--------------------------------------------------------------------------------------

GLUFResult GLUFDialog::SetDefaultElement(GLUF_CONTROL_TYPE nControlType, unsigned int iElement, GLUFElement* pElement)
{
	// If this Element type already exist in the list, simply update the stored Element
	for (auto it = m_DefaultElements.begin(); it != m_DefaultElements.end(); ++it)
	{
		if ((*it)->nControlType == nControlType &&
			(*it)->iElement == iElement)
		{
			(*it)->Element = *pElement;
			return GR_SUCCESS;
		}
	}

	// Otherwise, add a new entry
	GLUFElementHolder* pNewHolder;
	pNewHolder = new (std::nothrow) GLUFElementHolder;
	if (!pNewHolder)
		return GR_OUTOFMEMORY;

	pNewHolder->nControlType = nControlType;
	pNewHolder->iElement = iElement;
	pNewHolder->Element = *pElement;

	m_DefaultElements.push_back(pNewHolder);

	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------

GLUFElement* GLUFDialog::GetDefaultElement(GLUF_CONTROL_TYPE nControlType, unsigned int iElement)
{
	for (auto it = m_DefaultElements.cbegin(); it != m_DefaultElements.cend(); ++it)
	{
		if ((*it)->nControlType == nControlType &&
			(*it)->iElement == iElement)
		{
			return &(*it)->Element;
		}
	}

	return nullptr;
}


//--------------------------------------------------------------------------------------

GLUFResult GLUFDialog::AddStatic(int ID, std::string strText, float x, float y, float width, float height, bool bIsDefault,
GLUFStatic** ppCreated)
{
	GLUFResult hr = GR_SUCCESS;

	GLUFStatic* pStatic = new (std::nothrow)GLUFStatic(this);

	if (ppCreated)
		*ppCreated = pStatic;

	if (!pStatic)
		return GR_OUTOFMEMORY;

	hr = AddControl(pStatic);
	if (GLUF_FAILED(hr))
		return hr;

	// Set the ID and list index
	pStatic->SetID(ID);
	pStatic->SetText(strText);
	pStatic->SetLocation(x, y);
	pStatic->SetSize(width, height);
	pStatic->m_bIsDefault = bIsDefault;

	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------

GLUFResult GLUFDialog::AddButton(int ID, std::string strText, float x, float y, float width, float height, int nHotkey,
bool bIsDefault, GLUFButton** ppCreated)
{
	GLUFResult hr = GR_SUCCESS;

	GLUFButton* pButton = new (std::nothrow)GLUFButton(this);

	if (ppCreated)
		*ppCreated = pButton;

	if (!pButton)
		return GR_OUTOFMEMORY;

	hr = AddControl(pButton);
	if (GLUF_FAILED(hr))
		return hr;

	// Set the ID and list index
	pButton->SetID(ID);
	pButton->SetText(strText);
	pButton->SetLocation(x, y);
	pButton->SetSize(width, height);
	pButton->SetHotkey(nHotkey);
	pButton->m_bIsDefault = bIsDefault;

	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------

GLUFResult GLUFDialog::AddCheckBox(int ID, std::string strText, float x, float y, float width, float height, bool bChecked,
int nHotkey, bool bIsDefault, GLUFCheckBox** ppCreated)
{
	GLUFResult hr = GR_SUCCESS;

	GLUFCheckBox* pCheckBox = new (std::nothrow)GLUFCheckBox(this);

	if (ppCreated)
		*ppCreated = pCheckBox;

	if (!pCheckBox)
		return GR_OUTOFMEMORY;

	hr = AddControl(pCheckBox);
	if (GLUF_FAILED(hr))
		return hr;

	// Set the ID and list index
	pCheckBox->SetID(ID);
	pCheckBox->SetText(strText);
	pCheckBox->SetLocation(x, y);
	pCheckBox->SetSize(width, height);
	pCheckBox->SetHotkey(nHotkey);
	pCheckBox->m_bIsDefault = bIsDefault;
	pCheckBox->SetChecked(bChecked);

	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------

GLUFResult GLUFDialog::AddRadioButton(int ID, unsigned int nButtonGroup, std::string strText, float x, float y, float width, float height,
bool bChecked, int nHotkey, bool bIsDefault, GLUFRadioButton** ppCreated)
{
	GLUFResult hr = GR_SUCCESS;

	GLUFRadioButton* pRadioButton = new (std::nothrow)GLUFRadioButton(this);

	if (ppCreated)
		*ppCreated = pRadioButton;

	if (!pRadioButton)
		return GR_OUTOFMEMORY;

	hr = AddControl(pRadioButton);
	if (GLUF_FAILED(hr))
		return hr;

	// Set the ID and list index
	pRadioButton->SetID(ID);
	pRadioButton->SetText(strText);
	pRadioButton->SetButtonGroup(nButtonGroup);
	pRadioButton->SetLocation(x, y);
	pRadioButton->SetSize(width, height);
	pRadioButton->SetHotkey(nHotkey);
	pRadioButton->SetChecked(bChecked);
	pRadioButton->m_bIsDefault = bIsDefault;
	pRadioButton->SetChecked(bChecked);

	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------

GLUFResult GLUFDialog::AddComboBox(int ID, float x, float y, float width, float height, int nHotkey, bool bIsDefault,
GLUFComboBox** ppCreated)
{
	GLUFResult hr = GR_SUCCESS;

	GLUFComboBox* pComboBox = new (std::nothrow) GLUFComboBox(this);

	if (ppCreated)
		*ppCreated = pComboBox;

	if (!pComboBox)
		return GR_OUTOFMEMORY;

	hr = AddControl(pComboBox);
	if (GLUF_FAILED(hr))
		return hr;

	// Set the ID and list index
	pComboBox->SetID(ID);
	pComboBox->SetLocation(x, y);
	pComboBox->SetSize(width, height);
	pComboBox->SetHotkey(nHotkey);
	pComboBox->m_bIsDefault = bIsDefault;

	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------

GLUFResult GLUFDialog::AddSlider(int ID, float x, float y, float width, float height, float min, float max, float value,
bool bIsDefault, GLUFSlider** ppCreated)
{
	GLUFResult hr = GR_SUCCESS;

	GLUFSlider* pSlider = new (std::nothrow) GLUFSlider(this);

	if (ppCreated)
		*ppCreated = pSlider;

	if (!pSlider)
		return GR_OUTOFMEMORY;

	hr = AddControl(pSlider);
	if (GLUF_FAILED(hr))
		return hr;

	// Set the ID and list index
	pSlider->SetID(ID);
	pSlider->SetLocation(x, y);
	pSlider->SetSize(width, height);
	pSlider->m_bIsDefault = bIsDefault;
	pSlider->SetRange(min, max);
	pSlider->SetValue(value);
	pSlider->UpdateRects();

	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------

GLUFResult GLUFDialog::AddEditBox(int ID, std::string strText, float x, float y, float width, float height, bool bIsDefault,
GLUFEditBox** ppCreated)
{
	GLUFResult hr = GR_SUCCESS;

	GLUFEditBox* pEditBox = new (std::nothrow) GLUFEditBox(this);

	if (ppCreated)
		*ppCreated = pEditBox;

	if (!pEditBox)
		return GR_OUTOFMEMORY;

	hr = AddControl(pEditBox);
	if (GLUF_FAILED(hr))
		return hr;

	// Set the ID and position
	pEditBox->SetID(ID);
	pEditBox->SetLocation(x, y);
	pEditBox->SetSize(width, height);
	pEditBox->m_bIsDefault = bIsDefault;

	pEditBox->SetText(strText);

	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------

GLUFResult GLUFDialog::AddListBox(int ID, float x, float y, float width, float height, unsigned long dwStyle, GLUFListBox** ppCreated)
{
	GLUFResult hr = GR_SUCCESS;
	GLUFListBox* pListBox = new (std::nothrow) GLUFListBox(this);

	if (ppCreated)
		*ppCreated = pListBox;

	if (!pListBox)
		return GR_OUTOFMEMORY;

	hr = AddControl(pListBox);
	if (GLUF_FAILED(hr))
		return hr;

	// Set the ID and position
	pListBox->SetID(ID);
	pListBox->SetLocation(x, y);
	pListBox->SetSize(width, height);
	pListBox->SetStyle(dwStyle);

	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------
GLUFResult GLUFDialog::InitControl(GLUFControl* pControl)
{
	//GLUFResult hr;

	if (!pControl)
		return GR_INVALIDARG;

	pControl->m_Index = static_cast<unsigned int>(m_Controls.size());

	// Look for a default Element entries
	for (auto it = m_DefaultElements.begin(); it != m_DefaultElements.end(); ++it)
	{
		if ((*it)->nControlType == pControl->GetType())
			pControl->SetElement((*it)->iElement, &(*it)->Element);
	}

	GLUF_V_RETURN(pControl->OnInit());

	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------
GLUFResult GLUFDialog::AddControl(GLUFControl* pControl)
{
	GLUFResult hr = GR_SUCCESS;

	hr = InitControl(pControl);
	if (GLUF_FAILED(hr))
		return GLUFTRACE_ERR("GLUFDialog::InitControl", hr);

	// Add to the list
	m_Controls.push_back(pControl);

	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------
GLUFControl* GLUFDialog::GetControl(int ID)
{
	// Try to find the control with the given ID
	for (auto it = m_Controls.cbegin(); it != m_Controls.cend(); ++it)
	{
		if ((*it)->GetID() == ID)
		{
			return *it;
		}
	}

	// Not found
	return nullptr;
}


//--------------------------------------------------------------------------------------
GLUFControl* GLUFDialog::GetControl( int ID,  GLUF_CONTROL_TYPE nControlType)
{
	// Try to find the control with the given ID
	for (auto it = m_Controls.cbegin(); it != m_Controls.cend(); ++it)
	{
		if ((*it)->GetID() == ID && (*it)->GetType() == nControlType)
		{
			return *it;
		}
	}

	// Not found
	return nullptr;
}


//--------------------------------------------------------------------------------------
GLUFControl* GLUFDialog::GetNextControl(GLUFControl* pControl)
{
	int index = pControl->m_Index + 1;

	GLUFDialog* pDialog = pControl->m_pDialog;

	// Cycle through dialogs in the loop to find the next control. Note
	// that if only one control exists in all looped dialogs it will
	// be the returned 'next' control.
	while (index >= (int)pDialog->m_Controls.size())
	{
		pDialog = pDialog->m_pNextDialog;
		index = 0;
	}

	return pDialog->m_Controls[index];
}


//--------------------------------------------------------------------------------------
GLUFControl* GLUFDialog::GetPrevControl(GLUFControl* pControl)
{
	int index = pControl->m_Index - 1;

	GLUFDialog* pDialog = pControl->m_pDialog;

	// Cycle through dialogs in the loop to find the next control. Note
	// that if only one control exists in all looped dialogs it will
	// be the returned 'previous' control.
	while (index < 0)
	{
		pDialog = pDialog->m_pPrevDialog;
		if (!pDialog)
			pDialog = pControl->m_pDialog;

		index = int(pDialog->m_Controls.size()) - 1;
	}

	return pDialog->m_Controls[index];
}


//--------------------------------------------------------------------------------------
void GLUFDialog::ClearRadioButtonGroup(unsigned int nButtonGroup)
{
	// Find all radio buttons with the given group number
	for (auto it = m_Controls.cbegin(); it != m_Controls.cend(); ++it)
	{
		if ((*it)->GetType() == GLUF_CONTROL_RADIOBUTTON)
		{
			GLUFRadioButton* pRadioButton = (GLUFRadioButton*)*it;

			if (pRadioButton->GetButtonGroup() == nButtonGroup)
				pRadioButton->SetChecked(false, false);
		}
	}
}


//--------------------------------------------------------------------------------------
void GLUFDialog::ClearComboBox(int ID)
{
	GLUFComboBox* pComboBox = GetComboBox(ID);
	if (!pComboBox)
		return;

	pComboBox->RemoveAllItems();
}


//--------------------------------------------------------------------------------------
void GLUFDialog::RequestFocus(GLUFControl* pControl)
{
	if (s_pControlFocus == pControl)
		return;

	if (!pControl->CanHaveFocus())
		return;

	if (s_pControlFocus)
		s_pControlFocus->OnFocusOut();

	pControl->OnFocusIn();
	s_pControlFocus = pControl;
}


//--------------------------------------------------------------------------------------

GLUFResult GLUFDialog::DrawRect(GLUFRect pRect, Color color)
{
	GLUF_UNREFERENCED_PARAMETER(pRect);
	GLUF_UNREFERENCED_PARAMETER(color);
	// TODO -
	return GR_FAILURE;
}


//--------------------------------------------------------------------------------------

GLUFResult GLUFDialog::DrawSprite(GLUFElement* pElement, GLUFRect prcDest, float fDepth)
{
	// No need to draw fully transparent layers
	//if (pElement->TextureColor.Current.w == 0)
	//	return GR_SUCCESS;
	/*
	if (pElement->TextureColor.Current == pElement->TextureColor.States[GLUF_STATE_HIDDEN])
		return GR_SUCCESS;*/

	//set the blend color
	//Color4f colf = GLUFColorToFloat(pElement->TextureColor.Current);

	//glBlendFunc(GL_SRC_COLOR, GL_SOURCE0_ALPHA);
	//glBlendColor(colf.x, colf.y, colf.z, colf.a);
	//glEnablei(GL_BLEND, 0);
	//glBlendEquationSeparate(GL_FUNC_ADD, GL_MAX);

	GLUFRect rcTexture = pElement->rcTexture;

	GLUFRect rcScreen = prcDest;
	GLUFOffsetRect(rcScreen, m_x, m_y);

	// If caption is enabled, offset the Y position by its height.
	if (m_bCaption)
		GLUFOffsetRect(rcScreen, 0, -m_nCaptionHeight);

	rcScreen = GLUFScreenToClipspace(rcScreen);

	GLUFTextureNode* pTextureNode = GetTexture(pElement->iTexture);
	if (!pTextureNode)
		return GR_FAILURE;

	/*float fBBWidth = (float)m_pManager->m_nBackBufferWidth;
	float fBBHeight = (float)m_pManager->m_nBackBufferHeight;
	//float fTexWidth = (float)pTextureNode->dwWidth;
	//float fTexHeight = (float)pTextureNode->dwHeight;
	GLUFPoint texSize = GLUFBUFFERMANAGER.GetTextureSize(pTextureNode->m_pTextureElement);
	float fTexWidth = (float)texSize.x;
	float fTexHeight = (float)texSize.y;

	float fRectLeft = rcScreen.left / fBBWidth;
	float fRectTop = 1.0f - rcScreen.top / fBBHeight;
	float fRectRight = rcScreen.right / fBBWidth;
	float fRectBottom = 1.0f - rcScreen.bottom / fBBHeight;
	*/
	
	//float fRectLeft = 0.2f;
	//float fRectTop = 0.2625f;
	//float fRectRight = 0.325f;
	//float fRectBottom = 0.2f;

	// Add 6 sprite vertices
	//GLUFSpriteVertex SpriteVertex;

	/*// tri1
	//SpriteVertex.vPos = glm::vec3(fRectLeft, fRectTop, fDepth);
	//SpriteVertex.vTex = glm::vec2(fTexLeft, fTexTop);
	//SpriteVertex.vColor = pElement->TextureColor.Current;
	m_pManager->m_SpriteVertices.push_back(
		glm::vec3(rcScreen.left, rcScreen.top, fDepth), 
		pElement->TextureColor.Current, 
		glm::vec2(fTexLeft, fTexTop));

	//SpriteVertex.vPos = glm::vec3(fRectRight, fRectTop, fDepth);
	//SpriteVertex.vTex = glm::vec2(fTexRight, fTexTop);
	//SpriteVertex.vColor = pElement->TextureColor.Current;
	m_pManager->m_SpriteVertices.push_back(
		glm::vec3(rcScreen.right, rcScreen.top, fDepth), 
		pElement->TextureColor.Current, 
		glm::vec2(fTexRight, fTexTop));

	//SpriteVertex.vPos = glm::vec3(fRectLeft, fRectBottom, fDepth);
	//SpriteVertex.vTex = glm::vec2(fTexLeft, fTexBottom);
	//SpriteVertex.vColor = pElement->TextureColor.Current;
	m_pManager->m_SpriteVertices.push_back(
		glm::vec3(rcScreen.left, rcScreen.bottom, fDepth), 
		pElement->TextureColor.Current, 
		glm::vec2(fTexLeft, fTexBottom));

	// tri2
	//SpriteVertex.vPos = glm::vec3(fRectRight, fRectTop, fDepth);
	//SpriteVertex.vTex = glm::vec2(fTexRight, fTexTop);
	//SpriteVertex.vColor = pElement->TextureColor.Current;
	m_pManager->m_SpriteVertices.push_back(
		glm::vec3(rcScreen.right, rcScreen.top, fDepth), 
		pElement->TextureColor.Current, 
		glm::vec2(fTexRight, fTexTop));

	//SpriteVertex.vPos = glm::vec3(fRectRight, fRectBottom, fDepth);
	//SpriteVertex.vTex = glm::vec2(fTexRight, fTexBottom);
	//SpriteVertex.vColor = pElement->TextureColor.Current;
	m_pManager->m_SpriteVertices.push_back(
		glm::vec3(rcScreen.right, rcScreen.bottom, fDepth), 
		pElement->TextureColor.Current, 
		glm::vec2(fTexRight, fTexBottom));

	//SpriteVertex.vPos = glm::vec3(fRectLeft, fRectBottom, fDepth);
	//SpriteVertex.vTex = glm::vec2(fTexLeft, fTexBottom);
	//SpriteVertex.vColor = pElement->TextureColor.Current;
	m_pManager->m_SpriteVertices.push_back(
		glm::vec3(rcScreen.left, rcScreen.bottom, fDepth), 
		pElement->TextureColor.Current, 
		glm::vec2(fTexLeft, fTexBottom));*/


	m_pManager->m_SpriteVertices.push_back(
		glm::vec3(rcScreen.left, rcScreen.top, fDepth),
		pElement->TextureColor.Current,
		glm::vec2(rcTexture.left, rcTexture.top));

	m_pManager->m_SpriteVertices.push_back(
		glm::vec3(rcScreen.right, rcScreen.top, fDepth),
		pElement->TextureColor.Current,
		glm::vec2(rcTexture.right, rcTexture.top));

	m_pManager->m_SpriteVertices.push_back(
		glm::vec3(rcScreen.left, rcScreen.bottom, fDepth),
		pElement->TextureColor.Current,
		glm::vec2(rcTexture.left, rcTexture.bottom));

	m_pManager->m_SpriteVertices.push_back(
		glm::vec3(rcScreen.right, rcScreen.bottom, fDepth),
		pElement->TextureColor.Current,
		glm::vec2(rcTexture.right, rcTexture.bottom));


	// Why are we drawing the sprite every time?  This is very inefficient, but the sprite workaround doesn't have support for sorting now, so we have to
	// draw a sprite every time to keep the order correct between sprites and text.
	m_pManager->EndSprites();

	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------

GLUFResult GLUFDialog::CalcTextRect(std::string strText, GLUFElement* pElement, GLUFRect prcDest, int nCount)
{
	GLUFFontNode* pFontNode = GetFont(pElement->iFont);
	if (!pFontNode)
		return GR_FAILURE;

	GLUF_UNREFERENCED_PARAMETER(strText);
	GLUF_UNREFERENCED_PARAMETER(prcDest);
	GLUF_UNREFERENCED_PARAMETER(nCount);
	// TODO -

	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------

GLUFResult GLUFDialog::DrawText(std::string strText, GLUFElement* pElement, GLUFRect prcDest, bool bShadow, bool bCenter)
{
	// No need to draw fully transparent layers
	if (pElement->FontColor.Current.w == 0)
		return GR_SUCCESS;

	GLUFRect rcScreen = prcDest;
	GLUFOffsetRect(rcScreen, m_x, m_y);

	// If caption is enabled, offset the Y position by its height.
	if (m_bCaption)
		GLUFOffsetRect(rcScreen, 0, m_nCaptionHeight);

	//float fBBWidth = (float)m_pManager->m_nBackBufferWidth;
	//float fBBHeight = (float)m_pManager->m_nBackBufferHeight;

	//auto pd3dDevice = m_pManager->GetD3D11Device();
	//auto pd3d11DeviceContext = m_pManager->GetD3D11DeviceContext();

	if (bShadow)
	{
		GLUFRect rcShadow = rcScreen;
		GLUFOffsetRect(rcShadow, 1 / m_pManager->GetWindowSize().x, 1 / m_pManager->GetWindowSize().y);

		Color vShadowColor(0, 0, 0, 255);
		DrawTextGLUF(*m_pManager->GetFontNode(pElement->iFont), strText, rcShadow, vShadowColor, bCenter);

	}

	Color vFontColor = pElement->FontColor.Current;
	DrawTextGLUF(*m_pManager->GetFontNode(pElement->iFont), strText, rcScreen, vFontColor, bCenter);

	//reenable the control texture
	GLUFTextureNode* pTextureNode = GetTexture(0);
	GLUFBUFFERMANAGER.UseTexture(pTextureNode->m_pTextureElement, m_pManager->m_pSamplerLocation, GL_TEXTURE0);

	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------

void GLUFDialog::SetBackgroundColors(Color colorTopLeft, Color colorTopRight, Color colorBottomLeft,
	Color colorBottomRight)
{
	m_colorTopLeft = colorTopLeft;
	m_colorTopRight = colorTopRight;
	m_colorBottomLeft = colorBottomLeft;
	m_colorBottomRight = colorBottomRight;
}


//--------------------------------------------------------------------------------------
void GLUFDialog::SetNextDialog(GLUFDialog* pNextDialog)
{
	if (!pNextDialog)
		pNextDialog = this;

	m_pNextDialog = pNextDialog;
	if (pNextDialog)
		m_pNextDialog->m_pPrevDialog = this;
}


//--------------------------------------------------------------------------------------
void GLUFDialog::ClearFocus()
{
	if (s_pControlFocus)
	{
		s_pControlFocus->OnFocusOut();
		s_pControlFocus = nullptr;
	}

	//ReleaseCapture();
}


//--------------------------------------------------------------------------------------
void GLUFDialog::FocusDefaultControl()
{
	// Check for default control in this dialog
	for (auto it = m_Controls.cbegin(); it != m_Controls.cend(); ++it)
	{
		if ((*it)->m_bIsDefault)
		{
			// Remove focus from the current control
			ClearFocus();

			// Give focus to the default control
			s_pControlFocus = *it;
			s_pControlFocus->OnFocusIn();
			return;
		}
	}
}


//--------------------------------------------------------------------------------------
bool GLUFDialog::OnCycleFocus(bool bForward)
{
	GLUFControl* pControl = nullptr;
	GLUFDialog* pDialog = nullptr; // pDialog and pLastDialog are used to track wrapping of
	GLUFDialog* pLastDialog;    // focus from first control to last or vice versa.

	if (!s_pControlFocus)
	{
		// If s_pControlFocus is nullptr, we focus the first control of first dialog in
		// the case that bForward is true, and focus the last control of last dialog when
		// bForward is false.
		//
		if (bForward)
		{
			// Search for the first control from the start of the dialog
			// array.
			for (auto it = m_pManager->m_Dialogs.cbegin(); it != m_pManager->m_Dialogs.cend(); ++it)
			{
				pDialog = pLastDialog = *it;
				if (pDialog && !pDialog->m_Controls.empty())
				{
					pControl = pDialog->m_Controls[0];
					break;
				}
			}

			if (!pDialog || !pControl)
			{
				// No dialog has been registered yet or no controls have been
				// added to the dialogs. Cannot proceed.
				return true;
			}
		}
		else
		{
			// Search for the first control from the end of the dialog
			// array.
			for (auto it = m_pManager->m_Dialogs.crbegin(); it != m_pManager->m_Dialogs.crend(); ++it)
			{
				pDialog = pLastDialog = *it;
				if (pDialog && !pDialog->m_Controls.empty())
				{
					pControl = pDialog->m_Controls[pDialog->m_Controls.size() - 1];
					break;
				}
			}

			if (!pDialog || !pControl)
			{
				// No dialog has been registered yet or no controls have been
				// added to the dialogs. Cannot proceed.
				return true;
			}
		}
	}
	else if (s_pControlFocus->m_pDialog != this)
	{
		// If a control belonging to another dialog has focus, let that other
		// dialog handle this event by returning false.
		//
		return false;
	}
	else
	{
		// Focused control belongs to this dialog. Cycle to the
		// next/previous control.
		GLUF_ASSERT(pControl != 0);
		//_Analysis_assume_(pControl != 0);
		pLastDialog = s_pControlFocus->m_pDialog;
		pControl = (bForward) ? GetNextControl(s_pControlFocus) : GetPrevControl(s_pControlFocus);
		pDialog = pControl->m_pDialog;
	}

	GLUF_ASSERT(pControl != 0);
	//_Analysis_assume_(pControl != 0);

	for (int i = 0; i < 0xffff; i++)
	{
		// If we just wrapped from last control to first or vice versa,
		// set the focused control to nullptr. This state, where no control
		// has focus, allows the camera to work.
		int nLastDialogIndex = -1;
		auto fit = std::find(m_pManager->m_Dialogs.cbegin(), m_pManager->m_Dialogs.cend(), pLastDialog);
		if (fit != m_pManager->m_Dialogs.cend())
		{
			nLastDialogIndex = int(fit - m_pManager->m_Dialogs.begin());
		}

		int nDialogIndex = -1;
		fit = std::find(m_pManager->m_Dialogs.cbegin(), m_pManager->m_Dialogs.cend(), pDialog);
		if (fit != m_pManager->m_Dialogs.cend())
		{
			nDialogIndex = int(fit - m_pManager->m_Dialogs.begin());
		}

		if ((!bForward && nLastDialogIndex < nDialogIndex) ||
			(bForward && nDialogIndex < nLastDialogIndex))
		{
			if (s_pControlFocus)
				s_pControlFocus->OnFocusOut();
			s_pControlFocus = nullptr;
			return true;
		}

		// If we've gone in a full circle then focus doesn't change
		if (pControl == s_pControlFocus)
			return true;

		// If the dialog accepts keybord input and the control can have focus then
		// move focus
		if (pControl->m_pDialog->m_bKeyboardInput && pControl->CanHaveFocus())
		{
			if (s_pControlFocus)
				s_pControlFocus->OnFocusOut();
			s_pControlFocus = pControl;
			if (s_pControlFocus)
				s_pControlFocus->OnFocusIn();
			return true;
		}

		pLastDialog = pDialog;
		pControl = (bForward) ? GetNextControl(pControl) : GetPrevControl(pControl);
		pDialog = pControl->m_pDialog;
	}

	// If we reached this point, the chain of dialogs didn't form a complete loop
	GLUFTRACE_ERR("GLUFDialog: Multiple dialogs are improperly chained together", GR_FAILURE);
	return false;
}


//--------------------------------------------------------------------------------------
void GLUFDialog::InitDefaultElements()
{
	//TODO: make this less "Windows" dependent and ship with a font
	//GLUFFontPtr font = GLUFLoadFont("arial");
	char* rawData;
	unsigned long rawSize = 0;

	rawData = GLUFLoadFileIntoMemory("default.bff", &rawSize);
	GLUFFontPtr font = GLUFLoadFont(rawData, rawSize);
	free(rawData);

	int fontIndex = m_pManager->AddFont(font, 0.0625, FONT_WEIGHT_NORMAL);
	SetFont(0, fontIndex);


	GLUFElement Element;
	GLUFRect rcTexture;

	//-------------------------------------
	// Element for the caption
	//-------------------------------------
	m_CapElement.SetFont(0);
	GLUFSetRect(rcTexture, 0.0f, 0.078125f, 0.4296875f, 0.0f);//blank part of the texture
	m_CapElement.SetTexture(0, &rcTexture);
	m_CapElement.TextureColor.States[GLUF_STATE_NORMAL] = Color(255, 255, 255, 255);
	m_CapElement.FontColor.States[GLUF_STATE_NORMAL] = Color(255, 255, 255, 255);
	m_CapElement.SetFont(0, Color(255, 255, 255, 255), GT_LEFT | GT_VCENTER);
	// Pre-blend as we don't need to transition the state
	m_CapElement.TextureColor.Blend(GLUF_STATE_NORMAL, 10.0f);
	m_CapElement.FontColor.Blend(GLUF_STATE_NORMAL, 10.0f);

	//-------------------------------------
	// GLUFStatic
	//-------------------------------------
	Element.SetFont(0);
	Element.FontColor.States[GLUF_STATE_DISABLED] = Color(200, 200, 200, 200);

	// Assign the Element
	SetDefaultElement(GLUF_CONTROL_STATIC, 0, &Element);


	//-------------------------------------
	// GLUFButton - Button
	//-------------------------------------
	GLUFSetRect(rcTexture, 0.0f, 1.0f, 0.53125f, 0.7890625f);
	//GLUFSetRect(rcTexture, 0.53125f, 1.0f, 0.984375f, 0.7890625f);
	Element.SetTexture(0, &rcTexture);
	Element.SetFont(0);
	Element.TextureColor.States[GLUF_STATE_NORMAL] = Color(255, 255, 255, 75);
	Element.TextureColor.States[GLUF_STATE_PRESSED] = Color(255, 255, 255, 100);
	Element.FontColor.States[GLUF_STATE_MOUSEOVER] = Color(0, 0, 0, 50);

	// Assign the Element
	SetDefaultElement(GLUF_CONTROL_BUTTON, 0, &Element);


	//-------------------------------------
	// GLUFButton - Fill layer
	//-------------------------------------
	GLUFSetRect(rcTexture, 0.53125f, 1.0f, 0.984375f, 0.7890625f);
	Element.SetTexture(0, &rcTexture, Color(255, 255, 255, 0));
	Element.TextureColor.States[GLUF_STATE_MOUSEOVER] = Color(200, 200, 200, 40);
	Element.TextureColor.States[GLUF_STATE_PRESSED] = Color(0, 0, 0, 30);
	Element.TextureColor.States[GLUF_STATE_FOCUS] = Color(255, 255, 255, 15);


	// Assign the Element
	SetDefaultElement(GLUF_CONTROL_BUTTON, 1, &Element);


	//-------------------------------------
	// GLUFCheckBox - Box
	//-------------------------------------
	GLUFSetRect(rcTexture, 0.0f, 0.7890625f, 0.10546875f, 0.68359375f);
	Element.SetTexture(0, &rcTexture);
	Element.SetFont(0, Color(255, 255, 255, 255), GT_LEFT | GT_VCENTER);
	Element.FontColor.States[GLUF_STATE_DISABLED] = Color(200, 200, 200, 100);
	Element.TextureColor.States[GLUF_STATE_NORMAL] = Color(255, 255, 255, 75);
	Element.TextureColor.States[GLUF_STATE_FOCUS] = Color(255, 255, 255, 100);
	Element.TextureColor.States[GLUF_STATE_PRESSED] = Color(255, 255, 255, 127);

	// Assign the Element
	SetDefaultElement(GLUF_CONTROL_CHECKBOX, 0, &Element);


	//-------------------------------------
	// GLUFCheckBox - Check
	//-------------------------------------
	GLUFSetRect(rcTexture, 0.10546875f, 0.7890625f, 0.2109375f, 0.68359375f);
	Element.SetTexture(0, &rcTexture);

	// Assign the Element
	SetDefaultElement(GLUF_CONTROL_CHECKBOX, 1, &Element);


	//-------------------------------------
	// GLUFRadioButton - Box
	//-------------------------------------
	GLUFSetRect(rcTexture, 0.2109375f, 0.7890625f, 0.31640625f, 0.68359375f);
	Element.SetTexture(0, &rcTexture);
	Element.SetFont(0, Color(255, 255, 255, 255), GT_LEFT | GT_VCENTER);
	Element.FontColor.States[GLUF_STATE_DISABLED] = Color(200, 200, 200, 100);
	Element.TextureColor.States[GLUF_STATE_NORMAL] = Color(255, 255, 255, 75);
	Element.TextureColor.States[GLUF_STATE_FOCUS] = Color(255, 255, 255, 100);
	Element.TextureColor.States[GLUF_STATE_PRESSED] = Color(255, 255, 255, 127);

	// Assign the Element
	SetDefaultElement(GLUF_CONTROL_RADIOBUTTON, 0, &Element);


	//-------------------------------------
	// GLUFRadioButton - Check
	//-------------------------------------
	GLUFSetRect(rcTexture, 0.31640625f, 0.7890625f, 0.421875f, 0.68359375f);
	Element.SetTexture(0, &rcTexture);
	//Element.TextureColor.States[GLUF_STATE_HIDDEN] = Color(255, 255, 255, 255);

	// Assign the Element
	SetDefaultElement(GLUF_CONTROL_RADIOBUTTON, 1, &Element);


	//-------------------------------------
	// GLUFComboBox - Main
	//-------------------------------------
	GLUFSetRect(rcTexture, 0.02734375f, 0.5234375f, 0.96484375f, 0.3671875f);
	Element.SetTexture(0, &rcTexture);
	Element.SetFont(0);
	Element.TextureColor.States[GLUF_STATE_NORMAL] = Color(200, 200, 200, 150);
	Element.TextureColor.States[GLUF_STATE_FOCUS] = Color(230, 230, 230, 170);
	Element.TextureColor.States[GLUF_STATE_DISABLED] = Color(200, 200, 200, 70);
	Element.FontColor.States[GLUF_STATE_MOUSEOVER] = Color(0, 0, 0, 255);
	Element.FontColor.States[GLUF_STATE_PRESSED] = Color(0, 0, 0, 255);
	Element.FontColor.States[GLUF_STATE_DISABLED] = Color(200, 200, 200, 200);


	// Assign the Element
	SetDefaultElement(GLUF_CONTROL_COMBOBOX, 0, &Element);


	//-------------------------------------
	// GLUFComboBox - Button
	//-------------------------------------
	GLUFSetRect(rcTexture, 0.3828125f, 0.26171875f, 0.58984375f, 0.0703125f);
	Element.SetTexture(0, &rcTexture);
	Element.TextureColor.States[GLUF_STATE_NORMAL] = Color(255, 255, 255, 75);
	Element.TextureColor.States[GLUF_STATE_MOUSEOVER] = Color(255, 255, 255, 100);
	Element.TextureColor.States[GLUF_STATE_PRESSED] = Color(150, 150, 150, 127);
	Element.TextureColor.States[GLUF_STATE_FOCUS] = Color(255, 255, 255, 100);
	Element.TextureColor.States[GLUF_STATE_DISABLED] = Color(255, 255, 255, 35);

	// Assign the Element
	SetDefaultElement(GLUF_CONTROL_COMBOBOX, 1, &Element);


	//-------------------------------------
	// GLUFComboBox - Dropdown
	//-------------------------------------
	GLUFSetRect(rcTexture, 0.05078125f, 0.51953125f, 0.94140625f, 0.37109375f);
	Element.SetTexture(0, &rcTexture);
	Element.SetFont(0, Color(0, 0, 0, 255), GT_LEFT | GT_TOP);

	// Assign the Element
	SetDefaultElement(GLUF_CONTROL_COMBOBOX, 2, &Element);


	//-------------------------------------
	// GLUFComboBox - Selection
	//-------------------------------------
	GLUFSetRect(rcTexture, 0.046875f, 0.36328125f, 0.93359375f, 0.28515625f);
	Element.SetTexture(0, &rcTexture);
	Element.SetFont(0, Color(255, 255, 255, 255), GT_LEFT | GT_TOP);

	// Assign the Element
	SetDefaultElement(GLUF_CONTROL_COMBOBOX, 3, &Element);


	//-------------------------------------
	// GLUFSlider - Track
	//-------------------------------------
	GLUFSetRect(rcTexture, 0.00390625f, 0.26953125f, 0.36328125f, 0.109375f);
	Element.SetTexture(0, &rcTexture);
	Element.TextureColor.States[GLUF_STATE_NORMAL] = Color(255, 255, 255, 75);
	Element.TextureColor.States[GLUF_STATE_FOCUS] = Color(255, 255, 255, 100);
	Element.TextureColor.States[GLUF_STATE_DISABLED] = Color(255, 255, 255, 35);

	// Assign the Element
	SetDefaultElement(GLUF_CONTROL_SLIDER, 0, &Element);

	//-------------------------------------
	// GLUFSlider - Button
	//-------------------------------------
	GLUFSetRect(rcTexture, 0.58984375f, 0.24609375f, 0.75f, 0.0859375f);
	Element.SetTexture(0, &rcTexture);

	// Assign the Element
	SetDefaultElement(GLUF_CONTROL_SLIDER, 1, &Element);

	//-------------------------------------
	// GLUFScrollBar - Track
	//-------------------------------------
	float nScrollBarStartX = 0.765625f;
	float nScrollBarStartY = 0.234375f;
	GLUFSetRect(rcTexture, nScrollBarStartX + 0.0f, nScrollBarStartY - 0.08203125f, nScrollBarStartX + 0.0859375f, nScrollBarStartY - 0.125f);
	Element.SetTexture(0, &rcTexture);
	Element.TextureColor.States[GLUF_STATE_DISABLED] = Color(200, 200, 200, 255);

	// Assign the Element
	SetDefaultElement(GLUF_CONTROL_SCROLLBAR, 0, &Element);

	//-------------------------------------
	// GLUFScrollBar - Up Arrow
	//-------------------------------------
	GLUFSetRect(rcTexture, nScrollBarStartX + 0.0f, nScrollBarStartY - 0.00390625f, nScrollBarStartX + 0.0859375f, nScrollBarStartY - 0.08203125f);
	Element.SetTexture(0, &rcTexture);
	Element.TextureColor.States[GLUF_STATE_DISABLED] = Color(200, 200, 200, 255);


	// Assign the Element
	SetDefaultElement(GLUF_CONTROL_SCROLLBAR, 1, &Element);

	//-------------------------------------
	// GLUFScrollBar - Down Arrow
	//-------------------------------------
	GLUFSetRect(rcTexture, nScrollBarStartX + 0.0f, nScrollBarStartY - 0.125f, nScrollBarStartX + 0.0859375f, nScrollBarStartY - 0.20703125f);
	Element.SetTexture(0, &rcTexture);
	Element.TextureColor.States[GLUF_STATE_DISABLED] = Color(200, 200, 200, 255);


	// Assign the Element
	SetDefaultElement(GLUF_CONTROL_SCROLLBAR, 2, &Element);

	//-------------------------------------
	// GLUFScrollBar - Button
	//-------------------------------------
	GLUFSetRect(rcTexture, 0.859375f, 0.75f, 0.9296875f, 0.9140625f);
	Element.SetTexture(0, &rcTexture);

	// Assign the Element
	SetDefaultElement(GLUF_CONTROL_SCROLLBAR, 3, &Element);

	//-------------------------------------
	// GLUFEditBox
	//-------------------------------------
	// Element assignment:
	//   0 - text area
	//   1 - top left border
	//   2 - top border
	//   3 - top right border
	//   4 - left border
	//   5 - right border
	//   6 - lower left border
	//   7 - lower border
	//   8 - lower right border

	/*Element.SetFont(0, Color(0, 0, 0, 255), GT_LEFT | GT_TOP);
	//TODO: this
	// Assign the style
	GLUFSetRect(rcTexture, 0.0546875f, 0.6484375f, 0.94140625f, 0.55859375f);
	Element.SetTexture(0, &rcTexture);
	SetDefaultElement(GLUF_CONTROL_EDITBOX, 0, &Element);

	GLUFSetRect(rcTexture, 0.03125f, 0.6796875f, 0.0546875f, 0.6484375f);
	Element.SetTexture(0, &rcTexture);
	SetDefaultElement(GLUF_CONTROL_EDITBOX, 1, &Element);

	GLUFSetRect(rcTexture, 0.0546875f, 0.6796875f, 0.94140625f, 0.6484375f);
	Element.SetTexture(0, &rcTexture);
	SetDefaultElement(GLUF_CONTROL_EDITBOX, 2, &Element);

	GLUFSetRect(rcTexture, 0.94140625f, 0.6796875f, 0.9609375f, 0.6484375f);
	Element.SetTexture(0, &rcTexture);
	SetDefaultElement(GLUF_CONTROL_EDITBOX, 3, &Element);

	GLUFSetRect(rcTexture, 0.03125f, 0.6484375f, 0.0546875f, 0.55859375f);
	Element.SetTexture(0, &rcTexture);
	SetDefaultElement(GLUF_CONTROL_EDITBOX, 4, &Element);

	GLUFSetRect(rcTexture, 0.94140625f, 0.6484375f, 0.9609375f, 0.55859375f);
	Element.SetTexture(0, &rcTexture);
	SetDefaultElement(GLUF_CONTROL_EDITBOX, 5, &Element);

	GLUFSetRect(rcTexture, 0.03125f, 0.55859375f, 0.0546875f, 0.52734375f);
	Element.SetTexture(0, &rcTexture);
	SetDefaultElement(GLUF_CONTROL_EDITBOX, 6, &Element);

	GLUFSetRect(rcTexture, 0.0546875f, 0.55859375f, 0.94140625f, 0.52734375f);
	Element.SetTexture(0, &rcTexture);
	SetDefaultElement(GLUF_CONTROL_EDITBOX, 7, &Element);

	GLUFSetRect(rcTexture, 0.94140625f, 0.55859375f, 0.9609375f, 0.52734375f);
	Element.SetTexture(0, &rcTexture);
	SetDefaultElement(GLUF_CONTROL_EDITBOX, 8, &Element);*/

	//-------------------------------------
	// GLUFListBox - Main
	//-------------------------------------
	GLUFSetRect(rcTexture, 0.05078125f, 0.51953125f, 0.94140625f, 0.375f);
	Element.SetTexture(0, &rcTexture);
	Element.SetFont(0, Color(0, 0, 0, 255), GT_LEFT | GT_TOP);

	// Assign the Element
	SetDefaultElement(GLUF_CONTROL_LISTBOX, 0, &Element);

	//-------------------------------------
	// GLUFListBox - Selection
	//-------------------------------------

	GLUFSetRect(rcTexture, 0.0625f, 0.3515625f, 0.9375f, 0.28515625f);
	Element.SetTexture(0, &rcTexture);
	Element.SetFont(0, Color(255, 255, 255, 255), GT_LEFT | GT_TOP);

	// Assign the Element
	SetDefaultElement(GLUF_CONTROL_LISTBOX, 1, &Element);
}



//======================================================================================
// GLUFDialogResourceManager
//======================================================================================

//--------------------------------------------------------------------------------------
GLUFDialogResourceManager::GLUFDialogResourceManager() :
//m_pVSRenderUI11(nullptr),
//m_pPSRenderUI11(nullptr),
//m_pPSRenderUIUntex11(nullptr),
//m_pDepthStencilStateUI11(nullptr),
//m_pRasterizerStateUI11(nullptr),
//m_pBlendStateUI11(nullptr),
//m_pSamplerStateUI11(nullptr),
//m_pDepthStencilStateStored11(nullptr),
//m_pRasterizerStateStored11(nullptr),
//m_pBlendStateStored11(nullptr),
//m_pSamplerStateStored11(nullptr),
//m_pInputLayout11(nullptr),
m_pVBScreenQuadVAO(0),
m_pVBScreenQuadIndicies(0),
m_pVBScreenQuadPositions(0),
m_pVBScreenQuadColor(0),
m_pVBScreenQuadUVs(0),
m_pSamplerLocation(1),//SAMPLER LOCATION HERE:
//m_pVBScreenQuad11(nullptr),
//m_pSpriteBuffer11(nullptr),
//m_SpriteBufferBytes11(0)
m_SpriteBufferVao(0),
m_SpriteBufferPos(0),
m_SpriteBufferColors(0),
m_SpriteBufferTexCoords(0),
m_SpriteBufferIndices(0)
{
	glGenVertexArrays(1, &m_pVBScreenQuadVAO);
	glBindVertexArray(m_pVBScreenQuadVAO);
	glGenBuffers(1, &m_pVBScreenQuadIndicies);
	glGenBuffers(1, &m_pVBScreenQuadPositions);
	glGenBuffers(1, &m_pVBScreenQuadColor);
	glGenBuffers(1, &m_pVBScreenQuadUVs);

	glBindBuffer(GL_ARRAY_BUFFER, m_pVBScreenQuadPositions);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, m_pVBScreenQuadColor);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, m_pVBScreenQuadUVs);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	//this is static
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_pVBScreenQuadIndicies);

	GLubyte indices[6] = {	2, 1, 0, 
							2, 3, 1};
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLubyte), indices, GL_STATIC_DRAW);


	glGenVertexArrays(1, &m_SpriteBufferVao);
	glBindVertexArray(m_SpriteBufferVao);

	glGenBuffers(1, &m_SpriteBufferPos);
	glGenBuffers(1, &m_SpriteBufferColors);
	glGenBuffers(1, &m_SpriteBufferTexCoords);
	glGenBuffers(1, &m_SpriteBufferIndices);

	glBindBuffer(GL_ARRAY_BUFFER, m_SpriteBufferPos);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, m_SpriteBufferColors);
	glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, m_SpriteBufferTexCoords);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

	//this is static
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_SpriteBufferIndices);

	GLubyte indicesS[6] = { 2, 1, 0,
							2, 3, 1 };
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLubyte), indicesS, GL_STATIC_DRAW);
}


//--------------------------------------------------------------------------------------
GLUFDialogResourceManager::~GLUFDialogResourceManager()
{
	for (auto it = m_FontCache.begin(); it != m_FontCache.end(); ++it)
	{
		GLUF_SAFE_DELETE(*it);
	}
	m_FontCache.clear();

	for (auto it = m_TextureCache.begin(); it != m_TextureCache.end(); ++it)
	{
		GLUF_SAFE_DELETE(*it);
	}
	m_TextureCache.clear();

	//TODO: make this with a class in the buffer sections
	glBindVertexArray(m_pVBScreenQuadVAO);
	glDeleteBuffers(1, &m_pVBScreenQuadPositions);
	glDeleteBuffers(1, &m_pVBScreenQuadColor);
	glDeleteBuffers(1, &m_pVBScreenQuadUVs);
	glDeleteVertexArrays(1, &m_pVBScreenQuadVAO);

	glBindVertexArray(m_SpriteBufferVao);
	glDeleteBuffers(1, &m_SpriteBufferPos);
	glDeleteBuffers(1, &m_SpriteBufferColors);
	glDeleteBuffers(1, &m_SpriteBufferTexCoords);
	glDeleteVertexArrays(1, &m_SpriteBufferVao);
	glBindVertexArray(0);
}


//--------------------------------------------------------------------------------------
bool GLUFDialogResourceManager::MsgProc(GLUF_MESSAGE_TYPE msg, int param1, int param2, int param3, int param4)
{
	GLUF_UNREFERENCED_PARAMETER(msg);
	GLUF_UNREFERENCED_PARAMETER(param1);
	GLUF_UNREFERENCED_PARAMETER(param2);
	GLUF_UNREFERENCED_PARAMETER(param3);
	GLUF_UNREFERENCED_PARAMETER(param4);

	switch (msg)
	{
	case GM_RESIZE:

		int w, h;
		glfwGetWindowSize(g_pGLFWWindow, &w, &h);
		m_WndSize.width = (float)w;
		m_WndSize.height = (float)h;
	}

	return false;
}

/*

GLUFResult GLUFDialogResourceManager::OnD3D11CreateDevice(ID3D11Device* pd3dDevice, ID3D11DeviceContext* pd3d11DeviceContext)
{
	m_pd3d11Device = pd3dDevice;
	m_pd3d11DeviceContext = pd3d11DeviceContext;

	GLUFResult hr = GR_SUCCESS;

	// Compile Shaders
	ID3DBlob* pVSBlob = nullptr;
	ID3DBlob* pPSBlob = nullptr;
	ID3DBlob* pPSUntexBlob = nullptr;
	V_RETURN(D3DCompile(g_strUIEffectFile, g_uUIEffectFileSize, "none", nullptr, nullptr, "VS", "vs_4_0_level_9_1",
		D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY, 0, &pVSBlob, nullptr));
	V_RETURN(D3DCompile(g_strUIEffectFile, g_uUIEffectFileSize, "none", nullptr, nullptr, "PS", "ps_4_0_level_9_1",
		D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY, 0, &pPSBlob, nullptr));
	V_RETURN(D3DCompile(g_strUIEffectFile, g_uUIEffectFileSize, "none", nullptr, nullptr, "PSUntex", "ps_4_0_level_9_1",
		D3D10_SHADER_ENABLE_BACKWARDS_COMPATIBILITY, 0, &pPSUntexBlob, nullptr));

	// Create Shaders
	V_RETURN(pd3dDevice->CreateVertexShader(pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), nullptr, &m_pVSRenderUI11));
	GLUF_SetDebugName(m_pVSRenderUI11, "GLUFDialogResourceManager");

	V_RETURN(pd3dDevice->CreatePixelShader(pPSBlob->GetBufferPointer(), pPSBlob->GetBufferSize(), nullptr, &m_pPSRenderUI11));
	GLUF_SetDebugName(m_pPSRenderUI11, "GLUFDialogResourceManager");

	V_RETURN(pd3dDevice->CreatePixelShader(pPSUntexBlob->GetBufferPointer(), pPSUntexBlob->GetBufferSize(), nullptr, &m_pPSRenderUIUntex11));
	GLUF_SetDebugName(m_pPSRenderUIUntex11, "GLUFDialogResourceManager");

	// States
	D3D11_DEPTH_STENCIL_DESC DSDesc;
	ZeroMemory(&DSDesc, sizeof(D3D11_DEPTH_STENCIL_DESC));
	DSDesc.DepthEnable = FALSE;
	DSDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	DSDesc.DepthFunc = D3D11_COMPARISON_LESS;
	DSDesc.StencilEnable = FALSE;
	V_RETURN(pd3dDevice->CreateDepthStencilState(&DSDesc, &m_pDepthStencilStateUI11));
	GLUF_SetDebugName(m_pDepthStencilStateUI11, "GLUFDialogResourceManager");

	D3D11_RASTERIZER_DESC RSDesc;
	RSDesc.AntialiasedLineEnable = FALSE;
	RSDesc.CullMode = D3D11_CULL_BACK;
	RSDesc.DepthBias = 0;
	RSDesc.DepthBiasClamp = 0.0f;
	RSDesc.DepthClipEnable = TRUE;
	RSDesc.FillMode = D3D11_FILL_SOLID;
	RSDesc.FrontCounterClockwise = FALSE;
	RSDesc.MultisampleEnable = TRUE;
	RSDesc.ScissorEnable = FALSE;
	RSDesc.SlopeScaledDepthBias = 0.0f;
	V_RETURN(pd3dDevice->CreateRasterizerState(&RSDesc, &m_pRasterizerStateUI11));
	GLUF_SetDebugName(m_pRasterizerStateUI11, "GLUFDialogResourceManager");

	D3D11_BLEND_DESC BSDesc;
	ZeroMemory(&BSDesc, sizeof(D3D11_BLEND_DESC));

	BSDesc.RenderTarget[0].BlendEnable = TRUE;
	BSDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	BSDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	BSDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	BSDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	BSDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	BSDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	BSDesc.RenderTarget[0].RenderTargetWriteMask = 0x0F;

	V_RETURN(pd3dDevice->CreateBlendState(&BSDesc, &m_pBlendStateUI11));
	GLUF_SetDebugName(m_pBlendStateUI11, "GLUFDialogResourceManager");

	D3D11_SAMPLER_DESC SSDesc;
	ZeroMemory(&SSDesc, sizeof(D3D11_SAMPLER_DESC));
	SSDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	SSDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	SSDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	SSDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	SSDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	SSDesc.MaxAnisotropy = 16;
	SSDesc.MinLOD = 0;
	SSDesc.MaxLOD = D3D11_FLOAT32_MAX;
	if (pd3dDevice->GetFeatureLevel() < D3D_FEATURE_LEVEL_9_3)
	{
		SSDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
		SSDesc.MaxAnisotropy = 0;
	}
	V_RETURN(pd3dDevice->CreateSamplerState(&SSDesc, &m_pSamplerStateUI11));
	GLUF_SetDebugName(m_pSamplerStateUI11, "GLUFDialogResourceManager");

	// Create the texture objects in the cache arrays.
	for (size_t i = 0; i < m_TextureCache.size(); i++)
	{
		hr = CreateTexture11(static_cast<UINT>(i));
		if (FAILED(hr))
			return hr;
	}

	// Create input layout
	const D3D11_INPUT_ELEMENT_DESC layout[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	V_RETURN(pd3dDevice->CreateInputLayout(layout, ARRAYSIZE(layout), pVSBlob->GetBufferPointer(), pVSBlob->GetBufferSize(), &m_pInputLayout11));
	GLUF_SetDebugName(m_pInputLayout11, "GLUFDialogResourceManager");

	// Release the blobs
	SAFE_RELEASE(pVSBlob);
	SAFE_RELEASE(pPSBlob);
	SAFE_RELEASE(pPSUntexBlob);

	// Create a vertex buffer quad for rendering later
	D3D11_BUFFER_DESC BufDesc;
	BufDesc.ByteWidth = sizeof(GLUF_SCREEN_VERTEX_10) * 4;
	BufDesc.Usage = D3D11_USAGE_DYNAMIC;
	BufDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	BufDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	BufDesc.MiscFlags = 0;
	V_RETURN(pd3dDevice->CreateBuffer(&BufDesc, nullptr, &m_pVBScreenQuad11));
	GLUF_SetDebugName(m_pVBScreenQuad11, "GLUFDialogResourceManager");

	// Init the D3D11 font
	InitFont11(pd3dDevice, m_pInputLayout11);

	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------

GLUFResult GLUFDialogResourceManager::OnD3D11ResizedSwapChain(ID3D11Device* pd3dDevice,
const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc)
{
	GLUF_UNREFERENCED_PARAMETER(pd3dDevice);

	GLUFResult hr = GR_SUCCESS;

	m_nBackBufferWidth = pBackBufferSurfaceDesc->Width;
	m_nBackBufferHeight = pBackBufferSurfaceDesc->Height;

	return hr;
}


//--------------------------------------------------------------------------------------
void GLUFDialogResourceManager::OnD3D11ReleasingSwapChain()
{
}


//--------------------------------------------------------------------------------------
void GLUFDialogResourceManager::OnD3D11DestroyDevice()
{
	// Release the resources but don't clear the cache, as these will need to be
	// recreated if the device is recreated

	for (auto it = m_TextureCache.begin(); it != m_TextureCache.end(); ++it)
	{
		SAFE_RELEASE((*it)->pTexResView11);
		SAFE_RELEASE((*it)->pTexture11);
	}

	// D3D11
	SAFE_RELEASE(m_pVBScreenQuad11);
	SAFE_RELEASE(m_pSpriteBuffer11);
	m_SpriteBufferBytes11 = 0;
	SAFE_RELEASE(m_pInputLayout11);

	// Shaders
	SAFE_RELEASE(m_pVSRenderUI11);
	SAFE_RELEASE(m_pPSRenderUI11);
	SAFE_RELEASE(m_pPSRenderUIUntex11);

	// States
	SAFE_RELEASE(m_pDepthStencilStateUI11);
	SAFE_RELEASE(m_pRasterizerStateUI11);
	SAFE_RELEASE(m_pBlendStateUI11);
	SAFE_RELEASE(m_pSamplerStateUI11);

	SAFE_RELEASE(m_pDepthStencilStateStored11);
	SAFE_RELEASE(m_pRasterizerStateStored11);
	SAFE_RELEASE(m_pBlendStateStored11);
	SAFE_RELEASE(m_pSamplerStateStored11);

	EndFont11();
}


//--------------------------------------------------------------------------------------
void GLUFDialogResourceManager::StoreD3D11State( ID3D11DeviceContext* pd3dImmediateContext)
{
	pd3dImmediateContext->OMGetDepthStencilState(&m_pDepthStencilStateStored11, &m_StencilRefStored11);
	pd3dImmediateContext->RSGetState(&m_pRasterizerStateStored11);
	pd3dImmediateContext->OMGetBlendState(&m_pBlendStateStored11, m_BlendFactorStored11, &m_SampleMaskStored11);
	pd3dImmediateContext->PSGetSamplers(0, 1, &m_pSamplerStateStored11);
}


//--------------------------------------------------------------------------------------
void GLUFDialogResourceManager::RestoreD3D11State( ID3D11DeviceContext* pd3dImmediateContext)
{
	pd3dImmediateContext->OMSetDepthStencilState(m_pDepthStencilStateStored11, m_StencilRefStored11);
	pd3dImmediateContext->RSSetState(m_pRasterizerStateStored11);
	pd3dImmediateContext->OMSetBlendState(m_pBlendStateStored11, m_BlendFactorStored11, m_SampleMaskStored11);
	pd3dImmediateContext->PSSetSamplers(0, 1, &m_pSamplerStateStored11);

	SAFE_RELEASE(m_pDepthStencilStateStored11);
	SAFE_RELEASE(m_pRasterizerStateStored11);
	SAFE_RELEASE(m_pBlendStateStored11);
	SAFE_RELEASE(m_pSamplerStateStored11);
}

*/
//--------------------------------------------------------------------------------------
void GLUFDialogResourceManager::ApplyRenderUI()
{
	// Shaders
	//pd3dImmediateContext->VSSetShader(m_pVSRenderUI11, nullptr, 0);
	//pd3dImmediateContext->HSSetShader(nullptr, nullptr, 0);
	//pd3dImmediateContext->DSSetShader(nullptr, nullptr, 0);
	//pd3dImmediateContext->GSSetShader(nullptr, nullptr, 0);
	//pd3dImmediateContext->PSSetShader(m_pPSRenderUI11, nullptr, 0);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	GLUFSHADERMANAGER.UseProgram(g_UIProgram);

	ApplyOrtho();

	// States ???
	//pd3dImmediateContext->OMSetDepthStencilState(m_pDepthStencilStateUI11, 0);
	//pd3dImmediateContext->RSSetState(m_pRasterizerStateUI11);
	//float BlendFactor[4] = { 0, 0, 0, 0 };
	//pd3dImmediateContext->OMSetBlendState(m_pBlendStateUI11, BlendFactor, 0xFFFFFFFF);
	//pd3dImmediateContext->PSSetSamplers(0, 1, &m_pSamplerStateUI11);
}


//--------------------------------------------------------------------------------------
void GLUFDialogResourceManager::ApplyRenderUIUntex()
{
	// Shaders
	//pd3dImmediateContext->VSSetShader(m_pVSRenderUI11, nullptr, 0);
	//pd3dImmediateContext->HSSetShader(nullptr, nullptr, 0);
	//pd3dImmediateContext->DSSetShader(nullptr, nullptr, 0);
	//pd3dImmediateContext->GSSetShader(nullptr, nullptr, 0);
	//pd3dImmediateContext->PSSetShader(m_pPSRenderUIUntex11, nullptr, 0);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glDisableVertexAttribArray(2);//just in case
	GLUFSHADERMANAGER.UseProgram(g_UIProgramUntex);
	
	ApplyOrtho();

	// States
	//pd3dImmediateContext->OMSetDepthStencilState(m_pDepthStencilStateUI11, 0);
	//pd3dImmediateContext->RSSetState(m_pRasterizerStateUI11);
	//float BlendFactor[4] = { 0, 0, 0, 0 };
	//pd3dImmediateContext->OMSetBlendState(m_pBlendStateUI11, BlendFactor, 0xFFFFFFFF);
	//pd3dImmediateContext->PSSetSamplers(0, 1, &m_pSamplerStateUI11);
}

GLUFPoint GLUFDialogResourceManager::GetOrthoPoint()
{
	GLUFPoint pt = GetWindowSize();
	if (pt.x >= pt.y)
	{
		pt.x = pt.x / pt.y;
		pt.y = 1.0f;
	}
	else
	{
		pt.y = pt.y / pt.x;
		pt.x = 1.0f;
	}

	return pt;
}

glm::mat4 GLUFDialogResourceManager::GetOrthoMatrix()
{
	GLUFPoint pt = GetOrthoPoint();
	return glm::ortho(-pt.x, pt.x, -pt.y, pt.y);
}

void GLUFDialogResourceManager::ApplyOrtho()
{
	glm::mat4 mat = GetOrthoMatrix();
	glUniformMatrix4fv(0, 1, GL_FALSE, &mat[0][0]);
}

//--------------------------------------------------------------------------------------
void GLUFDialogResourceManager::BeginSprites()
{
	m_SpriteVertices.clear();
}


//--------------------------------------------------------------------------------------

void GLUFDialogResourceManager::EndSprites()
{
	//this ensures we do not get a "vector subscript out of range"
	if (m_SpriteVertices.size() == 0)
		return;
	// ensure our buffer size can hold our sprites ???
	//GLuint SpriteDataBytes = static_cast<GLuint>(m_SpriteVertices.size() * sizeof(GLUFSpriteVertex));
	/*if (m_SpriteBufferBytes < SpriteDataBytes)
	{
		SAFE_RELEASE(m_pSpriteBuffer11);
		m_SpriteBufferBytes11 = SpriteDataBytes;

		D3D11_BUFFER_DESC BufferDesc;
		BufferDesc.ByteWidth = m_SpriteBufferBytes11;
		BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
		BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
		BufferDesc.MiscFlags = 0;

		if (FAILED(pd3dDevice->CreateBuffer(&BufferDesc, nullptr, &m_pSpriteBuffer11)))
		{
			m_pSpriteBuffer11 = nullptr;
			m_SpriteBufferBytes11 = 0;
			return;
		}
		GLUF_SetDebugName(m_pSpriteBuffer11, "GLUFDialogResourceManager");
	}*/

	// Copy the sprites over
	/*D3D11_BOX destRegion;
	destRegion.left = 0;
	destRegion.right = SpriteDataBytes;
	destRegion.top = 0;
	destRegion.bottom = 1;
	destRegion.front = 0;
	destRegion.back = 1;*/
	//D3D11_MAPPED_SUBRESOURCE MappedResource;
	//if (GR_SUCCESS == pd3dImmediateContext->Map(m_pSpriteBuffer11, 0, D3D11_MAP_WRITE_DISCARD, 0, &MappedResource))
	//{
	//	memcpy(MappedResource.pData, (const void*)&m_SpriteVertices[0], SpriteDataBytes);
	//	pd3dImmediateContext->Unmap(m_pSpriteBuffer11, 0);
	//}

	//buffer the data
	glBindVertexArray(m_SpriteBufferVao);
	glBindBuffer(GL_ARRAY_BUFFER, m_SpriteBufferPos);
	glBufferData(GL_ARRAY_BUFFER, m_SpriteVertices.size() * sizeof(glm::vec3), m_SpriteVertices.data_pos(), GL_STREAM_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, m_SpriteBufferColors);
	glBufferData(GL_ARRAY_BUFFER, m_SpriteVertices.size() * sizeof(Color4f), m_SpriteVertices.data_color(), GL_STREAM_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, m_SpriteBufferTexCoords);
	glBufferData(GL_ARRAY_BUFFER, m_SpriteVertices.size() * sizeof(glm::vec2), m_SpriteVertices.data_tex(), GL_STREAM_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_SpriteBufferIndices);

	// Draw
	//UINT Stride = sizeof(GLUFSpriteVertex);
	//UINT Offset = 0;
	//pd3dImmediateContext->IASetVertexBuffers(0, 1, &m_pSpriteBuffer11, &Stride, &Offset);
	//pd3dImmediateContext->IASetInputLayout(m_pInputLayout11);
	//pd3dImmediateContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	//pd3dImmediateContext->Draw(static_cast<UINT>(m_SpriteVertices.size()), 0);

	ApplyRenderUI();
	/*
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, GLUFBUFFERMANAGER.GetTextureBufferId(g_pControlTexturePtr));
	glUniform1f(m_pSamplerLocation, 0);*/

	
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_BYTE, nullptr);

	m_SpriteVertices.clear();
}


//--------------------------------------------------------------------------------------
bool GLUFDialogResourceManager::RegisterDialog(GLUFDialog* pDialog)
{
	// Check that the dialog isn't already registered.
	for (auto it = m_Dialogs.cbegin(); it != m_Dialogs.cend(); ++it)
	{
		if (*it == pDialog)
			return true;
	}

	// Add to the list.
	m_Dialogs.push_back(pDialog);

	// Set up next and prev pointers.
	if (m_Dialogs.size() > 1)
		m_Dialogs[m_Dialogs.size() - 2]->SetNextDialog(pDialog);
	m_Dialogs[m_Dialogs.size() - 1]->SetNextDialog(m_Dialogs[0]);

	return true;
}


//--------------------------------------------------------------------------------------
void GLUFDialogResourceManager::UnregisterDialog(GLUFDialog* pDialog)
{
	// Search for the dialog in the list.
	for (size_t i = 0; i < m_Dialogs.size(); ++i)
	{
		if (m_Dialogs[i] == pDialog)
		{
			m_Dialogs.erase(m_Dialogs.begin() + i);
			if (!m_Dialogs.empty())
			{
				int l, r;

				if (0 == i)
					l = int(m_Dialogs.size() - 1);
				else
					l = int(i) - 1;

				if (m_Dialogs.size() == i)
					r = 0;
				else
					r = int(i);

				m_Dialogs[l]->SetNextDialog(m_Dialogs[r]);
			}
			return;
		}
	}
}


//--------------------------------------------------------------------------------------
void GLUFDialogResourceManager::EnableKeyboardInputForAllDialogs()
{
	// Enable keyboard input for all registered dialogs
	for (auto it = m_Dialogs.begin(); it != m_Dialogs.end(); ++it)
		(*it)->EnableKeyboardInput(true);
}

//--------------------------------------------------------------------------------------
GLUFPoint GLUFDialogResourceManager::GetWindowSize()
{
	if (m_WndSize.x == 0.0f || m_WndSize.y == 0.0f)
	{
		int w, h;
		glfwGetWindowSize(g_pGLFWWindow, &w, &h);
		m_WndSize.width = (float)w;
		m_WndSize.height = (float)h;
	}
	return m_WndSize;
}

//--------------------------------------------------------------------------------------
int GLUFDialogResourceManager::AddFont(GLUFFontPtr font, GLUFFontSize height, GLUF_FONT_WEIGHT weight)
{
	// See if this font already exists (this is simple)
	for (size_t i = 0; i < m_FontCache.size(); ++i)
	{
		GLUFFontNode* node = m_FontCache[i];
		if (font == node->m_pFontType && node->mSize == height)
			return static_cast<int>(i);
	}

	// Add a new font and try to create it
	GLUFFontNode* pNewFontNode = new (std::nothrow) GLUFFontNode;
	if (!pNewFontNode)
		return -1;

	//wcscpy_s(pNewFontNode->strFace, MAX_PATH, strFaceName);
	pNewFontNode->m_pFontType = font;
	pNewFontNode->mSize = height;
	pNewFontNode->mWeight = weight;
	//FT_Set_Char_Size(pNewFontNode->m_pFontType->mFontFace, 0, height * 64, 72, 72);
	m_FontCache.push_back(pNewFontNode);

	int iFont = (int)m_FontCache.size() - 1;

	// If a device is available, try to create immediately
	return iFont;
}


//--------------------------------------------------------------------------------------
int GLUFDialogResourceManager::AddTexture(GLUFTexturePtr texture)
{
	// See if this texture already exists
	for (size_t i = 0; i < m_TextureCache.size(); ++i)
	{
		GLUFTextureNode* pTextureNode = m_TextureCache[i];
		if (GLUFBUFFERMANAGER.CompareTextures(texture, pTextureNode->m_pTextureElement))
		{
			return static_cast<int>(i);
		}
	}

	// Add a new texture and try to create it
	GLUFTextureNode* pNewTextureNode = new (std::nothrow) GLUFTextureNode;
	if (!pNewTextureNode)
		return -1;

	//ZeroMemory(pNewTextureNode, sizeof(GLUFTextureNode));
	//pNewTextureNode->bFileSource = true;
	//wcscpy_s(pNewTextureNode->strFilename, MAX_PATH, strFilename);
	pNewTextureNode->m_pTextureElement = texture;
	m_TextureCache.push_back(pNewTextureNode);

	int iTexture = int(m_TextureCache.size()) - 1;

	// If a device is available, try to create immediately

	return iTexture;
}

/*
//--------------------------------------------------------------------------------------
int GLUFDialogResourceManager::AddTexture(LPCWSTR strResourceName, HMODULE hResourceModule)
{
	// See if this texture already exists
	for (size_t i = 0; i < m_TextureCache.size(); i++)
	{
		GLUFTextureNode* pTextureNode = m_TextureCache[i];
		if (!pTextureNode->bFileSource &&      // Sources must match
			pTextureNode->hResourceModule == hResourceModule) // Module handles must match
		{
			if (IS_INTRESOURCE(strResourceName))
			{
				// Integer-based ID
				if ((INT_PTR)strResourceName == pTextureNode->nResourceID)
					return static_cast<int>(i);
			}
			else
			{
				// String-based ID
				size_t nLen = 0;
				nLen = wcsnlen(strResourceName, MAX_PATH);
				if (0 == _wcsnicmp(pTextureNode->strFilename, strResourceName, nLen))
					return static_cast<int>(i);
			}
		}
	}

	// Add a new texture and try to create it
	GLUFTextureNode* pNewTextureNode = new (std::nothrow) GLUFTextureNode;
	if (!pNewTextureNode)
		return -1;

	ZeroMemory(pNewTextureNode, sizeof(GLUFTextureNode));
	pNewTextureNode->hResourceModule = hResourceModule;
	if (IS_INTRESOURCE(strResourceName))
	{
		pNewTextureNode->nResourceID = (int)(size_t)strResourceName;
	}
	else
	{
		pNewTextureNode->nResourceID = 0;
		wcscpy_s(pNewTextureNode->strFilename, MAX_PATH, strResourceName);
	}

	m_TextureCache.push_back(pNewTextureNode);

	int iTexture = int(m_TextureCache.size()) - 1;

	// If a device is available, try to create immediately

	return iTexture;
}


//--------------------------------------------------------------------------------------
GLUFResult GLUFDialogResourceManager::CreateTexture11( UINT iTexture)
{
	GLUFResult hr = GR_SUCCESS;

	GLUFTextureNode* pTextureNode = m_TextureCache[iTexture];

	if (!pTextureNode->bFileSource)
	{
		if (pTextureNode->nResourceID == 0xFFFF && pTextureNode->hResourceModule == (HMODULE)0xFFFF)
		{
			hr = GLUFCreateGUITextureFromInternalArray(m_pd3d11Device, &pTextureNode->pTexture11);
			if (FAILED(hr))
				return DXTRACE_ERR(L"GLUFCreateGUITextureFromInternalArray", hr);
			GLUF_SetDebugName(pTextureNode->pTexture11, "GLUF GUI Texture");
		}
	}

	// Store dimensions
	D3D11_TEXTURE2D_DESC desc;
	pTextureNode->pTexture11->GetDesc(&desc);
	pTextureNode->dwWidth = desc.Width;
	pTextureNode->dwHeight = desc.Height;

	// Create resource view
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Format = desc.Format;
	SRVDesc.Texture2D.MipLevels = 1;
	SRVDesc.Texture2D.MostDetailedMip = 0;
	hr = m_pd3d11Device->CreateShaderResourceView(pTextureNode->pTexture11, &SRVDesc, &pTextureNode->pTexResView11);
	if (FAILED(hr))
		return hr;

	GLUF_SetDebugName(pTextureNode->pTexResView11, "GLUF GUI Texture");

	return hr;
}
*/


//======================================================================================
// GLUFControl class
//======================================================================================

GLUFControl::GLUFControl(GLUFDialog* pDialog)
{
	m_Type = GLUF_CONTROL_BUTTON;
	m_pDialog = pDialog;
	m_ID = 0;
	m_nHotkey = 0;
	m_Index = 0;
	m_pUserData = nullptr;

	m_bEnabled = true;
	m_bVisible = true;
	m_bMouseOver = false;
	m_bHasFocus = false;
	m_bIsDefault = false;

	m_pDialog = nullptr;

	m_x = 0.0f;
	m_y = 0.0f;
	m_width = 0.0f;
	m_height = 0.0f;

	//ZeroMemory(&m_rcBoundingBox, sizeof(m_rcBoundingBox));
}


//--------------------------------------------------------------------------------------
GLUFControl::~GLUFControl()
{
	for (auto it = m_Elements.begin(); it != m_Elements.end(); ++it)
	{
		GLUFElement* pElement = *it;
		delete pElement;
	}
	m_Elements.clear();
}


//--------------------------------------------------------------------------------------
void GLUFControl::SetTextColor(Color color)
{
	GLUFElement* pElement = m_Elements[0];

	if (pElement)
		pElement->FontColor.States[GLUF_STATE_NORMAL] = color;
}


//--------------------------------------------------------------------------------------
GLUFResult GLUFControl::SetElement(unsigned int iElement, GLUFElement* pElement)
{
	if (!pElement)
		return GR_INVALIDARG;

	// Make certain the array is this large
	for (size_t i = m_Elements.size(); i <= iElement; i++)
	{
		GLUFElement* pNewElement = new (std::nothrow) GLUFElement();
		if (!pNewElement)
			return GR_OUTOFMEMORY;

		m_Elements.push_back(pNewElement);
	}

	// Update the data
	GLUFElement* pCurElement = m_Elements[iElement];
	*pCurElement = *pElement;

	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------
void GLUFControl::Refresh()
{
	m_bMouseOver = false;
	m_bHasFocus = false;

	for (auto it = m_Elements.begin(); it != m_Elements.end(); ++it)
	{
		(*it)->Refresh();
	}
}


//--------------------------------------------------------------------------------------
void GLUFControl::UpdateRects()
{
	GLUFSetRect(m_rcBoundingBox, m_x, m_y + m_height, m_x + m_width, m_y);
}


//======================================================================================
// GLUFStatic class
//======================================================================================

//--------------------------------------------------------------------------------------
GLUFStatic::GLUFStatic(GLUFDialog* pDialog)
{
	m_Type = GLUF_CONTROL_STATIC;
	m_pDialog = pDialog;

	//ZeroMemory(&m_strText, sizeof(m_strText));

	for (auto it = m_Elements.begin(); it != m_Elements.end(); ++it)
	{
		GLUFElement* pElement = *it;
		GLUF_SAFE_DELETE(pElement);
	}

	m_Elements.clear();
}


//--------------------------------------------------------------------------------------
void GLUFStatic::Render(float fElapsedTime)
{
	if (m_bVisible == false)
		return;

	GLUF_CONTROL_STATE iState = GLUF_STATE_NORMAL;

	if (m_bEnabled == false)
		iState = GLUF_STATE_DISABLED;

	GLUFElement* pElement = m_Elements[0];

	pElement->FontColor.Blend(iState, fElapsedTime);

	m_pDialog->DrawText(m_strText, pElement, m_rcBoundingBox, false, false);
}


//---------------------------------------------------------------------------------------
GLUFResult GLUFStatic::GetTextCopy(std::string& strDest, unsigned int bufferCount)
{
	// Validate incoming parameters
	if (bufferCount == 0)
	{
		return GR_INVALIDARG;
	}

	// Copy the window text
	strDest = m_strText;

	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------
GLUFResult GLUFStatic::SetText(std::string strText)
{
	m_strText = strText;
	return GR_SUCCESS;
}


//======================================================================================
// GLUFButton class
//======================================================================================

GLUFButton::GLUFButton(GLUFDialog* pDialog)
{
	m_Type = GLUF_CONTROL_BUTTON;
	m_pDialog = pDialog;

	m_bPressed = false;
	m_nHotkey = 0;
}

//--------------------------------------------------------------------------------------
bool GLUFButton::MsgProc(GLUF_MESSAGE_TYPE msg, int param1, int param2, int param3, int param4)
{
	if (!m_bEnabled || !m_bVisible)
		return false;

	GLUFPoint mousePos = m_pDialog->m_MousePositionDialogSpace;

	switch (msg)
	{

		case GM_CURSOR_POS:

			if (m_bPressed)
			{
				//if the button is pressed and the mouse is moved off, then unpress it
				if (!ContainsPoint(mousePos))
				{
					m_bPressed = false;

					ContainsPoint(mousePos);

					if (!m_pDialog->m_bKeyboardInput)
						m_pDialog->ClearFocus();
				}
			}

			break;
		case GM_MB:
		{
			if (param2 == GLFW_PRESS)
			{
				if (ContainsPoint(mousePos))
				{
					// Pressed while inside the control
					m_bPressed = true;
					//SetCapture(GLUFGetHWND());

					if (!m_bHasFocus)
						m_pDialog->RequestFocus(this);

					return true;

				}
			}
			else if (param2 == GLFW_RELEASE)
			{
				if (m_bPressed)
				{
					m_bPressed = false;
					//ReleaseCapture();

					if (!m_pDialog->m_bKeyboardInput)
						m_pDialog->ClearFocus();

					// Button click
					if (ContainsPoint(mousePos))
						m_pDialog->SendEvent(GLUF_EVENT_BUTTON_CLICKED, true, this);

					return true;
				}

			}
		}
		break;

		case GM_KEY:
		{

			if (param1 = GLFW_KEY_SPACE)
			{
				if (param3 == GLFW_PRESS)
				{
					m_bPressed = true;
				}
				if (param3 == GLFW_RELEASE)
				{
					m_bPressed = false;

					m_pDialog->SendEvent(GLUF_EVENT_BUTTON_CLICKED, true, this);
				}

				return true;
			}


				return true;

			break;
		}
	};

	return false;
}

//--------------------------------------------------------------------------------------
void GLUFButton::Render(float fElapsedTime)
{
	if (m_bVisible == false)
		return;

	float nOffsetX = 0.0f;
	float nOffsetY = 0.0f;

	GLUFPoint wndSize = m_pDialog->GetManager()->GetWindowSize();

	GLUF_CONTROL_STATE iState = GLUF_STATE_NORMAL;

	if (m_bVisible == false)
	{
		iState = GLUF_STATE_HIDDEN;
	}
	else if (m_bEnabled == false)
	{
		iState = GLUF_STATE_DISABLED;
	}
	else if (m_bPressed)
	{
		iState = GLUF_STATE_PRESSED;

		nOffsetX = 1.0f / wndSize.x;
		nOffsetY = 2.0f / wndSize.y;
	}
	else if (m_bMouseOver)
	{
		iState = GLUF_STATE_MOUSEOVER;

		nOffsetX = -1.0f / wndSize.x;
		nOffsetY = -2.0f / wndSize.y;
	}
	else if (m_bHasFocus)
	{
		iState = GLUF_STATE_FOCUS;
	}

	float fBlendRate = (iState == GLUF_STATE_PRESSED) ? 0.0f : 0.8f;

	GLUFRect rcWindow = m_rcBoundingBox;
	GLUFOffsetRect(rcWindow, nOffsetX, nOffsetY);


	// Background fill layer
	GLUFElement* pElement = m_Elements[0];

	// Blend current color
	pElement->TextureColor.Blend(iState, fElapsedTime, fBlendRate);
	pElement->FontColor.Blend(iState, fElapsedTime, fBlendRate);

	m_pDialog->DrawSprite(pElement, rcWindow, GLUF_FAR_BUTTON_DEPTH);
	//m_pDialog->DrawText(m_strText, pElement, rcWindow, false, true);

	// Main button
	pElement = m_Elements[1];

	// Blend current color
	pElement->TextureColor.Blend(iState, fElapsedTime, fBlendRate);
	pElement->FontColor.Blend(iState, fElapsedTime, fBlendRate);

	m_pDialog->DrawSprite(pElement, rcWindow, GLUF_NEAR_BUTTON_DEPTH);
	m_pDialog->DrawText(m_strText, pElement, rcWindow, false, true);
}



//======================================================================================
// GLUFCheckBox class
//======================================================================================

GLUFCheckBox::GLUFCheckBox(GLUFDialog* pDialog)
{
	m_Type = GLUF_CONTROL_CHECKBOX;
	m_pDialog = pDialog;

	m_bChecked = false;
}

//--------------------------------------------------------------------------------------

bool GLUFCheckBox::MsgProc(GLUF_MESSAGE_TYPE msg, int param1, int param2, int param3, int param4)
{
	if (!m_bEnabled || !m_bVisible)
		return false;

	GLUFPoint mousePos = m_pDialog->m_MousePositionDialogSpace;

	switch (msg)
	{

	case GM_CURSOR_POS:

		if (m_bPressed)
		{
			//if the button is pressed and the mouse is moved off, then unpress it
			if (!ContainsPoint(mousePos))
			{
				m_bPressed = false;

				ContainsPoint(mousePos);

				if (!m_pDialog->m_bKeyboardInput)
					m_pDialog->ClearFocus();
			}
		}

		break;
	case GM_MB:
	{
		if (param2 == GLFW_PRESS)
		{
			if (ContainsPoint(mousePos))
			{
				// Pressed while inside the control
				m_bPressed = true;
				//SetCapture(GLUFGetHWND());

				if (!m_bHasFocus)
					m_pDialog->RequestFocus(this);

				return true;

			}
		}
		else if (param2 == GLFW_RELEASE)
		{
			if (m_bPressed)
			{
				m_bPressed = false;
				//ReleaseCapture();

				if (!m_pDialog->m_bKeyboardInput)
					m_pDialog->ClearFocus();

				// Button click
				if (ContainsPoint(mousePos))
					SetCheckedInternal(!m_bChecked, true);

				return true;
			}

		}
	}
		break;

	case GM_KEY:
	{

		if (param1 = GLFW_KEY_SPACE)
		{
			if (param3 == GLFW_PRESS)
			{
				m_bPressed = true;
			}
			if (param3 == GLFW_RELEASE)
			{
				m_bPressed = false;

				SetCheckedInternal(!m_bChecked, true);
			}

			return true;
		}


		return true;

		break;
	}
	};

	return false;
}


//--------------------------------------------------------------------------------------

void GLUFCheckBox::SetCheckedInternal(bool bChecked, bool bFromInput)
{
	m_bChecked = bChecked;

	m_pDialog->SendEvent(GLUF_EVENT_CHECKBOXCHANGED, bFromInput, this);
}


//--------------------------------------------------------------------------------------
bool GLUFCheckBox::ContainsPoint(GLUFPoint pt)
{
	return (GLUFPtInRect(m_rcBoundingBox, pt) ||
		GLUFPtInRect(m_rcButton, pt));
}


//--------------------------------------------------------------------------------------
void GLUFCheckBox::UpdateRects()
{
	GLUFButton::UpdateRects();

	m_rcButton = m_rcBoundingBox;
	m_rcButton.right = m_rcButton.left + GLUFRectHeight(m_rcButton);

	m_rcText = m_rcBoundingBox;
	GLUFOffsetRect(m_rcText, GLUFRectWidth(m_rcButton)*1.25, 0.0f);
}


//--------------------------------------------------------------------------------------
void GLUFCheckBox::Render( float fElapsedTime)
{
	if (m_bVisible == false)
		return;
	GLUF_CONTROL_STATE iState = GLUF_STATE_NORMAL;

	if (m_bVisible == false)
		iState = GLUF_STATE_HIDDEN;
	else if (m_bEnabled == false)
		iState = GLUF_STATE_DISABLED;
	else if (m_bPressed)
		iState = GLUF_STATE_PRESSED;
	else if (m_bMouseOver)
		iState = GLUF_STATE_MOUSEOVER;
	else if (m_bHasFocus)
		iState = GLUF_STATE_FOCUS;

	GLUFElement* pElement = m_Elements[0];

	float fBlendRate = (iState == GLUF_STATE_PRESSED) ? 0.0f : 0.8f;

	pElement->TextureColor.Blend(iState, fElapsedTime, fBlendRate);
	pElement->FontColor.Blend(iState, fElapsedTime, fBlendRate);

	m_pDialog->DrawSprite(pElement, m_rcButton, GLUF_FAR_BUTTON_DEPTH);
	m_pDialog->DrawText(m_strText, pElement, m_rcText, false, false);

	if (m_bChecked)//TODO: how do i blend properly?
		iState = GLUF_STATE_HIDDEN;

	pElement = m_Elements[1];

	pElement->TextureColor.Blend(iState, fElapsedTime, fBlendRate);
	m_pDialog->DrawSprite(pElement, m_rcButton, GLUF_NEAR_BUTTON_DEPTH);
}


//======================================================================================
// GLUFRadioButton class
//======================================================================================

GLUFRadioButton::GLUFRadioButton( GLUFDialog* pDialog)
{
	m_Type = GLUF_CONTROL_RADIOBUTTON;
	m_pDialog = pDialog;
}

//--------------------------------------------------------------------------------------
bool GLUFRadioButton::MsgProc(GLUF_MESSAGE_TYPE msg, int param1, int param2, int param3, int param4)
{
	if (!m_bEnabled || !m_bVisible)
		return false;

	GLUFPoint mousePos = m_pDialog->m_MousePositionDialogSpace;

	switch (msg)
	{

	case GM_CURSOR_POS:

		if (m_bPressed)
		{
			//if the button is pressed and the mouse is moved off, then unpress it
			if (!ContainsPoint(mousePos))
			{
				m_bPressed = false;

				ContainsPoint(mousePos);

				if (!m_pDialog->m_bKeyboardInput)
					m_pDialog->ClearFocus();
			}
		}

		break;
	case GM_MB:
	{
		if (param2 == GLFW_PRESS)
		{
			if (ContainsPoint(mousePos))
			{
				// Pressed while inside the control
				m_bPressed = true;
				//SetCapture(GLUFGetHWND());

				if (!m_bHasFocus)
					m_pDialog->RequestFocus(this);

				return true;

			}
		}
		else if (param2 == GLFW_RELEASE)
		{
			if (m_bPressed || !m_bChecked)
			{
				if (ContainsPoint(mousePos))
				{
					m_bPressed = false;
					//ReleaseCapture();

					SetCheckedInternal(true, true, true);
					return true;
				}
			}

		}
	}
		break;

	case GM_KEY:
	{

		if (param1 = GLFW_KEY_SPACE)
		{
			if (param3 == GLFW_PRESS)
			{
				m_bPressed = true;
			}
			else if (param3 == GLFW_RELEASE)
			{
				if (m_bChecked)
					return false;

				m_bPressed = false;

				SetCheckedInternal(true, true, true);
			}
		}


		return true;

		break;
	}
	};

	return false;
}

//--------------------------------------------------------------------------------------

void GLUFRadioButton::SetCheckedInternal(bool bChecked, bool bClearGroup, bool bFromInput)
{
	if (bChecked && bClearGroup)
		m_pDialog->ClearRadioButtonGroup(m_nButtonGroup);

	m_bChecked = bChecked;
	m_pDialog->SendEvent(GLUF_EVENT_RADIOBUTTON_CHANGED, bFromInput, this);
}


//======================================================================================
// GLUFComboBox class
//======================================================================================

GLUFComboBox::GLUFComboBox( GLUFDialog* pDialog) : m_ScrollBar(pDialog)
{
	m_Type = GLUF_CONTROL_COMBOBOX;
	m_pDialog = pDialog;

	m_fDropHeight = 100.0f / m_pDialog->GetManager()->GetWindowSize().y;

	m_fSBWidth = 16.0f / m_pDialog->GetManager()->GetWindowSize().x;
	m_bOpened = false;
	m_iSelected = -1;
	m_iFocused = -1;
}


//--------------------------------------------------------------------------------------
GLUFComboBox::~GLUFComboBox()
{
	RemoveAllItems();
}


//--------------------------------------------------------------------------------------
void GLUFComboBox::SetTextColor(Color Color)
{
	GLUFElement* pElement = m_Elements[0];

	if (pElement)
		pElement->FontColor.States[GLUF_STATE_NORMAL] = Color;

	pElement = m_Elements[2];

	if (pElement)
		pElement->FontColor.States[GLUF_STATE_NORMAL] = Color;
}


//--------------------------------------------------------------------------------------
void GLUFComboBox::UpdateRects()
{

	GLUFButton::UpdateRects();

	m_rcButton = m_rcBoundingBox;
	m_rcButton.left = m_rcButton.right - GLUFRectHeight(m_rcButton);

	m_rcText = m_rcBoundingBox;
	m_rcText.right = m_rcButton.left;

	m_rcDropdown = m_rcText;
	GLUFOffsetRect(m_rcDropdown, 0, (0.90f * GLUFRectHeight(m_rcText)));
	m_rcDropdown.bottom += m_fDropHeight;
	m_rcDropdown.right -= m_fSBWidth;

	m_rcDropdownText = m_rcDropdown;
	m_rcDropdownText.left += (0.1f * GLUFRectWidth(m_rcDropdown));
	m_rcDropdownText.right -= (0.1f * GLUFRectWidth(m_rcDropdown));
	m_rcDropdownText.top += (0.1f * GLUFRectHeight(m_rcDropdown));
	m_rcDropdownText.bottom -= (0.1f * GLUFRectHeight(m_rcDropdown));

	// Update the scrollbar's rects
	m_ScrollBar.SetLocation(m_rcDropdown.right, m_rcDropdown.top + 2);
	m_ScrollBar.SetSize(m_fSBWidth, GLUFRectHeight(m_rcDropdown) - 2);
	GLUFFontNode* pFontNode = m_pDialog->GetManager()->GetFontNode(m_Elements[2]->iFont);
	if (pFontNode && pFontNode->mSize)
	{
		m_ScrollBar.SetPageSize(int(GLUFRectHeight(m_rcDropdownText) / pFontNode->mSize));

		// The selected item may have been scrolled off the page.
		// Ensure that it is in page again.
		m_ScrollBar.ShowItem(m_iSelected);
	}
}


//--------------------------------------------------------------------------------------
void GLUFComboBox::OnFocusOut()
{
	GLUFButton::OnFocusOut();

	m_bOpened = false;
}


//--------------------------------------------------------------------------------------
/*
bool GLUFComboBox::HandleKeyboard(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	const DWORD REPEAT_MASK = (0x40000000);

	if (!m_bEnabled || !m_bVisible)
		return false;

	// Let the scroll bar have a chance to handle it first
	if (m_ScrollBar.HandleKeyboard(uMsg, wParam, lParam))
		return true;

	switch (uMsg)
	{
	case WM_KEYDOWN:
	{
		switch (wParam)
		{
		case VK_RETURN:
			if (m_bOpened)
			{
				if (m_iSelected != m_iFocused)
				{
					m_iSelected = m_iFocused;
					m_pDialog->SendEvent(GLUF_EVENTCOMBOBOX_SELECTION_CHANGED, true, this);
				}
				m_bOpened = false;

				if (!m_pDialog->m_bKeyboardInput)
					m_pDialog->ClearFocus();

				return true;
			}
			break;

		case VK_F4:
			// Filter out auto-repeats
			if (lParam & REPEAT_MASK)
				return true;

			m_bOpened = !m_bOpened;

			if (!m_bOpened)
			{
				m_pDialog->SendEvent(GLUF_EVENTCOMBOBOX_SELECTION_CHANGED, true, this);

				if (!m_pDialog->m_bKeyboardInput)
					m_pDialog->ClearFocus();
			}

			return true;

		case VK_LEFT:
		case VK_UP:
			if (m_iFocused > 0)
			{
				m_iFocused--;
				m_iSelected = m_iFocused;

				if (!m_bOpened)
					m_pDialog->SendEvent(GLUF_EVENTCOMBOBOX_SELECTION_CHANGED, true, this);
			}

			return true;

		case VK_RIGHT:
		case VK_DOWN:
			if (m_iFocused + 1 < (int)GetNumItems())
			{
				m_iFocused++;
				m_iSelected = m_iFocused;

				if (!m_bOpened)
					m_pDialog->SendEvent(GLUF_EVENTCOMBOBOX_SELECTION_CHANGED, true, this);
			}

			return true;
		}
		break;
	}
	}

	return false;
}


//--------------------------------------------------------------------------------------

bool GLUFComboBox::HandleMouse(UINT uMsg, const POINT& pt, WPARAM wParam, LPARAM lParam)
{
	if (!m_bEnabled || !m_bVisible)
		return false;

	// Let the scroll bar handle it first.
	if (m_ScrollBar.HandleMouse(uMsg, pt, wParam, lParam))
		return true;

	switch (uMsg)
	{
	case WM_MOUSEMOVE:
	{
		if (m_bOpened && PtInRect(&m_rcDropdown, pt))
		{
			// Determine which item has been selected
			for (size_t i = 0; i < m_Items.size(); i++)
			{
				GLUFComboBoxItem* pItem = m_Items[i];
				if (pItem->bVisible &&
					PtInRect(&pItem->rcActive, pt))
				{
					m_iFocused = static_cast<int>(i);
				}
			}
			return true;
		}
		break;
	}

	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	{
		if (ContainsPoint(pt))
		{
			// Pressed while inside the control
			m_bPressed = true;
			SetCapture(GLUFGetHWND());

			if (!m_bHasFocus)
				m_pDialog->RequestFocus(this);

			// Toggle dropdown
			if (m_bHasFocus)
			{
				m_bOpened = !m_bOpened;

				if (!m_bOpened)
				{
					if (!m_pDialog->m_bKeyboardInput)
						m_pDialog->ClearFocus();
				}
			}

			return true;
		}

		// Perhaps this click is within the dropdown
		if (m_bOpened && PtInRect(&m_rcDropdown, pt))
		{
			// Determine which item has been selected
			for (size_t i = m_ScrollBar.GetTrackPos(); i < m_Items.size(); i++)
			{
				GLUFComboBoxItem* pItem = m_Items[i];
				if (pItem->bVisible &&
					PtInRect(&pItem->rcActive, pt))
				{
					m_iFocused = m_iSelected = static_cast<int>(i);
					m_pDialog->SendEvent(GLUF_EVENTCOMBOBOX_SELECTION_CHANGED, true, this);
					m_bOpened = false;

					if (!m_pDialog->m_bKeyboardInput)
						m_pDialog->ClearFocus();

					break;
				}
			}

			return true;
		}

		// Mouse click not on main control or in dropdown, fire an event if needed
		if (m_bOpened)
		{
			m_iFocused = m_iSelected;

			m_pDialog->SendEvent(GLUF_EVENTCOMBOBOX_SELECTION_CHANGED, true, this);
			m_bOpened = false;
		}

		// Make sure the control is no longer in a pressed state
		m_bPressed = false;

		// Release focus if appropriate
		if (!m_pDialog->m_bKeyboardInput)
		{
			m_pDialog->ClearFocus();
		}

		break;
	}

	case WM_LBUTTONUP:
	{
		if (m_bPressed && ContainsPoint(pt))
		{
			// Button click
			m_bPressed = false;
			ReleaseCapture();
			return true;
		}

		break;
	}

	case WM_MOUSEWHEEL:
	{
		int zDelta = (short)HIWORD(wParam) / WHEEL_DELTA;
		if (m_bOpened)
		{
			UINT uLines = 0;
			if (!SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &uLines, 0))
				uLines = 0;
			m_ScrollBar.Scroll(-zDelta * uLines);
		}
		else
		{
			if (zDelta > 0)
			{
				if (m_iFocused > 0)
				{
					m_iFocused--;
					m_iSelected = m_iFocused;

					if (!m_bOpened)
						m_pDialog->SendEvent(GLUF_EVENTCOMBOBOX_SELECTION_CHANGED, true, this);
				}
			}
			else
			{
				if (m_iFocused + 1 < (int)GetNumItems())
				{
					m_iFocused++;
					m_iSelected = m_iFocused;

					if (!m_bOpened)
						m_pDialog->SendEvent(GLUF_EVENTCOMBOBOX_SELECTION_CHANGED, true, this);
				}
			}
		}
		return true;
	}
	};

	return false;
}
*/

//--------------------------------------------------------------------------------------
bool GLUFComboBox::MsgProc(GLUF_MESSAGE_TYPE msg, int param1, int param2, int param3, int param4)
{
	//TODO:
	return false;
}

//--------------------------------------------------------------------------------------
void GLUFComboBox::OnHotkey()
{
	if (m_bOpened)
		return;

	if (m_iSelected == -1)
		return;

	if (m_pDialog->IsKeyboardInputEnabled())
		m_pDialog->RequestFocus(this);

	m_iSelected++;

	if (m_iSelected >= (int)m_Items.size())
		m_iSelected = 0;

	m_iFocused = m_iSelected;
	m_pDialog->SendEvent(GLUF_EVENT_COMBOBOX_SELECTION_CHANGED, true, this);
}


//--------------------------------------------------------------------------------------
void GLUFComboBox::Render( float fElapsedTime)
{
	if (m_bVisible == false)
		return;
	GLUF_CONTROL_STATE iState = GLUF_STATE_NORMAL;

	if (!m_bOpened)
		iState = GLUF_STATE_HIDDEN;

	// Dropdown box
	GLUFElement* pElement = m_Elements[2];

	// If we have not initialized the scroll bar page size,
	// do that now.
	static bool bSBInit;
	if (!bSBInit)
	{
		// Update the page size of the scroll bar
		if (m_pDialog->GetManager()->GetFontNode(pElement->iFont)->mSize)
			m_ScrollBar.SetPageSize(int(GLUFRectHeight(m_rcDropdownText) /
			(m_pDialog->GetManager()->GetFontNode(pElement->iFont)->mSize / m_pDialog->GetManager()->GetWindowSize().y)));
		else
			m_ScrollBar.SetPageSize(int(GLUFRectHeight(m_rcDropdownText)));
		bSBInit = true;
	}

	// Scroll bar --EDITED, only render any of this stuff if OPENED
	if (m_bOpened)
	{
		m_ScrollBar.Render(fElapsedTime);

		// Blend current color
		pElement->TextureColor.Blend(iState, fElapsedTime);
		pElement->FontColor.Blend(iState, fElapsedTime);

		m_pDialog->DrawSprite(pElement, m_rcDropdown, GLUF_NEAR_BUTTON_DEPTH);


		// Selection outline
		GLUFElement* pSelectionElement = m_Elements[3];
		pSelectionElement->TextureColor.Current = pElement->TextureColor.Current;
		pSelectionElement->FontColor.SetCurrent(pSelectionElement->FontColor.States[GLUF_STATE_NORMAL]);

		GLUFFontNode* pFont = m_pDialog->GetFont(pElement->iFont);
		if (pFont)
		{
			float curY = m_rcDropdownText.top;
			float nRemainingHeight = GLUFRectHeight(m_rcDropdownText);
			//WCHAR strDropdown[4096] = {0};

			for (size_t i = m_ScrollBar.GetTrackPos(); i < m_Items.size(); i++)
			{
				GLUFComboBoxItem* pItem = m_Items[i];

				// Make sure there's room left in the dropdown
				nRemainingHeight -= pFont->mSize / m_pDialog->GetManager()->GetWindowSize().y;
				if (nRemainingHeight < 0)
				{
					pItem->bVisible = false;
					continue;
				}

				GLUFSetRect(pItem->rcActive, m_rcDropdownText.left, curY + pFont->mSize, m_rcDropdownText.right, curY);
				curY -= pFont->mSize / m_pDialog->GetManager()->GetWindowSize().y;

				//debug
				//int blue = 50 * i;
				//m_pDialog->DrawRect( &pItem->rcActive, 0xFFFF0000 | blue );

				pItem->bVisible = true;

				if (m_bOpened)
				{
					if ((int)i == m_iFocused)
					{
						GLUFRect rc;
						GLUFSetRect(rc, m_rcDropdown.left, pItem->rcActive.top + (2 / m_pDialog->GetManager()->GetWindowSize().y), m_rcDropdown.right,
							pItem->rcActive.bottom - (2 / m_pDialog->GetManager()->GetWindowSize().y));
						m_pDialog->DrawSprite(pSelectionElement, rc, GLUF_NEAR_BUTTON_DEPTH);
						m_pDialog->DrawText(pItem->strText, pSelectionElement, pItem->rcActive);
					}
					else
					{
						m_pDialog->DrawText(pItem->strText, pElement, pItem->rcActive);
					}
				}
			}
		}
	}

	float fOffsetX = 0.0f;
	float fOffsetY = 0.0f;

	iState = GLUF_STATE_NORMAL;

	if (m_bVisible == false)
		iState = GLUF_STATE_HIDDEN;
	else if (m_bEnabled == false)
		iState = GLUF_STATE_DISABLED;
	else if (m_bPressed)
	{
		iState = GLUF_STATE_PRESSED;

		fOffsetX = 1.0f / m_pDialog->GetManager()->GetWindowSize().x;
		fOffsetY = 2.0f / m_pDialog->GetManager()->GetWindowSize().y;
	}
	else if (m_bMouseOver)
	{
		iState = GLUF_STATE_MOUSEOVER;

		fOffsetX = -1.0f / m_pDialog->GetManager()->GetWindowSize().x;
		fOffsetY = -2.0f / m_pDialog->GetManager()->GetWindowSize().y;
	}
	else if (m_bHasFocus)
		iState = GLUF_STATE_FOCUS;

	float fBlendRate = (iState == GLUF_STATE_PRESSED) ? 0.0f : 0.8f;

	// Button
	pElement = m_Elements[1];

	// Blend current color
	pElement->TextureColor.Blend(iState, fElapsedTime, fBlendRate);

	GLUFRect rcWindow = m_rcButton;
	GLUFOffsetRect(rcWindow, fOffsetX, fOffsetY);
	m_pDialog->DrawSprite(pElement, rcWindow, GLUF_FAR_BUTTON_DEPTH);

	if (m_bOpened)
		iState = GLUF_STATE_PRESSED;

	// Main text box
	pElement = m_Elements[0];

	// Blend current color
	pElement->TextureColor.Blend(iState, fElapsedTime, fBlendRate);
	pElement->FontColor.Blend(iState, fElapsedTime, fBlendRate);

	m_pDialog->DrawSprite(pElement, m_rcText, GLUF_NEAR_BUTTON_DEPTH);

	if (m_iSelected >= 0 && m_iSelected < (int)m_Items.size())
	{
		GLUFComboBoxItem* pItem = m_Items[m_iSelected];
		if (pItem)
		{
			m_pDialog->DrawText(pItem->strText, pElement, m_rcText, false, true);

		}
	}
}


//--------------------------------------------------------------------------------------

GLUFResult GLUFComboBox::AddItem(std::string strText, void* pData)
{
	// Validate parameters
	/*if (!strText)
	{
		return E_INVALIDARG;
	}*/

	// Create a new item and set the data
	GLUFComboBoxItem* pItem = new (std::nothrow) GLUFComboBoxItem;
	if (!pItem)
	{
		return GLUFTRACE_ERR("new", GR_OUTOFMEMORY);
	}

	//ZeroMemory(pItem, sizeof(GLUFComboBoxItem));
	//wcscpy_s(pItem->strText, 256, strText);
	pItem->strText = strText;
	pItem->pData = pData;

	m_Items.push_back(pItem);

	// Update the scroll bar with new range
	m_ScrollBar.SetTrackRange(0, (int)m_Items.size());

	// If this is the only item in the list, it's selected
	if (GetNumItems() == 1)
	{
		m_iSelected = 0;
		m_iFocused = 0;
		m_pDialog->SendEvent(GLUF_EVENT_COMBOBOX_SELECTION_CHANGED, false, this);
	}

	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------
void GLUFComboBox::RemoveItem( unsigned int index)
{
	auto it = m_Items.begin() + index;
	GLUFComboBoxItem* pItem = *it;
	GLUF_SAFE_DELETE(pItem);
	m_Items.erase(it);
	m_ScrollBar.SetTrackRange(0, (int)m_Items.size());
	if (m_iSelected >= (int)m_Items.size())
		m_iSelected = (int)m_Items.size() - 1;
}


//--------------------------------------------------------------------------------------
void GLUFComboBox::RemoveAllItems()
{
	for (auto it = m_Items.begin(); it != m_Items.end(); ++it)
	{
		GLUFComboBoxItem* pItem = *it;
		GLUF_SAFE_DELETE(pItem);
	}

	m_Items.clear();
	m_ScrollBar.SetTrackRange(0, 1);
	m_iFocused = m_iSelected = -1;
}


//--------------------------------------------------------------------------------------
bool GLUFComboBox::ContainsItem(std::string strText, unsigned int iStart)
{
	return (-1 != FindItem(strText, iStart));
}


//--------------------------------------------------------------------------------------
int GLUFComboBox::FindItem(std::string strText, unsigned int iStart)
{
	/*if (!strText)
		return -1;*/

	for (size_t i = iStart; i < m_Items.size(); i++)
	{
		GLUFComboBoxItem* pItem = m_Items[i];

		if (0 != pItem->strText.compare(strText))//REMEMBER if this returns 0, they are the same
		{
			return static_cast<int>(i);
		}
	}

	return -1;
}


//--------------------------------------------------------------------------------------
void* GLUFComboBox::GetSelectedData()
{
	if (m_iSelected < 0)
		return nullptr;

	GLUFComboBoxItem* pItem = m_Items[m_iSelected];
	return pItem->pData;
}


//--------------------------------------------------------------------------------------
GLUFComboBoxItem* GLUFComboBox::GetSelectedItem()
{
	if (m_iSelected < 0)
		return nullptr;

	return m_Items[m_iSelected];
}


//--------------------------------------------------------------------------------------
void* GLUFComboBox::GetItemData(std::string strText) 
{
	int index = FindItem(strText);
	if (index == -1)
	{
		return nullptr;
	}

	GLUFComboBoxItem* pItem = m_Items[index];
	if (!pItem)
	{
		GLUFTRACE_ERR("GLUFComboBox::GetItemData", GR_FAILURE);
		return nullptr;
	}

	return pItem->pData;
}


//--------------------------------------------------------------------------------------
void* GLUFComboBox::GetItemData( int nIndex)
{
	if (nIndex < 0 || nIndex >= (int)m_Items.size())
		return nullptr;

	return m_Items[nIndex]->pData;
}


//--------------------------------------------------------------------------------------
GLUFResult GLUFComboBox::SetSelectedByIndex( unsigned int index)
{
	if (index >= GetNumItems())
		return GR_INVALIDARG;

	m_iFocused = m_iSelected = index;
	m_pDialog->SendEvent(GLUF_EVENT_COMBOBOX_SELECTION_CHANGED, false, this);

	return GR_SUCCESS;
}



//--------------------------------------------------------------------------------------
GLUFResult GLUFComboBox::SetSelectedByText(std::string strText)
{
	/*if (!strText)
		return E_INVALIDARG;*/

	int index = FindItem(strText);
	if (index == -1)
		return GR_FAILURE;

	m_iFocused = m_iSelected = index;
	m_pDialog->SendEvent(GLUF_EVENT_COMBOBOX_SELECTION_CHANGED, false, this);

	return GR_SUCCESS;
}



//--------------------------------------------------------------------------------------
GLUFResult GLUFComboBox::SetSelectedByData( void* pData)
{
	for (size_t i = 0; i < m_Items.size(); i++)
	{
		GLUFComboBoxItem* pItem = m_Items[i];

		if (pItem->pData == pData)
		{
			m_iFocused = m_iSelected = static_cast<int>(i);
			m_pDialog->SendEvent(GLUF_EVENT_COMBOBOX_SELECTION_CHANGED, false, this);
			return GR_SUCCESS;
		}
	}

	return GR_FAILURE;
}


//======================================================================================
// GLUFSlider class
//======================================================================================

GLUFSlider::GLUFSlider( GLUFDialog* pDialog)
{
	m_Type = GLUF_CONTROL_SLIDER;
	m_pDialog = pDialog;

	m_fMin = 0.0f;
	m_fMax = 100.0f / m_pDialog->GetManager()->GetWindowSize().y;
	m_fValue = 0.5f * m_fMax;

	m_bPressed = false;
}


//--------------------------------------------------------------------------------------
bool GLUFSlider::ContainsPoint(GLUFPoint pt)
{
	return (GLUFPtInRect(m_rcBoundingBox, pt) ||
		GLUFPtInRect(m_rcButton, pt));
}


//--------------------------------------------------------------------------------------
void GLUFSlider::UpdateRects()
{
	GLUFControl::UpdateRects();

	m_rcButton = m_rcBoundingBox;
	m_rcButton.right = m_rcButton.left + GLUFRectHeight(m_rcButton);
	GLUFOffsetRect(m_rcButton, -GLUFRectWidth(m_rcButton) / 2, 0);

	m_fButtonX = (float)((m_fValue - m_fMin) * GLUFRectWidth(m_rcBoundingBox) / (m_fMax - m_fMin));
	GLUFOffsetRect(m_rcButton, m_fButtonX, 0);
}


//--------------------------------------------------------------------------------------
float GLUFSlider::ValueFromPos(float x)
{
	//this name is not accurate
	float fValuePerPixel = (m_fMax - m_fMin) / GLUFRectWidth(m_rcBoundingBox);
	return (m_fMin + fValuePerPixel * (x - m_rcBoundingBox.left));
}


//--------------------------------------------------------------------------------------
/*
bool GLUFSlider::HandleKeyboard(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if (!m_bEnabled || !m_bVisible)
		return false;

	switch (uMsg)
	{
	case WM_KEYDOWN:
	{
		switch (wParam)
		{
		case VK_HOME:
			SetValueInternal(m_nMin, true);
			return true;

		case VK_END:
			SetValueInternal(m_nMax, true);
			return true;

		case VK_LEFT:
		case VK_DOWN:
			SetValueInternal(m_nValue - 1, true);
			return true;

		case VK_RIGHT:
		case VK_UP:
			SetValueInternal(m_nValue + 1, true);
			return true;

		case VK_NEXT:
			SetValueInternal(m_nValue - (10 > (m_nMax - m_nMin) / 10 ? 10 : (m_nMax - m_nMin) / 10),
				true);
			return true;

		case VK_PRIOR:
			SetValueInternal(m_nValue + (10 > (m_nMax - m_nMin) / 10 ? 10 : (m_nMax - m_nMin) / 10),
				true);
			return true;
		}
		break;
	}
	}


	return false;
}


//--------------------------------------------------------------------------------------

bool GLUFSlider::HandleMouse(UINT uMsg, const POINT& pt, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if (!m_bEnabled || !m_bVisible)
		return false;

	switch (uMsg)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	{
		if (PtInRect(&m_rcButton, pt))
		{
			// Pressed while inside the control
			m_bPressed = true;
			SetCapture(GLUFGetHWND());

			m_nDragX = pt.x;
			//m_nDragY = pt.y;
			m_nDragOffset = m_nButtonX - m_nDragX;

			//m_nDragValue = m_nValue;

			if (!m_bHasFocus)
				m_pDialog->RequestFocus(this);

			return true;
		}

		if (PtInRect(&m_rcBoundingBox, pt))
		{
			m_nDragX = pt.x;
			m_nDragOffset = 0;
			m_bPressed = true;

			if (!m_bHasFocus)
				m_pDialog->RequestFocus(this);

			if (pt.x > m_nButtonX + m_x)
			{
				SetValueInternal(m_nValue + 1, true);
				return true;
			}

			if (pt.x < m_nButtonX + m_x)
			{
				SetValueInternal(m_nValue - 1, true);
				return true;
			}
		}

		break;
	}

	case WM_LBUTTONUP:
	{
		if (m_bPressed)
		{
			m_bPressed = false;
			ReleaseCapture();
			m_pDialog->SendEvent(GLUF_EVENTSLIDER_VALUE_CHANGED_UP, true, this);

			return true;
		}

		break;
	}

	case WM_MOUSEMOVE:
	{
		if (m_bPressed)
		{
			SetValueInternal(ValueFromPos(m_x + pt.x + m_nDragOffset), true);
			return true;
		}

		break;
	}

	case WM_MOUSEWHEEL:
	{
		int nScrollAmount = int((short)HIWORD(wParam)) / WHEEL_DELTA;
		SetValueInternal(m_nValue - nScrollAmount, true);
		return true;
	}
	};

	return false;
}
*/


//--------------------------------------------------------------------------------------
bool GLUFSlider::MsgProc(GLUF_MESSAGE_TYPE msg, int param1, int param2, int param3, int param4)
{
	if (!m_bEnabled || !m_bVisible)
		return false;

	GLUFPoint pt = m_pDialog->m_MousePositionDialogSpace;

	switch (msg)
	{
		case GM_MB:
			if (param2 == GLFW_PRESS)
			{
				if (GLUFPtInRect(m_rcButton, pt))
				{
					// Pressed while inside the control
					m_bPressed = true;
					//SetCapture(GLUFGetHWND());

					m_fDragX = pt.x;
					//m_nDragY = pt.y;
					m_fDragOffset = m_fButtonX - m_fDragX;

					//m_nDragValue = m_nValue;

					if (!m_bHasFocus)
						m_pDialog->RequestFocus(this);

					return true;
				}

				if (GLUFPtInRect(m_rcBoundingBox, pt))
				{

					if (!m_bHasFocus)
						m_pDialog->RequestFocus(this);

					SetValueInternal(ValueFromPos(pt.x), true);

					return true;
				}
			}
			else if (param2 == GLFW_RELEASE)
			{
				if (m_bPressed)
				{
					m_bPressed = false;
					//ReleaseCapture();
					m_pDialog->SendEvent(GLUF_EVENT_SLIDER_VALUE_CHANGED_UP, true, this);

					return true;
				}

				break;

			}
			break;
	
		case GM_CURSOR_POS:
		{

			if (m_bPressed)
			{
				SetValueInternal(ValueFromPos(m_x + pt.x + m_fDragOffset), true);
				return true;
			}

			break;
		}

		case GM_SCROLL:
		{
			int nScrollAmount = param2 / WHEEL_DELTA;
			SetValueInternal(m_fValue - nScrollAmount, true);
			return true;
		}
		case GM_KEY:
		{
			if (param3 == GLFW_RELEASE)
				break;

			switch (param1)
			{
			case GLFW_KEY_HOME:
				SetValueInternal(m_fMin, true);
				return true;

			case GLFW_KEY_END:
				SetValueInternal(m_fMax, true);
				return true;

			case GLFW_KEY_LEFT:
			case GLFW_KEY_DOWN:
				SetValueInternal(m_fValue - 0.03, true);
				return true;

			case GLFW_KEY_RIGHT:
			case GLFW_KEY_UP:
				SetValueInternal(m_fValue + 0.03, true);
				return true;

			case GLFW_KEY_PAGE_DOWN:
				SetValueInternal(m_fValue - (0.01 > (m_fMax - m_fMin) / 8 ? 8 : (m_fMax - m_fMin) / 8),
					true);
				return true;

			case GLFW_KEY_PAGE_UP:
				SetValueInternal(m_fValue + (0.01 > (m_fMax - m_fMin) / 8 ? 8 : (m_fMax - m_fMin) / 8),
					true);
				return true;
			}
			break;
		}
	};

	return false;
}

//--------------------------------------------------------------------------------------
void GLUFSlider::SetRange(float nMin, float nMax)
{
	m_fMin = nMin;
	m_fMax = nMax;

	SetValueInternal(m_fValue, false);
}


//--------------------------------------------------------------------------------------
void GLUFSlider::SetValueInternal(float nValue, bool bFromInput)
{
	// Clamp to range
	nValue = std::max(m_fMin, nValue);
	nValue = std::min(m_fMax, nValue);

	/* Because of floating point imperfections, we cannot do this well
	if (nValue == m_fValue)
		return;*/

	m_fValue = nValue;
	UpdateRects();

	m_pDialog->SendEvent(GLUF_EVENT_SLIDER_VALUE_CHANGED, bFromInput, this);
}


//--------------------------------------------------------------------------------------
void GLUFSlider::Render( float fElapsedTime)
{
	if (m_bVisible == false)
		return;

	float fOffsetX = 0.0f;
	float fOffsetY = 0.0f;

	GLUF_CONTROL_STATE iState = GLUF_STATE_NORMAL;

	if (m_bVisible == false)
	{
		iState = GLUF_STATE_HIDDEN;
	}
	else if (m_bEnabled == false)
	{
		iState = GLUF_STATE_DISABLED;
	}
	else if (m_bPressed)
	{
		iState = GLUF_STATE_PRESSED;

		fOffsetX = 1.0f / m_pDialog->GetManager()->GetWindowSize().x;
		fOffsetY = 2.0f / m_pDialog->GetManager()->GetWindowSize().y;
	}
	else if (m_bMouseOver)
	{
		iState = GLUF_STATE_MOUSEOVER;

		fOffsetX = -1.0f / m_pDialog->GetManager()->GetWindowSize().x;
		fOffsetY = -2.0f / m_pDialog->GetManager()->GetWindowSize().y;
	}
	else if (m_bHasFocus)
	{
		iState = GLUF_STATE_FOCUS;
	}

	float fBlendRate = (iState == GLUF_STATE_PRESSED) ? 0.0f : 0.8f;

	GLUFElement* pElement = m_Elements[0];

	// Blend current color
	pElement->TextureColor.Blend(iState, fElapsedTime, fBlendRate);
	m_pDialog->DrawSprite(pElement, m_rcBoundingBox, GLUF_FAR_BUTTON_DEPTH);

	pElement = m_Elements[1];

	// Blend current color
	pElement->TextureColor.Blend(iState, fElapsedTime, fBlendRate);
	m_pDialog->DrawSprite(pElement, m_rcButton, GLUF_NEAR_BUTTON_DEPTH);
}


//======================================================================================
// GLUFScrollBar class
//======================================================================================

GLUFScrollBar::GLUFScrollBar( GLUFDialog* pDialog)
{
	m_Type = GLUF_CONTROL_SCROLLBAR;
	m_pDialog = pDialog;

	m_bShowThumb = true;
	m_bDrag = false;

	GLUFSetRect(m_rcUpButton, 0, 0, 0, 0);
	GLUFSetRect(m_rcDownButton, 0, 0, 0, 0);
	GLUFSetRect(m_rcTrack, 0, 0, 0, 0);
	GLUFSetRect(m_rcThumb, 0, 0, 0, 0);
	m_nPosition = 0;
	m_nPageSize = 1;
	m_nStart = 0;
	m_nEnd = 1;
	m_Arrow = CLEAR;
	m_dArrowTS = 0.0;
}


//--------------------------------------------------------------------------------------
GLUFScrollBar::~GLUFScrollBar()
{
}


//--------------------------------------------------------------------------------------
void GLUFScrollBar::UpdateRects()
{
	GLUFControl::UpdateRects();

	// Make the buttons square

	GLUFSetRect(m_rcUpButton, m_rcBoundingBox.left, m_rcBoundingBox.top,
		m_rcBoundingBox.right, m_rcBoundingBox.top + GLUFRectWidth(m_rcBoundingBox));
	GLUFSetRect(m_rcDownButton, m_rcBoundingBox.left, m_rcBoundingBox.bottom - GLUFRectWidth(m_rcBoundingBox),
		m_rcBoundingBox.right, m_rcBoundingBox.bottom);
	GLUFSetRect(m_rcTrack, m_rcUpButton.left, m_rcUpButton.bottom,
		m_rcDownButton.right, m_rcDownButton.top);
	m_rcThumb.left = m_rcUpButton.left;
	m_rcThumb.right = m_rcUpButton.right;

	UpdateThumbRect();
}


//--------------------------------------------------------------------------------------
// Compute the dimension of the scroll thumb
void GLUFScrollBar::UpdateThumbRect()
{
	if (m_nEnd - m_nStart > m_nPageSize)
	{
		float nThumbHeight = std::max((GLUFRectHeight(m_rcTrack) * (float)m_nPageSize / (float)(m_nEnd - m_nStart)),
			SCROLLBAR_MINTHUMBSIZE / m_pDialog->GetManager()->GetWindowSize().y);
		int nMaxPosition = m_nEnd - m_nStart - m_nPageSize;
		m_rcThumb.top = m_rcTrack.top + float(m_nPosition - m_nStart) * (GLUFRectHeight(m_rcTrack) - nThumbHeight)
			/ nMaxPosition;
		m_rcThumb.bottom = m_rcThumb.top + nThumbHeight;
		m_bShowThumb = true;

	}
	else
	{
		// No content to scroll
		m_rcThumb.bottom = m_rcThumb.top;
		m_bShowThumb = false;
	}
}


//--------------------------------------------------------------------------------------
// Scroll() scrolls by nDelta items.  A positive value scrolls down, while a negative
// value scrolls up.
void GLUFScrollBar::Scroll( int nDelta)
{
	// Perform scroll
	m_nPosition += nDelta;

	// Cap position
	Cap();

	// Update thumb position
	UpdateThumbRect();
}


//--------------------------------------------------------------------------------------
void GLUFScrollBar::ShowItem( int nIndex)
{
	// Cap the index

	if (nIndex < 0)
		nIndex = 0;

	if (nIndex >= m_nEnd)
		nIndex = m_nEnd - 1;

	// Adjust position

	if (m_nPosition > nIndex)
		m_nPosition = nIndex;
	else if (m_nPosition + m_nPageSize <= nIndex)
		m_nPosition = nIndex - m_nPageSize + 1;

	UpdateThumbRect();
}


//--------------------------------------------------------------------------------------
/*
bool GLUFScrollBar::HandleKeyboard(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(uMsg);
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);
	return false;
}


//--------------------------------------------------------------------------------------

bool GLUFScrollBar::HandleMouse(UINT uMsg, const POINT& pt, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	static int ThumbOffsetY;

	m_LastMouse = pt;
	switch (uMsg)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	{
		// Check for click on up button

		if (PtInRect(&m_rcUpButton, pt))
		{
			SetCapture(GLUFGetHWND());
			if (m_nPosition > m_nStart)
				--m_nPosition;
			UpdateThumbRect();
			m_Arrow = CLICKED_UP;
			m_dArrowTS = GLUFGetTime();
			return true;
		}

		// Check for click on down button

		if (PtInRect(&m_rcDownButton, pt))
		{
			SetCapture(GLUFGetHWND());
			if (m_nPosition + m_nPageSize <= m_nEnd)
				++m_nPosition;
			UpdateThumbRect();
			m_Arrow = CLICKED_DOWN;
			m_dArrowTS = GLUFGetTime();
			return true;
		}

		// Check for click on thumb

		if (PtInRect(&m_rcThumb, pt))
		{
			SetCapture(GLUFGetHWND());
			m_bDrag = true;
			ThumbOffsetY = pt.y - m_rcThumb.top;
			return true;
		}

		// Check for click on track

		if (m_rcThumb.left <= pt.x &&
			m_rcThumb.right > pt.x)
		{
			SetCapture(GLUFGetHWND());
			if (m_rcThumb.top > pt.y &&
				m_rcTrack.top <= pt.y)
			{
				Scroll(-(m_nPageSize - 1));
				return true;
			}
			else if (m_rcThumb.bottom <= pt.y &&
				m_rcTrack.bottom > pt.y)
			{
				Scroll(m_nPageSize - 1);
				return true;
			}
		}

		break;
	}

	case WM_LBUTTONUP:
	{
		m_bDrag = false;
		ReleaseCapture();
		UpdateThumbRect();
		m_Arrow = CLEAR;
		break;
	}

	case WM_MOUSEMOVE:
	{
		if (m_bDrag)
		{
			m_rcThumb.bottom += pt.y - ThumbOffsetY - m_rcThumb.top;
			m_rcThumb.top = pt.y - ThumbOffsetY;
			if (m_rcThumb.top < m_rcTrack.top)
				OffsetRect(&m_rcThumb, 0, m_rcTrack.top - m_rcThumb.top);
			else if (m_rcThumb.bottom > m_rcTrack.bottom)
				OffsetRect(&m_rcThumb, 0, m_rcTrack.bottom - m_rcThumb.bottom);

			// Compute first item index based on thumb position

			int nMaxFirstItem = m_nEnd - m_nStart - m_nPageSize + 1;  // Largest possible index for first item
			int nMaxThumb = GLUFRectHeight(m_rcTrack) - GLUFRectHeight(m_rcThumb);  // Largest possible thumb position from the top

			m_nPosition = m_nStart +
				(m_rcThumb.top - m_rcTrack.top +
				nMaxThumb / (nMaxFirstItem * 2)) * // Shift by half a row to avoid last row covered by only one pixel
				nMaxFirstItem / nMaxThumb;

			return true;
		}

		break;
	}
	}

	return false;
}
*/


//--------------------------------------------------------------------------------------

bool GLUFScrollBar::MsgProc(GLUF_MESSAGE_TYPE msg, int param1, int param2, int param3, int param4)
{
	//UNREFERENCED_PARAMETER(wParam);

	if (GM_FOCUS == msg && param1 == GL_FALSE)
	{
		// The application just lost mouse capture. We may not have gotten
		// the WM_MOUSEUP message, so reset m_bDrag here.
		//if ((HWND)lParam != GLUFGetHWND())
		m_bDrag = false;
	}

	return false;
}


//--------------------------------------------------------------------------------------
void GLUFScrollBar::Render( float fElapsedTime)
{
	if (m_bVisible == false)
		return;

	// Check if the arrow button has been held for a while.
	// If so, update the thumb position to simulate repeated
	// scroll.
	if (m_Arrow != CLEAR)
	{
		double dCurrTime = GLUFGetTime();
		if (GLUFPtInRect(m_rcUpButton, m_LastMouse))
		{
			switch (m_Arrow)
			{
			case CLICKED_UP:
				if (SCROLLBAR_ARROWCLICK_DELAY < dCurrTime - m_dArrowTS)
				{
					Scroll(-1);
					m_Arrow = HELD_UP;
					m_dArrowTS = dCurrTime;
				}
				break;
			case HELD_UP:
				if (SCROLLBAR_ARROWCLICK_REPEAT < dCurrTime - m_dArrowTS)
				{
					Scroll(-1);
					m_dArrowTS = dCurrTime;
				}
				break;
			}
		}
		else if (GLUFPtInRect(m_rcDownButton, m_LastMouse))
		{
			switch (m_Arrow)
			{
			case CLICKED_DOWN:
				if (SCROLLBAR_ARROWCLICK_DELAY < dCurrTime - m_dArrowTS)
				{
					Scroll(1);
					m_Arrow = HELD_DOWN;
					m_dArrowTS = dCurrTime;
				}
				break;
			case HELD_DOWN:
				if (SCROLLBAR_ARROWCLICK_REPEAT < dCurrTime - m_dArrowTS)
				{
					Scroll(1);
					m_dArrowTS = dCurrTime;
				}
				break;
			}
		}
	}

	GLUF_CONTROL_STATE iState = GLUF_STATE_NORMAL;

	if (m_bVisible == false)
		iState = GLUF_STATE_HIDDEN;
	else if (m_bEnabled == false || m_bShowThumb == false)
		iState = GLUF_STATE_DISABLED;
	else if (m_bMouseOver)
		iState = GLUF_STATE_MOUSEOVER;
	else if (m_bHasFocus)
		iState = GLUF_STATE_FOCUS;


	float fBlendRate = (iState == GLUF_STATE_PRESSED) ? 0.0f : 0.8f;

	// Background track layer
	GLUFElement* pElement = m_Elements[0];

	// Blend current color
	pElement->TextureColor.Blend(iState, fElapsedTime, fBlendRate);
	m_pDialog->DrawSprite(pElement, m_rcTrack, GLUF_FAR_BUTTON_DEPTH);

	// Up Arrow
	pElement = m_Elements[1];

	// Blend current color
	pElement->TextureColor.Blend(iState, fElapsedTime, fBlendRate);
	m_pDialog->DrawSprite(pElement, m_rcUpButton, GLUF_NEAR_BUTTON_DEPTH);

	// Down Arrow
	pElement = m_Elements[2];

	// Blend current color
	pElement->TextureColor.Blend(iState, fElapsedTime, fBlendRate);
	m_pDialog->DrawSprite(pElement, m_rcDownButton, GLUF_NEAR_BUTTON_DEPTH);

	// Thumb button
	pElement = m_Elements[3];

	// Blend current color
	pElement->TextureColor.Blend(iState, fElapsedTime, fBlendRate);
	m_pDialog->DrawSprite(pElement, m_rcThumb, GLUF_NEAR_BUTTON_DEPTH);

}


//--------------------------------------------------------------------------------------
void GLUFScrollBar::SetTrackRange( int nStart,  int nEnd)
{
	m_nStart = nStart; m_nEnd = nEnd;
	Cap();
	UpdateThumbRect();
}


//--------------------------------------------------------------------------------------
void GLUFScrollBar::Cap()  // Clips position at boundaries. Ensures it stays within legal range.
{
	if (m_nPosition < m_nStart ||
		m_nEnd - m_nStart <= m_nPageSize)
	{
		m_nPosition = m_nStart;
	}
	else if (m_nPosition + m_nPageSize > m_nEnd)
		m_nPosition = m_nEnd - m_nPageSize + 1;
}


//======================================================================================
// GLUFListBox class
//======================================================================================

GLUFListBox::GLUFListBox( GLUFDialog* pDialog) : m_ScrollBar(pDialog)
{
	m_Type = GLUF_CONTROL_LISTBOX;
	m_pDialog = pDialog;

	GLUFPoint pt = m_pDialog->GetManager()->GetWindowSize();

	m_dwStyle = 0;
	m_fSBWidth = 16 / pt.x;
	m_nSelected = -1;
	m_nSelStart = 0;
	m_bDrag = false;
	m_fBorderX = 6 / pt.x;
	m_fBorderY = 6 / pt.y;
	m_fMargin = 5 / pt.x;
	m_fTextHeight = 0;
}


//--------------------------------------------------------------------------------------
GLUFListBox::~GLUFListBox()
{
	RemoveAllItems();
}

void GLUFListBox::SetBorderPixels(int nBorder, int nMargin)
{
	GLUFPoint pt = m_pDialog->GetManager()->GetWindowSize();
	m_fBorderX = (float)nBorder / pt.x;
	m_fBorderY = (float)nBorder / pt.y;

	m_fMargin = (float)nMargin / pt.x;
}


//--------------------------------------------------------------------------------------
void GLUFListBox::UpdateRects()
{
	GLUFControl::UpdateRects();

	m_rcSelection = m_rcBoundingBox;
	m_rcSelection.right -= m_fSBWidth;
	GLUFInflateRect(m_rcSelection, -m_fBorderX, -m_fBorderY);
	m_rcText = m_rcSelection;
	GLUFInflateRect(m_rcText, -m_fMargin, 0);

	// Update the scrollbar's rects
	m_ScrollBar.SetLocation(m_rcBoundingBox.right - m_fSBWidth, m_rcBoundingBox.top);
	m_ScrollBar.SetSize(m_fSBWidth, m_height);
	GLUFFontNode* pFontNode = m_pDialog->GetManager()->GetFontNode(m_Elements[0]->iFont);
	if (pFontNode && pFontNode->mSize)
	{
		m_ScrollBar.SetPageSize(int(GLUFRectHeight(m_rcText) / pFontNode->mSize));

		// The selected item may have been scrolled off the page.
		// Ensure that it is in page again.
		m_ScrollBar.ShowItem(m_nSelected);
	}
}


//--------------------------------------------------------------------------------------

GLUFResult GLUFListBox::AddItem(std::string wszText, void* pData)
{
	GLUFListBoxItem* pNewItem = new (std::nothrow) GLUFListBoxItem;
	if (!pNewItem)
		return GR_OUTOFMEMORY;

	//wcscpy_s(pNewItem->strText, 256, wszText);
	pNewItem->strText = wszText;
	pNewItem->pData = pData;
	GLUFSetRect(pNewItem->rcActive, 0, 0, 0, 0);
	pNewItem->bSelected = false;

	m_Items.push_back(pNewItem);
	m_ScrollBar.SetTrackRange(0, (int)m_Items.size());

	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------

GLUFResult GLUFListBox::InsertItem(int nIndex, std::string wszText, void* pData)
{
	GLUFListBoxItem* pNewItem = new (std::nothrow) GLUFListBoxItem;
	if (!pNewItem)
		return GR_OUTOFMEMORY;

	//wcscpy_s(pNewItem->strText, 256, wszText);
	pNewItem->strText = wszText;
	pNewItem->pData = pData;
	GLUFSetRect(pNewItem->rcActive, 0, 0, 0, 0);
	pNewItem->bSelected = false;

	m_Items[nIndex] = pNewItem;
	m_ScrollBar.SetTrackRange(0, (int)m_Items.size());

	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------
void GLUFListBox::RemoveItem( int nIndex)
{
	if (nIndex < 0 || nIndex >= (int)m_Items.size())
		return;

	auto it = m_Items.begin() + nIndex;
	GLUFListBoxItem* pItem = *it;
	delete pItem;
	m_Items.erase(it);
	m_ScrollBar.SetTrackRange(0, (int)m_Items.size());
	if (m_nSelected >= (int)m_Items.size())
		m_nSelected = int(m_Items.size()) - 1;

	m_pDialog->SendEvent(GLUF_EVENT_LISTBOX_SELECTION, true, this);
}


//--------------------------------------------------------------------------------------
void GLUFListBox::RemoveAllItems()
{
	for (auto it = m_Items.begin(); it != m_Items.end(); ++it)
	{
		GLUFListBoxItem* pItem = *it;
		delete pItem;
	}

	m_Items.clear();
	m_ScrollBar.SetTrackRange(0, 1);
}


//--------------------------------------------------------------------------------------
GLUFListBoxItem* GLUFListBox::GetItem( int nIndex)
{
	if (nIndex < 0 || nIndex >= (int)m_Items.size())
		return nullptr;

	return m_Items[nIndex];
}


//--------------------------------------------------------------------------------------
// For single-selection listbox, returns the index of the selected item.
// For multi-selection, returns the first selected item after the nPreviousSelected position.
// To search for the first selected item, the app passes -1 for nPreviousSelected.  For
// subsequent searches, the app passes the returned index back to GetSelectedIndex as.
// nPreviousSelected.
// Returns -1 on error or if no item is selected.
int GLUFListBox::GetSelectedIndex( int nPreviousSelected)
{
	if (nPreviousSelected < -1)
		return -1;

	if (m_dwStyle & MULTISELECTION)
	{
		// Multiple selection enabled. Search for the next item with the selected flag.
		for (int i = nPreviousSelected + 1; i < (int)m_Items.size(); ++i)
		{
			GLUFListBoxItem* pItem = m_Items[i];

			if (pItem->bSelected)
				return i;
		}

		return -1;
	}
	else
	{
		// Single selection
		return m_nSelected;
	}
}


//--------------------------------------------------------------------------------------
void GLUFListBox::SelectItem( int nNewIndex)
{
	// If no item exists, do nothing.
	if (m_Items.size() == 0)
		return;

	int nOldSelected = m_nSelected;

	// Adjust m_nSelected
	m_nSelected = nNewIndex;

	// Perform capping
	if (m_nSelected < 0)
		m_nSelected = 0;
	if (m_nSelected >= (int)m_Items.size())
		m_nSelected = int(m_Items.size()) - 1;

	if (nOldSelected != m_nSelected)
	{
		if (m_dwStyle & MULTISELECTION)
		{
			m_Items[m_nSelected]->bSelected = true;
		}

		// Update selection start
		m_nSelStart = m_nSelected;

		// Adjust scroll bar
		m_ScrollBar.ShowItem(m_nSelected);
	}

	m_pDialog->SendEvent(GLUF_EVENT_LISTBOX_SELECTION, true, this);
}


//--------------------------------------------------------------------------------------
/*
bool GLUFListBox::HandleKeyboard(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (!m_bEnabled || !m_bVisible)
		return false;

	// Let the scroll bar have a chance to handle it first
	if (m_ScrollBar.HandleKeyboard(uMsg, wParam, lParam))
		return true;

	switch (uMsg)
	{
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_UP:
		case VK_DOWN:
		case VK_NEXT:
		case VK_PRIOR:
		case VK_HOME:
		case VK_END:
		{
			// If no item exists, do nothing.
			if (m_Items.size() == 0)
				return true;

			int nOldSelected = m_nSelected;

			// Adjust m_nSelected
			switch (wParam)
			{
			case VK_UP:
				--m_nSelected; break;
			case VK_DOWN:
				++m_nSelected; break;
			case VK_NEXT:
				m_nSelected += m_ScrollBar.GetPageSize() - 1; break;
			case VK_PRIOR:
				m_nSelected -= m_ScrollBar.GetPageSize() - 1; break;
			case VK_HOME:
				m_nSelected = 0; break;
			case VK_END:
				m_nSelected = int(m_Items.size()) - 1; break;
			}

			// Perform capping
			if (m_nSelected < 0)
				m_nSelected = 0;
			if (m_nSelected >= (int)m_Items.size())
				m_nSelected = int(m_Items.size()) - 1;

			if (nOldSelected != m_nSelected)
			{
				if (m_dwStyle & MULTISELECTION)
				{
					// Multiple selection

					// Clear all selection
					for (int i = 0; i < (int)m_Items.size(); ++i)
					{
						GLUFListBoxItem* pItem = m_Items[i];
						pItem->bSelected = false;
					}

					if (GetKeyState(VK_SHIFT) < 0)
					{
						// Select all items from m_nSelStart to
						// m_nSelected
						int nEnd = std::max(m_nSelStart, m_nSelected);

						for (int n = std::min(m_nSelStart, m_nSelected); n <= nEnd; ++n)
							m_Items[n]->bSelected = true;
					}
					else
					{
						m_Items[m_nSelected]->bSelected = true;

						// Update selection start
						m_nSelStart = m_nSelected;
					}
				}
				else
					m_nSelStart = m_nSelected;

				// Adjust scroll bar

				m_ScrollBar.ShowItem(m_nSelected);

				// Send notification

				m_pDialog->SendEvent(GLUF_EVENTLISTBOX_SELECTION, true, this);
			}
			return true;
		}

			// Space is the hotkey for double-clicking an item.
			//
		case VK_SPACE:
			m_pDialog->SendEvent(GLUF_EVENTLISTBOX_ITEM_DBLCLK, true, this);
			return true;
		}
		break;
	}

	return false;
}


//--------------------------------------------------------------------------------------

bool GLUFListBox::HandleMouse(UINT uMsg, const POINT& pt, WPARAM wParam, LPARAM lParam)
{
	if (!m_bEnabled || !m_bVisible)
		return false;

	// First acquire focus
	if (WM_LBUTTONDOWN == uMsg)
		if (!m_bHasFocus)
			m_pDialog->RequestFocus(this);

	// Let the scroll bar handle it first.
	if (m_ScrollBar.HandleMouse(uMsg, pt, wParam, lParam))
		return true;

	switch (uMsg)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
		// Check for clicks in the text area
		if (!m_Items.empty() && PtInRect(&m_rcSelection, pt))
		{
			// Compute the index of the clicked item

			int nClicked;
			if (m_nTextHeight)
				nClicked = m_ScrollBar.GetTrackPos() + (pt.y - m_rcText.top) / m_nTextHeight;
			else
				nClicked = -1;

			// Only proceed if the click falls on top of an item.

			if (nClicked >= m_ScrollBar.GetTrackPos() &&
				nClicked < (int)m_Items.size() &&
				nClicked < m_ScrollBar.GetTrackPos() + m_ScrollBar.GetPageSize())
			{
				SetCapture(GLUFGetHWND());
				m_bDrag = true;

				// If this is a double click, fire off an event and exit
				// since the first click would have taken care of the selection
				// updating.
				if (uMsg == WM_LBUTTONDBLCLK)
				{
					m_pDialog->SendEvent(GLUF_EVENTLISTBOX_ITEM_DBLCLK, true, this);
					return true;
				}

				m_nSelected = nClicked;
				if (!(wParam & MK_SHIFT))
					m_nSelStart = m_nSelected;

				// If this is a multi-selection listbox, update per-item
				// selection data.

				if (m_dwStyle & MULTISELECTION)
				{
					// Determine behavior based on the state of Shift and Ctrl

					GLUFListBoxItem* pSelItem = m_Items[m_nSelected];
					if ((wParam & (MK_SHIFT | MK_CONTROL)) == MK_CONTROL)
					{
						// Control click. Reverse the selection of this item.

						pSelItem->bSelected = !pSelItem->bSelected;
					}
					else if ((wParam & (MK_SHIFT | MK_CONTROL)) == MK_SHIFT)
					{
						// Shift click. Set the selection for all items
						// from last selected item to the current item.
						// Clear everything else.

						int nBegin = std::min(m_nSelStart, m_nSelected);
						int nEnd = std::max(m_nSelStart, m_nSelected);

						for (int i = 0; i < nBegin; ++i)
						{
							GLUFListBoxItem* pItem = m_Items[i];
							pItem->bSelected = false;
						}

						for (int i = nEnd + 1; i < (int)m_Items.size(); ++i)
						{
							GLUFListBoxItem* pItem = m_Items[i];
							pItem->bSelected = false;
						}

						for (int i = nBegin; i <= nEnd; ++i)
						{
							GLUFListBoxItem* pItem = m_Items[i];
							pItem->bSelected = true;
						}
					}
					else if ((wParam & (MK_SHIFT | MK_CONTROL)) == (MK_SHIFT | MK_CONTROL))
					{
						// Control-Shift-click.

						// The behavior is:
						//   Set all items from m_nSelStart to m_nSelected to
						//     the same state as m_nSelStart, not including m_nSelected.
						//   Set m_nSelected to selected.

						int nBegin = std::min(m_nSelStart, m_nSelected);
						int nEnd = std::max(m_nSelStart, m_nSelected);

						// The two ends do not need to be set here.

						bool bLastSelected = m_Items[m_nSelStart]->bSelected;
						for (int i = nBegin + 1; i < nEnd; ++i)
						{
							GLUFListBoxItem* pItem = m_Items[i];
							pItem->bSelected = bLastSelected;
						}

						pSelItem->bSelected = true;

						// Restore m_nSelected to the previous value
						// This matches the Windows behavior

						m_nSelected = m_nSelStart;
					}
					else
					{
						// Simple click.  Clear all items and select the clicked
						// item.


						for (int i = 0; i < (int)m_Items.size(); ++i)
						{
							GLUFListBoxItem* pItem = m_Items[i];
							pItem->bSelected = false;
						}

						pSelItem->bSelected = true;
					}
				}  // End of multi-selection case

				m_pDialog->SendEvent(GLUF_EVENTLISTBOX_SELECTION, true, this);
			}

			return true;
		}
		break;

	case WM_LBUTTONUP:
	{
		ReleaseCapture();
		m_bDrag = false;

		if (m_nSelected != -1)
		{
			// Set all items between m_nSelStart and m_nSelected to
			// the same state as m_nSelStart
			int nEnd = std::max(m_nSelStart, m_nSelected);

			for (int n = std::min(m_nSelStart, m_nSelected) + 1; n < nEnd; ++n)
				m_Items[n]->bSelected = m_Items[m_nSelStart]->bSelected;
			m_Items[m_nSelected]->bSelected = m_Items[m_nSelStart]->bSelected;

			// If m_nSelStart and m_nSelected are not the same,
			// the user has dragged the mouse to make a selection.
			// Notify the application of this.
			if (m_nSelStart != m_nSelected)
				m_pDialog->SendEvent(GLUF_EVENTLISTBOX_SELECTION, true, this);

			m_pDialog->SendEvent(GLUF_EVENTLISTBOX_SELECTION_END, true, this);
		}
		return false;
	}

	case WM_MOUSEMOVE:
		if (m_bDrag)
		{
			// Compute the index of the item below cursor

			int nItem;
			if (m_nTextHeight)
				nItem = m_ScrollBar.GetTrackPos() + (pt.y - m_rcText.top) / m_nTextHeight;
			else
				nItem = -1;

			// Only proceed if the cursor is on top of an item.

			if (nItem >= (int)m_ScrollBar.GetTrackPos() &&
				nItem < (int)m_Items.size() &&
				nItem < m_ScrollBar.GetTrackPos() + m_ScrollBar.GetPageSize())
			{
				m_nSelected = nItem;
				m_pDialog->SendEvent(GLUF_EVENTLISTBOX_SELECTION, true, this);
			}
			else if (nItem < (int)m_ScrollBar.GetTrackPos())
			{
				// User drags the mouse above window top
				m_ScrollBar.Scroll(-1);
				m_nSelected = m_ScrollBar.GetTrackPos();
				m_pDialog->SendEvent(GLUF_EVENTLISTBOX_SELECTION, true, this);
			}
			else if (nItem >= m_ScrollBar.GetTrackPos() + m_ScrollBar.GetPageSize())
			{
				// User drags the mouse below window bottom
				m_ScrollBar.Scroll(1);
				m_nSelected = std::min((int)m_Items.size(), m_ScrollBar.GetTrackPos() +
					m_ScrollBar.GetPageSize()) - 1;
				m_pDialog->SendEvent(GLUF_EVENTLISTBOX_SELECTION, true, this);
			}
		}
		break;

	case WM_MOUSEWHEEL:
	{
		UINT uLines = 0;
		if (!SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &uLines, 0))
			uLines = 0;
		int nScrollAmount = int((short)HIWORD(wParam)) / WHEEL_DELTA * uLines;
		m_ScrollBar.Scroll(-nScrollAmount);
		return true;
	}
	}

	return false;
}
*/

//--------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
bool GLUFListBox::MsgProc(GLUF_MESSAGE_TYPE msg, int param1, int param2, int param3, int param4)
{
	//TODO:

	//UNREFERENCED_PARAMETER(wParam);

	if (GM_FOCUS == msg && param1 == GL_FALSE)
	{
		// The application just lost mouse capture. We may not have gotten
		// the WM_MOUSEUP message, so reset m_bDrag here.
		m_bDrag = false;
	}

	return false;
}


//--------------------------------------------------------------------------------------
void GLUFListBox::Render( float fElapsedTime)
{
	if (m_bVisible == false)
		return;

	GLUFElement* pElement = m_Elements[0];
	pElement->TextureColor.Blend(GLUF_STATE_NORMAL, fElapsedTime);
	pElement->FontColor.Blend(GLUF_STATE_NORMAL, fElapsedTime);

	GLUFElement* pSelElement = m_Elements[1];
	pSelElement->TextureColor.Blend(GLUF_STATE_NORMAL, fElapsedTime);
	pSelElement->FontColor.Blend(GLUF_STATE_NORMAL, fElapsedTime);

	m_pDialog->DrawSprite(pElement, m_rcBoundingBox, GLUF_FAR_BUTTON_DEPTH);

	// Render the text
	if (!m_Items.empty())
	{
		// Find out the height of a single line of text
		GLUFRect rc = m_rcText;
		GLUFRect rcSel = m_rcSelection;
		rc.bottom = rc.top - m_pDialog->GetManager()->GetFontNode(pElement->iFont)->mSize / m_pDialog->GetManager()->GetWindowSize().y;

		// Update the line height formation
		m_fTextHeight = rc.top - rc.bottom;

		static bool bSBInit;
		if (!bSBInit)
		{
			// Update the page size of the scroll bar
			if (m_fTextHeight)
				m_ScrollBar.SetPageSize(int(GLUFRectHeight(m_rcText) / m_fTextHeight));
			else
				m_ScrollBar.SetPageSize((GLUFRectHeight(m_rcText)));
			bSBInit = true;
		}

		rc.right = m_rcText.right;
		for (int i = m_ScrollBar.GetTrackPos(); i < (int)m_Items.size(); ++i)
		{
			if (rc.bottom > m_rcText.bottom)
				break;

			GLUFListBoxItem* pItem = m_Items[i];

			// Determine if we need to render this item with the
			// selected element.
			bool bSelectedStyle = false;

			if (!(m_dwStyle & MULTISELECTION) && i == m_nSelected)
				bSelectedStyle = true;
			else if (m_dwStyle & MULTISELECTION)
			{
				if (m_bDrag &&
					((i >= m_nSelected && i < m_nSelStart) ||
					(i <= m_nSelected && i > m_nSelStart)))
					bSelectedStyle = m_Items[m_nSelStart]->bSelected;
				else if (pItem->bSelected)
					bSelectedStyle = true;
			}

			if (bSelectedStyle)
			{
				rcSel.top = rc.top; rcSel.bottom = rc.bottom;
				m_pDialog->DrawSprite(pSelElement, rcSel, GLUF_NEAR_BUTTON_DEPTH);
				m_pDialog->DrawText(pItem->strText, pSelElement, rc);
			}
			else
				m_pDialog->DrawText(pItem->strText, pElement, rc);

			GLUFOffsetRect(rc, 0, m_fTextHeight);
		}
	}

	// Render the scroll bar

	m_ScrollBar.Render(fElapsedTime);
}


//======================================================================================
// GLUFEditBox class
//======================================================================================

// Static member initialization
bool GLUFEditBox::s_bHideCaret;   // If true, we don't render the caret.

// When scrolling, EDITBOX_SCROLLEXTENT is reciprocal of the amount to scroll.
// If EDITBOX_SCROLLEXTENT = 4, then we scroll 1/4 of the control each time.
#define EDITBOX_SCROLLEXTENT 4

//--------------------------------------------------------------------------------------
GLUFEditBox::GLUFEditBox(GLUFDialog* pDialog) : m_Buffer(pDialog->GetManager())
{
	m_Type = GLUF_CONTROL_EDITBOX;
	m_pDialog = pDialog;

	m_fBorderX  = 5 / m_pDialog->GetManager()->GetWindowSize().x;  // Default border width
	m_fSpacingX = 4 / m_pDialog->GetManager()->GetWindowSize().x;  // Default spacing
	m_fBorderY  = 5 / m_pDialog->GetManager()->GetWindowSize().y;
	m_fSpacingY = 4 / m_pDialog->GetManager()->GetWindowSize().y;

	m_bCaretOn = true;
	m_dfBlink = GetCaretBlinkTime() * 0.001f;
	m_dfLastBlink = glfwGetTime();
	s_bHideCaret = false;
	m_nFirstVisible = 0;
	m_TextColor = Color(16, 16, 16, 255);
	m_SelTextColor = Color(255, 255, 255, 255);
	m_SelBkColor = Color(40, 50, 92, 255);
	m_CaretColor = Color(0, 0, 0, 255);
	m_nCaret = m_nSelStart = 0;
	m_bInsertMode = true;

	m_bMouseDrag = false;
}


//--------------------------------------------------------------------------------------
GLUFEditBox::~GLUFEditBox()
{
}


//--------------------------------------------------------------------------------------
// PlaceCaret: Set the caret to a character position, and adjust the scrolling if
//             necessary.
//--------------------------------------------------------------------------------------
void GLUFEditBox::PlaceCaret( int nCP)
{
	GLUF_ASSERT(nCP >= 0 && nCP <= m_Buffer.GetTextSize());
	m_nCaret = nCP;

	// Obtain the X offset of the character.
	int nX1st, nX, nX2;
	m_Buffer.CPtoX(m_nFirstVisible, false, &nX1st);  // 1st visible char
	m_Buffer.CPtoX(nCP, false, &nX);  // LEAD
	// If nCP is the nul terminator, get the leading edge instead of trailing.
	if (nCP == m_Buffer.GetTextSize())
		nX2 = nX;
	else
		m_Buffer.CPtoX(nCP, true, &nX2);  // TRAIL

	// If the left edge of the char is smaller than the left edge of the 1st visible char,
	// we need to scroll left until this char is visible.
	if (nX < nX1st)
	{
		// Simply make the first visible character the char at the new caret position.
		m_nFirstVisible = nCP;
	}
	else // If the right of the character is bigger than the offset of the control's
		// right edge, we need to scroll right to this character.
		if (nX2 > nX1st + GLUFRectWidth(m_rcText))
		{
			// Compute the X of the new left-most pixel
			int nXNewLeft = nX2 - GLUFRectWidth(m_rcText);

			// Compute the char position of this character
			int nCPNew1st;
			bool nNewTrail;
			m_Buffer.XtoCP(nXNewLeft, &nCPNew1st, &nNewTrail);

			// If this coordinate is not on a character border,
			// start from the next character so that the caret
			// position does not fall outside the text rectangle.
			int nXNew1st;
			m_Buffer.CPtoX(nCPNew1st, false, &nXNew1st);
			if (nXNew1st < nXNewLeft)
				++nCPNew1st;

			m_nFirstVisible = nCPNew1st;
		}

}


//--------------------------------------------------------------------------------------
void GLUFEditBox::ClearText()
{
	m_Buffer.Clear();
	m_nFirstVisible = 0;
	PlaceCaret(0);
	m_nSelStart = 0;
}


//--------------------------------------------------------------------------------------
void GLUFEditBox::SetText( std::string wszText,  bool bSelected)
{
	//assert(wszText);

	m_Buffer.SetText(wszText);
	m_nFirstVisible = 0;
	// Move the caret to the end of the text
	PlaceCaret(m_Buffer.GetTextSize());
	m_nSelStart = bSelected ? 0 : m_nCaret;
}


//--------------------------------------------------------------------------------------

GLUFResult GLUFEditBox::GetTextCopy(std::string& strDest)
{
	//assert(strDest);

	//wcscpy_s(strDest, bufferCount, m_Buffer.GetBuffer());
	strDest = m_Buffer.GetBuffer();
	
	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------
void GLUFEditBox::DeleteSelectionText()
{
	int nFirst = std::min(m_nCaret, m_nSelStart);
	int nLast = std::max(m_nCaret, m_nSelStart);
	// Update caret and selection
	PlaceCaret(nFirst);
	m_nSelStart = m_nCaret;
	// Remove the characters
	for (int i = nFirst; i < nLast; ++i)
		m_Buffer.RemoveChar(nFirst);
}


//--------------------------------------------------------------------------------------
void GLUFEditBox::UpdateRects()
{
	GLUFControl::UpdateRects();

	// Update the text rectangle
	m_rcText = m_rcBoundingBox;
	// First inflate by m_nBorder to compute render rects
	GLUFInflateRect(m_rcText, -m_fBorderX, -m_fBorderY);

	// Update the render rectangles
	m_rcRender[0] = m_rcText;
	GLUFSetRect(m_rcRender[1], m_rcBoundingBox.left, m_rcBoundingBox.top, m_rcText.left, m_rcText.top);
	GLUFSetRect(m_rcRender[2], m_rcText.left, m_rcBoundingBox.top, m_rcText.right, m_rcText.top);
	GLUFSetRect(m_rcRender[3], m_rcText.right, m_rcBoundingBox.top, m_rcBoundingBox.right, m_rcText.top);
	GLUFSetRect(m_rcRender[4], m_rcBoundingBox.left, m_rcText.top, m_rcText.left, m_rcText.bottom);
	GLUFSetRect(m_rcRender[5], m_rcText.right, m_rcText.top, m_rcBoundingBox.right, m_rcText.bottom);
	GLUFSetRect(m_rcRender[6], m_rcBoundingBox.left, m_rcText.bottom, m_rcText.left, m_rcBoundingBox.bottom);
	GLUFSetRect(m_rcRender[7], m_rcText.left, m_rcText.bottom, m_rcText.right, m_rcBoundingBox.bottom);
	GLUFSetRect(m_rcRender[8], m_rcText.right, m_rcText.bottom, m_rcBoundingBox.right, m_rcBoundingBox.bottom);
	
	// Inflate further by m_nSpacing
	GLUFInflateRect(m_rcText, -m_fSpacingX, -m_fSpacingY);
}


#pragma warning(push)
#pragma warning( disable : 4616 6386 )
void GLUFEditBox::CopyToClipboard()
{
	// Copy the selection text to the clipboard
	if (m_nCaret != m_nSelStart/* && OpenClipboard(nullptr)*/)
	{
		//EmptyClipboard();

		/*HGLOBAL hBlock = GlobalAlloc(GMEM_MOVEABLE, sizeof(WCHAR) * (m_Buffer.GetTextSize() + 1));
		if (hBlock)
		{
			WCHAR* pwszText = (WCHAR*)GlobalLock(hBlock);
			if (pwszText)
			{
				int nFirst = std::min(m_nCaret, m_nSelStart);
				int nLast = std::max(m_nCaret, m_nSelStart);
				if (nLast - nFirst > 0)
				{
					memcpy(pwszText, m_Buffer.GetBuffer() + nFirst, (nLast - nFirst) * sizeof(WCHAR));
				}
				pwszText[nLast - nFirst] = L'\0';  // Terminate it
				GlobalUnlock(hBlock);
			}
			SetClipboardData(CF_UNICODETEXT, hBlock);
		}
		CloseClipboard();
		// We must not free the object until CloseClipboard is called.
		if (hBlock)
			GlobalFree(hBlock);*/

		//glfw makes this easy
		std::string str = "";
		std::string strBuffer = m_Buffer.GetBuffer();
		for (int i = m_nSelStart; i < m_nCaret; ++i)
		{
			str += strBuffer[i];
		}

		glfwSetClipboardString(g_pGLFWWindow, str.c_str());
	}
}


void GLUFEditBox::PasteFromClipboard()
{
	DeleteSelectionText();

	/*if (OpenClipboard(nullptr))
	{
		HANDLE handle = GetClipboardData(CF_UNICODETEXT);
		if (handle)
		{
			// Convert the ANSI string to Unicode, then
			// insert to our buffer.
			WCHAR* pwszText = (WCHAR*)GlobalLock(handle);
			if (pwszText)
			{
				// Copy all characters up to null.
				if (m_Buffer.InsertString(m_nCaret, pwszText))
					PlaceCaret(m_nCaret + (int)wcslen(pwszText));
				m_nSelStart = m_nCaret;
				GlobalUnlock(handle);
			}
		}
		CloseClipboard();
	}*/

	//glfw makes this easy
	const char* str;
	str = glfwGetClipboardString(g_pGLFWWindow);

	m_Buffer.InsertString(m_nSelStart, str);

	delete[] str;
}
#pragma warning(pop)


//--------------------------------------------------------------------------------------
/*
bool GLUFEditBox::HandleKeyboard(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);

	if (!m_bEnabled || !m_bVisible)
		return false;

	bool bHandled = false;

	switch (uMsg)
	{
	case WM_KEYDOWN:
	{
		switch (wParam)
		{
		case VK_TAB:
			// We don't process Tab in case keyboard input is enabled and the user
			// wishes to Tab to other controls.
			break;

		case VK_HOME:
			PlaceCaret(0);
			if (GetKeyState(VK_SHIFT) >= 0)
				// Shift is not down. Update selection
				// start along with the caret.
				m_nSelStart = m_nCaret;
			ResetCaretBlink();
			bHandled = true;
			break;

		case VK_END:
			PlaceCaret(m_Buffer.GetTextSize());
			if (GetKeyState(VK_SHIFT) >= 0)
				// Shift is not down. Update selection
				// start along with the caret.
				m_nSelStart = m_nCaret;
			ResetCaretBlink();
			bHandled = true;
			break;

		case VK_INSERT:
			if (GetKeyState(VK_CONTROL) < 0)
			{
				// Control Insert. Copy to clipboard
				CopyToClipboard();
			}
			else if (GetKeyState(VK_SHIFT) < 0)
			{
				// Shift Insert. Paste from clipboard
				PasteFromClipboard();
			}
			else
			{
				// Toggle caret insert mode
				m_bInsertMode = !m_bInsertMode;
			}
			break;

		case VK_DELETE:
			// Check if there is a text selection.
			if (m_nCaret != m_nSelStart)
			{
				DeleteSelectionText();
				m_pDialog->SendEvent(GLUF_EVENTEDITBOX_CHANGE, true, this);
			}
			else
			{
				// Deleting one character
				if (m_Buffer.RemoveChar(m_nCaret))
					m_pDialog->SendEvent(GLUF_EVENTEDITBOX_CHANGE, true, this);
			}
			ResetCaretBlink();
			bHandled = true;
			break;

		case VK_LEFT:
			if (GetKeyState(VK_CONTROL) < 0)
			{
				// Control is down. Move the caret to a new item
				// instead of a character.
				m_Buffer.GetPriorItemPos(m_nCaret, &m_nCaret);
				PlaceCaret(m_nCaret);
			}
			else if (m_nCaret > 0)
				PlaceCaret(m_nCaret - 1);
			if (GetKeyState(VK_SHIFT) >= 0)
				// Shift is not down. Update selection
				// start along with the caret.
				m_nSelStart = m_nCaret;
			ResetCaretBlink();
			bHandled = true;
			break;

		case VK_RIGHT:
			if (GetKeyState(VK_CONTROL) < 0)
			{
				// Control is down. Move the caret to a new item
				// instead of a character.
				m_Buffer.GetNextItemPos(m_nCaret, &m_nCaret);
				PlaceCaret(m_nCaret);
			}
			else if (m_nCaret < m_Buffer.GetTextSize())
				PlaceCaret(m_nCaret + 1);
			if (GetKeyState(VK_SHIFT) >= 0)
				// Shift is not down. Update selection
				// start along with the caret.
				m_nSelStart = m_nCaret;
			ResetCaretBlink();
			bHandled = true;
			break;

		case VK_UP:
		case VK_DOWN:
			// Trap up and down arrows so that the dialog
			// does not switch focus to another control.
			bHandled = true;
			break;

		default:
			bHandled = wParam != VK_ESCAPE;  // Let the application handle Esc.
		}
	}
	}
	return bHandled;
}


//--------------------------------------------------------------------------------------

bool GLUFEditBox::HandleMouse(UINT uMsg, const POINT& pt, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(wParam);
	UNREFERENCED_PARAMETER(lParam);

	if (!m_bEnabled || !m_bVisible)
		return false;

	switch (uMsg)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONDBLCLK:
	{
		if (!m_bHasFocus)
			m_pDialog->RequestFocus(this);

		if (!ContainsPoint(pt))
			return false;

		m_bMouseDrag = true;
		SetCapture(GLUFGetHWND());
		// Determine the character corresponding to the coordinates.
		int nCP, nTrail, nX1st;
		m_Buffer.CPtoX(m_nFirstVisible, FALSE, &nX1st);  // X offset of the 1st visible char
		if (m_Buffer.XtoCP(pt.x - m_rcText.left + nX1st, &nCP, &nTrail))
		{
			// Cap at the nul character.
			if (nTrail && nCP < m_Buffer.GetTextSize())
				PlaceCaret(nCP + 1);
			else
				PlaceCaret(nCP);
			m_nSelStart = m_nCaret;
			ResetCaretBlink();
		}
		return true;
	}

	case WM_LBUTTONUP:
		ReleaseCapture();
		m_bMouseDrag = false;
		break;

	case WM_MOUSEMOVE:
		if (m_bMouseDrag)
		{
			// Determine the character corresponding to the coordinates.
			int nCP, nTrail, nX1st;
			m_Buffer.CPtoX(m_nFirstVisible, FALSE, &nX1st);  // X offset of the 1st visible char
			if (m_Buffer.XtoCP(pt.x - m_rcText.left + nX1st, &nCP, &nTrail))
			{
				// Cap at the nul character.
				if (nTrail && nCP < m_Buffer.GetTextSize())
					PlaceCaret(nCP + 1);
				else
					PlaceCaret(nCP);
			}
		}
		break;
	}

	return false;
}
*/

//--------------------------------------------------------------------------------------
void GLUFEditBox::OnFocusIn()
{
	GLUFControl::OnFocusIn();

	ResetCaretBlink();
}


//--------------------------------------------------------------------------------------

bool GLUFEditBox::MsgProc(GLUF_MESSAGE_TYPE msg, int param1, int param2, int param3, int param4)
{
	//TOOD:
	/*
	UNREFERENCED_PARAMETER(lParam);

	if (!m_bEnabled || !m_bVisible)
		return false;

	switch (uMsg)
	{
		// Make sure that while editing, the keyup and keydown messages associated with 
		// WM_CHAR messages don't go to any non-focused controls or cameras
	case WM_KEYUP:
	case WM_KEYDOWN:
		return true;

	case WM_CHAR:
	{
		switch ((WCHAR)wParam)
		{
			// Backspace
		case VK_BACK:
		{
			// If there's a selection, treat this
			// like a delete key.
			if (m_nCaret != m_nSelStart)
			{
				DeleteSelectionText();
				m_pDialog->SendEvent(GLUF_EVENTEDITBOX_CHANGE, true, this);
			}
			else if (m_nCaret > 0)
			{
				// Move the caret, then delete the char.
				PlaceCaret(m_nCaret - 1);
				m_nSelStart = m_nCaret;
				m_Buffer.RemoveChar(m_nCaret);
				m_pDialog->SendEvent(GLUF_EVENTEDITBOX_CHANGE, true, this);
			}
			ResetCaretBlink();
			break;
		}

		case 24:        // Ctrl-X Cut
		case VK_CANCEL: // Ctrl-C Copy
		{
			CopyToClipboard();

			// If the key is Ctrl-X, delete the selection too.
			if ((WCHAR)wParam == 24)
			{
				DeleteSelectionText();
				m_pDialog->SendEvent(GLUF_EVENTEDITBOX_CHANGE, true, this);
			}

			break;
		}

			// Ctrl-V Paste
		case 22:
		{
			PasteFromClipboard();
			m_pDialog->SendEvent(GLUF_EVENTEDITBOX_CHANGE, true, this);
			break;
		}

			// Ctrl-A Select All
		case 1:
			if (m_nSelStart == m_nCaret)
			{
				m_nSelStart = 0;
				PlaceCaret(m_Buffer.GetTextSize());
			}
			break;

		case VK_RETURN:
			// Invoke the callback when the user presses Enter.
			m_pDialog->SendEvent(GLUF_EVENTEDITBOX_STRING, true, this);
			break;

			// Junk characters we don't want in the string
		case 26:  // Ctrl Z
		case 2:   // Ctrl B
		case 14:  // Ctrl N
		case 19:  // Ctrl S
		case 4:   // Ctrl D
		case 6:   // Ctrl F
		case 7:   // Ctrl G
		case 10:  // Ctrl J
		case 11:  // Ctrl K
		case 12:  // Ctrl L
		case 17:  // Ctrl Q
		case 23:  // Ctrl W
		case 5:   // Ctrl E
		case 18:  // Ctrl R
		case 20:  // Ctrl T
		case 25:  // Ctrl Y
		case 21:  // Ctrl U
		case 9:   // Ctrl I
		case 15:  // Ctrl O
		case 16:  // Ctrl P
		case 27:  // Ctrl [
		case 29:  // Ctrl ]
		case 28:  // Ctrl \ 
			break;

		default:
		{
			// If there's a selection and the user
			// starts to type, the selection should
			// be deleted.
			if (m_nCaret != m_nSelStart)
				DeleteSelectionText();

			// If we are in overwrite mode and there is already
			// a char at the caret's position, simply replace it.
			// Otherwise, we insert the char as normal.
			if (!m_bInsertMode && m_nCaret < m_Buffer.GetTextSize())
			{
				m_Buffer[m_nCaret] = (WCHAR)wParam;
				PlaceCaret(m_nCaret + 1);
				m_nSelStart = m_nCaret;
			}
			else
			{
				// Insert the char
				if (m_Buffer.InsertChar(m_nCaret, (WCHAR)wParam))
				{
					PlaceCaret(m_nCaret + 1);
					m_nSelStart = m_nCaret;
				}
			}
			ResetCaretBlink();
			m_pDialog->SendEvent(GLUF_EVENTEDITBOX_CHANGE, true, this);
		}
		}
		return true;
	}
	}*/
	return false;
}


//--------------------------------------------------------------------------------------
void GLUFEditBox::Render( float fElapsedTime)
{
	if (m_bVisible == false)
		return;

	int nSelStartX = 0, nCaretX = 0;  // Left and right X cordinates of the selection region

	GLUFElement* pElement = GetElement(0);
	if (pElement)
	{
		m_Buffer.SetFontNode(m_pDialog->GetFont(pElement->iFont));
		PlaceCaret(m_nCaret);  // Call PlaceCaret now that we have the font info (node),
		// so that scrolling can be handled.
	}

	// Render the control graphics
	for (int e = 0; e < 9; ++e)
	{
		pElement = m_Elements[e];
		pElement->TextureColor.Blend(GLUF_STATE_NORMAL, fElapsedTime);

		m_pDialog->DrawSprite(pElement, m_rcRender[e], GLUF_FAR_BUTTON_DEPTH);
	}

	//
	// Compute the X coordinates of the first visible character.
	//
	int nXFirst;
	m_Buffer.CPtoX(m_nFirstVisible, false, &nXFirst);

	//
	// Compute the X coordinates of the selection rectangle
	//
	m_Buffer.CPtoX(m_nCaret, false, &nCaretX);
	if (m_nCaret != m_nSelStart)
		m_Buffer.CPtoX(m_nSelStart, false, &nSelStartX);
	else
		nSelStartX = nCaretX;

	//
	// Render the selection rectangle
	//
	GLUFRect rcSelection;  // Make this available for rendering selected text
	if (m_nCaret != m_nSelStart)
	{
		int nSelLeftX = nCaretX, nSelRightX = nSelStartX;
		// Swap if left is bigger than right
		if (nSelLeftX > nSelRightX)
		{
			int nTemp = nSelLeftX; nSelLeftX = nSelRightX; nSelRightX = nTemp;
		}

		GLUFSetRect(rcSelection, nSelLeftX, m_rcText.top, nSelRightX, m_rcText.bottom);
		GLUFOffsetRect(rcSelection, m_rcText.left - nXFirst, 0);
		GLUFIntersectRect(rcSelection, m_rcText, rcSelection);

		m_pDialog->DrawRect(rcSelection, m_SelBkColor);
	}

	//
	// Render the text
	//
	// Element 0 for text
	m_Elements[0]->FontColor.SetCurrent(m_TextColor);
	m_pDialog->DrawText(m_Buffer.GetBuffer(m_nFirstVisible), m_Elements[0], m_rcText);

	// Render the selected text
	if (m_nCaret != m_nSelStart)
	{
		int nFirstToRender = std::max(m_nFirstVisible, std::min(m_nSelStart, m_nCaret));
		m_Elements[0]->FontColor.SetCurrent(m_SelTextColor);
		m_pDialog->DrawText(m_Buffer.GetBuffer(nFirstToRender),
			m_Elements[0], rcSelection, false);
	}

	//
	// Blink the caret
	//
	if (glfwGetTime() - m_dfLastBlink >= m_dfBlink)
	{
		m_bCaretOn = !m_bCaretOn;
		m_dfLastBlink = glfwGetTime();
	}

	//
	// Render the caret if this control has the focus
	//
	if (m_bHasFocus && m_bCaretOn && !s_bHideCaret)
	{
		// Start the rectangle with insert mode caret
		GLUFRect rcCaret =
		{
			m_rcText.left - nXFirst + nCaretX - 1, m_rcText.top,
			m_rcText.left - nXFirst + nCaretX + 1, m_rcText.bottom
		};

		// If we are in overwrite mode, adjust the caret rectangle
		// to fill the entire character.
		if (!m_bInsertMode)
		{
			// Obtain the right edge X coord of the current character
			int nRightEdgeX;
			m_Buffer.CPtoX(m_nCaret, true, &nRightEdgeX);
			rcCaret.right = m_rcText.left - nXFirst + nRightEdgeX;
		}

		m_pDialog->DrawRect(rcCaret, m_CaretColor);
	}
}


#define IN_FLOAT_CHARSET( c ) \
    ( (c) == '-' || (c) == '.' || ( (c) >= '0' && (c) <= '9' ) )


void GLUFEditBox::ParseFloatArray(float* pNumbers, int nCount)
{
	int nWritten = 0;  // Number of floats written
	const char* pToken, *pEnd;
	char wszToken[60];

	pToken = m_Buffer.GetBuffer().c_str();
	while (nWritten < nCount && *pToken != L'\0')
	{
		// Skip leading spaces
		while (*pToken == ' ')
			++pToken;

		if (*pToken == '\0')
			break;

		// Locate the end of number
		pEnd = pToken;
		while (IN_FLOAT_CHARSET(*pEnd))
			++pEnd;

		// Copy the token to our buffer
		int nTokenLen = std::min<int>(sizeof(wszToken) / sizeof(wszToken[0]) - 1, int(pEnd - pToken));
		strcpy_s(wszToken, nTokenLen, pToken);
		*pNumbers = (float)strtod(wszToken, nullptr);
		++nWritten;
		++pNumbers;
		pToken = pEnd;
	}
}


//--------------------------------------------------------------------------------------

void GLUFEditBox::SetTextFloatArray(const float* pNumbers, int nCount)
{
	char wszBuffer[512] =
	{
		0
	};
	char wszTmp[64];

	if (!pNumbers)
		return;

	for (int i = 0; i < nCount; ++i)
	{
		sprintf_s(wszTmp, 64, "%.4f ", pNumbers[i]);
		strcat_s(wszBuffer, 512, wszTmp);
	}

	// Don't want the last space
	if (nCount > 0 && strlen(wszBuffer) > 0)
		wszBuffer[strlen(wszBuffer) - 1] = 0;

	SetText(wszBuffer);
}


//--------------------------------------------------------------------------------------
void GLUFEditBox::ResetCaretBlink()
{
	m_bCaretOn = true;
	m_dfLastBlink = glfwGetTime();
}



//======================================================================================
// GLUFUniBuffer
//======================================================================================

/*  We do not need to set buffer size becuase std::string automatically expands
//--------------------------------------------------------------------------------------
bool GLUFUniBuffer::SetBufferSize(int nNewSize)
{
	// If the current size is already the maximum allowed,
	// we can't possibly allocate more.
	if (m_nBufferSize >= GLUF_MAX_EDITBOXLENGTH)
		return false;

	int nAllocateSize = (nNewSize == -1 || nNewSize < m_nBufferSize * 2) ? (m_nBufferSize ? m_nBufferSize *
		2 : 256) : nNewSize * 2;

	// Cap the buffer size at the maximum allowed.
	if (nAllocateSize > GLUF_MAX_EDITBOXLENGTH)
		nAllocateSize = GLUF_MAX_EDITBOXLENGTH;

	WCHAR* pTempBuffer = new (std::nothrow) WCHAR[nAllocateSize];
	if (!pTempBuffer)
		return false;

	ZeroMemory(pTempBuffer, sizeof(WCHAR) * nAllocateSize);

	if (m_pwszBuffer)
	{
		memcpy(pTempBuffer, m_pwszBuffer, m_nBufferSize * sizeof(WCHAR));
		delete[] m_pwszBuffer;
	}

	m_pwszBuffer = pTempBuffer;
	m_nBufferSize = nAllocateSize;
	return true;
}*/


//--------------------------------------------------------------------------------------
// Uniscribe -- Analyse() analyses the string in the buffer
//--------------------------------------------------------------------------------------
GLUFResult GLUFUniBuffer::Analyse()
{
	
	/*if (m_Analysis)
		(void)ScriptStringFree(&m_Analysis);*/

	//SCRIPT_CONTROL ScriptControl; // For uniscribe
	//SCRIPT_STATE ScriptState;   // For uniscribe
	//ZeroMemory(&ScriptControl, sizeof(ScriptControl));
	//ZeroMemory(&ScriptState, sizeof(ScriptState));

#pragma warning(push)
#pragma warning(disable : 4616 6309 6387 )
	//GLUFResult hr = ScriptApplyDigitSubstitution(nullptr, &ScriptControl, &ScriptState);
	//if (FAILED(hr))
	//	return hr;
#pragma warning(pop)

	if (!m_pFontNode)
		return GR_FAILURE;

	/*HDC hDC = nullptr;
	hr = ScriptStringAnalyse(hDC,
		m_pwszBuffer,
		(int)wcslen(m_pwszBuffer) + 1,  // nul is also analyzed.
		(int)wcslen(m_pwszBuffer) * 3 / 2 + 16,
		-1,
		SSA_BREAK | SSA_GLYPHS | SSA_FALLBACK | SSA_LINK,
		0,
		&ScriptControl,
		&ScriptState,
		nullptr,
		nullptr,
		nullptr,
		&m_Analysis);*/

	//get a list of x values, each one is the leading x value for each control
	/*m_CalcXValues.clear();


	//update the font size within FreeType
	FT_Set_Char_Size(m_pFontNode->m_pFontType->mFontFace, 0, m_pFontNode->mSize * 64, 96, 96);

	float currPos = 0.0f;
	for (unsigned int i = 0; i < m_Buffer.length(); ++i)
	{
		FT_Load_Char(m_pFontNode->m_pFontType->mFontFace, m_Buffer.length(), FT_LOAD_NO_BITMAP);
		float currValue = float(m_pFontNode->m_pFontType->mFontFace->glyph->advance.x / 64.0f);
		currValue /= m_pResMan->GetWindowSize().x;
		m_CalcXValues.push_back(currPos);
		currPos += currValue;
	}
	//now add the last edge
	m_CalcXValues.push_back(currPos);
	*/

	m_bAnalyseRequired = false;  // Analysis is up-to-date

	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------
GLUFUniBuffer::GLUFUniBuffer(GLUFDialogResourceManager *resMan, int nInitialSize) : m_pResMan(resMan)
{
	//m_nBufferSize = 0;
	m_Buffer = "";
	m_Buffer.resize(nInitialSize);
	m_bAnalyseRequired = true;
	//m_Analysis = nullptr;
	m_pFontNode = nullptr;

	//if (nInitialSize > 0)
	//	SetBufferSize(nInitialSize);
}


//--------------------------------------------------------------------------------------
GLUFUniBuffer::~GLUFUniBuffer()
{
	/*delete[] m_pwszBuffer;
	if (m_Analysis)
		(void)ScriptStringFree(&m_Analysis);*/
}



//--------------------------------------------------------------------------------------
void GLUFUniBuffer::Clear()
{
	//*m_pwszBuffer = L'\0';
	m_bAnalyseRequired = true;
	m_Buffer.clear();
}


//--------------------------------------------------------------------------------------
// Inserts the char at specified index.
// If nIndex == -1, insert to the end.
//--------------------------------------------------------------------------------------
bool GLUFUniBuffer::InsertChar( int nIndex,  char wChar)
{
	/*assert(nIndex >= 0);

	if (nIndex < 0 || nIndex >(int)wcslen(m_pwszBuffer))
		return false;  // invalid index

	// Check for maximum length allowed
	if (GetTextSize() + 1 >= GLUF_MAX_EDITBOXLENGTH)
		return false;

	if ((int)wcslen(m_pwszBuffer) + 1 >= m_nBufferSize)
	{
		if (!SetBufferSize(-1))
			return false;  // out of memory
	}

	assert(m_nBufferSize >= 2);

	// Shift the characters after the index, start by copying the null terminator
	WCHAR* dest = m_pwszBuffer + wcslen(m_pwszBuffer) + 1;
	WCHAR* stop = m_pwszBuffer + nIndex;
	WCHAR* src = dest - 1;

	while (dest > stop)
	{
		*dest-- = *src--;
	}

	// Set new character
	m_pwszBuffer[nIndex] = wChar;*/
	m_bAnalyseRequired = true;

	if ((m_Buffer.length() - 1) < nIndex)
	{
		m_Buffer.resize(nIndex + 1);
		m_Buffer[nIndex] = wChar;
	}
	else if (nIndex == -1)
	{
		m_Buffer += wChar;
	}
	else
	{
		m_Buffer.insert(nIndex, 1, wChar);
	}


	return true;
}


//--------------------------------------------------------------------------------------
// Removes the char at specified index.
// If nIndex == -1, remove the last char.
//--------------------------------------------------------------------------------------
bool GLUFUniBuffer::RemoveChar( int nIndex)
{
	m_bAnalyseRequired = true;
	if (nIndex > m_Buffer.length())
	{
		return false;
	}
	else
	{
		m_Buffer.erase(nIndex);
	}

	return true;
}


//--------------------------------------------------------------------------------------
// Inserts the first nCount characters of the string pStr at specified index.
// If nCount == -1, the entire string is inserted.
// If nIndex == -1, insert to the end.
//--------------------------------------------------------------------------------------

bool GLUFUniBuffer::InsertString(int nIndex, std::string str)
{
	/*assert(nIndex >= 0);
	if (nIndex < 0)
		return false;

	if (nIndex >(int)wcslen(m_pwszBuffer))
		return false;  // invalid index

	if (-1 == nCount)
		nCount = (int)wcslen(pStr);

	// Check for maximum length allowed
	if (GetTextSize() + nCount >= GLUF_MAX_EDITBOXLENGTH)
		return false;

	if ((int)wcslen(m_pwszBuffer) + nCount >= m_nBufferSize)
	{
		if (!SetBufferSize((int)wcslen(m_pwszBuffer) + nCount + 1))
			return false;  // out of memory
	}

	MoveMemory(m_pwszBuffer + nIndex + nCount, m_pwszBuffer + nIndex, sizeof(WCHAR) *
		(wcslen(m_pwszBuffer) - nIndex + 1));
	memcpy(m_pwszBuffer + nIndex, pStr, nCount * sizeof(WCHAR));*/
	m_bAnalyseRequired = true;

	if ((m_Buffer.length() - 1) < nIndex)
	{
		m_Buffer.resize(nIndex + 1 + str.length());
		m_Buffer.insert(nIndex, str);
	}
	else if (nIndex == -1)
	{
		m_Buffer += str.c_str();
	}
	else
	{
		m_Buffer.insert(nIndex, str);
	}

	return true;
}


//--------------------------------------------------------------------------------------
bool GLUFUniBuffer::SetText( std::string wszText)
{
	/*assert(wszText);

	size_t nRequired = wcslen(wszText) + 1;

	// Check for maximum length allowed
	if (nRequired >= GLUF_MAX_EDITBOXLENGTH)
		return false;

	while (GetBufferSize() < nRequired)
		if (!SetBufferSize(-1))
			break;
	// Check again in case out of memory occurred inside while loop.
	if (GetBufferSize() >= nRequired)
	{
		wcscpy_s(m_pwszBuffer, GetBufferSize(), wszText);
		m_bAnalyseRequired = true;
		return true;
	}
	else
		return false;*/

	m_bAnalyseRequired = true;

	m_Buffer.clear();
	m_Buffer = wszText;

	return true;
}


//--------------------------------------------------------------------------------------

GLUFResult GLUFUniBuffer::CPtoX(int nCP, bool bTrail, int* pX)
{
	GLUF_ASSERT(pX);
	*pX = 0;  // Default

	if (nCP + 1 > m_Buffer.length())
	{
		return GR_INVALIDARG;
	}

	GLUFResult hr = GR_SUCCESS;
	if (m_bAnalyseRequired)
		hr = Analyse();

	if (GLUF_FAILED(hr))
		return hr;
	

	if (bTrail)
	{
		nCP++;
	}

	*pX = m_CalcXValues[nCP];

	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------

GLUFResult GLUFUniBuffer::XtoCP(int nX, int* pCP, bool* pnTrail)
{
	GLUF_ASSERT(pCP && pnTrail);
	*pCP = 0; *pnTrail = false;  // Default

	GLUFResult hr = GR_SUCCESS;
	if (m_bAnalyseRequired)
		hr = Analyse();

	if (GLUF_FAILED(hr))
		return hr;

	//if (SUCCEEDED(hr))
	//{
		//hr = ScriptStringXtoCP(m_Analysis, nX, pCP, pnTrail);
		//if (FAILED(hr))
	//	{
	//		*pCP = 0; *pnTrail = FALSE;
	//		return false;
	//	}
	//}

	// If the coordinate falls outside the text region, we
	// can get character positions that don't exist.  We must
	// filter them here and convert them to those that do exist.
	/*if (*pCP == -1 && *pnTrail == TRUE)
	{
		*pCP = 0; *pnTrail = FALSE;
	}
	else if (*pCP > (int)wcslen(m_pwszBuffer) && *pnTrail == FALSE)
	{
		*pCP = (int)wcslen(m_pwszBuffer); *pnTrail = TRUE;
	}

	if (FAILED(hr))
	{
		*pCP = 0; *pnTrail = FALSE;
		return false;
	}*/

	unsigned int prev = 0;//the first element will ALWAYS be 0, so compensate
	unsigned int curr = 0;
	for (unsigned int i = 1; i < m_CalcXValues.size(); ++i)
	{
		curr = m_CalcXValues[i];

		if (nX < curr)
		{
			if ((curr - prev) / 2 < curr - nX)
			{
				//this would lead us to to trailing edge
				*pCP = i;
				*pnTrail = true;

				break;
			}
			else
			{
				*pCP = i - 1;
				*pnTrail = false;

				break;
			}
		}

		prev = curr;
	}

	return GR_SUCCESS;
}


//--------------------------------------------------------------------------------------

void GLUFUniBuffer::GetPriorItemPos(int nCP, int* pPrior)
{
	*pPrior = nCP;  // Default is the char itself

	if (m_bAnalyseRequired)
		if (GLUF_FAILED(Analyse()))
			return;

	/*const SCRIPT_LOGATTR* pLogAttr = ScriptString_pLogAttr(m_Analysis);
	if (!pLogAttr)
		return;

	if (!ScriptString_pcOutChars(m_Analysis))
		return;
	int nInitial = *ScriptString_pcOutChars(m_Analysis);
	if (nCP - 1 < nInitial)
		nInitial = nCP - 1;
	for (int i = nInitial; i > 0; --i)
		if (pLogAttr[i].fWordStop ||       // Either the fWordStop flag is set
			(!pLogAttr[i].fWhiteSpace &&  // Or the previous char is whitespace but this isn't.
			pLogAttr[i - 1].fWhiteSpace))
		{
		*pPrior = i;
		return;
		}
	// We have reached index 0.  0 is always a break point, so simply return it.
	*pPrior = 0;*/

	if (nCP = 0)
	{
		pPrior = 0;
		return;
	}

	*pPrior = m_CalcXValues[nCP - 1];
}


//--------------------------------------------------------------------------------------

void GLUFUniBuffer::GetNextItemPos(int nCP, int* pPrior)
{
	*pPrior = nCP;  // Default is the char itself

	/*GLUFResult hr = GR_SUCCESS;
	if (m_bAnalyseRequired)
		hr = Analyse();
	if (FAILED(hr))
		return;

	const SCRIPT_LOGATTR* pLogAttr = ScriptString_pLogAttr(m_Analysis);
	if (!pLogAttr)
		return;

	if (!ScriptString_pcOutChars(m_Analysis))
		return;
	int nInitial = *ScriptString_pcOutChars(m_Analysis);
	if (nCP + 1 < nInitial)
		nInitial = nCP + 1;

	int i = nInitial;
	int limit = *ScriptString_pcOutChars(m_Analysis);
	while (limit > 0 && i < limit - 1)
	{
		if (pLogAttr[i].fWordStop)      // Either the fWordStop flag is set
		{
			*pPrior = i;
			return;
		}
		else if (pLogAttr[i].fWhiteSpace &&  // Or this whitespace but the next char isn't.
			!pLogAttr[i + 1].fWhiteSpace)
		{
			*pPrior = i + 1;  // The next char is a word stop
			return;
		}

		++i;
		limit = *ScriptString_pcOutChars(m_Analysis);
	}
	// We have reached the end. It's always a word stop, so simply return it.
	*pPrior = *ScriptString_pcOutChars(m_Analysis) - 1;*/

	if (nCP == m_CalcXValues.size())
	{
		*pPrior = m_CalcXValues[nCP];
		return;
	}

	*pPrior = m_CalcXValues[nCP + 1];
}


//======================================================================================
// GLUF Text Operations
//======================================================================================

glm::mat4 g_TextOrtho;
glm::mat4 g_TextModelMatrix;

//--------------------------------------------------------------------------------------
void BeginText(glm::mat4 orthoMatrix)
{
	g_TextOrtho = orthoMatrix;
	g_TextVerticies.clear();
}


//--------------------------------------------------------------------------------------
void DrawTextGLUF(GLUFFontNode font, std::string strText, GLUFRect rcScreen, Color vFontColor, bool bCenter)
{
	//TODO: support centering (vertical AND horizontal, or both)
	//TODO: make another text method for text boxes (constraints i mean)

	//if (font.mSize > GLUFRectHeight(rcScreen))
	//	return;//no sense rendering if it is too big

	rcScreen = GLUFScreenToClipspace(rcScreen);

	g_TextModelMatrix = glm::translate(glm::mat4(), glm::vec3(rcScreen.left, rcScreen.top, 0.0f));

	int Row, Col;
	float U, V, U1, V1;

	float CurX = 0.0f;
	float CurY = 0.0f;

	//glBegin(GL_QUADS);
	float z = GLUF_NEAR_BUTTON_DEPTH;
	for (auto ch : strText)
	{
		float widthConverted = (font.m_pFontType->CellX * font.mSize) / font.m_pFontType->CellY;

		//lets support newlines :) (or if the nex char will go outside the rect)
		if (ch == '\n'/* || CurX + widthConverted > GLUFRectWidth(rcScreen)*/)
		{
			CurX = z;
			CurY += font.mSize * 1.1f;//assume a reasonible leding

			//if the next line will go off of the page, then don't draw it
			if (CurY + font.mSize > GLUFRectHeight(rcScreen))
				break;
		}


		Row = (ch - font.m_pFontType->Base) / font.m_pFontType->RowPitch;
		Col = (ch - font.m_pFontType->Base) - Row*font.m_pFontType->RowPitch;

		U = Col*font.m_pFontType->ColFactor;
		V = Row*font.m_pFontType->RowFactor;
		U1 = U + font.m_pFontType->ColFactor;
		V1 = V + font.m_pFontType->RowFactor;

		//glTexCoord2f(U, V1);  glVertex2i(CurX, CurY);
		//glTexCoord2f(U1, V1);  glVertex2i(CurX + font.m_pFontType->CellX, CurY);
		//glTexCoord2f(U1, V); glVertex2i(CurX + font.m_pFontType->CellX, CurY + font.m_pFontType->CellY);
		//glTexCoord2f(U, V); glVertex2i(CurX, CurY + font.m_pFontType->CellY);


		//triangle 1
		g_TextVerticies.push_back(
			glm::vec3(CurX, CurY, z),
			glm::vec2(U, V));
			//glm::vec2(z, 1.0f));

		g_TextVerticies.push_back(
			glm::vec3(CurX + widthConverted,CurY - font.mSize, z),
			glm::vec2(U1, V1));
			//glm::vec2(z, 1.0f));

		g_TextVerticies.push_back(
			glm::vec3(CurX + widthConverted, CurY, z),
			glm::vec2(U1, V));
			//glm::vec2(1.0f, 1.0f));

		//triangle 2
		g_TextVerticies.push_back(
			glm::vec3(CurX, CurY, z),
			glm::vec2(U, V));
			//glm::vec2(z, 1.0f));

		g_TextVerticies.push_back(
			glm::vec3(CurX, CurY - font.mSize, z),
			glm::vec2(U, V1));
			//glm::vec2(z, z));

		g_TextVerticies.push_back(
			glm::vec3(CurX + widthConverted, CurY - font.mSize, z),
			glm::vec2(U1, V1));
			//glm::vec2(1.0f, z));



		/*CurX += 0.05;

		//triangle 1
		g_TextVerticies.push_back(
			glm::vec3(CurX, CurY, z),
			glm::vec2(U, V));
		//glm::vec2(z, 1.0f));

		g_TextVerticies.push_back(
			glm::vec3(CurX + widthConverted, CurY - font.mSize, z),
			glm::vec2(U1, V1));
		//glm::vec2(z, 1.0f));

		g_TextVerticies.push_back(
			glm::vec3(CurX + widthConverted, CurY, z),
			glm::vec2(U1, V));
		//glm::vec2(1.0f, 1.0f));

		//triangle 2
		g_TextVerticies.push_back(
			glm::vec3(CurX, CurY, z),
			glm::vec2(U, V));
		//glm::vec2(z, 1.0f));

		g_TextVerticies.push_back(
			glm::vec3(CurX, CurY - font.mSize, z),
			glm::vec2(U, V1));
		//glm::vec2(z, z));

		g_TextVerticies.push_back(
			glm::vec3(CurX + widthConverted, CurY - font.mSize, z),
			glm::vec2(U1, V1));
		//glm::vec2(1.0f, z));*/


		CurX += ((float)font.m_pFontType->Width[ch] * font.mSize) / font.m_pFontType->CellY;

		z += 0.000001f;//to solve the depth problem
	}
	//glEnd();
	
	g_TextVerticies.set_color(vFontColor);

	EndText(font.m_pFontType);

}


void EndText(GLUFFontPtr font)
{
	
	//get the currently bound texture to rebind later
	//GLint tmpTexId;
	//glGetIntegerv(GL_TEXTURE_BINDING_2D, &tmpTexId);
	//GLUF_ASSERT(tmpTexId >= 0);

	//buffer the data
	glBindVertexArray(g_TextVAO);
	glBindBuffer(GL_ARRAY_BUFFER, g_TextPos);
	glBufferData(GL_ARRAY_BUFFER, g_TextVerticies.size() * sizeof(glm::vec3), g_TextVerticies.data_pos(), GL_STREAM_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, g_TextTexCoords);
	glBufferData(GL_ARRAY_BUFFER, g_TextVerticies.size() * sizeof(glm::vec2), g_TextVerticies.data_tex(), GL_STREAM_DRAW);

	GLUFSHADERMANAGER.UseProgram(g_TextProgram);

	//first uniform: model-view matrix
	glm::mat4 mv = g_TextOrtho * g_TextModelMatrix;
	glUniformMatrix4fv(0, 1, GL_FALSE, &mv[0][0]);

	//second, the sampler
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, font->TexID);
	glUniform1f(1, 0);

	//third, the color
	Color4f color = g_TextVerticies.get_color();
	glUniform4f(2, color.r, color.g, color.b, color.a);

	glEnableVertexAttribArray(0);//positions
	glEnableVertexAttribArray(1);//uvs

	glDrawArrays(GL_TRIANGLES, 0, g_TextVerticies.size());

	g_TextVerticies.clear();
	glBindVertexArray(0);

	//lastly, rebind the old texture
	//glBindTexture(GL_TEXTURE_BINDING_2D, tmpTexId);
}
