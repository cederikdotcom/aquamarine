#pragma once

#include "Allocator.hpp"

namespace Aquamarine {
    class CShmAllocator;
    class CBackend;
    class CSwapchain;

    class CShmBuffer : public IBuffer {
      public:
        virtual ~CShmBuffer();

        virtual eBufferCapability                      caps();
        virtual eBufferType                            type();
        virtual void                                   update(const Hyprutils::Math::CRegion& damage);
        virtual bool                                   isSynchronous();
        virtual bool                                   good();
        virtual SSHMAttrs                              shm();
        virtual std::tuple<uint8_t*, uint32_t, size_t> beginDataPtr(uint32_t flags);
        virtual void                                   endDataPtr();

      private:
        CShmBuffer(const SAllocatorBufferParams& params, Hyprutils::Memory::CWeakPointer<CShmAllocator> allocator_, Hyprutils::Memory::CSharedPointer<CSwapchain> swapchain);

        Hyprutils::Memory::CWeakPointer<CShmAllocator> allocator;

        //
        Hyprutils::Math::Vector2D pixelSize;
        uint32_t                  stride    = 0;
        uint64_t                  bufferLen = 0;
        uint8_t*                  data      = nullptr;
        int                       fd        = -1;

        //
        SSHMAttrs attrs{.success = false};

        friend class CShmAllocator;
    };

    class CShmAllocator : public IAllocator {
      public:
        ~CShmAllocator();
        static Hyprutils::Memory::CSharedPointer<CShmAllocator> create(Hyprutils::Memory::CWeakPointer<CBackend> backend_);

        virtual Hyprutils::Memory::CSharedPointer<IBuffer>      acquire(const SAllocatorBufferParams& params, Hyprutils::Memory::CSharedPointer<CSwapchain> swapchain_);
        virtual Hyprutils::Memory::CSharedPointer<CBackend>     getBackend();
        virtual int                                             drmFD();
        virtual eAllocatorType                                  type();

        //
        Hyprutils::Memory::CWeakPointer<CShmAllocator> self;

      private:
        CShmAllocator(Hyprutils::Memory::CWeakPointer<CBackend> backend_);

        // a vector purely for tracking (debugging) the buffers and nothing more
        std::vector<Hyprutils::Memory::CWeakPointer<CShmBuffer>> buffers;

        Hyprutils::Memory::CWeakPointer<CBackend>                backend;

        friend class CShmBuffer;
    };
};
