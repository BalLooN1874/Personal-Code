#pragma once

#include<new>
#include<stdint.h>
#include<assert.h>
#include <mutex>
#include <thread>
// Pool中最多能存储的实例数量, 超出最大数量会导致错误,
// 在Debug版本中会对分配进行检测, 超过最大数量会触发assert
const size_t g_blockSize = 1024;
const size_t g_MaxNumberOfBlock = 1000 * g_blockSize;

//显示创建pool
#define MEM_POOL_CREATE() \
    mem_pool::create()

//显示销毁pool
#define MEM_POOL_DESTORY() \
    mem_pool::destroy()

//POD类型，一切为了效率
#define MEM_POOL_ALLOC(type,size)  \
    static_cast<type*>(mem_pool::allocate_memory(size))

#define MEM_POOL_FREE(T, ptr, size)\
    mem_pool::free_memory(ptr, size)

// 分配用户自定义类型, 用于class, strcut等
#define  MEM_POOL_NEW(type, size) \
    new(mem_pool::allocate_memory(size)) type() 

//销毁用户自定义类型
#define MEM_POOL_DELETE(type, ptr, size) \
   if( ptr ) \
		ptr->~type(); \
   mem_pool::free_memory( ptr , size ) 

class mem_pool
{
    public :
		static void* allocate_memory ( const size_t nSize )
        {
            if ( nSize <= 0 )
                return nullptr;

            //检测pool是否已经分配，防止错误发生
            if ( nullptr == instance()->_memory )
				instance()->create_pool();
            return instance()->allocate_block ( nSize );
        }

		static void free_memory ( void* ptr, const size_t nSize )
        {
            //检测pool是否分配以及待释放的指针是否为空。
            if ( !instance()->_memory || !ptr || nSize <= 0 )
                return;
            // 在Debug模式下检测待释放对方是否是在Pool分配的
            assert ( instance()->contains_pointer( ptr ) && "object not allocated from this pool" );
            //将释放的内存还给pool
			instance()->free_block ( ( st_FreeBlock* ) ptr, nSize );
        }

		static void create()
        {
            if ( nullptr == instance()->_memory )
				instance()->create_pool();
        }

		static void destroy()
        {
            if ( instance()->_memory )
				instance()->destroy_pool();
        }
    private:
        struct st_FreeBlock
        {
            st_FreeBlock* next;
        };

		static mem_pool* instance()
		{
			static mem_pool pool;
			return &pool;
		}

        mem_pool() : _memory ( nullptr ), _head ( nullptr ), _blockSize ( 0 ), _offset ( 0 )
        {
        }

		~mem_pool()
		{
		}

        void create_pool()
        {
            _offset = g_blockSize % sizeof ( void* ) == 0 ?  g_blockSize :  g_blockSize + ( sizeof ( void* ) - 1 );
            _numFreeBlocks = g_MaxNumberOfBlock; // memory pool总的容量
            // __alignof(T)是为了检测T对齐的粒度
            _blockSize = __alignof ( char );
            size_t diff = _blockSize % __alignof ( char );
			if ( diff != 0 )
				_blockSize += __alignof (char) - diff;

            //如果分配的blockSize 比一个指针还小，那么就至少要分配一个指针的大小
            if ( _blockSize < sizeof ( uintptr_t ) )
                _blockSize = sizeof ( uintptr_t );

            _memory = aligned_malloc ( g_MaxNumberOfBlock * _blockSize, __alignof ( char ) );
            // 检测分配内存是否满足内存对齐条件
            assert ( is_aligend ( _memory, __alignof ( char ) ) && "memory not aligend" );
            // 将st_FreeBlock链表头设置为分配的值
            _head = ( st_FreeBlock* ) _memory;
            _head->next = nullptr;
        }

        void* aligned_malloc ( size_t size, size_t alignment )
        {
            // 分配足够的内存
            const size_t pointer_size  =  sizeof ( void* );
            //st_FreeBlock内存对齐需要的内存大小
            const size_t requested_size = size + alignment - 1 + pointer_size;
            // 分配的实际大小
            void* raw = malloc ( requested_size );
            uintptr_t start = ( uintptr_t ) raw + pointer_size;
            void* aligned = ( void* ) ( ( start + alignment - 1 ) & ~ ( alignment - 1 ) );
            * ( void** ) ( ( uintptr_t ) aligned - pointer_size ) = raw;
            // 返回实例对象真正的地址
            return aligned;
        }

        void destroy_pool()
        {
            aligned_free ( _memory );
            _memory = nullptr;
        }

        // 检测一个指针是否是在Pool中分配的, 用于防止错误释放
        bool contains_pointer ( void* ptr )
        {
            return ( uintptr_t ) ptr >= ( uintptr_t ) _memory && ( uintptr_t )  ptr < ( uintptr_t ) _memory + _blockSize * g_MaxNumberOfBlock;
        }
	
        //自定义大小
        st_FreeBlock* allocate_block ( const size_t nSize )
        {
			std::lock_guard<std::mutex> lock( _mtx );
            st_FreeBlock* block  = _head;
            // 这里维护的是空闲节点的数目
            if ( ( _numFreeBlocks -= nSize )  != 0 )
            {
                if ( !_head->next )
                {
                    int offset = nSize % sizeof ( void* ) == 0 ? nSize : nSize + ( sizeof ( void* ) - 1 );
                    _head = ( st_FreeBlock* ) ( ( ( uintptr_t ) _head ) + offset );
                    _head->next = nullptr;
                }
                else
                    _head = _head->next;
            }
            return block;
        }

        void free_block ( st_FreeBlock* block , const size_t nSize )
        {
			std::lock_guard<std::mutex> lock( _mtx );
            // 将内存归还到链表头
            if ( _numFreeBlocks > 0 )
                block->next = _head;

            _head = block;
            int Size = nSize % sizeof ( void* ) == 0 ? nSize : nSize + ( sizeof ( void* ) - 1 );
            _numFreeBlocks += Size;
        }

        void aligned_free ( void* aligned )
        {
            void* raw = * ( void** ) ( ( uintptr_t ) aligned - sizeof ( void* ) );
            free ( raw );
        }

		// 检测分配内存是否满足内存对齐条件
        bool is_aligend ( void* data, size_t aligendment )
        {
            return ( ( uintptr_t ) data & ( aligendment - 1 ) ) == 0;
        }
	private:
		mem_pool( const mem_pool & ) = delete;
		mem_pool & operator = ( const mem_pool & ) = delete;
    private:
        void* _memory;
        st_FreeBlock* _head;
		size_t _numFreeBlocks;
		size_t _blockSize;
		size_t _offset;
		std::mutex _mtx;
};

