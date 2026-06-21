#pragma once

/// @file
/// @brief A thread-local fixed-block memory pool plus a std-conforming Allocator wrapper, used to
/// pool the small, short-lived, per-RTP-packet shared_ptr nodes produced by std::allocate_shared
/// (e.g. allocate_shared<jrtplib::RTPPacket> on the capture path and allocate_shared<RtpBuffer::
/// Packet> in the jitter buffer). Those objects are fixed-size and churn at the packet rate, so a
/// pool removes them from the general allocator's hot path.
///
/// Pooling is active by default. Define HL_RTP_POOL=0 at compile time to make hl::PoolAllocator a
/// transparent passthrough to the global allocator (i.e. allocate_shared behaves like make_shared)
/// for A/B benchmarking without touching the call sites.

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <new>
#include <vector>

#ifndef HL_RTP_POOL
#	define HL_RTP_POOL 1
#endif

namespace hl
{
#if HL_RTP_POOL
	/// @class FixedBlockPool
	/// A process-wide, fixed-block pool with a lock-free thread-local fast path. Identical in
	/// design to the pcpp Layer pool: uniform 256-byte blocks carved from 64 KB chunks, an
	/// intrusive thread-local free list, and a per-block header tag so deallocate() is O(1) and
	/// lock-free for any block (and can tell pooled blocks from the global-allocator fallback used
	/// for oversized requests) regardless of the freeing thread. Uniform block size makes a block
	/// allocated on one thread safe to free on another (it joins the freeing thread's free list).
	class FixedBlockPool
	{
	public:
		/// Usable bytes handed back to the caller from a pooled block. Comfortably covers the
		/// shared_ptr nodes we pool (control block + RTPPacket / RtpBuffer::Packet, ~90-130 bytes).
		static constexpr std::size_t PayloadSize = 240;
		static constexpr std::size_t BlocksPerChunk = 256;

		static void* allocate(std::size_t size)
		{
			if (size > PayloadSize)
			{
				uint8_t* raw = static_cast<uint8_t*>(::operator new(size + HeaderSize));
				tagOf(raw) = TagGlobal;
				return raw + HeaderSize;
			}

			void*& head = freeListHead();
			if (head == nullptr)
				head = registry().refill();

			uint8_t* block = static_cast<uint8_t*>(head);
			head = nextOf(block);
			return block + HeaderSize;
		}

		static void deallocate(void* ptr) noexcept
		{
			if (ptr == nullptr)
				return;

			uint8_t* block = static_cast<uint8_t*>(ptr) - HeaderSize;
			if (tagOf(block) == TagPool)
			{
				void*& head = freeListHead();
				nextOf(block) = head;
				head = block;
			}
			else
			{
				::operator delete(static_cast<void*>(block));
			}
		}

	private:
		static constexpr std::size_t HeaderSize =
		    alignof(std::max_align_t) >= sizeof(uint64_t) ? alignof(std::max_align_t) : sizeof(uint64_t);
		static constexpr std::size_t BlockSize = HeaderSize + PayloadSize;

		static constexpr uint64_t TagPool = 0x504F4F4C52545008ULL;    // "POOLRTP\b"
		static constexpr uint64_t TagGlobal = 0x474C4F42524C0808ULL;  // "GLOBRL\b\b"

		static uint64_t& tagOf(void* block) noexcept
		{
			return *reinterpret_cast<uint64_t*>(block);
		}

		static void*& nextOf(void* block) noexcept
		{
			return *reinterpret_cast<void**>(static_cast<uint8_t*>(block) + HeaderSize);
		}

		static void*& freeListHead() noexcept
		{
			static thread_local void* head = nullptr;
			return head;
		}

		class ChunkRegistry
		{
		public:
			~ChunkRegistry()
			{
				std::lock_guard<std::mutex> lock(m_Mutex);
				for (uint8_t* chunk : m_Chunks)
					::operator delete(chunk);
				m_Chunks.clear();
			}

			void* refill()
			{
				const std::size_t chunkBytes = BlockSize * BlocksPerChunk;
				uint8_t* chunk = static_cast<uint8_t*>(::operator new(chunkBytes));

				{
					std::lock_guard<std::mutex> lock(m_Mutex);
					m_Chunks.push_back(chunk);
				}

				void* list = nullptr;
				for (std::size_t i = 0; i < BlocksPerChunk; ++i)
				{
					uint8_t* block = chunk + i * BlockSize;
					tagOf(block) = TagPool;
					nextOf(block) = list;
					list = block;
				}
				return list;
			}

		private:
			std::mutex m_Mutex;
			std::vector<uint8_t*> m_Chunks;
		};

		static ChunkRegistry& registry()
		{
			static ChunkRegistry instance;
			return instance;
		}
	};
#endif  // HL_RTP_POOL

	/// @class PoolAllocator
	/// A stateless, std-conforming Allocator suitable for std::allocate_shared. When HL_RTP_POOL is
	/// enabled it serves single-node allocations from FixedBlockPool; otherwise (and for any request
	/// that does not fit a pooled block) it delegates to the global allocator, matching make_shared.
	template <class T> struct PoolAllocator
	{
		using value_type = T;

		PoolAllocator() noexcept = default;
		template <class U> PoolAllocator(const PoolAllocator<U>&) noexcept {}

		T* allocate(std::size_t n)
		{
#if HL_RTP_POOL
			return static_cast<T*>(FixedBlockPool::allocate(n * sizeof(T)));
#else
			return static_cast<T*>(::operator new(n * sizeof(T)));
#endif
		}

		void deallocate(T* p, std::size_t /*n*/) noexcept
		{
#if HL_RTP_POOL
			FixedBlockPool::deallocate(p);
#else
			::operator delete(static_cast<void*>(p));
#endif
		}

		template <class U> bool operator==(const PoolAllocator<U>&) const noexcept { return true; }
		template <class U> bool operator!=(const PoolAllocator<U>&) const noexcept { return false; }
	};
}  // namespace hl
