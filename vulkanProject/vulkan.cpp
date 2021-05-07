#include "vulkan.h"

void initWindow() ;
void initVulkan();
void cleanupSwapChain();
void cleanup();
void recreateSwapChain();

void createInstance();
void setupDebugMessenger();
void createSurface();
void pickPhysicalDevice() ;
void createLogicalDevice();
void createSwapChain();
void createImageViews();
void createRenderPass();
void createFramebuffers();
void createCommandPool() ;
void createColorResources();
void createScreenResources();
void createDepthResources();
void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
void createTextureSampler();
void createCommandBuffers();


// Will be deplicated.
void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) ;
void createSyncObjects();

void updateUniformBuffer(uint32_t currentImage, GameObject* gameObject);

void initWindow() {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan", nullptr, nullptr);
    glfwSetFramebufferSizeCallback(window, nullptr);
}

void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSurface();
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createCommandPool();
    createColorResources();
    createScreenResources();
    createDepthResources();
    createFramebuffers();
    createTextureSampler();
    createCommandBuffers();
    createSyncObjects();
}

void cleanupSwapChain() {
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    vkDestroyImageView(device, screenImageView, nullptr);
    vkDestroyImage(device, screenImage, nullptr);
    vkFreeMemory(device, screenImageMemory, nullptr);

    vkDestroyImageView(device, colorImageView, nullptr);
    vkDestroyImage(device, colorImage, nullptr);
    vkFreeMemory(device, colorImageMemory, nullptr);

    vkDestroyRenderPass(device, renderPass, nullptr);

    for (auto framebuffer : screenFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }

    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void cleanup() {
    cleanupSwapChain();

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroySampler(device, textureSampler, nullptr);

    vkFreeCommandBuffers(device, commandPool, static_cast<uint32_t>(commandBuffers.size()), commandBuffers.data());
    vkDestroyCommandPool(device, commandPool, nullptr);

    vkDestroyDevice(device, nullptr);

    if (enableValidationLayers) {
        PFN_vkDestroyDebugUtilsMessengerEXT destroyDebugUtilsMessenger;
        destroyDebugUtilsMessenger = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

        destroyDebugUtilsMessenger(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
}

void recreateSwapChain() {
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createRenderPass();
    createColorResources();
    createScreenResources();
    createDepthResources();
    createFramebuffers();
    createCommandBuffers();

    imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);
}

void createInstance() {
    if (!enableValidationLayers) {
        throw std::runtime_error("validation layers requested, but not available!");
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
    createInfo.ppEnabledExtensionNames = instanceExtensions.data();

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo;
    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(instanceLayers.size());
        createInfo.ppEnabledLayerNames = instanceLayers.data();

        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.pNext = nullptr;
        debugCreateInfo.flags = 0;
        debugCreateInfo.messageSeverity =   VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
        debugCreateInfo.messageType =   VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT |
                                        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        debugCreateInfo.pfnUserCallback = debugCallback;
        debugCreateInfo.pUserData = nullptr;

        createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
    } else {
        createInfo.enabledLayerCount = 0;

        createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
        throw std::runtime_error("failed to create instance!");
    }
}

void setupDebugMessenger() {
    if (!enableValidationLayers) return;

    PFN_vkCreateDebugUtilsMessengerEXT createDebugUtilsMessengerEXT;
    createDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.messageSeverity =    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT;
    createInfo.messageType =    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;

    if (createDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger!");
    }
}

void createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
}

void pickPhysicalDevice() {
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    physicalDevice = devices[0];

    if (physicalDevice == VK_NULL_HANDLE) {
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    VkPhysicalDeviceProperties physicalDeviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

    VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
    if (counts & VK_SAMPLE_COUNT_64_BIT) { msaaSamples = VK_SAMPLE_COUNT_64_BIT; }
    if (counts & VK_SAMPLE_COUNT_32_BIT) { msaaSamples = VK_SAMPLE_COUNT_32_BIT; }
    if (counts & VK_SAMPLE_COUNT_16_BIT) { msaaSamples = VK_SAMPLE_COUNT_16_BIT; }
    if (counts & VK_SAMPLE_COUNT_8_BIT)  { msaaSamples = VK_SAMPLE_COUNT_8_BIT;  }
    if (counts & VK_SAMPLE_COUNT_4_BIT)  { msaaSamples = VK_SAMPLE_COUNT_4_BIT;  }
    if (counts & VK_SAMPLE_COUNT_2_BIT)  { msaaSamples = VK_SAMPLE_COUNT_2_BIT;  }
    else                                 { msaaSamples = VK_SAMPLE_COUNT_1_BIT;  }
}

void createLogicalDevice() {
    QueueFamilyIndices indices;

    std::vector<VkQueueFamilyProperties> qFamilyProp;
    uint32_t qFamilyNum;

    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qFamilyNum, nullptr);
    if (!qFamilyNum)
        throw std::runtime_error("사용가능한 패밀리 큐가 존재하지 않음.");
    qFamilyProp.resize(qFamilyNum);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qFamilyNum, qFamilyProp.data());

    VkBool32 isSupportedSurface = VK_FALSE;
    int i = 0;
    for (auto prop : qFamilyProp) {
        if (prop.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &isSupportedSurface);
        if  (isSupportedSurface) {
            indices.presentFamily = i;
        }
        if (indices.isComplete())
            break;
        i++;
    }

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    float queuePriority = 1.0f;
    for (uint32_t queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;

    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos = queueCreateInfos.data();

    createInfo.pEnabledFeatures = &deviceFeatures;

    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(deviceLayers.size());
        createInfo.ppEnabledLayerNames = deviceLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
        throw std::runtime_error("failed to create logical device!");
    }

    vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
    vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
}

void createSwapChain() {
    SwapChainSupportDetails swapChainSupport;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &swapChainSupport.capabilities);
    
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);
    if (!formatCount)
        throw std::runtime_error("스왑체인을 서포트 하지 않는 물리디바이스.");
    swapChainSupport.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, swapChainSupport.formats.data());

    uint32_t modeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &modeCount, nullptr);
    if (!modeCount)
        throw std::runtime_error("스왑체인을 서포트 하지 않는 물리디바이스.");
    swapChainSupport.presentModes.resize(modeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &modeCount, swapChainSupport.presentModes.data());

    VkSurfaceFormatKHR surfaceFormat;
    VkPresentModeKHR presentMode;
    VkExtent2D extent;

    bool hasValue = false;
    for (auto fmt : swapChainSupport.formats) {
        if (fmt.format == VK_FORMAT_B8G8R8A8_SRGB && fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = fmt;
            hasValue = true;
            break;
        }
    }
    if (!hasValue)
        surfaceFormat = swapChainSupport.formats[0];

    hasValue = false;
    for (auto mod : swapChainSupport.presentModes) {
        if (mod == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = mod;
            hasValue = true;
            break;
        }
    }
    if (!hasValue)
        presentMode = VK_PRESENT_MODE_FIFO_KHR;

    extent = swapChainSupport.capabilities.currentExtent;

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
        imageCount = swapChainSupport.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;

    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices indices;

    std::vector<VkQueueFamilyProperties> qFamilyProp;
    uint32_t qFamilyNum;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qFamilyNum, nullptr);
    if (!qFamilyNum)
        throw std::runtime_error("사용 가능한 큐 패밀리가 없음");
    qFamilyProp.resize(qFamilyNum);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qFamilyNum, qFamilyProp.data());

    VkBool32 isSurpportedSurface = VK_FALSE;
    int i = 0;
    for (auto prop : qFamilyProp) {
        if (prop.queueFlags & VK_QUEUE_GRAPHICS_BIT ) {
            indices.graphicsFamily = i;
        }
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &isSurpportedSurface);
        if (isSurpportedSurface) {
            indices.presentFamily = i;
        }
        if (indices.isComplete()) {
            break;
        }
        i++;
    }

    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

    if (indices.graphicsFamily != indices.presentFamily) {
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
        throw std::runtime_error("failed to create swap chain!");
    }

    uint32_t swapChainImageCount;

    vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, nullptr);
    swapChainImages.resize(swapChainImageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &swapChainImageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    for (uint32_t i = 0; i < swapChainImages.size(); i++) {
        VkImageViewCreateInfo createInfo;
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.format = swapChainImageFormat;
        createInfo.image = swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.components = {   VK_COMPONENT_SWIZZLE_R,
                                    VK_COMPONENT_SWIZZLE_G,
                                    VK_COMPONENT_SWIZZLE_B,
                                    VK_COMPONENT_SWIZZLE_A };
        createInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT,
                                        0,
                                        1,
                                        0,
                                        1};

        if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
            throw std::runtime_error("swapchain ImageView를 생성하는데 실패");
        }
    }
}

void createRenderPass() {
    std::vector<VkFormat> depthFormats = {  VK_FORMAT_D32_SFLOAT,
                                            VK_FORMAT_D32_SFLOAT_S8_UINT,
                                            VK_FORMAT_D24_UNORM_S8_UINT};

    VkFormat depthFormat;
    VkFormatProperties formatProp;
    for (auto fmt : depthFormats) {
        vkGetPhysicalDeviceFormatProperties(physicalDevice, fmt, &formatProp);
        if (formatProp.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            depthFormat = fmt;
            break;
        }
    }
    if (!depthFormat) 
        throw std::runtime_error("깊이 메모리의 요구사항을 만족하는 메모리 유형이 없음");
    
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = msaaSamples;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = depthFormat;
    depthAttachment.samples = msaaSamples;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription colorAttachmentResolve{};
    colorAttachmentResolve.format = swapChainImageFormat;
    colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentResolveRef{};
    colorAttachmentResolveRef.attachment = 2;
    colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments = &colorAttachmentResolveRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 3> attachments = {colorAttachment, depthAttachment, colorAttachmentResolve };
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("failed to create render pass!");
    }
}

void GameObject::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding alphaLayoutBinding{};
    alphaLayoutBinding.binding = 2;
    alphaLayoutBinding.descriptorCount = 1;
    alphaLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    alphaLayoutBinding.pImmutableSamplers = nullptr;
    alphaLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding texelBufferBinding{};
    texelBufferBinding.binding = 3;
    texelBufferBinding.descriptorCount = 1;
    texelBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    texelBufferBinding.pImmutableSamplers = nullptr;
    texelBufferBinding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutBinding texelVertexBufferBinding{};
    texelVertexBufferBinding.binding = 4;
    texelVertexBufferBinding.descriptorCount = 1;
    texelVertexBufferBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    texelVertexBufferBinding.pImmutableSamplers = nullptr;
    texelVertexBufferBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    std::array<VkDescriptorSetLayoutBinding, 5> bindings = {    uboLayoutBinding, 
                                                                samplerLayoutBinding, 
                                                                alphaLayoutBinding, 
                                                                texelBufferBinding, 
                                                                texelVertexBufferBinding 
                                                            };
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
    layoutInfo.pBindings = bindings.data();

    for (Models* m : models) {
        if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &m->descriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }
    }
}

void GameObject::createComputePipeline() {
    for (Models* m : models) {
        std::vector<char> compShaderCode = readFile("spv/Compute/exam.spv");
        VkShaderModule compShaderModule = createShaderModule(compShaderCode);

        VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo{};
        pipelineShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        pipelineShaderStageCreateInfo.pNext = 0;
        pipelineShaderStageCreateInfo.flags = 0;
        pipelineShaderStageCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        pipelineShaderStageCreateInfo.module = compShaderModule;
        pipelineShaderStageCreateInfo.pName = "main";
        pipelineShaderStageCreateInfo.pSpecializationInfo = 0;

        VkPushConstantRange pushConstantRange;
        pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(ComputeConstantLayouts);

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo{};
        pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutCreateInfo.pNext = 0;
        pipelineLayoutCreateInfo.flags = 0;
        pipelineLayoutCreateInfo.setLayoutCount = 1;
        pipelineLayoutCreateInfo.pSetLayouts = &m->descriptorSetLayout;
        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &pushConstantRange;

        if ( vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &m->computePipelineLayout) != VK_SUCCESS ) {
            throw std::runtime_error("pipelineLayout 생성 실패");
        }

        VkComputePipelineCreateInfo computePipelineCreateInfo{};
        computePipelineCreateInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        computePipelineCreateInfo.pNext = 0;
        computePipelineCreateInfo.flags = 0;
        computePipelineCreateInfo.stage = pipelineShaderStageCreateInfo;
        computePipelineCreateInfo.layout = m->computePipelineLayout;
        computePipelineCreateInfo.basePipelineHandle = 0;
        computePipelineCreateInfo.basePipelineIndex = 0;

        if ( vkCreateComputePipelines(device, 0, 1, &computePipelineCreateInfo, nullptr, &m->computesPipeline) != VK_SUCCESS) {
            throw std::runtime_error("computePipeline 생성 실패");
        }

        vkDestroyShaderModule(device, compShaderModule, 0);
    }
}

void GameObject::createGraphicsPipeline() {
    for (Models* m : models) {
        std::vector<char> vertShaderCode = readFile(m->_initParam.vertPath);
        std::vector<char> fragShaderCode = readFile(m->_initParam.fragPath);

        VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
        VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        // inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.topology = m->_initParam.topologyMode;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) swapChainExtent.width;
        viewport.height = (float) swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;

        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.pViewports = &viewport;
        viewportState.scissorCount = 1;
        viewportState.pScissors = &scissor;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = m->_initParam.polygonMode;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = m->_initParam.cullMode;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = msaaSamples;

        VkPipelineDepthStencilStateCreateInfo depthStencil{};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        VkPushConstantRange pushConstantRange{};
        pushConstantRange.stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
        pushConstantRange.offset = 0;
        pushConstantRange.size = sizeof(GraphicsConstantLayouts);

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m->descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 1;
        pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &m->pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.layout = m->pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m->graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(device, fragShaderModule, nullptr);
        vkDestroyShaderModule(device, vertShaderModule, nullptr);
    }
}

void createFramebuffers() {
    // swapchainImageView를 destination으로 하는 프레임 버퍼
    swapChainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<VkImageView, 3> attachments = {
            colorImageView,
            depthImageView,
            swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swapchain Framebuffer!");
        }
    }

    // screenImageView를 destination으로 하는 프레임 버퍼
    screenFramebuffers.resize(swapChainImageViews.size());

    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<VkImageView, 3> attachments = {
            colorImageView,
            depthImageView,
            screenImageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &screenFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create screen Framebuffer!");
        }
    }
}

void createCommandPool() {
    QueueFamilyIndices queueFamilyIndices;

    std::vector<VkQueueFamilyProperties> qFamilyProp;
    uint32_t qFamilyNum;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qFamilyNum, nullptr);
    if (!qFamilyNum)
        throw std::runtime_error("사용 가능한 큐 패밀리가 없음");
    qFamilyProp.resize(qFamilyNum);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &qFamilyNum, qFamilyProp.data());

    VkBool32 isSurpportedSurface = VK_FALSE;
    int i = 0;
    for (auto prop : qFamilyProp) {
        if (prop.queueFlags & VK_QUEUE_GRAPHICS_BIT ) {
            queueFamilyIndices.graphicsFamily = i;
        }
        vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &isSurpportedSurface);
        if (isSurpportedSurface) {
            queueFamilyIndices.presentFamily = i;
        }
        if (queueFamilyIndices.isComplete()) {
            break;
        }
        i++;
    }

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

    if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics command pool!");
    }
}

void createColorResources() {
    VkFormat colorFormat = swapChainImageFormat;

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext = nullptr;
    imageCreateInfo.flags = 0;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = colorFormat;
    imageCreateInfo.extent = {swapChainExtent.width, swapChainExtent.height, 1};
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = msaaSamples;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(device, &imageCreateInfo, nullptr, &colorImage) != VK_SUCCESS) {
        throw std::runtime_error("colorImage 생성 실패~");
    } 

    VkPhysicalDeviceMemoryProperties memProp;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProp);

    VkMemoryRequirements memRequir;
    vkGetImageMemoryRequirements(device, colorImage, &memRequir);

    int memTypeIndex = -1;
    for (int i = 0; i < memProp.memoryTypeCount; i++) {
        if (memRequir.memoryTypeBits & (1 << i) && memProp.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
            memTypeIndex = i;
            break;
        }
    }
    if (memTypeIndex == -1)
        throw std::runtime_error("ColorImage에서 요구하는 메모리 유형을 찾을 수 없음");

    VkMemoryAllocateInfo memAllocInfo;
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.pNext = nullptr;
    memAllocInfo.allocationSize = memRequir.size;
    memAllocInfo.memoryTypeIndex = memTypeIndex;

    if (vkAllocateMemory(device, &memAllocInfo, nullptr, &colorImageMemory) != VK_SUCCESS) {
        throw std::runtime_error("ColorImageMemory 할당 실패!");
    }

    vkBindImageMemory(device, colorImage, colorImageMemory, 0);

    VkImageViewCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.image = colorImage;
    createInfo.format = colorFormat;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.components = {   VK_COMPONENT_SWIZZLE_R,
                                VK_COMPONENT_SWIZZLE_G,
                                VK_COMPONENT_SWIZZLE_B,
                                VK_COMPONENT_SWIZZLE_A };
    createInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT,
                                    0,
                                    1,
                                    0,
                                    1};

    if (vkCreateImageView(device, &createInfo, nullptr, &colorImageView) != VK_SUCCESS) {
        throw std::runtime_error("colorImageView 생성 실패~");
    }
}

void createScreenResources() {
    VkFormat screenFormat = swapChainImageFormat;

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext = nullptr;
    imageCreateInfo.flags = 0;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = screenFormat;
    imageCreateInfo.extent = {swapChainExtent.width, swapChainExtent.height, 1};
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage =  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(device, &imageCreateInfo, nullptr, &screenImage) != VK_SUCCESS) {
        throw std::runtime_error("screenImage 생성 실패~");
    } 

    VkPhysicalDeviceMemoryProperties memProp;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProp);

    VkMemoryRequirements memRequir;
    vkGetImageMemoryRequirements(device, screenImage, &memRequir);

    int memTypeIndex = -1;
    for (int i = 0; i < memProp.memoryTypeCount; i++) {
        if (memRequir.memoryTypeBits & (1 << i) && memProp.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) {
            memTypeIndex = i;
            break;
        }
    }
    if (memTypeIndex == -1)
        throw std::runtime_error("ScreenImage에서 요구하는 메모리 유형을 찾을 수 없음");

    VkMemoryAllocateInfo memAllocInfo;
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.pNext = nullptr;
    memAllocInfo.allocationSize = memRequir.size;
    memAllocInfo.memoryTypeIndex = memTypeIndex;

    if (vkAllocateMemory(device, &memAllocInfo, nullptr, &screenImageMemory) != VK_SUCCESS) {
        throw std::runtime_error("ScreenImageMemory 할당 실패!");
    }

    vkBindImageMemory(device, screenImage, screenImageMemory, 0);

    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.image = screenImage;
    createInfo.format = screenFormat;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.components = {   VK_COMPONENT_SWIZZLE_R,
                                VK_COMPONENT_SWIZZLE_G,
                                VK_COMPONENT_SWIZZLE_B,
                                VK_COMPONENT_SWIZZLE_A };
    createInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT,
                                    0,
                                    1,
                                    0,
                                    1};

    if (vkCreateImageView(device, &createInfo, nullptr, &screenImageView) != VK_SUCCESS) {
        throw std::runtime_error("ScreenImageView 생성 실패~");
    }
}

void createDepthResources() {
    VkFormat depthFormat;
    std::vector<VkFormat> depthFormats = {  VK_FORMAT_D32_SFLOAT,
                                            VK_FORMAT_D32_SFLOAT_S8_UINT,
                                            VK_FORMAT_D24_UNORM_S8_UINT};

    bool hasValue = false;
    VkFormatProperties formatProp;
    for (auto fmt : depthFormats) {
        vkGetPhysicalDeviceFormatProperties(physicalDevice, fmt, &formatProp);
        if (formatProp.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT == VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) {
            depthFormat = fmt;
            hasValue = true;
            break;
        }
    }
    if (!hasValue) 
        throw std::runtime_error("깊이 메모리의 요구사항을 만족하는 메모리 유형이 없음");

    VkImageCreateInfo imageCreateInfo{};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext = nullptr;
    imageCreateInfo.flags = 0;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = depthFormat;
    imageCreateInfo.extent = {swapChainExtent.width, swapChainExtent.height, 1};
    imageCreateInfo.mipLevels = 1;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = msaaSamples;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(device, &imageCreateInfo, nullptr, &depthImage) != VK_SUCCESS) {
        throw std::runtime_error("depthImage 생성 실패~");
    } 

    VkMemoryRequirements memRequir;
    vkGetImageMemoryRequirements(device, depthImage, &memRequir);

    VkPhysicalDeviceMemoryProperties memProp;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProp);

    int memTypeIdx = -1;
    for (int i = 0; i < memProp.memoryTypeCount; i++) {
        if (memRequir.memoryTypeBits & (1 << i) && memProp.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
            memTypeIdx = i;
            break;
        }
    }
    if (memTypeIdx == -1) 
        throw std::runtime_error("depthImageMemory에서 요구하는 메모리 유형을 찾을 수 없음.");

    VkMemoryAllocateInfo memAllocInfo{};
    memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAllocInfo.pNext = nullptr;
    memAllocInfo.memoryTypeIndex = memTypeIdx;
    memAllocInfo.allocationSize = memRequir.size;

    if (vkAllocateMemory(device, &memAllocInfo, nullptr, &depthImageMemory) != VK_SUCCESS) {
        throw std::runtime_error("depthImageMemory 할당 실패.");
    }    

    vkBindImageMemory(device, depthImage, depthImageMemory, 0);

    VkImageViewCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.image = depthImage;
    createInfo.format = depthFormat;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.components = {   VK_COMPONENT_SWIZZLE_R,
                                VK_COMPONENT_SWIZZLE_G,
                                VK_COMPONENT_SWIZZLE_B,
                                VK_COMPONENT_SWIZZLE_A };
    createInfo.subresourceRange = { VK_IMAGE_ASPECT_DEPTH_BIT,
                                    0,
                                    1,
                                    0,
                                    1};

    if (vkCreateImageView(device, &createInfo, nullptr, &depthImageView) != VK_SUCCESS) {
        throw std::runtime_error("depthImageView 생성 실패~");
    }
}

void GameObject::createTextureImage() {
    for (Models* m : models) {
        int texWidth, texHeight, texChannels;
        stbi_uc* pixels = stbi_load(m->texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = texWidth * texHeight * 4;
        mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
            memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(device, stagingBufferMemory);

        stbi_image_free(pixels);

        VkImageCreateInfo imageCreateInfo;
        imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCreateInfo.pNext = nullptr;
        imageCreateInfo.flags = 0;
        imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
        imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        imageCreateInfo.extent = {static_cast<unsigned int>(texWidth), static_cast<unsigned int>(texHeight), 1};
        imageCreateInfo.mipLevels = mipLevels;
        imageCreateInfo.arrayLayers = 1;
        imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(device, &imageCreateInfo, nullptr, &m->textureImage) != VK_SUCCESS) {
            throw std::runtime_error("textureImage 생성 실패");
        }

        VkMemoryRequirements memRequir;
        vkGetImageMemoryRequirements(device, m->textureImage, &memRequir);

        VkPhysicalDeviceMemoryProperties memProp;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProp);

        int memTypeIdx = -1;
        for (int i = 0; i< memProp.memoryTypeCount; i++) {
            if (memRequir.memoryTypeBits & (1 << i) && memProp.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                memTypeIdx = i;
                break;
            }
        }
        if (memTypeIdx == -1) {
            throw std::runtime_error("textureImageMemory에서 요구하는 유형의 메모리를 찾을 수 없음.");
        }

        VkMemoryAllocateInfo memAlloc;
        memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAlloc.pNext = nullptr;
        memAlloc.memoryTypeIndex = memTypeIdx;
        memAlloc.allocationSize = memRequir.size;

        if (vkAllocateMemory(device, &memAlloc, nullptr, &m->textureImageMemory) != VK_SUCCESS) {
            throw std::runtime_error("textureImageMemory 할당 실패");
        }

        vkBindImageMemory(device, m->textureImage, m->textureImageMemory, 0);

        VkCommandBuffer recordBuffer;

        VkCommandBufferAllocateInfo cmdbufAllocInfo;
        cmdbufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdbufAllocInfo.pNext = nullptr;
        cmdbufAllocInfo.commandPool = commandPool;
        cmdbufAllocInfo.commandBufferCount = 1;
        cmdbufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        vkAllocateCommandBuffers(device, &cmdbufAllocInfo, &recordBuffer);

        VkCommandBufferBeginInfo cmdbufBeginInfo;
        cmdbufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdbufBeginInfo.pNext = nullptr;
        cmdbufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        Vertex();
        vkBeginCommandBuffer(recordBuffer, &cmdbufBeginInfo);

        VkImageMemoryBarrier imageMemoryBarrier{};
        imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        imageMemoryBarrier.image = m->textureImage;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
        imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
        imageMemoryBarrier.subresourceRange.layerCount = 1;
        // 모든 밉맵에 레이아웃을 적용하기 위해 dimension을 주입. 
        imageMemoryBarrier.subresourceRange.levelCount = mipLevels;

        vkCmdPipelineBarrier(   recordBuffer, 
                                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                0,
                                0, nullptr,
                                0, nullptr,
                                1, &imageMemoryBarrier);

        vkEndCommandBuffer(recordBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &recordBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &recordBuffer);

        cmdbufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdbufAllocInfo.pNext = nullptr;
        cmdbufAllocInfo.commandPool = commandPool;
        cmdbufAllocInfo.commandBufferCount = 1;
        cmdbufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        vkAllocateCommandBuffers(device, &cmdbufAllocInfo, &recordBuffer);

        cmdbufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdbufBeginInfo.pNext = nullptr;
        cmdbufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(recordBuffer, &cmdbufBeginInfo);

        VkBufferImageCopy bufImgCopy;
        bufImgCopy.bufferOffset = 0;
        bufImgCopy.bufferRowLength = 0;
        bufImgCopy.bufferImageHeight = 0;
        bufImgCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        bufImgCopy.imageSubresource.mipLevel = 0;
        bufImgCopy.imageSubresource.baseArrayLayer = 0;
        bufImgCopy.imageSubresource.layerCount = 1;
        bufImgCopy.imageOffset = {0, 0, 0};
        bufImgCopy.imageExtent = {  static_cast<unsigned int>(texWidth),
                                    static_cast<unsigned int>(texHeight),
                                    1};

        vkCmdCopyBufferToImage(recordBuffer, stagingBuffer, m->textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufImgCopy);

        vkEndCommandBuffer(recordBuffer);

        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &recordBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &recordBuffer);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);

        generateMipmaps(m->textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);
    }
}

void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
    // Check if image format supports linear blitting
    VkFormatProperties formatProperties;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

    if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
        throw std::runtime_error("texture image format does not support linear blitting!");
    }

    VkCommandBuffer commandBuffer;

    VkCommandBufferAllocateInfo cmdbufAllocInfo{};
    cmdbufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdbufAllocInfo.pNext = nullptr;
    cmdbufAllocInfo.commandPool = commandPool;
    cmdbufAllocInfo.commandBufferCount = 1;
    cmdbufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    vkAllocateCommandBuffers(device, &cmdbufAllocInfo, &commandBuffer);

    VkCommandBufferBeginInfo cmdbufBeginInfo{};
    cmdbufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdbufBeginInfo.pNext = nullptr;
    cmdbufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &cmdbufBeginInfo);

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        VkImageBlit blit{};
        blit.srcOffsets[0] = {0, 0, 0};
        blit.srcOffsets[1] = {mipWidth, mipHeight, 1};
        blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = {0, 0, 0};
        blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
        blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        vkCmdBlitImage(commandBuffer,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &blit,
            VK_FILTER_LINEAR);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(commandBuffer,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
        0, nullptr,
        0, nullptr,
        1, &barrier);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, nullptr);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

void createTextureSampler() {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = static_cast<float>(mipLevels);
    samplerInfo.mipLodBias = 0.0f;

    if (vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler) != VK_SUCCESS) {
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void GameObject::createTextureImageView() {
    VkImageViewCreateInfo createInfo;

    for (Models* m : models) {
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.pNext = nullptr;
        createInfo.flags = 0;
        createInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
        createInfo.image = m->textureImage;
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.components = {   VK_COMPONENT_SWIZZLE_R,
                                    VK_COMPONENT_SWIZZLE_G,
                                    VK_COMPONENT_SWIZZLE_B,
                                    VK_COMPONENT_SWIZZLE_A };
        createInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT,
                                        0,
                                        mipLevels,
                                        0,
                                        1};

        if (vkCreateImageView(device, &createInfo, nullptr, &m->textureImageView) != VK_SUCCESS) {
            throw std::runtime_error("textureImageView 생성 실패");
        }
    }
}

void GameObject::loadModel() {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    for (Models* m : models) {
        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, m->objectPath.c_str())) {
            throw std::runtime_error(warn + err);
        }

        std::unordered_map<Vertex, uint32_t> uniqueVertices{};

        uint32_t size(0);
        glm::vec3 AB(0.0f); 
        glm::vec3 AC(0.0f);  
        glm::vec3 Normal(0.0f);

        for (const auto& shape : shapes) {
            int i = 0; 
            for (const auto& index : shape.mesh.indices) {
                Vertex vertex{};
                vertex.pos = {
                    attrib.vertices[3 * index.vertex_index],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.texCoord = {
                    attrib.texcoords[2 * index.texcoord_index],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(m->vertices.size());
                    m->vertices.push_back(vertex);
                }

                m->indices.push_back(uniqueVertices[vertex]);

                ++i;

                // New Half
                if (!(i % 3)) {
                    size = m->vertices.size();

                    // AB_Vec
                    AB = m->vertices[size - 2].pos - m->vertices[size - 3].pos; 
                    // AC_Vec
                    AC = m->vertices[size - 1].pos - m->vertices[size - 3].pos;  

                    Normal = glm::cross(AB, AC);

                    m->vertices[size - 1].normal = Normal;
                    m->vertices[size - 2].normal = Normal;
                    m->vertices[size - 3].normal = Normal;
                }
            }
        }
    }
}

void GameObject::createVertexBuffer() {
    VkDeviceSize bufferSize;

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    void* data;

    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo cmdbufAllocInfo{};

    for (Models* m : models) {
        bufferSize = sizeof(m->vertices[0]) * m->vertices.size();

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, m->vertices.data(), (size_t) bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m->vertexBuffer, m->vertexBufferMemory);

        cmdbufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdbufAllocInfo.pNext = nullptr;
        cmdbufAllocInfo.commandPool = commandPool;
        cmdbufAllocInfo.commandBufferCount = 1;
        cmdbufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        vkAllocateCommandBuffers(device, &cmdbufAllocInfo, &commandBuffer);

        VkCommandBufferBeginInfo cmdbufBeginInfo{};
        cmdbufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdbufBeginInfo.pNext = nullptr;
        cmdbufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &cmdbufBeginInfo);

        VkBufferCopy copyRegion{};
        copyRegion.size = bufferSize;
        vkCmdCopyBuffer(commandBuffer, stagingBuffer, m->vertexBuffer, 1, &copyRegion);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, nullptr);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }
}

void GameObject::createIndexBuffer() {
    for (Models* m : models) {
        VkDeviceSize bufferSize = sizeof(m->indices[0]) * m->indices.size();

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, m->indices.data(), (size_t) bufferSize);
        vkUnmapMemory(device, stagingBufferMemory);

        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m->indexBuffer, m->indexBufferMemory);

        VkCommandBuffer commandBuffer;

        VkCommandBufferAllocateInfo cmdbufAllocInfo{};
        cmdbufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdbufAllocInfo.pNext = nullptr;
        cmdbufAllocInfo.commandPool = commandPool;
        cmdbufAllocInfo.commandBufferCount = 1;
        cmdbufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        vkAllocateCommandBuffers(device, &cmdbufAllocInfo, &commandBuffer);

        VkCommandBufferBeginInfo cmdbufBeginInfo{};
        cmdbufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdbufBeginInfo.pNext = nullptr;
        cmdbufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &cmdbufBeginInfo);

        VkBufferCopy copyRegion{};
        copyRegion.size = bufferSize;
        vkCmdCopyBuffer(commandBuffer, stagingBuffer, m->indexBuffer, 1, &copyRegion);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, nullptr);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

        vkDestroyBuffer(device, stagingBuffer, nullptr);
        vkFreeMemory(device, stagingBufferMemory, nullptr);
    }
}

void GameObject::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    for (Models* m : models) {
        m->uniformBuffers.resize(swapChainImages.size());
        m->uniformBuffersMemory.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            createBuffer (  bufferSize, 
                            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                            m->uniformBuffers[i], 
                            m->uniformBuffersMemory[i]);
        }
    }
}
/////////////////////////////////////////////////////////////////////////////////////////
void GameObject::createTexelUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(TexelBufferObject[0]) * TexelBufferObject.size();
    for (Models* m : models) {
        m->texelDeviceSize = bufferSize;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &m->texelUniformBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(device, m->texelUniformBuffer, &memRequirements);

        VkPhysicalDeviceMemoryProperties memProp;
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProp);

        int memTypeIdx = -1;
        for (int i = 0; i < memProp.memoryTypeCount; i++) {
            if (memRequirements.memoryTypeBits & (1 << i) && memProp.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
                memTypeIdx = i;
                break;
            }
        }
        if (memTypeIdx == -1) 
            throw std::runtime_error("요구되는 메모리를 할당하지 못함.");

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = memTypeIdx;

        if (vkAllocateMemory(device, &allocInfo, nullptr, &m->texelUniformBuffersMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(device, m->texelUniformBuffer, m->texelUniformBuffersMemory, 0);

        VkBufferViewCreateInfo bufferViewCreateInfo{};
        bufferViewCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
        bufferViewCreateInfo.pNext = 0;
        bufferViewCreateInfo.flags = 0;
        bufferViewCreateInfo.buffer = m->texelUniformBuffer;
        bufferViewCreateInfo.format = VK_FORMAT_R32G32B32A32_SFLOAT;
        bufferViewCreateInfo.offset = 0;
        bufferViewCreateInfo.range = VK_WHOLE_SIZE;

        if (vkCreateBufferView(device, &bufferViewCreateInfo, nullptr, &m->texelUniformBuffersView) != VK_SUCCESS) {
            throw std::runtime_error("texelUniformBuffers 생성 실패");
        }

        VkBuffer stagingBuffer;
        VkDeviceMemory stagingMemory;

        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(device, &bufferInfo, nullptr, &stagingBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        vkGetBufferMemoryRequirements(device, stagingBuffer, &memRequirements);

        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProp);

        memTypeIdx = -1;
        for (int i = 0; i < memProp.memoryTypeCount; i++) {
            if (memRequirements.memoryTypeBits & (1 << i) && memProp.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT == VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
                memTypeIdx = i;
                break;
            }
        }
        if (memTypeIdx == -1) 
            throw std::runtime_error("요구되는 메모리를 할당하지 못함.");

        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = memTypeIdx;

        if (vkAllocateMemory(device, &allocInfo, nullptr, &stagingMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(device, stagingBuffer, stagingMemory, 0);

        void* data;
        vkMapMemory(device, stagingMemory, 0, bufferSize, 0, &data);
        memcpy(data, TexelBufferObject.data(), bufferSize);

        // stagingBuffer에 copy받은 값 flushing.
        VkMappedMemoryRange memoryRange{};
        memoryRange.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
        memoryRange.pNext = 0;
        memoryRange.offset = 0;
        memoryRange.size = bufferSize;
        memoryRange.memory = stagingMemory;

        vkFlushMappedMemoryRanges(device, 1, &memoryRange);
        vkUnmapMemory(device, stagingMemory);

        VkCommandBuffer recordBuffer;

        VkCommandBufferAllocateInfo cmdbufAllocInfo{};
        cmdbufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdbufAllocInfo.pNext = 0;
        cmdbufAllocInfo.commandPool = commandPool;
        cmdbufAllocInfo.commandBufferCount = 1;
        cmdbufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

        vkAllocateCommandBuffers(device, &cmdbufAllocInfo, &recordBuffer);

        VkCommandBufferBeginInfo cmdbufBeginInfo{};
        cmdbufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        cmdbufBeginInfo.pNext = 0;
        cmdbufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        cmdbufBeginInfo.pInheritanceInfo = 0;

        vkBeginCommandBuffer(recordBuffer, &cmdbufBeginInfo);

        VkBufferMemoryBarrier bufMemBarrier{};
        bufMemBarrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        bufMemBarrier.pNext = 0;
        bufMemBarrier.srcAccessMask = 0;
        bufMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        bufMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        bufMemBarrier.buffer = m->texelUniformBuffer;
        bufMemBarrier.offset = 0;
        bufMemBarrier.size = m->texelDeviceSize;

        vkCmdPipelineBarrier(   recordBuffer, 
                                VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                VK_PIPELINE_STAGE_TRANSFER_BIT,
                                0,
                                0, 0,
                                1, &bufMemBarrier,
                                0, 0);

        VkBufferCopy bufferCopy{};
        bufferCopy.srcOffset = 0;
        bufferCopy.size = m->texelDeviceSize;
        bufferCopy.dstOffset = 0;

        vkCmdCopyBuffer(    recordBuffer, 
                            stagingBuffer,
                            m->texelUniformBuffer,
                            1,
                            &bufferCopy);

        vkEndCommandBuffer(recordBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.pNext = 0;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &recordBuffer;

        vkQueueSubmit(graphicsQueue, 1, &submitInfo, nullptr);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(device, commandPool, 1, &recordBuffer);
        vkFreeMemory(device, stagingMemory, 0);
        vkDestroyBuffer(device, stagingBuffer, 0);
    }
}

void GameObject::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 5> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[2].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    poolSizes[3].type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    poolSizes[3].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    poolSizes[4].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[4].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());

    for (Models* m : models) {
        if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &m->descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }
}

void GameObject::createDescriptorSets() 
{
    for (Models* m : models) {
        std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), m->descriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m->descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
        allocInfo.pSetLayouts = layouts.data();

        m->descriptorSets.resize(swapChainImages.size());
        if (vkAllocateDescriptorSets(device, &allocInfo, m->descriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkDescriptorBufferInfo bufferInfo{};
            bufferInfo.buffer = m->uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            VkDescriptorImageInfo imageInfo{};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = m->textureImageView;
            imageInfo.sampler = textureSampler;

            VkDescriptorImageInfo alphaImageInfo{};
            alphaImageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            alphaImageInfo.imageView = m->textureImageView;
            alphaImageInfo.sampler = textureSampler;

            VkDescriptorBufferInfo texelBufferInfo{};
            texelBufferInfo.buffer = m->texelUniformBuffer;
            texelBufferInfo.offset = 0;
            texelBufferInfo.range = sizeof(TexelBufferObject[0]);

            std::array<VkWriteDescriptorSet, 5> descriptorWrites{};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = m->descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = m->descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[2].dstSet = m->descriptorSets[i];
            descriptorWrites[2].dstBinding = 2;
            descriptorWrites[2].dstArrayElement = 0;
            descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[2].descriptorCount = 1;
            descriptorWrites[2].pImageInfo = &alphaImageInfo;

            descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[3].dstSet = m->descriptorSets[i];
            descriptorWrites[3].dstBinding = 3;
            descriptorWrites[3].dstArrayElement = 0;
            descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
            descriptorWrites[3].descriptorCount = 1;
            descriptorWrites[3].pBufferInfo = &texelBufferInfo;
            descriptorWrites[3].pTexelBufferView = &m->texelUniformBuffersView;

            descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[4].dstSet = m->descriptorSets[i];
            descriptorWrites[4].dstBinding = 4;
            descriptorWrites[4].dstArrayElement = 0;
            descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[4].descriptorCount = 1;
            descriptorWrites[4].pBufferInfo = &texelBufferInfo;

            vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }
    }
}

///////////////////////////////////////////////////
/////////////////      UI      ////////////////////
///////////////////////////////////////////////////

void UI::createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;


    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &this->descriptorSetLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor set layout!");
    }
}

void UI::createGraphicsPipeline() {
    auto vertShaderCode = readFile(this->_initParam.vertPath);
    auto fragShaderCode = readFile(this->_initParam.fragPath);

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = this->_initParam.topologyMode;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapChainExtent.width;
    viewport.height = (float) swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapChainExtent;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = this->_initParam.polygonMode;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = this->_initParam.cullMode;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = msaaSamples;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f;
    colorBlending.blendConstants[1] = 0.0f;
    colorBlending.blendConstants[2] = 0.0f;
    colorBlending.blendConstants[3] = 0.0f;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &this->descriptorSetLayout;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &this->pipelineLayout) != VK_SUCCESS) {
        throw std::runtime_error("failed to create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = this->pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &this->graphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

void UI::createTextureImage() {
    int texWidth, texHeight, texChannels;

    std::string path = "textures/Button.png";

    stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;

    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    stbi_image_free(pixels);

    VkImageCreateInfo imageCreateInfo;
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.pNext = nullptr;
    imageCreateInfo.flags = 0;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreateInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    imageCreateInfo.extent = {static_cast<unsigned int>(texWidth), static_cast<unsigned int>(texHeight), 1};
    imageCreateInfo.mipLevels = mipLevels;
    imageCreateInfo.arrayLayers = 1;
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageCreateInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    if (vkCreateImage(device, &imageCreateInfo, nullptr, &this->textureImage) != VK_SUCCESS) {
        throw std::runtime_error("textureImage 생성 실패");
    }

    VkMemoryRequirements memRequir;
    vkGetImageMemoryRequirements(device, this->textureImage, &memRequir);

    VkPhysicalDeviceMemoryProperties memProp;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProp);

    int memTypeIdx = -1;
    for (int i = 0; i< memProp.memoryTypeCount; i++) {
        if (memRequir.memoryTypeBits & (1 << i) && memProp.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT == VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT) {
            memTypeIdx = i;
            break;
        }
    }
    if (memTypeIdx == -1) {
        throw std::runtime_error("textureImageMemory에서 요구하는 유형의 메모리를 찾을 수 없음.");
    }

    VkMemoryAllocateInfo memAlloc;
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memAlloc.pNext = nullptr;
    memAlloc.memoryTypeIndex = memTypeIdx;
    memAlloc.allocationSize = memRequir.size;

    if (vkAllocateMemory(device, &memAlloc, nullptr, &this->textureImageMemory) != VK_SUCCESS) {
        throw std::runtime_error("textureImageMemory 할당 실패");
    }

    vkBindImageMemory(device, this->textureImage, this->textureImageMemory, 0);

    VkCommandBuffer recordBuffer;

    VkCommandBufferAllocateInfo cmdbufAllocInfo;
    cmdbufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdbufAllocInfo.pNext = nullptr;
    cmdbufAllocInfo.commandPool = commandPool;
    cmdbufAllocInfo.commandBufferCount = 1;
    cmdbufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    vkAllocateCommandBuffers(device, &cmdbufAllocInfo, &recordBuffer);

    VkCommandBufferBeginInfo cmdbufBeginInfo;
    cmdbufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdbufBeginInfo.pNext = nullptr;
    cmdbufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(recordBuffer, &cmdbufBeginInfo);

    VkImageMemoryBarrier imageMemoryBarrier{};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.image = this->textureImage;
    imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;
    imageMemoryBarrier.subresourceRange.layerCount = 1;
    // 모든 밉맵에 레이아웃을 적용하기 위해 dimension을 주입. 
    imageMemoryBarrier.subresourceRange.levelCount = mipLevels;

    vkCmdPipelineBarrier(   recordBuffer, 
                            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 
                            VK_PIPELINE_STAGE_TRANSFER_BIT,
                            0,
                            0, nullptr,
                            0, nullptr,
                            1, &imageMemoryBarrier);

    vkEndCommandBuffer(recordBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &recordBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &recordBuffer);

    cmdbufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdbufAllocInfo.pNext = nullptr;
    cmdbufAllocInfo.commandPool = commandPool;
    cmdbufAllocInfo.commandBufferCount = 1;
    cmdbufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    vkAllocateCommandBuffers(device, &cmdbufAllocInfo, &recordBuffer);

    cmdbufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdbufBeginInfo.pNext = nullptr;
    cmdbufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(recordBuffer, &cmdbufBeginInfo);

    VkBufferImageCopy bufImgCopy;
    bufImgCopy.bufferOffset = 0;
    bufImgCopy.bufferRowLength = 0;
    bufImgCopy.bufferImageHeight = 0;
    bufImgCopy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    bufImgCopy.imageSubresource.mipLevel = 0;
    bufImgCopy.imageSubresource.baseArrayLayer = 0;
    bufImgCopy.imageSubresource.layerCount = 1;
    bufImgCopy.imageOffset = {0, 0, 0};
    bufImgCopy.imageExtent = {  static_cast<unsigned int>(texWidth),
                                static_cast<unsigned int>(texHeight),
                                1};

    vkCmdCopyBufferToImage(recordBuffer, stagingBuffer, this->textureImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &bufImgCopy);

    vkEndCommandBuffer(recordBuffer);

    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &recordBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &recordBuffer);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);

    generateMipmaps(this->textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);
}

void UI::createTextureImageView() {
    VkImageViewCreateInfo createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
    createInfo.image = textureImage;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.components = {   VK_COMPONENT_SWIZZLE_R,
                                VK_COMPONENT_SWIZZLE_G,
                                VK_COMPONENT_SWIZZLE_B,
                                VK_COMPONENT_SWIZZLE_A };
    createInfo.subresourceRange = { VK_IMAGE_ASPECT_COLOR_BIT,
                                    0,
                                    mipLevels,
                                    0,
                                    1};

    if (vkCreateImageView(device, &createInfo, nullptr, &textureImageView) != VK_SUCCESS) {
        throw std::runtime_error("textureImageView 생성 실패");
    }
}

void UI::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(swapChainImages.size());
    uniformBuffersMemory.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        createBuffer(   bufferSize, 
                        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, 
                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
                        uniformBuffers[i], 
                        uniformBuffersMemory[i]);
    }
}

void UI::loadModel() {
    srand(time(NULL));
    std::unordered_map<Vertex, uint32_t> uniqueVertices{};

    float y0(normExtent[0]), x0(normExtent[1]), y1(normExtent[2]), x1(normExtent[3]);

    for (int ind = 0; ind < 1; ind++) {
        for (int i = 0; i < 7; i++) {
            Vertex vertex{};

            switch (i)
            {
            case 0:
                vertex.pos = glm::vec3(y0, x0, 0.0f);
                vertex.texCoord = glm::vec2(0.0f, 0.0f);
                break;

            case 1:
                vertex.pos = glm::vec3(y1, x0, 0.0f);
                vertex.texCoord = glm::vec2(1.0f, 0.0f);
                break;
            
            case 2:
                vertex.pos = glm::vec3(y1, x1, 0.0f);
                vertex.texCoord = glm::vec2(1.0f, 1.0f);
                break;

            case 3:
                vertex.pos = glm::vec3(y0, x0, 0.0f);
                vertex.texCoord = glm::vec2(0.0f, 0.0f);
                break;
            
            case 4:
                vertex.pos = glm::vec3(y0, x1, 0.0f);
                vertex.texCoord = glm::vec2(0.0f, 1.0f);
                break;
            
            case 5:
                vertex.pos = glm::vec3(y1, x1, 0.0f);
                vertex.texCoord = glm::vec2(1.0f, 1.0f);
                break;

            case 6:
                vertex.pos = glm::vec3(y0, x0, 0.0f);
                vertex.texCoord = glm::vec2(0.0f, 0.0f);
                break;
            }

            // 새로운 인스턴스 vertex이면
            if (uniqueVertices.count(vertex) == 0) {
                // 키: vertex 인스턴스 주소, 값: vertices 사이즈 크기
                uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                vertices.push_back(vertex);
            }

            indices.push_back(uniqueVertices[vertex]);
        }
    }
}

void UI::createVertexBuffer() {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, vertices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexBufferMemory);

    VkCommandBuffer commandBuffer;

    VkCommandBufferAllocateInfo cmdbufAllocInfo{};
    cmdbufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdbufAllocInfo.pNext = nullptr;
    cmdbufAllocInfo.commandPool = commandPool;
    cmdbufAllocInfo.commandBufferCount = 1;
    cmdbufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    vkAllocateCommandBuffers(device, &cmdbufAllocInfo, &commandBuffer);

    VkCommandBufferBeginInfo cmdbufBeginInfo{};
    cmdbufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdbufBeginInfo.pNext = nullptr;
    cmdbufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &cmdbufBeginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.size = bufferSize;
    vkCmdCopyBuffer(commandBuffer, stagingBuffer, vertexBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, nullptr);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void UI::createIndexBuffer() {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, indices.data(), (size_t) bufferSize);
    vkUnmapMemory(device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexBufferMemory);

    VkCommandBuffer commandBuffer;

    VkCommandBufferAllocateInfo cmdbufAllocInfo{};
    cmdbufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdbufAllocInfo.pNext = nullptr;
    cmdbufAllocInfo.commandPool = commandPool;
    cmdbufAllocInfo.commandBufferCount = 1;
    cmdbufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    vkAllocateCommandBuffers(device, &cmdbufAllocInfo, &commandBuffer);

    VkCommandBufferBeginInfo cmdbufBeginInfo{};
    cmdbufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdbufBeginInfo.pNext = nullptr;
    cmdbufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &cmdbufBeginInfo);

    VkBufferCopy copyRegion{};
    copyRegion.size = bufferSize;
    vkCmdCopyBuffer(commandBuffer, stagingBuffer, indexBuffer, 1, &copyRegion);

    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = nullptr;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, nullptr);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);

    vkDestroyBuffer(device, stagingBuffer, nullptr);
    vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void UI::createDescriptorPool() {
    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}

void UI::createDescriptorSets() {
    std::vector<VkDescriptorSetLayout> layouts(swapChainImages.size(), descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainImages.size());
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(swapChainImages.size());
    if (vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureImageView;
        imageInfo.sampler = textureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
    }
}

///////////////////////////////////////////////////
/////////////////      ETC      ///////////////////
///////////////////////////////////////////////////

void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
        // texelUniformBuffer -> texelVertexBuffer
    VkCommandBuffer recordBuffer;

    VkCommandBufferAllocateInfo cmdbufAllocInfo{};
    cmdbufAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdbufAllocInfo.pNext = 0;
    cmdbufAllocInfo.commandPool = commandPool;
    cmdbufAllocInfo.commandBufferCount = 1;
    cmdbufAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    vkAllocateCommandBuffers(device, &cmdbufAllocInfo, &recordBuffer) ;

    VkCommandBufferBeginInfo cmdbufBeginInfo{};
    cmdbufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdbufBeginInfo.pNext = 0;
    cmdbufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    cmdbufBeginInfo.pInheritanceInfo = 0;

    vkBeginCommandBuffer(recordBuffer, &cmdbufBeginInfo);

    VkBufferCopy bufferCopy{};
    bufferCopy.srcOffset = 0;
    bufferCopy.size = size;
    bufferCopy.dstOffset = 0;

    vkCmdCopyBuffer(recordBuffer, src, dst, 1, &bufferCopy);

    vkEndCommandBuffer(recordBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = 0;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &recordBuffer;
    
    vkQueueSubmit(graphicsQueue, 1, &submitInfo, nullptr);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, commandPool, 1, &recordBuffer);
}

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        throw std::runtime_error("failed to create buffer!");
    }

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkPhysicalDeviceMemoryProperties memProp;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProp);

    int memTypeIdx = -1;
    for (int i = 0; i < memProp.memoryTypeCount; i++) {
        if (memRequirements.memoryTypeBits & (1 << i) && memProp.memoryTypes[i].propertyFlags & properties == properties) {
            memTypeIdx = i;
            break;
        }
    }
    if (memTypeIdx == -1) 
        throw std::runtime_error("요구되는 메모리를 할당하지 못함.");

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = memTypeIdx;

    if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate buffer memory!");
    }

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void createCommandBuffers() {
    commandBuffers.resize(swapChainFramebuffers.size());

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void createSyncObjects() {
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

glm::mat4 getOrtho() {
    glm::mat4 orthographic_projection_matrix = {
    1.0f,
    0.0f,
    0.0f,
    0.0f,

    0.0f,
    -1.0f,
    0.0f,
    0.0f,

    0.0f,
    0.0f,
    1.0f / (0.1 - 10.0f),
    0.0f,

    0.0f,
    0.0f,
    0.1f / (0.1f - 10.0f),
    1.0f
    };

    return orthographic_projection_matrix;
}

glm::mat4 getPersp() {
    glm::mat4 persp;

    persp = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float) swapChainExtent.height, 0.1f, 300.0f);
    persp[1][1] *= -1;

    return persp;
}

glm::mat4 getTranslate(glm::vec3 position) {
    return glm::translate(glm::mat4(1.0f), position);
}

glm::mat4 getQuart(glm::vec3 rotationAxis, float rotationAngle) {
    glm::vec4 res(0.0f);
    float n;

    res.x = rotationAxis.x * sin(rotationAngle / 2);
    res.y = rotationAxis.y * sin(rotationAngle / 2);
    res.z = rotationAxis.z * sin(rotationAngle / 2);
    res.w = cos(rotationAngle / 2);

    n = 1.0f / sqrt(res.x*res.x + res.y*res.y + res.z*res.z + res.w*res.w);
    res.x *= n;
    res.y *= n;
    res.z *= n;
    res.w *= n;

    glm::mat4 res2Mat (
        1.0f - 2.0f*res.y*res.y - 2.0f*res.z*res.z,
        2.0f*res.x*res.y - 2.0f*res.z*res.w,
        2.0f*res.x*res.z + 2.0f*res.y*res.w, 
        0.0f,

        2.0f*res.x*res.y + 2.0f*res.z*res.w,
        1.0f - 2.0f*res.x*res.x - 2.0f*res.z*res.z, 
        2.0f*res.y*res.z - 2.0f*res.x*res.w, 
        0.0f,

        2.0f*res.x*res.z - 2.0f*res.y*res.w, 
        2.0f*res.y*res.z + 2.0f*res.x*res.w, 
        1.0f - 2.0f*res.x*res.x - 2.0f*res.y*res.y, 
        0.0f,

        0.0f, 
        0.0f, 
        0.0f, 
        1.0f
    );

    return res2Mat;
}

void updateUniformBuffer(uint32_t currentImage, GameObject* gameObject) {
    Camera* cam = cameraObejctList[0];
    Light* light = lightObjectList[0];

    glm::mat4 RotX = glm::rotate(glm::mat4(1.0f), glm::radians(gameObject->Rotate.x), glm::vec3(1, 0, 0));
    glm::mat4 RotY = glm::rotate(glm::mat4(1.0f), glm::radians(gameObject->Rotate.y), glm::vec3(0, 1, 0));
    glm::mat4 RotZ = glm::rotate(glm::mat4(1.0f), glm::radians(gameObject->Rotate.z), glm::vec3(0, 0, 1));

    UniformBufferObject ubo{};
    ubo.model   =   glm::translate(glm::mat4(1.0f), gameObject->getPosition()) 
                    * RotX 
                    * RotY 
                    * RotZ 
                    * glm::scale(glm::mat4(1.0f), gameObject->getScale());

    ubo.pitch   = glm::rotate(glm::mat4(1.0f), glm::radians(cam->getRotate().x), glm::vec3(1.0f, 0.0f, 0.0f));
    ubo.yaw     = glm::rotate(glm::mat4(1.0f), glm::radians(cam->getRotate().y), glm::vec3(0.0f, 1.0f, 0.0f));
    ubo.roll    = glm::rotate(glm::mat4(1.0f), glm::radians(cam->getRotate().z), glm::vec3(0.0f, 0.0f, 1.0f));

    ubo.view    = glm::lookAt(cam->getPosition(), cam->getPosition() + cam->getFront(), glm::vec3(0, 1, 0));
    ubo.proj    = getPersp();

    lightVec = light->getPosition() - gameObject->getPosition();
    lightVec.z *= -1.0f;

    void* data;

    for (Models* m : gameObject->models) {
        vkMapMemory(device, m->uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
        memcpy(data, &ubo, sizeof(ubo));
        vkUnmapMemory(device, m->uniformBuffersMemory[currentImage]);
    }
}

void getBufferData(VkBuffer buffer, VkDeviceSize deviceSize) {
    VkBuffer stagingBuf;
    VkDeviceMemory stagingMem;

    VkBufferCreateInfo bufCreateInfo{};
    bufCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufCreateInfo.pNext = 0;
    bufCreateInfo.flags = 0;
    bufCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    bufCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufCreateInfo.size = deviceSize;

    vkCreateBuffer(device, &bufCreateInfo, 0, &stagingBuf);

    VkPhysicalDeviceMemoryProperties memProp;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProp);

    VkMemoryRequirements memReq;
    vkGetBufferMemoryRequirements(device, stagingBuf, &memReq);

    int memTypeIdx = -1;
    for (int i = 0; i < memProp.memoryTypeCount; i++) {
        if (memReq.memoryTypeBits & (1 << i) && memProp.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT == VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
            memTypeIdx = i;
        }
    }

    if (memTypeIdx == -1) 
        throw std::runtime_error("staging buffer에서 원하는 메모리 유형을 찾을 수 없음.");

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = 0;
    allocInfo.memoryTypeIndex = memTypeIdx;
    allocInfo.allocationSize = memReq.size;

    vkAllocateMemory(device, &allocInfo, 0, &stagingMem);

    vkBindBufferMemory(device, stagingBuf, stagingMem, 0);

    void* data;
    vkMapMemory(device, stagingMem, 0, deviceSize, 0, &data);

    VkCommandBuffer recordBuffer;

    VkCommandBufferAllocateInfo cmdbufallocInfo{};
    cmdbufallocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cmdbufallocInfo.pNext = 0;
    cmdbufallocInfo.commandPool = commandPool;
    cmdbufallocInfo.commandBufferCount = 1;
    cmdbufallocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    vkAllocateCommandBuffers(device, &cmdbufallocInfo, &recordBuffer);

    VkCommandBufferBeginInfo cmdbufBeginInfo{};
    cmdbufBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdbufBeginInfo.pNext = 0;
    cmdbufBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    cmdbufBeginInfo.pInheritanceInfo = 0;

    vkBeginCommandBuffer(recordBuffer, &cmdbufBeginInfo);

    VkBufferCopy bufferCopy{};
    bufferCopy.srcOffset = 0;
    bufferCopy.size = deviceSize;
    bufferCopy.dstOffset = 0;

    vkCmdCopyBuffer(recordBuffer, buffer, stagingBuf, 1, &bufferCopy);

    vkEndCommandBuffer(recordBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.pNext = 0;
    submitInfo.commandBufferCount = 0;
    submitInfo.pCommandBuffers = &recordBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, 0);

    std::vector<float> res(8);
    memcpy(res.data(), &data, deviceSize);

    for (int i = 0; i < 8; i++)
        std::cout << res[i] << ", ";
    std::cout << std::endl;

    vkQueueWaitIdle(graphicsQueue);

    vkUnmapMemory(device, stagingMem);
    vkFreeMemory(device, stagingMem, 0);
    vkDestroyBuffer(device, stagingBuf, 0);
}

void drawFrame() {
    Camera* mainCam = cameraObejctList.at(0);

    // Begin
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(    device, 
                                                swapChain, 
                                                UINT64_MAX, 
                                                imageAvailableSemaphores[currentFrame], 
                                                VK_NULL_HANDLE, 
                                                &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // 이전의 사진이 그려지는 중이나 프레젠테이션 중이면 대기
    if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
    }
    imagesInFlight[imageIndex] = inFlightFences[currentFrame];

    VkCommandBuffer commandBuffer;

    // begin
    commandBuffer = commandBuffers[currentFrame];

    VkCommandBufferBeginInfo cmdbufbeginInfo{};
    cmdbufbeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    cmdbufbeginInfo.pNext = nullptr;
    cmdbufbeginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffers[currentFrame], &cmdbufbeginInfo);

    // GameObject
    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.pNext = nullptr;
    renderPassBeginInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = swapChainExtent;
    renderPassBeginInfo.renderPass = renderPass;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.pClearValues = clearValues.data();

    for (int i = 0; i < 3; i++)
        GraphicsConstantLayouts[0][i] = mainCam->getPosition()[i];
    for (int i = 0; i < 3; i++)
        GraphicsConstantLayouts[1][i] = lightVec[i];

    VkDeviceSize deviceOffset = {0};
    for (GameObject* obj : gameObjectList) {
        for (Models* m : obj->models) {
            // compute pipeline
            vkCmdBindDescriptorSets(    commandBuffer, 
                                        VK_PIPELINE_BIND_POINT_COMPUTE, 
                                        m->computePipelineLayout, 
                                        0, 
                                        1, 
                                        &m->descriptorSets[currentFrame], 
                                        0, 
                                        nullptr);
            vkCmdBindPipeline(  commandBuffer, 
                                VK_PIPELINE_BIND_POINT_COMPUTE, 
                                m->computesPipeline);

            vkCmdPushConstants(commandBuffer, m->computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputeConstantLayouts), &ComputeConstantLayouts );

            vkCmdDispatch( commandBuffer, 2, 1, 1);
        }
    }

    vkCmdBeginRenderPass(   commandBuffer, 
                            &renderPassBeginInfo, 
                            VK_SUBPASS_CONTENTS_INLINE);

    for (GameObject* obj : gameObjectList) {
        for (Models* m : obj->models) {
            // update UBO
            updateUniformBuffer(imageIndex, obj);

            // graphcis pipeline
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m->graphicsPipeline);

            vkCmdPushConstants(commandBuffer, m->pipelineLayout, VK_SHADER_STAGE_ALL_GRAPHICS, 0, sizeof(GraphicsConstantLayouts), &GraphicsConstantLayouts);

            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &m->vertexBuffer, &deviceOffset);
            vkCmdBindIndexBuffer(commandBuffer, m->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m->pipelineLayout, 0, 1, &m->descriptorSets[currentFrame], 0, nullptr);
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m->indices.size()), 1, 0, 0, 0);
        }
    }

    for (UI* obj : UIList) {
        VkDeviceSize deviceOffset = {0};

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, obj->graphicsPipeline);

        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &obj->vertexBuffer, &deviceOffset);
        vkCmdBindIndexBuffer(commandBuffer, obj->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, obj->pipelineLayout, 0, 1, &obj->descriptorSets[currentFrame], 0, nullptr);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(obj->indices.size()), 1, 0, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);

    // CommandBuffer의 레코딩을 끝낸다
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        throw std::runtime_error("커맨드 버퍼의 레코딩에 실패하였습니다!");
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkResetFences(device, 1, &inFlightFences[currentFrame]);

    if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;

    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    vkQueueWaitIdle(presentQueue);
    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

GameObject* createObject(   std::string Name,
                            std::string objectPath, 
                            std::string texturePath, 
                            glm::vec3 Position = glm::vec3(0.0f), 
                            glm::vec3 Rotate = glm::vec3(0.0f), 
                            glm::vec3 Scale = glm::vec3(1.0f), 
                            std::string fragTexture = "spv/GameObject/base.spv") {
    GameObject* newGameObject = new GameObject(Name, objectPath, texturePath, Position, Rotate, Scale);
    newGameObject->models[0]->_initParam.fragPath = fragTexture;

    gameObjectList.push_back(newGameObject);
    newGameObject->setIndex(gameObjectList.size());

    return gameObjectList[newGameObject->getIndex() - 1];
} 


UI* createUI(   std::string Name,
                bool clickable = true,
                glm::vec3 Position = glm::vec3(0.0f),
                glm::vec4 Extent = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), 
                glm::vec3 Rotate = glm::vec3(0.0f)) {
    if (Extent[2] > WIDTH || Extent[3] > HEIGHT) {
        throw std::runtime_error("Extent 범위를 초과 함.");
    }

    glm::vec4 normExtent;

    float hw, hh;
    hw = (WIDTH / 2.0f);
    hh = (HEIGHT / 2.0f);

    normExtent[0] = (Extent[0] - hw) / hw;
    normExtent[1] = (Extent[1] - hh) / hh;
    normExtent[2] = (Extent[2] - hw) / hw;
    normExtent[3] = (Extent[3] - hh) / hh;

    UI* newUIObject = new UI(Name, Position, Rotate, Extent, normExtent, clickable);

    UIList.push_back(newUIObject);
    newUIObject->setIndex(UIList.size());

    return UIList[newUIObject->getIndex() - 1];
} 

Camera* createCamera(glm::vec3 position = glm::vec3(0.0f), glm::vec3 rotation = glm::vec3(0.0f)) {
    Camera* newCamObject = new Camera(position, rotation);

    cameraObejctList.push_back(newCamObject);
    newCamObject->setIndex(cameraObejctList.size() - 1);

    return cameraObejctList[newCamObject->getIndex()];
} 

Light* createLight(glm::vec3 position = glm::vec3(0.0f), glm::vec3 rotation = glm::vec3(0.0f), glm::vec3 color = glm::vec3(1.0f)) {
    Light* newLightObject = new Light(position, rotation, color);

    lightObjectList.push_back(newLightObject);
    newLightObject->setIndex(lightObjectList.size() - 1);

    return lightObjectList[newLightObject->getIndex()];
} 

bool input::getKeyDown(int key) {
    if (!keyLock && glfwGetKey(window, key)){
        keyLock = true;
        return true;
    }
    else if (!glfwGetKey(window, key)) {
        keyLock = false;
    }
    return false;
}
bool input::getKey(int key) {
    if (glfwGetKey(window, key)){
        return true;
    }
    return false;
}
bool input::getKeyUp(int key) {
    if (keyLock && !glfwGetKey(window, key)){
        keyLock = false;
        return true;
    }
    return false;
}

bool input::getMouseButtonDown(int key) {
    if (!mouseLock && glfwGetMouseButton(window, key)){
        mouseLock = true;
        return true;
    }
    else if (!glfwGetMouseButton(window, key)) {
        mouseLock = false;
    }
    return false;
}

bool input::getMouseButton(int key) {
    if (glfwGetMouseButton(window, key))
        return true;
    return false;
}

bool input::getMouseButtonUp(int key) {
    if (mouseLock && !glfwGetMouseButton(window, key)){
        mouseLock = false;
        return true;
    }
    return false;
}

void input::getMousePos(double& w, double& h) {
    glfwGetCursorPos(window, &w, &h);
}
int input::getMouseVAxis() {
    _vpX = _vX;
    _vpY = _vY;
    glfwGetCursorPos(window, &_vX, &_vY);

    if ((_vY - _vpY) > 0.0f) {
        return 1;
    } else if ((_vY - _vpY) < 0.0f) {
        return -1;
    } else {
        return 0;
    }
}
int input::getMouseHAxis() {
    _hpX = _hX;
    _hpY = _hY;
    glfwGetCursorPos(window, &_hX, &_hY);
    
    if ((_hX - _hpX) > 0.0f) {
        return 1;
    } else if ((_hX - _hpX) < 0.0f) {
        return -1;
    } else {
        return 0;
    }
}
