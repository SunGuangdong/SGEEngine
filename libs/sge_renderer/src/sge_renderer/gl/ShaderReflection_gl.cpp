#include <sstream>

#include "GraphicsCommon_gl.h"
#include "GraphicsInterface_gl.h"
#include "ShadingProgram_gl.h"

#include "sge_renderer/renderer/ShaderReflection.h"

namespace sge {

BindLocation NumericUniformRefl::computeBindLocation() const {
	return BindLocation((short)bindLocation, (short)uniformType, (short)arraySize, 0);
}

BindLocation CBufferRefl::computeBindLocation() const {
	return BindLocation((short)gl_bindLocation, (short)UniformType::ConstantBuffer, 1, 0);
}

BindLocation TextureRefl::computeBindLocation() const {
#if !defined(__EMSCRIPTEN__)
	if (gl_textureTarget == GL_TEXTURE_1D) {
		return BindLocation((short)gl_bindLocation, (short)UniformType::Texture1D, short(arraySize), short(gl_bindUnit));
	}
#endif

	if (gl_textureTarget == GL_TEXTURE_2D) {
		return BindLocation((short)gl_bindLocation, (short)UniformType::Texture2D, short(arraySize), short(gl_bindUnit));
	}

	if (gl_textureTarget == GL_TEXTURE_CUBE_MAP) {
		return BindLocation((short)gl_bindLocation, (short)UniformType::TextureCube, short(arraySize), short(gl_bindUnit));
	}

	if (gl_textureTarget == GL_TEXTURE_3D) {
		return BindLocation((short)gl_bindLocation, (short)UniformType::Texture3D, short(arraySize), short(gl_bindUnit));
	}

	sgeAssert(false);
	return BindLocation();
}

BindLocation SamplerRefl::computeBindLocation() const {
	// There are no sampler in OpenGL.
	sgeAssert(false);
	return BindLocation();
}

//---------------------------------------------------------------
// ShaderRefl
//---------------------------------------------------------------
bool ShadingProgramRefl::create(ShadingProgram* const shadingProgram) {
	if (!shadingProgram || !shadingProgram->isValid()) {
		return false;
	}

	SGEDevice* const device = shadingProgram->getDevice();
	ShadingProgramGL* const shadingProgramGL = static_cast<ShadingProgramGL*>(shadingProgram);
	GLuint const program = shadingProgramGL->GL_GetProgram();

	const int MAX_NAME_LEN = 256;

	GLint numUniforms = 0;
	glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &numUniforms);
	GLint numUniformBlocks = 0;
	glGetProgramiv(program, GL_ACTIVE_UNIFORM_BLOCKS, &numUniformBlocks);

	int texBindUnit = 1;

	// Obtain the uniforms needed by the program.
	for (GLint t = 0; t < numUniforms; ++t) {
		char name[MAX_NAME_LEN + 3];
		GLint nameLen = 0;
		GLint size = 0;
		GLenum gl_type = SGE_GL_UNKNOWN;

		// Get the uniform name, bindLocation and type.
		glGetActiveUniform(program, t, MAX_NAME_LEN, &nameLen, &size, &gl_type, name);
		DumpAllGLErrors();
		sgeAssert(size >= 1);
		const GLint bindLocation = glGetUniformLocation(program, name);
		DumpAllGLErrors();

		// If the bind location is equal to -1 than the uniform isn't used.
		// https://www.opengl.org/wiki/Program_Introspection#Uniforms_and_blocks
		if (bindLocation < 0) {
			continue;
		}

		// Caution:
		// Different OpenGL implementations report OpenGL array uniforms differently.
		// For example on my current Intel HD, a single array is reported by:
		//     myUniformArray[0] myUniformArray[1] ... // myUniformArray[N] : One for every element, however the [0] has the correct num
		//     elements returned.
		// On other platforms it is just:
		//    myUniformArray
		bool isNameLookingLikeArray = false;
		if (nameLen > 3) {
			isNameLookingLikeArray = name[nameLen - 1] == ']';
			if (isNameLookingLikeArray && name[nameLen - 2] != '0') {
				continue;
			}
		}

		// Because of the array naming inconsistency we need to use that:
		std::string_view nameToUse =
		    (isNameLookingLikeArray)
		        ? nameToUse = std::string_view(name, std::max(nameLen - 3, 0)) // drop the [0] to be consistent with Direct 3D reflections.
		        : nameToUse = std::string_view(name);

		if (UniformType_FromGLNumericUniformType(gl_type) != UniformType::Unknown) {
			NumericUniformRefl uniform;

			uniform.name = nameToUse;
			uniform.nameStrIdx = device->getStringIndex(uniform.name);
			uniform.uniformType = UniformType_FromGLNumericUniformType(gl_type);
			uniform.bindLocation = bindLocation;
			uniform.arraySize = size;

			numericUnforms.add(uniform);
		}
		// then should be a texture
		else if (GL_NONE != GLUniformTypeToTextureType(gl_type)) {
			TextureRefl texture;

			texture.name = nameToUse;
			texture.nameStrIdx = device->getStringIndex(name);
			texture.gl_bindLocation = bindLocation;
			texture.gl_bindUnit = texBindUnit;
			texBindUnit += 1;
			texture.gl_textureTarget = GLUniformTypeToTextureType(gl_type);

			texture.arraySize = size;

			textures.add(texture);
		} else {
			// unknown uniform type
			sgeAssert(false);
		}
	}

	// Load the uniform buffers(cbuffers).
	for (GLint t = 0; t < numUniformBlocks; ++t) {
		// Uniform buffer name.
		char name[MAX_NAME_LEN];
		GLint nameLen = 0;
		glGetActiveUniformBlockName(program, t, MAX_NAME_LEN, &nameLen, name);

		// Obtain the uniform block size in bytes
		GLint blockSizeBytes = 0;
		glGetActiveUniformBlockiv(program, t, GL_UNIFORM_BLOCK_DATA_SIZE, &blockSizeBytes);

		// Todo: Add support for custom bind locations.
		GLint bindLocation = t;
		glUniformBlockBinding(program, bindLocation, bindLocation);

		// Obtain the number of variables in the block.
		GLint numVariableInBlock = 0;
		glGetActiveUniformBlockiv(program, t, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &numVariableInBlock);

		// Obtain the variables that are used in the uniform buffer.
		std::vector<GLint> varIndices(numVariableInBlock);
		std::vector<GLint> varTypes(numVariableInBlock);
		std::vector<GLint> varOffsets(numVariableInBlock);
		std::vector<GLint> varArrSize(numVariableInBlock);

		glGetActiveUniformBlockiv(program, t, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, varIndices.data());
		glGetActiveUniformsiv(program, numVariableInBlock, (GLuint*)varIndices.data(), GL_UNIFORM_TYPE, varTypes.data());
		glGetActiveUniformsiv(program, numVariableInBlock, (GLuint*)varIndices.data(), GL_UNIFORM_OFFSET, varOffsets.data());
		glGetActiveUniformsiv(program, numVariableInBlock, (GLuint*)varIndices.data(), GL_UNIFORM_SIZE, varArrSize.data());

		CBufferRefl cbuffer;

		cbuffer.name = name;
		cbuffer.nameStrIdx = device->getStringIndex(cbuffer.name);
		cbuffer.sizeBytes = blockSizeBytes;
		cbuffer.gl_bindLocation = bindLocation;

		// Read the variables in the uniform block.
		for (GLint v = 0; v < numVariableInBlock; ++v) {
			char varName[MAX_NAME_LEN];
#if !defined(__EMSCRIPTEN__)
			// TODO try emscripten_glGetActiveUniformName
			glGetActiveUniformName(program, varIndices[v], MAX_NAME_LEN, NULL, varName);
#else
			sprintf(varName, "emsc_var%d", v);
#endif


			CBufferVariableRefl var;
			var.name = varName;
			var.nameStrIdx = device->getStringIndex(var.name);
			var.arraySize = varArrSize[v];
			var.offset = varOffsets[v];
			var.type = UniformType_FromGLNumericUniformType(varTypes[v]);

			cbuffer.variables.push_back(var);
		}

		cbuffers.add(cbuffer);
	}

	// Read vertex shader attributes.
	GLint numVSAttribs = 0;
	glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &numVSAttribs);

	for (GLint t = 0; t < numVSAttribs; ++t) {
		char attribName[MAX_NAME_LEN];
		GLint nameLen = 0, size;
		GLenum gl_type = SGE_GL_UNKNOWN;

		// Get the attribute name, type, bind location.
		glGetActiveAttrib(program, t, MAX_NAME_LEN, &nameLen, &size, &gl_type, attribName);

		// Convert it to sge compatible uniform/attribute type.
		const UniformType::Enum uniformType = UniformType_FromGLNumericUniformType(gl_type);

		// Only numeric types are possible.
		const GLint bindLocation = glGetAttribLocation(program, attribName);

		VertShaderAttrib attrib;
		attrib.name = attribName;
		attrib.type = uniformType;
		attrib.attributeLocation = bindLocation;

		inputVertices.push_back(attrib);
	}

	return true;
}

} // namespace sge
