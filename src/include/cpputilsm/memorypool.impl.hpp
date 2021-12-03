
#ifndef CPPUTILSM_MEMORYPOOL_IMPL_HPP
#define CPPUTILSM_MEMORYPOOL_IMPL_HPP

#ifndef CPPUTILSM_MEMORYPOOL_HPP
#include "memorypool.hpp"
#endif




namespace cpputilsm{

template <size_t BufferSize>
MemoryPool<BufferSize>::MemoryPool()
{
	MemoryPoolBase::Init(m_vBuffer,BufferSize);
}


}  // namespace cpputilsm{


#endif  // #ifndef CPPUTILSM_MEMORYPOOL_IMPL_HPP
