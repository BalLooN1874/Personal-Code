#pragma once

#include<new>
#include<stdint.h>
#include<assert.h>
#include <mutex>
#include <thread>
// Pool������ܴ洢��ʵ������, ������������ᵼ�´���,
// ��Debug�汾�л�Է�����м��, ������������ᴥ��assert
const size_t g_blockSize = 1024;
const size_t g_MaxNumberOfBlock = 1000 * g_blockSize;

//��ʾ����pool
#define MEM_POOL_CREATE() \
    mem_pool::create()

//��ʾ����pool
#define MEM_POOL_DESTORY() \
    mem_pool::destroy()

//POD���ͣ�һ��Ϊ��Ч��
#define MEM_POOL_ALLOC(type,size)  \
    static_cast<type*>(mem_pool::allocate_memory(size))

#define MEM_POOL_FREE(T, ptr, size)\
    mem_pool::free_memory(ptr, size)

// �����û��Զ�������, ����class, strcut��
#define  MEM_POOL_NEW(type, size) \
    new(mem_pool::allocate_memory(size)) type() 

//�����û��Զ�������
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

            //���pool�Ƿ��Ѿ����䣬��ֹ������
            if ( nullptr == instance()->_memory )
				instance()->create_pool();
            return instance()->allocate_block ( nSize );
        }

		static void free_memory ( void* ptr, const size_t nSize )
        {
            //���pool�Ƿ�����Լ����ͷŵ�ָ���Ƿ�Ϊ�ա�
            if ( !instance()->_memory || !ptr || nSize <= 0 )
                return;
            // ��Debugģʽ�¼����ͷŶԷ��Ƿ�����Pool�����
            assert ( instance()->contains_pointer( ptr ) && "object not allocated from this pool" );
            //���ͷŵ��ڴ滹��pool
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
            _numFreeBlocks = g_MaxNumberOfBlock; // memory pool�ܵ�����
            // __alignof(T)��Ϊ�˼��T���������
            _blockSize = __alignof ( char );
            size_t diff = _blockSize % __alignof ( char );
			if ( diff != 0 )
				_blockSize += __alignof (char) - diff;

            //��������blockSize ��һ��ָ�뻹С����ô������Ҫ����һ��ָ��Ĵ�С
            if ( _blockSize < sizeof ( uintptr_t ) )
                _blockSize = sizeof ( uintptr_t );

            _memory = aligned_malloc ( g_MaxNumberOfBlock * _blockSize, __alignof ( char ) );
            // �������ڴ��Ƿ������ڴ��������
            assert ( is_aligend ( _memory, __alignof ( char ) ) && "memory not aligend" );
            // ��st_FreeBlock����ͷ����Ϊ�����ֵ
            _head = ( st_FreeBlock* ) _memory;
            _head->next = nullptr;
        }

        void* aligned_malloc ( size_t size, size_t alignment )
        {
            // �����㹻���ڴ�
            const size_t pointer_size  =  sizeof ( void* );
            //st_FreeBlock�ڴ������Ҫ���ڴ��С
            const size_t requested_size = size + alignment - 1 + pointer_size;
            // �����ʵ�ʴ�С
            void* raw = malloc ( requested_size );
            uintptr_t start = ( uintptr_t ) raw + pointer_size;
            void* aligned = ( void* ) ( ( start + alignment - 1 ) & ~ ( alignment - 1 ) );
            * ( void** ) ( ( uintptr_t ) aligned - pointer_size ) = raw;
            // ����ʵ�����������ĵ�ַ
            return aligned;
        }

        void destroy_pool()
        {
            aligned_free ( _memory );
            _memory = nullptr;
        }

        // ���һ��ָ���Ƿ�����Pool�з����, ���ڷ�ֹ�����ͷ�
        bool contains_pointer ( void* ptr )
        {
            return ( uintptr_t ) ptr >= ( uintptr_t ) _memory && ( uintptr_t )  ptr < ( uintptr_t ) _memory + _blockSize * g_MaxNumberOfBlock;
        }
	
        //�Զ����С
        st_FreeBlock* allocate_block ( const size_t nSize )
        {
			std::lock_guard<std::mutex> lock( _mtx );
            st_FreeBlock* block  = _head;
            // ����ά�����ǿ��нڵ����Ŀ
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
            // ���ڴ�黹������ͷ
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

		// �������ڴ��Ƿ������ڴ��������
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

