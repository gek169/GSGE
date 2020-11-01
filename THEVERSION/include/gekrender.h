
// Welcome to My Renderer.

/*
License for Your use of this project:
~~~~~
Copyright 2018 DMHSW

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
==================Licenses for code included and used in this
project:=================================
~~~~~~~~~~~~~~~
stb_image and the entire stb library was Public Domain code.

~~~~~~~~~~~~~~~~~~~~~~~~~~~
Copyright 2014 TheBennyBox

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ License for Gl3
** Copyright (c) 2007-2012 The Khronos Group Inc.
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and/or associated documentation files (the
** "Materials"), to deal in the Materials without restriction, including
** without limitation the rights to use, copy, modify, merge, publish,
** distribute, sublicense, and/or sell copies of the Materials, and to
** permit persons to whom the Materials are furnished to do so, subject to
** the following conditions:
**
** The above copyright notice and this permission notice shall be included
** in all copies or substantial portions of the Materials.
**
** THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
** EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
** MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
** IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
** CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
** TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
** MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~License for GLFW~~~
Copyright © 2002-2006 Marcus Geelnard

Copyright © 2006-2011 Camilla Berglund

This software is provided ‘as-is’, without any express or implied warranty. In
no event will the authors be held liable for any damages arising from the use of
this software.

Permission is granted to anyone to use this software for any purpose, including
commercial applications, and to alter it and redistribute it freely, subject to
the following restrictions:

1. The origin of this software must not be misrepresented; you must not claim
that you wrote the original software. If you use this software in a product, an
acknowledgment in the product documentation would be appreciated but is not
required.

2. Altered source versions must be plainly marked as such, and must not be
misrepresented as being the original software.

3. This notice may not be removed or altered from any source distribution.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~License for GLM~~
Copyright (c) 2005 - 2014 G-Truc Creation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef GEKRENDER
#define GEKRENDER

// Welcome to the GkScene Namespace!
// The functions are... Mostly... Declared in other files (CPP files)
// This is typical for most C++ programs.

// The Scene class itself is defined in the SceneAPI.h header, and its main draw
// function (and a couple others) are defined in the CPP file.

// In an effort to reduce the number of files, early on, I moved many functions
// out of CPP files to

#include "GL3/gl3.h"
#include "GL3/gl3w.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <vector>
#define GLM_FORCE_RADIANS
#define GLM_ENABLE_EXPERIMENTAL
#include "FBO.h"
#include "SafeTexture.h"
#include "Shader.h"
#include "camera.h"
#include "mesh.h"
#include "obj_loader.h"
#include "texture.h"
#include "transform.h"
#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/transform.hpp>
//#include "resource_manager.h"
#include "GLRenderObject.h"
#include "IODevice.h"
#include "SceneAPI.h"
#include "pointLight.h"

#ifndef uint
#define uint unsigned int
#endif
namespace gekRender {
// Prototypes for utility functions
// Shape Makers
IndexedModel createSphere(float radius, unsigned int longitudinal_complexity, unsigned int latitudinal_complexity, glm::vec3 color = glm::vec3(0, 0, 0));
// NOTE: createBox was MOVED to Obj Loader so that it could be used as an error
// shape.

// TODO test CreateCone and CreatePlane.
IndexedModel createCone(float radius, float h, unsigned int numsubdiv, glm::vec3 color = glm::vec3(0, 0, 0)); // note: CoM is h/4 up from the base

// Creates a plane with Y = up.

IndexedModel createPlane(float Xdim, float Zdim, uint Xsub, uint Zsub, bool smooth, glm::vec3 color, float texScale);
IndexedModel genHeightMap(unsigned char* data, uint w, uint h, int comp, float Xdim, float Zdim, float yScale,
						  bool smooth, // Just passed to plane generator.
						  float texScale);
// Takes in single-byte block data and generates textured voxel.
// Texture is assumed to be a 16x16 texture atlas
// Also colors the voxel with colors, if colors doesn't have enough colors,
// then it uses black.
// Value zero is air, all others generate a cube.
// Does not generate excess faces
// Does not do greedy meshing.
// Supports ONLY single-textured voxels.
// No blocks with different textures on different sides.
IndexedModel Voxel2Model(
	unsigned char* blockData, uint xdim, uint ydim, uint zdim,
	std::vector<glm::vec3>* colors = nullptr
);

glm::quat faceTowardPoint(glm::vec3 pos, glm::vec3 target, glm::vec3 up);

glm::vec3 planeIntersect(const glm::vec4& p1, const glm::vec4& p2, const glm::vec4& p3);

std::vector<glm::vec3> getFrustCorners(const std::vector<glm::vec4>& frustum);

std::vector<glm::vec4> extractFrustum(const glm::mat4& source);
// Wanna code?
}; // namespace gekRender

#endif
