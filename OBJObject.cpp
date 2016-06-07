#include "OBJObject.h"
#include "Window.h"

using namespace std;

#define RUN_SPEED  500.0f
#define TURN_SPEED  900.0f


/* Initialize the object, parse it and set up buffers. */
OBJObject::OBJObject(const char *filepath, int material) 
{
	//Initialize movement.
	this->currentDirection = glm::vec3(0.0f, 0.0f, 1.0f);
	this->currentSpeed = 0.0f;
	this->currentTurnSpeed = 0.0f;
	//Initialize World and material.
	this->toWorld = glm::mat4(1.0f);//Default at the origin.
	this->toWorld[3] = glm::vec4(0.0, 1.0f, 0.0, 1.0f);
	this->material = material;//Set the material to the passed in material number!
	//Parse the object @ filepath.
	this->parse(filepath);
	//Setup the object.
	this->setupObject();
	//Setup the object material.
	this->setupMaterial();
	//Initialize BoxCoords.
	this->setCube();
	this->bindCube();
	this->collisionFlag = 0;
}

/* Deconstructor to safely delete when finished. */
OBJObject::~OBJObject()
{
	//Properly de-allocate all resources once they've outlived their purpose.
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
}

/* Populate the face indices, vertices, and normals vectors with the OBJ Object data. */
void OBJObject::parse(const char *filepath)
{
	//Initialize min, max, scale values for each coordinate.
	minX = INFINITY, minY = INFINITY, minZ = INFINITY;//Minimum set to infinity so first value is always inputted.
	maxX = -INFINITY, maxY = -INFINITY, maxZ = -INFINITY;//Maximum set to -infinity so first value is always inputted.
	scale_v = -INFINITY;//Same for scale, used to find "max" scale or the longest axis. This ensures the ranges of vertices are [-1, 1].
	//Open the file for reading called objFile.
	std::FILE * objFile = fopen(filepath, "r");
	if (objFile == NULL) return;
	//Read the file until the end; "# are commments to be ignored".
	while (1) {
		char buf[BUFSIZ];
		int check = fscanf(objFile, "%s", buf);
		if (check == EOF) break;
		//Read in lines that start with "v". Add into vertices.
		if (strcmp(buf, "v") == 0) {
			glm::vec3 vertex;
			fscanf(objFile, "%f %f %f\n", &vertex.x, &vertex.y, &vertex.z);
			vertices.push_back(vertex);
			//Calculate min, max for x, y, z.
			if (vertex.x < minX) { minX = vertex.x; }
			if (vertex.y < minY) { minY = vertex.y; }
			if (vertex.z < minZ) { minZ = vertex.z; }
			if (vertex.x > maxX) { maxX = vertex.x; }
			if (vertex.y > maxY) { maxY = vertex.y; }
			if (vertex.z > maxZ) { maxZ = vertex.z; }
			continue;
		}
		//Read in lines that start with "vn". Add into normals.
		if (strcmp(buf, "vn") == 0) {
			glm::vec3 normalVertex;
			fscanf(objFile, "%f %f %f\n", &normalVertex.x, &normalVertex.y, &normalVertex.z);
			normals.push_back(normalVertex);
			continue;
		}
		//Read in lines that start with "f". Add into indices.
		if (strcmp(buf, "f") == 0) {//Note: Read only left or right side since they are identical on both sides.
			unsigned int faces_v[3], faces_vn[3];
			fscanf(objFile, "%d//%d %d//%d %d//%d\n", &faces_v[0], &faces_vn[0], &faces_v[1], &faces_vn[1], &faces_v[2], &faces_vn[2]);
			//unsigned int faces_v[3], faces_vn[3], faces_vt[3];
			//fscanf(objFile, "%d/%d/%d %d/%d/%d %d/%d/%d\n", &faces_v[0], &faces_vt[0], &faces_vn[0], &faces_v[1], &faces_vt[1], &faces_vn[1], &faces_v[2], &faces_vt[2], &faces_vn[2]);
			indices.push_back(faces_v[0] - 1);
			indices.push_back(faces_v[1] - 1);
			indices.push_back(faces_v[2] - 1);
			continue;

		}
	}
	fclose(objFile);
	//Calculate average x, y, z.
	avgX = (minX + maxX) / 2;
	avgY = (minY + maxY) / 2;
	avgZ = (minX + maxZ) / 2;
	//Calculate scale value.
	float scale_x = (maxX - avgX);
	float scale_y = (maxY - avgY);
	float scale_z = (maxZ - avgZ);
	if (scale_x > scale_v) { scale_v = scale_x; }
	if (scale_y > scale_v) { scale_v = scale_y; }
	if (scale_z > scale_v) { scale_v = scale_z; }
	//Subtract the average to center all objects and multiply by (1/scale) to bring them down to size.
	for (unsigned int i = 0; i < vertices.size(); i++)
	{
		vertices[i].x = vertices[i].x - avgX;
		vertices[i].y = vertices[i].y - avgY;
		vertices[i].z = vertices[i].z - avgZ;
		vertices[i] *= (1 / (scale_v));
		//Throw everything into a container to hold all values.
		Container container;
		container.vertex = vertices[i];
		container.normal = normals[i];
		container.texCoord = glm::vec2(0.0f, 0.0f);
		containers.push_back(container);
	}

	this->average = glm::vec3(avgX, avgY, avgZ);
	this->longestDim = scale_v;
}

/* Setup the object for modern openGL rendering. */
void OBJObject::setupObject()
{
	//Create buffers/arrays.
	glGenVertexArrays(1, &this->VAO);
	glGenBuffers(1, &this->VBO);
	glGenBuffers(1, &this->EBO);

	//Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
	glBindVertexArray(VAO); //Bind vertex array object.

	glBindBuffer(GL_ARRAY_BUFFER, VBO); //Bind Container buffer.
	glBufferData(GL_ARRAY_BUFFER, this->containers.size() * sizeof(Container), &this->containers[0], GL_STATIC_DRAW); //Set vertex buffer to the Container.
	
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); //Bind indices buffer.
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(int), &this->indices[0], GL_STATIC_DRAW);
	
	//Vertex Positions.
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0,//This first parameter x should be the same as the number passed into the line "layout (location = x)" in the vertex shader. In this case, it's 0. Valid values are 0 to GL_MAX_UNIFORM_LOCATIONS.
		3, //This second line tells us how any components there are per vertex. In this case, it's 3 (we have an x, y, and z component).
		GL_FLOAT, //What type these components are.
		GL_FALSE, //GL_TRUE means the values should be normalized. GL_FALSE means they shouldn't.
		sizeof(Container), //Offset between consecutive vertex attributes. Since each of our vertices have 3 floats, they should have the size of 3 floats in between.
		(GLvoid*)0); //Offset of the first vertex's component. In our case it's 0 since we don't pad the vertices array with anything.

	//Vertex Normals.
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Container), (GLvoid*)offsetof(Container, normal));
	
	//Vertex Texture Coords.
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Container), (GLvoid*)offsetof(Container, texCoord));

	//Unbind.
	glBindBuffer(GL_ARRAY_BUFFER, 0); //Note that this is allowed, the call to glVertexAttribPointer registered VBO as the currently bound vertex buffer object so afterwards we can safely unbind.
	glBindVertexArray(0); //Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs), remember: do NOT unbind the EBO, keep it bound to this VAO.
}

/* Setup the material of the object. We can define different materials here as well! */
void OBJObject::setupMaterial()
{
	if (this->material == 1)
	{	/* Polished gold */
		this->objMaterial.ambient = glm::vec3(0.24725f, 0.2245f, 0.0645f);
		this->objMaterial.diffuse = glm::vec3(0.34615f, 0.3143f, 0.0903f);
		this->objMaterial.specular = glm::vec3(0.797357f, 0.723991f, 0.208006f);
		this->objMaterial.shininess = 83.2f;
	}
	else if (this->material == 2)
	{   /* Obsidian */
		this->objMaterial.ambient = glm::vec3(0.05375f, 0.05f, 0.06625f);
		this->objMaterial.diffuse = glm::vec3(0.18275f, 0.17f, 0.22525f);
		this->objMaterial.specular = glm::vec3(0.332741f, 0.328634f, 0.346435f);
		this->objMaterial.shininess = 38.4f;
	}
	else if (this->material == 3)
	{	/* Jade */
		this->objMaterial.ambient = glm::vec3(0.135f, 0.2225f, 0.1575f);
		this->objMaterial.diffuse = glm::vec3(0.54f, 0.89f, 0.63f);
		this->objMaterial.specular = glm::vec3(0.316228f, 0.316228f, 0.316228f);
		this->objMaterial.shininess = 12.8f;
	}
	else if (this->material == 4)
	{   /* Red Obsidian */
		this->objMaterial.ambient = glm::vec3(1.0f, 0.0f, 0.0f);
		this->objMaterial.diffuse = glm::vec3(0.18275f, 0.17f, 0.22525f);
		this->objMaterial.specular = glm::vec3(0.332741f, 0.328634f, 0.346435f);
		this->objMaterial.shininess = 38.4f;
	}
	else
	{	/* None selected, set to black */
		this->objMaterial.ambient = glm::vec3(1.0f, 0.0f, 0.0f);
		this->objMaterial.diffuse = glm::vec3(0.01f, 0.01f, 0.01f);
		this->objMaterial.specular = glm::vec3(0.5f, 0.5f, 0.5f);
		this->objMaterial.shininess = 32.0f;
	}
}

/* Update Material if needed from any changes in the material struct */
void OBJObject::updateMaterial(GLuint shaderProgram) 
{
	glUniform3f(glGetUniformLocation(shaderProgram, "material.ambient"), objMaterial.ambient.x, objMaterial.ambient.y, objMaterial.ambient.z);
	glUniform3f(glGetUniformLocation(shaderProgram, "material.diffuse"), objMaterial.diffuse.x, objMaterial.diffuse.y, objMaterial.diffuse.z);
	glUniform3f(glGetUniformLocation(shaderProgram, "material.specular"), objMaterial.specular.x, objMaterial.specular.y, objMaterial.specular.z);
	glUniform1f(glGetUniformLocation(shaderProgram, "material.shininess"), objMaterial.shininess);
	//Update the reflection intensity.
	glUniform1f(glGetUniformLocation(shaderProgram, "reflect_intensity"), objMaterial.shininess / 100.0f);
}

/* Render the object in modern openGL using a shader program. */
void OBJObject::draw(GLuint shaderProgram)
{
	//Calculate combination of the model (toWorld), view (camera inverse), and perspective matrices. Send to shader.
	glm::mat4 MVP = Window::P * Window::V * toWorld;
	glm::mat4 model = this->toWorld;
	glm::mat4 view = Window::V;
	glm::mat4 projection = Window::P;
	//Set MVP(Total calculated, easier to multiply in the shader) for the shader.
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "MVP"), 1, GL_FALSE, &MVP[0][0]);
	//Set individual components for shader calculations (Model, View, Projection).
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
	glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
	//Update the viewPosition.
	glUniform3f(glGetUniformLocation(shaderProgram, "viewPos"), Window::camera_pos.x, Window::camera_pos.y, Window::camera_pos.z);
	//Update toon_shade.
	glUniform1i(glGetUniformLocation(shaderProgram, "toon_shade"), Window::toon_shading);
	//Update the material.
	updateMaterial(shaderProgram);
	//Bind for rendering.
	glBindVertexArray(this->VAO);
	glDrawElements(GL_TRIANGLES, (GLsizei)this->indices.size(), GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

void OBJObject::W_movement(glm::vec2 boundaries)
{
	glm::vec3 current_position = glm::vec3(this->toWorld[3]);
	glm::vec3 displacement = this->currentDirection * (RUN_SPEED * Window::delta);
	glm::vec3 new_position = current_position + displacement;
	//Check the boundaries.
	if (new_position.x > boundaries.x || new_position.z > boundaries.y || new_position.x < 0 || new_position.z < 0)
	{
		new_position = current_position;
	}
	this->toWorld[3] = glm::vec4(new_position, 1.0f);
}

void OBJObject::A_movement(glm::vec2 boundaries)
{
	glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), ((TURN_SPEED * Window::delta) / 180.0f * glm::pi<float>()), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::vec4 current_direction = glm::vec4(this->currentDirection, 1.0f);
	current_direction = rotate * current_direction;
	glm::vec3 new_direction = glm::vec3(current_direction);
	this->currentDirection = glm::normalize(new_direction);
	this->toWorld = toWorld * rotate;
}

void OBJObject::S_movement(glm::vec2 boundaries)
{
	glm::vec3 current_position = glm::vec3(this->toWorld[3]);
	glm::vec3 displacement = this->currentDirection * (RUN_SPEED * Window::delta);
	glm::vec3 new_position = current_position - displacement;
	//Check the boundaries.
	if (new_position.x > boundaries.x || new_position.z > boundaries.y || new_position.x < 0 || new_position.z < 0)
	{
		new_position = current_position;
	}
	this->toWorld[3] = glm::vec4(new_position, 1.0f);
}

void OBJObject::D_movement(glm::vec2 boundaries)
{
	glm::mat4 rotate = glm::rotate(glm::mat4(1.0f), ((-TURN_SPEED * Window::delta) / 180.0f * glm::pi<float>()), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::vec4 current_direction = glm::vec4(this->currentDirection, 1.0f);
	current_direction = rotate * current_direction;
	glm::vec3 new_direction = glm::vec3(current_direction);
	this->currentDirection = glm::normalize(new_direction);
	this->toWorld = toWorld * rotate;
}

void OBJObject::update_height(float height)
{
	this->toWorld[3].y = height + 0.8f;
}

bool OBJObject::collision(OBJObject * obj2) {

	bool collisionX = toWorld[3][0] + ((maxX - minX) / 2 - average.x) / longestDim >= obj2->toWorld[3][0] - 0.5f &&
		obj2->toWorld[3][0] + ((obj2->maxX - obj2->minX) / 2 - obj2->average.x) / obj2->longestDim >= toWorld[3][0] - 0.5f;

	bool collisionY = toWorld[3][1] + ((maxY - minY) / 2 - average.y) / longestDim >= obj2->toWorld[3][1] - 0.5f &&
		obj2->toWorld[3][1] + ((obj2->maxY - obj2->minY) / 2 - obj2->average.y) / obj2->longestDim >= toWorld[3][1] - 0.5f;

	bool collisionZ = toWorld[3][2] + ((maxZ - minZ) / 2 - average.z) / longestDim >= obj2->toWorld[3][2] - 0.5f &&
		obj2->toWorld[3][2] + ((obj2->maxZ - obj2->minZ) / 2 - obj2->average.z) / obj2->longestDim >= toWorld[3][2] - 0.5f;

	bool collide = collisionX && collisionY && collisionZ;
	if (collide) {
		collisionFlag = 1;
		obj2->collisionFlag = 1;
	}
	else {
		collisionFlag = 0;
		obj2->collisionFlag = 0;
	}
	return collisionX && collisionY && collisionZ;
}

void OBJObject::setCube() {

	boxCoords.clear();

	glm::vec3 v0 = glm::vec3(toWorld[3][0] + ((maxX - minX) / 2), toWorld[3][1] + ((maxY - minY) / 2), toWorld[3][2] + ((maxZ - minZ) / 2));
	glm::vec3 v1 = glm::vec3(toWorld[3][0] - ((maxX - minX) / 2), toWorld[3][1] + ((maxY - minY) / 2), toWorld[3][2] + ((maxZ - minZ) / 2));
	glm::vec3 v2 = glm::vec3(toWorld[3][0] + ((maxX - minX) / 2), toWorld[3][1] - ((maxY - minY) / 2), toWorld[3][2] + ((maxZ - minZ) / 2));
	glm::vec3 v3 = glm::vec3(toWorld[3][0] - ((maxX - minX) / 2), toWorld[3][1] - ((maxY - minY) / 2), toWorld[3][2] + ((maxZ - minZ) / 2));

	glm::vec3 v4 = glm::vec3(toWorld[3][0] + ((maxX - minX) / 2), toWorld[3][1] + ((maxY - minY) / 2), toWorld[3][2] - ((maxZ - minZ) / 2));
	glm::vec3 v5 = glm::vec3(toWorld[3][0] - ((maxX - minX) / 2), toWorld[3][1] + ((maxY - minY) / 2), toWorld[3][2] - ((maxZ - minZ) / 2));
	glm::vec3 v6 = glm::vec3(toWorld[3][0] + ((maxX - minX) / 2), toWorld[3][1] - ((maxY - minY) / 2), toWorld[3][2] - ((maxZ - minZ) / 2));
	glm::vec3 v7 = glm::vec3(toWorld[3][0] - ((maxX - minX) / 2), toWorld[3][1] - ((maxY - minY) / 2), toWorld[3][2] - ((maxZ - minZ) / 2));

	v0 = glm::vec3((v0.x - average.x) / longestDim, (v0.y - average.y) / longestDim, (v0.z - average.z) / longestDim);
	v1 = glm::vec3((v1.x - average.x) / longestDim, (v1.y - average.y) / longestDim, (v1.z - average.z) / longestDim);
	v2 = glm::vec3((v2.x - average.x) / longestDim, (v2.y - average.y) / longestDim, (v2.z - average.z) / longestDim);
	v3 = glm::vec3((v3.x - average.x) / longestDim, (v3.y - average.y) / longestDim, (v3.z - average.z) / longestDim);
	v4 = glm::vec3((v4.x - average.x) / longestDim, (v4.y - average.y) / longestDim, (v4.z - average.z) / longestDim);
	v5 = glm::vec3((v5.x - average.x) / longestDim, (v5.y - average.y) / longestDim, (v5.z - average.z) / longestDim);
	v6 = glm::vec3((v6.x - average.x) / longestDim, (v6.y - average.y) / longestDim, (v6.z - average.z) / longestDim);
	v7 = glm::vec3((v7.x - average.x) / longestDim, (v7.y - average.y) / longestDim, (v7.z - average.z) / longestDim);

	//front face
	boxCoords.push_back(v0);
	boxCoords.push_back(v1);
	boxCoords.push_back(v2);
	boxCoords.push_back(v0);
	boxCoords.push_back(v2);
	boxCoords.push_back(v3);

	//right face
	boxCoords.push_back(v0);
	boxCoords.push_back(v3);
	boxCoords.push_back(v4);
	boxCoords.push_back(v4);
	boxCoords.push_back(v5);
	boxCoords.push_back(v0);

	//top face
	boxCoords.push_back(v0);
	boxCoords.push_back(v5);
	boxCoords.push_back(v6);
	boxCoords.push_back(v6);
	boxCoords.push_back(v1);
	boxCoords.push_back(v0);

	//back face
	boxCoords.push_back(v7);
	boxCoords.push_back(v4);
	boxCoords.push_back(v5);
	boxCoords.push_back(v5);
	boxCoords.push_back(v6);
	boxCoords.push_back(v7);

	//left face
	boxCoords.push_back(v7);
	boxCoords.push_back(v2);
	boxCoords.push_back(v1);
	boxCoords.push_back(v1);
	boxCoords.push_back(v6);
	boxCoords.push_back(v7);

	//bottom face
	boxCoords.push_back(v7);
	boxCoords.push_back(v4);
	boxCoords.push_back(v3);
	boxCoords.push_back(v3);
	boxCoords.push_back(v2);
	boxCoords.push_back(v7);

}

void OBJObject::bindCube() {
	glGenVertexArrays(1, &VAOBOX);
	glGenBuffers(1, &VBOBOX);

	// Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
	glBindVertexArray(VAOBOX);

	glBindBuffer(GL_ARRAY_BUFFER, VBOBOX);
	glBufferData(GL_ARRAY_BUFFER, boxCoords.size() * sizeof(glm::vec3), &boxCoords[0], GL_STATIC_DRAW);
	glVertexAttribPointer(0,// This first parameter x should be the same as the number passed into the line "layout (location = x)" in the vertex shader. In this case, it's 0. Valid values are 0 to GL_MAX_UNIFORM_LOCATIONS.
		3, // This second line tells us how any components there are per vertex. In this case, it's 3 (we have an x, y, and z component)
		GL_FLOAT, // What type these components are
		GL_FALSE, // GL_TRUE means the values should be normalized. GL_FALSE means they shouldn't
		3 * sizeof(GLfloat), // Offset between consecutive vertex attributes. Since each of our vertices have 3 floats, they should have the size of 3 floats in between
		(GLvoid*)0); // Offset of the first vertex's component. In our case it's 0 since we don't pad the vertices array with anything.

	glEnableVertexAttribArray(0);


	glBindBuffer(GL_ARRAY_BUFFER, 0); // Note that this is allowed, the call to glVertexAttribPointer registered VBO as the currently bound vertex buffer object so afterwards we can safely unbind

	glBindVertexArray(0); // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs), remember: do NOT unbind the EBO, keep it bound to this VAO
}

void OBJObject::drawBox(GLuint shaderProgram) {

	glm::mat4 MVP = Window::P * Window::V * toWorld;
	setCube();

	GLuint collideID = glGetUniformLocation(shaderProgram, "collisionFlag");
	if (collisionFlag) {
		glUniform1i(collideID, 1);
	}
	else {
		glUniform1i(collideID, 0);
	}
	// We need to calculate this because as of GLSL version 1.40 (OpenGL 3.1, released March 2009), gl_ModelViewProjectionMatrix has been
	// removed from the language. The user is expected to supply this matrix to the shader when using modern OpenGL.
	GLuint MatrixID = glGetUniformLocation(shaderProgram, "MVP");
	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

	glBindVertexArray(VAOBOX);
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, boxCoords.size());
	glBindVertexArray(0);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}
