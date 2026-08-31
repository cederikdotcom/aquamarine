#include <aquamarine/allocator/Shm.hpp>
#include <aquamarine/backend/Backend.hpp>
#include <aquamarine/allocator/Swapchain.hpp>
#include "FormatUtils.hpp"
#include "Shared.hpp"
#include <drm_fourcc.h>
#include <cstring>
#include <sys/mman.h>
#include <unistd.h>

using namespace Aquamarine;
using namespace Hyprutils::Memory;
#define SP CSharedPointer
#define WP CWeakPointer

static uint32_t bppFromDRMFormat(uint32_t format) {
    switch (format) {
        case DRM_FORMAT_RGB565:
        case DRM_FORMAT_BGR565: return 2;
        default: return 4; // all other supported formats are 32-bit
    }
}

Aquamarine::CShmBuffer::CShmBuffer(const SAllocatorBufferParams& params, Hyprutils::Memory::CWeakPointer<CShmAllocator> allocator_,
                                   Hyprutils::Memory::CSharedPointer<CSwapchain> swapchain) : allocator(allocator_) {
    // a swapchain without an explicit format normally resolves one from the first
    // buffer's dmabuf attrs, which a shm buffer doesn't have: pick a sane default
    attrs.format = params.format == DRM_FORMAT_INVALID ? DRM_FORMAT_ARGB8888 : params.format;

    pixelSize = params.size;
    stride    = (uint32_t)params.size.x * bppFromDRMFormat(attrs.format);
    bufferLen = (uint64_t)stride * (uint64_t)params.size.y;

    fd = memfd_create("aq-shm-buffer", MFD_CLOEXEC | MFD_ALLOW_SEALING);
    if (fd < 0) {
        allocator->backend->log(AQ_LOG_ERROR, std::format("failed to memfd_create a shm buffer: {}", strerror(errno)));
        return;
    }

    int ret = 0;
    do {
        ret = ftruncate(fd, bufferLen);
    } while (ret < 0 && errno == EINTR);

    if (ret < 0) {
        allocator->backend->log(AQ_LOG_ERROR, std::format("failed to ftruncate a shm buffer: {}", strerror(errno)));
        return;
    }

    data = (uint8_t*)mmap(nullptr, bufferLen, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        data = nullptr;
        allocator->backend->log(AQ_LOG_ERROR, "failed to mmap a shm buffer");
        return;
    }

    // zero-filled by ftruncate already; just fill in the attrs
    size         = pixelSize;
    attrs.size   = pixelSize;
    attrs.fd     = fd;
    attrs.stride = stride;
    attrs.offset = 0;

    attrs.success = true;

    allocator->backend->log(AQ_LOG_DEBUG, std::format("SHM: Allocated a new buffer with fd {}, size {} and format {}", fd, attrs.size, fourccToName(attrs.format)));
}

Aquamarine::CShmBuffer::~CShmBuffer() {
    events.destroy.emit();

    TRACE(allocator->backend->log(AQ_LOG_TRACE, std::format("SHM: dropping buffer {}", fd)));

    if (data)
        munmap(data, bufferLen);
    if (fd >= 0)
        close(fd);
}

eBufferCapability Aquamarine::CShmBuffer::caps() {
    return eBufferCapability::BUFFER_CAPABILITY_DATAPTR;
}

eBufferType Aquamarine::CShmBuffer::type() {
    return eBufferType::BUFFER_TYPE_SHM;
}

void Aquamarine::CShmBuffer::update(const Hyprutils::Math::CRegion& damage) {
    ; // nothing to do
}

bool Aquamarine::CShmBuffer::isSynchronous() {
    return true;
}

bool Aquamarine::CShmBuffer::good() {
    return attrs.success && data;
}

SSHMAttrs Aquamarine::CShmBuffer::shm() {
    return attrs;
}

std::tuple<uint8_t*, uint32_t, size_t> Aquamarine::CShmBuffer::beginDataPtr(uint32_t flags) {
    return {data, attrs.format, bufferLen};
}

void Aquamarine::CShmBuffer::endDataPtr() {
    ; // nothing to do
}

Aquamarine::CShmAllocator::~CShmAllocator() {
    ; // nothing to do
}

SP<CShmAllocator> Aquamarine::CShmAllocator::create(Hyprutils::Memory::CWeakPointer<CBackend> backend_) {
    auto a  = SP<CShmAllocator>(new CShmAllocator(backend_));
    a->self = a;

    backend_->log(AQ_LOG_DEBUG, "SHM: created a shm allocator");

    return a;
}

SP<IBuffer> Aquamarine::CShmAllocator::acquire(const SAllocatorBufferParams& params, SP<CSwapchain> swapchain_) {
    if (params.size.x < 1 || params.size.y < 1) {
        backend->log(AQ_LOG_ERROR, std::format("Couldn't allocate a shm buffer with invalid size {}", params.size));
        return nullptr;
    }

    auto buf = SP<IBuffer>(new CShmBuffer(params, self, swapchain_));
    if (!buf->good())
        return nullptr;

    return buf;
}

SP<CBackend> Aquamarine::CShmAllocator::getBackend() {
    return backend.lock();
}

int Aquamarine::CShmAllocator::drmFD() {
    return -1;
}

eAllocatorType Aquamarine::CShmAllocator::type() {
    return eAllocatorType::AQ_ALLOCATOR_TYPE_SHM;
}

Aquamarine::CShmAllocator::CShmAllocator(Hyprutils::Memory::CWeakPointer<CBackend> backend_) : backend(backend_) {
    ; // nothing to do
}
