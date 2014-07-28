// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the OBJGLUF_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// OBJGLUF_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#pragma once

#pragma warning (disable : 4251)

#ifdef OBJGLUF_EXPORTS
#define OBJGLUF_API __declspec(dllexport)
#else
#define OBJGLUF_API __declspec(dllimport)
#endif


#include <GL/glew.h>
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/quaternion.hpp>

#include <vector>
#include <memory>
#include <map>
#include <list>
#include <string>

#include "GLUFWindow.h"

#ifndef OBJGLUF_EXPORTS
#ifndef SUPPRESS_RADIAN_ERROR
#error "GLM is using radians as input, to suppress this error, #define SUPPRESS_RADIAN_ERROR"
#endif
#endif

#ifdef USING_ASSIMP
inline glm::vec2 AssimpToGlm(aiVector2D v)
{
	return glm::vec2(v.x, v.y);
}

inline glm::vec3 AssimpToGlm(aiVector3D v)
{
	return glm::vec3(v.x, v.y, v.z);
}
inline glm::vec3 AssimpToGlm(aiColor3D v)
{
	return glm::vec3(v.r, v.g, v.b);
}

inline glm::vec4 AssimpToGlm(aiColor4D v)
{
	return glm::vec4(v.r, v.g, v.b, v.a);
}

#endif


typedef void(*GLUFErrorMethod)(const char* message, const char* funcName, const char* sourceFile, unsigned int lineNum);

void OBJGLUF_API GLUFRegisterErrorMethod(GLUFErrorMethod method);
GLUFErrorMethod OBJGLUF_API GLUFGetErrorMethod();

#define GLUF_ERROR(message) GLUFGetErrorMethod()(message, __FUNCTION__, __FILE__, __LINE__);
#define GLUF_ASSERT(statement)	if(!statement){GLUF_ERROR("Assertion Error!");}

typedef glm::u8vec4 Color;//only accepts numbers from 0 to 255
typedef glm::vec3 Color3f;




//a little treat for initializing streambuf's with existing data
struct OBJGLUF_API MemStreamBuf : public std::streambuf
{
	MemStreamBuf(char* data, std::ptrdiff_t length)
	{
		setg(data, data, data + length);
	}
};



#define GLUF_UNIVERSAL_TRANSFORM_UNIFORM_BLOCK_LOCATION 0
//#define GLUF_UNIVERSAL_VERTEX_POSITION_LOCATION 0
//#define GLUF_UNIVERSAL_VERTEX_NORMAL_LOCATION 1
//#define GLFU_UNIVERSAL_VERTEX_UVCOORD_LOCATION 2

#define GLUF_PI (double)3.14159265358979
#define GLUF_PI_F (float)3.14159265358979

#define DEG_TO_RAD(value) (value *(GLUF_PI / 180))
#define DEG_TO_RAD_F(value) (value *(GLUF_PI_F / 180))



typedef std::vector<glm::vec4> Vec4Array;
typedef std::vector<glm::vec3> Vec3Array;
typedef std::vector<glm::vec2> Vec2Array;
typedef std::vector<GLushort>  IndexArray;

#include <stack>

class OBJGLUF_API GLUFMatrixStack
{
	std::stack<glm::mat4> mStack;
public:
	void Push(const glm::mat4& matrix);
	void Pop(void);
	const glm::mat4& Top();//this returns the matrix that is a concatination of all subsequent matrices
	size_t Size();
	void Empty();
};





enum GLUFShaderType
{
	SH_VERTEX_SHADER = GL_VERTEX_SHADER,
	SH_TESS_CONTROL_SHADER = GL_TESS_CONTROL_SHADER,
	SH_TESS_EVALUATION_SHADER = GL_TESS_EVALUATION_SHADER,
	SH_GEOMETRY_SHADER = GL_FRAGMENT_SHADER,
	SH_FRAGMENT_SHADER = GL_FRAGMENT_SHADER
};

enum GLUFShaderInputVertexDataType
{
	IN_POSITIONS = 0,
	IN_NORMALS = 1,
	IN_UVCOORDS = 2
};

enum GLUFShaderUniformTransformMatrixType
{
	IN_M = 4,
	IN_V = 3,
	IN_P = 2,
	IN_MV = 1,
	IN_MVP = 0
};

struct OBJGLUF_API GLUFShaderInfoStruct
{
	bool mSuccess = false;

	std::vector<char> mLog;

	operator bool() const
	{
		return mSuccess;
	}
};

typedef GLUFShaderInfoStruct GLUFCompileOutputStruct;
typedef GLUFShaderInfoStruct GLUFLinkOutputStruct;


//make all of the classes opaque
class OBJGLUF_API GLUFShader;
class OBJGLUF_API GLUFComputeShader;
class OBJGLUF_API GLUFProgram;
class OBJGLUF_API GLUFSeperateProgram;

typedef std::shared_ptr<GLUFShader>  GLUFShaderPtr;
typedef std::weak_ptr<GLUFShader>    GLUFShaderPtrWeak;
typedef std::shared_ptr<GLUFProgram> GLUFProgramPtr;
typedef std::weak_ptr<GLUFProgram>   GLUFProgramPtrWeak;

typedef std::map<GLUFShaderType, const char*> GLUFShaderSourceList;
typedef std::map<GLUFShaderType, std::string> GLUFShaderPathList;//do a little bit of fudging to get around this being the same as the above
typedef std::vector<GLuint> GLUFShaderIdList;
typedef std::vector<GLuint> GLUFProgramIdList;
typedef std::vector<std::string> GLUFShaderNameList;
typedef std::vector<std::string> GLUFProgramNameList;
typedef std::vector<GLUFShaderPtr> GLUFShaderPtrList;
typedef std::vector<GLUFProgramPtr> GLUFProgramPtrList;
typedef std::vector<GLUFShaderPtrWeak> GLUFShaderPtrListWeak;
typedef std::vector<GLUFProgramPtrWeak> GLUFProgramPtrListWeak;


class OBJGLUF_API GLUFShaderManager
{

	//a list of logs
	std::map<GLUFShaderPtr, GLUFShaderInfoStruct> mCompileLogs;
	std::map<GLUFProgramPtr, GLUFShaderInfoStruct> mLinklogs;

	//a little helper function for creating things
	GLUFShaderPtr CreateShader(std::string shad, GLUFShaderType type, bool file);

	static GLUFProgramPtr m_pCurrentProgram;

	static GLuint GetCurrProgramPtr();

	friend class GLUFBufferManager;

public:

	//for creating things

	GLUFShaderPtr CreateShaderFromFile(std::string filePath, GLUFShaderType type);
	GLUFShaderPtr CreateShaderFromMemory(const char* text, GLUFShaderType type);

	GLUFProgramPtr CreateProgram(GLUFShaderPtrList shaders);
	GLUFProgramPtr CreateProgram(GLUFShaderSourceList shaderSources);
	GLUFProgramPtr CreateProgram(GLUFShaderPathList shaderPaths);


	//for removing things

	void DeleteShader(GLUFShaderPtr shader);
	void DeleteProgram(GLUFProgramPtr program);

	void FlushSavedShaders();
	void FlushSavedPrograms();
	void FlushLogs();


	//for accessing things
	const GLuint	  GetShaderId(GLUFShaderPtr shader) const;
	const GLUFShaderType  GetShaderType(GLUFShaderPtr shader) const;
	const GLuint	  GetProgramId(GLUFProgramPtr program) const;

	const GLUFCompileOutputStruct GetShaderLog(GLUFShaderPtr shaderPtr) const;

	const GLUFLinkOutputStruct GetProgramLog(GLUFProgramPtr programPtr) const;


	//for using things

	void UseProgram(GLUFProgramPtr program);
	void UseProgramNull();


};

#define ShaderManager static_pointer_cast<GLRenderer>(QuicksandEngine::g_pApp->m_Renderer)->mShaderManager


/*
Usage examples


//create the shader
GLUFShaderPtr shad = CreateShader("shader.glsl", ST_VERTEX_SHADER);



*/


//typedef GLuint GLBuffer;
//typedef GLuint GLVertexBuffer;
//typedef GLuint GLIndexBuffer;

enum GLUFTransformUniformType
{
	UT_MODEL = 0,
	UT_VIEW = 1,
	UT_PROJ = 2,
	UT_MODELVIEW = 3,
	UT_MVP = 4
};

//this matches with the layout location of them in the shader
enum GLUFVertexAttributeType
{
	VAO_POSITIONS = 0,
	VAO_NORMALS = 1,
	VAO_TEXCOORDS = 2,
	VAO_INDICIES = 3,
};
/*
struct GLUFMatrixTransformBlockPtrs
{
protected:
	friend class GLUFMatrixTransformBlock;
	glm::mat4 *pM;
	glm::mat4 *pV;
	glm::mat4 *pP;
	glm::mat4 *pMV;
	glm::mat4 *pMVP;
public:

	GLUFMatrixTransformBlockPtrs(glm::mat4 *m, glm::mat4 *v, glm::mat4 *p, glm::mat4 *mv, glm::mat4 *mvp) : pM(new glm::mat4(*m)), pV(new glm::mat4(*v)), pP(new glm::mat4(*p)), pMV(new glm::mat4(*mv)), pMVP(new glm::mat4(*mvp))
	{}
	~GLUFMatrixTransformBlockPtrs(){ delete pM, pV, pP, pMV, pMVP; }

	bool ModifyValue(GLUFTransformUniformType type, glm::mat4 data)
	{
		switch (type)
		{
		case UT_MODEL:
			*pM = data;
			*pMV = *pV * *pM;
			*pMVP = *pP * *pMV;
		case UT_VIEW:
			*pV = data;
			*pMV = *pV * *pM;
			*pMVP = *pP * *pMV;
		case UT_PROJ:
			*pP = data;
			*pMVP = *pP * *pMV;
		default:
			GLUF_ERROR("Cannot modify MV or MVP data directly");
			return false;
		}
		return true;
	}

	glm::mat4 operator [](unsigned short index)
	{
		switch (index)
		{
		case UT_MODEL:
			return *pM;
		case UT_VIEW:
			return *pV;
		case UT_PROJ:
			return *pP;
		case UT_MODELVIEW:
			return *pMV;
		case UT_MVP:
			return *pMVP;
		default:
			return glm::mat4();
		}
	}
};*/

struct OBJGLUF_API GLUFMatrixTransformBlockParam
{
protected:
	friend struct GLUFMatrixTransformBlock;
	friend class GLUFUniformBuffer;
	friend class GLUFBufferManager;
	glm::mat4 pM;
	glm::mat4 pV;
	glm::mat4 pP;
	glm::mat4 pMV;
	glm::mat4 pMVP;
public:

	GLUFMatrixTransformBlockParam(glm::mat4 m = glm::mat4(), glm::mat4 v = glm::mat4(), glm::mat4 p = glm::mat4()) : pM(m), pV(v), pP(p), pMV(v * m), pMVP(p * v * m){}
};

struct OBJGLUF_API GLUFMatrixTransformBlock
{
protected:
	//friend GLUFMatrixTransformBlockPtrs;
	friend class GLUFUniformBuffer;
	friend GLUFBufferManager;
	glm::mat4* pM;
	glm::mat4* pV;
	glm::mat4* pP;
	glm::mat4* pMV;
	glm::mat4* pMVP;

	GLUFMatrixTransformBlock() : 
		pM(new glm::mat4()), pV(new glm::mat4()), pP(new glm::mat4()), pMV(new glm::mat4()), pMVP(new glm::mat4())//possibly make this more efficient
	{}
public:


	GLUFMatrixTransformBlock& operator=(GLUFMatrixTransformBlock other)
	{
		pM = other.pM;
		pV = other.pV;
		pP = other.pP;

		pMV = other.pMV;
		pMVP = other.pMVP;
		return *this;
	}

	GLUFMatrixTransformBlock& operator=(GLUFMatrixTransformBlockParam other)
	{
		*pM = other.pM;
		*pV = other.pV;
		*pP = other.pP;

		*pMV = other.pMV;
		*pMVP = other.pMVP;
		return *this;
	}

	
	~GLUFMatrixTransformBlock()
	{
		pM = pV = pP = pMV = pMVP = nullptr;
	}

	void TrueDelete(){ delete pM, pV, pP, pMV, pMVP; pM = pV = pP = pMV = pMVP = nullptr; }

	bool ModifyValue(GLUFTransformUniformType type, glm::mat4 data)
	{
		switch (type)
		{
		case UT_MODEL:
			*pM = data;
			*pMV = *pV * *pM;
			*pMVP = *pP * *pMV;
			break;
		case UT_VIEW:
			*pV = data;
			*pMV = *pV * *pM;
			*pMVP = *pP * *pMV;
			break;
		case UT_PROJ:
			*pP = data;
			*pMVP = *pP * *pMV;
			break;
		default:
			GLUF_ERROR("Cannot modify MV or MVP data directly");
			return false;
		}
		return true;
	}

	glm::mat4 operator [](unsigned short index)
	{
		switch (index)
		{
		case UT_MODEL:
			return *pM;
		case UT_VIEW:
			return *pV;
		case UT_PROJ:
			return *pP;
		case UT_MODELVIEW:
			return *pMV;
		case UT_MVP:
			return *pMVP;
		default:
			return glm::mat4();
		}
	}
};

struct OBJGLUF_API GLUFVAOData
{
	Vec3Array* mPositions;
	Vec3Array* mNormals;
	Vec2Array* mUVCoords;
	IndexArray* mIndicies;

	GLUFVAOData(Vec3Array* positions, Vec3Array* normals, Vec2Array* uvCoords, IndexArray* indicies) : mPositions(positions), mNormals(normals), mUVCoords(uvCoords), mIndicies(indicies){}

	~GLUFVAOData(){ mPositions = nullptr; mNormals = nullptr; mUVCoords = nullptr; mIndicies = nullptr; }
}; 


struct OBJGLUF_API GLUFUniformBlockStandardLayoutData
{
	GLushort mBinding;
	const char* mName;//if this is NOT NULL then it is assumed that this string should be used

	GLubyte* m_pData;//this should be pre-aligned to the layout in use
};

//WORK IN PROGRESS
struct GLUFUniformBlockSharedLayoutData
{
	GLushort mBinding;
	const char* mName;//this is the name of the uniform block, if this is not null, then this will be used to retreive the binding point

	const char** mVariableNames;//This is an array of strings in the format $(UniformBlockName).$(AttributeName) in the order they should be assigned
	
	GLuint  *m_pLengths;//this is a list of lengths for each element in the m_pData array;
	GLubyte *m_pData;//This is the data that is in order
};

class OBJGLUF_API GLUFUniformBuffer;
class OBJGLUF_API GLUFVertexArrayObject;
class OBJGLUF_API GLUFTextureBuffer;


typedef std::weak_ptr<GLUFUniformBuffer> GLUFUniformBufferPtrWeak;
typedef std::weak_ptr<GLUFVertexArrayObject> GLUFVertexArrayPtrWeak;
typedef std::weak_ptr<GLUFTextureBuffer> GLUFTexturePtrWeak;

typedef std::shared_ptr<GLUFUniformBuffer> GLUFUniformBufferPtr;
typedef std::shared_ptr<GLUFVertexArrayObject> GLUFVertexArrayPtr;
typedef std::shared_ptr<GLUFTextureBuffer> GLUFTexturePtr;

class OBJGLUF_API GLUFMaterial
{
protected:
	Color mDiffuse, mAmbient, mSpecular, mEmissive;
	GLfloat mPower;

	GLUFTexturePtr m_pTexture;

public:
	void SetDiffuse(Color diffuse){ mDiffuse = diffuse; }
	Color GetDiffuse(){ return mDiffuse; }

	void SetAmbient(Color ambient){ mAmbient = ambient; }
	Color GetAmbient(){ return mAmbient; }

	void SetSpecular(Color specular, GLfloat power){ mSpecular = specular; mPower = power; }
	Color GetSpecular(){ return mSpecular; }
	GLfloat GetPower(){ return mPower; }

	void SetEmissive(Color emissive){ mEmissive = emissive; }
	Color GetEmissive(){ return mEmissive; }

	void SetTexture(GLUFTexturePtr texture){ m_pTexture = texture; }
	GLUFTexturePtr GetTexture(){ return m_pTexture; }

};

typedef std::shared_ptr<GLUFMaterial> GLUFMaterialPtr;

enum GLUFTextureFileFormat
{
	TFF_DDS = 0//,
	//TFF_PNG = 1,
	//TFF_JPG = 2
};

//this is a similar system to the shaders system
class OBJGLUF_API GLUFBufferManager
{
	//std::map<std::string, GLUniformBufferPtrWeak> mUniformBuffers;
	//std::map<std::string, GLVertexArrayPtrWeak> mVertexArrayBuffers;
	//friend class DdsResourceLoader;

	//GLTexturePtr CreateTexture(GLuint glTexId);

	//friend class Scene;

	GLUFUniformBufferPtr m_pBoundUniformBuffer;

public:


	void ResetBufferBindings();//this is used to "unuse" the bound textures and buffers after done drawing

	GLUFVertexArrayPtr CreateVertexArray();
	GLUFUniformBufferPtr CreateUniformArray();

	void DeleteVertexBuffer(GLUFVertexArrayPtr vertArray);
	void DeleteUniformBuffer(GLUFUniformBufferPtr buffer);

	void DrawVertexArray(GLUFVertexArrayPtr vertArray, GLenum topology = GL_TRIANGLES);
	void DrawVertexArrayInstanced(GLUFVertexArrayPtr vertArray, GLuint instanceCount, GLenum topology = GL_TRIANGLES);

	GLuint GetUniformTransformBufferId(GLUFUniformBufferPtr buffer);

	void BindUniformArray(GLUFUniformBufferPtr buffer);

	void ModifyVertexArray(GLUFVertexArrayPtr vertArray, GLUFVertexAttributeType type, Vec3Array data);
	void ModifyVertexArray(GLUFVertexArrayPtr vertArray, GLUFVertexAttributeType type, Vec2Array data);
	void MoidfyVertexIncidies(GLUFVertexArrayPtr vertArray, IndexArray data);
	void ModifyVertexArray(GLUFVertexArrayPtr vertArray, Vec3Array positions, Vec3Array normals, Vec2Array texCoords, IndexArray indicies);

	GLUFMatrixTransformBlock MapUniformTransform(GLUFUniformBufferPtr buffer);
	void UnMapUniformTransform(GLUFUniformBufferPtr buffer);

	GLUFVAOData MapVertexArray(GLUFVertexArrayPtr vertArray);
	void        UnMapVertexArray(GLUFVertexArrayPtr vertArray);

	//this is for modifying standard things such as transforms, or material.
	void ModifyUniformTransformMatrix(GLUFUniformBufferPtr buffer, GLUFTransformUniformType type, glm::mat4 data);//the transform uniform block that is standard to all shaders will always be location of 1
	void ModifyUniformTransformMatrix(GLUFUniformBufferPtr buffer, GLUFMatrixTransformBlockParam data);//this REQUIRES all matrices to exist
	void ModifyUniformMaterial(GLUFUniformBufferPtr buffer, GLUFMaterial mat){};//todo
	void ModifyLighting(GLUFUniformBufferPtr buffer){ };//todo

	//for non-generic circumstances
	void ModifyUniform1f(GLUFUniformBufferPtr buffer, GLuint location, float data){};
	void ModifyUniform1d(GLUFUniformBufferPtr buffer, GLuint location, double data){};
	void ModifyUniform1i(GLUFUniformBufferPtr buffer, GLuint location, int data){};
	void ModifyUniform1ui(GLUFUniformBufferPtr buffer, GLuint location, unsigned int data){};

	void ModifyUniform2f(GLUFUniformBufferPtr buffer, GLuint location, glm::vec2 data){};
	void ModifyUniform2d(GLUFUniformBufferPtr buffer, GLuint location, glm::dvec2 data){};
	void ModifyUniform2i(GLUFUniformBufferPtr buffer, GLuint location, glm::ivec2 data){};

	void ModifyUniform3f(GLUFUniformBufferPtr buffer, GLuint location, glm::vec3 data){};
	void ModifyUniform3d(GLUFUniformBufferPtr buffer, GLuint location, glm::dvec3 data){};
	void ModifyUniform3i(GLUFUniformBufferPtr buffer, GLuint location, glm::ivec3 data){};

	void ModifyUniform4f(GLUFUniformBufferPtr buffer, GLuint location, glm::vec4 data){};
	void ModifyUniform4d(GLUFUniformBufferPtr buffer, GLuint location, glm::dvec4 data){};
	void ModifyUniform4i(GLUFUniformBufferPtr buffer, GLuint location, glm::ivec4 data){};


	void ModifyUniformMatrix3f(GLUFUniformBufferPtr buffer, GLuint location, glm::mat3 data){};
	void ModifyUniformMatrix3d(GLUFUniformBufferPtr buffer, GLuint location, glm::dmat3 data){};
	void ModifyUniformMatrix4f(GLUFUniformBufferPtr buffer, GLuint location, glm::mat4 data){};
	void ModifyUniformMatrix4d(GLUFUniformBufferPtr buffer, GLuint location, glm::dmat4 data){};

	//TODO: add support for custom uniform blocks
	//WORK IN PROGRESS
	void ModifyUniformBlock(GLUFUniformBufferPtr buffer, GLUFUniformBlockStandardLayoutData param){};


	//texture methods
	GLUFTexturePtr CreateTextureBuffer();

	void UseTexture(GLUFTexturePtr texture);

	//NOTE: call CreateTextureBuffer() FIRST
	void LoadTextureFromFile(GLUFTexturePtr texture, std::string filePath, GLUFTextureFileFormat format);

	//NOTE: call CreateTextureBuffer() FIRST
	void LoadTextureFromMemory(GLUFTexturePtr texture, char* data, uint64_t length, GLUFTextureFileFormat format);

	//void BufferTexture(GLUFTexturePtr texture, GLsizei length, GLvoid* data){};

};
