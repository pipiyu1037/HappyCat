#include "VKDevice.h"

namespace happycat {

std::unique_ptr<VKDevice> VKDevice::Create(const CreateInfo& info) {
    auto device = std::unique_ptr<VKDevice>(new VKDevice());
    if (!device->Initialize(info)) {
        HC_CORE_ERROR("Failed to create Vulkan device");
        return nullptr;
    }
    return device;
}

VKDevice::~VKDevice() {
    if (m_Device != VK_NULL_HANDLE) {
        vkDestroyDevice(m_Device, nullptr);
        HC_CORE_INFO("Vulkan device destroyed");
    }
}

bool VKDevice::Initialize(const CreateInfo& info) {
    // Create physical device wrapper
    m_PhysicalDevice = VKPhysicalDevice::Create(info.physicalDevice, info.surface);
    if (!m_PhysicalDevice) {
        return false;
    }

    const auto& queueFamilies = m_PhysicalDevice->GetQueueFamilyIndices();

    // Create queues (collect unique queue family indices)
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<u32> uniqueQueueFamilies;

    if (queueFamilies.graphics.has_value()) {
        uniqueQueueFamilies.insert(queueFamilies.graphics.value());
    }
    if (queueFamilies.compute.has_value()) {
        uniqueQueueFamilies.insert(queueFamilies.compute.value());
    }
    if (queueFamilies.transfer.has_value()) {
        uniqueQueueFamilies.insert(queueFamilies.transfer.value());
    }
    if (queueFamilies.present.has_value()) {
        uniqueQueueFamilies.insert(queueFamilies.present.value());
    }

    float queuePriority = 1.0f;
    for (u32 queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    // Device features
    VkPhysicalDeviceFeatures deviceFeatures{};
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.fillModeNonSolid = VK_TRUE;
    deviceFeatures.wideLines = VK_TRUE;

    // Device create info
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<u32>(queueCreateInfos.size());
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<u32>(std::size(DEVICE_EXTENSIONS));
    createInfo.ppEnabledExtensionNames = DEVICE_EXTENSIONS;

    // Enable validation layers for device (same as instance)
    if (info.enableValidation) {
        createInfo.enabledLayerCount = static_cast<u32>(std::size(VALIDATION_LAYERS));
        createInfo.ppEnabledLayerNames = VALIDATION_LAYERS;
    } else {
        createInfo.enabledLayerCount = 0;
    }

    // Create logical device
    VkResult result = vkCreateDevice(info.physicalDevice, &createInfo, nullptr, &m_Device);
    if (result != VK_SUCCESS) {
        HC_CORE_ERROR("Failed to create logical device: {0}", static_cast<i32>(result));
        return false;
    }

    HC_CORE_INFO("Vulkan logical device created");

    // Get queue handles
    if (queueFamilies.graphics.has_value()) {
        m_GraphicsQueue = std::make_unique<VKQueue>(m_Device, queueFamilies.graphics.value(), 0);
    }
    if (queueFamilies.compute.has_value()) {
        m_ComputeQueue = std::make_unique<VKQueue>(m_Device, queueFamilies.compute.value(), 0);
    }
    if (queueFamilies.transfer.has_value()) {
        m_TransferQueue = std::make_unique<VKQueue>(m_Device, queueFamilies.transfer.value(), 0);
    }
    if (queueFamilies.present.has_value()) {
        m_PresentQueue = std::make_unique<VKQueue>(m_Device, queueFamilies.present.value(), 0);
    }

    return true;
}

void VKDevice::WaitIdle() {
    vkDeviceWaitIdle(m_Device);
}

} // namespace happycat
