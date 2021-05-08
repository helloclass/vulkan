#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <iostream>
#include <unistd.h>
#include <fstream>
#include <stdexcept>
#include <algorithm>
#include <chrono>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <cstdint>
#include <array>
#include <optional>
#include <set>
#include <unordered_map>

float ComputeConstantLayouts[][4] = {
    { 0.0f, 0.0f, 0.0f, 0.0f },     // CamPos
    { 0.0f, 0.0f, 0.0f, 0.0f },     // MousePos
    { 0.0f, 10.0f, 10.0f, 0.0f }      // HalfPos
};

float GraphicsConstantLayouts[][4] = {
    { 0.0f, 0.0f, 0.0f, 0.0f },     // CamPos
    { 0.0f, 0.0f, 0.0f, 0.0f },     // LightPos
    { 0.58f, 0.58f, 0.58f, 0.0f },  // Normal
};

glm::vec3 lightVec = glm::vec3(0.0f);

uint32_t WIDTH = 800;
uint32_t HEIGHT = 600;

const int MAX_FRAMES_IN_FLIGHT = 2;

const std::vector<const char*> instanceLayers = {
    "VK_LAYER_KHRONOS_validation",
	"VK_LAYER_LUNARG_standard_validation",

};
const std::vector<const char*> instanceExtensions = {
    "VK_KHR_surface",
	"VK_KHR_xcb_surface",
	"VK_EXT_debug_utils",
	"VK_EXT_debug_report",
    "VK_KHR_get_physical_device_properties2"
};

const std::vector<const char*> deviceLayers = {

};
const std::vector<const char*> deviceExtensions = {
    "VK_KHR_swapchain",
    // "VK_KHR_ray_tracing"
    // "VK_KHR_get_memory_requirements2",
    // "VK_EXT_descriptor_indexing",
    // "VK_KHR_buffer_device_address",
    // "VK_KHR_deferred_host_operations",
    // "VK_KHR_pipeline_library",
    // "VK_KHR_maintenance3"
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct Vertex {
    glm::vec3 pos;
    glm::vec2 texCoord;
    glm::vec3 normal;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, texCoord);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, normal);

        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const {
        return  pos == other.pos && 
                texCoord == other.texCoord &&
                normal == other.normal;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

struct UniformBufferObject {
    alignas(16) glm::mat4 model;

    alignas(16) glm::mat4 pitch;
    alignas(16) glm::mat4 yaw;
    alignas(16) glm::mat4 roll;

    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
};

std::vector<float> TexelBufferObject;

GLFWwindow* window;

VkInstance instance;
VkDebugUtilsMessengerEXT debugMessenger;
VkSurfaceKHR surface;

VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
VkDevice device;

VkQueue graphicsQueue;
VkQueue presentQueue;

VkSwapchainKHR swapChain;
std::vector<VkImage> swapChainImages;
VkFormat swapChainImageFormat;
VkExtent2D swapChainExtent;
std::vector<VkImageView> swapChainImageViews;
std::vector<VkFramebuffer> swapChainFramebuffers;

VkRenderPass renderPass;
VkCommandPool commandPool;
std::vector<VkCommandBuffer> commandBuffers;

VkImage colorImage;
VkDeviceMemory colorImageMemory;
VkImageView colorImageView;

VkImage screenImage;
VkDeviceMemory screenImageMemory;
VkImageView screenImageView;
std::vector<VkFramebuffer> screenFramebuffers;

VkImage depthImage;
VkDeviceMemory depthImageMemory;
VkImageView depthImageView;

uint32_t mipLevels;
VkSampler textureSampler;

std::vector<VkSemaphore> imageAvailableSemaphores;
std::vector<VkSemaphore> renderFinishedSemaphores;
std::vector<VkFence> inFlightFences;
std::vector<VkFence> imagesInFlight;
size_t currentFrame = 0;

bool framebufferResized = false;

VkShaderModule createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = 0;
    createInfo.flags = 0;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
                                                        VkDebugUtilsMessageTypeFlagsEXT messageType, 
                                                        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
                                                        void* pUserData) {
    switch (messageSeverity)
    {
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT:
        std::cerr << "ERROR: " << pCallbackData->pMessage << std::endl;
        break;
    
    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT:
        std::cerr << "WARNING: " << pCallbackData->pMessage << std::endl;
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT:
        std::cerr << "INFO: " << pCallbackData->pMessage << std::endl;
        break;

    case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT:
        std::cerr << "VERBOSE: " << pCallbackData->pMessage << std::endl;
        break;
    
    default:
        break;
    }

    return VK_FALSE;
}

void getImageData(VkImage& src, VkDeviceSize& imageSize, VkCommandBuffer& commandbuffer) {
    // 스테깅 버퍼, 스테깅 버퍼 메모리 생성
    // 커맨드 버퍼 할당
    // 커맨드 버퍼 비긴 인포
    // Image2Buffer
    // memcpy

    // Staging buffer 생성
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    VkBufferCreateInfo bufferCreateInfo{};
    bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreateInfo.pNext = NULL;
    bufferCreateInfo.flags = 0;
    bufferCreateInfo.size = imageSize;
    // Image에서 값을 받아온다.
    bufferCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferCreateInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
        throw std::runtime_error("staging Buffer 생성 오류");
    }

    // staging buffer Memory 생성
    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, stagingBuffer, &memReq);

    VkPhysicalDeviceMemoryProperties memProp;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProp);

    int memoryTypeIdx = -1;
    for (int i = 0; i < memProp.memoryTypeCount; i++) {
        if (memReq.memoryTypeBits & (1 << i) && memProp.memoryTypes[i].propertyFlags  & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
            memoryTypeIdx = i;
            break;
        }
    }
    if (memoryTypeIdx == -1)
        throw std::runtime_error("stagingBuffer가 요구하는 메모리 유형을 찾을 수 없음.");

    VkMemoryAllocateInfo memAllocInfo{};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.allocationSize = memReq.size;
    memAllocInfo.memoryTypeIndex = memoryTypeIdx;

    if (vkAllocateMemory(device, &memAllocInfo, nullptr, &stagingBufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("stagingBufferMemory생성 오류");
    }

    vkBindBufferMemory(device, stagingBuffer, stagingBufferMemory, 0);

    // begin commandBuffer
    VkCommandBufferBeginInfo cmdbufBeginInfo{};
    cmdbufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdbufBeginInfo.pNext = nullptr;
    cmdbufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    cmdbufBeginInfo.pInheritanceInfo = nullptr;

    vkBeginCommandBuffer(commandbuffer, &cmdbufBeginInfo);

    // convert Image layout
    VkImageMemoryBarrier imageMemBarrier{};
    imageMemBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemBarrier.pNext = nullptr;
    imageMemBarrier.image = src;
    imageMemBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageMemBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    imageMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageMemBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemBarrier.subresourceRange.baseMipLevel = 0;
    imageMemBarrier.subresourceRange.layerCount = 1;
    imageMemBarrier.subresourceRange.levelCount = 1;

    vkCmdPipelineBarrier(   commandbuffer,
                            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            0,
                            0, nullptr,
                            0, nullptr,
                            1, &imageMemBarrier);

    VkBufferImageCopy bufferImageCopy{};

    bufferImageCopy.bufferOffset = 0;
    bufferImageCopy.bufferRowLength = 0;
    bufferImageCopy.bufferImageHeight = 0;
    bufferImageCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bufferImageCopy.imageSubresource.mipLevel = 0;
    bufferImageCopy.imageSubresource.baseArrayLayer = 0;
    bufferImageCopy.imageSubresource.layerCount = 1;
    bufferImageCopy.imageOffset = { 0,
                                    0,
                                    0};
    bufferImageCopy.imageExtent = { static_cast<unsigned int>(swapChainExtent.width), 
                                    static_cast<unsigned int>(swapChainExtent.height), 
                                    1 };

    vkCmdCopyImageToBuffer(commandbuffer, src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuffer, 1, &bufferImageCopy);

    vkEndCommandBuffer(commandbuffer);

    VkSemaphore transSemaphore;

    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    if (vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &transSemaphore) != VK_SUCCESS) {
        throw std::runtime_error("transSemaphore 생성 실패");
    }

    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_TRANSFER_BIT};

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandbuffer;
    // Graphics Pipeline 내 waitStages에 도달하였을 때 까지 대기
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = &transSemaphore;
    submitInfo.pWaitDstStageMask = waitStages;
    
    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("commandBuffer를 제출하는데 실패.");
    }

    vkQueueWaitIdle(graphicsQueue);
    vkFreeCommandBuffers(device, commandPool, 1, &commandbuffer);

    int* res;
    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    // memcpy(res, data, imageSize);
    vkUnmapMemory(device, stagingBufferMemory);

    vkDestroySemaphore(device, transSemaphore, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
    vkDestroyBuffer(device, stagingBuffer, nullptr);
}

class ColliderBox {
public: 
    float posX, posY, posZ;
    float localPosX, localPosY, localPosZ;
    float sizeX, sizeY, sizeZ;

    void setSize2D(float x, float y, float localX, float localY, float sizeX, float sizeY) {
        this->posX = x;
        this->posY = y;
        this->localPosX = localX;
        this->localPosY = localY;
        this->sizeX = sizeX;
        this->sizeY = sizeY;
    }

    void setSize2D(glm::vec2 center, glm::vec2 scale) {
        this->posX = center.x;
        this->posY = center.y;
        this->localPosX = 0.0f;
        this->localPosY = 0.0f;
        this->sizeX = scale.x;
        this->sizeY = scale.y;
    }

    void setSize3D( float x, float y, float z,
                    float localX, float localY, float localZ,
                    float sizeX, float sizeY, float sizeZ) {
        this->posX = x;
        this->posY = y;
        this->posZ = z;
        this->localPosX = localX;
        this->localPosY = localY;
        this->localPosZ = localZ;
        this->sizeX = sizeX;
        this->sizeY = sizeY;
        this->sizeZ = sizeZ;
    }

    void setSize3D(glm::vec3 center, glm::vec3 local, glm::vec3 scale) {
        this->posX = center.x;
        this->posY = center.y;
        this->posZ = center.z;
        this->localPosX = local.x;
        this->localPosY = local.y;
        this->localPosZ = local.z;
        this->sizeX = scale.x;
        this->sizeY = scale.y;
        this->sizeZ = scale.z;
    }

    bool isCollision2D(ColliderBox* target) {
        bool x, y;

        x = !((posX + sizeX) < (target->posX - target->sizeX) || (posX - sizeX) > (target->posX + target->sizeX));
        y = !((posY + sizeY) < (target->posY - target->sizeY) || (posX - sizeY) > (target->posY + target->sizeY));

        return x && y;
    }

    bool isCollision3D(ColliderBox* target) {
        bool x, y, z;
        float px, py, pz, tx, ty, tz;

        px = posX + localPosX;
        py = posY + localPosY;
        pz = posZ + localPosZ;
        tx = target->posX + target->localPosX; 
        ty = target->posY + target->localPosY; 
        tz = target->posZ + target->localPosZ; 

        x = !((px + sizeX) < (tx - target->sizeX) || (px - sizeX) > (tx + target->sizeX));
        y = !((py + sizeY) < (ty - target->sizeY) || (py - sizeY) > (ty + target->sizeY));
        z = !((pz + sizeZ) < (tz - target->sizeZ) || (pz - sizeZ) > (tz + target->sizeZ));

        return (x && y) && z;
    }
};

// 그래픽스파이프라인 초기 속성 (per GameObject)
struct initParam {
public:
    std::string vertPath;
    std::string fragPath;

    // VK_PRIMITIVE_TOPOLOGY_
    VkPrimitiveTopology topologyMode;
    // VK_POLYGON_
    VkPolygonMode polygonMode;
    // VK_CULL_MODE_
    VkCullModeFlagBits cullMode;
}; // _initParam

class Models {
public:
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

    VkBuffer texelUniformBuffer;
    VkDeviceMemory texelUniformBuffersMemory;
    VkBufferView texelUniformBuffersView;
    VkDeviceSize texelDeviceSize;
    void* texelDataPoint;

    VkBuffer texelVertexBuffer;
    VkDeviceMemory texelVertexBuffersMemory;
    VkBufferView texelVertexBuffersView;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    VkPipeline graphicsPipeline;

    /////////////////////////////////
    std::string Name;

    std::string objectPath;
    std::string texturePath;

    glm::vec3 Position;
    glm::vec3 Rotate;
    glm::vec3 Scale;

    struct initParam _initParam;

    Models(std::string name, std::string objPath, std::string textPath, std::string fragPath = "spv/GameObject/base.spv") {
        this->Name = name;
        
        this->objectPath = objPath;
        this->texturePath = textPath;

        Position = glm::vec3(0.0f);
        Rotate = glm::vec3(0.0f);
        Scale = glm::vec3(1.0f);

        this->_initParam.vertPath = "spv/GameObject/vert.spv";
        this->_initParam.fragPath = fragPath;

        this->_initParam.topologyMode = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        this->_initParam.polygonMode = VK_POLYGON_MODE_FILL;
        this->_initParam.cullMode = VK_CULL_MODE_NONE;
    }

    Models(std::string name, std::string objPath, std::string textPath, glm::vec3 pos, glm::vec3 scale) {
        this->Name = name;

        this->objectPath = objPath;
        this->texturePath = textPath;

        this->Position = pos;
        this->Rotate = glm::vec3(0.0f);
        this->Scale = scale;

        this->_initParam.vertPath = "spv/GameObject/vert.spv";
        this->_initParam.fragPath = "spv/GameObject/base.spv";

        this->_initParam.topologyMode = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        this->_initParam.polygonMode = VK_POLYGON_MODE_FILL;
        this->_initParam.cullMode = VK_CULL_MODE_NONE;
    }

    Models(std::string name, std::string objPath, std::string textPath, glm::vec3 scale) {
        this->Name = name;

        this->objectPath = objPath;
        this->texturePath = textPath;

        Position = glm::vec3(0.0f);
        Rotate = glm::vec3(0.0f);
        this->Scale = scale;

        this->_initParam.vertPath = "spv/GameObject/vert.spv";
        this->_initParam.fragPath = "spv/GameObject/base.spv";

        this->_initParam.topologyMode = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        this->_initParam.polygonMode = VK_POLYGON_MODE_FILL;
        this->_initParam.cullMode = VK_CULL_MODE_NONE;
    }
};

class Transpose {
public:
    glm::vec3 velo;
    glm::vec3 accel;

    void setVelocity(glm::vec3 vel) {
        this->velo = vel;
    }
    void setAccel(glm::vec3 accel) {
        this->accel = accel;
    }

    glm::vec3 getVelocity() {
        return this->velo;
    }
    glm::vec3 getAccel() {
        return this->accel;
    }

    glm::vec3 velBySec(float nanoSec) {
        velo += accel * nanoSec;
        return velo * nanoSec;
    }
};

class GameObject {
public:
    // Union Function
    VkDescriptorSetLayout descriptorSetLayout;

    VkPipelineLayout computePipelineLayout;
    VkPipelineLayout pipelineLayout;

    VkPipeline computesPipeline;

    // standard Function
    std::vector<Models*> models;

    uint32_t Index;

    std::string Name;

    glm::vec3 Position;
    glm::vec3 Rotate;
    glm::vec3 Scale;

    ColliderBox* collider;

    void createDescriptorSetLayout();
    void createComputePipeline();
    void createGraphicsPipeline();
    void createTextureImage();
    void createTextureImageView();
    void loadModel();
    void createVertexBuffer();
    void createIndexBuffer();
    void createUniformBuffers();
    void createTexelUniformBuffers();
    void createDescriptorPool();
    void createDescriptorSets();
    void allocateTexelUniformBuffer();

public:
    GameObject(std::string Name, std::string objectPath, std::string texturePath) {
        this->Name = Name; 

        models.push_back(new Models(Name, objectPath, texturePath));

        this->Position = glm::vec3(0.0f);
        this->Rotate = glm::vec3(0.0f);
        this->Scale = glm::vec3(1.0f);

        this->collider = NULL;
    }

    GameObject(std::string Name, std::string objectPath, std::string texturePath, glm::vec3 Position, glm::vec3 Rotate, glm::vec3 Scale) {
        this->Name = Name; 

        models.push_back(new Models(Name, objectPath, texturePath));

        this->Position = Position;
        this->Rotate = Rotate;
        this->Scale = Scale;

        this->collider = NULL;
    }

    // getter setter
    void setIndex(uint32_t idx)             { this->Index = idx; }
    void setName(std::string name)          { this->Name = name; }
    void setPosition(glm::vec3 pos)         { this->Position = pos; }
    void setRotate(glm::vec3 rot)           { this->Rotate = rot; }
    void setScale(glm::vec3 scale)          { this->Scale = scale; }

    uint32_t getIndex()                     { return Index; }
    std::string getName()                   { return Name; }
    glm::vec3 getPosition()                 { return Position; }
    glm::vec3 getRotate()                   { return Rotate; }
    glm::vec3 getScale()                    { return Scale; }

    // Transpose
    void Move(glm::vec3 vel)                { 
                                                this->Position += vel;
                                                this->collider->posX = this->Position.x;
                                                this->collider->posY = this->Position.y;
                                                this->collider->posZ = this->Position.z;
                                            }

    // append subModel 
    void appendModel(std::string Name, std::string objectPath, std::string texturePath, std::string fragPath = "spv/GameObject/base.spv") {
        models.push_back(new Models(Name, objectPath, texturePath, fragPath));
    }

    void appendModel(std::string Name, std::string objectPath, std::string texturePath, glm::vec3 pos, glm::vec3 scale) {
        models.push_back(new Models(Name, objectPath, texturePath, pos, scale));
    }

    void setCollider(glm::vec3 scale) {
        this->collider = new ColliderBox();

        this->collider->setSize3D(this->Position, glm::vec3(0.0f), scale);
    }

    void setCollider(glm::vec3 localPos, glm::vec3 scale) {
        this->collider = new ColliderBox();
        this->collider->setSize3D(this->Position, localPos, scale);
    }

    bool isCollider(GameObject* go) {
        if (!go->collider)
            return false;

        return this->collider->isCollision3D(go->collider);
    }

    void drawCollider() {
        this->appendModel(  "collider",
                            "models/Effect/collider.obj", 
                            "textures/effect/collider.png", 
                            glm::vec3(this->collider->localPosX , this->collider->localPosY, this->collider->localPosZ),
                            glm::vec3(this->collider->sizeX, this->collider->sizeY, this->collider->sizeZ));
    }

    // find Child
    Models* find(std::string name) {
        for (Models* m : models) {
            if (m->Name == name)
                return m;
        }
        return NULL;
    } 

    void initObject() {
        createDescriptorSetLayout();
        createComputePipeline();
        createGraphicsPipeline();
        createTextureImage();
        createTextureImageView();
        loadModel();
        createVertexBuffer();
        createIndexBuffer();
        createUniformBuffers();
        createTexelUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
    }

    void refresh() {
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

        for (Models* m : models) {
            vkDestroyPipeline(device, m->graphicsPipeline, nullptr);

            for (size_t i = 0; i < swapChainImages.size(); i++) {
                vkDestroyBuffer(device, m->uniformBuffers[i], nullptr);
                vkFreeMemory(device, m->uniformBuffersMemory[i], nullptr);
            }

            vkDestroyBuffer(device, m->texelUniformBuffer, nullptr);
            vkFreeMemory(device, m->texelUniformBuffersMemory, nullptr);
            vkDestroyBufferView(device, m->texelUniformBuffersView, nullptr);

            vkDestroyDescriptorPool(device, m->descriptorPool, nullptr);

            createGraphicsPipeline();
            createUniformBuffers();
            createTexelUniformBuffers();
            createDescriptorPool();
            createDescriptorSets();
        }
    }

    void destroy() {
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
        vkDestroyPipelineLayout(device, computePipelineLayout, nullptr);

        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        vkDestroyPipeline(device, computesPipeline, nullptr);

        for (Models* m : models) {
            vkDestroyPipeline(device, m->graphicsPipeline, nullptr);

            for (size_t i = 0; i < swapChainImages.size(); i++) {
                vkDestroyBuffer(device, m->uniformBuffers[i], nullptr);
                vkFreeMemory(device, m->uniformBuffersMemory[i], nullptr);
            }
            
            vkDestroyBuffer(device, m->texelUniformBuffer, nullptr);
            vkFreeMemory(device, m->texelUniformBuffersMemory, nullptr);
            vkDestroyBufferView(device, m->texelUniformBuffersView, nullptr);

            vkDestroyBuffer(device, m->texelVertexBuffer, nullptr);
            vkFreeMemory(device, m->texelVertexBuffersMemory, nullptr);
            vkDestroyBufferView(device, m->texelVertexBuffersView, nullptr);

            vkDestroyDescriptorPool(device, m->descriptorPool, nullptr);

            vkDestroyImageView(device, m->textureImageView, nullptr);

            vkDestroyImage(device, m->textureImage, nullptr);
            vkFreeMemory(device, m->textureImageMemory, nullptr);

            vkDestroyBuffer(device, m->indexBuffer, nullptr);
            vkFreeMemory(device, m->indexBufferMemory, nullptr);

            vkDestroyBuffer(device, m->vertexBuffer, nullptr);
            vkFreeMemory(device, m->vertexBufferMemory, nullptr);
        }
    }
};

class UI {
public:
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkImageView textureImageView;

    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;

    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    //////////////////////
    uint32_t Index;

    std::string Name;

    bool clickable;

    std::string objectPath;
    std::string texturePath;

    glm::vec3 Position;
    glm::vec3 Rotate;

    glm::vec4 normExtent;
    glm::vec4 Extent;

    struct initParam _initParam;

    void createDescriptorSetLayout();
    void createGraphicsPipeline();
    void createTextureImage();
    void createTextureImageView();
    void createUniformBuffers();
    void loadModel();
    void createVertexBuffer();
    void createIndexBuffer();
    void createDescriptorPool();
    void createDescriptorSets();

public:
    UI(std::string Name) {
        this->Name = Name; 

        this->Position = glm::vec3(0.0f);
        this->Rotate = glm::vec3(0.0f);
        this->Extent = glm::vec4(1.0f);

        this->_initParam.vertPath = "spv/UI/vert.spv";
        this->_initParam.fragPath = "spv/UI/frag.spv";

        this->_initParam.topologyMode = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        this->_initParam.polygonMode = VK_POLYGON_MODE_FILL;
        this->_initParam.cullMode = VK_CULL_MODE_NONE;
    }

    UI(std::string Name, glm::vec3 Position, glm::vec3 Rotate, glm::vec4 extent, glm::vec4 normExtent, bool clickable) {
        this->Name = Name;

        this->clickable = clickable;

        this->Position = Position;
        this->Rotate = Rotate;
        this->Extent = extent;
        this->normExtent = normExtent;

        this->_initParam.vertPath = "spv/UI/vert.spv";
        this->_initParam.fragPath = "spv/UI/frag.spv";

        this->_initParam.topologyMode = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        this->_initParam.polygonMode = VK_POLYGON_MODE_FILL;
        this->_initParam.cullMode = VK_CULL_MODE_NONE;
    }

    UI(std::string Name, glm::vec3 Position, glm::vec3 Rotate, glm::vec4 Extent, bool clickable) {
        glm::vec4 normExtent;

        float hw, hh;
        hw = (WIDTH / 2.0f);
        hh = (HEIGHT / 2.0f);

        normExtent[0] = (Extent[0] - hw) / hw;
        normExtent[1] = (Extent[1] - hh) / hh;
        normExtent[2] = (Extent[2] - hw) / hw;
        normExtent[3] = (Extent[3] - hh) / hh;

        this->Name = Name;

        this->clickable = clickable;

        this->Position = Position;
        this->Rotate = Rotate;
        this->Extent = Extent;
        this->normExtent = normExtent;

        this->_initParam.vertPath = "spv/UI/vert.spv";
        this->_initParam.fragPath = "spv/UI/frag.spv";

        this->_initParam.topologyMode = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        this->_initParam.polygonMode = VK_POLYGON_MODE_FILL;
        this->_initParam.cullMode = VK_CULL_MODE_NONE;
    }

    void setIndex(uint32_t idx)             { this->Index = idx; }
    void setName(std::string name)          { this->Name = name; }
    void setObjectPath(std::string path)    { this->objectPath = path; }
    void setTexturePath(std::string path)   { this->texturePath = path; }
    void setPosition(glm::vec3 pos)         { this->Position = pos; }
    void setRotate(glm::vec3 rot)           { this->Rotate = rot; }
    void setExtent(glm::vec4 ext)           { this->Extent = ext; }
    void setNormExtent(glm::vec4 ext)           { this->normExtent = ext; }

    uint32_t getIndex()                     { return Index; }
    std::string getName()                   { return Name; }
    std::string getObjectPath()             { return objectPath; }
    std::string getTexturePath()            { return texturePath; }
    glm::vec3 getPosition()                 { return Position; }
    glm::vec3 getRotate()                   { return Rotate; }
    glm::vec4 getExtent()                   { return Extent; }
    glm::vec4 getNormExtent()               { return normExtent; }

    void initObject() {
        createDescriptorSetLayout();
        createGraphicsPipeline();
        createTextureImage();
        createTextureImageView();
        createUniformBuffers();
        loadModel();
        createVertexBuffer();
        createIndexBuffer();
        createDescriptorPool();
        createDescriptorSets();
    }

    void refresh() {
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);

        createGraphicsPipeline();
        createUniformBuffers();
        createDescriptorPool();
        createDescriptorSets();
    }

    void destroy() {
        vkDestroyPipeline(device, graphicsPipeline, nullptr);
        vkDestroyPipelineLayout(device, pipelineLayout, nullptr);

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            vkDestroyBuffer(device, uniformBuffers[i], nullptr);
            vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
        }

        vkDestroyImageView(device, textureImageView, nullptr);

        vkDestroyImage(device, textureImage, nullptr);
        vkFreeMemory(device, textureImageMemory, nullptr);

        vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);

        vkDestroyBuffer(device, indexBuffer, nullptr);
        vkFreeMemory(device, indexBufferMemory, nullptr);

        vkDestroyBuffer(device, vertexBuffer, nullptr);
        vkFreeMemory(device, vertexBufferMemory, nullptr);
    }
};

class Text{
    public:
        std::vector<UI*> textUI;
        std::string text;

    void initObject(std::string text, glm::vec3 position) {
        this->text = text;

        textUI.resize(text.size());
        for (int i = 0; i < text.size(); i++) {
            textUI[i] = new UI("", position, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec4(0, 0, 16, 16), false);
        }
    }
};

class Camera {
private:
    uint32_t Index; 

    glm::vec3 Up;

    glm::vec3 CameraPosition;
    glm::vec3 CameraTarget;
    glm::vec3 CameraDirection;
    glm::vec3 cameraFront;
    glm::vec3 cameraUp;

    glm::vec3 Rotation;

public: 
    Camera() {
        cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
        cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);
    }

    Camera(glm::vec3 pos, glm::vec3 rot) {
        cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
        cameraUp    = glm::vec3(0.0f, 1.0f,  0.0f);

        CameraPosition = pos;
        Rotation = rot;
    }

    void setIndex(uint32_t Index)           { this->Index = Index; }
    void setPosition(glm::vec3 pos)         { this->CameraPosition = pos; }
    void setTarget(glm::vec3 target)        { this->CameraTarget = target; }
    void setDirection(glm::vec3 direc)      { this->CameraDirection = direc; }
    void setFront(glm::vec3 front)          { this->cameraFront = front; }
    void setUp(glm::vec3 up)                { this->cameraUp = up; }
    void setRotate(glm::vec3 rot)           { this->Rotation = rot; }

    uint32_t  getIndex()                    { return Index; }
    glm::vec3 getPosition()                 { return CameraPosition; }
    glm::vec3 getTarget()                   { return CameraTarget; }
    glm::vec3 getDirection()                { return CameraDirection; }
    glm::vec3 getFront()                    { return cameraFront; }
    glm::vec3 getUp()                       { return cameraUp; }
    glm::vec3 getRotate()                   { return Rotation; }

    void pitch  (float x)                   { this->Rotation.x += x; }
    void yaw    (float y)                   { this->Rotation.y += y; }
    void roll   (float z)                   { this->Rotation.z += z; }

    void moveX(glm::vec3 delta) {
        CameraPosition += delta * 0.001f; 
    }

    void moveY(float delta) {
        CameraPosition += cameraUp * delta* 0.001f; 
    }

    void moveZ(glm::vec3 delta) {
        CameraPosition += delta * 0.001f; 
    }
};

class Light {
private:
    uint32_t Index; 

    glm::vec3 Position;
    glm::vec3 Rotation;

    glm::vec3 Color;

    // float ambientStrength = 0.3f;
    // float specularStrength = 5.0f;

    // float shininess = 32.0f;

public: 
    Light(glm::vec3 pos, glm::vec3 rot, glm::vec3 col) {
        Position = pos;
        Rotation = rot;
        Color = col;
    }

    void setIndex(uint32_t Index)           { this->Index = Index; }
    void setPosition(glm::vec3 pos)         { this->Position = pos; }
    void setRotation(glm::vec3 rot)         { this->Rotation = rot; }

    uint32_t  getIndex()                    { return Index; }
    glm::vec3 getPosition()                 { return Position; }
    glm::vec3 getRotation()                 { return Rotation; }

    void pitch  (float x)                   { this->Rotation.x += x; }
    void yaw    (float y)                   { this->Rotation.y += y; }
    void roll   (float z)                   { this->Rotation.z += z; }

    void move(glm::vec3 delta) {
        Position += delta * 0.001f; 
    };
    void rotate(glm::vec3 delta) {
        Rotation += delta * 0.001f; 
    };
};

std::vector<GameObject*> gameObjectList;
std::vector<UI*> UIList;
std::vector<Camera*> cameraObejctList;
std::vector<Light*> lightObjectList;

class routine {
public:
    std::chrono::_V2::system_clock::time_point startTime;
    std::chrono::_V2::system_clock::time_point currentTime;
    std::chrono::_V2::system_clock::time_point previousTime;
    // Time Per Routine
    float _TIME, _TIME_PER_UPDATE;

    virtual void Awake() {
        TexelBufferObject.push_back(1.0f);
        TexelBufferObject.push_back(0.0f);
        TexelBufferObject.push_back(0.0f);
        TexelBufferObject.push_back(1.0f);
        TexelBufferObject.push_back(1.0f);
        TexelBufferObject.push_back(0.0f);
        TexelBufferObject.push_back(0.0f);
        TexelBufferObject.push_back(1.0f);
    }

    virtual void Start() {
        startTime = std::chrono::high_resolution_clock::now();
        previousTime = startTime;
    }

    virtual void Update() {
        currentTime = std::chrono::high_resolution_clock::now();

        _TIME = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
        _TIME_PER_UPDATE = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - previousTime).count();

        previousTime = currentTime;
    }

    virtual void PhysicalUpdate() {

    }

    virtual void End() {}
};

namespace input {
        bool mouseLock = false;
        bool keyLock = false;

        double _hX = -1;
        double _hY = -1;
        double _hpX = -1;
        double _hpY = -1;
        double _vX = -1;
        double _vY = -1;
        double _vpX = -1;
        double _vpY = -1;

        bool getKeyDown(int key);
        bool getKey(int key);
        bool getKeyUp(int key);

        bool getMouseButtonDown(int key);
        bool getMouseButton(int key);
        bool getMouseButtonUp(int key);

        void getMousePos(double& w, double& h);
        int getMouseVAxis();
        int getMouseHAxis();
}