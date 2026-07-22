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

#include <atomic>
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
				s_activeGlobal.fetch_add(1, std::memory_order_relaxed);
				return raw + HeaderSize;
			}

			void*& head = freeListHead();
			if (head == nullptr)
				head = registry().refill();

			uint8_t* block = static_cast<uint8_t*>(head);
			head = nextOf(block);
			s_activeBlocks.fetch_add(1, std::memory_order_relaxed);
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
				s_activeBlocks.fetch_sub(1, std::memory_order_relaxed);
			}
			else
			{
				::operator delete(static_cast<void*>(block));
				s_activeGlobal.fetch_sub(1, std::memory_order_relaxed);
			}
		}

		/// @name Diagnostics (relaxed gauges — not synchronization)
		/// @{
		/// Blocks currently handed out from the pool = pooled allocate() minus
		/// pooled deallocate(). Unlike capacityBlocks() (the chunk high-water,
		/// which only ever grows) this DROPS when objects are freed, so a rising
		/// activeBlocks() is a genuine leak of referenced objects rather than a
		/// traffic peak that merely carved extra chunks. Covers every pooled
		/// shared_ptr node (allocate_shared<RTPPacket> + jitter-buffer packets).
		static std::int64_t activeBlocks() noexcept
		{
			return s_activeBlocks.load(std::memory_order_relaxed);
		}
		/// Live oversized allocations that overflowed to ::operator new.
		static std::int64_t activeGlobalAllocations() noexcept
		{
			return s_activeGlobal.load(std::memory_order_relaxed);
		}
		/// Total blocks ever carved (chunks x BlocksPerChunk) = high-water capacity.
		static std::int64_t capacityBlocks() noexcept
		{
			return s_chunks.load(std::memory_order_relaxed) * static_cast<std::int64_t>(BlocksPerChunk);
		}
		/// Approximate live pooled payload bytes (excludes per-block header).
		static std::int64_t activeBytes() noexcept
		{
			return activeBlocks() * static_cast<std::int64_t>(PayloadSize);
		}
		/// @}

	private:
		inline static std::atomic<std::int64_t> s_activeBlocks{0};
		inline static std::atomic<std::int64_t> s_activeGlobal{0};
		inline static std::atomic<std::int64_t> s_chunks{0};

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
				s_chunks.fetch_add(1, std::memory_order_relaxed);

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

	/// Mode-independent accessors for the pool diagnostics, so callers need not know
	/// whether HL_RTP_POOL is compiled in. When pooling is disabled they return -1
	/// ("not applicable") since allocate_shared then behaves like make_shared.
#if HL_RTP_POOL
	inline std::int64_t poolActiveBlocks()   noexcept { return FixedBlockPool::activeBlocks(); }
	inline std::int64_t poolCapacityBlocks() noexcept { return FixedBlockPool::capacityBlocks(); }
	inline std::int64_t poolActiveGlobal()   noexcept { return FixedBlockPool::activeGlobalAllocations(); }
	inline std::int64_t poolActiveBytes()    noexcept { return FixedBlockPool::activeBytes(); }
#else
	inline std::int64_t poolActiveBlocks()   noexcept { return -1; }
	inline std::int64_t poolCapacityBlocks() noexcept { return -1; }
	inline std::int64_t poolActiveGlobal()   noexcept { return -1; }
	inline std::int64_t poolActiveBytes()    noexcept { return -1; }
#endif

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
