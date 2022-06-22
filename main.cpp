// This has been adapted from the Vulkan tutorial

#include "MyProject.hpp"

const std::string MODEL_PATH = "models/";
const std::string TEXTURE_PATH = "textures/";

// The uniform buffer object used in this example
struct UniformBufferObject
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
	alignas(16) glm::vec3 eyePos;
	alignas(16) glm::vec3 lightDir;
	alignas(16) glm::vec3 refl;
};

class Loader
{
public:
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	// Load all objects in file
	Loader(std::string file)
	{
		std::cout << "Trying to open " << file << std::endl;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
							  file.c_str()))
		{
			throw std::runtime_error(warn + err);
		}
	}

	void loadModelFromIndex(Model &model, int objIndex)
	{
		std::cout << "*****************SHAPES******************" << std::endl;
		tinyobj::shape_t shape = shapes[objIndex];
		std::cout << shape.name << std::endl;
		
		for (const auto &index : shape.mesh.indices)
		{
			Vertex vertex{};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]};

			if (attrib.texcoords.size() > 0 && index.texcoord_index != -1)
			{
				vertex.texCoord = {
					attrib.texcoords[2 * index.texcoord_index + 0],
					1 - attrib.texcoords[2 * index.texcoord_index + 1]};
			}
			else
			{
				vertex.texCoord = {0.0f, 0.0f};
			}

			vertex.norm = {
				attrib.normals[3 * index.normal_index + 0],
				attrib.normals[3 * index.normal_index + 1],
				attrib.normals[3 * index.normal_index + 2]};

			model.vertices.push_back(vertex);
			model.indices.push_back(model.vertices.size() - 1);
		}
	}
};

class SceneObject
{
public:
	Model model;
	Texture texture;
	DescriptorSet DS;
	glm::vec3 position;
	glm::vec3 rotationAxis;
	float rotation;
	glm::mat4 matrix;

	void init(BaseProject *bp, DescriptorSetLayout *DSL1, Loader &loader, int index, Texture &text);
	void init(BaseProject *bp, DescriptorSetLayout *DSL1, Loader &loader, int index, Texture &text, 
	glm::vec3 pos, glm::vec3 rotAxis, float rot);

	void cleanup();
};

class Interactable : public SceneObject 
{
public:
	SceneObject* activate;
	bool active;
	bool set;

	void init(BaseProject *bp, DescriptorSetLayout *DSL1, Loader &loader, int index, Texture &text, 
	glm::vec3 pos, glm::vec3 rotAxis, float rot, SceneObject* act) 
	{
		SceneObject::init(bp, DSL1, loader, index, text, pos, rotAxis, rot);
		activate = act;
		active = false;
		set = false;
	}
};

void SceneObject::init(BaseProject *bp, DescriptorSetLayout *DSL1, Loader &loader, int index, Texture &text)
{
	loader.loadModelFromIndex(model, index);
	model.init(bp, "");
	texture = text;
	DS.init(bp, DSL1, {{0, UNIFORM, sizeof(UniformBufferObject), nullptr}, {1, TEXTURE, 0, &texture}});
	matrix = glm::mat4(1.0f);
}

void SceneObject::init(BaseProject *bp, DescriptorSetLayout *DSL1, Loader &loader, int index, Texture &text, 
	glm::vec3 pos, glm::vec3 rotAxis, float rot)
{
	loader.loadModelFromIndex(model, index);
	model.init(bp, "");
	texture = text;
	DS.init(bp, DSL1, {{0, UNIFORM, sizeof(UniformBufferObject), nullptr}, {1, TEXTURE, 0, &texture}});
	position = pos;
	rotationAxis = rotAxis;
	rotation = rot;
	matrix = glm::mat4(1.0f);
}

void SceneObject::cleanup()
{
	DS.cleanup();
	texture.cleanup();
	model.cleanup();
}

// MAIN !
class MyProject : public BaseProject
{
protected:
	// Here you list all the Vulkan objects you need:

	// Descriptor Layouts [what will be passed to the shaders]
	DescriptorSetLayout DSL1;

	// Pipelines [Shader couples]
	Pipeline P1;

	// Models, textures and Descriptors (values assigned to the uniforms)
	SceneObject copperKey;
	SceneObject goldKey;
	SceneObject doorSide;
	SceneObject goldKeyHole4; // Door4
	SceneObject copperKeyHole2; // Door2
	Interactable lever1;
	Interactable lever3;
	Interactable lever5;
	SceneObject door5;
	SceneObject door4;
	SceneObject door3;
	SceneObject door2;
	SceneObject door1;
	SceneObject floor;
	SceneObject wallW;
	SceneObject wallE;
	SceneObject wallN;
	SceneObject wallS;
	SceneObject ceiling;
	std::vector<SceneObject> allObjects;
	std::vector<Interactable*> interactables;

	Texture floorTexture;
	Texture doorTexture;
	Texture wallTexture;
	Texture ceilingTexture;
	Texture copperKeyTexture;
	Texture goldKeyTexture;
	Texture leverTexture;
	Texture doorSideTexture;
	
	// Lights
	glm::vec3 torchLightDir;

	// Here you set the main application parameters
	void setWindowParameters()
	{
		// window size, titile and initial background
		windowWidth = 800;
		windowHeight = 600;
		windowTitle = "My Project";
		initialBackgroundColor = {0.0f, 0.0f, 0.0f, 1.0f};

		// Descriptor pool sizes
		uniformBlocksInPool = 19;
		texturesInPool = 19;
		setsInPool = 19;
	}

	// Here you load and setup all your Vulkan objects
	void localInit()
	{
		// Descriptor Layouts [what will be passed to the shaders]
		DSL1.init(this, {// this array contains the binding:
						 // first  element : the binding number
						 // second element : the time of element (buffer or texture)
						 // third  element : the pipeline stage where it will be used
						 {0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
						 {1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}});

		// Pipelines [Shader couples]
		// The last array, is a vector of pointer to the layouts of the sets that will
		// be used in this pipeline. The first element will be set 0, and so on..
		P1.init(this, "shaders/vert.spv", "shaders/frag.spv", {&DSL1});

		// Models, textures and Descriptors (values assigned to the uniforms)
		Loader loader = Loader(MODEL_PATH + "Dungeon.diff3.obj");

		floorTexture.init(this, TEXTURE_PATH + "Floor.png");
		doorTexture.init(this, TEXTURE_PATH + "Door.png");
		wallTexture.init(this, TEXTURE_PATH + "Wall.png");
		ceilingTexture.init(this, TEXTURE_PATH + "Ceiling.png");
		copperKeyTexture.init(this, TEXTURE_PATH + "CopperKey.png");
		goldKeyTexture.init(this, TEXTURE_PATH + "GoldKey.png");
		leverTexture.init(this, TEXTURE_PATH + "Lever.png");
		doorSideTexture.init(this, TEXTURE_PATH + "DoorSide.png");

		// Do this for every object
		copperKey.init(this, &DSL1, loader, 0, copperKeyTexture);
		goldKey.init(this, &DSL1, loader, 1, goldKeyTexture);
		doorSide.init(this, &DSL1, loader, 2, doorSideTexture);
		goldKeyHole4.init(this, &DSL1, loader, 3, goldKeyTexture);
		copperKeyHole2.init(this, &DSL1, loader, 4, copperKeyTexture);

		// Levers
		lever1.init(this, &DSL1, loader, 5, leverTexture, glm::vec3(3.0, 0.5, 3.5), glm::vec3(1.0f, 0.0f, 0.0f), -90.0f, &door1);
		lever3.init(this, &DSL1, loader, 6, leverTexture, glm::vec3(9.5, 0.5, 4.0), glm::vec3(0.0f, 0.0f, 1.0f), 90.0f, &door3);
		lever5.init(this, &DSL1, loader, 7, leverTexture, glm::vec3(4.5, 0.5, -1.0), glm::vec3(0.0f, 0.0f, 1.0f), 90.0f, &door5);

		door5.init(this, &DSL1, loader, 8, doorTexture, glm::vec3(4.399166, 0.0, -2.0), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f);
		door4.init(this, &DSL1, loader, 9, doorTexture);
		door3.init(this, &DSL1, loader, 10, doorTexture, glm::vec3(9.399166, 0.0, 3.0), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f);
		door2.init(this, &DSL1, loader, 11, doorTexture);
		door1.init(this, &DSL1, loader, 12, doorTexture, glm::vec3(4, 0, 3.399166), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f);

		floor.init(this, &DSL1, loader, 13, floorTexture);
		wallW.init(this, &DSL1, loader, 14, wallTexture);
		wallE.init(this, &DSL1, loader, 15, wallTexture);
		wallN.init(this, &DSL1, loader, 16, wallTexture);
		wallS.init(this, &DSL1, loader, 17, wallTexture);
		ceiling.init(this, &DSL1, loader, 18, ceilingTexture);

		allObjects.insert(allObjects.end(), {copperKey, goldKey, doorSide, goldKeyHole4, 
		copperKeyHole2, lever1, lever3, lever5, door5, door4, door3, door2, door1, floor, wallW, wallE, wallN, wallS, ceiling});

		interactables.insert(interactables.end(), {&lever1, &lever3, &lever5});
	}

	// Here you destroy all the objects you created!
	void localCleanup()
	{
		for (SceneObject obj : allObjects)
		{
			obj.cleanup();
		}
		P1.cleanup();
		DSL1.cleanup();
	}

	void SendToCommandBuffer(VkCommandBuffer &commandBuffer, int currentImage, SceneObject &obj)
	{
		VkBuffer vertexBuffers[] = {obj.model.vertexBuffer};
		// property .vertexBuffer of models, contains the VkBuffer handle to its vertex buffer
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		// property .indexBuffer of models, contains the VkBuffer handle to its index buffer
		vkCmdBindIndexBuffer(commandBuffer, obj.model.indexBuffer, 0,
							 VK_INDEX_TYPE_UINT32);

		// property .pipelineLayout of a pipeline contains its layout.
		// property .descriptorSets of a descriptor set contains its elements.
		vkCmdBindDescriptorSets(commandBuffer,
								VK_PIPELINE_BIND_POINT_GRAPHICS,
								P1.pipelineLayout, 0, 1, &obj.DS.descriptorSets[currentImage],
								0, nullptr);

		// property .indices.size() of models, contains the number of triangles * 3 of the mesh.
		vkCmdDrawIndexed(commandBuffer,
						 static_cast<uint32_t>(obj.model.indices.size()), 1, 0, 0, 0);
	}

	// Here it is the creation of the command buffer:
	// You send to the GPU all the objects you want to draw,
	// with their buffers and textures
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage)
	{

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
						  P1.graphicsPipeline);

		for (SceneObject obj : allObjects)
		{
			SendToCommandBuffer(commandBuffer, currentImage, obj);
		}
	}

	glm::mat4 CameraMovement(float time)
	{
		static float lastTime = 0.0f;
		float deltaT = time - lastTime;
		lastTime = time;

		const float ROT_SPEED = glm::radians(90.0f);
		const float MOVE_SPEED = 3.0f;
		const float MOUSE_RES = 500.0f;

		static double old_xpos = 0, old_ypos = 0;
		double xpos, ypos;
		glfwGetCursorPos(window, &xpos, &ypos);
		double m_dx = xpos - old_xpos;
		double m_dy = ypos - old_ypos;
		old_xpos = xpos;
		old_ypos = ypos;
		// std::cout << xpos << " " << ypos << " " << m_dx << " " << m_dy << "\n";

		if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
		{
			CamAng.y += -m_dx * ROT_SPEED / MOUSE_RES;
			CamAng.x += -m_dy * ROT_SPEED / MOUSE_RES;
		}

		if (glfwGetKey(window, GLFW_KEY_LEFT))
		{
			CamAng.y += deltaT * ROT_SPEED;
		}
		if (glfwGetKey(window, GLFW_KEY_RIGHT))
		{
			CamAng.y -= deltaT * ROT_SPEED;
		}
		if (glfwGetKey(window, GLFW_KEY_UP))
		{
			CamAng.x += deltaT * ROT_SPEED;
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN))
		{
			CamAng.x -= deltaT * ROT_SPEED;
		}
		if (glfwGetKey(window, GLFW_KEY_Q))
		{
			CamAng.z -= deltaT * ROT_SPEED;
		}
		if (glfwGetKey(window, GLFW_KEY_E))
		{
			CamAng.z += deltaT * ROT_SPEED;
		}

		glm::mat3 CamMatDir = glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.y, glm::vec3(0.0f, 1.0f, 0.0f))) *
						   glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.x, glm::vec3(1.0f, 0.0f, 0.0f))) *
						   glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.z, glm::vec3(0.0f, 0.0f, 1.0f)));

		torchLightDir = CamMatDir * glm::vec3(0.0f, 0.0f, 1.0f);
		CamDir = CamMatDir * glm::vec3(0.0f, 0.0f, 1.0f);

		if (glfwGetKey(window, GLFW_KEY_A))
		{
			CamPos -= MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) * deltaT;
		}
		if (glfwGetKey(window, GLFW_KEY_D))
		{
			CamPos += MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) * deltaT;
		}
		if (glfwGetKey(window, GLFW_KEY_S))
		{
			CamPos += MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;
		}
		if (glfwGetKey(window, GLFW_KEY_W))
		{
			CamPos -= MOVE_SPEED * glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;
		}

		//std::cout << "Cam Pos: " << CamPos[0] << " " << CamPos[1] << " " << CamPos[2] << "\n";
		
		glm::mat4 CamMat = glm::translate(glm::transpose(glm::mat4(CamMatDir)), -CamPos);
		
		return CamMat;
	}

	void checkInteraction() 
	{
		for (Interactable* obj: interactables)
		{
			float distance = glm::distance(CamPos, obj->position);

			if (distance < 0.9) 
			{
				if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !obj->set)
				{
					obj->set = true;
					obj->active = !obj->active;

					if (obj->active)
					{
						glm::mat4 T1 = glm::translate(glm::mat4(1), obj->position);
						glm::mat4 R1 = glm::rotate(glm::mat4(1), glm::radians(obj->rotation), obj->rotationAxis); // Principal transformation
						glm::mat4 MT1 = T1 * R1 * glm::inverse(T1);

						obj->matrix = MT1;

						glm::mat4 T2 = glm::translate(glm::mat4(1), obj->activate->position);
						glm::mat4 R2 = glm::rotate(glm::mat4(1), glm::radians(obj->activate->rotation), obj->activate->rotationAxis); // Principal transformation
						glm::mat4 MT2 = T2 * R2 * glm::inverse(T2);

						obj->activate->matrix = MT2;
					}
					else
					{
						obj->matrix = glm::mat4(1.0f);
						obj->activate->matrix = glm::mat4(1.0f);
					}
				}
			}

			if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) 
			{
				obj->set = false;
			}
		}

		
	}

	// Here is where you update the uniforms.
	// Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage)
	{
		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

		glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);

		checkInteraction();

		UniformBufferObject ubo{};

		void *data;

		ubo.view = CameraMovement(time);

		ubo.proj = glm::perspective(glm::radians(45.0f),
									swapChainExtent.width / (float)swapChainExtent.height,
									0.1f, 10.0f);
		ubo.proj[1][1] *= -1;

		ubo.eyePos = CamPos;
		ubo.lightDir = torchLightDir;

		// CopperKey
		updateObjectUniform(ubo, currentImage, &data, copperKey, glm::mat4(1.0f), 1.0f);
		updateObjectUniform(ubo, currentImage, &data, copperKeyHole2, glm::mat4(1.0f), 1.0f);

		// GoldKey
		updateObjectUniform(ubo, currentImage, &data, goldKey, glm::mat4(1.0f), 1.0f);
		updateObjectUniform(ubo, currentImage, &data, goldKeyHole4, glm::mat4(1.0f), 1.0f);

		// Levers
		updateObjectUniform(ubo, currentImage, &data, lever1, glm::mat4(1.0f), 1.0f);
		updateObjectUniform(ubo, currentImage, &data, lever3, glm::mat4(1.0f), 1.0f);
		updateObjectUniform(ubo, currentImage, &data, lever5, glm::mat4(1.0f), 1.0f);

		// Doors
		updateObjectUniform(ubo, currentImage, &data, doorSide, glm::mat4(1.0f));
		updateObjectUniform(ubo, currentImage, &data, door5, glm::mat4(1.0f));
		updateObjectUniform(ubo, currentImage, &data, door4, glm::mat4(1.0f));
		updateObjectUniform(ubo, currentImage, &data, door3, glm::mat4(1.0f));
		updateObjectUniform(ubo, currentImage, &data, door2, glm::mat4(1.0f));
		updateObjectUniform(ubo, currentImage, &data, door1, glm::mat4(1.0f));

		// Floor
		updateObjectUniform(ubo, currentImage, &data, floor, glm::mat4(1.0f));
		
		// Walls
		updateObjectUniform(ubo, currentImage, &data, wallW, glm::mat4(1.0f));
		updateObjectUniform(ubo, currentImage, &data, wallE, glm::mat4(1.0f));
		updateObjectUniform(ubo, currentImage, &data, wallN, glm::mat4(1.0f));
		updateObjectUniform(ubo, currentImage, &data, wallS, glm::mat4(1.0f));

		// Ceiling
		updateObjectUniform(ubo, currentImage, &data, ceiling, glm::mat4(1.0f));
		
	}

	void updateObjectUniform(UniformBufferObject &ubo, uint32_t currentImage, void **data, SceneObject &obj, glm::mat4 modelMatrix) {
		ubo.model = obj.matrix;
		ubo.refl = glm::vec3(0.0f);

		// Here is where you actually update your uniforms
		vkMapMemory(device, obj.DS.uniformBuffersMemory[0][currentImage], 0,
					sizeof(ubo), 0, data);
		memcpy(*data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, obj.DS.uniformBuffersMemory[0][currentImage]);
	}

	void updateObjectUniform(UniformBufferObject &ubo, uint32_t currentImage, void **data, SceneObject &obj, glm::mat4 modelMatrix, float refl) {
		ubo.model = obj.matrix;
		ubo.refl = glm::vec3(refl);
		//std::cout << "REFL " << refl << std::endl;

		// Here is where you actually update your uniforms
		vkMapMemory(device, obj.DS.uniformBuffersMemory[0][currentImage], 0,
					sizeof(ubo), 0, data);
		memcpy(*data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, obj.DS.uniformBuffersMemory[0][currentImage]);
	}
};

// This is the main: probably you do not need to touch this!
int main()
{
	MyProject app;

	try
	{
		app.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}
