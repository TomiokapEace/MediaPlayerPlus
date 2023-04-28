#include <VAAPIVulkan.hpp>
#include <VAAPI.hpp>

#include "../qmvk/PhysicalDevice.hpp"
#include "../qmvk/Image.hpp"

#include <vulkan/VulkanInstance.hpp>

#include <QMPlay2Core.hpp>
#include <FFCommon.hpp>
#include <Frame.hpp>

#include <QDebug>

#include <va/va_drmcommon.h>
#include <sys/ioctl.h>
#include <unistd.h>

// libdrm/drm_fourcc.h

#define DRM_FORMAT_MOD_INVALID 0x00ffffffffffffffull
#define DRM_FORMAT_MOD_LINEAR 0ull

// linux/dma-buf.h

struct dma_buf_sync {
    uint64_t flags;
};

#define DMA_BUF_SYNC_READ (1 << 0)
#define DMA_BUF_SYNC_WRITE (2 << 0)
#define DMA_BUF_SYNC_RW (DMA_BUF_SYNC_READ | DMA_BUF_SYNC_WRITE)
#define DMA_BUF_SYNC_START (0 << 2)
#define DMA_BUF_SYNC_END (1 << 2)

#define DMA_BUF_BASE 'b'
#define DMA_BUF_IOCTL_SYNC _IOW(DMA_BUF_BASE, 0, struct dma_buf_sync)

using namespace QmVk;

struct FDCustomData : public MemoryObjectBase::CustomData
{
    ~FDCustomData()
    {
        for (auto &&fd : fds)
            ::close(fd);
    }

    vector<int> fds;
};

VAAPIVulkan::VAAPIVulkan(const shared_ptr<VAAPI> &vaapi)
    : m_vkInstance(static_pointer_cast<Instance>(QMPlay2Core.gpuInstance()))
    , m_vaapi(vaapi)
{
    auto physicalDevice = m_vkInstance->physicalDevice();

    if (!physicalDevice->checkExtensions({
        VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME,
    })) {
        QMPlay2Core.logError("VA-API :: Can't interoperate with Vulkan");
        m_error = true;
        return;
    }

    m_hasDrmFormatModifier = physicalDevice->checkExtensions({
        VK_EXT_IMAGE_DRM_FORMAT_MODIFIER_EXTENSION_NAME,
    });
}
VAAPIVulkan::~VAAPIVulkan()
{}

QString VAAPIVulkan::name() const
{
    return VAAPIWriterName;
}

void VAAPIVulkan::map(Frame &frame)
{
    if (m_error || frame.vulkanImage())
        return;

    lock_guard<mutex> locker(m_mutex);

    const auto format = (frame.pixelFormat() == AV_PIX_FMT_P016)
        ? vk::Format::eG16B16R162Plane420Unorm
        : vk::Format::eG8B8R82Plane420Unorm
    ;

    auto device = m_vkInstance->device();

    if (!m_images.empty())
    {
        auto image = m_images.begin()->second.get();
        if (image->device() != device || image->format() != format)
            m_images.clear();
    }

    if (!device || !frame.isHW())
        return;

    VASurfaceID id = frame.hwData();
    if (m_availableSurfaces.count(id) == 0)
        return;

    int vaField = frame.isInterlaced()
        ? (frame.isTopFieldFirst() != frame.isSecondField())
            ? VA_TOP_FIELD
            : VA_BOTTOM_FIELD
        : VA_FRAME_PICTURE
    ;
    if (!m_vaapi->filterVideo(frame, id, vaField))
        return;

    auto &vkImage = m_images[id];

    if (!vkImage)
    {
        VADRMPRIMESurfaceDescriptor vaSurfaceDescr = {};
        const bool exported = vaExportSurfaceHandle(
            m_vaapi->VADisp,
            id,
            VA_SURFACE_ATTRIB_MEM_TYPE_DRM_PRIME_2,
            VA_EXPORT_SURFACE_READ_ONLY | VA_EXPORT_SURFACE_SEPARATE_LAYERS,
            &vaSurfaceDescr
        ) == VA_STATUS_SUCCESS;

        if (exported)
        {
            bool isLinear = false;

            MemoryObject::FdDescriptors fdDescriptors(vaSurfaceDescr.num_objects);
            for (uint32_t i = 0; i < vaSurfaceDescr.num_objects; ++i)
            {
                fdDescriptors[i].first = vaSurfaceDescr.objects[i].fd;
                fdDescriptors[i].second = ::lseek(vaSurfaceDescr.objects[i].fd, 0, SEEK_END);
                if (!m_hasDrmFormatModifier && i == 0)
                {
                    const auto drmFormatModifier = vaSurfaceDescr.objects[i].drm_format_modifier;
                    isLinear = (drmFormatModifier == DRM_FORMAT_MOD_LINEAR || drmFormatModifier == DRM_FORMAT_MOD_INVALID);
                }
            }

            vector<vk::DeviceSize> offsets(vaSurfaceDescr.num_layers);
            for (uint32_t i = 0; i < vaSurfaceDescr.num_layers; ++i)
                offsets[i] = vaSurfaceDescr.layers[i].offset[0];

            try
            {
                constexpr auto externalMemoryHandleType = vk::ExternalMemoryHandleTypeFlagBits::eDmaBufEXT;

                vk::ImageDrmFormatModifierExplicitCreateInfoEXT imageDrmFormatModifierExplicitCreateInfo;
                vk::SubresourceLayout drmFormatModifierPlaneLayout;

                auto imageCreateInfoCallback = [&](uint32_t plane, vk::ImageCreateInfo &imageCreateInfo) {
                    if (!m_hasDrmFormatModifier)
                        return;

                    if (plane >= vaSurfaceDescr.num_layers)
                        throw vk::LogicError("Pitches count and planes count missmatch");

                    auto drmFormatModifier = vaSurfaceDescr.objects[min(plane, vaSurfaceDescr.num_objects - 1)].drm_format_modifier;
                    if (drmFormatModifier == DRM_FORMAT_MOD_INVALID)
                        drmFormatModifier = DRM_FORMAT_MOD_LINEAR;

                    imageDrmFormatModifierExplicitCreateInfo.drmFormatModifier = drmFormatModifier;
                    imageDrmFormatModifierExplicitCreateInfo.drmFormatModifierPlaneCount = 1;
                    imageDrmFormatModifierExplicitCreateInfo.pPlaneLayouts = &drmFormatModifierPlaneLayout;
                    imageDrmFormatModifierExplicitCreateInfo.pNext = imageCreateInfo.pNext;

                    drmFormatModifierPlaneLayout.rowPitch = vaSurfaceDescr.layers[plane].pitch[0];

                    imageCreateInfo.tiling = vk::ImageTiling::eDrmFormatModifierEXT;
                    imageCreateInfo.pNext = &imageDrmFormatModifierExplicitCreateInfo;
                };

                vkImage = Image::createExternalImport(
                    device,
                    vk::Extent2D(frame.width(), frame.height()),
                    format,
                    isLinear,
                    externalMemoryHandleType,
                    imageCreateInfoCallback
                );

                auto fdCustomData = make_unique<FDCustomData>();
                for (auto &&fdDescriptor : fdDescriptors)
                    fdCustomData->fds.push_back(::dup(fdDescriptor.first));
                vkImage->setCustomData(move(fdCustomData));

                vkImage->importFD(
                    fdDescriptors,
                    offsets,
                    externalMemoryHandleType
                );
            }
            catch (const vk::Error &e)
            {
                QMPlay2Core.logError(QString("VA-API :: %1").arg(e.what()));
                vkImage.reset();
                for (uint32_t o = 0; o < vaSurfaceDescr.num_objects; ++o)
                    ::close(vaSurfaceDescr.objects[o].fd);
            }
        }

        if (!vkImage)
            m_error = true;
    }

    if (vkImage)
    {
        frame.setVulkanImage(vkImage);
        frame.setOnDestroyFn([vaapi = m_vaapi] {
        });
    }
    else
    {
        m_images.erase(id);
    }
}
void VAAPIVulkan::clear()
{
    lock_guard<mutex> locker(m_mutex);
    m_availableSurfaces.clear();
    m_images.clear();
}

HWInterop::SyncDataPtr VAAPIVulkan::sync(const vector<Frame> &frames, vk::SubmitInfo *submitInfo)
{
    Q_UNUSED(submitInfo)

    for (auto &&frame : frames)
    {
        if (!frame.isHW())
            continue;

        auto image = frame.vulkanImage();
        if (!image)
            continue;

        const auto &fds = image->customData<FDCustomData>()->fds;
        for (auto &&fd : fds)
        {
            const dma_buf_sync sync = {
                DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW
            };
            ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);
        }
        for (auto &&fd : fds)
        {
            const dma_buf_sync sync = {
                DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW
            };
            ioctl(fd, DMA_BUF_IOCTL_SYNC, &sync);
        }
    }

    return nullptr;
}

void VAAPIVulkan::insertAvailableSurface(uintptr_t id)
{
    lock_guard<mutex> locker(m_mutex);
    m_availableSurfaces.insert(id);
}
