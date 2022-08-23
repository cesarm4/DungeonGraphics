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
	int x; 
	int y;

	void init(BaseProject *bp, DescriptorSetLayout *DSL1, Loader &loader, int index, Texture &text);
	void init(BaseProject *bp, DescriptorSetLayout *DSL1, Loader &loader, int index, Texture &text, 
	glm::vec3 pos, glm::vec3 rotAxis, float rot);

	void cleanup();
};

// Interactable: objects that activate other objects (open the door)
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
		activate = act;     // door linked to the interactable object
		active = false;     // interactable object has been used
		set = false;
	}
};

// specific class with attribute hasKey, can be activated only if the player has collected the related key
class KeyHole : public Interactable 
{
public:
    // true if the player has collected the key
	bool hasKey = false;

	void init(BaseProject *bp, DescriptorSetLayout *DSL1, Loader &loader, int index, Texture &text, 
	glm::vec3 pos, SceneObject* act) 
	{
		SceneObject::init(bp, DSL1, loader, index, text, pos, glm::vec3(0.0f), 0.0f);
		activate = act;     // door linked to the keyhole
		active = false;     // key has been used to open the door
		set = false;
	}
};

void SceneObject::init(BaseProject *bp, DescriptorSetLayout *DSL1, Loader &loader, int index, Texture &text)
{
	loader.loadModelFromIndex(model, index);
	model.init(bp, "");
	texture = text;
	DS.init(bp, DSL1, {{0, UNIFORM, sizeof(UniformBufferObject), nullptr}, {1, TEXTURE, 0, &texture}});
    // transformation matrix
	matrix = glm::mat4(1.0f);
}

void SceneObject::init(BaseProject *bp, DescriptorSetLayout *DSL1, Loader &loader, int index, Texture &text, 
	glm::vec3 pos, glm::vec3 rotAxis, float rot)
{
	loader.loadModelFromIndex(model, index);
	model.init(bp, "");
	texture = text;
	DS.init(bp, DSL1, {{0, UNIFORM, sizeof(UniformBufferObject), nullptr}, {1, TEXTURE, 0, &texture}});
	position = pos; // starting position
	rotationAxis = rotAxis; // axis used for object rotation
	rotation = rot; // angle of rotation
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
	KeyHole goldKeyHole4; // Door4
	KeyHole copperKeyHole2; // Door2
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
	SceneObject endPlane;
	std::vector<SceneObject> allObjects;
	std::vector<Interactable*> interactables;
	std::vector<KeyHole*> keyHoles;

	Texture floorTexture;
	Texture doorTexture;
	Texture doorFlipTexture;
	Texture wallTexture;
	Texture ceilingTexture;
	Texture copperKeyTexture;
	Texture goldKeyTexture;
	Texture leverTexture;
	Texture doorSideTexture;
	Texture endTexture;
	
	// Lights
	glm::vec3 torchLightDir;

	char map[24][25] = {	
		{'*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*'},
		{'*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*',' ',' ','*','*','*','*','*','*'},
		{'*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*',' ',' ','*','*','*','*','*','*'},
		{'*','*','*','*','*','*','*','*','*','*','*','*',' ',' ',' ','*','*',' ',' ','*','*','*','*','*'},
		{'*','*','*','*','*','*','*','*','*','*','*','*',' ','*',' ',' ',' ','*',' ','*','*','*','*','*'},
		{'*','*','*','*','*','*',' ','*','*','*','*','*',' ','*','*',' ',' ',' ',' ',' ','*','*','*','*'},
		{'*','*','*','*','*','*',' ','*','*','*',' ',' ',' ',' ',' ','*','*','*',' ',' ','*','*','*','*'},
		{'*','*','*','*','*','*',' ','*','*','*','d','*','*','*',' ','*',' ','*',' ','*','*','*','*','*'},
		{'*','*','*','*','*','*','*','*','*',' ',' ','*',' ',' ',' ','*',' ','*',' ','*','*','*','*','*'},
		{'*','*','*','*','*','*','o',' ',' ',' ',' ',' ','*',' ',' ','*',' ',' ',' ','*','*','*','*','*'},
		{'*','*','*','*','*','*','*','*','*',' ',' ','*','*',' ','*',' ',' ','*',' ','*','*','*','*','*'},
		{'*','*','*','*','*','*',' ',' ',' ','*','*',' ','*',' ','*',' ',' ','*',' ','*','*','*','*','*'},
		{'*','*','*','*',' ',' ',' ',' ',' ',' ','d',' ',' ',' ','*','d','*',' ',' ',' ','*',' ','*','*'},
		{'*','*','*','*',' ','*',' ',' ',' ','*','*',' ','*','*',' ',' ','*','*','d','*',' ',' ',' ','*'},
		{'*','*','*','*',' ','*','*','*','*','*','*','*',' ',' ',' ','*','*',' ',' ',' ','*',' ','*','*'},
		{'*','*',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ',' ','*','*',' ','*',' ',' ',' ','*',' ','*','*'},
		{'*','*',' ','*',' ','*','*','*','*','*','*','*','*','*','*',' ',' ','*',' ','*',' ',' ','*','*'},
		{'*','*',' ','*',' ',' ',' ',' ',' ',' ',' ',' ',' ','d','f','*',' ',' ',' ',' ',' ','*','*','*'},
		{'*','*',' ','*',' ',' ',' ','*','*','*',' ',' ',' ','*','*','*','*','*',' ','*','*','*','*','*'},
		{'*','*',' ','*','*','*','*','*',' ','*','*','*','*',' ',' ',' ',' ',' ',' ','*','*','*','*','*'},
		{'*',' ',' ',' ','*','*','*',' ',' ',' ',' ',' ',' ',' ','*','*','*','*','*','*','*','*','*','*'},
		{'*',' ',' ',' ',' ',' ',' ',' ','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*'},
		{'*',' ',' ',' ','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*'},
		{'*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*','*'}
	};

	int mapWidth = 25;
	int mapHeight = 24;

    // collision variables
	const float checkRadius = 0.15; //max distance from walls
	const int checkSteps = 12;

	float distanceFromKey = 1.5f; //min distance to pick key

	// Here you set the main application parameters
	void setWindowParameters()
	{
		// window size, titile and initial background
		windowWidth = 1380;
		windowHeight = 850;
		windowTitle = "Dungeon";
		initialBackgroundColor = {0.0f, 0.0f, 0.0f, 1.0f};

		// Descriptor pool sizes
		uniformBlocksInPool = 20;
		texturesInPool = 20;
		setsInPool = 20;
	}

	// Load and setup of your Vulkan objects
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

		// Load objects from file
		Loader loader = Loader(MODEL_PATH + "DungeonEnd.diff3.obj");

        // Texture loading
		floorTexture.init(this, TEXTURE_PATH + "terra.png");
		doorTexture.init(this, TEXTURE_PATH + "wood_door.jpg");
		doorFlipTexture.init(this, TEXTURE_PATH + "wood_door_flip.jpg");
		wallTexture.init(this, TEXTURE_PATH + "muro_rosso.jpg");
		ceilingTexture.init(this, TEXTURE_PATH + "trak_tile_red.jpg");
		copperKeyTexture.init(this, TEXTURE_PATH + "CopperKey.png");
		goldKeyTexture.init(this, TEXTURE_PATH + "GoldKey.png");
		leverTexture.init(this, TEXTURE_PATH + "Lever.png");
		doorSideTexture.init(this, TEXTURE_PATH + "DoorSide2.png");
		endTexture.init(this, TEXTURE_PATH + "end.png");

		// Objects initialization
		copperKey.init(this, &DSL1, loader, 0, copperKeyTexture, glm::vec3(15.0, 0.0, 3.0), glm::vec3(0.0f), 0.0f);
		goldKey.init(this, &DSL1, loader, 1, goldKeyTexture, glm::vec3(10.0, 0.0, -8.0), glm::vec3(0.0f), 0.0f);
		doorSide.init(this, &DSL1, loader, 2, doorSideTexture);
		
        // Key Holes
        goldKeyHole4.init(this, &DSL1, loader, 3, goldKeyTexture, glm::vec3(11.55, 0.5, 3.95), &door4);
		copperKeyHole2.init(this, &DSL1, loader, 4, copperKeyTexture, glm::vec3(6.95, 0.5, 8.45), &door2);

		// Levers (interactable objects)
		lever1.init(this, &DSL1, loader, 5, leverTexture, glm::vec3(3.0, 0.5, 3.5), glm::vec3(1.0f, 0.0f, 0.0f), -90.0f, &door1);
		lever3.init(this, &DSL1, loader, 6, leverTexture, glm::vec3(9.5, 0.5, 4.0), glm::vec3(0.0f, 0.0f, 1.0f), 90.0f, &door3);
		lever5.init(this, &DSL1, loader, 7, leverTexture, glm::vec3(4.5, 0.5, -1.0), glm::vec3(0.0f, 0.0f, 1.0f), 90.0f, &door5);

        // Doors with rotation parameters (axis and angle)
		door5.init(this, &DSL1, loader, 8, doorFlipTexture, glm::vec3(4.4, 0.0, -2.0), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f);
		door4.init(this, &DSL1, loader, 9, doorTexture, glm::vec3(12.4, 0.0, 4.0), glm::vec3(0.0f, 1.0f, 0.0f), 90.0f);
		door3.init(this, &DSL1, loader, 10, doorFlipTexture, glm::vec3(9.4, 0.0, 3.0), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f);
		door2.init(this, &DSL1, loader, 11, doorTexture, glm::vec3(7.0, 0.0, 7.6), glm::vec3(0.0f, 1.0f, 0.0f), 90.0f);
		door1.init(this, &DSL1, loader, 12, doorFlipTexture, glm::vec3(4, 0, 3.4), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f);

		floor.init(this, &DSL1, loader, 13, floorTexture);
		wallW.init(this, &DSL1, loader, 14, wallTexture);
		wallE.init(this, &DSL1, loader, 15, wallTexture);
		wallN.init(this, &DSL1, loader, 16, wallTexture);
		wallS.init(this, &DSL1, loader, 17, wallTexture);
		ceiling.init(this, &DSL1, loader, 18, ceilingTexture);
        
        // Plane with final message for victory
		endPlane.init(this, &DSL1, loader, 19, endTexture);

		allObjects.insert(allObjects.end(), {copperKey, goldKey, doorSide, goldKeyHole4, 
		copperKeyHole2, lever1, lever3, lever5, door5, door4, door3, door2, door1, floor, wallW, wallE, wallN, wallS, ceiling, endPlane});

		interactables.insert(interactables.end(), {&lever1, &lever3, &lever5});
		keyHoles.insert(keyHoles.end(), {&goldKeyHole4, &copperKeyHole2});
	}

	// Destroy all the objects created before closing
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

	// Conversion from 3D coordinates to map coordinates
	glm::ivec2 posToMap(float x, float y) {
		// (9, 6) is the initial position of the player in the map
        // round to have int values, fmax and fmin to avoid negative values or values outside the map
		int mapX = round(fmax(0.0f, fmin(mapWidth-1,  (x+6))));
		int mapY = round(fmax(0.0f, fmin(mapHeight-1, (y+9))));

		return glm::ivec2(mapX, mapY);
	}

    // x, y: points on the circumference of checkRadius around player position
	bool canStepPoint(float x, float y) {
		glm::ivec2 mapPos = posToMap(x, y);
		int pix = (int)map[mapPos.y][mapPos.x];
        // check if (y,x) coord in the map corresponds to a wall (*) or a door (d)
		return pix != '*' && pix != 'd';
	}

    // parameters x, y: camera position
	bool canStep(float x, float y) {
		for(int i = 0; i < checkSteps; i++) {
            // check whether there is an obstacle in the circumference of radius checkRadius around the player (x, y)
			if(!canStepPoint(x + cos(6.2832 * i / (float)checkSteps) * checkRadius,
							 y + sin(6.2832 * i / (float)checkSteps) * checkRadius)) {
				return false;
			}
		}
		return true;
	}

    // Implementation of player movement
	glm::mat4 CameraMovement(float time)
	{
		static float lastTime = 0.0f;
		float deltaT = time - lastTime; // time from prev call
		lastTime = time;

		const float ROT_SPEED = glm::radians(90.0f);
		const float MOVE_SPEED = 3.0f;

        // control camera rotation with left right up down keys
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
			if (CamAng.x < glm::radians(90.0f))
			{
				CamAng.x += deltaT * ROT_SPEED;
			}
		}
		if (glfwGetKey(window, GLFW_KEY_DOWN))
		{
			if (CamAng.x > glm::radians(-90.0f))
			{
				CamAng.x -= deltaT * ROT_SPEED;
			}
		}
        
        // camera rotation matrix, z axis not used
		glm::mat3 CamMatDir = glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.y, glm::vec3(0.0f, 1.0f, 0.0f))) *
						   glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.x, glm::vec3(1.0f, 0.0f, 0.0f)));
        
		torchLightDir = CamMatDir * glm::vec3(0.0f, 0.0f, 1.0f);
		CamDir = CamMatDir * glm::vec3(0.0f, 0.0f, 1.0f);

		glm::vec3 oldCamPos = CamPos;

        // control camera position with A D S W keys
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

        // control for walls collision
		if(!canStep(CamPos.x, CamPos.z)) {
			CamPos = oldCamPos;
		}

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
                // when pressing P, for one time make set parameter true and change active param to open/close doors
				if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !obj->set)
				{
					obj->set = true;
					obj->active = !obj->active;
					
					// Open
					if (obj->active)
					{
						// Lever (to be rotated when activated)
						glm::mat4 T1 = glm::translate(glm::mat4(1), obj->position);
						glm::mat4 R1 = glm::rotate(glm::mat4(1), glm::radians(obj->rotation), obj->rotationAxis); // Principal transformation
						glm::mat4 MT1 = T1 * R1 * glm::inverse(T1);

						obj->matrix = MT1;

						// Door (to be rotated when opened/closed)
						glm::mat4 T2 = glm::translate(glm::mat4(1), obj->activate->position);
						glm::mat4 R2 = glm::rotate(glm::mat4(1), glm::radians(obj->activate->rotation), obj->activate->rotationAxis); // Principal transformation
						glm::mat4 MT2 = T2 * R2 * glm::inverse(T2);
						
                        // free space added in the map in place of d (door)
						glm::ivec2 mapPos = posToMap(obj->activate->position.x, obj->activate->position.z);
						map[mapPos.y][mapPos.x] = ' ';

						obj->activate->matrix = MT2;
					}
					// Close
					else
					{
						obj->matrix = glm::mat4(1.0f);
                        // remove rotation and go back to initial door position
						obj->activate->matrix = glm::mat4(1.0f);

                        // d (door) in the map in place of free space
						glm::ivec2 mapPos = posToMap(obj->activate->position.x, obj->activate->position.z);
						map[mapPos.y][mapPos.x] = 'd';
					}
				}
			}
            
            // when set is false the player can interact again with the related object
			if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) 
			{
				obj->set = false;
			}
		}
	}
	
    // same as checkInteraction, but in this case also check if player has the related key
	void checkKeyHoles()
	{
		for (KeyHole* obj: keyHoles)
		{
			float distance = glm::distance(CamPos, obj->position);

			if (distance < 2.0) 
			{
				if (obj->hasKey && glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS && !obj->set)
				{
					obj->set = true;
					obj->active = !obj->active;
					
					// Open
					if (obj->active)
					{
						// Door
						glm::mat4 T2 = glm::translate(glm::mat4(1), obj->activate->position);
						glm::mat4 R2 = glm::rotate(glm::mat4(1), glm::radians(obj->activate->rotation), obj->activate->rotationAxis); // Principal transformation
						glm::mat4 MT2 = T2 * R2 * glm::inverse(T2);
						
						glm::ivec2 mapPos = posToMap(obj->activate->position.x, obj->activate->position.z);
						map[mapPos.y][mapPos.x] = ' ';

						obj->activate->matrix = MT2;
					}
					// Close
					else
					{
						obj->matrix = glm::mat4(1.0f);
						obj->activate->matrix = glm::mat4(1.0f);

						glm::ivec2 mapPos = posToMap(obj->activate->position.x, obj->activate->position.z);
						map[mapPos.y][mapPos.x] = 'd';
					}
				}
			}

			if (glfwGetKey(window, GLFW_KEY_P) == GLFW_RELEASE) 
			{
				obj->set = false;
			}
		}
	}

    // allows the player to collect a key when close to it and sets hasKey to true
	void checkKeys()
	{
		float distance = glm::distance(CamPos, goldKey.position);

		if (distance < distanceFromKey) 
		{
			if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
			{
				goldKeyHole4.hasKey = true;
			}
		}

		distance = glm::distance(CamPos, copperKey.position);

		if (distance < distanceFromKey) 
		{
			if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
			{
				copperKeyHole2.hasKey = true;
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

        // call to check methods
		checkInteraction();
		checkKeyHoles();
		checkKeys();

		UniformBufferObject ubo{};

		void *data;

		ubo.view = CameraMovement(time);

		ubo.proj = glm::perspective(glm::radians(45.0f),
									swapChainExtent.width / (float)swapChainExtent.height,
									0.1f, 10.0f);
		ubo.proj[1][1] *= -1;

		ubo.eyePos = CamPos;
		ubo.lightDir = torchLightDir;

        // When key are collected, show them in the bottom right corner of the screen as inventary
		// CopperKey
		if (copperKeyHole2.hasKey)
		{
			glm::vec3 hor = glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1));

			glm::vec3 ver = glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.y, glm::vec3(0.0f, 1.0f, 0.0f))) *
						   glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.x, glm::vec3(1.0f, 0.0f, 0.0f))) *
						   glm::vec3(0, 1, 0);

			glm::vec3 distance = (CamPos - 0.13f * CamDir + 0.045f*hor - 0.045f*ver) - copperKey.position;

			glm::mat4 T1 = glm::translate(glm::mat4(1), distance);
			glm::mat4 Torigin = glm::translate(glm::mat4(1), copperKey.position);
			glm::mat4 R1 = glm::rotate(glm::mat4(1), glm::radians(90.0f), glm::vec3(0, 1, 0));
			glm::mat4 R2 = glm::rotate(glm::mat4(1), glm::radians(90.0f), glm::vec3(0, 0, 1));

			glm::mat4 R3 = glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.y, glm::vec3(0.0f, 1.0f, 0.0f))) *
						   glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.x, glm::vec3(1.0f, 0.0f, 0.0f)));

			glm::mat4 S1 = glm::scale(glm::mat4(1), glm::vec3(0.08f));

			copperKey.matrix = T1 * Torigin * R3 * R1 * R2 * S1 * glm::inverse(Torigin);

		}
        // update object position
		updateObjectUniform(ubo, currentImage, &data, copperKey, glm::mat4(1.0f), 1.0f);
		updateObjectUniform(ubo, currentImage, &data, copperKeyHole2, glm::mat4(1.0f), 1.0f);

		// GoldKey
		if (goldKeyHole4.hasKey)
		{
			glm::vec3 hor = glm::vec3(glm::rotate(glm::mat4(1.0f), CamAng.y, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1));

			glm::vec3 ver = glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.y, glm::vec3(0.0f, 1.0f, 0.0f))) *
						   glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.x, glm::vec3(1.0f, 0.0f, 0.0f))) *
						   glm::vec3(0, 1, 0);

			glm::vec3 distance = (CamPos - 0.13f * CamDir + 0.06f*hor - 0.045f*ver) - goldKey.position;

			glm::mat4 T1 = glm::translate(glm::mat4(1), distance);
			glm::mat4 Torigin = glm::translate(glm::mat4(1), goldKey.position);
			glm::mat4 R1 = glm::rotate(glm::mat4(1), glm::radians(90.0f), glm::vec3(0, 1, 0));
			glm::mat4 R2 = glm::rotate(glm::mat4(1), glm::radians(90.0f), glm::vec3(0, 0, 1));

			glm::mat4 R3 = glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.y, glm::vec3(0.0f, 1.0f, 0.0f))) *
						   glm::mat3(glm::rotate(glm::mat4(1.0f), CamAng.x, glm::vec3(1.0f, 0.0f, 0.0f)));

			glm::mat4 S1 = glm::scale(glm::mat4(1), glm::vec3(0.08f));

			goldKey.matrix = T1 * Torigin * R3 * R1 * R2 * S1 * glm::inverse(Torigin);

		}
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

		// End Plane
		updateObjectUniform(ubo, currentImage, &data, endPlane, glm::mat4(1.0f));
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
